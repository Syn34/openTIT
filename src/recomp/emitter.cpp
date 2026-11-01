#include <sstream>
#include "recomp/emitter.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

#include "ppu/ppu_disasm.h"

namespace opentit::recomp {

using ppu::Insn;
using ppu::Op;

namespace {

std::string S(const char* f, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return buf;
}

std::string lower_name(Op op) {
    std::string s = ppu::op_name(op);
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

u64 mask64(u32 mb, u32 me) {
    const u64 m1 = ~0ull >> mb;
    const u64 m2 = ~0ull << (63 - me);
    return mb <= me ? (m1 & m2) : (m1 | m2);
}

std::string hex64(u64 v) { return S("0x%016llXull", static_cast<unsigned long long>(v)); }

// Signed immediate as a C++ literal that adds correctly to a u64.
std::string simm(s64 v) {
    if (v < 0) return S("(u64)%lldll", static_cast<long long>(v));
    return S("%lldull", static_cast<long long>(v));
}

class FunctionEmitter {
public:
    FunctionEmitter(const loader::Elf& elf, const analysis::Program& prog, const analysis::Function& fn, bool annotate,
                    std::map<u32, bool>* referenced, EmitStats* stats)
        : elf_(elf), prog_(prog), fn_(fn), annotate_(annotate), referenced_(referenced), stats_(stats) {}

    std::string run();

private:
    void line(const std::string& s) { out_ += "    " + s + "\n"; }
    std::string fname(u32 a) {
        if (referenced_) (*referenced_)[a] = true;
        return S("f_%08X", a);
    }
    bool is_function(u32 a) const { return prog_.functions.count(a) != 0; }
    bool is_local(u32 a) const { return fn_.labels.count(a) != 0 || a == fn_.start; }

    // EA helpers
    std::string ea_d(const Insn i, s64 d) const {
        if (i.ra() == 0) return S("(u32)%s", simm(d).c_str());
        return S("(u32)(r[%u] + %s)", i.ra(), simm(d).c_str());
    }
    std::string ea_x(const Insn i) const {
        if (i.ra() == 0) return S("(u32)r[%u]", i.rb());
        return S("(u32)(r[%u] + r[%u])", i.ra(), i.rb());
    }

    std::string branch_cond(u32 bo, u32 bi, bool allow_ctr, std::string* pre);
    void emit_branch_to(u32 target, u32 pc, bool link);
    void emit_insn(u32 pc, u32 raw);
    void fallback(Op op) {
        if (stats_) stats_->fallback_ops[op]++;
    }

    const loader::Elf& elf_;
    const analysis::Program& prog_;
    const analysis::Function& fn_;
    bool annotate_;
    std::map<u32, bool>* referenced_;
    EmitStats* stats_;
    std::string out_;
};

std::string FunctionEmitter::branch_cond(u32 bo, u32 bi, bool allow_ctr, std::string* pre) {
    std::string ctr_c, cr_c;
    if (allow_ctr && !(bo & 0x04)) {
        *pre = "--ctx.ctr;";
        ctr_c = (bo & 0x02) ? "ctx.ctr == 0" : "ctx.ctr != 0";
    }
    if (!(bo & 0x10)) cr_c = (bo & 0x08) ? S("cr[%u]", bi) : S("!cr[%u]", bi);
    if (ctr_c.empty() && cr_c.empty()) return "";
    if (ctr_c.empty()) return cr_c;
    if (cr_c.empty()) return ctr_c;
    return "(" + ctr_c + ") && " + cr_c;
}

// Unconditional transfer to `target` (b / tail call / call).
void FunctionEmitter::emit_branch_to(u32 target, u32 pc, bool link) {
    if (link) {
        line(S("ctx.lr = 0x%08X;", pc + 4));
        if (is_function(target)) line(fname(target) + "(ctx);");
        else line(S("call_indirect(ctx, 0x%08X, 0x%08X);", target, pc));
        return;
    }
    if (is_local(target) && target != fn_.start) {
        line(S("goto L_%08X;", target));
    } else if (target == fn_.start) {
        line(S("goto L_%08X;", target));
    } else if (is_function(target)) {
        line("return " + fname(target) + "(ctx);");
    } else {
        line(S("return call_indirect(ctx, 0x%08X, 0x%08X);", target, pc));
    }
}

void FunctionEmitter::emit_insn(u32 pc, u32 raw) {
    const Insn i(raw);
    const Op op = ppu::decode(raw);
    const u32 next = pc + 4;
    const u32 d = i.rd(), a = i.ra(), b = i.rb(), s = i.rs();
    const bool rc = i.rc();
    auto rec0 = [&](u32 reg) {
        if (rc) line(S("rt::cr0(ctx, r[%u]);", reg));
    };
    auto frec = [&] {
        if (rc) line("rt::fpr_record(ctx);");
    };

    switch (op) {
    // ------------------------------------------------------------ branches
    case Op::B: {
        const u32 t = i.aa() ? static_cast<u32>(i.li()) : pc + static_cast<u32>(i.li());
        if (i.lk() && t == next) {
            line(S("ctx.lr = 0x%08X;", next));
            break;
        }
        emit_branch_to(t, pc, i.lk());
        break;
    }
    case Op::BC: {
        const u32 t = i.aa() ? static_cast<u32>(i.bd()) : pc + static_cast<u32>(i.bd());
        std::string pre;
        const std::string c = branch_cond(i.bo(), i.bi(), true, &pre);
        if (!pre.empty()) line(pre);
        if (i.lk()) {
            line(S("ctx.lr = 0x%08X;", next));
            if (t == next) break;
            const std::string call =
                is_function(t) ? fname(t) + "(ctx);" : S("call_indirect(ctx, 0x%08X, 0x%08X);", t, pc);
            line(c.empty() ? call : "if (" + c + ") " + call);
            break;
        }
        std::string act;
        if (is_local(t)) act = S("goto L_%08X;", t);
        else if (is_function(t)) act = "return " + fname(t) + "(ctx);";
        else act = S("return call_indirect(ctx, 0x%08X, 0x%08X);", t, pc);
        line(c.empty() ? act : "if (" + c + ") " + act);
        break;
    }
    case Op::BCLR: {
        std::string pre;
        const std::string c = branch_cond(i.bo(), i.bi(), true, &pre);
        if (!pre.empty()) line(pre);
        if (i.lk()) {
            std::string body = S("{ const u32 t_ = (u32)ctx.lr; ctx.lr = 0x%08X; call_indirect(ctx, t_, 0x%08X); }", next, pc);
            line(c.empty() ? body : "if (" + c + ") " + body);
        } else {
            line(c.empty() ? "return;" : "if (" + c + ") return;");
        }
        break;
    }
    case Op::BCCTR: {
        std::string pre;
        const std::string c = branch_cond(i.bo(), i.bi(), false, &pre);
        if (i.lk()) {
            std::string body = S("{ const u32 t_ = (u32)ctx.ctr; ctx.lr = 0x%08X; call_indirect(ctx, t_, 0x%08X); }", next, pc);
            line(c.empty() ? body : "if (" + c + ") " + body);
            break;
        }
        auto jt = fn_.jump_tables.find(pc);
        std::string body;
        if (jt != fn_.jump_tables.end()) {
            body = "switch ((u32)ctx.ctr) {";
            std::set<u32> done;
            for (u32 t : jt->second.targets) {
                if (!done.insert(t).second) continue;
                if (is_local(t)) body += S(" case 0x%08X: goto L_%08X;", t, t);
                else body += S(" case 0x%08X: return %s(ctx);", t, fname(t).c_str());
            }
            body += S(" default: return call_indirect(ctx, (u32)ctx.ctr, 0x%08X); }", pc);
        } else {
            body = S("return call_indirect(ctx, (u32)ctx.ctr, 0x%08X);", pc);
        }
        line(c.empty() ? body : "if (" + c + ") { " + body + " }");
        break;
    }
    case Op::SC: line(S("ppu_syscall(ctx, 0x%08X);", pc)); break;

    // ------------------------------------------------------------ traps
    case Op::TW:
    case Op::TWI: {
        if (d == 31 && op == Op::TW && a == 0 && b == 0) { line(S("trap(ctx, 0x%08X);", pc)); break; }
        const std::string rhs = op == Op::TWI ? S("%d", i.simm16()) : S("(s64)(s32)r[%u]", b);
        line(S("if (rt::trap_cond(%u, (s64)(s32)r[%u], %s)) trap(ctx, 0x%08X);", d, a, rhs.c_str(), pc));
        break;
    }
    case Op::TD:
    case Op::TDI: {
        const std::string rhs = op == Op::TDI ? S("%d", i.simm16()) : S("(s64)r[%u]", b);
        line(S("if (rt::trap_cond(%u, (s64)r[%u], %s)) trap(ctx, 0x%08X);", d, a, rhs.c_str(), pc));
        break;
    }

    // ------------------------------------------------------------ CR ops
    case Op::MCRF:
        for (int k = 0; k < 4; ++k) line(S("cr[%u] = cr[%u];", i.crfd() * 4 + k, i.crfs() * 4 + k));
        break;
    case Op::CRAND: line(S("cr[%u] = cr[%u] & cr[%u];", d, a, b)); break;
    case Op::CROR: line(S("cr[%u] = cr[%u] | cr[%u];", d, a, b)); break;
    case Op::CRXOR: line(S("cr[%u] = cr[%u] ^ cr[%u];", d, a, b)); break;
    case Op::CRNAND: line(S("cr[%u] = (u8)!(cr[%u] & cr[%u]);", d, a, b)); break;
    case Op::CRNOR: line(S("cr[%u] = (u8)!(cr[%u] | cr[%u]);", d, a, b)); break;
    case Op::CREQV: line(S("cr[%u] = (u8)(cr[%u] == cr[%u]);", d, a, b)); break;
    case Op::CRANDC: line(S("cr[%u] = (u8)(cr[%u] & !cr[%u]);", d, a, b)); break;
    case Op::CRORC: line(S("cr[%u] = (u8)(cr[%u] | !cr[%u]);", d, a, b)); break;
    case Op::MFCR:
    case Op::MFOCRF: line(S("r[%u] = ctx.get_cr();", d)); break;
    case Op::MTCRF:
    case Op::MTOCRF:
        for (u32 f = 0; f < 8; ++f)
            if (i.crm() & (0x80u >> f))
                for (u32 k = 0; k < 4; ++k)
                    line(S("cr[%u] = (u8)((r[%u] >> %u) & 1);", f * 4 + k, s, 31 - (f * 4 + k)));
        break;

    // ------------------------------------------------------------ integer arithmetic
    case Op::ADDI:
        if (a == 0) line(S("r[%u] = %s;", d, hex64(static_cast<u64>(static_cast<s64>(i.simm16()))).c_str()));
        else line(S("r[%u] = r[%u] + %s;", d, a, simm(i.simm16()).c_str()));
        break;
    case Op::ADDIS: {
        const s64 v = static_cast<s64>(static_cast<s32>(static_cast<u32>(i.simm16()) << 16));
        if (a == 0) line(S("r[%u] = %s;", d, hex64(static_cast<u64>(v)).c_str()));
        else line(S("r[%u] = r[%u] + %s;", d, a, simm(v).c_str()));
        break;
    }
    case Op::ADDIC:
    case Op::ADDIC_:
        line(S("r[%u] = rt::add_ca(r[%u], %s, 0, ctx.xer_ca);", d, a, simm(i.simm16()).c_str()));
        if (op == Op::ADDIC_) line(S("rt::cr0(ctx, r[%u]);", d));
        break;
    case Op::SUBFIC:
        line(S("r[%u] = rt::add_ca(~r[%u], %s, 1, ctx.xer_ca);", d, a, simm(i.simm16()).c_str()));
        break;
    case Op::MULLI: line(S("r[%u] = (u64)((s64)r[%u] * %lldll);", d, a, static_cast<long long>(i.simm16()))); break;

    case Op::ADD:
    case Op::SUBF:
    case Op::ADDC:
    case Op::SUBFC:
    case Op::ADDE:
    case Op::SUBFE:
    case Op::ADDZE:
    case Op::SUBFZE:
    case Op::ADDME:
    case Op::SUBFME:
    case Op::NEG: {
        // Normalise everything to x + y + cin.
        std::string x = S("r[%u]", a), y = S("r[%u]", b), cin = "0";
        bool carry = true;
        switch (op) {
        case Op::ADD: carry = false; break;
        case Op::SUBF: x = S("~r[%u]", a); cin = "1"; carry = false; break;
        case Op::ADDC: break;
        case Op::SUBFC: x = S("~r[%u]", a); cin = "1"; break;
        case Op::ADDE: cin = "ctx.xer_ca"; break;
        case Op::SUBFE: x = S("~r[%u]", a); cin = "ctx.xer_ca"; break;
        case Op::ADDZE: y = "0"; cin = "ctx.xer_ca"; break;
        case Op::SUBFZE: x = S("~r[%u]", a); y = "0"; cin = "ctx.xer_ca"; break;
        case Op::ADDME: y = "~0ull"; cin = "ctx.xer_ca"; break;
        case Op::SUBFME: x = S("~r[%u]", a); y = "~0ull"; cin = "ctx.xer_ca"; break;
        case Op::NEG: x = S("~r[%u]", a); y = "0"; cin = "1"; carry = false; break;
        default: break;
        }
        if (!i.oe()) {
            if (!carry) {
                if (op == Op::ADD) line(S("r[%u] = r[%u] + r[%u];", d, a, b));
                else if (op == Op::SUBF) line(S("r[%u] = r[%u] - r[%u];", d, b, a));
                else line(S("r[%u] = 0 - r[%u];", d, a));
            } else {
                line(S("r[%u] = rt::add_ca(%s, %s, %s, ctx.xer_ca);", d, x.c_str(), y.c_str(), cin.c_str()));
            }
        } else {
            line(S("{ const u64 x_ = %s, y_ = %s; u8 ca_; const u64 r_ = rt::add_ca(x_, y_, %s, ca_);", x.c_str(),
                   y.c_str(), cin.c_str()));
            line(S("  rt::set_ov(ctx, rt::add_overflow(x_, y_, r_));%s r[%u] = r_; }", carry ? " ctx.xer_ca = ca_;" : "",
                   d));
        }
        rec0(d);
        break;
    }
    case Op::MULLW:
        line(S("r[%u] = (u64)((s64)(s32)r[%u] * (s64)(s32)r[%u]);", d, a, b));
        if (i.oe()) line(S("rt::set_ov(ctx, (s64)r[%u] != (s64)(s32)r[%u]);", d, d));
        rec0(d);
        break;
    case Op::MULLD:
        if (i.oe()) line(S("rt::set_ov(ctx, rt::mulld_overflow(r[%u], r[%u]));", a, b));
        line(S("r[%u] = (u64)((s64)r[%u] * (s64)r[%u]);", d, a, b));
        rec0(d);
        break;
    case Op::MULHW: line(S("r[%u] = (u64)(((s64)(s32)r[%u] * (s64)(s32)r[%u]) >> 32);", d, a, b)); rec0(d); break;
    case Op::MULHWU: line(S("r[%u] = ((u64)(u32)r[%u] * (u64)(u32)r[%u]) >> 32;", d, a, b)); rec0(d); break;
    case Op::MULHD: line(S("r[%u] = rt::mulhd(r[%u], r[%u]);", d, a, b)); rec0(d); break;
    case Op::MULHDU: line(S("r[%u] = rt::mulhdu(r[%u], r[%u]);", d, a, b)); rec0(d); break;
    case Op::DIVW:
    case Op::DIVWU:
    case Op::DIVD:
    case Op::DIVDU: {
        const std::string n = lower_name(op);
        if (i.oe()) {
            if (op == Op::DIVW)
                line(S("rt::set_ov(ctx, (s32)r[%u] == 0 || ((s32)r[%u] == (s32)0x80000000 && (s32)r[%u] == -1));", b, a, b));
            else if (op == Op::DIVD)
                line(S("rt::set_ov(ctx, r[%u] == 0 || (r[%u] == 0x8000000000000000ull && (s64)r[%u] == -1));", b, a, b));
            else if (op == Op::DIVWU) line(S("rt::set_ov(ctx, (u32)r[%u] == 0);", b));
            else line(S("rt::set_ov(ctx, r[%u] == 0);", b));
        }
        line(S("r[%u] = rt::%s(r[%u], r[%u]);", d, n.c_str(), a, b));
        rec0(d);
        break;
    }

    // ------------------------------------------------------------ compares
    case Op::CMPI:
        if (i.l10()) line(S("rt::cr_cmp<s64>(ctx, %u, (s64)r[%u], %d);", i.crfd(), a, i.simm16()));
        else line(S("rt::cr_cmp<s32>(ctx, %u, (s32)r[%u], %d);", i.crfd(), a, i.simm16()));
        break;
    case Op::CMPLI:
        if (i.l10()) line(S("rt::cr_cmp<u64>(ctx, %u, r[%u], %uull);", i.crfd(), a, i.uimm16()));
        else line(S("rt::cr_cmp<u32>(ctx, %u, (u32)r[%u], %uu);", i.crfd(), a, i.uimm16()));
        break;
    case Op::CMP:
        if (i.l10()) line(S("rt::cr_cmp<s64>(ctx, %u, (s64)r[%u], (s64)r[%u]);", i.crfd(), a, b));
        else line(S("rt::cr_cmp<s32>(ctx, %u, (s32)r[%u], (s32)r[%u]);", i.crfd(), a, b));
        break;
    case Op::CMPL:
        if (i.l10()) line(S("rt::cr_cmp<u64>(ctx, %u, r[%u], r[%u]);", i.crfd(), a, b));
        else line(S("rt::cr_cmp<u32>(ctx, %u, (u32)r[%u], (u32)r[%u]);", i.crfd(), a, b));
        break;

    // ------------------------------------------------------------ logical
    case Op::ORI:
        if (raw == 0x60000000) { line("/* nop */"); break; }
        line(S("r[%u] = r[%u] | 0x%Xull;", a, s, i.uimm16()));
        break;
    case Op::ORIS: line(S("r[%u] = r[%u] | 0x%Xull;", a, s, i.uimm16() << 16)); break;
    case Op::XORI: line(S("r[%u] = r[%u] ^ 0x%Xull;", a, s, i.uimm16())); break;
    case Op::XORIS: line(S("r[%u] = r[%u] ^ 0x%Xull;", a, s, i.uimm16() << 16)); break;
    case Op::ANDI_: line(S("r[%u] = r[%u] & 0x%Xull;", a, s, i.uimm16())); line(S("rt::cr0(ctx, r[%u]);", a)); break;
    case Op::ANDIS_: line(S("r[%u] = r[%u] & 0x%Xull;", a, s, i.uimm16() << 16)); line(S("rt::cr0(ctx, r[%u]);", a)); break;
    case Op::AND: line(S("r[%u] = r[%u] & r[%u];", a, s, b)); rec0(a); break;
    case Op::ANDC: line(S("r[%u] = r[%u] & ~r[%u];", a, s, b)); rec0(a); break;
    case Op::OR:
        if (s == b) line(S("r[%u] = r[%u];", a, s));
        else line(S("r[%u] = r[%u] | r[%u];", a, s, b));
        rec0(a);
        break;
    case Op::ORC: line(S("r[%u] = r[%u] | ~r[%u];", a, s, b)); rec0(a); break;
    case Op::XOR: line(S("r[%u] = r[%u] ^ r[%u];", a, s, b)); rec0(a); break;
    case Op::NAND: line(S("r[%u] = ~(r[%u] & r[%u]);", a, s, b)); rec0(a); break;
    case Op::NOR: line(S("r[%u] = ~(r[%u] | r[%u]);", a, s, b)); rec0(a); break;
    case Op::EQV: line(S("r[%u] = ~(r[%u] ^ r[%u]);", a, s, b)); rec0(a); break;
    case Op::EXTSB: line(S("r[%u] = (u64)(s64)(s8)r[%u];", a, s)); rec0(a); break;
    case Op::EXTSH: line(S("r[%u] = (u64)(s64)(s16)r[%u];", a, s)); rec0(a); break;
    case Op::EXTSW: line(S("r[%u] = (u64)(s64)(s32)r[%u];", a, s)); rec0(a); break;
    case Op::CNTLZW: line(S("r[%u] = rt::clz32((u32)r[%u]);", a, s)); rec0(a); break;
    case Op::CNTLZD: line(S("r[%u] = rt::clz64(r[%u]);", a, s)); rec0(a); break;

    // ------------------------------------------------------------ rotate / shift
    case Op::RLWINM:
    case Op::RLWNM:
    case Op::RLWIMI: {
        const u32 mb = i.mb32(), me = i.me32();
        const u64 m = mask64(mb + 32, me + 32);
        const std::string rot = op == Op::RLWNM ? S("rt::rotl32((u32)r[%u], (u32)r[%u] & 31)", s, b)
                                : i.sh32() == 0 ? S("(u32)r[%u]", s)
                                                : S("rt::rotl32((u32)r[%u], %u)", s, i.sh32());
        std::string val;
        if (mb <= me) val = S("(u64)(%s & 0x%08Xu)", rot.c_str(), static_cast<u32>(m));
        else val = S("(rt::dup32(%s) & %s)", rot.c_str(), hex64(m).c_str());
        if (op == Op::RLWIMI) line(S("r[%u] = %s | (r[%u] & %s);", a, val.c_str(), a, hex64(~m).c_str()));
        else line(S("r[%u] = %s;", a, val.c_str()));
        rec0(a);
        break;
    }
    case Op::RLDICL:
    case Op::RLDICR:
    case Op::RLDIC:
    case Op::RLDIMI: {
        const u32 sh = i.sh64(), mbe = i.mbe64();
        u64 m;
        if (op == Op::RLDICL) m = mask64(mbe, 63);
        else if (op == Op::RLDICR) m = mask64(0, mbe);
        else m = mask64(mbe, 63 - sh);
        const std::string rot = sh ? S("rt::rotl64(r[%u], %u)", s, sh) : S("r[%u]", s);
        if (op == Op::RLDIMI) line(S("r[%u] = (%s & %s) | (r[%u] & %s);", a, rot.c_str(), hex64(m).c_str(), a, hex64(~m).c_str()));
        else line(S("r[%u] = %s & %s;", a, rot.c_str(), hex64(m).c_str()));
        rec0(a);
        break;
    }
    case Op::RLDCL:
    case Op::RLDCR: {
        const u64 m = op == Op::RLDCL ? mask64(i.mbe64(), 63) : mask64(0, i.mbe64());
        line(S("r[%u] = rt::rotl64(r[%u], (u32)r[%u] & 63) & %s;", a, s, b, hex64(m).c_str()));
        rec0(a);
        break;
    }
    case Op::SLW: line(S("r[%u] = rt::slw(r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SRW: line(S("r[%u] = rt::srw(r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SLD: line(S("r[%u] = rt::sld(r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SRD: line(S("r[%u] = rt::srd(r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SRAW: line(S("r[%u] = rt::sraw(ctx, r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SRAD: line(S("r[%u] = rt::srad(ctx, r[%u], r[%u]);", a, s, b)); rec0(a); break;
    case Op::SRAWI: {
        const u32 sh = i.sh32();
        line(S("{ const s32 v_ = (s32)r[%u]; ctx.xer_ca = (u8)(v_ < 0 && (((u32)v_) & 0x%08Xu) != 0); r[%u] = (u64)(s64)(v_ >> %u); }",
               s, sh ? ((1u << sh) - 1) : 0u, a, sh));
        rec0(a);
        break;
    }
    case Op::SRADI: {
        const u32 sh = i.sh64();
        const u64 m = sh ? ((u64(1) << sh) - 1) : 0;
        line(S("{ const s64 v_ = (s64)r[%u]; ctx.xer_ca = (u8)(v_ < 0 && (((u64)v_) & %s) != 0); r[%u] = (u64)(v_ >> %u); }",
               s, hex64(m).c_str(), a, sh));
        rec0(a);
        break;
    }

    // ------------------------------------------------------------ integer loads
#define LOAD_D(OPC, EXPR)                                                     \
    case Op::OPC: line(S("r[%u] = " EXPR ";", d, ea_d(i, i.simm16()).c_str())); break;
#define LOAD_DU(OPC, EXPR)                                                                        \
    case Op::OPC:                                                                                 \
        line(S("{ const u64 ea_ = r[%u] + %s; r[%u] = " EXPR "; r[%u] = ea_; }", a, simm(i.simm16()).c_str(), d, \
               "(u32)ea_", a));                                                                   \
        break;
#define LOAD_X(OPC, EXPR) \
    case Op::OPC: line(S("r[%u] = " EXPR ";", d, ea_x(i).c_str())); break;
#define LOAD_XU(OPC, EXPR)                                                                              \
    case Op::OPC:                                                                                       \
        line(S("{ const u64 ea_ = r[%u] + r[%u]; r[%u] = " EXPR "; r[%u] = ea_; }", a, b, d, "(u32)ea_", a)); \
        break;

    LOAD_D(LBZ, "mem::r8(%s)")
    LOAD_D(LHZ, "mem::r16(%s)")
    LOAD_D(LHA, "(u64)(s64)(s16)mem::r16(%s)")
    LOAD_D(LWZ, "mem::r32(%s)")
    LOAD_DU(LBZU, "mem::r8(%s)")
    LOAD_DU(LHZU, "mem::r16(%s)")
    LOAD_DU(LHAU, "(u64)(s64)(s16)mem::r16(%s)")
    LOAD_DU(LWZU, "mem::r32(%s)")
    LOAD_X(LBZX, "mem::r8(%s)")
    LOAD_X(LHZX, "mem::r16(%s)")
    LOAD_X(LHAX, "(u64)(s64)(s16)mem::r16(%s)")
    LOAD_X(LWZX, "mem::r32(%s)")
    LOAD_X(LWAX, "(u64)(s64)(s32)mem::r32(%s)")
    LOAD_X(LDX, "mem::r64(%s)")
    LOAD_X(LHBRX, "bswap16(mem::r16(%s))")
    LOAD_X(LWBRX, "bswap32(mem::r32(%s))")
    LOAD_XU(LBZUX, "mem::r8(%s)")
    LOAD_XU(LHZUX, "mem::r16(%s)")
    LOAD_XU(LHAUX, "(u64)(s64)(s16)mem::r16(%s)")
    LOAD_XU(LWZUX, "mem::r32(%s)")
    LOAD_XU(LWAUX, "(u64)(s64)(s32)mem::r32(%s)")
    LOAD_XU(LDUX, "mem::r64(%s)")
    case Op::LD: line(S("r[%u] = mem::r64(%s);", d, ea_d(i, i.ds()).c_str())); break;
    case Op::LWA: line(S("r[%u] = (u64)(s64)(s32)mem::r32(%s);", d, ea_d(i, i.ds()).c_str())); break;
    case Op::LDU: line(S("{ const u64 ea_ = r[%u] + %s; r[%u] = mem::r64((u32)ea_); r[%u] = ea_; }", a, simm(i.ds()).c_str(), d, a)); break;
    case Op::LMW: {
        line(S("{ const u32 ea_ = %s;", ea_d(i, i.simm16()).c_str()));
        for (u32 k = d; k < 32; ++k) line(S("  r[%u] = mem::r32(ea_ + %u);", k, 4 * (k - d)));
        line("}");
        break;
    }
    case Op::LSWI: line(S("rt::lswi(ctx, %u, %s, %u);", d, a ? S("(u32)r[%u]", a).c_str() : "0u", b)); break;
    case Op::LSWX: line(S("if (ctx.xer_cnt) rt::lswi(ctx, %u, %s, ctx.xer_cnt);", d, ea_x(i).c_str())); break;
    case Op::LWARX: line(S("r[%u] = rt::lwarx(ctx, %s);", d, ea_x(i).c_str())); break;
    case Op::LDARX: line(S("r[%u] = rt::ldarx(ctx, %s);", d, ea_x(i).c_str())); break;
#undef LOAD_D
#undef LOAD_DU
#undef LOAD_X
#undef LOAD_XU

    // ------------------------------------------------------------ integer stores
#define STORE_D(OPC, FN, CAST) \
    case Op::OPC: line(S("mem::" FN "(%s, " CAST "r[%u]);", ea_d(i, i.simm16()).c_str(), s)); break;
#define STORE_DU(OPC, FN, CAST)                                                                                 \
    case Op::OPC:                                                                                              \
        line(S("{ const u64 ea_ = r[%u] + %s; mem::" FN "((u32)ea_, " CAST "r[%u]); r[%u] = ea_; }", a, simm(i.simm16()).c_str(), s, a)); \
        break;
#define STORE_X(OPC, FN, CAST) \
    case Op::OPC: line(S("mem::" FN "(%s, " CAST "r[%u]);", ea_x(i).c_str(), s)); break;
#define STORE_XU(OPC, FN, CAST)                                                                          \
    case Op::OPC:                                                                                        \
        line(S("{ const u64 ea_ = r[%u] + r[%u]; mem::" FN "((u32)ea_, " CAST "r[%u]); r[%u] = ea_; }", a, b, s, a)); \
        break;

    STORE_D(STB, "w8", "(u8)")
    STORE_D(STH, "w16", "(u16)")
    STORE_D(STW, "w32", "(u32)")
    STORE_DU(STBU, "w8", "(u8)")
    STORE_DU(STHU, "w16", "(u16)")
    STORE_DU(STWU, "w32", "(u32)")
    STORE_X(STBX, "w8", "(u8)")
    STORE_X(STHX, "w16", "(u16)")
    STORE_X(STWX, "w32", "(u32)")
    STORE_X(STDX, "w64", "")
    STORE_XU(STBUX, "w8", "(u8)")
    STORE_XU(STHUX, "w16", "(u16)")
    STORE_XU(STWUX, "w32", "(u32)")
    STORE_XU(STDUX, "w64", "")
#undef STORE_D
#undef STORE_DU
#undef STORE_X
#undef STORE_XU
    case Op::STD: line(S("mem::w64(%s, r[%u]);", ea_d(i, i.ds()).c_str(), s)); break;
    case Op::STDU: line(S("{ const u64 ea_ = r[%u] + %s; mem::w64((u32)ea_, r[%u]); r[%u] = ea_; }", a, simm(i.ds()).c_str(), s, a)); break;
    case Op::STHBRX: line(S("mem::w16(%s, bswap16((u16)r[%u]));", ea_x(i).c_str(), s)); break;
    case Op::STWBRX: line(S("mem::w32(%s, bswap32((u32)r[%u]));", ea_x(i).c_str(), s)); break;
    case Op::STMW: {
        line(S("{ const u32 ea_ = %s;", ea_d(i, i.simm16()).c_str()));
        for (u32 k = s; k < 32; ++k) line(S("  mem::w32(ea_ + %u, (u32)r[%u]);", 4 * (k - s), k));
        line("}");
        break;
    }
    case Op::STSWI: line(S("rt::stswi(ctx, %u, %s, %u);", s, a ? S("(u32)r[%u]", a).c_str() : "0u", b)); break;
    case Op::STSWX: line(S("if (ctx.xer_cnt) rt::stswi(ctx, %u, %s, ctx.xer_cnt);", s, ea_x(i).c_str())); break;
    case Op::STWCX: line(S("rt::stwcx(ctx, %s, (u32)r[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STDCX: line(S("rt::stdcx(ctx, %s, r[%u]);", ea_x(i).c_str(), s)); break;

    // ------------------------------------------------------------ SPRs / sync / cache
    case Op::MFSPR:
        switch (i.spr()) {
        case 1: line(S("r[%u] = ctx.get_xer();", d)); break;
        case 8: line(S("r[%u] = ctx.lr;", d)); break;
        case 9: line(S("r[%u] = ctx.ctr;", d)); break;
        case 256: line(S("r[%u] = ctx.vrsave;", d)); break;
        case 268: line(S("r[%u] = read_timebase();", d)); break;
        case 269: line(S("r[%u] = read_timebase() >> 32;", d)); break;
        default: line(S("r[%u] = 0; /* mfspr %u (unsupported) */", d, i.spr())); fallback(op); break;
        }
        break;
    case Op::MTSPR:
        switch (i.spr()) {
        case 1: line(S("ctx.set_xer(r[%u]);", s)); break;
        case 8: line(S("ctx.lr = r[%u];", s)); break;
        case 9: line(S("ctx.ctr = r[%u];", s)); break;
        case 256: line(S("ctx.vrsave = (u32)r[%u];", s)); break;
        default: line(S("/* mtspr %u, r%u (ignored) */", i.spr(), s)); fallback(op); break;
        }
        break;
    case Op::MFTB:
        if (i.spr() == 269) line(S("r[%u] = read_timebase() >> 32;", d));
        else line(S("r[%u] = read_timebase();", d));
        break;
    case Op::MFMSR: line(S("r[%u] = 0x8000000000000000ull; /* MSR.SF */", d)); fallback(op); break;
    case Op::MTMSR:
    case Op::MTMSRD:
    case Op::ECIWX:
    case Op::ECOWX:
    case Op::DCBI: line(S("/* %s: privileged, ignored */", ppu::op_name(op))); fallback(op); break;
    case Op::SYNC:
    case Op::EIEIO: line("std::atomic_thread_fence(std::memory_order_seq_cst);"); break;
    case Op::ISYNC: line("/* isync */"); break;
    case Op::DCBF:
    case Op::DCBST:
    case Op::DCBT:
    case Op::DCBTST:
    case Op::ICBI: line(S("/* %s */", lower_name(op).c_str())); break;
    case Op::DCBZ: line(S("rt::dcbz(%s);", ea_x(i).c_str())); break;

    // ------------------------------------------------------------ FP loads / stores
    case Op::LFS: line(S("f[%u] = (f64)mem::rf32(%s);", d, ea_d(i, i.simm16()).c_str())); break;
    case Op::LFD: line(S("ctx.set_fpr_bits(%u, mem::r64(%s));", d, ea_d(i, i.simm16()).c_str())); break;
    case Op::LFSX: line(S("f[%u] = (f64)mem::rf32(%s);", d, ea_x(i).c_str())); break;
    case Op::LFDX: line(S("ctx.set_fpr_bits(%u, mem::r64(%s));", d, ea_x(i).c_str())); break;
    case Op::LFSU: line(S("{ const u64 ea_ = r[%u] + %s; f[%u] = (f64)mem::rf32((u32)ea_); r[%u] = ea_; }", a, simm(i.simm16()).c_str(), d, a)); break;
    case Op::LFDU: line(S("{ const u64 ea_ = r[%u] + %s; ctx.set_fpr_bits(%u, mem::r64((u32)ea_)); r[%u] = ea_; }", a, simm(i.simm16()).c_str(), d, a)); break;
    case Op::LFSUX: line(S("{ const u64 ea_ = r[%u] + r[%u]; f[%u] = (f64)mem::rf32((u32)ea_); r[%u] = ea_; }", a, b, d, a)); break;
    case Op::LFDUX: line(S("{ const u64 ea_ = r[%u] + r[%u]; ctx.set_fpr_bits(%u, mem::r64((u32)ea_)); r[%u] = ea_; }", a, b, d, a)); break;
    case Op::STFS: line(S("mem::wf32(%s, (f32)f[%u]);", ea_d(i, i.simm16()).c_str(), s)); break;
    case Op::STFD: line(S("mem::w64(%s, ctx.fpr_bits(%u));", ea_d(i, i.simm16()).c_str(), s)); break;
    case Op::STFSX: line(S("mem::wf32(%s, (f32)f[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STFDX: line(S("mem::w64(%s, ctx.fpr_bits(%u));", ea_x(i).c_str(), s)); break;
    case Op::STFIWX: line(S("mem::w32(%s, (u32)ctx.fpr_bits(%u));", ea_x(i).c_str(), s)); break;
    case Op::STFSU: line(S("{ const u64 ea_ = r[%u] + %s; mem::wf32((u32)ea_, (f32)f[%u]); r[%u] = ea_; }", a, simm(i.simm16()).c_str(), s, a)); break;
    case Op::STFDU: line(S("{ const u64 ea_ = r[%u] + %s; mem::w64((u32)ea_, ctx.fpr_bits(%u)); r[%u] = ea_; }", a, simm(i.simm16()).c_str(), s, a)); break;
    case Op::STFSUX: line(S("{ const u64 ea_ = r[%u] + r[%u]; mem::wf32((u32)ea_, (f32)f[%u]); r[%u] = ea_; }", a, b, s, a)); break;
    case Op::STFDUX: line(S("{ const u64 ea_ = r[%u] + r[%u]; mem::w64((u32)ea_, ctx.fpr_bits(%u)); r[%u] = ea_; }", a, b, s, a)); break;

    // ------------------------------------------------------------ FP arithmetic
    case Op::FADD: line(S("f[%u] = f[%u] + f[%u];", d, a, b)); frec(); break;
    case Op::FSUB: line(S("f[%u] = f[%u] - f[%u];", d, a, b)); frec(); break;
    case Op::FMUL: line(S("f[%u] = f[%u] * f[%u];", d, a, i.rc_reg())); frec(); break;
    case Op::FDIV: line(S("f[%u] = f[%u] / f[%u];", d, a, b)); frec(); break;
    case Op::FADDS: line(S("f[%u] = rt::to_single(f[%u] + f[%u]);", d, a, b)); frec(); break;
    case Op::FSUBS: line(S("f[%u] = rt::to_single(f[%u] - f[%u]);", d, a, b)); frec(); break;
    case Op::FMULS: line(S("f[%u] = rt::to_single(f[%u] * f[%u]);", d, a, i.rc_reg())); frec(); break;
    case Op::FDIVS: line(S("f[%u] = rt::to_single(f[%u] / f[%u]);", d, a, b)); frec(); break;
    case Op::FSQRT: line(S("f[%u] = std::sqrt(f[%u]);", d, b)); frec(); break;
    case Op::FSQRTS: line(S("f[%u] = rt::to_single(std::sqrt(f[%u]));", d, b)); frec(); break;
    case Op::FRES: line(S("f[%u] = rt::to_single(1.0 / f[%u]);", d, b)); frec(); break;
    case Op::FRSQRTE: line(S("f[%u] = 1.0 / std::sqrt(f[%u]);", d, b)); frec(); break;
    case Op::FMADD: line(S("f[%u] = std::fma(f[%u], f[%u], f[%u]);", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FMSUB: line(S("f[%u] = std::fma(f[%u], f[%u], -f[%u]);", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FNMADD: line(S("f[%u] = -std::fma(f[%u], f[%u], f[%u]);", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FNMSUB: line(S("f[%u] = -std::fma(f[%u], f[%u], -f[%u]);", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FMADDS: line(S("f[%u] = rt::to_single(std::fma(f[%u], f[%u], f[%u]));", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FMSUBS: line(S("f[%u] = rt::to_single(std::fma(f[%u], f[%u], -f[%u]));", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FNMADDS: line(S("f[%u] = rt::to_single(-std::fma(f[%u], f[%u], f[%u]));", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FNMSUBS: line(S("f[%u] = rt::to_single(-std::fma(f[%u], f[%u], -f[%u]));", d, a, i.rc_reg(), b)); frec(); break;
    case Op::FSEL: line(S("f[%u] = rt::fsel(f[%u], f[%u], f[%u]);", d, a, b, i.rc_reg())); frec(); break;
    case Op::FMR: line(S("f[%u] = f[%u];", d, b)); frec(); break;
    case Op::FNEG: line(S("f[%u] = -f[%u];", d, b)); frec(); break;
    case Op::FABS: line(S("f[%u] = std::fabs(f[%u]);", d, b)); frec(); break;
    case Op::FNABS: line(S("f[%u] = -std::fabs(f[%u]);", d, b)); frec(); break;
    case Op::FRSP: line(S("f[%u] = rt::to_single(f[%u]);", d, b)); frec(); break;
    case Op::FCTIW: line(S("ctx.set_fpr_bits(%u, rt::fctiw(f[%u], false));", d, b)); frec(); break;
    case Op::FCTIWZ: line(S("ctx.set_fpr_bits(%u, rt::fctiw(f[%u], true));", d, b)); frec(); break;
    case Op::FCTID: line(S("ctx.set_fpr_bits(%u, rt::fctid(f[%u], false));", d, b)); frec(); break;
    case Op::FCTIDZ: line(S("ctx.set_fpr_bits(%u, rt::fctid(f[%u], true));", d, b)); frec(); break;
    case Op::FCFID: line(S("f[%u] = rt::fcfid(ctx.fpr_bits(%u));", d, b)); frec(); break;
    case Op::FCMPU:
    case Op::FCMPO: line(S("rt::fcmp(ctx, %u, f[%u], f[%u]);", i.crfd(), a, b)); break;
    case Op::MFFS: line(S("ctx.set_fpr_bits(%u, (u64)ctx.fpscr);", d)); frec(); break;
    case Op::MTFSF: line(S("rt::mtfsf(ctx, 0x%02X, ctx.fpr_bits(%u));", i.flm(), b)); frec(); break;
    case Op::MTFSB0: line(S("ctx.fpscr &= ~0x%08Xu;", 1u << (31 - d))); frec(); break;
    case Op::MTFSB1: line(S("ctx.fpscr |= 0x%08Xu;", 1u << (31 - d))); frec(); break;
    case Op::MTFSFI:
        line(S("ctx.fpscr = (ctx.fpscr & ~0x%08Xu) | 0x%08Xu;", 0xF0000000u >> (4 * i.crfd()),
               i.imm_fpscr() << (28 - 4 * i.crfd())));
        frec();
        break;
    case Op::MCRFS:
        for (u32 k = 0; k < 4; ++k)
            line(S("cr[%u] = (u8)((ctx.fpscr >> %u) & 1);", i.crfd() * 4 + k, 31 - (i.crfs() * 4 + k)));
        break;

    // ------------------------------------------------------------ VMX memory
    case Op::LVX:
    case Op::LVXL:
    case Op::LVEBX:
    case Op::LVEHX:
    case Op::LVEWX: line(S("v[%u] = rt::lvx(%s);", d, ea_x(i).c_str())); break;
    case Op::LVLX:
    case Op::LVLXL: line(S("v[%u] = rt::lvlx(%s);", d, ea_x(i).c_str())); break;
    case Op::LVRX:
    case Op::LVRXL: line(S("v[%u] = rt::lvrx(%s);", d, ea_x(i).c_str())); break;
    case Op::LVSL: line(S("v[%u] = vmx::lvsl(%s);", d, ea_x(i).c_str())); break;
    case Op::LVSR: line(S("v[%u] = vmx::lvsr(%s);", d, ea_x(i).c_str())); break;
    case Op::STVX:
    case Op::STVXL: line(S("rt::stvx(%s, v[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STVEBX: line(S("rt::stvebx(%s, v[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STVEHX: line(S("rt::stvehx(%s, v[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STVEWX: line(S("rt::stvewx(%s, v[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STVLX:
    case Op::STVLXL: line(S("rt::stvlx(%s, v[%u]);", ea_x(i).c_str(), s)); break;
    case Op::STVRX:
    case Op::STVRXL: line(S("rt::stvrx(%s, v[%u]);", ea_x(i).c_str(), s)); break;

    // ------------------------------------------------------------ VMX special forms
    case Op::VSEL:
    case Op::VPERM:
    case Op::VMHADDSHS:
    case Op::VMHRADDSHS:
    case Op::VMLADDUHM:
    case Op::VMSUMUBM:
    case Op::VMSUMMBM:
    case Op::VMSUMUHM:
    case Op::VMSUMUHS:
    case Op::VMSUMSHM:
    case Op::VMSUMSHS:
    case Op::VMADDFP:
    case Op::VNMSUBFP:
        line(S("v[%u] = vmx::%s(v[%u], v[%u], v[%u]);", d, lower_name(op).c_str(), a, b, i.rc_reg()));
        break;
    case Op::VSLDOI: line(S("v[%u] = vmx::vsldoi(v[%u], v[%u], %u);", d, a, b, i.vsh())); break;
    case Op::VSPLTB:
    case Op::VSPLTH:
    case Op::VSPLTW:
    case Op::VCFUX:
    case Op::VCFSX:
    case Op::VCTUXS:
    case Op::VCTSXS: line(S("v[%u] = vmx::%s(v[%u], %u);", d, lower_name(op).c_str(), b, i.vuimm())); break;
    case Op::VSPLTISB:
    case Op::VSPLTISH:
    case Op::VSPLTISW: line(S("v[%u] = vmx::%s(%d);", d, lower_name(op).c_str(), i.vsimm())); break;
    case Op::VREFP:
    case Op::VRSQRTEFP:
    case Op::VEXPTEFP:
    case Op::VLOGEFP:
    case Op::VRFIN:
    case Op::VRFIZ:
    case Op::VRFIP:
    case Op::VRFIM:
    case Op::VUPKHSB:
    case Op::VUPKHSH:
    case Op::VUPKLSB:
    case Op::VUPKLSH:
    case Op::VUPKHPX:
    case Op::VUPKLPX: line(S("v[%u] = vmx::%s(v[%u]);", d, lower_name(op).c_str(), b)); break;
    case Op::MFVSCR: line(S("v[%u] = v128::zero(); v[%u].uw(3) = ctx.vscr;", d, d)); break;
    case Op::MTVSCR: line(S("ctx.vscr = v[%u].uw(3);", b)); break;

    case Op::INVALID:
        line(S("illegal_instruction(ctx, 0x%08X, 0x%08X);", pc, raw));
        break;

    default:
        if (op >= Op::VCMPBFP && op <= Op::VCMPGTUW) {
            line(S("v[%u] = vmx::%s(v[%u], v[%u]);", d, lower_name(op).c_str(), a, b));
            if (i.vrc()) line(S("vmx::%s(cr, v[%u]);", op == Op::VCMPBFP ? "cr6_bounds" : "cr6", d));
            break;
        }
        if (op >= Op::VADDUBM && op <= Op::VPKPX) {
            line(S("v[%u] = vmx::%s(v[%u], v[%u]);", d, lower_name(op).c_str(), a, b));
            break;
        }
        // Should be unreachable: every decoded op has an emitter above.
        line(S("illegal_instruction(ctx, 0x%08X, 0x%08X); /* no emitter for %s */", pc, raw, ppu::op_name(op)));
        fallback(op);
        break;
    }
}

bool falls_through(u32 raw) {
    const Insn i(raw);
    const Op op = ppu::decode(raw);
    if (op == Op::INVALID) return false;
    if (op == Op::B) return i.lk();
    if (op == Op::BCLR || op == Op::BCCTR) return i.lk() || (i.bo() & 0x14) != 0x14;
    if (op == Op::TW && i.rd() == 31 && i.ra() == 0 && i.rb() == 0) return false;
    return true;
}

std::string FunctionEmitter::run() {
    out_ += S("OPENTIT_FN(%08X) {\n", fn_.start);
    out_ += "    u64* const r = ctx.gpr; f64* const f = ctx.fpr; v128* const v = ctx.vr; u8* const cr = ctx.cr;\n";
    out_ += "    (void)r; (void)f; (void)v; (void)cr;\n";

    // Bodies are emitted in address order, but the entry is not always the
    // lowest address (e.g. an entry stub that branches back into shared code
    // placed before it). In that case jump to the entry first.
    const bool entry_not_first = !fn_.insns.empty() && fn_.insns.front() != fn_.start;
    const bool start_is_target = fn_.labels.count(fn_.start) != 0 || entry_not_first;
    if (entry_not_first) out_ += S("    goto L_%08X;\n", fn_.start);
    for (std::size_t k = 0; k < fn_.insns.size(); ++k) {
        const u32 pc = fn_.insns[k];
        const u32 raw = elf_.read_u32(pc);
        if (fn_.labels.count(pc) || (pc == fn_.start && start_is_target)) out_ += S("L_%08X:;\n", pc);
        if (annotate_) out_ += S("    // %08X: %s\n", pc, ppu::disasm(raw, pc).c_str());
        emit_insn(pc, raw);
        if (stats_) stats_->instructions++;

        if (falls_through(raw)) {
            const bool next_emitted = k + 1 < fn_.insns.size() && fn_.insns[k + 1] == pc + 4;
            if (!next_emitted) {
                // Fallthrough into another function (or off the end of code).
                if (is_function(pc + 4)) line("return " + fname(pc + 4) + "(ctx);");
                else line(S("return call_indirect(ctx, 0x%08X, 0x%08X);", pc + 4, pc));
            }
        }
    }
    out_ += "}\n\n";
    return out_;
}

}  // namespace

// ---------------------------------------------------------------- GPR caching
// Rewrites a function so its GPRs live in C++ locals (r[N] -> rN). Guest
// memory stores go through byte pointers that might alias ctx.gpr, so with
// the registers in ctx the compiler reloads them after every store; locals
// can't alias. ctx.gpr is brought up to date before anything that can look
// at it (calls, indirect branches, syscalls, traps, lswi/stswi, returns) and
// reloaded after calls, so callees see and change registers as usual.
namespace {

bool word_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// r[N] occurrences: replace with rN; collect used/written registers.
std::string rename_gprs(const std::string& in, u32* used, u32* written) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t k = 0; k < in.size(); ++k) {
        if (in[k] == 'r' && k + 2 < in.size() && in[k + 1] == '[' && (k == 0 || !word_char(in[k - 1])) &&
            std::isdigit(static_cast<unsigned char>(in[k + 2]))) {
            std::size_t e = k + 2;
            u32 n = 0;
            while (e < in.size() && std::isdigit(static_cast<unsigned char>(in[e]))) n = n * 10 + static_cast<u32>(in[e++] - '0');
            if (e < in.size() && in[e] == ']' && n < 32) {
                *used |= 1u << n;
                std::size_t a = e + 1;
                while (a < in.size() && in[a] == ' ') ++a;
                if (a + 1 < in.size() && in[a] == '=' && in[a + 1] != '=') *written |= 1u << n;
                out += "r" + std::to_string(n);
                k = e;
                continue;
            }
        }
        out += in[k];
    }
    return out;
}

bool has_call(const std::string& l) {
    static const char* const kEscapes[] = {"call_indirect(ctx", "ppu_syscall(ctx", "trap(ctx", "illegal_instruction(ctx",
                                           "rt::lswi(ctx", "rt::stswi(ctx"};
    for (const char* e : kEscapes)
        if (l.find(e) != std::string::npos) return true;
    // direct calls: f_XXXXXXXX(ctx)
    for (std::size_t p = l.find("f_"); p != std::string::npos; p = l.find("f_", p + 2))
        if ((p == 0 || !word_char(l[p - 1])) && p + 15 <= l.size() && l.compare(p + 10, 5, "(ctx)") == 0) return true;
    return false;
}

bool has_return(const std::string& l) {
    for (std::size_t p = l.find("return"); p != std::string::npos; p = l.find("return", p + 6))
        if ((p == 0 || !word_char(l[p - 1])) && (p + 6 >= l.size() || !word_char(l[p + 6]))) return true;
    return false;
}

std::string cache_gprs(const std::string& fn_text) {
    u32 used = 0, written = 0;
    std::string body = rename_gprs(fn_text, &used, &written);
    std::string store, reload;
    for (u32 n = 0; n < 32; ++n) {
        if (written & (1u << n)) store += "ctx.gpr[" + std::to_string(n) + "] = r" + std::to_string(n) + "; ";
        if (used & (1u << n)) reload += "r" + std::to_string(n) + " = ctx.gpr[" + std::to_string(n) + "]; ";
    }
    std::string decl = "    u64 ";
    bool first = true;
    for (u32 n = 0; n < 32; ++n) {
        if (!(used & (1u << n))) continue;
        decl += (first ? "" : ", ") + std::string("r") + std::to_string(n) + " = ctx.gpr[" + std::to_string(n) + "]";
        first = false;
    }
    decl += ";\n";
    std::string out;
    out.reserve(body.size() + body.size() / 4);
    std::size_t at = 0;
    bool header_done = false;
    while (at < body.size()) {
        const std::size_t nl = body.find('\n', at);
        const std::string l = body.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
        at = nl == std::string::npos ? body.size() : nl + 1;
        if (!header_done) {
            out += l + "\n";
            if (l.rfind("OPENTIT_FN(", 0) == 0 || l.rfind("OPENTIT_FN_RECOMP(", 0) == 0) {
                if (used) out += decl;
                header_done = true;
            }
            continue;
        }
        if (l.find("u64* const r = ctx.gpr;") != std::string::npos) {
            std::string m = l;
            m.replace(m.find("u64* const r = ctx.gpr; "), 24, "");
            out += m + "\n";
            continue;
        }
        if (l.find("(void)r; ") != std::string::npos) {
            std::string m = l;
            m.replace(m.find("(void)r; "), 9, "");
            out += m + "\n";
            continue;
        }
        const bool call = has_call(l), ret = has_return(l);
        if ((call || ret) && !store.empty()) out += "    { " + store + "}\n";
        out += l + "\n";
        if (call && !reload.empty()) out += "    { " + reload + "}\n";
    }
    return out;
}

}  // namespace

std::set<u32> scan_decomp_sources(const std::string& dir, std::string* error) {
    namespace fs = std::filesystem;
    std::set<u32> out;
    std::error_code ec;
    if (!fs::exists(dir, ec)) {
        if (error) *error = "decomp folder not found: " + dir;
        return out;
    }
    for (const auto& e : fs::recursive_directory_iterator(dir, ec)) {
        const auto ext = e.path().extension().string();
        if (!e.is_regular_file() || (ext != ".cpp" && ext != ".cc")) continue;
        std::ifstream in(e.path(), std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        const std::string t = ss.str();
        // DECOMP(XXXXXXXX, name) at the start of a line
        for (std::size_t p = t.find("DECOMP("); p != std::string::npos; p = t.find("DECOMP(", p + 7)) {
            if (p > 0 && t[p - 1] != '\n') continue;
            std::size_t q = p + 7;
            while (q < t.size() && t[q] == ' ') ++q;
            u32 v = 0;
            int digits = 0;
            while (q < t.size() && std::isxdigit(static_cast<unsigned char>(t[q])) && digits < 8) {
                v = v * 16 + static_cast<u32>(std::isdigit(static_cast<unsigned char>(t[q])) ? t[q] - '0' : (std::tolower(t[q]) - 'a' + 10));
                ++q;
                ++digits;
            }
            if (digits == 8) out.insert(v);
        }
    }
    return out;
}

std::string emit_function(const loader::Elf& elf, const analysis::Program& prog, const analysis::Function& fn,
                          bool annotate, std::map<u32, bool>* referenced, EmitStats* stats, bool cache) {
    FunctionEmitter e(elf, prog, fn, annotate, referenced, stats);
    return cache ? cache_gprs(e.run()) : e.run();
}

namespace {

const char* kFileHeader =
    "// Generated by opentit-recomp from your own EBOOT.elf.\n"
    "// Contains code derived from the game binary: keep it local, never commit it.\n"
    "#include \"opentit/recomp.h\"\n"
    "#include <atomic>\n"
    "#if defined(__clang__) || defined(__GNUC__)\n"
    "#pragma GCC diagnostic ignored \"-Wunused-label\"\n"
    "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n"
    "#endif\n"
    "using namespace opentit;\n\n";

// Write only when the content changed, so re-running the recompiler doesn't
// force a full rebuild of ~200 large translation units.
bool write_if_changed(const std::string& path, const std::string& content) {
    {
        std::ifstream in(path, std::ios::binary);
        if (in) {
            std::string old((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (old == content) return true;
        }
    }
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << content;
    return static_cast<bool>(f);
}

}  // namespace

bool emit_program(const loader::Elf& elf, const analysis::Program& prog, const EmitOptions& opts, EmitStats* stats,
                  std::string* error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(opts.out_dir, ec);
    if (ec) {
        if (error) *error = "cannot create " + opts.out_dir + ": " + ec.message();
        return false;
    }
    std::set<std::string> written;

    std::size_t file_index = 0;
    std::string body;
    std::map<u32, bool> referenced;
    std::size_t insns_in_file = 0;

    auto flush = [&]() -> bool {
        if (body.empty()) return true;
        std::string decls;
        // Forward-declare everything referenced (including functions defined
        // later in this same file).
        for (const auto& [addr, _] : referenced) decls += S("OPENTIT_FN(%08X);\n", addr);
        const std::string path = S("%s/ppu_%s%04zu.cpp", opts.out_dir.c_str(),
                                   opts.module_name.empty() ? "" : (opts.module_name + "_").c_str(), file_index++);
        if (!write_if_changed(path, std::string(kFileHeader) + decls + "\n" + body)) {
            if (error) *error = "cannot write " + path;
            return false;
        }
        written.insert(fs::path(path).filename().string());
        body.clear();
        referenced.clear();
        insns_in_file = 0;
        if (stats) stats->files++;
        return true;
    };

    for (const auto& [addr, fn] : prog.functions) {
        std::string text = emit_function(elf, prog, fn, opts.annotate, &referenced, stats, opts.cache_gprs);
        if (opts.decompiled.count(addr)) {
            // Keep the original for A/B testing; the decomp source defines f_XXXXXXXX.
            const std::string head = S("OPENTIT_FN(%08X) {", addr);
            if (text.compare(0, head.size(), head) == 0)
                text.replace(0, head.size(), S("OPENTIT_FN_RECOMP(%08X) {", addr));
        }
        if (auto it = opts.names.find(addr); it != opts.names.end()) text = "// " + it->second + "\n" + text;
        body += text;
        insns_in_file += fn.insns.size();
        if (stats) stats->functions++;
        if (insns_in_file >= opts.insns_per_file && !flush()) return false;
    }
    if (!flush()) return false;

    // Function table used by the runtime's indirect-call dispatcher.
    {
        const std::string table_name =
            opts.module_name.empty() ? std::string("ppu_table.cpp") : "ppu_" + opts.module_name + "_table.cpp";
        const std::string path = opts.out_dir + "/" + table_name;
        std::string t = kFileHeader;
        for (const auto& [addr, _] : prog.functions) t += S("OPENTIT_FN(%08X);\n", addr);
        if (opts.module_name.empty()) {
            t += "\nnamespace opentit {\nconst FunctionEntry g_recompiled_functions[] = {\n";
            for (const auto& [addr, _] : prog.functions) t += S("    {0x%08X, f_%08X},\n", addr, addr);
            t += "};\nconst std::size_t g_recompiled_function_count = sizeof(g_recompiled_functions) / "
                 "sizeof(g_recompiled_functions[0]);\n}  // namespace opentit\n";
        } else {
            t += "\nnamespace {\nconst ::opentit::FunctionEntry kFunctions[] = {\n";
            for (const auto& [addr, _] : prog.functions) t += S("    {0x%08X, f_%08X},\n", addr, addr);
            t += S("};\nconst ::opentit::ModuleRegistrar kRegistrar(\"%s\", 0x%08X, kFunctions, "
                   "sizeof(kFunctions) / sizeof(kFunctions[0]));\n}  // namespace\n",
                   opts.module_name.c_str(), opts.module_base);
        }
        if (!write_if_changed(path, t)) {
            if (error) *error = "cannot write " + path;
            return false;
        }
        written.insert(table_name);
    }
    // Remove stale outputs left by an earlier, larger run.
    for (const auto& e : fs::directory_iterator(opts.out_dir, ec)) {
        const auto name = e.path().filename().string();
        if (name.rfind("ppu_", 0) == 0 && e.path().extension() == ".cpp" && !written.count(name)) fs::remove(e.path(), ec);
    }
    return true;
}

}  // namespace opentit::recomp
