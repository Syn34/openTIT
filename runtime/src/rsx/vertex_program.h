// RSX vertex program microcode: decoder, disassembler and analysis.
//
// Each instruction is 128 bits (four big-endian words D0..D3) and carries a
// vector op and a scalar op that execute together. Both read up to three
// source operands from temps (R0-R47), vertex inputs (v0-v15) or constants
// (c[0..467]); results go to temps, the address register, the condition
// register, or output attributes (o[0..15]).
#pragma once

#include <array>
#include <set>
#include <string>
#include <vector>

#include "common/types.h"

namespace opentit::rsx::vp {

enum VecOp : u32 {
    VEC_NOP = 0x00, VEC_MOV, VEC_MUL, VEC_ADD, VEC_MAD, VEC_DP3, VEC_DPH, VEC_DP4, VEC_DST, VEC_MIN, VEC_MAX,
    VEC_SLT, VEC_SGE, VEC_ARL, VEC_FRC, VEC_FLR, VEC_SEQ, VEC_SFL, VEC_SGT, VEC_SLE, VEC_SNE, VEC_STR, VEC_SSG,
    VEC_TXL = 0x19,
};
enum ScaOp : u32 {
    SCA_NOP = 0x00, SCA_MOV, SCA_RCP, SCA_RCC, SCA_RSQ, SCA_EXP, SCA_LOG, SCA_LIT, SCA_BRA, SCA_BRI, SCA_CAL,
    SCA_CLI, SCA_RET, SCA_LG2, SCA_EX2, SCA_SIN, SCA_COS, SCA_BRB, SCA_CLB, SCA_PSH, SCA_POP,
};
enum RegType : u32 { REG_TEMP = 1, REG_INPUT = 2, REG_CONST = 3 };

// Condition bits (cond field): lt=1, eq=2, gt=4
enum : u32 { COND_LT = 1, COND_EQ = 2, COND_GT = 4 };

struct Source {
    u32 type = 0;      // RegType
    u32 index = 0;     // temp index; for inputs/constants the instruction's input_src/const_src
    u8 swizzle[4] = {0, 1, 2, 3};
    bool neg = false;
    bool abs = false;
};

struct Insn {
    u32 d[4] = {};

    // ---- D0
    u32 addr_swz() const { return d[0] & 3; }
    u32 mask_w() const { return (d[0] >> 2) & 3; }   // condition swizzle
    u32 mask_z() const { return (d[0] >> 4) & 3; }
    u32 mask_y() const { return (d[0] >> 6) & 3; }
    u32 mask_x() const { return (d[0] >> 8) & 3; }
    u32 cond() const { return (d[0] >> 10) & 7; }
    bool cond_test() const { return (d[0] >> 13) & 1; }
    bool cond_update0() const { return (d[0] >> 14) & 1; }
    u32 dst_tmp() const { return (d[0] >> 15) & 0x3F; }
    bool src_abs(int n) const { return (d[0] >> (21 + n)) & 1; }
    u32 addr_reg_sel() const { return (d[0] >> 24) & 1; }
    u32 cond_reg_sel() const { return (d[0] >> 25) & 1; }
    bool saturate() const { return (d[0] >> 26) & 1; }
    bool index_input() const { return (d[0] >> 27) & 1; }
    bool cond_update1() const { return (d[0] >> 29) & 1; }
    bool vec_result() const { return (d[0] >> 30) & 1; }  // vector op writes o[dst], scalar writes temps
    // ---- D1
    u32 input_src() const { return (d[1] >> 8) & 0xF; }
    u32 const_src() const { return (d[1] >> 12) & 0x3FF; }
    u32 vec_op() const { return (d[1] >> 22) & 0x1F; }
    u32 sca_op() const { return (d[1] >> 27) & 0x1F; }
    // ---- D2
    u32 tex_num() const { return (d[2] >> 8) & 3; }
    // ---- D3
    bool end() const { return d[3] & 1; }
    bool index_const() const { return (d[3] >> 1) & 1; }
    u32 dst() const { return (d[3] >> 2) & 0x1F; }  // output attribute, 0x1F = none
    u32 sca_dst_tmp() const { return (d[3] >> 7) & 0x3F; }
    u32 vec_mask() const { return (d[3] >> 13) & 0xF; }  // bit 3 = x .. bit 0 = w
    u32 sca_mask() const { return (d[3] >> 17) & 0xF; }
    u32 branch_index() const { return (d[3] >> 23) & 0x1F; }
    bool brb_cond_true() const { return (d[3] >> 28) & 1; }
    // Branch target (instruction index) for BRA/BRI/CAL/CLI/BRB/CLB.
    u32 branch_target() const {
        return (((d[0] >> 23) & 1) << 9) | ((d[2] & 0x3F) << 3) | (d[3] >> 29);
    }

    u32 raw_src(int n) const;  // the 17-bit operand field
    Source src(int n) const;
    bool vec_writes() const { return vec_op() != VEC_NOP; }
    bool sca_writes() const { return sca_op() != SCA_NOP && !is_flow(); }
    bool is_flow() const {
        const u32 s = sca_op();
        return s == SCA_BRA || s == SCA_BRI || s == SCA_CAL || s == SCA_CLI || s == SCA_RET || s == SCA_BRB ||
               s == SCA_CLB;
    }
};

Insn fetch(const u32* ucode, u32 index);  // ucode = 4 words per instruction (host order values)

const char* vec_op_name(u32 op);
const char* sca_op_name(u32 op);
const char* input_name(u32 index);   // "in_pos", "in_weight", ...
const char* output_name(u32 index);  // "HPOS", "COL0", ...

// One or two lines of assembly ("MUL R0.xyz, v0, c[4]; RSQ R1.x, R0.w").
std::string disasm(const Insn& i);

struct Program {
    u32 start = 0;                   // first instruction slot
    std::vector<Insn> insns;         // start .. the END instruction (inclusive)
    std::set<u32> branch_targets;    // absolute slots
    u32 inputs_read = 0;             // bitmask of v0..v15
    u32 outputs_written = 0;         // bitmask of o[0..15]
    u64 temps_used = 0;              // bitmask of R0..R47 (written or read)
    std::set<u32> constants;         // directly addressed constants
    bool indexed_constants = false;  // c[a0.x + n]
    bool uses_textures = false;      // TXL (vertex texture fetch)
    bool has_end = false;            // false: ran off the end of program memory
};

// Collect a program starting at `start`, following branches, up to the END
// flag. `ucode` holds 512 instructions.
Program analyze(const u32* ucode, u32 start);

}  // namespace opentit::rsx::vp
