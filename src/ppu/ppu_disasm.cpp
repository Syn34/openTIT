#include "ppu/ppu_disasm.h"

#include <cctype>
#include <cstdio>

#include "ppu/ppu_decoder.h"

namespace opentit::ppu {

namespace {

std::string lower(const char* s) {
    std::string r;
    for (; *s; ++s) r.push_back(*s == '_' ? '.' : static_cast<char>(std::tolower(static_cast<unsigned char>(*s))));
    return r;
}

std::string fmt(const char* f, auto... args) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), f, args...);
    return buf;
}

}  // namespace

std::string disasm(u32 raw, u32 pc) {
    const Insn i(raw);
    const Op op = decode(raw);
    std::string m = lower(op_name(op));
    if (op == Op::INVALID) return fmt(".long 0x%08X", raw);

    const auto rc = [&] { return i.rc() ? std::string(".") : std::string(); };

    switch (op) {
    case Op::B: {
        const u32 t = i.aa() ? static_cast<u32>(i.li()) : pc + static_cast<u32>(i.li());
        return fmt("%s 0x%08X", i.lk() ? "bl" : "b", t);
    }
    case Op::BC: {
        const u32 t = i.aa() ? static_cast<u32>(i.bd()) : pc + static_cast<u32>(i.bd());
        return fmt("bc%s %u, %u, 0x%08X", i.lk() ? "l" : "", i.bo(), i.bi(), t);
    }
    case Op::BCLR:
        if (i.bo() == 20) return i.lk() ? "blrl" : "blr";
        return fmt("bclr%s %u, %u", i.lk() ? "l" : "", i.bo(), i.bi());
    case Op::BCCTR:
        if (i.bo() == 20) return i.lk() ? "bctrl" : "bctr";
        return fmt("bcctr%s %u, %u", i.lk() ? "l" : "", i.bo(), i.bi());
    case Op::SC: return "sc";

    case Op::ADDI:
        if (i.ra() == 0) return fmt("li r%u, %d", i.rd(), i.simm16());
        return fmt("addi r%u, r%u, %d", i.rd(), i.ra(), i.simm16());
    case Op::ADDIS:
        if (i.ra() == 0) return fmt("lis r%u, 0x%X", i.rd(), i.uimm16());
        return fmt("addis r%u, r%u, 0x%X", i.rd(), i.ra(), i.uimm16());
    case Op::MULLI: case Op::SUBFIC: case Op::ADDIC: case Op::ADDIC_:
        return fmt("%s r%u, r%u, %d", m.c_str(), i.rd(), i.ra(), i.simm16());
    case Op::ORI:
        if (raw == 0x60000000) return "nop";
        [[fallthrough]];
    case Op::ORIS: case Op::XORI: case Op::XORIS: case Op::ANDI_: case Op::ANDIS_:
        return fmt("%s r%u, r%u, 0x%X", m.c_str(), i.ra(), i.rs(), i.uimm16());
    case Op::CMPI:
        return fmt("cmp%si cr%u, r%u, %d", i.l10() ? "d" : "w", i.crfd(), i.ra(), i.simm16());
    case Op::CMPLI:
        return fmt("cmpl%si cr%u, r%u, 0x%X", i.l10() ? "d" : "w", i.crfd(), i.ra(), i.uimm16());
    case Op::CMP:
        return fmt("cmp%s cr%u, r%u, r%u", i.l10() ? "d" : "w", i.crfd(), i.ra(), i.rb());
    case Op::CMPL:
        return fmt("cmpl%s cr%u, r%u, r%u", i.l10() ? "d" : "w", i.crfd(), i.ra(), i.rb());
    case Op::TDI: case Op::TWI:
        return fmt("%s %u, r%u, %d", m.c_str(), i.rd(), i.ra(), i.simm16());

    case Op::RLWINM: case Op::RLWIMI:
        return fmt("%s%s r%u, r%u, %u, %u, %u", m.c_str(), rc().c_str(), i.ra(), i.rs(), i.sh32(), i.mb32(), i.me32());
    case Op::RLWNM:
        return fmt("%s%s r%u, r%u, r%u, %u, %u", m.c_str(), rc().c_str(), i.ra(), i.rs(), i.rb(), i.mb32(), i.me32());
    case Op::RLDICL: case Op::RLDICR: case Op::RLDIC: case Op::RLDIMI:
        return fmt("%s%s r%u, r%u, %u, %u", m.c_str(), rc().c_str(), i.ra(), i.rs(), i.sh64(), i.mbe64());
    case Op::RLDCL: case Op::RLDCR:
        return fmt("%s%s r%u, r%u, r%u, %u", m.c_str(), rc().c_str(), i.ra(), i.rs(), i.rb(), i.mbe64());
    case Op::SRAWI:
        return fmt("srawi%s r%u, r%u, %u", rc().c_str(), i.ra(), i.rs(), i.sh32());
    case Op::SRADI:
        return fmt("sradi%s r%u, r%u, %u", rc().c_str(), i.ra(), i.rs(), i.sh64());

    // D-form integer loads / stores
    case Op::LBZ: case Op::LBZU: case Op::LHZ: case Op::LHZU: case Op::LHA: case Op::LHAU:
    case Op::LWZ: case Op::LWZU: case Op::STB: case Op::STBU: case Op::STH: case Op::STHU:
    case Op::STW: case Op::STWU: case Op::LMW: case Op::STMW:
        return fmt("%s r%u, %d(r%u)", m.c_str(), i.rd(), i.simm16(), i.ra());
    case Op::LD: case Op::LDU: case Op::LWA: case Op::STD: case Op::STDU:
        return fmt("%s r%u, %d(r%u)", m.c_str(), i.rd(), i.ds(), i.ra());
    case Op::LFS: case Op::LFSU: case Op::LFD: case Op::LFDU:
    case Op::STFS: case Op::STFSU: case Op::STFD: case Op::STFDU:
        return fmt("%s f%u, %d(r%u)", m.c_str(), i.rd(), i.simm16(), i.ra());
    case Op::LSWI: case Op::STSWI:
        return fmt("%s r%u, r%u, %u", m.c_str(), i.rd(), i.ra(), i.rb());

    case Op::LFSX: case Op::LFSUX: case Op::LFDX: case Op::LFDUX:
    case Op::STFSX: case Op::STFSUX: case Op::STFDX: case Op::STFDUX: case Op::STFIWX:
        return fmt("%s f%u, r%u, r%u", m.c_str(), i.rd(), i.ra(), i.rb());
    case Op::LVSL: case Op::LVSR: case Op::LVEBX: case Op::LVEHX: case Op::LVEWX: case Op::LVX:
    case Op::LVXL: case Op::LVLX: case Op::LVRX: case Op::LVLXL: case Op::LVRXL:
    case Op::STVEBX: case Op::STVEHX: case Op::STVEWX: case Op::STVX: case Op::STVXL:
    case Op::STVLX: case Op::STVRX: case Op::STVLXL: case Op::STVRXL:
        return fmt("%s v%u, r%u, r%u", m.c_str(), i.rd(), i.ra(), i.rb());

    case Op::MFSPR: {
        const u32 s = i.spr();
        if (s == 8) return fmt("mflr r%u", i.rd());
        if (s == 9) return fmt("mfctr r%u", i.rd());
        if (s == 1) return fmt("mfxer r%u", i.rd());
        return fmt("mfspr r%u, %u", i.rd(), s);
    }
    case Op::MTSPR: {
        const u32 s = i.spr();
        if (s == 8) return fmt("mtlr r%u", i.rs());
        if (s == 9) return fmt("mtctr r%u", i.rs());
        if (s == 1) return fmt("mtxer r%u", i.rs());
        return fmt("mtspr %u, r%u", s, i.rs());
    }
    case Op::MFTB: return fmt("mftb r%u, %u", i.rd(), i.spr());
    case Op::MFCR: return fmt("mfcr r%u", i.rd());
    case Op::MFOCRF: return fmt("mfocrf r%u, 0x%02X", i.rd(), i.crm());
    case Op::MTCRF: return fmt("mtcrf 0x%02X, r%u", i.crm(), i.rs());
    case Op::MTOCRF: return fmt("mtocrf 0x%02X, r%u", i.crm(), i.rs());
    case Op::MCRF: return fmt("mcrf cr%u, cr%u", i.crfd(), i.crfs());
    case Op::CRNOR: case Op::CRANDC: case Op::CRXOR: case Op::CRNAND:
    case Op::CRAND: case Op::CREQV: case Op::CRORC: case Op::CROR:
        return fmt("%s %u, %u, %u", m.c_str(), i.crbd(), i.crba(), i.crbb());
    case Op::SYNC: return (i.bits(9, 10) == 1) ? "lwsync" : "sync";
    case Op::ISYNC: case Op::EIEIO: return m;
    case Op::DCBF: case Op::DCBST: case Op::DCBT: case Op::DCBTST: case Op::DCBZ:
    case Op::DCBI: case Op::ICBI:
        return fmt("%s r%u, r%u", m.c_str(), i.ra(), i.rb());

    // Floating point
    case Op::FMR: case Op::FNEG: case Op::FABS: case Op::FNABS: case Op::FRSP: case Op::FCTIW:
    case Op::FCTIWZ: case Op::FCTID: case Op::FCTIDZ: case Op::FCFID: case Op::FSQRT: case Op::FSQRTS:
    case Op::FRES: case Op::FRSQRTE:
        return fmt("%s%s f%u, f%u", m.c_str(), rc().c_str(), i.rd(), i.rb());
    case Op::FADD: case Op::FADDS: case Op::FSUB: case Op::FSUBS: case Op::FDIV: case Op::FDIVS:
        return fmt("%s%s f%u, f%u, f%u", m.c_str(), rc().c_str(), i.rd(), i.ra(), i.rb());
    case Op::FMUL: case Op::FMULS:
        return fmt("%s%s f%u, f%u, f%u", m.c_str(), rc().c_str(), i.rd(), i.ra(), i.rc_reg());
    case Op::FMADD: case Op::FMADDS: case Op::FMSUB: case Op::FMSUBS: case Op::FNMADD:
    case Op::FNMADDS: case Op::FNMSUB: case Op::FNMSUBS: case Op::FSEL:
        return fmt("%s%s f%u, f%u, f%u, f%u", m.c_str(), rc().c_str(), i.rd(), i.ra(), i.rc_reg(), i.rb());
    case Op::FCMPU: case Op::FCMPO:
        return fmt("%s cr%u, f%u, f%u", m.c_str(), i.crfd(), i.ra(), i.rb());
    case Op::MFFS: return fmt("mffs%s f%u", rc().c_str(), i.rd());
    case Op::MTFSF: return fmt("mtfsf%s 0x%02X, f%u", rc().c_str(), i.flm(), i.rb());

    // VMX
    case Op::VSPLTISB: case Op::VSPLTISH: case Op::VSPLTISW:
        return fmt("%s v%u, %d", m.c_str(), i.rd(), i.vsimm());
    case Op::VSPLTB: case Op::VSPLTH: case Op::VSPLTW: case Op::VCFUX: case Op::VCFSX:
    case Op::VCTUXS: case Op::VCTSXS:
        return fmt("%s v%u, v%u, %u", m.c_str(), i.rd(), i.rb(), i.vuimm());
    case Op::VSLDOI:
        return fmt("vsldoi v%u, v%u, v%u, %u", i.rd(), i.ra(), i.rb(), i.vsh());
    case Op::VMHADDSHS: case Op::VMHRADDSHS: case Op::VMLADDUHM: case Op::VMSUMUBM: case Op::VMSUMMBM:
    case Op::VMSUMUHM: case Op::VMSUMUHS: case Op::VMSUMSHM: case Op::VMSUMSHS: case Op::VSEL: case Op::VPERM:
        return fmt("%s v%u, v%u, v%u, v%u", m.c_str(), i.rd(), i.ra(), i.rb(), i.rc_reg());
    case Op::VMADDFP: case Op::VNMSUBFP:
        return fmt("%s v%u, v%u, v%u, v%u", m.c_str(), i.rd(), i.ra(), i.rc_reg(), i.rb());
    case Op::MFVSCR: return fmt("mfvscr v%u", i.rd());
    case Op::MTVSCR: return fmt("mtvscr v%u", i.rb());
    case Op::VREFP: case Op::VRSQRTEFP: case Op::VEXPTEFP: case Op::VLOGEFP: case Op::VRFIN:
    case Op::VRFIZ: case Op::VRFIP: case Op::VRFIM: case Op::VUPKHSB: case Op::VUPKHSH:
    case Op::VUPKLSB: case Op::VUPKLSH: case Op::VUPKHPX: case Op::VUPKLPX:
        return fmt("%s v%u, v%u", m.c_str(), i.rd(), i.rb());
    default: break;
    }

    if (op >= Op::VCMPBFP && op <= Op::VCMPGTUW)
        return fmt("%s%s v%u, v%u, v%u", m.c_str(), i.vrc() ? "." : "", i.rd(), i.ra(), i.rb());
    if (op >= Op::VMHADDSHS)
        return fmt("%s v%u, v%u, v%u", m.c_str(), i.rd(), i.ra(), i.rb());

    // Remaining X / XO-form integer ops.
    switch (op) {
    case Op::NEG: case Op::ADDZE: case Op::SUBFZE: case Op::ADDME: case Op::SUBFME:
        return fmt("%s%s%s r%u, r%u", m.c_str(), i.oe() ? "o" : "", rc().c_str(), i.rd(), i.ra());
    case Op::SUBFC: case Op::ADDC: case Op::SUBF: case Op::SUBFE: case Op::ADDE: case Op::ADD:
    case Op::MULLD: case Op::MULLW: case Op::DIVDU: case Op::DIVWU: case Op::DIVD: case Op::DIVW:
        return fmt("%s%s%s r%u, r%u, r%u", m.c_str(), i.oe() ? "o" : "", rc().c_str(), i.rd(), i.ra(), i.rb());
    case Op::MULHDU: case Op::MULHWU: case Op::MULHD: case Op::MULHW:
        return fmt("%s%s r%u, r%u, r%u", m.c_str(), rc().c_str(), i.rd(), i.ra(), i.rb());
    case Op::EXTSB: case Op::EXTSH: case Op::EXTSW: case Op::CNTLZW: case Op::CNTLZD:
        return fmt("%s%s r%u, r%u", m.c_str(), rc().c_str(), i.ra(), i.rs());
    case Op::OR:
        if (i.rs() == i.rb()) return fmt("mr%s r%u, r%u", rc().c_str(), i.ra(), i.rs());
        [[fallthrough]];
    case Op::AND: case Op::ANDC: case Op::NOR: case Op::EQV: case Op::XOR: case Op::ORC: case Op::NAND:
    case Op::SLW: case Op::SRW: case Op::SRAW: case Op::SLD: case Op::SRD: case Op::SRAD:
        return fmt("%s%s r%u, r%u, r%u", m.c_str(), rc().c_str(), i.ra(), i.rs(), i.rb());
    case Op::STWCX: case Op::STDCX:
        return fmt("%s. r%u, r%u, r%u", m.c_str(), i.rs(), i.ra(), i.rb());
    default:
        return fmt("%s r%u, r%u, r%u", m.c_str(), i.rd(), i.ra(), i.rb());
    }
}

}  // namespace opentit::ppu
