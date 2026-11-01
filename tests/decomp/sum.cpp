// A decompiled version of the synthetic test program's SumToN (testgen.cpp):
//     r3 = 1 + 2 + ... + r3   (a ctr loop in the original)
#include "opentit/decomp.h"

using namespace opentit;
using namespace opentit::decomp;

int g_decomp_sum_calls = 0;

DECOMP(00010000, SumToN) {
    ++g_decomp_sum_calls;
    const u64 n = arg<u64>(ctx, 0);
    ret(ctx, n * (n + 1) / 2);
}
