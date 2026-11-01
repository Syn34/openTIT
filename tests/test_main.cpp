// End-to-end tests: the synthetic ELF from testgen.cpp has been recompiled
// to C++ at build time; here we load it into guest memory and run it.
#include <chrono>
#include <cstring>
#include <optional>

#include "../runtime/src/kernel.h"
#include "../runtime/src/runtime.h"
#include "analysis/functions.h"
#include "loader/elf.h"
#include "opentit/memory.h"
#include "opentit/recomp.h"
#include "test.h"
#include "testgen_layout.h"

using namespace opentit;
using namespace testgen;

namespace {

std::optional<loader::Elf> g_elf;
PPUContext g_ctx;

PPUContext& fresh() {
    PPUContext& c = g_ctx;
    const u32 stack = c.stack_addr, size = c.stack_size;
    c = PPUContext{};
    c.stack_addr = stack;
    c.stack_size = size;
    c.gpr[1] = stack + size - 0x100;
    return c;
}

u64 call(Fn f, PPUContext& c) {
    rtm::call_opd(c, opd(f));
    return c.gpr[3];
}

u64 ref_sum(u64 n) { return n * (n + 1) / 2; }

u32 ref_clz32(u32 x) {
    u32 n = 0;
    for (u32 bit = 0x80000000u; bit && !(x & bit); bit >>= 1) ++n;
    return n;
}

}  // namespace

TEST(analysis_finds_all_functions_and_jump_table) {
    analysis::Options o;
    const auto prog = analysis::analyze(*g_elf, o);
    CHECK_EQ(prog.functions.size(), static_cast<std::size_t>(Count));
    std::size_t tables = 0, targets = 0;
    for (const auto& [a, fn] : prog.functions) {
        tables += fn.jump_tables.size();
        for (const auto& [_, jt] : fn.jump_tables) targets += jt.targets.size();
        CHECK(fn.invalid.empty());
    }
    CHECK_EQ(tables, 1u);
    CHECK_EQ(targets, 4u);
}

TEST(ctr_loop_and_conditional_return) {
    for (u64 n : {0ull, 1ull, 10ull, 1000ull}) {
        auto& c = fresh();
        c.gpr[3] = n;
        CHECK_EQ(call(SumToN, c), ref_sum(n));
    }
}

TEST(ppu_recompiled_speed) {
    // Not a correctness check: prints the throughput of a recompiled ctr loop (MemOps adds stores).
    auto& c = fresh();
    const u64 n = 100000000;
    c.gpr[3] = n;
    const auto t0 = std::chrono::steady_clock::now();
    const u64 r = call(SumToN, c);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    CHECK_EQ(r, ref_sum(n));
    std::printf("  PPU recompiled SumToN: %.0f M iterations/s\n", n / s / 1e6);
}

TEST(rotate_mask_shift_arith) {
    const u32 out = kScratch;
    for (u64 x : {0x12345678ull, 0xFFFFFFFF80000001ull, 0x00000000DEADBEEFull, 0x7ull, 0xFFFFFFFFFFFFFFF9ull}) {
        auto& c = fresh();
        c.gpr[3] = x;
        c.gpr[4] = out;
        call(Bits, c);
        const u32 lo = static_cast<u32>(x);
        const u32 rot8 = (lo << 8) | (lo >> 24);
        CHECK_EQ(mem::r64(out + 0), u64(rot8 & 0xFF));                               // rlwinm 8,24,31
        CHECK_EQ(mem::r64(out + 8), (x >> 32) | 0);                                   // rldicl 32,32 = x >> 32
        CHECK_EQ(mem::r64(out + 16), u64(s64(x) >> 4));                               // sradi 4
        CHECK_EQ(mem::r64(out + 24), u64(s64(s32(lo) / 8)));                          // srawi+addze = /8
        CHECK_EQ(mem::r64(out + 32), u64(ref_clz32(lo)));               // cntlzw
        CHECK_EQ(mem::r64(out + 40), u64(s64(s16(lo))));                              // extsh
        CHECK_EQ(mem::r64(out + 48), u64(0) - x);                                     // neg
        CHECK_EQ(mem::r64(out + 56), (u64(lo) * u64(lo)) >> 32);                      // mulhwu
        CHECK_EQ(u32(mem::r64(out + 64)), u32(s32(lo) / 7));                          // divw
        const u32 mask = 0x00FF0000u;                                                  // MASK(8,15)
        const u32 r4 = (lo << 4) | (lo >> 28);
        CHECK_EQ(mem::r64(out + 72), (0xFFFFFFFF00000000ull | ((r4 & mask) | (0xFFFFFFFFu & ~mask))) & 0xFFFFFFFFFFFFFFFFull);
        CHECK_EQ(mem::r64(out + 80), 0ull);                                           // slw by 33
        CHECK_EQ(mem::r64(out + 88), u64(lo >> 4));                                   // srw by 4
    }
}

