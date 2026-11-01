// VMX (AltiVec) register type and instruction semantics for recompiled code.
//
// Layout: a v128 holds the guest vector as one little-endian 128-bit value,
// i.e. host byte i is guest (big-endian) byte 15-i. Element-wise operations
// are therefore order-independent, and only cross-element operations
// (permutes, merges, packs, splats, loads/stores) use the *be accessors.
// This matches what SSE/NEON want, so these loops auto-vectorise well and
// can later be replaced by intrinsics without changing the generated code.
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#include "common/types.h"

namespace opentit {

struct alignas(16) v128 {
    union {
        u8 u8_[16];
        s8 s8_[16];
        u16 u16_[8];
        s16 s16_[8];
        u32 u32_[4];
        s32 s32_[4];
        u64 u64_[2];
        f32 f32_[4];
    };

    // Big-endian (guest) element accessors.
    u8& ub(int i) { return u8_[15 - i]; }
    u8 ub(int i) const { return u8_[15 - i]; }
    s8& sb(int i) { return s8_[15 - i]; }
    s8 sb(int i) const { return s8_[15 - i]; }
    u16& uh(int i) { return u16_[7 - i]; }
    u16 uh(int i) const { return u16_[7 - i]; }
    s16& sh(int i) { return s16_[7 - i]; }
    s16 sh(int i) const { return s16_[7 - i]; }
    u32& uw(int i) { return u32_[3 - i]; }
    u32 uw(int i) const { return u32_[3 - i]; }
    s32& sw(int i) { return s32_[3 - i]; }
    s32 sw(int i) const { return s32_[3 - i]; }
    f32& fw(int i) { return f32_[3 - i]; }
    f32 fw(int i) const { return f32_[3 - i]; }

