// Writes a small, valid PS3-style PPU ELF made of hand-assembled test
// functions. It is recompiled by opentit-recomp at build time and the
// generated C++ is executed by opentit_tests - an end-to-end check of the
// decoder, analysis, emitter and runtime that needs no game files.
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "common/types.h"
#include "testgen_layout.h"

using namespace opentit;
using namespace testgen;

namespace {

class Asm {
public:
    u32 pc() const { return kTextBase + static_cast<u32>(code.size() * 4); }
    void emit(u32 w) { code.push_back(w); }
    void label(const std::string& n) { labels[n] = pc(); }

    // --- forms
    void d(u32 op, u32 rt, u32 ra, s32 imm) { emit((op << 26) | (rt << 21) | (ra << 16) | (u32(imm) & 0xFFFF)); }
    void x(u32 rt, u32 ra, u32 rb, u32 xo, u32 rc = 0) { emit((31u << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (xo << 1) | rc); }
    void xo(u32 rt, u32 ra, u32 rb, u32 xo_, u32 oe = 0, u32 rc = 0) {
        emit((31u << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (oe << 10) | (xo_ << 1) | rc);
    }

    // --- integer
    void li(u32 rt, s32 v) { d(14, rt, 0, v); }
    void lis(u32 rt, s32 v) { d(15, rt, 0, v); }
    void addi(u32 rt, u32 ra, s32 v) { d(14, rt, ra, v); }
    void ori(u32 ra, u32 rs, u32 v) { d(24, rs, ra, static_cast<s32>(v)); }
    void mr(u32 ra, u32 rs) { x(rs, ra, rs, 444); }
    void add(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 266); }
    void addc(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 10); }
    void adde(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 138); }
    void subfc(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 8); }
    void subfe(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 136); }
    void addze(u32 rt, u32 ra) { xo(rt, ra, 0, 202); }
    void neg(u32 rt, u32 ra) { xo(rt, ra, 0, 104); }
    void mulhwu(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 11); }
    void divw(u32 rt, u32 ra, u32 rb) { xo(rt, ra, rb, 491); }
    void extsw(u32 ra, u32 rs) { x(rs, ra, 0, 986); }
    void extsh(u32 ra, u32 rs) { x(rs, ra, 0, 922); }
    void cntlzw(u32 ra, u32 rs) { x(rs, ra, 0, 26); }
    void slw(u32 ra, u32 rs, u32 rb) { x(rs, ra, rb, 24); }
    void srw(u32 ra, u32 rs, u32 rb) { x(rs, ra, rb, 536); }
    void srawi(u32 ra, u32 rs, u32 sh) { x(rs, ra, sh, 824); }
    void sradi(u32 ra, u32 rs, u32 sh) {
        emit((31u << 26) | (rs << 21) | (ra << 16) | ((sh & 31) << 11) | (413u << 2) | ((sh >> 5) << 1));
    }
    void rlwinm(u32 ra, u32 rs, u32 sh, u32 mb, u32 me) {
        emit((21u << 26) | (rs << 21) | (ra << 16) | (sh << 11) | (mb << 6) | (me << 1));
    }
    void rlwimi(u32 ra, u32 rs, u32 sh, u32 mb, u32 me) {
        emit((20u << 26) | (rs << 21) | (ra << 16) | (sh << 11) | (mb << 6) | (me << 1));
    }
    void rldicl(u32 ra, u32 rs, u32 sh, u32 mb) {
        emit((30u << 26) | (rs << 21) | (ra << 16) | ((sh & 31) << 11) | ((mb & 31) << 6) | ((mb >> 5) << 5) |
             ((sh >> 5) << 1));
    }
    void cmpwi(u32 bf, u32 ra, s32 v) { emit((11u << 26) | (bf << 23) | (ra << 16) | (u32(v) & 0xFFFF)); }
    void cmplwi(u32 bf, u32 ra, u32 v) { emit((10u << 26) | (bf << 23) | (ra << 16) | (v & 0xFFFF)); }
    void cmpw(u32 bf, u32 ra, u32 rb) { emit((31u << 26) | (bf << 23) | (ra << 16) | (rb << 11)); }
    void cmplw(u32 bf, u32 ra, u32 rb) { emit((31u << 26) | (bf << 23) | (ra << 16) | (rb << 11) | (32u << 1)); }
    void crand(u32 bt, u32 ba, u32 bb) { emit((19u << 26) | (bt << 21) | (ba << 16) | (bb << 11) | (257u << 1)); }
    void crnor(u32 bt, u32 ba, u32 bb) { emit((19u << 26) | (bt << 21) | (ba << 16) | (bb << 11) | (33u << 1)); }
    void mfcr(u32 rt) { emit((31u << 26) | (rt << 21) | (19u << 1)); }
    void mtspr(u32 spr, u32 rs) { emit((31u << 26) | (rs << 21) | ((spr & 31) << 16) | ((spr >> 5) << 11) | (467u << 1)); }
    void mfspr(u32 rt, u32 spr) { emit((31u << 26) | (rt << 21) | ((spr & 31) << 16) | ((spr >> 5) << 11) | (339u << 1)); }
    void mtctr(u32 rs) { mtspr(9, rs); }
    void mtlr(u32 rs) { mtspr(8, rs); }
    void mflr(u32 rt) { mfspr(rt, 8); }
    void sc() { emit(0x44000002); }

    // --- memory
    void lwz(u32 rt, s32 d_, u32 ra) { d(32, rt, ra, d_); }
    void stw(u32 rs, s32 d_, u32 ra) { d(36, rs, ra, d_); }
    void stwu(u32 rs, s32 d_, u32 ra) { d(37, rs, ra, d_); }
    void lbz(u32 rt, s32 d_, u32 ra) { d(34, rt, ra, d_); }
    void stb(u32 rs, s32 d_, u32 ra) { d(38, rs, ra, d_); }
    void lhz(u32 rt, s32 d_, u32 ra) { d(40, rt, ra, d_); }
    void lha(u32 rt, s32 d_, u32 ra) { d(42, rt, ra, d_); }
    void sth(u32 rs, s32 d_, u32 ra) { d(44, rs, ra, d_); }
    void ld(u32 rt, s32 ds, u32 ra) { d(58, rt, ra, ds & ~3); }
    void std_(u32 rs, s32 ds, u32 ra) { d(62, rs, ra, ds & ~3); }
    void stdu(u32 rs, s32 ds, u32 ra) { d(62, rs, ra, (ds & ~3) | 1); }
    void lwzx(u32 rt, u32 ra, u32 rb) { x(rt, ra, rb, 23); }
    void lwarx(u32 rt, u32 ra, u32 rb) { x(rt, ra, rb, 20); }
    void stwcx(u32 rs, u32 ra, u32 rb) { x(rs, ra, rb, 150, 1); }
    void lwbrx(u32 rt, u32 ra, u32 rb) { x(rt, ra, rb, 534); }

    // --- floating point
    void fa(u32 xo_, u32 frt, u32 fra, u32 frb, u32 frc, u32 op = 63) {
        emit((op << 26) | (frt << 21) | (fra << 16) | (frb << 11) | (frc << 6) | (xo_ << 1));
    }
    void fmadd(u32 t, u32 a, u32 c, u32 b) { fa(29, t, a, b, c); }
    void fmuls(u32 t, u32 a, u32 c) { fa(25, t, a, 0, c, 59); }
    void fadds(u32 t, u32 a, u32 b) { fa(21, t, a, b, 0, 59); }
    void fctiwz(u32 t, u32 b) { emit((63u << 26) | (t << 21) | (b << 11) | (15u << 1)); }
    void stfd(u32 s, s32 d_, u32 ra) { d(54, s, ra, d_); }
    void stfs(u32 s, s32 d_, u32 ra) { d(52, s, ra, d_); }

    // --- VMX
    void vx(u32 xo_, u32 vd, u32 va, u32 vb) { emit((4u << 26) | (vd << 21) | (va << 16) | (vb << 11) | xo_); }
    void va(u32 xo_, u32 vd, u32 va_, u32 vb, u32 vc) {
        emit((4u << 26) | (vd << 21) | (va_ << 16) | (vb << 11) | (vc << 6) | xo_);
    }
    void lvx(u32 vd, u32 ra, u32 rb) { x(vd, ra, rb, 103); }
    void stvx(u32 vs, u32 ra, u32 rb) { x(vs, ra, rb, 231); }
    void lvsl(u32 vd, u32 ra, u32 rb) { x(vd, ra, rb, 6); }

    // --- branches (label fixups)
    void b(const std::string& l, bool link = false) { fix(l, 'b'); emit((18u << 26) | (link ? 1 : 0)); }
    void bl(const std::string& l) { b(l, true); }
    void bc(u32 bo, u32 bi, const std::string& l) { fix(l, 'c'); emit((16u << 26) | (bo << 21) | (bi << 16)); }
    void blr() { emit(0x4E800020); }
    void bclr(u32 bo, u32 bi) { emit((19u << 26) | (bo << 21) | (bi << 16) | (16u << 1)); }
    void bctr() { emit(0x4E800420); }
    void bctrl() { emit(0x4E800421); }
    void word_rel(const std::string& l, const std::string& base) { rels.push_back({code.size(), l, base}); emit(0); }

    void resolve() {
        for (auto& [idx, l, kind] : fixups) {
            const u32 at = kTextBase + static_cast<u32>(idx * 4);
            const s32 off = static_cast<s32>(labels.at(l) - at);
            if (kind == 'b') code[idx] |= u32(off) & 0x03FFFFFC;
            else code[idx] |= u32(off) & 0xFFFC;
        }
        for (auto& [idx, l, base] : rels) code[idx] = labels.at(l) - labels.at(base);
    }

    std::vector<u32> code;
    std::map<std::string, u32> labels;

