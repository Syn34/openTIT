// sysPrxForUser: process/thread basics, lightweight mutexes and condvars.
#include <chrono>
#include <mutex>
#include <unordered_map>

#include "../hle.h"
#include "../kernel.h"
#include "../log.h"

using namespace opentit;
using namespace opentit::rtm;

namespace {

// sys_lwmutex_t: u32 owner, u32 waiter, u32 attribute, u32 recursive_count, u32 sleep_queue, u32 pad
constexpr u32 kLwmutexFree = 0xFFFFFFFF;

std::mutex g_lw_mutex;
std::unordered_map<u32, std::shared_ptr<kernel::Mutex>> g_lwmutexes;  // by guest address
std::unordered_map<u32, std::shared_ptr<kernel::Cond>> g_lwconds;     // by guest address

std::shared_ptr<kernel::Mutex> lwmutex(u32 addr) {
    std::lock_guard<std::mutex> lk(g_lw_mutex);
    auto it = g_lwmutexes.find(addr);
    if (it != g_lwmutexes.end()) return it->second;
    // Statically initialised lwmutex (SYS_LWMUTEX_INITIALIZER) - create lazily.
    auto m = std::make_shared<kernel::Mutex>();
    m->recursive = (mem::r32(addr + 8) & 0x10) != 0;
    g_lwmutexes[addr] = m;
    return m;
}

void sync_guest_owner(u32 addr, const kernel::Mutex& m) {
    mem::w32(addr, m.owner ? m.owner : kLwmutexFree);
    mem::w32(addr + 12, m.count > 0 ? m.count - 1 : 0);
}

}  // namespace

OPENTIT_HLE(sysPrxForUser, sys_initialize_tls) {
    // (thread_id, tls_seg_addr, tls_seg_size, tls_mem_size)
    const u32 base = alloc_tls();
    ctx.gpr[13] = base + 0x7030;
    LOG_INFO("tls", "main thread TLS at 0x%08X (r13=0x%08llX)", base, (unsigned long long)ctx.gpr[13]);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_process_exit) { exit_process(static_cast<int>(arg32(ctx, 0))); }

OPENTIT_HLE(sysPrxForUser, sys_prx_exitspawn_with_level) { exit_process(0); }

OPENTIT_HLE(sysPrxForUser, sys_time_get_system_time) {
    static const auto t0 = std::chrono::steady_clock::now();
    ret(ctx, static_cast<u64>(
                 std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count()));
}

OPENTIT_HLE(sysPrxForUser, sys_process_is_stack) {
    const u32 a = arg32(ctx, 0);
    ret(ctx, (a >= mem::kStackBase && a < mem::kStackBase + mem::kStackRegionSize) ? 1 : 0);
}

OPENTIT_HLE_OK(sysPrxForUser, _sys_process_atexitspawn)
OPENTIT_HLE_OK(sysPrxForUser, _sys_process_at_Exitspawn)
OPENTIT_HLE_OK(sysPrxForUser, _sys_spu_printf_initialize)
OPENTIT_HLE_OK(sysPrxForUser, _sys_spu_printf_finalize)
OPENTIT_HLE_OK(sysPrxForUser, sys_prx_register_library)

