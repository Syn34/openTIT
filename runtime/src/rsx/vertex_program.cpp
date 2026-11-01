#include "vertex_program.h"

#include <algorithm>
#include <cstdio>

namespace opentit::rsx::vp {

namespace {
const char* const kVecNames[32] = {"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DPH", "DP4", "DST", "MIN", "MAX",
                                   "SLT", "SGE", "ARL", "FRC", "FLR", "SEQ", "SFL", "SGT", "SLE", "SNE", "STR",
                                   "SSG", "VEC17", "VEC18", "TXL"};
const char* const kScaNames[32] = {"NOP", "MOV", "RCP", "RCC", "RSQ", "EXP", "LOG", "LIT", "BRA", "BRI", "CAL",
                                   "CLI", "RET", "LG2", "EX2", "SIN", "COS", "BRB", "CLB", "PSH", "POP"};
const char* const kInputs[16] = {"in_pos", "in_weight", "in_normal", "in_diff_color", "in_spec_color", "in_fog",
                                 "in_point_size", "in_7", "in_tc0", "in_tc1", "in_tc2", "in_tc3",
                                 "in_tc4", "in_tc5", "in_tc6", "in_tc7"};
const char* const kOutputs[16] = {"HPOS", "COL0", "COL1", "BFC0", "BFC1", "FOGC", "PSZ", "TEX0",
                                  "TEX1", "TEX2", "TEX3", "TEX4", "TEX5", "TEX6", "TEX7", "O15"};

// Which of src0/src1/src2 each vector op reads (bit n = src n).
u32 vec_sources(u32 op) {
    switch (op) {
    case VEC_NOP: return 0;
    case VEC_MOV: case VEC_ARL: case VEC_FRC: case VEC_FLR: case VEC_SSG: case VEC_TXL: return 1;
    case VEC_ADD: return 1 | 4;
    case VEC_MAD: return 7;
    case VEC_SFL: case VEC_STR: return 0;
    default: return 3;
    }
}
}  // namespace

const char* vec_op_name(u32 op) { return op < 32 && kVecNames[op] ? kVecNames[op] : "VEC?"; }
const char* sca_op_name(u32 op) { return op < 32 && kScaNames[op] ? kScaNames[op] : "SCA?"; }
const char* input_name(u32 i) { return kInputs[i & 15]; }
const char* output_name(u32 i) { return kOutputs[i & 15]; }

u32 Insn::raw_src(int n) const {
    switch (n) {
    case 0: return ((d[1] & 0xFF) << 9) | (d[2] >> 23);
    case 1: return (d[2] >> 6) & 0x1FFFF;
    default: return ((d[2] & 0x3F) << 11) | (d[3] >> 21);
    }
}

Source Insn::src(int n) const {
    const u32 r = raw_src(n);
    Source s;
    s.type = r & 3;
    s.index = s.type == REG_TEMP ? (r >> 2) & 0x3F : s.type == REG_INPUT ? input_src() : const_src();
    s.swizzle[3] = (r >> 8) & 3;
    s.swizzle[2] = (r >> 10) & 3;
    s.swizzle[1] = (r >> 12) & 3;
    s.swizzle[0] = (r >> 14) & 3;
    s.neg = (r >> 16) & 1;
    s.abs = src_abs(n);
    return s;
}

Insn fetch(const u32* ucode, u32 index) {
    Insn i;
    for (int k = 0; k < 4; ++k) i.d[k] = ucode[(index & 511) * 4 + k];
    return i;
}

namespace {
std::string mask_str(u32 m) {  // bit 3 = x .. bit 0 = w
    if (m == 0xF) return "";
    std::string s = ".";
    if (m & 8) s += 'x';
    if (m & 4) s += 'y';
    if (m & 2) s += 'z';
    if (m & 1) s += 'w';
    return s;
}

std::string src_str(const Insn& i, int n) {
    const Source s = i.src(n);
    std::string r;
    char buf[48];
    switch (s.type) {
    case REG_TEMP: std::snprintf(buf, sizeof buf, "R%u", s.index); break;
    case REG_INPUT: std::snprintf(buf, sizeof buf, "v%u", s.index); break;
    case REG_CONST:
        if (i.index_const()) std::snprintf(buf, sizeof buf, "c[A%u.%c+%u]", i.addr_reg_sel(), "xyzw"[i.addr_swz()], s.index);
        else std::snprintf(buf, sizeof buf, "c[%u]", s.index);
        break;
    default: std::snprintf(buf, sizeof buf, "?%u", s.type); break;
    }
    r = buf;
    if (!(s.swizzle[0] == 0 && s.swizzle[1] == 1 && s.swizzle[2] == 2 && s.swizzle[3] == 3)) {
        r += '.';
        if (s.swizzle[0] == s.swizzle[1] && s.swizzle[1] == s.swizzle[2] && s.swizzle[2] == s.swizzle[3]) {
            r += "xyzw"[s.swizzle[0]];
        } else {
            for (int k = 0; k < 4; ++k) r += "xyzw"[s.swizzle[k]];
        }
    }
    if (s.abs) r = "|" + r + "|";
    if (s.neg) r = "-" + r;
    return r;
}

std::string cond_suffix(const Insn& i) {
    if (!i.cond_test() || i.cond() == 7) return "";
    static const char* const kCond[8] = {"FL", "LT", "EQ", "LE", "GT", "NE", "GE", "TR"};
    std::string s = std::string(" (") + kCond[i.cond()] + std::to_string(i.cond_reg_sel());
    const u32 sw[4] = {i.mask_x(), i.mask_y(), i.mask_z(), i.mask_w()};
    if (!(sw[0] == 0 && sw[1] == 1 && sw[2] == 2 && sw[3] == 3)) {
        s += '.';
        for (u32 c : sw) s += "xyzw"[c];
    }
    return s + ")";
}

std::string dst_str(const Insn& i, bool sca) {
    const u32 mask = sca ? i.sca_mask() : i.vec_mask();
    const u32 tmp = sca ? i.sca_dst_tmp() : i.dst_tmp();
    const bool result = sca ? !i.vec_result() : i.vec_result();
    std::string out;
    if (result && i.dst() != 0x1F) out = std::string("o[") + output_name(i.dst()) + "]" + mask_str(mask);
    if (tmp != 0x3F) {
        const bool arl = !sca && i.vec_op() == VEC_ARL;
        const std::string t = (arl ? "A" : "R") + std::to_string(tmp) + mask_str(mask);
        out = out.empty() ? t : out + "+" + t;
    }
    if (out.empty()) out = "CC" + std::to_string(i.cond_reg_sel()) + mask_str(mask);
    return out;
}
}  // namespace

std::string disasm(const Insn& i) {
    std::string out;
    const u32 vop = i.vec_op(), sop = i.sca_op();
    const bool sat = i.saturate();
    if (vop != VEC_NOP) {
        out += vec_op_name(vop);
        if (sat) out += "_sat";
        if (i.cond_update0() || i.cond_update1()) out += "C";
        out += " " + dst_str(i, false);
        const u32 used = vec_sources(vop);
        for (int n = 0; n < 3; ++n)
            if (used & (1u << n)) out += ", " + src_str(i, n);
        if (vop == VEC_TXL) out += ", TEX" + std::to_string(i.tex_num());
        out += cond_suffix(i);
    }
    if (sop != SCA_NOP) {
        if (!out.empty()) out += "; ";
        out += sca_op_name(sop);
        if (i.is_flow()) {
            if (sop == SCA_RET) {
                // no operands
            } else if (sop == SCA_BRB || sop == SCA_CLB) {
                out += " " + std::to_string(i.branch_target()) + (i.brb_cond_true() ? " if b" : " if !b") +
                       std::to_string(i.branch_index());
            } else {
                out += " " + std::to_string(i.branch_target());
            }
            out += cond_suffix(i);
        } else {
            if (sat) out += "_sat";
            out += " " + dst_str(i, true) + ", " + src_str(i, 2);
            if (vop == VEC_NOP) out += cond_suffix(i);
        }
    }
    if (out.empty()) out = "NOP";
    if (i.end()) out += "  END";
    return out;
}

Program analyze(const u32* ucode, u32 start) {
    Program p;
    p.start = start & 511;
    std::vector<bool> seen(512, false);
    std::vector<u32> work{p.start};
    u32 last = p.start;
    while (!work.empty()) {
        u32 pc = work.back();
        work.pop_back();
        while (pc < 512 && !seen[pc]) {
            seen[pc] = true;
            last = std::max(last, pc);
            const Insn i = fetch(ucode, pc);
            const u32 sop = i.sca_op();
            if (i.is_flow() && sop != SCA_RET) {
                const u32 t = i.branch_target();
                p.branch_targets.insert(t);
                work.push_back(t);
            }
            if (i.end()) {
                p.has_end = true;
                break;
            }
            if (sop == SCA_RET) break;
            ++pc;
        }
    }
    for (u32 pc = p.start; pc <= last; ++pc) {
        const Insn i = fetch(ucode, pc);
        p.insns.push_back(i);
        const u32 vop = i.vec_op(), sop = i.sca_op();
        u32 used = vec_sources(vop);
        if (sop != SCA_NOP && !i.is_flow()) used |= 4;
        for (int n = 0; n < 3; ++n) {
            if (!(used & (1u << n))) continue;
            const Source s = i.src(n);
            if (s.type == REG_INPUT) p.inputs_read |= 1u << s.index;
            else if (s.type == REG_TEMP) p.temps_used |= 1ull << s.index;
            else if (s.type == REG_CONST) {
                if (i.index_const()) p.indexed_constants = true;
                else if (s.index < 468) p.constants.insert(s.index);
            }
        }
        if (vop == VEC_TXL) p.uses_textures = true;
        if (vop != VEC_NOP) {
            if (i.vec_result() && i.dst() != 0x1F) p.outputs_written |= 1u << i.dst();
            if (i.dst_tmp() != 0x3F && vop != VEC_ARL) p.temps_used |= 1ull << i.dst_tmp();
        }
        if (sop != SCA_NOP && !i.is_flow()) {
            if (!i.vec_result() && i.dst() != 0x1F) p.outputs_written |= 1u << i.dst();
            if (i.sca_dst_tmp() != 0x3F) p.temps_used |= 1ull << i.sca_dst_tmp();
        }
    }
    return p;
}

}  // namespace opentit::rsx::vp
