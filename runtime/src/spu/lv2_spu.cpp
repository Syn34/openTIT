// LV2 SPU subsystem: SPU images, thread groups and SPU threads.
//
// Each SPU thread runs the SPU interpreter on its own host thread. The
// SPU-side LV2 ABI (stop-and-signal codes and the interrupt-mailbox event
// protocol used by sys_spu_thread_send_event & co) is implemented in the
// stop / mailbox hooks below.
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <thread>
#include <vector>

#include "../kernel.h"
#include "../log.h"
#include "../vfs.h"
#include "opentit/memory.h"
#include "opentit/spu.h"

namespace opentit::kernel {

namespace {

using namespace rtm;

constexpr u64 SYS_SPU_THREAD_EVENT_USER_KEY = 0xFFFFFFFF53505501ull;
constexpr u32 SYS_SPU_SEGMENT_TYPE_COPY = 1, SYS_SPU_SEGMENT_TYPE_FILL = 2, SYS_SPU_SEGMENT_TYPE_INFO = 4;
constexpr u32 JOIN_GROUP_EXIT = 1, JOIN_ALL_THREADS_EXIT = 2, JOIN_TERMINATED = 4;

u64 A(PPUContext& c, int n) { return c.gpr[3 + n]; }
u32 A32(PPUContext& c, int n) { return static_cast<u32>(c.gpr[3 + n]); }

struct SpuGroup;

struct SpuThreadObj : Object {
    spu::SPUThread spu;
    std::weak_ptr<SpuGroup> group;
    u32 index = 0;
    u32 entry = 0;
    u64 args[4] = {};
    std::shared_ptr<EventQueue> spup[64];      // connected via sys_spu_thread_connect_event
    std::map<u32, std::shared_ptr<EventQueue>> spuq;  // bound via sys_spu_thread_bind_queue
    s32 exit_status = 0;
    bool exited = false;
    u64 cfg = 0;
};

struct SpuGroup : Object {
    std::string name;
    u32 id = 0;
    u32 num = 0;
    s32 prio = 100;
    s32 type = 0;
    std::vector<std::shared_ptr<SpuThreadObj>> threads;
    std::vector<std::thread> hosts;
    std::mutex m;
    std::condition_variable cv;
    bool running = false;
    u32 running_count = 0;
    u32 join_cause = 0;
    s32 exit_status = 0;
};

// ---------------------------------------------------------------- images
// sys_spu_image { u32 type; u32 entry_point; u32 segs; s32 nsegs }
// sys_spu_segment { s32 type; u32 ls; u32 size; u32 addr_or_value }
s32 build_image(u32 img, const u8* elf, std::size_t size) {
    if (size < 52 || std::memcmp(elf, "\x7f" "ELF", 4) != 0 || load_be<u16>(elf + 18) != 23) return -1;
    const u32 phoff = load_be<u32>(elf + 28);
    const u16 phnum = load_be<u16>(elf + 44);
    struct Seg { u32 type, ls, size, addr; };
    std::vector<Seg> segs;
    // Copy the whole ELF into kernel memory so segment addresses stay valid.
    const u32 copy = alloc_guest(static_cast<u32>(size), 128);
    if (!copy) return -1;
    mem::write_bytes(copy, elf, size);
    for (u16 k = 0; k < phnum; ++k) {
        const u8* p = elf + phoff + 32u * k;
        const u32 type = load_be<u32>(p), off = load_be<u32>(p + 4), va = load_be<u32>(p + 8);
        const u32 filesz = load_be<u32>(p + 16), memsz = load_be<u32>(p + 20);
        if (type == 1) {
            if (filesz) segs.push_back({SYS_SPU_SEGMENT_TYPE_COPY, va, filesz, copy + off});
            if (memsz > filesz) segs.push_back({SYS_SPU_SEGMENT_TYPE_FILL, va + filesz, memsz - filesz, 0});
        } else if (type == 4) {
            segs.push_back({SYS_SPU_SEGMENT_TYPE_INFO, va, filesz, copy + off});
        }
    }
    const u32 segs_addr = alloc_guest(static_cast<u32>(16 * std::max<std::size_t>(segs.size(), 1)), 16);
    for (std::size_t k = 0; k < segs.size(); ++k) {
        const u32 s = segs_addr + 16 * static_cast<u32>(k);
        mem::w32(s, segs[k].type);
        mem::w32(s + 4, segs[k].ls);
        mem::w32(s + 8, segs[k].size);
        mem::w32(s + 12, segs[k].addr);
    }
    mem::w32(img, 0);  // SYS_SPU_IMAGE_TYPE_USER
    mem::w32(img + 4, load_be<u32>(elf + 24));
    mem::w32(img + 8, segs_addr);
    mem::w32(img + 12, static_cast<u32>(segs.size()));
    return 0;
}

void apply_image(spu::SPUThread& t, u32 img) {
    const u32 segs = mem::r32(img + 8);
    const s32 nsegs = static_cast<s32>(mem::r32(img + 12));
    for (s32 k = 0; k < nsegs; ++k) {
        const u32 s = segs + 16 * static_cast<u32>(k);
        const u32 type = mem::r32(s), ls = mem::r32(s + 4), size = mem::r32(s + 8), v = mem::r32(s + 12);
        if (ls + size > spu::kLsSize) continue;
        t.touch(ls, size);
        if (type == SYS_SPU_SEGMENT_TYPE_COPY) std::memcpy(t.ls + ls, mem::ptr(v), size);
        else if (type == SYS_SPU_SEGMENT_TYPE_FILL)
            for (u32 o = 0; o + 4 <= size; o += 4) store_be<u32>(t.ls + ls + o, v);
    }
}

// ---------------------------------------------------------------- SPU-side LV2 ABI
bool on_stop(SpuThreadObj& th, u32 code) {
    spu::SPUThread& t = th.spu;
    switch (code) {
    case 0x100:  // sys_spu_thread_yield
        std::this_thread::yield();
        return true;
    case 0x101: {  // sys_spu_thread_group_exit(status in out mbox)
        u32 status = 0;
        t.pop_out_mbox(&status);
        if (auto g = th.group.lock()) {
            std::lock_guard<std::mutex> lk(g->m);
            g->join_cause = JOIN_GROUP_EXIT;
            g->exit_status = static_cast<s32>(status);
            for (auto& o : g->threads) o->spu.kill = true;
        }
        return false;
    }
    case 0x102: {  // sys_spu_thread_exit(status in out mbox)
        u32 status = 0;
        t.pop_out_mbox(&status);
        th.exit_status = static_cast<s32>(status);
        return false;
    }
    case 0x110:    // sys_spu_thread_receive_event(spuq in out mbox)
    case 0x111: {  // sys_spu_thread_tryreceive_event
        u32 spuq = 0;
        t.pop_out_mbox(&spuq);
        auto it = th.spuq.find(spuq);
        if (it == th.spuq.end()) {
            t.push_in_mbox(CELL_EINVAL);
            return true;
        }
        EventQueue& q = *it->second;
        std::unique_lock<std::mutex> lk(q.m);
        if (code == 0x111 && q.events.empty()) {
            lk.unlock();
            t.push_in_mbox(CELL_EBUSY);
            return true;
        }
        while (q.events.empty() && !q.destroyed && !t.kill) q.cv.wait_for(lk, std::chrono::milliseconds(1));
        if (q.events.empty()) {
            lk.unlock();
            t.push_in_mbox(CELL_ECANCELED);
            return !t.kill;
        }
        const auto e = q.events.front();
        q.events.pop_front();
        lk.unlock();
        std::lock_guard<std::mutex> ml(t.m);
        t.in_mbox.clear();
        t.in_mbox.push_back(CELL_OK);
        t.in_mbox.push_back(static_cast<u32>(e.data1));
        t.in_mbox.push_back(static_cast<u32>(e.data2));
        t.in_mbox.push_back(static_cast<u32>(e.data3));
        t.cv.notify_all();
        return true;
    }
    default:
        LOG_WARN("spu", "%s: stop 0x%X at LS 0x%05X (thread stops)", t.name.c_str(), code, t.pc - 4);
        th.exit_status = static_cast<s32>(code);
        return false;
    }
}

void on_out_intr_mbox(SpuThreadObj& th, u32 value) {
    spu::SPUThread& t = th.spu;
    const u32 code = value >> 24;
    u32 data = 0;
    t.pop_out_mbox(&data);
    if (code < 128) {  // sys_spu_thread_send_event (< 64) / throw_event (< 128)
        const u32 port = code & 63;
        u32 res = CELL_OK;
        if (auto q = th.spup[port]) {
            if (!send_event(*q, SYS_SPU_THREAD_EVENT_USER_KEY, th.spu.id, (u64(port) << 32) | (value & 0xFFFFFF), data))
                res = CELL_EBUSY;
        } else {
            res = 0x80010013;  // ENOTCONN
        }
        if (code < 64) t.push_in_mbox(res);
        return;
    }
    if (code == 128 || code == 192) {  // sys_event_flag_set_bit(_impatient): flag id in low bits, bit in out mbox
        u32 res = CELL_OK;
        if (auto f = get<EventFlag>(value & 0xFFFFFF)) {
            std::lock_guard<std::mutex> lk(f->m);
            f->pattern |= 1ull << (data & 63);
            f->cv.notify_all();
        } else {
            res = CELL_ESRCH;
        }
        if (code == 128) t.push_in_mbox(res);
        return;
    }
    LOG_WARN("spu", "%s: unknown interrupt mailbox code 0x%08X", t.name.c_str(), value);
}

void run_thread(std::shared_ptr<SpuThreadObj> th, std::shared_ptr<SpuGroup> g) {
    spu::SPUThread& t = th->spu;
    t.pc = th->entry;
    t.stop_code = 0;
    SpuThreadObj* self = th.get();  // the object owns the hooks; no shared_ptr cycle
    t.on_stop = [self](spu::SPUThread&, u32 code) { return on_stop(*self, code); };
    t.on_out_intr_mbox = [self](spu::SPUThread&, u32 v) { on_out_intr_mbox(*self, v); };
    // Arguments: $3..$6 = arg1..arg4 in the preferred doubleword.
    for (int k = 0; k < 4; ++k) {
        t.gpr[3 + k] = v128::zero();
        t.gpr[3 + k].u64_[1] = th->args[k];
    }
    LOG_INFO("spu", "start %s (group '%s') at LS 0x%05X", t.name.c_str(), g->name.c_str(), th->entry);
    spu::run(t);
    std::lock_guard<std::mutex> lk(g->m);
    th->exited = true;
    if (--g->running_count == 0) {
        g->running = false;
        if (!g->join_cause) g->join_cause = JOIN_ALL_THREADS_EXIT;
        g->cv.notify_all();
    }
}

std::shared_ptr<SpuThreadObj> thread(u32 id) { return get<SpuThreadObj>(id); }

}  // namespace

// Extent of an SPU ELF in memory, from its program and section headers.
static u32 spu_elf_extent(const u8* e) {
    if (std::memcmp(e, "\x7f" "ELF", 4) != 0) return 0;
    u32 end = 52;
    const u32 phoff = load_be<u32>(e + 28), shoff = load_be<u32>(e + 32);
    const u16 phnum = load_be<u16>(e + 44), shnum = load_be<u16>(e + 48);
    end = std::max(end, phoff + 32u * phnum);
    for (u16 k = 0; k < phnum; ++k) {
        const u8* p = e + phoff + 32u * k;
        end = std::max(end, load_be<u32>(p + 4) + load_be<u32>(p + 16));
    }
    if (shoff && shnum < 64) end = std::max<u32>(end, shoff + 40u * shnum);
    return end;
}

s32 spu_image_import(u32 img, u32 src, u32 size) {
    if (!size) size = spu_elf_extent(mem::ptr(src));
    return build_image(img, mem::ptr(src), size);
}

s32 spu_elf_info(u32 elf, u32* entry, u32* nsegs, std::vector<u32>* seg_words) {
    const u8* e = mem::ptr(elf);
    if (std::memcmp(e, "\x7f" "ELF", 4) != 0 || load_be<u16>(e + 18) != 23) return -1;
    const u32 phoff = load_be<u32>(e + 28);
    const u16 phnum = load_be<u16>(e + 44);
    u32 n = 0;
    for (u16 k = 0; k < phnum; ++k) {
        const u8* p = e + phoff + 32u * k;
        const u32 type = load_be<u32>(p), off = load_be<u32>(p + 4), va = load_be<u32>(p + 8);
        const u32 filesz = load_be<u32>(p + 16), memsz = load_be<u32>(p + 20);
        auto add = [&](u32 t, u32 ls, u32 sz, u32 a) {
            ++n;
            if (seg_words) seg_words->insert(seg_words->end(), {t, ls, sz, a});
        };
        if (type == 1) {
            if (filesz) add(SYS_SPU_SEGMENT_TYPE_COPY, va, filesz, elf + off);
            if (memsz > filesz) add(SYS_SPU_SEGMENT_TYPE_FILL, va + filesz, memsz - filesz, 0);
        } else if (type == 4) {
            add(SYS_SPU_SEGMENT_TYPE_INFO, va, filesz, elf + off);
        }
    }
    if (entry) *entry = load_be<u32>(e + 24);
    if (nsegs) *nsegs = n;
    return 0;
}

bool spu_syscall(PPUContext& c, u32 n, u64* result) {
    u64& r = *result;
    r = CELL_OK;
    switch (n) {
    case 169: return true;  // sys_spu_initialize(max_usable, max_raw)

    // ---- images
    case 156: {  // sys_spu_image_open(img*, path)
        const std::string path = mem::read_string(A32(c, 1));
        auto host = vfs::translate(path);
        std::ifstream f(host ? host->string() : std::string(), std::ios::binary);
        if (!f) { r = CELL_ENOENT; return true; }
        std::vector<u8> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        r = build_image(A32(c, 0), d.data(), d.size()) == 0 ? CELL_OK : 0x80010008;  // ENOEXEC-ish
        return true;
    }
    case 157: {  // _sys_spu_image_import(img*, src ea, size, arg)
        const u32 src = A32(c, 1);
        r = spu_image_import(A32(c, 0), src, A32(c, 2)) == 0 ? CELL_OK : CELL_EINVAL;
        return true;
    }
    case 158: return true;  // _sys_spu_image_close
    case 155:               // _sys_spu_image_get_information(img*, entry*, nsegs*)
        if (A32(c, 1)) mem::w32(A32(c, 1), mem::r32(A32(c, 0) + 4));
        if (A32(c, 2)) mem::w32(A32(c, 2), mem::r32(A32(c, 0) + 12));
        return true;
    case 159: {  // _sys_spu_image_get_segments(img*, segs*, nseg)
        const u32 img = A32(c, 0);
        const u32 n2 = std::min<u32>(A32(c, 2), mem::r32(img + 12));
        std::memmove(mem::ptr(A32(c, 1)), mem::ptr(mem::r32(img + 8)), 16 * n2);
        return true;
    }
    case 260: {  // sys_spu_image_open_by_fd(img*, fd, offset)
        r = CELL_ENOSYS;
        LOG_WARN("spu", "sys_spu_image_open_by_fd not supported yet");
        return true;
    }

    // ---- thread groups
    case 170: {  // sys_spu_thread_group_create(u32* id, u32 num, s32 prio, attr*)
        auto g = std::make_shared<SpuGroup>();
        g->num = A32(c, 1);
        g->prio = static_cast<s32>(A32(c, 2));
        const u32 attr = A32(c, 3);
        if (attr) {
            g->name = mem::read_string(mem::r32(attr + 4), std::max<u32>(mem::r32(attr), 1));
            g->type = static_cast<s32>(mem::r32(attr + 8));
        }
        g->threads.resize(g->num);
        g->id = add_object(g);
        mem::w32(A32(c, 0), g->id);
        LOG_INFO("spu", "thread group '%s' id 0x%08X: %u SPUs, prio %d, type 0x%X", g->name.c_str(), g->id, g->num,
                 g->prio, g->type);
        return true;
    }
    case 171: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; return true;
    case 172: {  // sys_spu_thread_initialize(u32* thread, u32 group, u32 spu_num, img*, attr*, arg*)
        auto g = get<SpuGroup>(A32(c, 1));
        const u32 idx = A32(c, 2);
        if (!g || idx >= g->num) { r = CELL_ESRCH; return true; }
        auto th = std::make_shared<SpuThreadObj>();
        th->group = g;
        th->index = idx;
        const u32 img = A32(c, 3);
        apply_image(th->spu, img);
        th->entry = mem::r32(img + 4);
        if (const u32 attr = A32(c, 4)) th->spu.name = mem::read_string(mem::r32(attr), 64);
        if (th->spu.name.empty()) th->spu.name = g->name + "#" + std::to_string(idx);
        if (const u32 args = A32(c, 5))
            for (int k = 0; k < 4; ++k) th->args[k] = mem::r64(args + 8 * k);
        th->spu.id = add_object(th);
        th->spu.group_id = g->id;
        g->threads[idx] = th;
        mem::w32(A32(c, 0), th->spu.id);
        return true;
    }
    case 166: {  // sys_spu_thread_set_argument(id, arg*)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        for (int k = 0; k < 4; ++k) th->args[k] = mem::r64(A32(c, 1) + 8 * k);
        return true;
    }
    case 173: {  // sys_spu_thread_group_start(id)
        auto g = get<SpuGroup>(A32(c, 0));
        if (!g) { r = CELL_ESRCH; return true; }
        std::lock_guard<std::mutex> lk(g->m);
        for (auto& h : g->hosts)
            if (h.joinable()) h.join();
        g->hosts.clear();
        g->running = true;
        g->join_cause = 0;
        g->running_count = 0;
        for (auto& th : g->threads) {
            if (!th) continue;
            th->exited = false;
            th->spu.kill = false;
            ++g->running_count;
            g->hosts.emplace_back(run_thread, th, g);
        }
        return true;
    }
    case 174: case 175: case 176: return true;  // suspend / resume / yield
    case 177: {  // sys_spu_thread_group_terminate(id, value)
        auto g = get<SpuGroup>(A32(c, 0));
        if (!g) { r = CELL_ESRCH; return true; }
        std::lock_guard<std::mutex> lk(g->m);
        g->join_cause = JOIN_TERMINATED;
        g->exit_status = static_cast<s32>(A32(c, 1));
        for (auto& th : g->threads)
            if (th) th->spu.kill = true;
        return true;
    }
    case 178: {  // sys_spu_thread_group_join(id, u32* cause, u32* status)
        auto g = get<SpuGroup>(A32(c, 0));
        if (!g) { r = CELL_ESRCH; return true; }
        std::vector<std::thread> hosts;
        {
            std::unique_lock<std::mutex> lk(g->m);
            g->cv.wait(lk, [&] { return !g->running; });
            hosts.swap(g->hosts);
        }
        for (auto& h : hosts)
            if (h.joinable()) h.join();
        if (A32(c, 1)) mem::w32(A32(c, 1), g->join_cause);
        if (A32(c, 2)) mem::w32(A32(c, 2), static_cast<u32>(g->exit_status));
        return true;
    }
    case 179: if (auto g = get<SpuGroup>(A32(c, 0))) g->prio = static_cast<s32>(A32(c, 1)); return true;
    case 180: {
        auto g = get<SpuGroup>(A32(c, 0));
        mem::w32(A32(c, 1), g ? static_cast<u32>(g->prio) : 100u);
        return true;
    }
    case 165: {  // sys_spu_thread_get_exit_status(id, s32*)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        if (!th->exited) { r = CELL_ESTAT; return true; }
        mem::w32(A32(c, 1), static_cast<u32>(th->exit_status));
        return true;
    }

    // ---- LS / mailbox / signal access
    case 181: {  // sys_spu_thread_write_ls(id, lsa, u64 value, type)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        const u32 lsa = A32(c, 1) & spu::kLsMask;
        const u64 v = A(c, 2);
        th->spu.touch(lsa, 8);
        switch (A32(c, 3)) {
        case 1: th->spu.ls[lsa] = static_cast<u8>(v); break;
        case 2: store_be<u16>(th->spu.ls + lsa, static_cast<u16>(v)); break;
        case 4: store_be<u32>(th->spu.ls + lsa, static_cast<u32>(v)); break;
        case 8: store_be<u64>(th->spu.ls + lsa, v); break;
        default: r = CELL_EINVAL; break;
        }
        return true;
    }
    case 182: {  // sys_spu_thread_read_ls(id, lsa, u64* value, type)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        const u32 lsa = A32(c, 1) & spu::kLsMask;
        u64 v = 0;
        switch (A32(c, 3)) {
        case 1: v = th->spu.ls[lsa]; break;
        case 2: v = load_be<u16>(th->spu.ls + lsa); break;
        case 4: v = load_be<u32>(th->spu.ls + lsa); break;
        case 8: v = load_be<u64>(th->spu.ls + lsa); break;
        default: r = CELL_EINVAL; return true;
        }
        mem::w64(A32(c, 2), v);
        return true;
    }
    case 184: {  // sys_spu_thread_write_snr(id, number, value)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        th->spu.write_snr(A32(c, 1) & 1, A32(c, 2));
        return true;
    }
    case 190: {  // sys_spu_thread_write_spu_mb(id, value)
        auto th = thread(A32(c, 0));
        if (!th) { r = CELL_ESRCH; return true; }
        th->spu.push_in_mbox(A32(c, 1));
        return true;
    }
    case 187: if (auto th = thread(A32(c, 0))) th->cfg = A(c, 1); return true;
    case 188: {
        auto th = thread(A32(c, 0));
        mem::w64(A32(c, 1), th ? th->cfg : 0);
        return true;
    }

    // ---- events
    case 185: case 186: return true;  // group (run/exception) events: not generated yet
    case 191: {  // sys_spu_thread_connect_event(id, eq, et, spup)
        auto th = thread(A32(c, 0));
        auto q = get<EventQueue>(A32(c, 1));
        if (!th || !q) { r = CELL_ESRCH; return true; }
        th->spup[A32(c, 3) & 63] = q;
        return true;
    }
    case 192: {
        if (auto th = thread(A32(c, 0))) th->spup[A32(c, 2) & 63].reset();
        return true;
    }
    case 193: {  // sys_spu_thread_bind_queue(id, spuq, spuq_num)
        auto th = thread(A32(c, 0));
        auto q = get<EventQueue>(A32(c, 1));
        if (!th || !q) { r = CELL_ESRCH; return true; }
        th->spuq[A32(c, 2)] = q;
        return true;
    }
    case 194: {
        if (auto th = thread(A32(c, 0))) th->spuq.erase(A32(c, 1));
        return true;
    }
    case 251: {  // sys_spu_thread_group_connect_event_all_threads(gid, eq, u64 req, u8* spup)
        auto g = get<SpuGroup>(A32(c, 0));
        auto q = get<EventQueue>(A32(c, 1));
        if (!g || !q) { r = CELL_ESRCH; return true; }
        const u64 req = A(c, 2);
        for (u32 port = 0; port < 64; ++port) {
            if (!(req & (1ull << port))) continue;
            bool free = true;
            for (auto& th : g->threads)
                if (th && th->spup[port]) free = false;
            if (!free) continue;
            for (auto& th : g->threads)
                if (th) th->spup[port] = q;
            mem::w8(A32(c, 3), static_cast<u8>(port));
            return true;
        }
        r = CELL_EBUSY;
        return true;
    }
    case 252: {  // ..._disconnect_event_all_threads(gid, spup)
        if (auto g = get<SpuGroup>(A32(c, 0)))
            for (auto& th : g->threads)
                if (th) th->spup[A32(c, 1) & 63].reset();
        return true;
    }
    default:
        return false;
    }
}

}  // namespace opentit::kernel
