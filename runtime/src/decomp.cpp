#include "opentit/decomp.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

#include "runtime.h"

namespace opentit::decomp {

namespace {
std::atomic<bool> g_all_off{false};
std::mutex g_m;
std::unordered_set<u32>& off_set() {
    static std::unordered_set<u32> s;
    return s;
}
std::atomic<bool> g_any_off{false};
}  // namespace

bool enabled(u32 addr) {
    if (g_all_off.load(std::memory_order_relaxed)) return false;
    if (!g_any_off.load(std::memory_order_relaxed)) return true;
    std::lock_guard<std::mutex> lk(g_m);
    return !off_set().count(addr);
}
void disable_all() { g_all_off = true; }
void disable(u32 addr) {
    std::lock_guard<std::mutex> lk(g_m);
    off_set().insert(addr);
    g_any_off = true;
}

u64 call(PPUContext& ctx, u32 addr, std::initializer_list<u64> args) {
    int r = 3;
    for (u64 a : args) ctx.gpr[r++] = a;
    const u64 saved_lr = ctx.lr;
    call_indirect(ctx, addr, 0);
    ctx.lr = saved_lr;
    return ctx.gpr[3];
}

f64 callf(PPUContext& ctx, u32 addr, std::initializer_list<u64> args, std::initializer_list<f64> fargs) {
    int f = 1;
    for (f64 a : fargs) ctx.fpr[f++] = a;
    call(ctx, addr, args);
    return ctx.fpr[1];
}

u64 call_ptr(PPUContext& ctx, u32 opd, std::initializer_list<u64> args) {
    const u64 saved_r2 = ctx.gpr[2];
    ctx.gpr[2] = mem::r32(opd + 4);
    const u64 r = call(ctx, mem::r32(opd), args);
    ctx.gpr[2] = saved_r2;
    return r;
}

Stack::Stack(PPUContext& ctx, u32 bytes) : ctx_(ctx), saved_r1_(ctx.gpr[1]) {
    // Space below a fresh 112-byte linkage area so callees can use their own frames.
    const u32 size = ((bytes + 15) & ~15u) + 112;
    const u32 sp = (static_cast<u32>(saved_r1_) - size) & ~15u;
    mem::w32(sp, static_cast<u32>(saved_r1_));  // back-chain
    ctx_.gpr[1] = sp;
    base_ = sp + 112;
}

Stack::~Stack() { ctx_.gpr[1] = saved_r1_; }

}  // namespace opentit::decomp