// ---------------------------------------------------------------- PRX loading
// The game binary is recompiled ahead of time; dynamically loaded PRX code is
// not supported yet. Report success with a fake module id and log loudly.
static void fake_prx_load(PPUContext& ctx, const char* fn, u32 path_addr) {
    static u32 next = 0x23000100;
    LOG_WARN("prx", "%s(\"%s\") - PRX loading not supported, returning fake id", fn,
             path_addr ? mem::read_string(path_addr).c_str() : "?");
    ret(ctx, next++);
}
OPENTIT_HLE(sysPrxForUser, sys_prx_load_module) { fake_prx_load(ctx, "sys_prx_load_module", arg32(ctx, 0)); }
OPENTIT_HLE(sysPrxForUser, sys_prx_load_module_by_fd) { fake_prx_load(ctx, "sys_prx_load_module_by_fd", 0); }
OPENTIT_HLE(sysPrxForUser, sys_prx_load_module_on_memcontainer) {
    fake_prx_load(ctx, "sys_prx_load_module_on_memcontainer", arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, sys_prx_load_module_on_memcontainer_by_fd) {
    fake_prx_load(ctx, "sys_prx_load_module_on_memcontainer_by_fd", 0);
}

// ---------------------------------------------------------------- threads
OPENTIT_HLE(sysPrxForUser, sys_ppu_thread_create) {
    // (u64* thread_id, u32 entry_opd, u64 arg, s32 prio, u32 stacksize, u64 flags, const char* name)
    const u32 id_out = arg32(ctx, 0);
    const u32 entry = arg32(ctx, 1);
    const u64 a = arg(ctx, 2);
    const s32 prio = static_cast<s32>(arg32(ctx, 3));
    const u32 stack = arg32(ctx, 4);
    const u64 flags = arg(ctx, 5);
    const std::string name = mem::read_string(arg32(ctx, 6), 64);
    const u32 id = kernel::create_thread(entry, a, prio, stack, (flags & 1) != 0, name, true);
    if (!id) {
        ret(ctx, CELL_ENOMEM);
        return;
    }
    if (id_out) mem::w64(id_out, id);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_ppu_thread_get_id) {
    mem::w64(arg32(ctx, 0), kernel::current_thread_id());
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_ppu_thread_exit) { throw ThreadExit{arg(ctx, 0)}; }

OPENTIT_HLE(sysPrxForUser, sys_ppu_thread_once) {
    // (s32* once_ctrl, void (*init)())
    static std::mutex m;
    std::lock_guard<std::mutex> lk(m);
    const u32 ctrl = arg32(ctx, 0);
    if (mem::r32(ctrl) == 0) {
        mem::w32(ctrl, 1);
        hle::call_guest(ctx, arg32(ctx, 1), {});
    }
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- lwmutex
OPENTIT_HLE(sysPrxForUser, sys_lwmutex_create) {
    // (sys_lwmutex_t*, sys_lwmutex_attribute_t* {u32 protocol, u32 recursive, char name[8]})
    const u32 addr = arg32(ctx, 0);
    const u32 attr = arg32(ctx, 1);
    auto m = std::make_shared<kernel::Mutex>();
    const u32 protocol = attr ? mem::r32(attr) : 2;
    const u32 recursive = attr ? mem::r32(attr + 4) : 0x20;
    m->recursive = recursive == 0x10;
    {
        std::lock_guard<std::mutex> lk(g_lw_mutex);
        g_lwmutexes[addr] = m;
    }
    mem::w32(addr, kLwmutexFree);
    mem::w32(addr + 4, 0);
    mem::w32(addr + 8, protocol | recursive);
    mem::w32(addr + 12, 0);
    mem::w32(addr + 16, kernel::add_object(m));
    mem::w32(addr + 20, 0);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_lwmutex_destroy) {
    std::lock_guard<std::mutex> lk(g_lw_mutex);
    g_lwmutexes.erase(arg32(ctx, 0));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_lwmutex_lock) {
    const u32 addr = arg32(ctx, 0);
    auto m = lwmutex(addr);
    const u32 r = m->lock(kernel::current_thread_id(), arg(ctx, 1));
    if (r == CELL_OK) sync_guest_owner(addr, *m);
    ret(ctx, r);
}

OPENTIT_HLE(sysPrxForUser, sys_lwmutex_trylock) {
    const u32 addr = arg32(ctx, 0);
    auto m = lwmutex(addr);
    const u32 r = m->trylock(kernel::current_thread_id());
    if (r == CELL_OK) sync_guest_owner(addr, *m);
    ret(ctx, r);
}

OPENTIT_HLE(sysPrxForUser, sys_lwmutex_unlock) {
    const u32 addr = arg32(ctx, 0);
    auto m = lwmutex(addr);
    const u32 r = m->unlock(kernel::current_thread_id());
    if (r == CELL_OK) sync_guest_owner(addr, *m);
    ret(ctx, r);
}

// ---------------------------------------------------------------- lwcond
OPENTIT_HLE(sysPrxForUser, sys_lwcond_create) {
    // (sys_lwcond_t* {u32 lwmutex, u32 queue}, sys_lwmutex_t*, attr*)
    const u32 addr = arg32(ctx, 0);
    const u32 mutex_addr = arg32(ctx, 1);
    auto c = std::make_shared<kernel::Cond>();
    c->mutex = lwmutex(mutex_addr);
    {
        std::lock_guard<std::mutex> lk(g_lw_mutex);
        g_lwconds[addr] = c;
    }
    mem::w32(addr, mutex_addr);
    mem::w32(addr + 4, kernel::add_object(c));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_lwcond_destroy) {
    std::lock_guard<std::mutex> lk(g_lw_mutex);
    g_lwconds.erase(arg32(ctx, 0));
    ret(ctx, CELL_OK);
}

static std::shared_ptr<kernel::Cond> lwcond(u32 addr) {
    std::lock_guard<std::mutex> lk(g_lw_mutex);
    auto it = g_lwconds.find(addr);
    return it == g_lwconds.end() ? nullptr : it->second;
}

OPENTIT_HLE(sysPrxForUser, sys_lwcond_wait) {
    const u32 addr = arg32(ctx, 0);
    auto c = lwcond(addr);
    if (!c) {
        ret(ctx, CELL_ESRCH);
        return;
    }
    const u32 tid = kernel::current_thread_id();
    kernel::Mutex& mx = *c->mutex;
    const u32 mutex_addr = mem::r32(addr);
    std::unique_lock<std::mutex> lk(mx.m);
    if (mx.owner != tid) {
        ret(ctx, CELL_EPERM);
        return;
    }
    const u32 saved = mx.count;
    mx.owner = 0;
    mx.count = 0;
    mem::w32(mutex_addr, kLwmutexFree);
    mx.cv.notify_one();
    const u64 gen = c->signals;
    const u64 timeout = arg(ctx, 1);
    bool ok = true;
    if (timeout) ok = c->cv.wait_for(lk, std::chrono::microseconds(timeout), [&] { return c->signals != gen; });
    else c->cv.wait(lk, [&] { return c->signals != gen; });
    mx.cv.wait(lk, [&] { return mx.owner == 0; });
    mx.owner = tid;
    mx.count = saved;
    mem::w32(mutex_addr, tid);
    ret(ctx, ok ? CELL_OK : CELL_ETIMEDOUT);
}

static void lwcond_signal(PPUContext& ctx, bool all) {
    auto c = lwcond(arg32(ctx, 0));
    if (!c) {
        ret(ctx, CELL_ESRCH);
        return;
    }
    {
        std::lock_guard<std::mutex> lk(c->mutex->m);
        ++c->signals;
    }
    if (all) c->cv.notify_all();
    else c->cv.notify_one();
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(sysPrxForUser, sys_lwcond_signal) { lwcond_signal(ctx, false); }
OPENTIT_HLE(sysPrxForUser, sys_lwcond_signal_all) { lwcond_signal(ctx, true); }

// ---------------------------------------------------------------- mmapper
namespace {
std::mutex g_mm_mutex;
std::unordered_map<u32, u32> g_mm_blocks;  // mem_id -> size
std::unordered_map<u32, u32> g_mm_mapped;  // addr -> mem_id
u32 g_mm_next = 0x1D000100;
}  // namespace

OPENTIT_HLE(sysPrxForUser, sys_mmapper_allocate_memory) {
    // (size, flags, u32* mem_id)
    std::lock_guard<std::mutex> lk(g_mm_mutex);
    const u32 id = g_mm_next += 0x100;
    g_mm_blocks[id] = arg32(ctx, 0);
    mem::w32(arg32(ctx, 2), id);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_mmapper_map_memory) {
    // (addr, mem_id, flags)
    std::lock_guard<std::mutex> lk(g_mm_mutex);
    auto it = g_mm_blocks.find(arg32(ctx, 1));
    if (it == g_mm_blocks.end()) {
        ret(ctx, CELL_ESRCH);
        return;
    }
    mem::commit(arg32(ctx, 0), it->second);
    g_mm_mapped[arg32(ctx, 0)] = it->first;
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_mmapper_unmap_memory) {
    // (addr, u32* mem_id)
    std::lock_guard<std::mutex> lk(g_mm_mutex);
    auto it = g_mm_mapped.find(arg32(ctx, 0));
    if (it == g_mm_mapped.end()) {
        ret(ctx, CELL_EINVAL);
        return;
    }
    if (arg32(ctx, 1)) mem::w32(arg32(ctx, 1), it->second);
    g_mm_mapped.erase(it);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sysPrxForUser, sys_mmapper_free_memory) {
    std::lock_guard<std::mutex> lk(g_mm_mutex);
    ret(ctx, g_mm_blocks.erase(arg32(ctx, 0)) ? CELL_OK : CELL_ESRCH);
}
