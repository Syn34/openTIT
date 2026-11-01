// Per-instruction SPU semantics, shared by the interpreter and (later) the
// static SPU recompiler. Every op has the signature
//     u32 op(SPUThread& t, u32 raw, u32 pc)   -> returns the next pc
// so recompiled code can call them with constant `raw` and let the compiler
// fold the field extraction away.
//
// Register model: SPU registers reuse v128 (see vmx.h): host-little-endian
// 128-bit value == the big-endian quadword, so whole-register shifts are plain
// 128-bit shifts and BE element i is accessed with .uw(i)/.uh(i)/.ub(i).
// The "preferred slot" is BE word 0.
#pragma once

#include <cmath>
#include <cstring>
#if defined(__SSSE3__) || defined(__AVX__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64)))
#include <tmmintrin.h>
#endif

#include "opentit/memory.h"
#include "opentit/spu.h"
#include "spu/spu_decoder.h"

namespace opentit::spu::ops {

// ---------------------------------------------------------------- helpers
inline u32 RT(u32 r) { return r & 0x7F; }
inline u32 RA(u32 r) { return (r >> 7) & 0x7F; }
inline u32 RB(u32 r) { return (r >> 14) & 0x7F; }
inline u32 RC(u32 r) { return r & 0x7F; }
inline u32 RRR_RT(u32 r) { return (r >> 21) & 0x7F; }
inline s32 I7(u32 r) { return Insn(r).i7(); }
inline u32 U7(u32 r) { return (r >> 14) & 0x7F; }
inline u32 I8(u32 r) { return (r >> 14) & 0xFF; }
inline s32 I10(u32 r) { return Insn(r).i10(); }
inline s32 I16(u32 r) { return Insn(r).i16(); }
inline u32 U16(u32 r) { return (r >> 7) & 0xFFFF; }
inline u32 I18(u32 r) { return (r >> 7) & 0x3FFFF; }

inline u32 pref(const v128& v) { return v.uw(0); }
inline v128 splat32(u32 x) { v128 r; for (auto& e : r.u32_) e = x; return r; }
inline v128 splat16(u16 x) { v128 r; for (auto& e : r.u16_) e = x; return r; }
inline v128 splat8(u8 x) { v128 r; for (auto& e : r.u8_) e = x; return r; }
inline u64 hi64(const v128& v) { return v.u64_[1]; }
inline u64 lo64(const v128& v) { return v.u64_[0]; }
inline v128 make(u64 hi, u64 lo) { v128 r; r.u64_[1] = hi; r.u64_[0] = lo; return r; }

// 128-bit logical shifts (BE bit order == numeric order of the 128-bit value).
inline v128 shl128(const v128& v, u32 n) {
    if (n >= 128) return v128::zero();
    if (n == 0) return v;
    if (n >= 64) return make(lo64(v) << (n - 64), 0);
    return make((hi64(v) << n) | (lo64(v) >> (64 - n)), lo64(v) << n);
}
inline v128 shr128(const v128& v, u32 n) {
    if (n >= 128) return v128::zero();
    if (n == 0) return v;
    if (n >= 64) return make(0, hi64(v) >> (n - 64));
    return make(hi64(v) >> n, (lo64(v) >> n) | (hi64(v) << (64 - n)));
}
inline v128 rotl128(const v128& v, u32 n) {
    n &= 127;
    if (!n) return v;
    const v128 a = shl128(v, n), b = shr128(v, 128 - n);
    return make(hi64(a) | hi64(b), lo64(a) | lo64(b));
}

inline f64 dbl(const v128& v, int i) { const u64 b = i ? lo64(v) : hi64(v); f64 d; std::memcpy(&d, &b, 8); return d; }
inline void set_dbl(v128& v, int i, f64 d) { u64 b; std::memcpy(&b, &d, 8); if (i) v.u64_[0] = b; else v.u64_[1] = b; }

// SPU single precision has no denormals: flush to (signed) zero.
inline f32 ftz(f32 x) {
    u32 b;
    std::memcpy(&b, &x, 4);
    if ((b & 0x7F800000u) == 0) b &= 0x80000000u;
    std::memcpy(&x, &b, 4);
    return x;
}

#define T (t.gpr)
#define DEF(name) inline u32 name([[maybe_unused]] SPUThread& t, [[maybe_unused]] u32 raw, [[maybe_unused]] u32 pc)
#define NEXT return pc + 4

// Element-wise helpers
#define MAP_W(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 4; ++i) { const u32 x = a.u32_[i], y = b.u32_[i]; (void)x; (void)y; r.u32_[i] = (expr); } T[RT(raw)] = r; NEXT; }
#define MAP_H(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 8; ++i) { const u16 x = a.u16_[i], y = b.u16_[i]; (void)x; (void)y; r.u16_[i] = static_cast<u16>(expr); } T[RT(raw)] = r; NEXT; }
#define MAP_B(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 16; ++i) { const u8 x = a.u8_[i], y = b.u8_[i]; (void)x; (void)y; r.u8_[i] = static_cast<u8>(expr); } T[RT(raw)] = r; NEXT; }
#define MAP_WI(expr) { const v128 a = T[RA(raw)]; const s32 imm = I10(raw); (void)imm; v128 r; for (int i = 0; i < 4; ++i) { const u32 x = a.u32_[i]; r.u32_[i] = (expr); } T[RT(raw)] = r; NEXT; }
#define MAP_HI(expr) { const v128 a = T[RA(raw)]; const s32 imm = I10(raw); (void)imm; v128 r; for (int i = 0; i < 8; ++i) { const u16 x = a.u16_[i]; r.u16_[i] = static_cast<u16>(expr); } T[RT(raw)] = r; NEXT; }
#define MAP_BI(expr) { const v128 a = T[RA(raw)]; const u8 imm = static_cast<u8>(I10(raw)); (void)imm; v128 r; for (int i = 0; i < 16; ++i) { const u8 x = a.u8_[i]; r.u8_[i] = static_cast<u8>(expr); } T[RT(raw)] = r; NEXT; }
#define MASK(c) ((c) ? ~0u : 0u)

// ---------------------------------------------------------------- control / system
DEF(LNOP) { NEXT; }
DEF(NOP) { NEXT; }
DEF(SYNC) { std::atomic_thread_fence(std::memory_order_seq_cst); NEXT; }
DEF(DSYNC) { std::atomic_thread_fence(std::memory_order_seq_cst); NEXT; }
DEF(HBR) { NEXT; }
DEF(HBRA) { NEXT; }
DEF(HBRR) { NEXT; }
DEF(MFSPR) { T[RT(raw)] = v128::zero(); NEXT; }
DEF(MTSPR) { NEXT; }
DEF(FSCRRD) { T[RT(raw)] = v128::zero(); NEXT; }
DEF(FSCRWR) { NEXT; }
DEF(STOP) { t.stopped = true; t.stop_code = raw & 0x3FFF; NEXT; }
DEF(STOPD) { t.stopped = true; t.stop_code = 0x3FFF; NEXT; }
DEF(RDCH) { const u32 v = rdch(t, RA(raw)); T[RT(raw)] = make(u64(v) << 32, 0); NEXT; }
DEF(RCHCNT) { const u32 v = rchcnt(t, RA(raw)); T[RT(raw)] = make(u64(v) << 32, 0); NEXT; }
DEF(WRCH) { wrch(t, RA(raw), pref(T[RT(raw)])); NEXT; }

// halt-if (treated as a stop with a recognisable code)
inline u32 halt(SPUThread& t, u32 pc) { t.stopped = true; t.stop_code = 0x10000; return pc + 4; }
DEF(HEQ) { return s32(pref(T[RA(raw)])) == s32(pref(T[RB(raw)])) ? halt(t, pc) : pc + 4; }
DEF(HGT) { return s32(pref(T[RA(raw)])) > s32(pref(T[RB(raw)])) ? halt(t, pc) : pc + 4; }
DEF(HLGT) { return pref(T[RA(raw)]) > pref(T[RB(raw)]) ? halt(t, pc) : pc + 4; }
DEF(HEQI) { return s32(pref(T[RA(raw)])) == I10(raw) ? halt(t, pc) : pc + 4; }
DEF(HGTI) { return s32(pref(T[RA(raw)])) > I10(raw) ? halt(t, pc) : pc + 4; }
DEF(HLGTI) { return pref(T[RA(raw)]) > u32(I10(raw)) ? halt(t, pc) : pc + 4; }

// ---------------------------------------------------------------- branches
inline u32 rel(u32 pc, u32 raw) { return (pc + (u32(I16(raw)) << 2)) & 0x3FFFC; }
inline u32 abs16(u32 raw) { return (u32(I16(raw)) << 2) & 0x3FFFC; }
inline v128 link(u32 pc) { return make(u64((pc + 4) & 0x3FFFC) << 32, 0); }

DEF(BR) { return rel(pc, raw); }
DEF(BRA) { return abs16(raw); }
DEF(BRSL) { T[RT(raw)] = link(pc); return rel(pc, raw); }
DEF(BRASL) { T[RT(raw)] = link(pc); return abs16(raw); }
DEF(BRZ) { return pref(T[RT(raw)]) == 0 ? rel(pc, raw) : pc + 4; }
DEF(BRNZ) { return pref(T[RT(raw)]) != 0 ? rel(pc, raw) : pc + 4; }
DEF(BRHZ) { return T[RT(raw)].uh(1) == 0 ? rel(pc, raw) : pc + 4; }
DEF(BRHNZ) { return T[RT(raw)].uh(1) != 0 ? rel(pc, raw) : pc + 4; }
DEF(BI) { return pref(T[RA(raw)]) & 0x3FFFC; }
DEF(IRET) { return t.srr0 & 0x3FFFC; }
DEF(BISLED) { const u32 target = pref(T[RA(raw)]) & 0x3FFFC; T[RT(raw)] = link(pc); return (t.event_stat & t.event_mask) ? target : pc + 4; }
DEF(BISL) { const u32 target = pref(T[RA(raw)]) & 0x3FFFC; T[RT(raw)] = link(pc); return target; }
DEF(BIZ) { return pref(T[RT(raw)]) == 0 ? (pref(T[RA(raw)]) & 0x3FFFC) : pc + 4; }
DEF(BINZ) { return pref(T[RT(raw)]) != 0 ? (pref(T[RA(raw)]) & 0x3FFFC) : pc + 4; }
DEF(BIHZ) { return T[RT(raw)].uh(1) == 0 ? (pref(T[RA(raw)]) & 0x3FFFC) : pc + 4; }
DEF(BIHNZ) { return T[RT(raw)].uh(1) != 0 ? (pref(T[RA(raw)]) & 0x3FFFC) : pc + 4; }

// ---------------------------------------------------------------- loads / stores
DEF(LQD) { T[RT(raw)] = t.lsq(pref(T[RA(raw)]) + (u32(I10(raw)) << 4)); NEXT; }
DEF(LQX) { T[RT(raw)] = t.lsq(pref(T[RA(raw)]) + pref(T[RB(raw)])); NEXT; }
DEF(LQA) { T[RT(raw)] = t.lsq(abs16(raw)); NEXT; }
DEF(LQR) { T[RT(raw)] = t.lsq(rel(pc, raw)); NEXT; }
DEF(STQD) { t.set_lsq(pref(T[RA(raw)]) + (u32(I10(raw)) << 4), T[RT(raw)]); NEXT; }
DEF(STQX) { t.set_lsq(pref(T[RA(raw)]) + pref(T[RB(raw)]), T[RT(raw)]); NEXT; }
DEF(STQA) { t.set_lsq(abs16(raw), T[RT(raw)]); NEXT; }
DEF(STQR) { t.set_lsq(rel(pc, raw), T[RT(raw)]); NEXT; }

// insertion-control generation
inline v128 ctrl_default() { v128 r; for (int i = 0; i < 16; ++i) r.ub(i) = static_cast<u8>(0x10 + i); return r; }
inline v128 cbd(u32 addr) { v128 r = ctrl_default(); r.ub(addr & 15) = 0x03; return r; }
inline v128 chd(u32 addr) { v128 r = ctrl_default(); r.uh((addr & 15) >> 1) = 0x0203; return r; }
inline v128 cwd(u32 addr) { v128 r = ctrl_default(); r.uw((addr & 15) >> 2) = 0x00010203; return r; }
inline v128 cdd(u32 addr) { v128 r = ctrl_default(); const int w = (addr & 8) ? 2 : 0; r.uw(w) = 0x00010203; r.uw(w + 1) = 0x04050607; return r; }
DEF(CBD) { T[RT(raw)] = cbd(pref(T[RA(raw)]) + u32(I7(raw))); NEXT; }
DEF(CHD) { T[RT(raw)] = chd(pref(T[RA(raw)]) + u32(I7(raw))); NEXT; }
DEF(CWD) { T[RT(raw)] = cwd(pref(T[RA(raw)]) + u32(I7(raw))); NEXT; }
DEF(CDD) { T[RT(raw)] = cdd(pref(T[RA(raw)]) + u32(I7(raw))); NEXT; }
DEF(CBX) { T[RT(raw)] = cbd(pref(T[RA(raw)]) + pref(T[RB(raw)])); NEXT; }
DEF(CHX) { T[RT(raw)] = chd(pref(T[RA(raw)]) + pref(T[RB(raw)])); NEXT; }
DEF(CWX) { T[RT(raw)] = cwd(pref(T[RA(raw)]) + pref(T[RB(raw)])); NEXT; }
DEF(CDX) { T[RT(raw)] = cdd(pref(T[RA(raw)]) + pref(T[RB(raw)])); NEXT; }

// ---------------------------------------------------------------- immediates
DEF(IL) { T[RT(raw)] = splat32(u32(I16(raw))); NEXT; }
DEF(ILH) { T[RT(raw)] = splat16(static_cast<u16>(U16(raw))); NEXT; }
DEF(ILHU) { T[RT(raw)] = splat32(U16(raw) << 16); NEXT; }
DEF(ILA) { T[RT(raw)] = splat32(I18(raw)); NEXT; }
DEF(IOHL) { v128& r = T[RT(raw)]; for (auto& e : r.u32_) e |= U16(raw); NEXT; }
DEF(FSMBI) { v128 r; const u32 m = U16(raw); for (int i = 0; i < 16; ++i) r.ub(i) = ((m >> (15 - i)) & 1) ? 0xFF : 0; T[RT(raw)] = r; NEXT; }

// ---------------------------------------------------------------- integer arithmetic
DEF(A) MAP_W(x + y)
DEF(AH) MAP_H(x + y)
DEF(AI) MAP_WI(x + u32(imm))
DEF(AHI) MAP_HI(x + u16(imm))
DEF(SF) MAP_W(y - x)
DEF(SFH) MAP_H(y - x)
DEF(SFI) MAP_WI(u32(imm) - x)
DEF(SFHI) MAP_HI(u16(imm) - x)
DEF(CG) MAP_W(u32((u64(x) + y) >> 32))
DEF(BG) MAP_W(y >= x ? 1u : 0u)
DEF(ADDX) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] = a.u32_[i] + b.u32_[i] + (r.u32_[i] & 1); NEXT; }
DEF(SFX) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] = b.u32_[i] + ~a.u32_[i] + (r.u32_[i] & 1); NEXT; }
DEF(CGX) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] = u32((u64(a.u32_[i]) + b.u32_[i] + (r.u32_[i] & 1)) >> 32); NEXT; }
DEF(BGX) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] = (s64(u64(b.u32_[i])) - s64(a.u32_[i]) - s64(1 - (r.u32_[i] & 1))) >= 0 ? 1u : 0u; NEXT; }