TEST(load_store_widths_and_update_forms) {
    auto& c = fresh();
    const u32 buf = kScratch + 0x200;
    std::memset(mem::ptr(buf), 0, 64);
    c.gpr[3] = buf;
    const u64 r = call(MemOps, c);
    const u64 expect = 0x1234 + 0xFE + u64(-32768) + 0x8000 + 0x55 + 0x55;
    CHECK_EQ(r, expect);
    CHECK_EQ(mem::r32(buf + 16), 0x55u);
    CHECK_EQ(c.gpr[10], u64(buf + 16));  // stwu updated the base register
}

TEST(nested_calls_with_stack_frame) {
    auto& c = fresh();
    const u64 sp = c.gpr[1];
    c.gpr[3] = 50;
    c.gpr[31] = 0xABCDEF;
    CHECK_EQ(call(CallChain, c), ref_sum(50) + ref_sum(49));
    CHECK_EQ(c.gpr[1], sp);          // stack restored
    CHECK_EQ(c.gpr[31], 0xABCDEFull);  // callee-saved restored
}

TEST(jump_table_switch) {
    const s64 expect[] = {100, 200, 300, 400};
    for (u64 k = 0; k < 6; ++k) {
        auto& c = fresh();
        c.gpr[2] = kTocBase;
        c.gpr[3] = k;
        CHECK_EQ(s64(call(Switch, c)), k < 4 ? expect[k] : -1);
    }
}

TEST(floating_point) {
    auto& c = fresh();
    c.fpr[1] = 3.75;
    c.fpr[2] = -2.5;
    CHECK_EQ(s32(call(Fp, c)), s32(3.75 * -2.5 + 3.75));
    const f32 single = static_cast<f32>(static_cast<f32>(3.75f * -2.5f) + -2.5f);
    CHECK_EQ(mem::rf32(kScratch + 0x100), single);
}

TEST(vmx_add_rotate_permute) {
    auto& c = fresh();
    const u32 in = kScratch + 0x400, out = kScratch + 0x500;
    const f32 vals[4] = {1.5f, -2.0f, 10.0f, 0.25f};
    for (int k = 0; k < 4; ++k) mem::wf32(in + 4 * k, vals[k]);
    c.gpr[3] = in;
    c.gpr[4] = out;
    call(Vec, c);
    // vsldoi by 4 bytes rotates words left by one.
    for (int k = 0; k < 4; ++k) CHECK_EQ(mem::rf32(out + 4 * k), vals[(k + 1) % 4] + 1.0f);
    // vperm with lvsl(3) = bytes in[3..18] of in||in
    for (int k = 0; k < 16; ++k) CHECK_EQ(mem::r8(out + 16 + k), mem::r8(in + (k + 3) % 16));
}

TEST(carry_chain_128bit) {
    auto& c = fresh();
    c.gpr[3] = 0x1;
    c.gpr[4] = 0xFFFFFFFFFFFFFFFFull;
    c.gpr[5] = 0x2;
    c.gpr[6] = 0x1;
    call(Add128, c);
    CHECK_EQ(c.gpr[3], 0x4ull);  // 1 + 2 + carry
    CHECK_EQ(c.gpr[4], 0x0ull);
    CHECK_EQ(c.gpr[5], u64(-1));  // (1:FFFF..) - (2:1) = (-1 : FFFF..FE)
    CHECK_EQ(c.gpr[6], 0xFFFFFFFFFFFFFFFEull);
}