private:
    void fix(const std::string& l, char kind) { fixups.push_back({code.size(), l, kind}); }
    struct Fix { std::size_t idx; std::string label; char kind; };
    struct Rel { std::size_t idx; std::string label, base; };
    std::vector<Fix> fixups;
    std::vector<Rel> rels;
};

void put32(std::vector<u8>& v, std::size_t off, u32 x) { store_be<u32>(v.data() + off, x); }
void put16(std::vector<u8>& v, std::size_t off, u16 x) { store_be<u16>(v.data() + off, x); }
void put64(std::vector<u8>& v, std::size_t off, u64 x) { store_be<u64>(v.data() + off, x); }

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: opentit-testgen <out.elf>\n");
        return 2;
    }
    Asm a;
    std::map<Fn, std::string> fn_labels;
    auto func = [&](Fn f, const char* name) {
        fn_labels[f] = name;
        a.label(name);
    };

    // r3 = sum(1..r3)
    func(SumToN, "sum_to_n");
    a.mr(4, 3);
    a.li(3, 0);
    a.cmpwi(0, 4, 0);
    a.bclr(12, 2);  // beqlr
    a.mtctr(4);
    a.label("sum_loop");
    a.add(3, 3, 4);
    a.addi(4, 4, -1);
    a.bc(16, 0, "sum_loop");  // bdnz
    a.blr();

    // Bits: x = r3, out = r4
    func(Bits, "bits");
    a.rlwinm(5, 3, 8, 24, 31);  a.std_(5, 0, 4);
    a.rldicl(5, 3, 32, 32);     a.std_(5, 8, 4);
    a.sradi(5, 3, 4);           a.std_(5, 16, 4);
    a.srawi(5, 3, 3); a.addze(5, 5); a.std_(5, 24, 4);  // signed /8 toward zero
    a.cntlzw(5, 3);             a.std_(5, 32, 4);
    a.extsh(5, 3);              a.std_(5, 40, 4);
    a.neg(5, 3);                a.std_(5, 48, 4);
    a.mulhwu(5, 3, 3);          a.std_(5, 56, 4);
    a.li(7, 7); a.divw(5, 3, 7); a.std_(5, 64, 4);
    a.li(5, -1); a.rlwimi(5, 3, 4, 8, 15); a.std_(5, 72, 4);
    a.li(7, 33); a.slw(5, 3, 7); a.std_(5, 80, 4);
    a.li(7, 4); a.srw(5, 3, 7); a.std_(5, 88, 4);
    a.blr();

    // MemOps: r3 = buffer. Returns a checksum of loaded values; writes too.
    func(MemOps, "memops");
    a.li(5, 0x1234);  a.sth(5, 0, 3);  a.lha(6, 0, 3);        // 0x1234
    a.li(5, -2);      a.stb(5, 2, 3);  a.lbz(7, 2, 3);        // 0xFE
    a.li(5, -32768);  a.sth(5, 4, 3);  a.lha(8, 4, 3);        // -32768 sign-extended
    a.lhz(9, 4, 3);                                           // 0x8000
    a.mr(10, 3);
    a.li(5, 0x55);    a.stwu(5, 16, 10);                      // r10 += 16
    a.lwz(11, 0, 10);                                         // 0x55
    a.li(12, 0); a.lwbrx(12, 10, 12);                         // 0x55000000 (byte reversed)
    a.add(3, 6, 7); a.add(3, 3, 8); a.add(3, 3, 9); a.add(3, 3, 11);
    a.rlwinm(12, 12, 8, 24, 31); a.add(3, 3, 12);             // + 0x55
    a.blr();

    // CallChain: sum(n) + sum(n-1), with a real stack frame.
    func(CallChain, "call_chain");
    a.mflr(0);
    a.std_(0, 16, 1);
    a.stdu(1, -128, 1);
    a.std_(31, 112, 1);
    a.mr(31, 3);
    a.bl("sum_to_n");
    a.std_(3, 120, 1);
    a.addi(3, 31, -1);
    a.bl("sum_to_n");
    a.ld(4, 120, 1);
    a.add(3, 3, 4);
    a.ld(31, 112, 1);
    a.addi(1, 1, 128);
    a.ld(0, 16, 1);
    a.mtlr(0);
    a.blr();

    // Switch: compiler-style relative jump table, base loaded from the TOC.
    func(Switch, "switch");
    a.cmplwi(7, 3, 3);
    a.bc(12, 29, "sw_default");  // bgt cr7
    a.lwz(11, 0, 2);             // toc[0] = &table
    a.rlwinm(9, 3, 2, 0, 29);
    a.lwzx(0, 9, 11);
    a.extsw(0, 0);
    a.add(0, 0, 11);
    a.mtctr(0);
    a.bctr();
    a.label("sw_table");
    a.word_rel("sw_c0", "sw_table");
    a.word_rel("sw_c1", "sw_table");
    a.word_rel("sw_c2", "sw_table");
    a.word_rel("sw_c3", "sw_table");
    a.label("sw_c0"); a.li(3, 100); a.blr();
    a.label("sw_c1"); a.li(3, 200); a.blr();
    a.label("sw_c2"); a.li(3, 300); a.blr();
    a.label("sw_c3"); a.li(3, 400); a.blr();
    a.label("sw_default"); a.li(3, -1); a.blr();

    // Fp: r3 = (int)(f1*f2 + f1); f3 = single(f1*f2) + single(f2) stored at scratch+0x100 as float
    func(Fp, "fp");
    a.fmadd(0, 1, 2, 1);
    a.fctiwz(0, 0);
    a.stfd(0, -8, 1);
    a.lwz(3, -4, 1);
    a.fmuls(3, 1, 2);
    a.fadds(3, 3, 2);
    a.lis(4, kScratch >> 16);
    a.ori(4, 4, kScratch & 0xFFFF);
    a.stfs(3, 0x100, 4);
    a.blr();

    // Vec: out = rotate_words_left(in + 1.0f); out2 (out+16) = vperm(in, in, lvsl(3)) i.e. bytes shifted by 3
    func(Vec, "vec");
    a.lvx(0, 0, 3);
    a.vx(908, 1, 1, 0);          // vspltisw v1, 1
    a.vx(842, 1, 0, 1);          // vcfsx v1, v1, 0
    a.vx(10, 2, 0, 1);           // vaddfp v2, v0, v1
    a.va(44, 3, 2, 2, 4);        // vsldoi v3, v2, v2, 4
    a.stvx(3, 0, 4);
    a.li(5, 3);
    a.lvsl(4, 0, 5);             // permute control for shift by 3
    a.va(43, 5, 0, 0, 4);        // vperm v5, v0, v0, v4
    a.li(5, 16);
    a.stvx(5, 4, 5);
    a.blr();

    // Add128: (r3:r4) + (r5:r6) -> r3:r4 ; (r3:r4)-(r5:r6) computed first into r7:r8
    func(Add128, "add128");
    a.subfc(8, 6, 4);   // r8 = r4 - r6
    a.subfe(7, 5, 3);   // r7 = r3 - r5 - borrow
    a.addc(4, 4, 6);
    a.adde(3, 3, 5);
    a.mr(5, 7);
    a.mr(6, 8);
    a.blr();

    // CrLogic
    func(CrLogic, "crlogic");
    a.cmpw(0, 3, 4);
    a.cmpwi(1, 3, 0);
    a.cmplw(2, 3, 4);
    a.crand(31, 0, 4);   // cr7.so = cr0.lt & cr1.lt
    a.crnor(30, 2, 6);   // cr7.eq = !(cr0.eq | cr1.eq)
    a.mfcr(3);
    a.rlwinm(3, 3, 0, 0, 31);
    a.blr();

    // Syscall
    func(Syscall, "syscall_tb");
    a.li(11, 147);
    a.sc();
    a.blr();

    // TailCall
    func(TailCall, "tail_call");
    a.addi(3, 3, 1);
    a.b("sum_to_n");

    // AtomicInc
    func(AtomicInc, "atomic_inc");
    a.label("ai_retry");
    a.lwarx(4, 0, 3);
    a.addi(4, 4, 1);
    a.stwcx(4, 0, 3);
    a.bc(4, 2, "ai_retry");  // bne-
    a.mr(3, 4);
    a.blr();

    // Indirect: call SumToN through its OPD with bctrl (exercises call_indirect).
    func(Indirect, "indirect");
    a.mflr(0);
    a.std_(0, 16, 1);
    a.stdu(1, -112, 1);
    a.std_(2, 40, 1);
    a.lis(12, opd(SumToN) >> 16);
    a.ori(12, 12, opd(SumToN) & 0xFFFF);
    a.lwz(0, 0, 12);
    a.lwz(2, 4, 12);
    a.mtctr(0);
    a.bctrl();
    a.ld(2, 40, 1);
    a.addi(1, 1, 112);
    a.ld(0, 16, 1);
    a.mtlr(0);
    a.blr();

    a.resolve();

    // ------------------------------------------------------------ ELF image
    const u32 text_size = static_cast<u32>(a.code.size() * 4);
    const u32 text_off = 0x1000;
    const u32 data_off = 0x3000;
    std::vector<u8> data(kDataSize, 0);
    for (u32 f = 0; f < Count; ++f) {
        store_be<u32>(data.data() + 8 * f, a.labels.at(fn_labels.at(static_cast<Fn>(f))));
        store_be<u32>(data.data() + 8 * f + 4, kTocBase);
    }
    store_be<u32>(data.data() + (kTocBase - kDataBase), a.labels.at("sw_table"));

    const char shstr[] = "\0.text\0.opd\0.data\0.shstrtab\0";
    const u32 shstr_off = data_off + kDataSize;
    const u32 sh_off = (shstr_off + sizeof(shstr) + 7) & ~7u;
    const u16 shnum = 5;
    std::vector<u8> f(sh_off + shnum * 64, 0);

    // ELF header
    std::memcpy(f.data(), "\x7f" "ELF", 4);
    f[4] = 2; f[5] = 2; f[6] = 1; f[7] = 0x66;
    put16(f, 16, 2);      // ET_EXEC
    put16(f, 18, 21);     // EM_PPC64
    put32(f, 20, 1);
    put64(f, 24, opd(SumToN));
    put64(f, 32, 64);     // phoff
    put64(f, 40, sh_off);
    put16(f, 52, 64);
    put16(f, 54, 56);
    put16(f, 56, 2);
    put16(f, 58, 64);
    put16(f, 60, shnum);
    put16(f, 62, 4);      // shstrndx

    auto phdr = [&](int i, u32 flags, u32 off, u32 va, u32 filesz, u32 memsz) {
        const std::size_t o = 64 + i * 56;
        put32(f, o, 1);
        put32(f, o + 4, flags);
        put64(f, o + 8, off);
        put64(f, o + 16, va);
        put64(f, o + 24, va);
        put64(f, o + 32, filesz);
        put64(f, o + 40, memsz);
        put64(f, o + 48, 0x10000);
    };
    phdr(0, 5, text_off, kTextBase, text_size, text_size);
    phdr(1, 6, data_off, kDataBase, kDataSize, kDataSize);

    for (std::size_t k = 0; k < a.code.size(); ++k) put32(f, text_off + k * 4, a.code[k]);
    std::memcpy(f.data() + data_off, data.data(), data.size());
    std::memcpy(f.data() + shstr_off, shstr, sizeof(shstr));

    auto shdr = [&](int i, u32 name, u32 type, u64 flags, u32 addr, u32 off, u32 size) {
        const std::size_t o = sh_off + i * 64;
        put32(f, o, name);
        put32(f, o + 4, type);
        put64(f, o + 8, flags);
        put64(f, o + 16, addr);
        put64(f, o + 24, off);
        put64(f, o + 32, size);
    };
    shdr(1, 1, 1, 6, kTextBase, text_off, text_size);           // .text  (ALLOC|EXEC)
    shdr(2, 7, 1, 3, kOpdBase, data_off, 8 * Count);            // .opd   (ALLOC|WRITE)
    shdr(3, 12, 1, 3, kOpdBase + 8 * Count, data_off + 8 * Count, kDataSize - 8 * Count);  // .data
    shdr(4, 18, 3, 0, 0, shstr_off, sizeof(shstr));             // .shstrtab

    std::FILE* out = std::fopen(argv[1], "wb");
    if (!out) {
        std::perror(argv[1]);
        return 1;
    }
    std::fwrite(f.data(), 1, f.size(), out);
    std::fclose(out);
    std::printf("wrote %s: %zu instructions, %u functions\n", argv[1], a.code.size(), static_cast<u32>(Count));
    return 0;
}