DEF(MPY) MAP_W(u32(s32(s16(x)) * s32(s16(y))))
DEF(MPYU) MAP_W(u32(u16(x)) * u32(u16(y)))
DEF(MPYH) MAP_W(((x >> 16) * u32(u16(y))) << 16)
DEF(MPYS) MAP_W(u32((s32(s16(x)) * s32(s16(y))) >> 16))
DEF(MPYHH) MAP_W(u32(s32(s16(x >> 16)) * s32(s16(y >> 16))))
DEF(MPYHHU) MAP_W((x >> 16) * (y >> 16))
DEF(MPYI) MAP_WI(u32(s32(s16(x)) * imm))
DEF(MPYUI) MAP_WI(u32(u16(x)) * u32(u16(imm)))
DEF(MPYHHA) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] += u32(s32(s16(a.u32_[i] >> 16)) * s32(s16(b.u32_[i] >> 16))); NEXT; }
DEF(MPYHHAU) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 4; ++i) r.u32_[i] += (a.u32_[i] >> 16) * (b.u32_[i] >> 16); NEXT; }
DEF(MPYA) { const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = u32(s32(s16(a.u32_[i])) * s32(s16(b.u32_[i]))) + c.u32_[i]; T[RRR_RT(raw)] = r; NEXT; }

