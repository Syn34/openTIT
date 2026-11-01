// Decoder / disassembler / NID unit tests.
#include "loader/nid.h"
#include "ppu/ppu_decoder.h"
#include "ppu/ppu_disasm.h"
#include "test.h"

using namespace opentit;
using ppu::Op;

TEST(decode_common_instructions) {
    CHECK(ppu::decode(0x7C0802A6) == Op::MFSPR);   // mflr r0
    CHECK(ppu::decode(0xF8010010) == Op::STD);     // std r0, 16(r1)
    CHECK(ppu::decode(0xF821FF81) == Op::STDU);    // stdu r1, -128(r1)
    CHECK(ppu::decode(0x4E800020) == Op::BCLR);    // blr
    CHECK(ppu::decode(0x4E800420) == Op::BCCTR);   // bctr
    CHECK(ppu::decode(0x60000000) == Op::ORI);     // nop
    CHECK(ppu::decode(0x7C6A1B78) == Op::OR);      // mr r10, r3
    CHECK(ppu::decode(0x796A0020) == Op::RLDICL);  // clrldi r10, r11, 32
    CHECK(ppu::decode(0x7C0007B4) == Op::EXTSW);
    CHECK(ppu::decode(0x7C09582E) == Op::LWZX);
    CHECK(ppu::decode(0x44000002) == Op::SC);
    CHECK(ppu::decode(0x7C632674) == Op::SRADI);   // sradi r3, r3, 4
    CHECK(ppu::decode(0x7D6B0E70) == Op::SRAWI);
    CHECK(ppu::decode(0x7C0004AC) == Op::SYNC);
    CHECK(ppu::decode(0xFC00101E) == Op::FCTIWZ);
    CHECK(ppu::decode(0x1000038C) == Op::VSPLTISW);
    CHECK(ppu::decode(0x1000002B) == Op::VPERM);
    CHECK(ppu::decode(0x7C0000CE) == Op::LVX);
    CHECK(ppu::decode(0x7C00040E) == Op::LVLX);    // Cell-specific
    CHECK(ppu::decode(0x7C0001CE) == Op::STVX);
    CHECK(ppu::decode(0x00000000) == Op::INVALID);
    CHECK(ppu::decode(0xFFFFFF4C) == Op::INVALID); // jump-table data
}

TEST(xo_forms_with_oe) {
    // add (OE=0) and addo (OE=1) decode to the same op.
    CHECK(ppu::decode(0x7C632214) == Op::ADD);
    CHECK(ppu::decode(0x7C632614) == Op::ADD);
    CHECK(ppu::Insn(0x7C632614).oe());
    CHECK(ppu::decode(0x7C6321D6) == Op::MULLW);
    CHECK(ppu::decode(0x7C6323D6) == Op::DIVW);
}

TEST(field_extraction) {
    const ppu::Insn rld(0x79491788);  // rldic r9, r10, 2, 30
    CHECK_EQ(rld.rs(), 10u);
    CHECK_EQ(rld.ra(), 9u);
    CHECK_EQ(rld.sh64(), 2u);
    CHECK_EQ(rld.mbe64(), 30u);
    const ppu::Insn mf(0x7C0802A6);
    CHECK_EQ(mf.spr(), 8u);
    const ppu::Insn b(0x4BFFFFF0);  // b -16
    CHECK_EQ(b.li(), -16);
}

TEST(disassembly_text) {
    CHECK_STR(ppu::disasm(0x7C0802A6, 0), "mflr r0");
    CHECK_STR(ppu::disasm(0x4E800020, 0), "blr");
    CHECK_STR(ppu::disasm(0x60000000, 0), "nop");
    CHECK_STR(ppu::disasm(0x3860FFFF, 0), "li r3, -1");
    CHECK_STR(ppu::disasm(0x48000011, 0x1000), "bl 0x00001010");
    CHECK_STR(ppu::disasm(0x8162829C, 0), "lwz r11, -32100(r2)");
}

TEST(nid_hashing) {
    CHECK_EQ(loader::compute_nid("sys_process_exit"), 0xE6F2C1E7u);
    CHECK_EQ(loader::compute_nid("cellGcmInitBody"), 0xD7AA255Du);
    CHECK_STR(loader::nid_name(0x744680A2) ? loader::nid_name(0x744680A2) : "", "sys_initialize_tls");
    CHECK_STR(loader::nid_display_name("cellSpurs", 0x12345678), "cellSpurs_12345678");
}
