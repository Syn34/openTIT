#include "fragment_program.h"

#include <cstdio>
#include <cstring>

namespace opentit::rsx::fp {

namespace {
const char* const kOpNames[0x46] = {
    "NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DP4", "DST", "MIN", "MAX", "SLT", "SGE", "SLE", "SGT",
    "SNE", "SEQ", "FRC", "FLR", "KIL", "PK4", "UP4", "DDX", "DDY", "TEX", "TXP", "TXD", "RCP", "RSQ",
    "EX2", "LG2", "LIT", "LRP", "STR", "SFL", "COS", "SIN", "PK2", "UP2", "POW", "PKB", "UPB", "PK16",
    "UP16", "BEM", "PKG", "UPG", "DP2A", "TXL", "OP30", "TXB", "OP32", "TEXBEM", "TXPBEM", "BEMLUM", "REFL", "TIMESWTEX",
    "DP2", "NRM", "DIV", "DIVSQ", "LIF", "FENCT", "FENCB", "OP3F", "BRK", "CAL", "IFE", "LOOP", "REP", "RET"};
const char* const kInputs[16] = {"WPOS", "COL0", "COL1", "FOGC", "TEX0", "TEX1", "TEX2", "TEX3",
                                 "TEX4", "TEX5", "TEX6", "TEX7", "TEX8", "TEX9", "SSA", "IN15"};

u32 read_word(const u8* p) {
    const u32 be = load_be<u32>(p);
    return (be << 16) | (be >> 16);
}

bool reads_constant(const u32 d[4]) {
    return (d[1] & 3) == REG_CONST || (d[2] & 3) == REG_CONST || (d[3] & 3) == REG_CONST;
}
}  // namespace

const char* op_name(u32 op) { return op < 0x46 ? kOpNames[op] : "OP?"; }
const char* input_name(u32 attr) { return kInputs[attr & 15]; }

int source_count(u32 op) {
    switch (op) {
    case NOP: case KIL: case STR: case SFL: case FENCT: case FENCB:
    case BRK: case CAL: case IFE: case LOOP: case REP: case RET:
        return 0;
    case MAD: case LRP: case DP2A: case TXD:
        return 3;
    case MUL: case ADD: case DP3: case DP4: case DST: case MIN: case MAX: case SLT: case SGE: case SLE:
    case SGT: case SNE: case SEQ: case POW: case DIV: case DIVSQ: case DP2: case REFL: case BEM:
    case TXL: case TXB:
        return 2;
    default:
        return 1;
    }
}

bool Insn::is_texture() const {
    switch (opcode()) {
    case TEX: case TXP: case TXD: case TXL: case TXB: case TEXBEM: case TXPBEM: return true;
    default: return false;
    }
}

Source Insn::src(int n) const {
    const u32 w = d[1 + n];
    Source s;
    s.type = w & 3;
    s.index = s.type == REG_INPUT ? input_attr() : (w >> 2) & 0x3F;
    s.half = (w >> 8) & 1;
    for (int c = 0; c < 4; ++c) s.swizzle[c] = (w >> (9 + 2 * c)) & 3;
    s.neg = (w >> 17) & 1;
    s.abs = n == 0 ? (w >> 29) & 1 : (w >> 18) & 1;
    return s;
}

Program parse(const u8* data, u32 limit) {
    Program p;
    u32 off = 0;
    // Skip leading NOPs (a program that is all NOPs still ends at the END bit).
    while (off + 16 <= limit) {
        const u32 d0 = read_word(data + off);
        if (((d0 >> 24) & 0x3F) != 0 || (d0 & 1)) break;
        off += 16;
    }
    p.start = off;
    while (true) {
        if (off + 16 > limit) {
            p.truncated = true;
            break;
        }
        Insn i;
        i.offset = off - p.start;
        for (int k = 0; k < 4; ++k) i.d[k] = read_word(data + off + 4 * k);
        off += 16;
        if (reads_constant(i.d)) {
            if (off + 16 > limit) {
                p.truncated = true;
                break;
            }
            i.has_constant = true;
            for (int k = 0; k < 4; ++k) i.constant[k] = read_word(data + off + 4 * k);
            off += 16;
        }
        const u32 op = i.opcode();
        if (i.is_texture()) p.textures |= 1u << i.tex_num();
        if (op == KIL) p.has_kill = true;
        if (i.is_flow()) p.has_branches = true;
        for (int n = 0; n < source_count(op); ++n)
            if (i.src(n).type == REG_INPUT) p.inputs |= 1u << i.src(n).index;
        if (op != NOP && !i.no_dest() && !i.is_flow() && op != KIL && !i.never_executes()) {
            if (i.dest_half()) p.halves_written |= 1u << (i.dest_reg() & 31);
            else {
                p.temps_written |= 1u << (i.dest_reg() & 31);
                if (i.dest_reg() == 1 && (i.write_mask() & 4)) p.writes_depth = true;
            }
        }
        const bool end = i.end();
        p.insns.push_back(i);
        if (end) break;
    }
    p.size = off - p.start;
    return p;
}

namespace {
std::string swz(const u8 s[4]) {
    if (s[0] == 0 && s[1] == 1 && s[2] == 2 && s[3] == 3) return "";
    std::string r = ".";
    if (s[0] == s[1] && s[1] == s[2] && s[2] == s[3]) return r + "xyzw"[s[0]];
    for (int c = 0; c < 4; ++c) r += "xyzw"[s[c]];
    return r;
}

float as_float(u32 bits) {
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

std::string src_str(const Insn& i, int n) {
    const Source s = i.src(n);
    std::string r;
    char buf[96];
    switch (s.type) {
    case REG_TEMP: std::snprintf(buf, sizeof buf, "%c%u", s.half ? 'H' : 'R', s.index); r = buf; break;
    case REG_INPUT: r = std::string("f[") + input_name(s.index) + "]"; break;
    case REG_CONST:
        std::snprintf(buf, sizeof buf, "{%g, %g, %g, %g}", as_float(i.constant[0]), as_float(i.constant[1]),
                      as_float(i.constant[2]), as_float(i.constant[3]));
        r = buf;
        break;
    default: r = "?"; break;
    }
    r += swz(s.swizzle);
    if (s.abs) r = "|" + r + "|";
    if (s.neg) r = "-" + r;
    return r;
}

std::string cond_str(const Insn& i) {
    if (i.always_executes()) return "";
    static const char* const kCond[8] = {"FL", "LT", "EQ", "LE", "GT", "NE", "GE", "TR"};
    const u32 c = (i.exec_lt() ? 1 : 0) | (i.exec_eq() ? 2 : 0) | (i.exec_gt() ? 4 : 0);
    std::string r = std::string(" (") + kCond[c] + std::to_string(i.cond_reg());
    const u8 sw[4] = {static_cast<u8>(i.cond_swizzle(0)), static_cast<u8>(i.cond_swizzle(1)),
                      static_cast<u8>(i.cond_swizzle(2)), static_cast<u8>(i.cond_swizzle(3))};
    return r + swz(sw) + ")";
}
}  // namespace

std::string disasm(const Insn& i) {
    const u32 op = i.opcode();
    std::string r = op_name(op);
    char buf[64];
    switch (op) {
    case IFE:
        std::snprintf(buf, sizeof buf, " else=%u end=%u", i.else_offset(), i.end_offset());
        return r + buf + cond_str(i) + (i.end() ? "  END" : "");
    case LOOP: case REP:
        std::snprintf(buf, sizeof buf, " count=%u init=%u inc=%d end=%u", i.rep_count(), i.loop_init(),
                      static_cast<int>(static_cast<s8>(i.loop_increment())), i.end_offset());
        return r + buf + cond_str(i) + (i.end() ? "  END" : "");
    case BRK: case RET: case CAL: case FENCT: case FENCB: case NOP:
        return r + cond_str(i) + (i.end() ? "  END" : "");
    default:
        break;
    }
    static const char* const kPrec[4] = {"R", "H", "X", "B"};
    if (op != KIL) r += kPrec[i.precision()];
    if (i.saturate()) r += "_sat";
    if (i.set_cond()) r += "C" + std::to_string(i.cond_mod_reg());
    static const char* const kScale[8] = {"", "_x2", "_x4", "_x8", "", "_d2", "_d4", "_d8"};
    r += kScale[i.scale()];
    if (i.is_texture() && i.exp_tex()) r += "_bx2";
    r += " ";
    if (op != KIL) {
        if (i.no_dest()) {
            r += "RC";
        } else {
            std::snprintf(buf, sizeof buf, "%c%u", i.dest_half() ? 'H' : 'R', i.dest_reg());
            r += buf;
        }
        const u32 m = i.write_mask();
        if (m != 0xF) {
            r += '.';
            for (int c = 0; c < 4; ++c)
                if (m & (1u << c)) r += "xyzw"[c];
        }
    }
    const int n = source_count(op);
    for (int k = 0; k < n; ++k) r += (op == KIL && k == 0 ? "" : ", ") + src_str(i, k);
    if (i.is_texture()) r += ", TEX" + std::to_string(i.tex_num());
    r += cond_str(i);
    if (i.end()) r += "  END";
    return r;
}

}  // namespace opentit::rsx::fp