// ---------------------------------------------------------------- logical
DEF(AND) MAP_W(x & y)
DEF(ANDC) MAP_W(x & ~y)
DEF(OR) MAP_W(x | y)
DEF(ORC) MAP_W(x | ~y)
DEF(XOR) MAP_W(x ^ y)
DEF(NAND) MAP_W(~(x & y))
DEF(NOR) MAP_W(~(x | y))
DEF(EQV) MAP_W(~(x ^ y))
DEF(ANDI) MAP_WI(x & u32(imm))
DEF(ANDHI) MAP_HI(x & u16(imm))
DEF(ANDBI) MAP_BI(x & imm)
DEF(ORI) MAP_WI(x | u32(imm))
DEF(ORHI) MAP_HI(x | u16(imm))
DEF(ORBI) MAP_BI(x | imm)
DEF(XORI) MAP_WI(x ^ u32(imm))
DEF(XORHI) MAP_HI(x ^ u16(imm))
DEF(XORBI) MAP_BI(x ^ imm)
DEF(ORX) { const v128 a = T[RA(raw)]; T[RT(raw)] = make(u64(a.u32_[0] | a.u32_[1] | a.u32_[2] | a.u32_[3]) << 32, 0); NEXT; }
DEF(SELB) { const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)]; T[RRR_RT(raw)] = make((hi64(a) & ~hi64(c)) | (hi64(b) & hi64(c)), (lo64(a) & ~lo64(c)) | (lo64(b) & lo64(c))); NEXT; }
DEF(SHUFB) {
    const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)];
    v128 r;