    static v128 zero() {
        v128 r;
        r.u64_[0] = r.u64_[1] = 0;
        return r;
    }
};

namespace vmx {

// ---- memory (16-byte big-endian quadwords)
inline v128 load(const u8* p) {
    v128 r;
    for (int i = 0; i < 16; ++i) r.u8_[15 - i] = p[i];
    return r;
}
inline void store(u8* p, const v128& v) {
    for (int i = 0; i < 16; ++i) p[i] = v.u8_[15 - i];
}

template <typename T>
inline T sat(s64 v) {
    if (v < static_cast<s64>(std::numeric_limits<T>::min())) return std::numeric_limits<T>::min();
    if (v > static_cast<s64>(std::numeric_limits<T>::max())) return std::numeric_limits<T>::max();
    return static_cast<T>(v);
}

#define OPENTIT_VMX_MAP(name, N, field, expr)          \
    inline v128 name(const v128& a, const v128& b) {   \
        v128 r;                                        \
        for (int i = 0; i < N; ++i) {                  \
            auto x = a.field[i];                       \
            auto y = b.field[i];                       \
            r.field[i] = static_cast<decltype(x)>(expr); \
        }                                              \
        return r;                                      \
    }

// modular arithmetic
OPENTIT_VMX_MAP(vaddubm, 16, u8_, x + y)
OPENTIT_VMX_MAP(vadduhm, 8, u16_, x + y)
OPENTIT_VMX_MAP(vadduwm, 4, u32_, x + y)
OPENTIT_VMX_MAP(vsububm, 16, u8_, x - y)
OPENTIT_VMX_MAP(vsubuhm, 8, u16_, x - y)
OPENTIT_VMX_MAP(vsubuwm, 4, u32_, x - y)
OPENTIT_VMX_MAP(vaddcuw, 4, u32_, (u64(x) + u64(y)) >> 32)
OPENTIT_VMX_MAP(vsubcuw, 4, u32_, x >= y ? 1u : 0u)
// saturating
OPENTIT_VMX_MAP(vaddubs, 16, u8_, sat<u8>(s64(x) + y))
OPENTIT_VMX_MAP(vadduhs, 8, u16_, sat<u16>(s64(x) + y))
OPENTIT_VMX_MAP(vadduws, 4, u32_, sat<u32>(s64(x) + y))
OPENTIT_VMX_MAP(vaddsbs, 16, s8_, sat<s8>(s64(x) + y))
OPENTIT_VMX_MAP(vaddshs, 8, s16_, sat<s16>(s64(x) + y))
OPENTIT_VMX_MAP(vaddsws, 4, s32_, sat<s32>(s64(x) + y))
OPENTIT_VMX_MAP(vsububs, 16, u8_, sat<u8>(s64(x) - y))
OPENTIT_VMX_MAP(vsubuhs, 8, u16_, sat<u16>(s64(x) - y))
OPENTIT_VMX_MAP(vsubuws, 4, u32_, sat<u32>(s64(x) - y))
OPENTIT_VMX_MAP(vsubsbs, 16, s8_, sat<s8>(s64(x) - y))
OPENTIT_VMX_MAP(vsubshs, 8, s16_, sat<s16>(s64(x) - y))
OPENTIT_VMX_MAP(vsubsws, 4, s32_, sat<s32>(s64(x) - y))
// min / max / average
OPENTIT_VMX_MAP(vmaxub, 16, u8_, std::max(x, y))
OPENTIT_VMX_MAP(vmaxuh, 8, u16_, std::max(x, y))
OPENTIT_VMX_MAP(vmaxuw, 4, u32_, std::max(x, y))
OPENTIT_VMX_MAP(vmaxsb, 16, s8_, std::max(x, y))
OPENTIT_VMX_MAP(vmaxsh, 8, s16_, std::max(x, y))
OPENTIT_VMX_MAP(vmaxsw, 4, s32_, std::max(x, y))
OPENTIT_VMX_MAP(vminub, 16, u8_, std::min(x, y))
OPENTIT_VMX_MAP(vminuh, 8, u16_, std::min(x, y))
OPENTIT_VMX_MAP(vminuw, 4, u32_, std::min(x, y))
OPENTIT_VMX_MAP(vminsb, 16, s8_, std::min(x, y))
OPENTIT_VMX_MAP(vminsh, 8, s16_, std::min(x, y))
OPENTIT_VMX_MAP(vminsw, 4, s32_, std::min(x, y))
OPENTIT_VMX_MAP(vavgub, 16, u8_, (u32(x) + y + 1) >> 1)
OPENTIT_VMX_MAP(vavguh, 8, u16_, (u32(x) + y + 1) >> 1)
OPENTIT_VMX_MAP(vavguw, 4, u32_, (u64(x) + y + 1) >> 1)
OPENTIT_VMX_MAP(vavgsb, 16, s8_, (s32(x) + y + 1) >> 1)
OPENTIT_VMX_MAP(vavgsh, 8, s16_, (s32(x) + y + 1) >> 1)
OPENTIT_VMX_MAP(vavgsw, 4, s32_, (s64(x) + y + 1) >> 1)
// rotates / shifts (shift count = low bits of each element of b)
OPENTIT_VMX_MAP(vrlb, 16, u8_, u8((x << (y & 7)) | (x >> ((8 - (y & 7)) & 7))))
OPENTIT_VMX_MAP(vrlh, 8, u16_, u16((x << (y & 15)) | (x >> ((16 - (y & 15)) & 15))))
OPENTIT_VMX_MAP(vrlw, 4, u32_, (x << (y & 31)) | (x >> ((32 - (y & 31)) & 31)))
OPENTIT_VMX_MAP(vslb, 16, u8_, x << (y & 7))
OPENTIT_VMX_MAP(vslh, 8, u16_, x << (y & 15))
OPENTIT_VMX_MAP(vslw, 4, u32_, x << (y & 31))
OPENTIT_VMX_MAP(vsrb, 16, u8_, x >> (y & 7))
OPENTIT_VMX_MAP(vsrh, 8, u16_, x >> (y & 15))
OPENTIT_VMX_MAP(vsrw, 4, u32_, x >> (y & 31))
OPENTIT_VMX_MAP(vsrab, 16, s8_, x >> (y & 7))
OPENTIT_VMX_MAP(vsrah, 8, s16_, x >> (y & 15))
OPENTIT_VMX_MAP(vsraw, 4, s32_, x >> (y & 31))
// logical
OPENTIT_VMX_MAP(vand, 2, u64_, x & y)
OPENTIT_VMX_MAP(vandc, 2, u64_, x & ~y)
OPENTIT_VMX_MAP(vor, 2, u64_, x | y)
OPENTIT_VMX_MAP(vnor, 2, u64_, ~(x | y))
OPENTIT_VMX_MAP(vxor, 2, u64_, x ^ y)
// float
OPENTIT_VMX_MAP(vaddfp, 4, f32_, x + y)
OPENTIT_VMX_MAP(vsubfp, 4, f32_, x - y)
OPENTIT_VMX_MAP(vmaxfp, 4, f32_, std::isnan(x) ? x : std::isnan(y) ? y : std::max(x, y))
OPENTIT_VMX_MAP(vminfp, 4, f32_, std::isnan(x) ? x : std::isnan(y) ? y : std::min(x, y))
#undef OPENTIT_VMX_MAP

// ---- compares (return mask vector; caller records CR6 when Rc=1)
#define OPENTIT_VMX_CMP(name, N, field, ufield, expr)               \
    inline v128 name(const v128& a, const v128& b) {                \
        v128 r;                                                     \
        for (int i = 0; i < N; ++i) {                               \
            auto x = a.field[i];                                    \
            auto y = b.field[i];                                    \
            r.ufield[i] = (expr) ? static_cast<std::remove_reference_t<decltype(r.ufield[0])>>(~0ull) : 0; \
        }                                                           \
        return r;                                                   \
    }
OPENTIT_VMX_CMP(vcmpequb, 16, u8_, u8_, x == y)
OPENTIT_VMX_CMP(vcmpequh, 8, u16_, u16_, x == y)
OPENTIT_VMX_CMP(vcmpequw, 4, u32_, u32_, x == y)
OPENTIT_VMX_CMP(vcmpgtub, 16, u8_, u8_, x > y)
OPENTIT_VMX_CMP(vcmpgtuh, 8, u16_, u16_, x > y)
OPENTIT_VMX_CMP(vcmpgtuw, 4, u32_, u32_, x > y)
OPENTIT_VMX_CMP(vcmpgtsb, 16, s8_, u8_, x > y)
OPENTIT_VMX_CMP(vcmpgtsh, 8, s16_, u16_, x > y)
OPENTIT_VMX_CMP(vcmpgtsw, 4, s32_, u32_, x > y)
OPENTIT_VMX_CMP(vcmpeqfp, 4, f32_, u32_, x == y)
OPENTIT_VMX_CMP(vcmpgefp, 4, f32_, u32_, x >= y)
OPENTIT_VMX_CMP(vcmpgtfp, 4, f32_, u32_, x > y)
#undef OPENTIT_VMX_CMP

inline v128 vcmpbfp(const v128& a, const v128& b) {
    v128 r;
    for (int i = 0; i < 4; ++i) {
        const f32 x = a.f32_[i], y = b.f32_[i];
        u32 v = 0;
        if (!(x <= y)) v |= 0x80000000u;
        if (!(x >= -y)) v |= 0x40000000u;
        r.u32_[i] = v;
    }
    return r;
}

// CR6 summary: bit 0 (lt) = all elements true, bit 2 (eq) = all false.
inline void cr6(u8* cr, const v128& m) {
    const bool all = m.u64_[0] == ~0ull && m.u64_[1] == ~0ull;
    const bool none = m.u64_[0] == 0 && m.u64_[1] == 0;
    cr[24] = all;
    cr[25] = 0;
    cr[26] = none;
    cr[27] = 0;
}
inline void cr6_bounds(u8* cr, const v128& m) {
    cr[24] = 0;
    cr[25] = 0;
    cr[26] = m.u64_[0] == 0 && m.u64_[1] == 0;
    cr[27] = 0;
}

// ---- three-operand
inline v128 vsel(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int i = 0; i < 2; ++i) r.u64_[i] = (a.u64_[i] & ~c.u64_[i]) | (b.u64_[i] & c.u64_[i]);
    return r;
}
inline v128 vperm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int i = 0; i < 16; ++i) {
        const int idx = c.ub(i) & 31;
        r.ub(i) = idx < 16 ? a.ub(idx) : b.ub(idx - 16);
    }
    return r;
}
inline v128 vsldoi(const v128& a, const v128& b, int sh) {
    v128 r;
    for (int i = 0; i < 16; ++i) {
        const int j = i + sh;
        r.ub(i) = j < 16 ? a.ub(j) : b.ub(j - 16);
    }
    return r;
}
inline v128 vmaddfp(const v128& a, const v128& b, const v128& c) {  // a*c + b
    v128 r;
    for (int i = 0; i < 4; ++i) r.f32_[i] = std::fma(a.f32_[i], c.f32_[i], b.f32_[i]);
    return r;
}
inline v128 vnmsubfp(const v128& a, const v128& b, const v128& c) {  // -(a*c - b)
    v128 r;
    for (int i = 0; i < 4; ++i) r.f32_[i] = -std::fma(a.f32_[i], c.f32_[i], -b.f32_[i]);
    return r;
}
inline v128 vmhaddshs(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int i = 0; i < 8; ++i) r.s16_[i] = sat<s16>(((s32(a.s16_[i]) * b.s16_[i]) >> 15) + c.s16_[i]);
    return r;
}
inline v128 vmhraddshs(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int i = 0; i < 8; ++i)
        r.s16_[i] = sat<s16>(((s32(a.s16_[i]) * b.s16_[i] + 0x4000) >> 15) + c.s16_[i]);
    return r;
}
inline v128 vmladduhm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int i = 0; i < 8; ++i) r.u16_[i] = static_cast<u16>(u32(a.u16_[i]) * b.u16_[i] + c.u16_[i]);
    return r;
}
inline v128 vmsumubm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w) {
        u32 s = c.uw(w);
        for (int k = 0; k < 4; ++k) s += u32(a.ub(w * 4 + k)) * b.ub(w * 4 + k);
        r.uw(w) = s;
    }
    return r;
}
inline v128 vmsummbm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w) {
        s32 s = c.sw(w);
        for (int k = 0; k < 4; ++k) s += s32(a.sb(w * 4 + k)) * b.ub(w * 4 + k);
        r.sw(w) = s;
    }
    return r;
}
inline v128 vmsumuhm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w) r.uw(w) = c.uw(w) + u32(a.uh(w * 2)) * b.uh(w * 2) + u32(a.uh(w * 2 + 1)) * b.uh(w * 2 + 1);
    return r;
}
inline v128 vmsumuhs(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w)
        r.uw(w) = sat<u32>(s64(c.uw(w)) + u64(a.uh(w * 2)) * b.uh(w * 2) + u64(a.uh(w * 2 + 1)) * b.uh(w * 2 + 1));
    return r;
}
inline v128 vmsumshm(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w)
        r.sw(w) = static_cast<s32>(s64(c.sw(w)) + s32(a.sh(w * 2)) * b.sh(w * 2) + s32(a.sh(w * 2 + 1)) * b.sh(w * 2 + 1));
    return r;
}
inline v128 vmsumshs(const v128& a, const v128& b, const v128& c) {
    v128 r;
    for (int w = 0; w < 4; ++w)
        r.sw(w) = sat<s32>(s64(c.sw(w)) + s32(a.sh(w * 2)) * b.sh(w * 2) + s32(a.sh(w * 2 + 1)) * b.sh(w * 2 + 1));
    return r;
}

