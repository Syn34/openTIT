// Cell SPU instruction decoder.
#pragma once

#include <string>

#include "common/types.h"

namespace opentit::spu {

enum class Op : u16 {
    INVALID = 0,
#define OP(name, bits, opc, form) name,
#include "spu/spu_ops.inc"
#undef OP
    COUNT
};

enum class Form : u8 { NONE, RR, RRR, RI7, RI8, RI10, RI16, RI18 };

const char* op_name(Op op);
Form op_form(Op op);

// O(1): one table lookup on the top 11 bits.
Op decode(u32 raw);

struct Insn {
    u32 raw;
    constexpr explicit Insn(u32 r) : raw(r) {}
    // RR / RI7 / RI8 / RI10 / RI16 / RI18: rt is always the low 7 bits.
    constexpr u32 rt() const { return raw & 0x7F; }
    constexpr u32 ra() const { return (raw >> 7) & 0x7F; }
    constexpr u32 rb() const { return (raw >> 14) & 0x7F; }
    // RRR: target in bits 4-10, third source in the low 7 bits.
    constexpr u32 rrr_rt() const { return (raw >> 21) & 0x7F; }
    constexpr u32 rc() const { return raw & 0x7F; }
    constexpr s32 i7() const { return static_cast<s32>(static_cast<s8>(static_cast<u8>(((raw >> 14) & 0x7F) << 1)) >> 1); }
    constexpr u32 u7() const { return (raw >> 14) & 0x7F; }
    constexpr u32 i8() const { return (raw >> 14) & 0xFF; }
    constexpr s32 i10() const { return static_cast<s32>(static_cast<s16>(static_cast<u16>(((raw >> 14) & 0x3FF) << 6)) >> 6); }
    constexpr u32 u10() const { return (raw >> 14) & 0x3FF; }
    constexpr s32 i16() const { return static_cast<s16>(static_cast<u16>((raw >> 7) & 0xFFFF)); }
    constexpr u32 u16_() const { return (raw >> 7) & 0xFFFF; }
    constexpr u32 i18() const { return (raw >> 7) & 0x3FFFF; }
};

bool is_branch(Op op);   // control transfer (excluding stop)

std::string disasm(u32 raw, u32 pc);

}  // namespace opentit::spu