#if defined(__SSSE3__) || defined(__AVX__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64)))
    // Host byte k holds BE byte 15-k, so BE selector s picks host byte 15-(s&15) = ~s & 15.
    const __m128i x = _mm_load_si128(reinterpret_cast<const __m128i*>(&c));
    const __m128i va = _mm_load_si128(reinterpret_cast<const __m128i*>(&a));
    const __m128i vb = _mm_load_si128(reinterpret_cast<const __m128i*>(&b));
    const __m128i idx = _mm_andnot_si128(x, _mm_set1_epi8(0x0F));
    const __m128i from_b = _mm_cmpeq_epi8(_mm_and_si128(x, _mm_set1_epi8(0x10)), _mm_set1_epi8(0x10));
    __m128i v = _mm_or_si128(_mm_andnot_si128(from_b, _mm_shuffle_epi8(va, idx)), _mm_and_si128(from_b, _mm_shuffle_epi8(vb, idx)));
    const __m128i hi3 = _mm_and_si128(x, _mm_set1_epi8(static_cast<char>(0xE0)));
    const __m128i m00 = _mm_cmpeq_epi8(_mm_and_si128(x, _mm_set1_epi8(static_cast<char>(0xC0))), _mm_set1_epi8(static_cast<char>(0x80)));
    const __m128i mFF = _mm_cmpeq_epi8(hi3, _mm_set1_epi8(static_cast<char>(0xC0)));
    const __m128i m80 = _mm_cmpeq_epi8(hi3, _mm_set1_epi8(static_cast<char>(0xE0)));
    v = _mm_andnot_si128(_mm_or_si128(m00, _mm_or_si128(mFF, m80)), v);
    v = _mm_or_si128(v, _mm_or_si128(mFF, _mm_and_si128(m80, _mm_set1_epi8(static_cast<char>(0x80)))));
    _mm_store_si128(reinterpret_cast<__m128i*>(&r), v);
    T[RRR_RT(raw)] = r;
    NEXT;
