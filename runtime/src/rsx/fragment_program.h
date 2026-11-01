// RSX fragment program microcode: reader, disassembler and analysis.
//
// Fragment programs live in guest memory (RSX local or main) at the address
// set by NV4097_SET_SHADER_PROGRAM. Each instruction is 16 bytes: four
// words, each stored with its 16-bit halves swapped (read big endian, then
// rotate by 16). An instruction that reads a constant is followed by a
// 16-byte slot holding that constant's value inline. The last instruction
// has the END bit set.
#pragma once

#include <array>
#include <string>
#include <vector>

#include "common/types.h"

namespace opentit::rsx::fp {

enum Op : u32 {
    NOP = 0x00, MOV, MUL, ADD, MAD, DP3, DP4, DST, MIN, MAX, SLT, SGE, SLE, SGT, SNE, SEQ, FRC, FLR, KIL, PK4, UP4,
    DDX, DDY, TEX, TXP, TXD, RCP, RSQ, EX2, LG2, LIT, LRP, STR, SFL, COS, SIN, PK2, UP2, POW, PKB, UPB, PK16, UP16,
    BEM, PKG, UPG, DP2A, TXL, TXB = 0x31, TEXBEM = 0x33, TXPBEM, BEMLUM, REFL, TIMESWTEX, DP2, NRM, DIV, DIVSQ, LIF,
    FENCT, FENCB, BRK = 0x40, CAL, IFE, LOOP, REP, RET,
};
enum RegType : u32 { REG_TEMP = 0, REG_INPUT = 1, REG_CONST = 2 };

// Interpolated inputs (src_attr_reg_num)
enum Input : u32 { IN_WPOS = 0, IN_COL0, IN_COL1, IN_FOGC, IN_TEX0, IN_TEX9 = IN_TEX0 + 9, IN_SSA };

struct Source {
    u32 type = 0;
    u32 index = 0;  // temp index, or the instruction's input attribute
    bool half = false;
    u8 swizzle[4] = {0, 1, 2, 3};
    bool neg = false;
    bool abs = false;
};

struct Insn {
    u32 offset = 0;          // byte offset from the program start
    u32 d[4] = {};           // dst, src0, src1, src2 after the halfword swap
    bool has_constant = false;
    u32 constant[4] = {};    // float bits of the inline constant

    // ---- dst (word 0)
    bool end() const { return d[0] & 1; }
    u32 dest_reg() const { return (d[0] >> 1) & 0x3F; }
    bool dest_half() const { return (d[0] >> 7) & 1; }
    bool set_cond() const { return (d[0] >> 8) & 1; }
    u32 write_mask() const { return (d[0] >> 9) & 0xF; }  // bit 0 = x .. bit 3 = w
    u32 input_attr() const { return (d[0] >> 13) & 0xF; }
    u32 tex_num() const { return (d[0] >> 17) & 0xF; }
    bool exp_tex() const { return (d[0] >> 21) & 1; }   // _bx2
    u32 precision() const { return (d[0] >> 22) & 3; }  // 0 fp32, 1 fp16, 2 fx12, 3 fx9
    bool no_dest() const { return (d[0] >> 30) & 1; }
    bool saturate() const { return (d[0] >> 31) & 1; }
    u32 opcode() const { return ((d[0] >> 24) & 0x3F) | (((d[2] >> 31) & 1) << 6); }
    // ---- src0 (word 1): execution condition
    bool exec_lt() const { return (d[1] >> 18) & 1; }
    bool exec_eq() const { return (d[1] >> 19) & 1; }
    bool exec_gt() const { return (d[1] >> 20) & 1; }
    u32 cond_swizzle(int c) const { return (d[1] >> (21 + 2 * c)) & 3; }
    u32 cond_mod_reg() const { return (d[1] >> 30) & 1; }  // which CC set_cond updates
    u32 cond_reg() const { return (d[1] >> 31) & 1; }      // which CC the condition reads
    // ---- src1 (word 2)
    u32 scale() const { return (d[2] >> 28) & 7; }  // 1 x2, 2 x4, 3 x8, 5 /2, 6 /4, 7 /8
    u32 src_precision(int n) const { return (d[2] >> (19 + 3 * n)) & 7; }
    u32 else_offset() const { return d[2] & 0x7FFFFFFF; }  // IFE (in instructions)
    u32 rep_count() const { return (d[2] >> 2) & 0xFF; }   // LOOP/REP
    u32 loop_init() const { return (d[2] >> 10) & 0xFF; }
    u32 loop_increment() const { return (d[2] >> 19) & 0xFF; }
    // ---- src2 (word 3)
    u32 end_offset() const { return d[3]; }  // IFE/LOOP/REP (in instructions)
    u32 index_offset() const { return (d[3] >> 19) & 0x7FF; }
    bool use_index_reg() const { return (d[3] >> 30) & 1; }
    bool perspective_correct() const { return (d[3] >> 31) & 1; }

    Source src(int n) const;
    bool always_executes() const { return exec_lt() && exec_eq() && exec_gt(); }
    bool never_executes() const { return !exec_lt() && !exec_eq() && !exec_gt(); }
    bool is_texture() const;
    bool is_flow() const { return opcode() >= BRK; }
};

const char* op_name(u32 op);
const char* input_name(u32 attr);  // "WPOS", "COL0", ..., "TEX9", "SSA"
int source_count(u32 op);          // operands read by an op (0-3)

struct Program {
    u32 start = 0;              // byte offset of the first instruction (leading NOPs skipped)
    u32 size = 0;               // bytes from start through the END instruction's constant slot
    std::vector<Insn> insns;
    u32 textures = 0;           // bitmask of texture units sampled
    u32 inputs = 0;             // bitmask of input attributes read
    u32 temps_written = 0;      // R0-R31 (bit n); outputs are R0 (color), R1.z (depth), R2-R4 (MRT)
    u32 halves_written = 0;     // H0-H31 for half-precision destinations
    bool has_branches = false;
    bool has_kill = false;
    bool writes_depth = false;  // R1.z written at full precision
    bool truncated = false;     // no END found within the byte limit
};

// Parse from a byte buffer in guest (big-endian) order. `limit` bounds the
// scan; real programs are a few hundred bytes to a few KiB.
Program parse(const u8* data, u32 limit);

std::string disasm(const Insn& i);

}  // namespace opentit::rsx::fp
