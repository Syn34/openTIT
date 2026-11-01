// Architectural state of one PPU hardware thread, as seen by recompiled code.
#pragma once

#include <cstring>

#include "common/types.h"
#include "opentit/vmx.h"

namespace opentit {

struct PPUContext {
    u64 gpr[32] = {};
    f64 fpr[32] = {};
    v128 vr[32] = {};

    // Condition register stored one bit per byte, indexed by IBM CR bit
    // number (cr[4*n + 0..3] = LT, GT, EQ, SO of field n).
    u8 cr[32] = {};

    u64 lr = 0;
    u64 ctr = 0;
    u8 xer_so = 0;
    u8 xer_ov = 0;
    u8 xer_ca = 0;
    u8 xer_cnt = 0;  // byte count for lswx / stswx
    u32 fpscr = 0;
    u32 vscr = 0x00010000;  // NJ=1 by default on PS3
    u32 vrsave = 0;

    // Load-reserve state (lwarx/ldarx -> stwcx./stdcx.)
    u32 reserve_addr = 0xFFFFFFFF;
    u64 reserve_value = 0;  // big-endian raw value at reservation time

    // Runtime bookkeeping (not architectural).
    u32 thread_id = 0;
    u32 stack_addr = 0;
    u32 stack_size = 0;
    u64 hle_last_nid = 0;
    bool exit_requested = false;

    // FPR bit-level access
    u64 fpr_bits(int n) const {
        u64 b;
        std::memcpy(&b, &fpr[n], 8);
        return b;
    }
    void set_fpr_bits(int n, u64 b) { std::memcpy(&fpr[n], &b, 8); }

    u32 get_cr() const {
        u32 v = 0;
        for (int i = 0; i < 32; ++i) v |= u32(cr[i] & 1) << (31 - i);
        return v;
    }
    void set_cr(u32 v) {
        for (int i = 0; i < 32; ++i) cr[i] = (v >> (31 - i)) & 1;
    }
    u64 get_xer() const {
        return (u64(xer_so) << 31) | (u64(xer_ov) << 30) | (u64(xer_ca) << 29) | xer_cnt;
    }
    void set_xer(u64 v) {
        xer_so = (v >> 31) & 1;
        xer_ov = (v >> 30) & 1;
        xer_ca = (v >> 29) & 1;
        xer_cnt = v & 0x7F;
    }
};

// Calling convention helpers for HLE functions (PPC64 ELFv1: args in r3..r10,
// return in r3; float args in f1..f13).
inline u64 arg(const PPUContext& ctx, int n) { return ctx.gpr[3 + n]; }
inline u32 arg32(const PPUContext& ctx, int n) { return static_cast<u32>(ctx.gpr[3 + n]); }
inline void ret(PPUContext& ctx, u64 v) { ctx.gpr[3] = v; }

}  // namespace opentit