#endif
    for (int i = 0; i < 16; ++i) {
        const u8 s = c.ub(i);
        if ((s & 0xC0) == 0x80) r.ub(i) = 0x00;
        else if ((s & 0xE0) == 0xC0) r.ub(i) = 0xFF;
        else if ((s & 0xE0) == 0xE0) r.ub(i) = 0x80;
        else r.ub(i) = (s & 0x10) ? b.ub(s & 0x0F) : a.ub(s & 0x0F);
    }
    T[RRR_RT(raw)] = r;
    NEXT;
}

// ---------------------------------------------------------------- misc integer
DEF(CLZ) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) { u32 x = a.u32_[i], n = 0; for (u32 bit = 0x80000000u; bit && !(x & bit); bit >>= 1) ++n; r.u32_[i] = n; } T[RT(raw)] = r; NEXT; }
DEF(CNTB) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 16; ++i) { u8 x = a.u8_[i], n = 0; for (; x; x &= x - 1) ++n; r.u8_[i] = n; } T[RT(raw)] = r; NEXT; }
DEF(AVGB) MAP_B((u32(x) + y + 1) >> 1)
DEF(ABSDB) MAP_B(x > y ? x - y : y - x)
DEF(SUMB) {
    const v128 a = T[RA(raw)], b = T[RB(raw)];
    v128 r;
    for (int w = 0; w < 4; ++w) {
        u32 sa = 0, sb = 0;
        for (int k = 0; k < 4; ++k) { sa += a.ub(w * 4 + k); sb += b.ub(w * 4 + k); }
        r.uh(w * 2) = static_cast<u16>(sb);
        r.uh(w * 2 + 1) = static_cast<u16>(sa);
    }
    T[RT(raw)] = r;
    NEXT;
}
DEF(XSBH) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 8; ++i) r.u16_[i] = static_cast<u16>(s16(s8(a.u16_[i]))); T[RT(raw)] = r; NEXT; }
DEF(XSHW) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = u32(s32(s16(a.u32_[i]))); T[RT(raw)] = r; NEXT; }
DEF(XSWD) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 2; ++i) r.u64_[i] = u64(s64(s32(a.u64_[i]))); T[RT(raw)] = r; NEXT; }
DEF(FSM) { const u32 m = pref(T[RA(raw)]); v128 r; for (int i = 0; i < 4; ++i) r.uw(i) = MASK((m >> (3 - i)) & 1); T[RT(raw)] = r; NEXT; }
DEF(FSMH) { const u32 m = pref(T[RA(raw)]); v128 r; for (int i = 0; i < 8; ++i) r.uh(i) = ((m >> (7 - i)) & 1) ? 0xFFFF : 0; T[RT(raw)] = r; NEXT; }
DEF(FSMB) { const u32 m = pref(T[RA(raw)]); v128 r; for (int i = 0; i < 16; ++i) r.ub(i) = ((m >> (15 - i)) & 1) ? 0xFF : 0; T[RT(raw)] = r; NEXT; }
DEF(GB) { const v128 a = T[RA(raw)]; u32 m = 0; for (int i = 0; i < 4; ++i) m = (m << 1) | (a.uw(i) & 1); T[RT(raw)] = make(u64(m) << 32, 0); NEXT; }
DEF(GBH) { const v128 a = T[RA(raw)]; u32 m = 0; for (int i = 0; i < 8; ++i) m = (m << 1) | (a.uh(i) & 1); T[RT(raw)] = make(u64(m) << 32, 0); NEXT; }
DEF(GBB) { const v128 a = T[RA(raw)]; u32 m = 0; for (int i = 0; i < 16; ++i) m = (m << 1) | (a.ub(i) & 1); T[RT(raw)] = make(u64(m) << 32, 0); NEXT; }

