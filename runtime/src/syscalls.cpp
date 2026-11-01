// LV2 system calls invoked directly with `sc` (r11 = syscall number).
//
// Covers every syscall number referenced by Sly 4's EBOOT. Unknown calls log
// once and return ENOSYS so a missing implementation is obvious in the log.
#include <chrono>
#include <cstdio>
#include <ctime>
#include <set>
#include <thread>

#include "kernel.h"
#include "log.h"
#include "opentit/memory.h"

namespace opentit {

namespace kernel {

namespace {

using namespace rtm;

u64 A(PPUContext& c, int n) { return c.gpr[3 + n]; }
u32 A32(PPUContext& c, int n) { return static_cast<u32>(c.gpr[3 + n]); }

// ------------------------------------------------------------ objects
struct Semaphore : Object {
    std::mutex m;
    std::condition_variable cv;
    s32 value = 0, max = 0;
};

struct EventFlag : Object {
    std::mutex m;
    std::condition_variable cv;
    u64 pattern = 0;
};

struct RwLock : Object {
    std::mutex m;
    std::condition_variable cv;
    u32 readers = 0;
    u32 writer = 0;  // owning thread id
};

u32 rwlock_op(PPUContext& c, u32 n) {
    auto rw = get<RwLock>(A32(c, 0));
    if (!rw) return CELL_ESRCH;
    const u32 tid = current_thread_id();
    std::unique_lock<std::mutex> lk(rw->m);
    switch (n) {
    case 122:  // rlock(id, timeout)
        if (rw->writer == tid) return CELL_EDEADLK;
        if (!wait_for(rw->cv, lk, A(c, 1), [&] { return rw->writer == 0; })) return CELL_ETIMEDOUT;
        ++rw->readers;
        return CELL_OK;
    case 123:  // tryrlock
        if (rw->writer) return CELL_EBUSY;
        ++rw->readers;
        return CELL_OK;
    case 124:  // runlock
        if (!rw->readers) return CELL_EPERM;
        if (--rw->readers == 0) rw->cv.notify_all();
        return CELL_OK;
    case 125:  // wlock(id, timeout)
        if (rw->writer == tid) return CELL_EDEADLK;
        if (!wait_for(rw->cv, lk, A(c, 1), [&] { return rw->writer == 0 && rw->readers == 0; })) return CELL_ETIMEDOUT;
        rw->writer = tid;
        return CELL_OK;
    case 126:  // trywlock
        if (rw->writer || rw->readers) return CELL_EBUSY;
        rw->writer = tid;
        return CELL_OK;
    case 127:  // wunlock
        if (rw->writer != tid) return CELL_EPERM;
        rw->writer = 0;
        rw->cv.notify_all();
        return CELL_OK;
    default:
        return CELL_ENOSYS;
    }
}

u32 sys_mutex_create(PPUContext& c) {
    // (u32* mutex_id, sys_mutex_attribute_t* attr)  attr: protocol, recursive, pshared, adaptive, key, flags, name
    auto mx = std::make_shared<Mutex>();
    const u32 attr = A32(c, 1);
    mx->recursive = attr && mem::r32(attr + 4) == 0x10;  // SYS_SYNC_RECURSIVE
    mem::w32(A32(c, 0), add_object(mx));
    return CELL_OK;
}

u32 sys_cond_create(PPUContext& c) {
    // (u32* cond_id, u32 mutex_id, attr*)
    auto mx = get<Mutex>(A32(c, 1));
    if (!mx) return CELL_ESRCH;
    auto cv = std::make_shared<Cond>();
    cv->mutex = mx;
    mem::w32(A32(c, 0), add_object(cv));
    return CELL_OK;
}

u32 sys_cond_wait(PPUContext& c) {
    auto cv = get<Cond>(A32(c, 0));
    if (!cv) return CELL_ESRCH;
    const u32 tid = current_thread_id();
    Mutex& mx = *cv->mutex;
    std::unique_lock<std::mutex> lk(mx.m);
    if (mx.owner != tid) return CELL_EPERM;
    const u32 saved = mx.count;
    mx.owner = 0;
    mx.count = 0;
    mx.cv.notify_one();
    const u64 gen = cv->signals;
    bool ok = true;
    const u64 timeout = A(c, 1);
    if (timeout) ok = cv->cv.wait_for(lk, std::chrono::microseconds(timeout), [&] { return cv->signals != gen; });
    else cv->cv.wait(lk, [&] { return cv->signals != gen; });
    mx.cv.wait(lk, [&] { return mx.owner == 0; });
    mx.owner = tid;
    mx.count = saved;
    return ok ? CELL_OK : CELL_ETIMEDOUT;
}

u32 sys_cond_signal(PPUContext& c, bool all) {
    auto cv = get<Cond>(A32(c, 0));
    if (!cv) return CELL_ESRCH;
    std::lock_guard<std::mutex> lk(cv->mutex->m);
    ++cv->signals;
    if (all) cv->cv.notify_all();
    else cv->cv.notify_one();
    return CELL_OK;
}

u32 sys_event_queue_create(PPUContext& c) {
    // (u32* equeue_id, attr*, u64 event_queue_key, s32 size)
    auto q = std::make_shared<EventQueue>();
    q->key = A(c, 2);
    q->size = A32(c, 3) ? A32(c, 3) : 127;
    mem::w32(A32(c, 0), add_object(q));
    return CELL_OK;
}

u32 sys_event_queue_receive(PPUContext& c) {
    // (u32 equeue_id, sys_event_t* dummy, u64 timeout) -> event in r4..r7
    auto q = get<EventQueue>(A32(c, 0));
    if (!q) return CELL_ESRCH;
    std::unique_lock<std::mutex> lk(q->m);
    if (!wait_for(q->cv, lk, A(c, 2), [&] { return !q->events.empty() || q->destroyed; })) return CELL_ETIMEDOUT;
    if (q->destroyed) return CELL_ECANCELED;
    const auto e = q->events.front();
    q->events.pop_front();
    c.gpr[4] = e.source;
    c.gpr[5] = e.data1;
    c.gpr[6] = e.data2;
    c.gpr[7] = e.data3;
    return CELL_OK;
}

u32 sys_event_port_send(PPUContext& c) {
    // (u32 eport_id, u64 data1, u64 data2, u64 data3)
    auto p = get<EventPort>(A32(c, 0));
    if (!p) return CELL_ESRCH;
    auto q = p->queue.lock();
    if (!q) return 0x80010013;  // ENOTCONN
    std::lock_guard<std::mutex> lk(q->m);
    if (q->events.size() >= q->size) return CELL_EBUSY;
    q->events.push_back({p->name, A(c, 1), A(c, 2), A(c, 3)});
    q->cv.notify_one();
    return CELL_OK;
}

u32 sys_event_flag_wait(PPUContext& c) {
    // (u32 id, u64 bitptn, u32 mode, u64* result, u64 timeout)
    auto f = get<EventFlag>(A32(c, 0));
    if (!f) return CELL_ESRCH;
    const u64 bits = A(c, 1);
    const u32 mode = A32(c, 2);
    const bool and_mode = (mode & 0xF) == 0x01;  // SYS_EVENT_FLAG_WAIT_AND
    auto ready = [&] { return and_mode ? (f->pattern & bits) == bits : (f->pattern & bits) != 0; };
    std::unique_lock<std::mutex> lk(f->m);
    if (!wait_for(f->cv, lk, A(c, 4), ready)) return CELL_ETIMEDOUT;
    if (A32(c, 3)) mem::w64(A32(c, 3), f->pattern);
    if (mode & 0x10) f->pattern &= ~bits;  // CLEAR
    else if (mode & 0x20) f->pattern = 0;  // CLEAR_ALL
    return CELL_OK;
}

u32 sys_semaphore_wait(PPUContext& c, bool try_only) {
    auto s = get<Semaphore>(A32(c, 0));
    if (!s) return CELL_ESRCH;
    std::unique_lock<std::mutex> lk(s->m);
    if (try_only) {
        if (s->value <= 0) return CELL_EBUSY;
    } else if (!wait_for(s->cv, lk, A(c, 1), [&] { return s->value > 0; })) {
        return CELL_ETIMEDOUT;
    }
    --s->value;
    return CELL_OK;
}

std::set<u32>& warned() {
    static std::set<u32> s;
    return s;
}

}  // namespace

const char* syscall_name(u32 n) {
    switch (n) {
    case 1: return "sys_process_getpid";
    case 3: return "sys_process_exit";
    case 14: return "sys_process_is_stack";
    case 22: return "_sys_process_exit";
    case 25: return "sys_process_get_sdk_version";
    case 41: return "_sys_ppu_thread_exit";
    case 43: return "sys_ppu_thread_yield";
    case 44: return "sys_ppu_thread_join";
    case 45: return "sys_ppu_thread_detach";
    case 47: return "sys_ppu_thread_set_priority";
    case 48: return "sys_ppu_thread_get_priority";
    case 49: return "sys_ppu_thread_get_stack_information";
    case 70: return "sys_timer_create";
    case 71: return "sys_timer_destroy";
    case 73: return "_sys_timer_start";
    case 74: return "sys_timer_stop";
    case 75: return "sys_timer_connect_event_queue";
    case 76: return "sys_timer_disconnect_event_queue";
    case 82: return "sys_event_flag_create";
    case 83: return "sys_event_flag_destroy";
    case 85: return "sys_event_flag_wait";
    case 86: return "sys_event_flag_trywait";
    case 87: return "sys_event_flag_set";
    case 90: return "sys_semaphore_create";
    case 91: return "sys_semaphore_destroy";
    case 92: return "sys_semaphore_wait";
    case 93: return "sys_semaphore_trywait";
    case 94: return "sys_semaphore_post";
    case 100: return "sys_mutex_create";
    case 101: return "sys_mutex_destroy";
    case 102: return "sys_mutex_lock";
    case 103: return "sys_mutex_trylock";
    case 104: return "sys_mutex_unlock";
    case 105: return "sys_cond_create";
    case 106: return "sys_cond_destroy";
    case 107: return "sys_cond_wait";
    case 108: return "sys_cond_signal";
    case 109: return "sys_cond_signal_all";
    case 118: return "sys_event_flag_clear";
    case 120: return "sys_rwlock_create";
    case 121: return "sys_rwlock_destroy";
    case 122: return "sys_rwlock_rlock";
    case 123: return "sys_rwlock_tryrlock";
    case 124: return "sys_rwlock_runlock";
    case 125: return "sys_rwlock_wlock";
    case 126: return "sys_rwlock_trywlock";
    case 127: return "sys_rwlock_wunlock";
    case 128: return "sys_event_queue_create";
    case 129: return "sys_event_queue_destroy";
    case 130: return "sys_event_queue_receive";
    case 131: return "sys_event_queue_tryreceive";
    case 133: return "sys_event_queue_drain";
    case 134: return "sys_event_port_create";
    case 135: return "sys_event_port_destroy";
    case 136: return "sys_event_port_connect_local";
    case 137: return "sys_event_port_disconnect";
    case 138: return "sys_event_port_send";
    case 141: return "sys_timer_usleep";
    case 142: return "sys_timer_sleep";
    case 144: return "sys_time_get_timezone";
    case 145: return "sys_time_get_current_time";
    case 147: return "sys_time_get_timebase_frequency";
    case 156: return "sys_spu_image_open";
    case 169: return "sys_spu_initialize";
    case 181: return "sys_spu_thread_write_ls";
    case 182: return "sys_spu_thread_read_ls";
    case 190: return "sys_spu_thread_write_spu_mb";
    case 260: return "sys_spu_image_open_by_fd";
    case 330: return "sys_mmapper_allocate_address";
    case 331: return "sys_mmapper_free_address";
    case 348: return "sys_memory_allocate";
    case 349: return "sys_memory_free";
    case 351: return "sys_memory_get_page_attribute";
    case 352: return "sys_memory_get_user_memory_size";
    case 402: return "sys_tty_read";
    case 403: return "sys_tty_write";
    case 802: return "sys_fs_read";
    case 803: return "sys_fs_write";
    case 988: return "sys_dbg_get_ppu_thread_name";
    default: return "unknown";
    }
}

void dispatch_syscall(PPUContext& c, u32 pc) {
    const u32 n = static_cast<u32>(c.gpr[11]);
    LOG_TRACE("syscall", "%s(%u) r3=0x%llX r4=0x%llX r5=0x%llX at 0x%08X", syscall_name(n), n,
              (unsigned long long)c.gpr[3], (unsigned long long)c.gpr[4], (unsigned long long)c.gpr[5], pc);
    u64 r = CELL_OK;
    switch (n) {
    case 1: r = 0x01000500; break;
    case 3:
    case 22: exit_process(static_cast<int>(A32(c, 0)));
    case 14: {  // sys_process_is_stack(addr)
        const u32 a = A32(c, 0);
        r = (a >= mem::kStackBase && a < mem::kStackBase + mem::kStackRegionSize) ? 1 : 0;
        break;
    }
    case 25: mem::w32(A32(c, 1), elf().process_param().sdk_version); break;
    case 41: throw ThreadExit{A(c, 0)};
    case 43: std::this_thread::yield(); break;
    case 44: {  // sys_ppu_thread_join(id, u64* status)
        auto th = thread_by_id(A32(c, 0));
        if (!th) { r = CELL_ESRCH; break; }
        if (th->host.joinable()) th->host.join();
        if (A32(c, 1)) mem::w64(A32(c, 1), th->exit_status);
        break;
    }
    case 45: {
        auto th = thread_by_id(A32(c, 0));
        if (!th) { r = CELL_ESRCH; break; }
        if (th->host.joinable()) th->host.detach();
        break;
    }
    case 47: {
        if (auto th = thread_by_id(A32(c, 0))) th->priority = static_cast<s32>(A32(c, 1));
        break;
    }
    case 48: {
        auto th = thread_by_id(A32(c, 0));
        mem::w32(A32(c, 1), th ? static_cast<u32>(th->priority) : 1001u);
        break;
    }
    case 49: {  // (sys_ppu_thread_stack_t* {u32 addr, u32 size})
        mem::w32(A32(c, 0), c.stack_addr);
        mem::w32(A32(c, 0) + 4, c.stack_size);
        break;
    }

    // ---- event flags
    case 82: {  // (u32* id, attr*, u64 init)
        auto f = std::make_shared<EventFlag>();
        f->pattern = A(c, 2);
        mem::w32(A32(c, 0), add_object(f));
        break;
    }
    case 83: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 85: r = sys_event_flag_wait(c); break;
    case 86: {
        c.gpr[7] = 1;  // minimal timeout to emulate trywait
        r = sys_event_flag_wait(c);
        if (r == CELL_ETIMEDOUT) r = CELL_EBUSY;
        break;
    }
    case 87: {
        auto f = get<EventFlag>(A32(c, 0));
        if (!f) { r = CELL_ESRCH; break; }
        std::lock_guard<std::mutex> lk(f->m);
        f->pattern |= A(c, 1);
        f->cv.notify_all();
        break;
    }
    case 118: {
        auto f = get<EventFlag>(A32(c, 0));
        if (!f) { r = CELL_ESRCH; break; }
        std::lock_guard<std::mutex> lk(f->m);
        f->pattern &= A(c, 1);
        break;
    }

    // ---- semaphores
    case 90: {  // (u32* id, attr*, s32 initial, s32 max)
        auto s = std::make_shared<Semaphore>();
        s->value = static_cast<s32>(A32(c, 2));
        s->max = static_cast<s32>(A32(c, 3));
        mem::w32(A32(c, 0), add_object(s));
        break;
    }
    case 91: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 92: r = sys_semaphore_wait(c, false); break;
    case 93: r = sys_semaphore_wait(c, true); break;
    case 94: {
        auto s = get<Semaphore>(A32(c, 0));
        if (!s) { r = CELL_ESRCH; break; }
        std::lock_guard<std::mutex> lk(s->m);
        s->value += static_cast<s32>(A32(c, 1));
        s->cv.notify_all();
        break;
    }

    // ---- mutex / cond
    case 100: r = sys_mutex_create(c); break;
    case 101: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 102: {
        auto mx = get<Mutex>(A32(c, 0));
        r = mx ? mx->lock(current_thread_id(), A(c, 1)) : CELL_ESRCH;
        break;
    }
    case 103: {
        auto mx = get<Mutex>(A32(c, 0));
        r = mx ? mx->trylock(current_thread_id()) : CELL_ESRCH;
        break;
    }
    case 104: {
        auto mx = get<Mutex>(A32(c, 0));
        r = mx ? mx->unlock(current_thread_id()) : CELL_ESRCH;
        break;
    }
    case 105: r = sys_cond_create(c); break;
    case 106: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 107: r = sys_cond_wait(c); break;
    case 108: r = sys_cond_signal(c, false); break;
    case 109: r = sys_cond_signal(c, true); break;
    case 120: mem::w32(A32(c, 0), add_object(std::make_shared<RwLock>())); break;
    case 121: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 122: case 123: case 124: case 125: case 126: case 127: r = rwlock_op(c, n); break;

    // ---- event queues / ports
    case 128: r = sys_event_queue_create(c); break;
    case 129: {
        if (auto q = get<EventQueue>(A32(c, 0))) {
            std::lock_guard<std::mutex> lk(q->m);
            q->destroyed = true;
            q->cv.notify_all();
        }
        r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH;
        break;
    }
    case 130: r = sys_event_queue_receive(c); break;
    case 133: {
        if (auto q = get<EventQueue>(A32(c, 0))) {
            std::lock_guard<std::mutex> lk(q->m);
            q->events.clear();
        } else {
            r = CELL_ESRCH;
        }
        break;
    }
    case 134: {  // (u32* eport_id, s32 port_type, u64 name)
        auto p = std::make_shared<EventPort>();
        p->name = A(c, 2);
        const u32 id = add_object(p);
        if (!p->name) p->name = id;
        mem::w32(A32(c, 0), id);
        break;
    }
    case 135: r = remove_object(A32(c, 0)) ? CELL_OK : CELL_ESRCH; break;
    case 136: {
        auto p = get<EventPort>(A32(c, 0));
        auto q = get<EventQueue>(A32(c, 1));
        if (!p || !q) { r = CELL_ESRCH; break; }
        p->queue = q;
        break;
    }
    case 137: {
        if (auto p = get<EventPort>(A32(c, 0))) p->queue.reset();
        else r = CELL_ESRCH;
        break;
    }
    case 138: r = sys_event_port_send(c); break;

    // ---- time
    case 141: std::this_thread::sleep_for(std::chrono::microseconds(A(c, 0))); break;
    case 142: std::this_thread::sleep_for(std::chrono::seconds(A32(c, 0))); break;
    case 144:
        if (A32(c, 0)) mem::w32(A32(c, 0), 0);
        if (A32(c, 1)) mem::w32(A32(c, 1), 0);
        break;
    case 145: {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        mem::w64(A32(c, 0), static_cast<u64>(ns / 1000000000));
        mem::w64(A32(c, 1), static_cast<u64>(ns % 1000000000));
        break;
    }
    case 147: r = 79800000; break;

    // ---- memory
    case 330: {  // sys_mmapper_allocate_address(size, flags, alignment, u32* addr)
        const u32 size = A32(c, 0);
        const u32 align = A32(c, 2) ? A32(c, 2) : 0x10000000;
        const u32 a = mmapper_space().alloc(size, align);
        if (!a) { r = CELL_ENOMEM; break; }
        mem::commit(a, size);
        mem::w32(A32(c, 3), a);
        break;
    }
    case 331: r = mmapper_space().free(A32(c, 0)) ? CELL_OK : CELL_EINVAL; break;
    case 348: {  // sys_memory_allocate(size, flags, u32* addr)
        const u32 size = A32(c, 0);
        const u64 flags = A(c, 1);
        const u32 align = flags == 0x200 ? 0x10000 : 0x100000;  // 64K pages or 1M pages
        const u32 a = user_memory().alloc(size, align);
        if (!a) { r = CELL_ENOMEM; break; }
        std::memset(mem::ptr(a), 0, size);
        mem::w32(A32(c, 2), a);
        break;
    }
    case 349: r = user_memory().free(A32(c, 0)) ? CELL_OK : CELL_EINVAL; break;
    case 351: {  // sys_memory_get_page_attribute(addr, attr*) {u64 attribute, u64 access_right, u32 page_size, u32 pad}
        const u32 out = A32(c, 1);
        mem::w64(out, 0x40000ull);
        mem::w64(out + 8, 0xFull);
        mem::w32(out + 16, 0x100000);
        mem::w32(out + 20, 0);
        break;
    }
    case 352: {  // sys_memory_get_user_memory_size({u32 total, u32 available})
        auto& um = user_memory();
        mem::w32(A32(c, 0), um.size());
        mem::w32(A32(c, 0) + 4, um.size() - um.used());
        break;
    }

    // ---- tty
    case 402: r = 0; mem::w32(A32(c, 3), 0); break;
    case 403: {  // sys_tty_write(ch, buf, len, u32* written)
        const u32 len = A32(c, 2);
        std::string s(reinterpret_cast<const char*>(mem::ptr(A32(c, 1))), len);
        std::fwrite(s.data(), 1, s.size(), stdout);
        std::fflush(stdout);
        if (A32(c, 3)) mem::w32(A32(c, 3), len);
        break;
    }

    // ---- SPU (not yet emulated - see docs/ROADMAP.md)
    case 156:
    case 169:
    case 181:
    case 182:
    case 190:
    case 260:
    case 70: case 71: case 73: case 74: case 75: case 76:
    case 988:
    default: {
        bool first;
        {
            static std::mutex m;
            std::lock_guard<std::mutex> lk(m);
            first = warned().insert(n).second;
        }
        if (first)
            LOG_WARN("syscall", "unimplemented syscall %u (%s) at 0x%08X, r3=0x%llX", n, syscall_name(n), pc,
                     (unsigned long long)c.gpr[3]);
        r = (n == 169 || n == 988) ? CELL_OK : CELL_ENOSYS;
        break;
    }
    }
    c.gpr[3] = r;
}

}  // namespace kernel

void ppu_syscall(PPUContext& ctx, u32 pc) { kernel::dispatch_syscall(ctx, pc); }

}  // namespace opentit
