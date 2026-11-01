// The decomp mechanism: a hand-written function replaces the recompiled one
// for direct calls, indirect calls and tail calls, and can be switched off.
#include "opentit/decomp.h"
#include "opentit/memory.h"
#include "../runtime/src/runtime.h"
#include "test.h"
#include "testgen_layout.h"

using namespace opentit;
using namespace testgen;

extern int g_decomp_sum_calls;

namespace {
u64 run(Fn f, u64 r3) {
    static PPUContext c;
    static bool init = false;
    if (!init) {
        c.stack_size = 0x10000;
        c.stack_addr = rtm::alloc_stack(c.stack_size);
        init = true;
    }
    c.gpr[1] = c.stack_addr + c.stack_size - 0x100;
    c.gpr[3] = r3;
    rtm::call_opd(c, opd(f));
    return c.gpr[3];
}
}  // namespace

TEST(DecompReplacesRecompiledFunction) {
    const int before = g_decomp_sum_calls;
    CHECK_EQ(run(SumToN, 100), 5050u);             // via the function table
    CHECK_EQ(run(TailCall, 9), 55u);               // TailCall branches to SumToN(n + 1)
    CHECK_EQ(run(Indirect, 20), 210u);             // bctrl through SumToN's descriptor
    CHECK_EQ(g_decomp_sum_calls, before + 3);

    decomp::disable(0x00010000);                   // A/B: back to the recompiled original
    CHECK_EQ(run(SumToN, 100), 5050u);
    CHECK_EQ(g_decomp_sum_calls, before + 3);
}

TEST(DecompBigEndianViews) {
    struct Thing {
        decomp::be_u32 flags;
        decomp::be_f32 x;
        decomp::be_u16 id;
        u8 pad[2];
    };
    static_assert(sizeof(Thing) == 12);
    const u32 ea = testgen::kScratch + 0x800;
    mem::w32(ea, 0x11223344);
    mem::wf32(ea + 4, 2.5f);
    mem::w16(ea + 8, 0xBEEF);
    auto& t = decomp::ref<Thing>(ea);
    CHECK_EQ(static_cast<u32>(t.flags), 0x11223344u);
    CHECK_EQ(static_cast<f32>(t.x), 2.5f);
    CHECK_EQ(static_cast<u16>(t.id), 0xBEEFu);
    t.flags |= 0x80000000u;
    t.x = 1.0f;
    CHECK_EQ(mem::r32(ea), 0x91223344u);
    CHECK_EQ(mem::rf32(ea + 4), 1.0f);
    CHECK_EQ(decomp::load<u16>(ea + 8), 0xBEEFu);
}
