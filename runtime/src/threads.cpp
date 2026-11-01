// PPU threads: each guest thread runs recompiled code on its own host thread.
#include <cstdlib>
#include <thread>

#include "kernel.h"
#include "log.h"
#include "opentit/memory.h"
#include "opentit/spu_recomp.h"

namespace opentit {

namespace {
thread_local PPUContext* t_ctx = nullptr;
thread_local u32 t_thread_id = 0;
std::mutex g_threads_mutex;
std::unordered_map<u32, std::shared_ptr<kernel::Thread>> g_threads;
std::atomic<u32> g_next_thread_id{0x01000100};  // 0x01000000 is the main thread
}  // namespace

namespace rtm {

PPUContext* current_ctx() { return t_ctx; }
void set_current_ctx(PPUContext* ctx) { t_ctx = ctx; }

void exit_process(int code) {
    LOG_INFO("kernel", "process exit (%d)", code);
    spu::flush_recording();
    write_exit_dump();
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(code);
}

}  // namespace rtm

namespace kernel {

u32 current_thread_id() { return t_thread_id; }

std::shared_ptr<Thread> thread_by_id(u32 id) {
    std::lock_guard<std::mutex> lk(g_threads_mutex);
    auto it = g_threads.find(id);
    return it == g_threads.end() ? nullptr : it->second;
}

void register_main_thread(PPUContext& ctx, u32 id) {
    t_ctx = &ctx;
    t_thread_id = id;
    ctx.thread_id = id;
}

u32 create_thread(u32 entry_opd, u64 arg, s32 prio, u32 stack_size, bool joinable, const std::string& name,
                  bool start_now) {
    auto th = std::make_shared<Thread>();
    th->id = g_next_thread_id.fetch_add(1);
    th->name = name;
    th->joinable = joinable;
    th->priority = prio;
    th->ctx = std::make_unique<PPUContext>();
    PPUContext& ctx = *th->ctx;

    if (stack_size < 0x4000) stack_size = 0x4000;
    ctx.stack_size = stack_size;
    ctx.stack_addr = rtm::alloc_stack(stack_size);
    if (!ctx.stack_addr) {
        LOG_ERROR("thread", "out of stack space creating '%s'", name.c_str());
        return 0;
    }
    ctx.gpr[1] = ctx.stack_addr + stack_size - 0x70;
    ctx.gpr[3] = arg;
    const u32 tls = rtm::alloc_tls();
    ctx.gpr[13] = tls + 0x7030;
    ctx.thread_id = th->id;

    {
        std::lock_guard<std::mutex> lk(g_threads_mutex);
        g_threads[th->id] = th;
    }
    LOG_INFO("thread", "create '%s' id=0x%08X entry=0x%08X arg=0x%llX stack=0x%08X+0x%X", name.c_str(), th->id,
             entry_opd, (unsigned long long)arg, ctx.stack_addr, stack_size);

    auto body = [th, entry_opd] {
        t_ctx = th->ctx.get();
        t_thread_id = th->id;
        u64 status = 0;
        try {
            rtm::call_opd(*th->ctx, entry_opd);
            status = th->ctx->gpr[3];
        } catch (const rtm::ThreadExit& e) {
            status = e.code;
        }
        LOG_INFO("thread", "'%s' (0x%08X) exited with 0x%llX", th->name.c_str(), th->id, (unsigned long long)status);
        std::lock_guard<std::mutex> lk(th->m);
        th->exit_status = status;
        th->finished = true;
        th->cv.notify_all();
    };
    (void)start_now;  // threads always start immediately in this runtime
    th->host = std::thread(body);
    if (!joinable) th->host.detach();
    return th->id;
}

}  // namespace kernel

}  // namespace opentit