// ---------------------------------------------------------------- element shifts / rotates
DEF(SHL) MAP_W((y & 0x3F) > 31 ? 0u : x << (y & 0x3F))
DEF(SHLH) MAP_H((y & 0x1F) > 15 ? 0u : u32(x) << (y & 0x1F))
DEF(SHLI) { const u32 n = U7(raw) & 0x3F; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = n > 31 ? 0 : a.u32_[i] << n; T[RT(raw)] = r; NEXT; }
DEF(SHLHI) { const u32 n = U7(raw) & 0x1F; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 8; ++i) r.u16_[i] = n > 15 ? 0 : static_cast<u16>(a.u16_[i] << n); T[RT(raw)] = r; NEXT; }
DEF(ROT) MAP_W((x << (y & 31)) | (x >> ((32 - (y & 31)) & 31)))
DEF(ROTH) MAP_H((u32(x) << (y & 15)) | (u32(x) >> ((16 - (y & 15)) & 15)))
DEF(ROTI) { const u32 n = U7(raw) & 31; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = (a.u32_[i] << n) | (a.u32_[i] >> ((32 - n) & 31)); T[RT(raw)] = r; NEXT; }
DEF(ROTHI) { const u32 n = U7(raw) & 15; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 8; ++i) r.u16_[i] = static_cast<u16>((u32(a.u16_[i]) << n) | (u32(a.u16_[i]) >> ((16 - n) & 15))); T[RT(raw)] = r; NEXT; }
DEF(ROTM) MAP_W(((0u - y) & 0x3F) > 31 ? 0u : x >> ((0u - y) & 0x3F))
DEF(ROTHM) MAP_H(((0u - y) & 0x1F) > 15 ? 0u : u32(x) >> ((0u - y) & 0x1F))
DEF(ROTMI) { const u32 n = (0u - U7(raw)) & 0x3F; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = n > 31 ? 0 : a.u32_[i] >> n; T[RT(raw)] = r; NEXT; }
DEF(ROTHMI) { const u32 n = (0u - U7(raw)) & 0x1F; const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 8; ++i) r.u16_[i] = n > 15 ? 0 : static_cast<u16>(a.u16_[i] >> n); T[RT(raw)] = r; NEXT; }
DEF(ROTMA) MAP_W(u32(s32(x) >> std::min<u32>((0u - y) & 0x3F, 31)))
DEF(ROTMAH) MAP_H(u16(s16(x) >> std::min<u32>((0u - y) & 0x1F, 15)))
DEF(ROTMAI) { const u32 n = std::min<u32>((0u - U7(raw)) & 0x3F, 31); const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.u32_[i] = u32(s32(a.u32_[i]) >> n); T[RT(raw)] = r; NEXT; }
DEF(ROTMAHI) { const u32 n = std::min<u32>((0u - U7(raw)) & 0x1F, 15); const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 8; ++i) r.u16_[i] = u16(s16(a.u16_[i]) >> n); T[RT(raw)] = r; NEXT; }

// ---------------------------------------------------------------- quadword shifts / rotates
DEF(SHLQBI) { T[RT(raw)] = shl128(T[RA(raw)], pref(T[RB(raw)]) & 7); NEXT; }
DEF(SHLQBII) { T[RT(raw)] = shl128(T[RA(raw)], U7(raw) & 7); NEXT; }
DEF(SHLQBY) { const u32 n = pref(T[RB(raw)]) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shl128(T[RA(raw)], n * 8); NEXT; }
DEF(SHLQBYI) { const u32 n = U7(raw) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shl128(T[RA(raw)], n * 8); NEXT; }
DEF(SHLQBYBI) { const u32 n = (pref(T[RB(raw)]) >> 3) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shl128(T[RA(raw)], n * 8); NEXT; }
DEF(ROTQBI) { T[RT(raw)] = rotl128(T[RA(raw)], pref(T[RB(raw)]) & 7); NEXT; }
DEF(ROTQBII) { T[RT(raw)] = rotl128(T[RA(raw)], U7(raw) & 7); NEXT; }
DEF(ROTQBY) { T[RT(raw)] = rotl128(T[RA(raw)], (pref(T[RB(raw)]) & 0xF) * 8); NEXT; }
DEF(ROTQBYI) { T[RT(raw)] = rotl128(T[RA(raw)], (U7(raw) & 0xF) * 8); NEXT; }
DEF(ROTQBYBI) { T[RT(raw)] = rotl128(T[RA(raw)], ((pref(T[RB(raw)]) >> 3) & 0xF) * 8); NEXT; }
DEF(ROTQMBI) { T[RT(raw)] = shr128(T[RA(raw)], (0u - pref(T[RB(raw)])) & 7); NEXT; }
DEF(ROTQMBII) { T[RT(raw)] = shr128(T[RA(raw)], (0u - U7(raw)) & 7); NEXT; }
DEF(ROTQMBY) { const u32 n = (0u - pref(T[RB(raw)])) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shr128(T[RA(raw)], n * 8); NEXT; }
DEF(ROTQMBYI) { const u32 n = (0u - U7(raw)) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shr128(T[RA(raw)], n * 8); NEXT; }
DEF(ROTQMBYBI) { const u32 n = (0u - (pref(T[RB(raw)]) >> 3)) & 0x1F; T[RT(raw)] = n > 15 ? v128::zero() : shr128(T[RA(raw)], n * 8); NEXT; }