// ---- multiply even / odd (BE element numbering)
#define OPENTIT_VMX_MULEO(name, acc, racc, N, start)                             \
    inline v128 name(const v128& a, const v128& b) {                            \
        v128 r;                                                                 \
        for (int i = 0; i < N; ++i) r.racc(i) = static_cast<std::remove_reference_t<decltype(r.racc(0))>>( \
            a.acc(2 * i + start) * b.acc(2 * i + start));                        \
        return r;                                                               \
    }
OPENTIT_VMX_MULEO(vmuleub, ub, uh, 8, 0)
OPENTIT_VMX_MULEO(vmuloub, ub, uh, 8, 1)
OPENTIT_VMX_MULEO(vmulesb, sb, sh, 8, 0)
OPENTIT_VMX_MULEO(vmulosb, sb, sh, 8, 1)
OPENTIT_VMX_MULEO(vmuleuh, uh, uw, 4, 0)
OPENTIT_VMX_MULEO(vmulouh, uh, uw, 4, 1)
OPENTIT_VMX_MULEO(vmulesh, sh, sw, 4, 0)
OPENTIT_VMX_MULEO(vmulosh, sh, sw, 4, 1)
#undef OPENTIT_VMX_MULEO

// ---- sums across
inline v128 vsum4ubs(const v128& a, const v128& b) {
    v128 r;
    for (int w = 0; w < 4; ++w) {
        s64 s = b.uw(w);
        for (int k = 0; k < 4; ++k) s += a.ub(w * 4 + k);
        r.uw(w) = sat<u32>(s);
    }
    return r;
}
inline v128 vsum4sbs(const v128& a, const v128& b) {
    v128 r;
    for (int w = 0; w < 4; ++w) {
        s64 s = b.sw(w);
        for (int k = 0; k < 4; ++k) s += a.sb(w * 4 + k);
        r.sw(w) = sat<s32>(s);
    }
    return r;
}
inline v128 vsum4shs(const v128& a, const v128& b) {
    v128 r;
    for (int w = 0; w < 4; ++w) r.sw(w) = sat<s32>(s64(b.sw(w)) + a.sh(w * 2) + a.sh(w * 2 + 1));
    return r;
}
inline v128 vsum2sws(const v128& a, const v128& b) {
    v128 r = v128::zero();
    r.sw(1) = sat<s32>(s64(a.sw(0)) + a.sw(1) + b.sw(1));
    r.sw(3) = sat<s32>(s64(a.sw(2)) + a.sw(3) + b.sw(3));
    return r;
}
inline v128 vsumsws(const v128& a, const v128& b) {
    v128 r = v128::zero();
    r.sw(3) = sat<s32>(s64(a.sw(0)) + a.sw(1) + a.sw(2) + a.sw(3) + b.sw(3));
    return r;
}

