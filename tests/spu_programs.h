// SPU test programs shared by the SPU code generator step (spu_testgen) and
// the tests that run them recompiled.
#pragma once

#include <vector>

#include "common/types.h"

namespace spu_programs {

using opentit::s32;
using opentit::u32;

inline u32 rr(u32 op11, u32 rt, u32 ra, u32 rb) { return (op11 << 21) | ((rb & 0x7F) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
inline u32 ri10(u32 op8, u32 rt, u32 ra, s32 i10) { return (op8 << 24) | ((u32(i10) & 0x3FF) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
inline u32 ri16(u32 op9, u32 rt, s32 i16) { return (op9 << 23) | ((u32(i16) & 0xFFFF) << 7) | (rt & 0x7F); }
inline u32 rrr(u32 op4, u32 rt, u32 ra, u32 rb, u32 rc) { return (op4 << 28) | ((rt & 0x7F) << 21) | ((rb & 0x7F) << 14) | ((ra & 0x7F) << 7) | (rc & 0x7F); }
inline u32 IL(u32 rt, s32 v) { return ri16(0x081, rt, v); }
inline u32 ILHU(u32 rt, u32 v) { return ri16(0x082, rt, static_cast<s32>(v)); }
inline u32 IOHL(u32 rt, u32 v) { return ri16(0x0C1, rt, static_cast<s32>(v)); }
inline u32 A(u32 rt, u32 ra, u32 rb) { return rr(0x0C0, rt, ra, rb); }
inline u32 AI(u32 rt, u32 ra, s32 i) { return ri10(0x1C, rt, ra, i); }
inline u32 BRNZ(u32 rt, s32 words) { return ri16(0x042, rt, words); }
inline u32 BRSL(u32 rt, s32 words) { return ri16(0x066, rt, words); }
inline u32 BI(u32 ra) { return rr(0x1A8, 0, ra, 0); }
inline u32 STOP(u32 code) { return code & 0x3FFF; }
inline u32 SHUFB(u32 rt, u32 ra, u32 rb, u32 rc) { return rrr(0xB, rt, ra, rb, rc); }

constexpr u32 kLoopAt = 0x1000;   // sum loop with a shuffle, ends in STOP 1
constexpr u32 kCallAt = 0x2000;   // calls a function 1000 times, ends in STOP 2
constexpr u32 kFuncAt = 0x3000;
constexpr u32 kLoopCount = 20000000;

inline std::vector<u32> loop_program() {
    return {ILHU(5, kLoopCount >> 16), IOHL(5, kLoopCount & 0xFFFF), IL(3, 0),
            A(3, 3, 5), AI(6, 3, 7), SHUFB(7, 6, 3, 5), AI(5, 5, -1), BRNZ(5, -4), STOP(1)};
}

// $3 = 3 * 1000 via a called function; $8 counts loop iterations.
inline std::vector<u32> call_program() {
    const s32 to_func = static_cast<s32>((kFuncAt - (kCallAt + 12)) / 4);
    return {IL(3, 0), IL(4, 1000), IL(8, 0),
            BRSL(0, to_func), AI(8, 8, 1), AI(4, 4, -1), BRNZ(4, -3), STOP(2)};
}
inline std::vector<u32> func_program() { return {AI(3, 3, 3), BI(0)}; }

}  // namespace spu_programs
