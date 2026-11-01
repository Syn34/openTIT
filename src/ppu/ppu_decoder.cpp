#include "ppu/ppu_decoder.h"

#include <array>

namespace opentit::ppu {

namespace {

constexpr const char* kNames[] = {
#define OP(name) #name,
#include "ppu/ppu_ops.inc"
#undef OP
};

Op decode_19(const Insn i) {
    switch (i.bits(21, 30)) {
    case 0: return Op::MCRF;
    case 16: return Op::BCLR;
    case 33: return Op::CRNOR;
    case 129: return Op::CRANDC;
    case 150: return Op::ISYNC;
    case 193: return Op::CRXOR;
    case 225: return Op::CRNAND;
    case 257: return Op::CRAND;
    case 289: return Op::CREQV;
    case 417: return Op::CRORC;
    case 449: return Op::CROR;
    case 528: return Op::BCCTR;
    default: return Op::INVALID;
    }
}

Op decode_30(const Insn i) {
    switch (i.bits(27, 29)) {
    case 0: return Op::RLDICL;
    case 1: return Op::RLDICR;
    case 2: return Op::RLDIC;
    case 3: return Op::RLDIMI;
    default: break;
    }
    switch (i.bits(27, 30)) {
    case 8: return Op::RLDCL;
    case 9: return Op::RLDCR;
    default: return Op::INVALID;
    }
}

Op decode_31(const Insn i) {
    // XS-form sradi uses a 9-bit extended opcode with sh5 in bit 30.
    if (i.bits(21, 29) == 413) return Op::SRADI;

    switch (i.bits(21, 30)) {
    case 0: return Op::CMP;
    case 4: return Op::TW;
    case 6: return Op::LVSL;
    case 7: return Op::LVEBX;
    case 19: return i.crm_one() ? Op::MFOCRF : Op::MFCR;
    case 20: return Op::LWARX;
    case 21: return Op::LDX;
    case 23: return Op::LWZX;
    case 24: return Op::SLW;
    case 26: return Op::CNTLZW;
    case 27: return Op::SLD;
    case 28: return Op::AND;
    case 32: return Op::CMPL;
    case 38: return Op::LVSR;
    case 39: return Op::LVEHX;
    case 53: return Op::LDUX;
    case 54: return Op::DCBST;
    case 55: return Op::LWZUX;
    case 58: return Op::CNTLZD;
    case 60: return Op::ANDC;
    case 68: return Op::TD;
    case 71: return Op::LVEWX;
    case 83: return Op::MFMSR;
    case 84: return Op::LDARX;
    case 86: return Op::DCBF;
    case 87: return Op::LBZX;
    case 103: return Op::LVX;
    case 119: return Op::LBZUX;
    case 124: return Op::NOR;
    case 135: return Op::STVEBX;
    case 144: return i.crm_one() ? Op::MTOCRF : Op::MTCRF;
    case 146: return Op::MTMSR;
    case 149: return Op::STDX;
    case 150: return Op::STWCX;
    case 151: return Op::STWX;
    case 167: return Op::STVEHX;
    case 178: return Op::MTMSRD;
    case 181: return Op::STDUX;
    case 183: return Op::STWUX;
    case 199: return Op::STVEWX;
    case 214: return Op::STDCX;
    case 215: return Op::STBX;
    case 231: return Op::STVX;
    case 246: return Op::DCBTST;
    case 247: return Op::STBUX;
    case 278: return Op::DCBT;
    case 279: return Op::LHZX;
    case 284: return Op::EQV;
    case 310: return Op::ECIWX;
    case 311: return Op::LHZUX;
    case 316: return Op::XOR;
    case 339: return Op::MFSPR;
    case 341: return Op::LWAX;
    case 343: return Op::LHAX;
    case 359: return Op::LVXL;
    case 371: return Op::MFTB;
    case 373: return Op::LWAUX;
    case 375: return Op::LHAUX;
    case 407: return Op::STHX;
    case 412: return Op::ORC;
    case 438: return Op::ECOWX;
    case 439: return Op::STHUX;
    case 444: return Op::OR;
    case 467: return Op::MTSPR;
    case 470: return Op::DCBI;
    case 476: return Op::NAND;
    case 487: return Op::STVXL;
    case 519: return Op::LVLX;
    case 533: return Op::LSWX;
    case 534: return Op::LWBRX;
    case 535: return Op::LFSX;
    case 536: return Op::SRW;
    case 539: return Op::SRD;
    case 551: return Op::LVRX;
    case 567: return Op::LFSUX;
    case 597: return Op::LSWI;
    case 598: return Op::SYNC;
    case 599: return Op::LFDX;
    case 631: return Op::LFDUX;
    case 647: return Op::STVLX;
    case 661: return Op::STSWX;
    case 662: return Op::STWBRX;
    case 663: return Op::STFSX;
    case 679: return Op::STVRX;
    case 695: return Op::STFSUX;
    case 725: return Op::STSWI;
    case 727: return Op::STFDX;
    case 759: return Op::STFDUX;
    case 775: return Op::LVLXL;
    case 790: return Op::LHBRX;
    case 792: return Op::SRAW;
    case 794: return Op::SRAD;
    case 807: return Op::LVRXL;
    case 824: return Op::SRAWI;
    case 854: return Op::EIEIO;
    case 903: return Op::STVLXL;
    case 918: return Op::STHBRX;
    case 922: return Op::EXTSH;
    case 935: return Op::STVRXL;
    case 954: return Op::EXTSB;
    case 982: return Op::ICBI;
    case 983: return Op::STFIWX;
    case 986: return Op::EXTSW;
    case 1014: return Op::DCBZ;
    default: break;
    }

    // XO-form arithmetic: 9-bit extended opcode, OE in bit 21.
    switch (i.bits(22, 30)) {
    case 8: return Op::SUBFC;
    case 9: return Op::MULHDU;
    case 10: return Op::ADDC;
    case 11: return Op::MULHWU;
    case 40: return Op::SUBF;
    case 73: return Op::MULHD;
    case 75: return Op::MULHW;
    case 104: return Op::NEG;
    case 136: return Op::SUBFE;
    case 138: return Op::ADDE;
    case 200: return Op::SUBFZE;
    case 202: return Op::ADDZE;
    case 232: return Op::SUBFME;
    case 233: return Op::MULLD;
    case 234: return Op::ADDME;
    case 235: return Op::MULLW;
    case 266: return Op::ADD;
    case 457: return Op::DIVDU;
    case 459: return Op::DIVWU;
    case 489: return Op::DIVD;
    case 491: return Op::DIVW;
    default: return Op::INVALID;
    }
}

Op decode_58(const Insn i) {
    switch (i.raw & 3) {
    case 0: return Op::LD;
    case 1: return Op::LDU;
    case 2: return Op::LWA;
    default: return Op::INVALID;
    }
}

Op decode_62(const Insn i) {
    switch (i.raw & 3) {
    case 0: return Op::STD;
    case 1: return Op::STDU;
    default: return Op::INVALID;
    }
}

Op decode_59(const Insn i) {
    switch (i.bits(26, 30)) {
    case 18: return Op::FDIVS;
    case 20: return Op::FSUBS;
    case 21: return Op::FADDS;
    case 22: return Op::FSQRTS;
    case 24: return Op::FRES;
    case 25: return Op::FMULS;
    case 28: return Op::FMSUBS;
    case 29: return Op::FMADDS;
    case 30: return Op::FNMSUBS;
    case 31: return Op::FNMADDS;
    default: return Op::INVALID;
    }
}

Op decode_63(const Insn i) {
    const u32 xo5 = i.bits(26, 30);
    if (xo5 >= 16) {
        switch (xo5) {
        case 18: return Op::FDIV;
        case 20: return Op::FSUB;
        case 21: return Op::FADD;
        case 22: return Op::FSQRT;
        case 23: return Op::FSEL;
        case 25: return Op::FMUL;
        case 26: return Op::FRSQRTE;
        case 28: return Op::FMSUB;
        case 29: return Op::FMADD;
        case 30: return Op::FNMSUB;
        case 31: return Op::FNMADD;
        default: return Op::INVALID;
        }
    }
    switch (i.bits(21, 30)) {
    case 0: return Op::FCMPU;
    case 12: return Op::FRSP;
    case 14: return Op::FCTIW;
    case 15: return Op::FCTIWZ;
    case 32: return Op::FCMPO;
    case 38: return Op::MTFSB1;
    case 40: return Op::FNEG;
    case 64: return Op::MCRFS;
    case 70: return Op::MTFSB0;
    case 72: return Op::FMR;
    case 134: return Op::MTFSFI;
    case 136: return Op::FNABS;
    case 264: return Op::FABS;
    case 583: return Op::MFFS;
    case 711: return Op::MTFSF;
    case 814: return Op::FCTID;
    case 815: return Op::FCTIDZ;
    case 846: return Op::FCFID;
    default: return Op::INVALID;
    }
}

Op decode_4(const Insn i) {
    switch (i.bits(26, 31)) {
    case 32: return Op::VMHADDSHS;
    case 33: return Op::VMHRADDSHS;
    case 34: return Op::VMLADDUHM;
    case 36: return Op::VMSUMUBM;
    case 37: return Op::VMSUMMBM;
    case 38: return Op::VMSUMUHM;
    case 39: return Op::VMSUMUHS;
    case 40: return Op::VMSUMSHM;
    case 41: return Op::VMSUMSHS;
    case 42: return Op::VSEL;
    case 43: return Op::VPERM;
    case 44: return Op::VSLDOI;
    case 46: return Op::VMADDFP;
    case 47: return Op::VNMSUBFP;
    default: break;
    }
    switch (i.bits(22, 31)) {
    case 966: return Op::VCMPBFP;
    case 198: return Op::VCMPEQFP;
    case 6: return Op::VCMPEQUB;
    case 70: return Op::VCMPEQUH;
    case 134: return Op::VCMPEQUW;
    case 454: return Op::VCMPGEFP;
    case 710: return Op::VCMPGTFP;
    case 774: return Op::VCMPGTSB;
    case 838: return Op::VCMPGTSH;
    case 902: return Op::VCMPGTSW;
    case 518: return Op::VCMPGTUB;
    case 582: return Op::VCMPGTUH;
    case 646: return Op::VCMPGTUW;
    default: break;
    }
    switch (i.bits(21, 31)) {
    case 0: return Op::VADDUBM;
    case 64: return Op::VADDUHM;
    case 128: return Op::VADDUWM;
    case 384: return Op::VADDCUW;
    case 512: return Op::VADDUBS;
    case 576: return Op::VADDUHS;
    case 640: return Op::VADDUWS;
    case 768: return Op::VADDSBS;
    case 832: return Op::VADDSHS;
    case 896: return Op::VADDSWS;
    case 10: return Op::VADDFP;
    case 74: return Op::VSUBFP;
    case 1024: return Op::VSUBUBM;
    case 1088: return Op::VSUBUHM;
    case 1152: return Op::VSUBUWM;
    case 1408: return Op::VSUBCUW;
    case 1536: return Op::VSUBUBS;
    case 1600: return Op::VSUBUHS;
    case 1664: return Op::VSUBUWS;
    case 1792: return Op::VSUBSBS;
    case 1856: return Op::VSUBSHS;
    case 1920: return Op::VSUBSWS;
    case 2: return Op::VMAXUB;
    case 66: return Op::VMAXUH;
    case 130: return Op::VMAXUW;
    case 258: return Op::VMAXSB;
    case 322: return Op::VMAXSH;
    case 386: return Op::VMAXSW;
    case 1034: return Op::VMAXFP;
    case 514: return Op::VMINUB;
    case 578: return Op::VMINUH;
    case 642: return Op::VMINUW;
    case 770: return Op::VMINSB;
    case 834: return Op::VMINSH;
    case 898: return Op::VMINSW;
    case 1098: return Op::VMINFP;
    case 1026: return Op::VAVGUB;
    case 1090: return Op::VAVGUH;
    case 1154: return Op::VAVGUW;
    case 1282: return Op::VAVGSB;
    case 1346: return Op::VAVGSH;
    case 1410: return Op::VAVGSW;
    case 4: return Op::VRLB;
    case 68: return Op::VRLH;
    case 132: return Op::VRLW;
    case 260: return Op::VSLB;
    case 324: return Op::VSLH;
    case 388: return Op::VSLW;
    case 452: return Op::VSL;
    case 516: return Op::VSRB;
    case 580: return Op::VSRH;
    case 644: return Op::VSRW;
    case 708: return Op::VSR;
    case 772: return Op::VSRAB;
    case 836: return Op::VSRAH;
    case 900: return Op::VSRAW;
    case 1028: return Op::VAND;
    case 1092: return Op::VANDC;
    case 1156: return Op::VOR;
    case 1284: return Op::VNOR;
    case 1220: return Op::VXOR;
    case 1540: return Op::MFVSCR;
    case 1604: return Op::MTVSCR;
    case 8: return Op::VMULOUB;
    case 72: return Op::VMULOUH;
    case 264: return Op::VMULOSB;
    case 328: return Op::VMULOSH;
    case 520: return Op::VMULEUB;
    case 584: return Op::VMULEUH;
    case 776: return Op::VMULESB;
    case 840: return Op::VMULESH;
    case 1544: return Op::VSUM4UBS;
    case 1800: return Op::VSUM4SBS;
    case 1608: return Op::VSUM4SHS;
    case 1672: return Op::VSUM2SWS;
    case 1928: return Op::VSUMSWS;
    case 266: return Op::VREFP;
    case 330: return Op::VRSQRTEFP;
    case 394: return Op::VEXPTEFP;
    case 458: return Op::VLOGEFP;
    case 522: return Op::VRFIN;
    case 586: return Op::VRFIZ;
    case 650: return Op::VRFIP;
    case 714: return Op::VRFIM;
    case 778: return Op::VCFUX;
    case 842: return Op::VCFSX;
    case 906: return Op::VCTUXS;
    case 970: return Op::VCTSXS;
    case 12: return Op::VMRGHB;
    case 76: return Op::VMRGHH;
    case 140: return Op::VMRGHW;
    case 268: return Op::VMRGLB;
    case 332: return Op::VMRGLH;
    case 396: return Op::VMRGLW;
    case 524: return Op::VSPLTB;
    case 588: return Op::VSPLTH;
    case 652: return Op::VSPLTW;
    case 780: return Op::VSPLTISB;
    case 844: return Op::VSPLTISH;
    case 908: return Op::VSPLTISW;
    case 1036: return Op::VSLO;
    case 1100: return Op::VSRO;
    case 14: return Op::VPKUHUM;
    case 78: return Op::VPKUWUM;
    case 142: return Op::VPKUHUS;
    case 206: return Op::VPKUWUS;
    case 270: return Op::VPKSHUS;
    case 334: return Op::VPKSWUS;
    case 398: return Op::VPKSHSS;
    case 462: return Op::VPKSWSS;
    case 782: return Op::VPKPX;
    case 526: return Op::VUPKHSB;
    case 590: return Op::VUPKHSH;
    case 654: return Op::VUPKLSB;
    case 718: return Op::VUPKLSH;
    case 846: return Op::VUPKHPX;
    case 974: return Op::VUPKLPX;
    default: return Op::INVALID;
    }
}

}  // namespace

const char* op_name(Op op) {
    const auto idx = static_cast<std::size_t>(op);
    return idx < static_cast<std::size_t>(Op::COUNT) ? kNames[idx] : "?";
}

Op decode(u32 raw) {
    const Insn i(raw);
    switch (i.opcd()) {
    case 2: return Op::TDI;
    case 3: return Op::TWI;
    case 4: return decode_4(i);
    case 7: return Op::MULLI;
    case 8: return Op::SUBFIC;
    case 10: return Op::CMPLI;
    case 11: return Op::CMPI;
    case 12: return Op::ADDIC;
    case 13: return Op::ADDIC_;
    case 14: return Op::ADDI;
    case 15: return Op::ADDIS;
    case 16: return Op::BC;
    case 17: return (raw & 2) ? Op::SC : Op::INVALID;
    case 18: return Op::B;
    case 19: return decode_19(i);
    case 20: return Op::RLWIMI;
    case 21: return Op::RLWINM;
    case 23: return Op::RLWNM;
    case 24: return Op::ORI;
    case 25: return Op::ORIS;
    case 26: return Op::XORI;
    case 27: return Op::XORIS;
    case 28: return Op::ANDI_;
    case 29: return Op::ANDIS_;
    case 30: return decode_30(i);
    case 31: return decode_31(i);
    case 32: return Op::LWZ;
    case 33: return Op::LWZU;
    case 34: return Op::LBZ;
    case 35: return Op::LBZU;
    case 36: return Op::STW;
    case 37: return Op::STWU;
    case 38: return Op::STB;
    case 39: return Op::STBU;
    case 40: return Op::LHZ;
    case 41: return Op::LHZU;
    case 42: return Op::LHA;
    case 43: return Op::LHAU;
    case 44: return Op::STH;
    case 45: return Op::STHU;
    case 46: return Op::LMW;
    case 47: return Op::STMW;
    case 48: return Op::LFS;
    case 49: return Op::LFSU;
    case 50: return Op::LFD;
    case 51: return Op::LFDU;
    case 52: return Op::STFS;
    case 53: return Op::STFSU;
    case 54: return Op::STFD;
    case 55: return Op::STFDU;
    case 58: return decode_58(i);
    case 59: return decode_59(i);
    case 62: return decode_62(i);
    case 63: return decode_63(i);
    default: return Op::INVALID;
    }
}

bool is_branch(Op op) {
    return op == Op::B || op == Op::BC || op == Op::BCLR || op == Op::BCCTR;
}

}  // namespace opentit::ppu