// ---- float unary
#define OPENTIT_VMX_FUN(name, expr)                     \
    inline v128 name(const v128& b) {                   \
        v128 r;                                         \
        for (int i = 0; i < 4; ++i) {                   \
            const f32 x = b.f32_[i];                    \
            r.f32_[i] = (expr);                         \
        }                                               \
        return r;                                       \
    }
OPENTIT_VMX_FUN(vrefp, 1.0f / x)
OPENTIT_VMX_FUN(vrsqrtefp, 1.0f / std::sqrt(x))
OPENTIT_VMX_FUN(vexptefp, std::exp2(x))
OPENTIT_VMX_FUN(vlogefp, std::log2(x))
OPENTIT_VMX_FUN(vrfin, std::nearbyint(x))
OPENTIT_VMX_FUN(vrfiz, std::trunc(x))
OPENTIT_VMX_FUN(vrfip, std::ceil(x))
OPENTIT_VMX_FUN(vrfim, std::floor(x))
#undef OPENTIT_VMX_FUN

inline v128 vcfsx(const v128& b, int uimm) {
    v128 r;
    for (int i = 0; i < 4; ++i) r.f32_[i] = std::ldexp(static_cast<f32>(b.s32_[i]), -uimm);
    return r;
}
inline v128 vcfux(const v128& b, int uimm) {
    v128 r;
    for (int i = 0; i < 4; ++i) r.f32_[i] = std::ldexp(static_cast<f32>(b.u32_[i]), -uimm);
    return r;
}
inline v128 vctsxs(const v128& b, int uimm) {
    v128 r;
    for (int i = 0; i < 4; ++i) {
        const f64 x = std::ldexp(static_cast<f64>(b.f32_[i]), uimm);
        r.s32_[i] = std::isnan(x) ? 0 : x >= 2147483647.0 ? 0x7FFFFFFF : x <= -2147483648.0 ? s32(0x80000000) : s32(x);
    }
    return r;
}
inline v128 vctuxs(const v128& b, int uimm) {
    v128 r;
    for (int i = 0; i < 4; ++i) {
        const f64 x = std::ldexp(static_cast<f64>(b.f32_[i]), uimm);
        r.u32_[i] = std::isnan(x) ? 0 : x >= 4294967295.0 ? 0xFFFFFFFFu : x <= 0.0 ? 0u : u32(x);
    }
    return r;
}