// ---------------------------------------------------------------- compares
DEF(CEQ) MAP_W(MASK(x == y))
DEF(CEQH) MAP_H(x == y ? 0xFFFF : 0)
DEF(CEQB) MAP_B(x == y ? 0xFF : 0)
DEF(CEQI) MAP_WI(MASK(x == u32(imm)))
DEF(CEQHI) MAP_HI(x == u16(imm) ? 0xFFFF : 0)
DEF(CEQBI) MAP_BI(x == imm ? 0xFF : 0)
DEF(CGT) MAP_W(MASK(s32(x) > s32(y)))
DEF(CGTH) MAP_H(s16(x) > s16(y) ? 0xFFFF : 0)
DEF(CGTB) MAP_B(s8(x) > s8(y) ? 0xFF : 0)
DEF(CGTI) MAP_WI(MASK(s32(x) > imm))
DEF(CGTHI) MAP_HI(s16(x) > s16(imm) ? 0xFFFF : 0)
DEF(CGTBI) MAP_BI(s8(x) > s8(imm) ? 0xFF : 0)
DEF(CLGT) MAP_W(MASK(x > y))
DEF(CLGTH) MAP_H(x > y ? 0xFFFF : 0)
DEF(CLGTB) MAP_B(x > y ? 0xFF : 0)
DEF(CLGTI) MAP_WI(MASK(x > u32(imm)))
DEF(CLGTHI) MAP_HI(x > u16(imm) ? 0xFFFF : 0)
DEF(CLGTBI) MAP_BI(x > imm ? 0xFF : 0)

