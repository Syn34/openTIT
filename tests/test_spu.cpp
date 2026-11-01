// SPU decoder + interpreter tests using hand-assembled programs.
#include <cstring>
#include <vector>

#include "opentit/memory.h"
#include "opentit/spu.h"
#include "spu/spu_decoder.h"
#include "test.h"
#include "testgen_layout.h"

using namespace opentit;
using spu::Op;

namespace {

// ---- encoders
u32 rr(u32 op11, u32 rt, u32 ra, u32 rb) { return (op11 << 21) | ((rb & 0x7F) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
u32 ri7(u32 op11, u32 rt, u32 ra, s32 i7) { return (op11 << 21) | ((u32(i7) & 0x7F) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
u32 ri8(u32 op10, u32 rt, u32 ra, u32 i8) { return (op10 << 22) | ((i8 & 0xFF) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
u32 ri10(u32 op8, u32 rt, u32 ra, s32 i10) { return (op8 << 24) | ((u32(i10) & 0x3FF) << 14) | ((ra & 0x7F) << 7) | (rt & 0x7F); }
u32 ri16(u32 op9, u32 rt, s32 i16) { return (op9 << 23) | ((u32(i16) & 0xFFFF) << 7) | (rt & 0x7F); }
u32 ri18(u32 op7, u32 rt, u32 i18) { return (op7 << 25) | ((i18 & 0x3FFFF) << 7) | (rt & 0x7F); }
u32 rrr(u32 op4, u32 rt, u32 ra, u32 rb, u32 rc) { return (op4 << 28) | ((rt & 0x7F) << 21) | ((rb & 0x7F) << 14) | ((ra & 0x7F) << 7) | (rc & 0x7F); }

u32 IL(u32 rt, s32 v) { return ri16(0x081, rt, v); }
u32 ILHU(u32 rt, u32 v) { return ri16(0x082, rt, static_cast<s32>(v)); }
u32 IOHL(u32 rt, u32 v) { return ri16(0x0C1, rt, static_cast<s32>(v)); }
u32 ILA(u32 rt, u32 v) { return ri18(0x21, rt, v); }
u32 A(u32 rt, u32 ra, u32 rb) { return rr(0x0C0, rt, ra, rb); }
u32 AI(u32 rt, u32 ra, s32 i) { return ri10(0x1C, rt, ra, i); }
u32 SF(u32 rt, u32 ra, u32 rb) { return rr(0x040, rt, ra, rb); }
u32 BRNZ(u32 rt, s32 words) { return ri16(0x042, rt, words); }
u32 BRSL(u32 rt, s32 words) { return ri16(0x066, rt, words); }
u32 BI(u32 ra) { return rr(0x1A8, 0, ra, 0); }
u32 STOP(u32 code) { return code & 0x3FFF; }
u32 LQD(u32 rt, u32 ra, s32 qoff) { return ri10(0x34, rt, ra, qoff); }
u32 STQD(u32 rt, u32 ra, s32 qoff) { return ri10(0x24, rt, ra, qoff); }
u32 CWD(u32 rt, u32 ra, s32 i7) { return ri7(0x1F6, rt, ra, i7); }
u32 SHUFB(u32 rt, u32 ra, u32 rb, u32 rc) { return rrr(0xB, rt, ra, rb, rc); }
u32 ROTQBYI(u32 rt, u32 ra, s32 n) { return ri7(0x1FC, rt, ra, n); }
u32 SHLQBYI(u32 rt, u32 ra, s32 n) { return ri7(0x1FF, rt, ra, n); }
u32 FSMBI(u32 rt, u32 m) { return ri16(0x065, rt, static_cast<s32>(m)); }
u32 SELB(u32 rt, u32 ra, u32 rb, u32 rc) { return rrr(0x8, rt, ra, rb, rc); }
u32 CEQ(u32 rt, u32 ra, u32 rb) { return rr(0x3C0, rt, ra, rb); }
u32 FA(u32 rt, u32 ra, u32 rb) { return rr(0x2C4, rt, ra, rb); }
u32 FM(u32 rt, u32 ra, u32 rb) { return rr(0x2C6, rt, ra, rb); }
u32 FMA(u32 rt, u32 ra, u32 rb, u32 rc) { return rrr(0xE, rt, ra, rb, rc); }
u32 CSFLT(u32 rt, u32 ra, u32 scale) { return ri8(0x1DA, rt, ra, 155 - scale); }
u32 CFLTS(u32 rt, u32 ra, u32 scale) { return ri8(0x1D8, rt, ra, 173 - scale); }
u32 WRCH(u32 ch, u32 rt) { return rr(0x10D, rt, ch, 0); }
u32 RDCH(u32 rt, u32 ch) { return rr(0x00D, rt, ch, 0); }
u32 MPY(u32 rt, u32 ra, u32 rb) { return rr(0x3C4, rt, ra, rb); }
u32 ROTMI(u32 rt, u32 ra, s32 n) { return ri7(0x079, rt, ra, -n); }
u32 ROTMAI(u32 rt, u32 ra, s32 n) { return ri7(0x07A, rt, ra, -n); }
u32 CLZ(u32 rt, u32 ra) { return rr(0x2A5, rt, ra, 0); }
u32 GBB(u32 rt, u32 ra) { return rr(0x1B2, rt, ra, 0); }

// Load `ea` into register rt (ILHU+IOHL).
void li32(std::vector<u32>& p, u32 rt, u32 v) {
    p.push_back(ILHU(rt, v >> 16));
    p.push_back(IOHL(rt, v & 0xFFFF));
}

void dma(std::vector<u32>& p, u32 cmd, u32 lsa_reg, u32 ea_reg, u32 size_reg, u32 tag_reg, u32 cmd_reg) {
    p.push_back(WRCH(spu::MFC_LSA, lsa_reg));
    p.push_back(WRCH(spu::MFC_EAL, ea_reg));
    p.push_back(WRCH(spu::MFC_Size, size_reg));
    p.push_back(WRCH(spu::MFC_TagID, tag_reg));
    p.push_back(IL(cmd_reg, static_cast<s32>(cmd)));
    p.push_back(WRCH(spu::MFC_Cmd, cmd_reg));
}

u32 run_prog(spu::SPUThread& t, const std::vector<u32>& prog, u32 at = 0x100) {
    for (std::size_t k = 0; k < prog.size(); ++k) t.set_ls32(at + 4 * static_cast<u32>(k), prog[k]);
    t.pc = at;
    return spu::run(t);
}

}  // namespace

TEST(spu_decode_forms) {
    CHECK(spu::decode(IL(3, 5)) == Op::IL);
    CHECK(spu::decode(ILA(3, 0x3FFFF)) == Op::ILA);
    CHECK(spu::decode(A(1, 2, 3)) == Op::A);
    CHECK(spu::decode(AI(1, 2, -1)) == Op::AI);
    CHECK(spu::decode(SHUFB(1, 2, 3, 4)) == Op::SHUFB);
    CHECK(spu::decode(CSFLT(1, 2, 0)) == Op::CSFLT);
    CHECK(spu::decode(0x40200000) == Op::NOP);
    CHECK(spu::decode(0x00200000) == Op::LNOP);
    CHECK(spu::decode(0x00000000) == Op::STOP);
    CHECK_STR(spu::disasm(AI(5, 6, -16), 0), "ai $5, $6, -16");
    CHECK_STR(spu::disasm(LQD(9, 1, 2), 0), "lqd $9, 32($1)");
}

TEST(spu_loop_and_stop) {
    spu::SPUThread t;
    // $3 = sum(10..1)
    const std::vector<u32> p = {IL(3, 0), IL(4, 10), A(3, 3, 4), AI(4, 4, -1), BRNZ(4, -2), STOP(0x123)};
    CHECK_EQ(run_prog(t, p), 0x123u);
    CHECK_EQ(t.gpr[3].uw(0), 55u);
    CHECK_EQ(t.gpr[3].uw(3), 55u);  // il splats to every slot, a is element-wise
}

TEST(spu_call_return_and_immediates) {
    spu::SPUThread t;
    std::vector<u32> p = {IL(3, 41), BRSL(0, 3), STOP(1), STOP(2), AI(3, 3, 1), BI(0)};
    CHECK_EQ(run_prog(t, p), 1u);
    CHECK_EQ(t.gpr[3].uw(0), 42u);
    CHECK_EQ(t.gpr[0].uw(0), 0x108u);  // link = address after brsl
    p = {ILHU(5, 0x1234), IOHL(5, 0x5678), ILA(6, 0x3FFFF), STOP(0)};
    run_prog(t, p);
    CHECK_EQ(t.gpr[5].uw(2), 0x12345678u);
    CHECK_EQ(t.gpr[6].uw(1), 0x3FFFFu);
}

TEST(spu_shuffle_insert_and_quadword_shifts) {
    spu::SPUThread t;
    // Insert word 0xCAFEBABE into slot 2 of a zero quad using cwd + shufb.
    std::vector<u32> p;
    li32(p, 10, 0xCAFEBABE);
    p.push_back(IL(11, 0));
    p.push_back(IL(12, 0));
    p.push_back(CWD(13, 12, 8));          // control for word at byte 8
    p.push_back(SHUFB(11, 10, 11, 13));   // ra = value (preferred slot), rb = target
    p.push_back(FSMBI(14, 0xF000));       // first 4 bytes 0xFF
    p.push_back(ROTQBYI(15, 11, 8));      // rotate left 8 bytes: slot2 -> slot0
    p.push_back(SHLQBYI(16, 11, 12));     // shift left 12: everything gone except slot 3 (zero)
    p.push_back(SELB(17, 11, 14, 14));    // bytes where mask set take rb (=mask)
    p.push_back(STOP(0));
    run_prog(t, p);
    CHECK_EQ(t.gpr[11].uw(0), 0u);
    CHECK_EQ(t.gpr[11].uw(2), 0xCAFEBABEu);
    CHECK_EQ(t.gpr[14].uw(0), 0xFFFFFFFFu);
    CHECK_EQ(t.gpr[14].uw(1), 0u);
    CHECK_EQ(t.gpr[15].uw(0), 0xCAFEBABEu);
    CHECK_EQ(t.gpr[16].uw(0), 0u);
    CHECK_EQ(t.gpr[17].uw(0), 0xFFFFFFFFu);
    CHECK_EQ(t.gpr[17].uw(2), 0xCAFEBABEu);
}

TEST(spu_integer_misc) {
    spu::SPUThread t;
    std::vector<u32> p;
    li32(p, 3, 0xFFFF8000);  // low half = -32768
    p.push_back(IL(4, 3));
    p.push_back(MPY(5, 3, 4));       // -32768 * 3
    p.push_back(ROTMI(6, 3, 4));     // logical >> 4
    p.push_back(ROTMAI(7, 3, 4));    // arithmetic >> 4
    p.push_back(CLZ(8, 4));          // clz(3) = 30
    p.push_back(SF(9, 4, 3));        // $3 - $4
    p.push_back(CEQ(10, 4, 4));
    p.push_back(GBB(11, 10));        // all bytes 0xFF -> 0xFFFF
    p.push_back(STOP(0));
    run_prog(t, p);
    CHECK_EQ(s32(t.gpr[5].uw(0)), -98304);
    CHECK_EQ(t.gpr[6].uw(0), 0x0FFFF800u);
    CHECK_EQ(t.gpr[7].uw(0), 0xFFFFF800u);
    CHECK_EQ(t.gpr[8].uw(0), 30u);
    CHECK_EQ(t.gpr[9].uw(0), 0xFFFF7FFDu);
    CHECK_EQ(t.gpr[10].uw(3), 0xFFFFFFFFu);
    CHECK_EQ(t.gpr[11].uw(0), 0xFFFFu);
}

TEST(spu_float_and_conversions) {
    spu::SPUThread t;
    std::vector<u32> p = {IL(3, 6), CSFLT(4, 3, 1), FA(5, 4, 4), FM(6, 5, 4), FMA(7, 4, 4, 5), CFLTS(8, 7, 2), STOP(0)};
    run_prog(t, p);
    CHECK_EQ(t.gpr[4].fw(0), 3.0f);    // 6 / 2^1
    CHECK_EQ(t.gpr[5].fw(1), 6.0f);
    CHECK_EQ(t.gpr[6].fw(2), 18.0f);
    CHECK_EQ(t.gpr[7].fw(3), 15.0f);   // 3*3 + 6
    CHECK_EQ(t.gpr[8].sw(0), 60);      // 15 * 2^2
}

TEST(spu_dma_get_put_and_mailbox) {
    spu::SPUThread t;
    const u32 ea = testgen::kScratch + 0x800;
    for (u32 k = 0; k < 4; ++k) mem::w32(ea + 4 * k, 100 + k);
    t.push_in_mbox(7);
    std::vector<u32> p;
    p.push_back(ILA(20, 0x1000));    // LS buffer
    li32(p, 21, ea);
    p.push_back(IL(22, 16));         // size
    p.push_back(IL(23, 3));          // tag
    dma(p, spu::MFC_GET_CMD, 20, 21, 22, 23, 24);
    p.push_back(IL(25, 1 << 3));
    p.push_back(WRCH(spu::MFC_WrTagMask, 25));
    p.push_back(IL(26, 2));
    p.push_back(WRCH(spu::MFC_WrTagUpdate, 26));
    p.push_back(RDCH(27, spu::MFC_RdTagStat));
    p.push_back(LQD(30, 20, 0));
    p.push_back(RDCH(31, spu::SPU_RdInMbox));   // 7
    p.push_back(A(30, 30, 31));                 // +7 to the preferred slot only
    p.push_back(STQD(30, 20, 1));               // LS 0x1010
    p.push_back(AI(28, 20, 16));
    p.push_back(AI(29, 21, 16));
    dma(p, spu::MFC_PUT_CMD, 28, 29, 22, 23, 24);
    p.push_back(WRCH(spu::SPU_WrOutMbox, 27));
    p.push_back(STOP(0));
    run_prog(t, p);
    for (u32 k = 0; k < 4; ++k) CHECK_EQ(mem::r32(ea + 16 + 4 * k), k == 0 ? 107u : 100u + k);
    u32 out = 0;
    CHECK(t.pop_out_mbox(&out));
    CHECK_EQ(out, 1u << 3);
}

TEST(spu_lock_line_reservation) {
    spu::SPUThread t;
    const u32 ea = testgen::kScratch + 0xC00;  // 128-byte aligned
    std::memset(mem::ptr(ea), 0, 128);
    mem::w32(ea, 5);
    std::vector<u32> p;
    p.push_back(ILA(20, 0x2000));
    li32(p, 21, ea);
    p.push_back(IL(22, 128));
    p.push_back(IL(23, 0));
    dma(p, spu::MFC_GETLLAR_CMD, 20, 21, 22, 23, 24);
    p.push_back(RDCH(25, spu::MFC_RdAtomicStat));  // 4
    p.push_back(LQD(26, 20, 0));
    p.push_back(AI(26, 26, 1));
    p.push_back(STQD(26, 20, 0));
    dma(p, spu::MFC_PUTLLC_CMD, 20, 21, 22, 23, 24);
    p.push_back(RDCH(27, spu::MFC_RdAtomicStat));  // 0 = success
    // second PUTLLC without a reservation must fail
    dma(p, spu::MFC_PUTLLC_CMD, 20, 21, 22, 23, 24);
    p.push_back(RDCH(28, spu::MFC_RdAtomicStat));  // 1 = failure
    p.push_back(STOP(0));
    run_prog(t, p);
    CHECK_EQ(t.gpr[25].uw(0), 4u);
    CHECK_EQ(t.gpr[27].uw(0), 0u);
    CHECK_EQ(t.gpr[28].uw(0), 1u);
    CHECK_EQ(mem::r32(ea), 6u);
}