// ---- merges / splats (BE semantics)
inline v128 vmrghb(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 8; ++i) { r.ub(2 * i) = a.ub(i); r.ub(2 * i + 1) = b.ub(i); } return r; }
inline v128 vmrghh(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 4; ++i) { r.uh(2 * i) = a.uh(i); r.uh(2 * i + 1) = b.uh(i); } return r; }
inline v128 vmrghw(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 2; ++i) { r.uw(2 * i) = a.uw(i); r.uw(2 * i + 1) = b.uw(i); } return r; }
inline v128 vmrglb(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 8; ++i) { r.ub(2 * i) = a.ub(i + 8); r.ub(2 * i + 1) = b.ub(i + 8); } return r; }
inline v128 vmrglh(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 4; ++i) { r.uh(2 * i) = a.uh(i + 4); r.uh(2 * i + 1) = b.uh(i + 4); } return r; }
inline v128 vmrglw(const v128& a, const v128& b) { v128 r; for (int i = 0; i < 2; ++i) { r.uw(2 * i) = a.uw(i + 2); r.uw(2 * i + 1) = b.uw(i + 2); } return r; }
inline v128 vspltb(const v128& b, int i) { v128 r; const u8 x = b.ub(i & 15); for (auto& e : r.u8_) e = x; return r; }
inline v128 vsplth(const v128& b, int i) { v128 r; const u16 x = b.uh(i & 7); for (auto& e : r.u16_) e = x; return r; }
inline v128 vspltw(const v128& b, int i) { v128 r; const u32 x = b.uw(i & 3); for (auto& e : r.u32_) e = x; return r; }
inline v128 vspltisb(int s) { v128 r; for (auto& e : r.s8_) e = static_cast<s8>(s); return r; }
inline v128 vspltish(int s) { v128 r; for (auto& e : r.s16_) e = static_cast<s16>(s); return r; }
inline v128 vspltisw(int s) { v128 r; for (auto& e : r.s32_) e = s; return r; }