TEST(condition_register_logic) {
    struct In { s32 a, b; };
    for (In in : {In{-5, 3}, In{4, 4}, In{7, -1}, In{0, 0}}) {
        auto& c = fresh();
        c.gpr[3] = static_cast<u64>(static_cast<s64>(in.a));
        c.gpr[4] = static_cast<u64>(static_cast<s64>(in.b));
        const u32 cr = static_cast<u32>(call(CrLogic, c));
        auto bit = [&](int b) { return (cr >> (31 - b)) & 1; };
        CHECK_EQ(bit(0), u32(in.a < in.b));
        CHECK_EQ(bit(1), u32(in.a > in.b));
        CHECK_EQ(bit(2), u32(in.a == in.b));
        CHECK_EQ(bit(4), u32(in.a < 0));
        CHECK_EQ(bit(6), u32(in.a == 0));
        CHECK_EQ(bit(8), u32(u32(in.a) < u32(in.b)));
        CHECK_EQ(bit(9), u32(u32(in.a) > u32(in.b)));
        CHECK_EQ(bit(31), u32(in.a < in.b && in.a < 0));
        CHECK_EQ(bit(30), u32(!(in.a == in.b || in.a == 0)));
    }
}

TEST(syscall_dispatch) {
    auto& c = fresh();
    CHECK_EQ(call(Syscall, c), 79800000ull);
}

TEST(tail_call) {
    auto& c = fresh();
    c.gpr[3] = 9;
    CHECK_EQ(call(TailCall, c), ref_sum(10));
}

TEST(reservation_atomic_increment) {
    auto& c = fresh();
    const u32 w = kScratch + 0x600;
    mem::w32(w, 41);
    c.gpr[3] = w;
    CHECK_EQ(call(AtomicInc, c), 42ull);
    CHECK_EQ(mem::r32(w), 42u);
}

TEST(indirect_call_through_descriptor) {
    auto& c = fresh();
    c.gpr[2] = kTocBase;
    c.gpr[3] = 20;
    CHECK_EQ(call(Indirect, c), ref_sum(20));
    CHECK_EQ(c.gpr[2], u64(kTocBase));
}

TEST(entry_above_shared_body) {
    auto& c = fresh();
    c.gpr[3] = 10;
    CHECK_EQ(call(BackEntry, c), 16ull);
}

int main() {
    std::string err;
    g_elf = loader::Elf::load(OPENTIT_TEST_ELF, &err);
    if (!g_elf) {
        std::printf("cannot load test ELF: %s\n", err.c_str());
        return 1;
    }
    rtm::set_elf(&*g_elf);
    if (!mem::init()) return 1;
    for (const auto& s : g_elf->segments()) {
        if (s.type != loader::PT_LOAD || !s.memsz) continue;
        mem::commit(static_cast<u32>(s.vaddr), static_cast<u32>(s.memsz));
        std::memcpy(mem::ptr(static_cast<u32>(s.vaddr)), g_elf->data().data() + s.offset, s.filesz);
    }
    rtm::kernel_init();
    rtm::init_dispatch(g_recompiled_functions, g_recompiled_function_count);
    g_ctx.stack_size = 0x40000;
    g_ctx.stack_addr = rtm::alloc_stack(g_ctx.stack_size);
    kernel::register_main_thread(g_ctx, 0x01000000);

    int failed_cases = 0;
    for (const auto& tc : t::cases()) {
        const int before = t::failures();
        tc.fn();
        const bool ok = t::failures() == before;
        failed_cases += !ok;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", tc.name);
    }
    std::printf("\n%zu tests, %d checks, %d failed checks\n", t::cases().size(), t::checks(), t::failures());
    return failed_cases ? 1 : 0;
}
