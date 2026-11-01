#include "spu/spu_decoder.h"

#include <array>
#include <cctype>
#include <cstdio>

namespace opentit::spu {

namespace {

struct Info {
    const char* name;
    Form form;
};

constexpr Info kInfo[] = {
    {"INVALID", Form::NONE},
#define OP(name, bits, opc, form) {#name, Form::form},
#include "spu/spu_ops.inc"
#undef OP
};

struct Table {
    std::array<Op, 2048> t{};
    Table() {
        t.fill(Op::INVALID);
        struct E { Op op; int bits; u32 opc; };
        const E entries[] = {
#define OP(name, bits, opc, form) {Op::name, bits, opc},
#include "spu/spu_ops.inc"
#undef OP
        };
        // Shorter opcodes first so the (disjoint) longer ones are never overwritten wrongly.
        for (int bits : {4, 7, 8, 9, 10, 11})
            for (const auto& e : entries) {
                if (e.bits != bits) continue;
                const u32 lo = e.opc << (11 - bits), hi = (e.opc + 1) << (11 - bits);
                for (u32 i = lo; i < hi; ++i) t[i] = e.op;
            }
    }
};

const Table& table() {
    static const Table t;
    return t;
}

}  // namespace

const char* op_name(Op op) { return kInfo[static_cast<std::size_t>(op)].name; }
Form op_form(Op op) { return kInfo[static_cast<std::size_t>(op)].form; }

Op decode(u32 raw) { return table().t[raw >> 21]; }

bool is_branch(Op op) {
    switch (op) {
    case Op::BR: case Op::BRA: case Op::BRSL: case Op::BRASL: case Op::BRZ: case Op::BRNZ: case Op::BRHZ:
    case Op::BRHNZ: case Op::BI: case Op::BISL: case Op::BIZ: case Op::BINZ: case Op::BIHZ: case Op::BIHNZ:
    case Op::IRET: case Op::BISLED:
        return true;
    default:
        return false;
    }
}

std::string disasm(u32 raw, u32 pc) {
    const Op op = decode(raw);
    if (op == Op::INVALID) {
        char b[24];
        std::snprintf(b, sizeof(b), ".long 0x%08X", raw);
        return b;
    }
    std::string m = op_name(op);
    for (auto& c : m) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const Insn i(raw);
    char b[96];
    switch (op_form(op)) {
    case Form::RRR:
        std::snprintf(b, sizeof(b), "%s $%u, $%u, $%u, $%u", m.c_str(), i.rrr_rt(), i.ra(), i.rb(), i.rc());
        break;
    case Form::RI7:
        std::snprintf(b, sizeof(b), "%s $%u, $%u, %d", m.c_str(), i.rt(), i.ra(), i.i7());
        break;
    case Form::RI8:
        std::snprintf(b, sizeof(b), "%s $%u, $%u, %u", m.c_str(), i.rt(), i.ra(), i.i8());
        break;
    case Form::RI10:
        if (op == Op::LQD || op == Op::STQD)
            std::snprintf(b, sizeof(b), "%s $%u, %d($%u)", m.c_str(), i.rt(), i.i10() * 16, i.ra());
        else
            std::snprintf(b, sizeof(b), "%s $%u, $%u, %d", m.c_str(), i.rt(), i.ra(), i.i10());
        break;
    case Form::RI16:
        if (op == Op::BR || op == Op::BRSL || op == Op::BRZ || op == Op::BRNZ || op == Op::BRHZ || op == Op::BRHNZ ||
            op == Op::LQR || op == Op::STQR)
            std::snprintf(b, sizeof(b), "%s $%u, 0x%05X", m.c_str(), i.rt(), (pc + (i.i16() << 2)) & 0x3FFFC);
        else if (op == Op::BRA || op == Op::BRASL || op == Op::LQA || op == Op::STQA)
            std::snprintf(b, sizeof(b), "%s $%u, 0x%05X", m.c_str(), i.rt(), (i.i16() << 2) & 0x3FFFC);
        else
            std::snprintf(b, sizeof(b), "%s $%u, 0x%04X", m.c_str(), i.rt(), i.u16_());
        break;
    case Form::RI18:
        std::snprintf(b, sizeof(b), "%s $%u, 0x%05X", m.c_str(), i.rt(), i.i18());
        break;
    default:
        if (op == Op::STOP || op == Op::STOPD) std::snprintf(b, sizeof(b), "%s 0x%X", m.c_str(), raw & 0x3FFF);
        else if (op == Op::LNOP || op == Op::NOP || op == Op::SYNC || op == Op::DSYNC) std::snprintf(b, sizeof(b), "%s", m.c_str());
        else if (op == Op::RDCH || op == Op::RCHCNT) std::snprintf(b, sizeof(b), "%s $%u, $ch%u", m.c_str(), i.rt(), i.ra());
        else if (op == Op::WRCH) std::snprintf(b, sizeof(b), "wrch $ch%u, $%u", i.ra(), i.rt());
        else if (op == Op::BI || op == Op::IRET) std::snprintf(b, sizeof(b), "%s $%u", m.c_str(), i.ra());
        else std::snprintf(b, sizeof(b), "%s $%u, $%u, $%u", m.c_str(), i.rt(), i.ra(), i.rb());
        break;
    }
    return b;
}

}  // namespace opentit::spu