// ---- whole-register shifts
inline v128 vsl(const v128& a, const v128& b) {
    const int s = b.ub(15) & 7;
    v128 r;
    r.u64_[1] = s ? (a.u64_[1] << s) | (a.u64_[0] >> (64 - s)) : a.u64_[1];
    r.u64_[0] = a.u64_[0] << s;
    return r;
}
inline v128 vsr(const v128& a, const v128& b) {
    const int s = b.ub(15) & 7;
    v128 r;
    r.u64_[0] = s ? (a.u64_[0] >> s) | (a.u64_[1] << (64 - s)) : a.u64_[0];
    r.u64_[1] = a.u64_[1] >> s;
    return r;
}
inline v128 vslo(const v128& a, const v128& b) {
    const int s = (b.ub(15) >> 3) & 15;
    v128 r = v128::zero();
    for (int i = 0; i + s < 16; ++i) r.ub(i) = a.ub(i + s);
    return r;
}
inline v128 vsro(const v128& a, const v128& b) {
    const int s = (b.ub(15) >> 3) & 15;
    v128 r = v128::zero();
    for (int i = s; i < 16; ++i) r.ub(i) = a.ub(i - s);
    return r;
}

// ---- pack (a supplies the high/first half in BE order)
#define OPENTIT_VMX_PACK(name, N, src, dst, conv)                     \
    inline v128 name(const v128& a, const v128& b) {                  \
        v128 r;                                                       \
        for (int i = 0; i < N; ++i) {                                 \
            r.dst(i) = conv(a.src(i));                                \
            r.dst(i + N) = conv(b.src(i));                            \
        }                                                             \
        return r;                                                     \
    }
