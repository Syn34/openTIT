// Indirect-call dispatcher, trap/illegal handlers and the timebase.
#include <chrono>
#include <cstdlib>
#include <vector>

#include "log.h"
#include "opentit/recomp.h"
#include "runtime.h"

namespace opentit {

namespace rtm {

namespace {
const loader::Elf* g_elf = nullptr;
Config g_config;

// Flat table indexed by (addr - g_lo) / 4: one load per indirect call.
std::vector<GuestFn> g_flat;
u32 g_lo = 0;
}  // namespace

Config& config() { return g_config; }
const loader::Elf& elf() { return *g_elf; }
void set_elf(const loader::Elf* e) { g_elf = e; }

void init_dispatch(const FunctionEntry* table, std::size_t count) {
    if (!count) return;
    u32 lo = 0xFFFFFFFF, hi = 0;
    for (std::size_t i = 0; i < count; ++i) {
        lo = std::min(lo, table[i].addr);
        hi = std::max(hi, table[i].addr);
    }
    g_lo = lo;
    g_flat.assign((hi - lo) / 4 + 1, nullptr);
    for (std::size_t i = 0; i < count; ++i) g_flat[(table[i].addr - lo) / 4] = table[i].fn;
    LOG_INFO("dispatch", "%zu recompiled functions (0x%08X..0x%08X)", count, lo, hi);
}

GuestFn lookup(u32 addr) {
    const u32 idx = (addr - g_lo) / 4;
    if (addr < g_lo || idx >= g_flat.size() || (addr & 3)) return nullptr;
    return g_flat[idx];
}

void call_code(PPUContext& ctx, u32 code_addr, u32 toc) {
    ctx.gpr[2] = toc;
    call_indirect(ctx, code_addr, 0);
}

void call_opd(PPUContext& ctx, u32 opd) { call_code(ctx, mem::r32(opd), mem::r32(opd + 4)); }

}  // namespace rtm

void call_indirect(PPUContext& ctx, u32 target, u32 from) {
    if (GuestFn fn = rtm::lookup(target)) {
        fn(ctx);
        return;
    }
    if (rtm::hle_try_call(ctx, target)) return;
    LOG_ERROR("dispatch",
              "indirect branch to 0x%08X (from 0x%08X) has no recompiled function.\n"
              "  If this is a real entry point, add 0x%08X to your --entries file and re-run recompile.",
              target, from, target);
    std::abort();
}

void illegal_instruction(PPUContext& ctx, u32 pc, u32 raw) {
    LOG_ERROR("ppu", "illegal instruction 0x%08X at 0x%08X (lr=0x%08llX)", raw, pc, (unsigned long long)ctx.lr);
    std::abort();
}

void trap(PPUContext& ctx, u32 pc) {
    LOG_ERROR("ppu", "trap at 0x%08X (r3=0x%llX lr=0x%08llX)", pc, (unsigned long long)ctx.gpr[3],
              (unsigned long long)ctx.lr);
    std::abort();
}

u64 read_timebase() {
    // PS3 timebase runs at 79.8 MHz.
    static const auto t0 = std::chrono::steady_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
    return static_cast<u64>(ns) * 798 / 10000;
}

}  // namespace opentit
