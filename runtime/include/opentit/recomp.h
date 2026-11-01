// Header included by every generated translation unit.
//
// It defines the ABI between recompiled guest functions and the runtime:
//   * every guest function becomes `void f_XXXXXXXX(PPUContext&)`
//   * direct calls are direct C++ calls
//   * indirect calls / unresolved jumps go through call_indirect()
//   * `sc` goes through ppu_syscall()
#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "common/types.h"
#include "opentit/memory.h"
#include "opentit/ppu_context.h"
#include "opentit/vmx.h"

namespace opentit {

using GuestFn = void (*)(PPUContext&);

struct FunctionEntry {
    u32 addr;
    GuestFn fn;
};

// Provided by the generated function table (ppu_table.cpp).
extern const FunctionEntry g_recompiled_functions[];
extern const std::size_t g_recompiled_function_count;

// Provided by the runtime.
void call_indirect(PPUContext& ctx, u32 target, u32 from);
void ppu_syscall(PPUContext& ctx, u32 pc);
[[noreturn]] void illegal_instruction(PPUContext& ctx, u32 pc, u32 raw);
void trap(PPUContext& ctx, u32 pc);
u64 read_timebase();

namespace rt {

inline u32 rotl32(u32 x, u32 n) {
    n &= 31;
    return n ? (x << n) | (x >> (32 - n)) : x;
}
inline u64 rotl64(u64 x, u32 n) {
    n &= 63;
    return n ? (x << n) | (x >> (64 - n)) : x;
}
inline u64 dup32(u32 x) { return (u64(x) << 32) | x; }

inline u32 clz32(u32 x) {
#if defined(_MSC_VER)
    unsigned long i;
    return _BitScanReverse(&i, x) ? 31 - i : 32;
#else
    return x ? static_cast<u32>(__builtin_clz(x)) : 32;
#endif
}
inline u32 clz64(u64 x) {
#if defined(_MSC_VER)
    unsigned long i;
    return _BitScanReverse64(&i, x) ? 63 - i : 64;
#else
    return x ? static_cast<u32>(__builtin_clzll(x)) : 64;
#endif
}

// Record LT/GT/EQ/SO into CR field n from a signed comparison.
template <typename T>
inline void cr_cmp(PPUContext& ctx, int n, T a, T b) {
    u8* c = ctx.cr + n * 4;
    c[0] = a < b;
    c[1] = a > b;
    c[2] = a == b;
    c[3] = ctx.xer_so;
}
inline void cr0(PPUContext& ctx, u64 v) { cr_cmp<s64>(ctx, 0, static_cast<s64>(v), 0); }

// a + b + carry_in, producing carry out.
inline u64 add_ca(u64 a, u64 b, u8 cin, u8& cout) {
    const u64 s = a + b;
    const u64 r = s + cin;
    cout = static_cast<u8>((s < a) | (r < s));
    return r;
}
inline void set_ov(PPUContext& ctx, bool ov) {
    ctx.xer_ov = ov;
    ctx.xer_so |= static_cast<u8>(ov);
}
inline bool add_overflow(u64 a, u64 b, u64 r) { return ((a ^ r) & (b ^ r)) >> 63; }

// Shifts with PowerPC semantics.
inline u64 slw(u64 s, u64 b) { const u32 n = b & 63; return n > 31 ? 0 : u64(u32(s << n)); }
inline u64 srw(u64 s, u64 b) { const u32 n = b & 63; return n > 31 ? 0 : u64(u32(s) >> n); }
inline u64 sld(u64 s, u64 b) { const u32 n = b & 127; return n > 63 ? 0 : s << n; }
inline u64 srd(u64 s, u64 b) { const u32 n = b & 127; return n > 63 ? 0 : s >> n; }
inline u64 sraw(PPUContext& ctx, u64 s, u64 b) {
    const u32 n = b & 63;
    const s32 v = static_cast<s32>(s);
    if (n > 31) {
        ctx.xer_ca = v < 0;
        return static_cast<u64>(s64(v < 0 ? -1 : 0));
    }
    const s32 r = v >> n;
    ctx.xer_ca = (v < 0) && (u32(v) & ((1u << n) - 1)) != 0;
    return static_cast<u64>(s64(r));
}
inline u64 srad(PPUContext& ctx, u64 s, u64 b) {
    const u32 n = b & 127;
    const s64 v = static_cast<s64>(s);
    if (n > 63) {
        ctx.xer_ca = v < 0;
        return static_cast<u64>(v < 0 ? -1 : 0);
    }
    const s64 r = n ? (v >> n) : v;
    ctx.xer_ca = (v < 0) && n && (u64(v) & ((u64(1) << n) - 1)) != 0;
    return static_cast<u64>(r);
}

// Integer division with the PowerPC "undefined result" cases made safe.
inline u64 divw(u64 a, u64 b) {
    const s32 x = s32(a), y = s32(b);
    if (y == 0 || (x == s32(0x80000000) && y == -1)) return 0;
    return u64(u32(x / y));
}
inline u64 divwu(u64 a, u64 b) { return u32(b) ? u64(u32(a) / u32(b)) : 0; }
inline u64 divd(u64 a, u64 b) {
    const s64 x = s64(a), y = s64(b);
    if (y == 0 || (x == s64(0x8000000000000000ull) && y == -1)) return 0;
    return u64(x / y);
}
inline u64 divdu(u64 a, u64 b) { return b ? a / b : 0; }

inline u64 mulhd(u64 a, u64 b) {
#if defined(_MSC_VER)
    return static_cast<u64>(__mulh(static_cast<s64>(a), static_cast<s64>(b)));
#else
    return static_cast<u64>((static_cast<__int128>(static_cast<s64>(a)) * static_cast<s64>(b)) >> 64);
#endif
}
inline u64 mulhdu(u64 a, u64 b) {
#if defined(_MSC_VER)
    return __umulh(a, b);
#else
    return static_cast<u64>((static_cast<unsigned __int128>(a) * b) >> 64);
#endif
}

inline bool mulld_overflow(u64 a, u64 b) {
    const u64 lo = a * b;
    const u64 hi = mulhd(a, b);
    return hi != static_cast<u64>(static_cast<s64>(lo) >> 63);
}

// Trap condition: TO bits 0..4 = lt, gt, eq, ltu, gtu
inline bool trap_cond(u32 to, s64 a, s64 b) {
    return ((to & 0x10) && a < b) || ((to & 0x08) && a > b) || ((to & 0x04) && a == b) ||
           ((to & 0x02) && u64(a) < u64(b)) || ((to & 0x01) && u64(a) > u64(b));
}

// ---- floating point
inline void fcmp(PPUContext& ctx, int n, f64 a, f64 b) {
    u8* c = ctx.cr + n * 4;
    const bool un = std::isnan(a) || std::isnan(b);
    c[0] = !un && a < b;
    c[1] = !un && a > b;
    c[2] = !un && a == b;
    c[3] = un;
    ctx.fpscr = (ctx.fpscr & ~0xF000u) | (u32(c[0]) << 15) | (u32(c[1]) << 14) | (u32(c[2]) << 13) | (u32(un) << 12);
}
inline f64 to_single(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

inline u64 fctiw(f64 x, bool truncate) {
    if (std::isnan(x)) return 0x80000000u;
    const f64 r = truncate ? std::trunc(x) : std::nearbyint(x);
    if (r >= 2147483647.0) return 0x7FFFFFFFu;
    if (r <= -2147483648.0) return 0x80000000u;
    return u64(u32(s32(r)));
}
inline u64 fctid(f64 x, bool truncate) {
    if (std::isnan(x)) return 0x8000000000000000ull;
    const f64 r = truncate ? std::trunc(x) : std::nearbyint(x);
    if (r >= 9223372036854775807.0) return 0x7FFFFFFFFFFFFFFFull;
    if (r <= -9223372036854775808.0) return 0x8000000000000000ull;
    return u64(s64(r));
}
inline f64 fcfid(u64 bits) { return static_cast<f64>(static_cast<s64>(bits)); }
inline f64 fsel(f64 a, f64 b, f64 c) { return (a >= 0.0) ? c : b; }
inline void fpr_record(PPUContext& ctx) {
    // CR1 <- FPSCR[FX, FEX, VX, OX]
    ctx.cr[4] = (ctx.fpscr >> 31) & 1;
    ctx.cr[5] = (ctx.fpscr >> 30) & 1;
    ctx.cr[6] = (ctx.fpscr >> 29) & 1;
    ctx.cr[7] = (ctx.fpscr >> 28) & 1;
}

// ---- Cell unaligned vector loads/stores
inline v128 lvlx(u32 ea) {
    v128 r = v128::zero();
    const u32 s = ea & 15;
    for (u32 i = 0; i < 16 - s; ++i) r.ub(static_cast<int>(i)) = mem::r8(ea + i);
    return r;
}
inline v128 lvrx(u32 ea) {
    v128 r = v128::zero();
    const u32 s = ea & 15;
    const u32 base = ea & ~15u;
    for (u32 i = 0; i < s; ++i) r.ub(static_cast<int>(16 - s + i)) = mem::r8(base + i);
    return r;
}
inline void stvlx(u32 ea, const v128& v) {
    const u32 s = ea & 15;
    for (u32 i = 0; i < 16 - s; ++i) mem::w8(ea + i, v.ub(static_cast<int>(i)));
}
inline void stvrx(u32 ea, const v128& v) {
    const u32 s = ea & 15;
    const u32 base = ea & ~15u;
    for (u32 i = 0; i < s; ++i) mem::w8(base + i, v.ub(static_cast<int>(16 - s + i)));
}
inline v128 lvx(u32 ea) { return vmx::load(mem::ptr(ea & ~15u)); }
inline void stvx(u32 ea, const v128& v) { vmx::store(mem::ptr(ea & ~15u), v); }
inline void stvebx(u32 ea, const v128& v) { mem::w8(ea, v.ub(ea & 15)); }
inline void stvehx(u32 ea, const v128& v) { ea &= ~1u; mem::w16(ea, v.uh((ea & 15) >> 1)); }
inline void stvewx(u32 ea, const v128& v) { ea &= ~3u; mem::w32(ea, v.uw((ea & 15) >> 2)); }

inline void dcbz(u32 ea) { std::memset(mem::ptr(ea & ~127u), 0, 128); }

// ---- string / multiple word
inline void lswi(PPUContext& ctx, int rd, u32 ea, u32 nb) {
    if (nb == 0) nb = 32;
    int r = rd - 1;
    for (u32 i = 0; i < nb; ++i) {
        if ((i & 3) == 0) { r = (r + 1) & 31; ctx.gpr[r] = 0; }
        ctx.gpr[r] |= u64(mem::r8(ea + i)) << (24 - 8 * (i & 3));
    }
}
inline void stswi(PPUContext& ctx, int rs, u32 ea, u32 nb) {
    if (nb == 0) nb = 32;
    int r = rs - 1;
    for (u32 i = 0; i < nb; ++i) {
        if ((i & 3) == 0) r = (r + 1) & 31;
        mem::w8(ea + i, static_cast<u8>(ctx.gpr[r] >> (24 - 8 * (i & 3))));
    }
}

// ---- reservations
inline u64 lwarx(PPUContext& ctx, u32 ea) {
    ctx.reserve_addr = ea;
    const u32 v = mem::r32(ea);
    ctx.reserve_value = v;
    return v;
}
inline u64 ldarx(PPUContext& ctx, u32 ea) {
    ctx.reserve_addr = ea;
    const u64 v = mem::r64(ea);
    ctx.reserve_value = v;
    return v;
}
inline void stwcx(PPUContext& ctx, u32 ea, u32 v) {
    const bool ok = ctx.reserve_addr == ea && mem::cas32(ea, static_cast<u32>(ctx.reserve_value), v);
    ctx.reserve_addr = 0xFFFFFFFF;
    ctx.cr[0] = 0; ctx.cr[1] = 0; ctx.cr[2] = ok; ctx.cr[3] = ctx.xer_so;
}
inline void stdcx(PPUContext& ctx, u32 ea, u64 v) {
    const bool ok = ctx.reserve_addr == ea && mem::cas64(ea, ctx.reserve_value, v);
    ctx.reserve_addr = 0xFFFFFFFF;
    ctx.cr[0] = 0; ctx.cr[1] = 0; ctx.cr[2] = ok; ctx.cr[3] = ctx.xer_so;
}

// ---- FPSCR helpers
inline void mtfsf(PPUContext& ctx, u32 fm, u64 bits) {
    u32 mask = 0;
    for (int i = 0; i < 8; ++i)
        if (fm & (0x80 >> i)) mask |= 0xF0000000u >> (i * 4);
    ctx.fpscr = (ctx.fpscr & ~mask) | (static_cast<u32>(bits) & mask);
}

}  // namespace rt

}  // namespace opentit

// Shorthand used by generated code.
#define OPENTIT_FN(addr) void f_##addr(::opentit::PPUContext& ctx)
