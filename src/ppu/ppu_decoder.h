// Cell Broadband Engine PPU instruction decoder (PowerPC 64 + VMX).
//
// Field accessors follow the IBM bit numbering used by the PowerPC manuals:
// bit 0 is the most significant bit of the 32-bit instruction word.
#pragma once

#include <string>

#include "common/types.h"

namespace opentit::ppu {

enum class Op : u16 {
#define OP(name) name,
#include "ppu/ppu_ops.inc"
#undef OP
    COUNT
};

const char* op_name(Op op);

struct Insn {
    u32 raw = 0;

    constexpr Insn() = default;
    constexpr explicit Insn(u32 r) : raw(r) {}

    // Extract IBM bits [from, to] inclusive.
    constexpr u32 bits(int from, int to) const { return (raw >> (31 - to)) & ((1u << (to - from + 1)) - 1); }

    constexpr u32 opcd() const { return raw >> 26; }
    constexpr u32 rd() const { return bits(6, 10); }  // also rs / rt / frt / frs / vd / vs / bo / to
    constexpr u32 rs() const { return rd(); }
    constexpr u32 ra() const { return bits(11, 15); }  // also bi / fra / va
    constexpr u32 rb() const { return bits(16, 20); }  // also frb / vb / sh / nb
    constexpr u32 rc_reg() const { return bits(21, 25); }  // frc (A-form) / vc (VA-form)
    constexpr u32 bo() const { return rd(); }
    constexpr u32 bi() const { return ra(); }
    constexpr bool rc() const { return raw & 1; }
    constexpr bool lk() const { return raw & 1; }
    constexpr bool aa() const { return (raw >> 1) & 1; }
    constexpr bool oe() const { return (raw >> 10) & 1; }
    constexpr s32 simm16() const { return static_cast<s16>(raw & 0xFFFF); }
    constexpr u32 uimm16() const { return raw & 0xFFFF; }
    constexpr s32 ds() const { return static_cast<s16>(raw & 0xFFFC); }
    constexpr s32 bd() const { return static_cast<s16>(raw & 0xFFFC); }
    constexpr s32 li() const {
        u32 v = raw & 0x03FFFFFC;
        if (v & 0x02000000) v |= 0xFC000000;
        return static_cast<s32>(v);
    }
    constexpr u32 crfd() const { return bits(6, 8); }
    constexpr u32 crfs() const { return bits(11, 13); }
    constexpr bool l10() const { return bits(10, 10); }  // cmp L bit (64-bit compare)
    constexpr u32 crbd() const { return rd(); }
    constexpr u32 crba() const { return ra(); }
    constexpr u32 crbb() const { return rb(); }
    constexpr u32 sh32() const { return rb(); }
    constexpr u32 mb32() const { return bits(21, 25); }
    constexpr u32 me32() const { return bits(26, 30); }
    constexpr u32 sh64() const { return bits(16, 20) | (bits(30, 30) << 5); }
    constexpr u32 mbe64() const { return bits(21, 25) | (bits(26, 26) << 5); }
    constexpr u32 spr() const { return bits(11, 15) | (bits(16, 20) << 5); }
    constexpr u32 crm() const { return bits(12, 19); }
    constexpr bool crm_one() const { return bits(11, 11); }  // mfocrf / mtocrf
    constexpr u32 flm() const { return bits(7, 14); }
    constexpr u32 imm_fpscr() const { return bits(16, 19); }
    constexpr u32 vuimm() const { return ra(); }
    constexpr s32 vsimm() const {
        s32 v = static_cast<s32>(ra());
        return (v & 0x10) ? v - 32 : v;
    }
    constexpr u32 vsh() const { return bits(22, 25); }
    constexpr bool vrc() const { return bits(21, 21); }
};

Op decode(u32 raw);

// Returns true for instructions that end a basic block.
bool is_branch(Op op);

}  // namespace opentit::ppu
