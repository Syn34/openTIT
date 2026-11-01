// Recompiled SPU code (built from tests/spu_programs.h) against the interpreter.
#include <chrono>
#include <cstdio>

#include "opentit/spu.h"
#include "opentit/spu_recomp.h"
#include "spu_programs.h"
#include "test.h"

using namespace opentit;

namespace {
void load(spu::SPUThread& t, u32 at, const std::vector<u32>& p) {
    for (std::size_t k = 0; k < p.size(); ++k) t.set_ls32(at + 4 * static_cast<u32>(k), p[k]);
}
}  // namespace

TEST(SpuRecompiledMatchesAndIsUsed) {
    CHECK(spu::registered_region_count() >= 3);
    spu::SPUThread t;
    load(t, spu_programs::kCallAt, spu_programs::call_program());
    load(t, spu_programs::kFuncAt, spu_programs::func_program());
    const u64 before = spu::dispatch_stats().region_calls;
    t.pc = spu_programs::kCallAt;
    CHECK_EQ(spu::run(t), 2u);
    CHECK_EQ(t.gpr[3].uw(0), 3000u);
    CHECK_EQ(t.gpr[8].uw(0), 1000u);
    CHECK(spu::dispatch_stats().region_calls > before + 1000);  // caller re-entered after every return

    // Same program shifted by one word no longer matches the compiled code: interpreted, same result.
    spu::SPUThread u;
    load(u, spu_programs::kFuncAt, spu_programs::func_program());
    const auto prog = spu_programs::call_program();
    load(u, spu_programs::kCallAt, prog);
    u.set_ls32(spu_programs::kCallAt + 4, spu_programs::IL(4, 10));  // patch the loop count
    const u64 v0 = spu::dispatch_stats().validations;
    u.pc = spu_programs::kCallAt;
    CHECK_EQ(spu::run(u), 2u);
    CHECK_EQ(u.gpr[3].uw(0), 30u);
    CHECK(spu::dispatch_stats().validations > v0);
}

TEST(SpuRecompiledSpeed) {
    spu::SPUThread t;
    load(t, spu_programs::kLoopAt, spu_programs::loop_program());
    t.pc = spu_programs::kLoopAt;
    const auto t0 = std::chrono::steady_clock::now();
    CHECK_EQ(spu::run(t), 1u);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  SPU recompiled: %.0f M instructions/s\n", 5.0 * spu_programs::kLoopCount / s / 1e6);
    // sum_{i=1..N} i, wrapped to 32 bits
    const u64 n = spu_programs::kLoopCount;
    CHECK_EQ(t.gpr[3].uw(0), static_cast<u32>(n * (n + 1) / 2));
}

#include <random>

#include "opentit/spu_ops.h"

TEST(SpuShufbMatchesReference) {
    std::mt19937 rng(1234);
    spu::SPUThread t;
    const u32 raw = spu_programs::SHUFB(10, 1, 2, 3);
    for (int iter = 0; iter < 2000; ++iter) {
        for (int k = 0; k < 16; ++k) {
            t.gpr[1].ub(k) = static_cast<u8>(rng());
            t.gpr[2].ub(k) = static_cast<u8>(rng());
            t.gpr[3].ub(k) = static_cast<u8>(rng());  // covers 0x80/0xC0/0xE0 selector classes
        }
        spu::ops::SHUFB(t, raw, 0);
        bool ok = true;
        for (int i = 0; i < 16; ++i) {
            const u8 s = t.gpr[3].ub(i);
            u8 want;
            if ((s & 0xC0) == 0x80) want = 0x00;
            else if ((s & 0xE0) == 0xC0) want = 0xFF;
            else if ((s & 0xE0) == 0xE0) want = 0x80;
            else want = (s & 0x10) ? t.gpr[2].ub(s & 0x0F) : t.gpr[1].ub(s & 0x0F);
            ok &= t.gpr[10].ub(i) == want;
        }
        CHECK(ok);
        if (!ok) break;
    }
}