// ---------------------------------------------------------------- single precision float
#define MAP_F(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 4; ++i) { const f32 x = a.f32_[i], y = b.f32_[i]; r.f32_[i] = ftz(expr); } T[RT(raw)] = r; NEXT; }
#define MAP_FC(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 4; ++i) { const f32 x = a.f32_[i], y = b.f32_[i]; r.u32_[i] = MASK(expr); } T[RT(raw)] = r; NEXT; }
DEF(FA) MAP_F(x + y)
DEF(FS) MAP_F(x - y)
DEF(FM) MAP_F(x * y)
DEF(FCEQ) MAP_FC(x == y)
DEF(FCGT) MAP_FC(x > y)
DEF(FCMEQ) MAP_FC(std::fabs(x) == std::fabs(y))
DEF(FCMGT) MAP_FC(std::fabs(x) > std::fabs(y))
DEF(FMA) { const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = ftz(a.f32_[i] * b.f32_[i] + c.f32_[i]); T[RRR_RT(raw)] = r; NEXT; }
DEF(FMS) { const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = ftz(a.f32_[i] * b.f32_[i] - c.f32_[i]); T[RRR_RT(raw)] = r; NEXT; }
DEF(FNMS) { const v128 a = T[RA(raw)], b = T[RB(raw)], c = T[RC(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = ftz(c.f32_[i] - a.f32_[i] * b.f32_[i]); T[RRR_RT(raw)] = r; NEXT; }
// Estimates are computed exactly; FI (the refinement step) passes the estimate through.
DEF(FREST) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = ftz(1.0f / a.f32_[i]); T[RT(raw)] = r; NEXT; }
DEF(FRSQEST) { const v128 a = T[RA(raw)]; v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = ftz(1.0f / std::sqrt(std::fabs(a.f32_[i]))); T[RT(raw)] = r; NEXT; }
DEF(FI) { T[RT(raw)] = T[RB(raw)]; NEXT; }
DEF(CFLTS) {
    const v128 a = T[RA(raw)];
    const int scale = 173 - static_cast<int>(I8(raw));
    v128 r;
    for (int i = 0; i < 4; ++i) {
        const f64 x = std::ldexp(static_cast<f64>(a.f32_[i]), scale);
        r.s32_[i] = std::isnan(x) ? 0 : x >= 2147483647.0 ? 0x7FFFFFFF : x <= -2147483648.0 ? s32(0x80000000) : s32(x);
    }
    T[RT(raw)] = r;
    NEXT;
}
DEF(CFLTU) {
    const v128 a = T[RA(raw)];
    const int scale = 173 - static_cast<int>(I8(raw));
    v128 r;
    for (int i = 0; i < 4; ++i) {
        const f64 x = std::ldexp(static_cast<f64>(a.f32_[i]), scale);
        r.u32_[i] = std::isnan(x) || x <= 0.0 ? 0u : x >= 4294967295.0 ? 0xFFFFFFFFu : u32(x);
    }
    T[RT(raw)] = r;
    NEXT;
}
DEF(CSFLT) { const v128 a = T[RA(raw)]; const int scale = 155 - static_cast<int>(I8(raw)); v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = static_cast<f32>(std::ldexp(static_cast<f64>(a.s32_[i]), -scale)); T[RT(raw)] = r; NEXT; }
DEF(CUFLT) { const v128 a = T[RA(raw)]; const int scale = 155 - static_cast<int>(I8(raw)); v128 r; for (int i = 0; i < 4; ++i) r.f32_[i] = static_cast<f32>(std::ldexp(static_cast<f64>(a.u32_[i]), -scale)); T[RT(raw)] = r; NEXT; }

// ---------------------------------------------------------------- double precision
#define MAP_D(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 2; ++i) { const f64 x = dbl(a, i), y = dbl(b, i); set_dbl(r, i, expr); } T[RT(raw)] = r; NEXT; }
#define MAP_DT(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128& r = T[RT(raw)]; for (int i = 0; i < 2; ++i) { const f64 x = dbl(a, i), y = dbl(b, i), z = dbl(r, i); set_dbl(r, i, expr); } NEXT; }
#define MAP_DC(expr) { const v128 a = T[RA(raw)], b = T[RB(raw)]; v128 r; for (int i = 0; i < 2; ++i) { const f64 x = dbl(a, i), y = dbl(b, i); const u64 m = (expr) ? ~0ull : 0ull; if (i) r.u64_[0] = m; else r.u64_[1] = m; } T[RT(raw)] = r; NEXT; }
DEF(DFA) MAP_D(x + y)
DEF(DFS) MAP_D(x - y)
DEF(DFM) MAP_D(x * y)
DEF(DFMA) MAP_DT(x * y + z)
DEF(DFMS) MAP_DT(x * y - z)
DEF(DFNMS) MAP_DT(z - x * y)
DEF(DFNMA) MAP_DT(-(x * y + z))
DEF(DFCEQ) MAP_DC(x == y)
DEF(DFCGT) MAP_DC(x > y)
DEF(DFCMEQ) MAP_DC(std::fabs(x) == std::fabs(y))
DEF(DFCMGT) MAP_DC(std::fabs(x) > std::fabs(y))
DEF(DFTSV) {
    const v128 a = T[RA(raw)];
    const u32 m = U7(raw);
    v128 r;
    for (int i = 0; i < 2; ++i) {
        const f64 x = dbl(a, i);
        const bool neg = std::signbit(x);
        bool hit = false;
        if ((m & 0x40) && std::isnan(x)) hit = true;
        if ((m & 0x20) && std::isinf(x) && !neg) hit = true;
        if ((m & 0x10) && std::isinf(x) && neg) hit = true;
        if ((m & 0x08) && x == 0.0 && !neg) hit = true;
        if ((m & 0x04) && x == 0.0 && neg) hit = true;
        if ((m & 0x02) && std::fpclassify(x) == FP_SUBNORMAL && !neg) hit = true;
        if ((m & 0x01) && std::fpclassify(x) == FP_SUBNORMAL && neg) hit = true;
        const u64 v = hit ? ~0ull : 0ull;
        if (i) r.u64_[0] = v; else r.u64_[1] = v;
    }
    T[RT(raw)] = r;
    NEXT;
}
DEF(FESD) { const v128 a = T[RA(raw)]; v128 r; set_dbl(r, 0, a.fw(0)); set_dbl(r, 1, a.fw(2)); T[RT(raw)] = r; NEXT; }
DEF(FRDS) { const v128 a = T[RA(raw)]; v128 r = v128::zero(); r.fw(0) = static_cast<f32>(dbl(a, 0)); r.fw(2) = static_cast<f32>(dbl(a, 1)); T[RT(raw)] = r; NEXT; }

DEF(INVALID) { t.stopped = true; t.stop_code = 0x20000; NEXT; }

#undef MAP_W
#undef MAP_H
#undef MAP_B
#undef MAP_WI
#undef MAP_HI
#undef MAP_BI
#undef MAP_F
#undef MAP_FC
#undef MAP_D
#undef MAP_DT
#undef MAP_DC
#undef MASK
#undef DEF
#undef NEXT
#undef T

}  // namespace opentit::spu::ops