#define OPENTIT_TRUNC8(x) static_cast<u8>(x)
#define OPENTIT_TRUNC16(x) static_cast<u16>(x)
#define OPENTIT_SATU8(x) sat<u8>(x)
#define OPENTIT_SATU16(x) sat<u16>(x)
#define OPENTIT_SATS8(x) sat<s8>(x)
#define OPENTIT_SATS16(x) sat<s16>(x)
OPENTIT_VMX_PACK(vpkuhum, 8, uh, ub, OPENTIT_TRUNC8)
OPENTIT_VMX_PACK(vpkuwum, 4, uw, uh, OPENTIT_TRUNC16)
OPENTIT_VMX_PACK(vpkuhus, 8, uh, ub, OPENTIT_SATU8)
OPENTIT_VMX_PACK(vpkuwus, 4, uw, uh, OPENTIT_SATU16)
OPENTIT_VMX_PACK(vpkshus, 8, sh, ub, OPENTIT_SATU8)
OPENTIT_VMX_PACK(vpkswus, 4, sw, uh, OPENTIT_SATU16)
OPENTIT_VMX_PACK(vpkshss, 8, sh, sb, OPENTIT_SATS8)
OPENTIT_VMX_PACK(vpkswss, 4, sw, sh, OPENTIT_SATS16)
#undef OPENTIT_VMX_PACK
#undef OPENTIT_TRUNC8
#undef OPENTIT_TRUNC16
#undef OPENTIT_SATU8
#undef OPENTIT_SATU16
#undef OPENTIT_SATS8
#undef OPENTIT_SATS16

inline u16 pack_pixel(u32 w) {
    return static_cast<u16>(((w >> 9) & 0x8000) | ((w >> 9) & 0x7C00) | ((w >> 6) & 0x03E0) | ((w >> 3) & 0x001F));
}
inline v128 vpkpx(const v128& a, const v128& b) {
    v128 r;
    for (int i = 0; i < 4; ++i) {
        r.uh(i) = pack_pixel(a.uw(i));
        r.uh(i + 4) = pack_pixel(b.uw(i));
    }
    return r;
}
inline u32 unpack_pixel(u16 h) {
    const u32 a = (h & 0x8000) ? 0xFF : 0;
    return (a << 24) | (u32((h >> 10) & 31) << 16) | (u32((h >> 5) & 31) << 8) | u32(h & 31);
}

// ---- unpack (sign-extend high or low half, BE order)
inline v128 vupkhsb(const v128& b) { v128 r; for (int i = 0; i < 8; ++i) r.sh(i) = b.sb(i); return r; }
inline v128 vupklsb(const v128& b) { v128 r; for (int i = 0; i < 8; ++i) r.sh(i) = b.sb(i + 8); return r; }
inline v128 vupkhsh(const v128& b) { v128 r; for (int i = 0; i < 4; ++i) r.sw(i) = b.sh(i); return r; }
inline v128 vupklsh(const v128& b) { v128 r; for (int i = 0; i < 4; ++i) r.sw(i) = b.sh(i + 4); return r; }
inline v128 vupkhpx(const v128& b) { v128 r; for (int i = 0; i < 4; ++i) r.uw(i) = unpack_pixel(b.uh(i)); return r; }
inline v128 vupklpx(const v128& b) { v128 r; for (int i = 0; i < 4; ++i) r.uw(i) = unpack_pixel(b.uh(i + 4)); return r; }

// ---- permute-control generation
inline v128 lvsl(u32 ea) { v128 r; const int s = ea & 15; for (int i = 0; i < 16; ++i) r.ub(i) = static_cast<u8>(s + i); return r; }
inline v128 lvsr(u32 ea) { v128 r; const int s = ea & 15; for (int i = 0; i < 16; ++i) r.ub(i) = static_cast<u8>(16 - s + i); return r; }

}  // namespace vmx

}  // namespace opentit
