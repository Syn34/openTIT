#include <set>
#include "analysis/functions.h"

#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>

#include "loader/nid.h"
#include "ppu/ppu_decoder.h"

namespace opentit::analysis {

using ppu::Insn;
using ppu::Op;

namespace {

// Destination GPR written by an instruction (-1 if none / not tracked).
int dest_gpr(Op op, const Insn i) {
    switch (op) {
    case Op::ORI: case Op::ORIS: case Op::XORI: case Op::XORIS: case Op::ANDI_: case Op::ANDIS_:
    case Op::AND: case Op::ANDC: case Op::NOR: case Op::EQV: case Op::XOR: case Op::ORC: case Op::OR:
    case Op::NAND: case Op::EXTSB: case Op::EXTSH: case Op::EXTSW: case Op::CNTLZW: case Op::CNTLZD:
    case Op::RLWIMI: case Op::RLWINM: case Op::RLWNM: case Op::RLDICL: case Op::RLDICR: case Op::RLDIC:
    case Op::RLDIMI: case Op::RLDCL: case Op::RLDCR: case Op::SLW: case Op::SRW: case Op::SRAW:
    case Op::SRAWI: case Op::SLD: case Op::SRD: case Op::SRAD: case Op::SRADI:
    case Op::STBU: case Op::STHU: case Op::STWU: case Op::STDU: case Op::STBUX: case Op::STHUX:
    case Op::STWUX: case Op::STDUX: case Op::STFSU: case Op::STFDU: case Op::STFSUX: case Op::STFDUX:
    case Op::LFSU: case Op::LFDU: case Op::LFSUX: case Op::LFDUX:
        return static_cast<int>(i.ra());
    case Op::CMP: case Op::CMPI: case Op::CMPL: case Op::CMPLI:
    case Op::STB: case Op::STH: case Op::STW: case Op::STD: case Op::STBX: case Op::STHX: case Op::STWX:
    case Op::STDX: case Op::MTSPR: case Op::MTCRF: case Op::MTOCRF: case Op::SC:
        return -1;
    default:
        break;
    }
    if (op >= Op::MULLI && op <= Op::DIVW) return static_cast<int>(i.rd());
    if (op >= Op::LBZ && op <= Op::LDARX) return static_cast<int>(i.rd());  // update forms also write ra (rare here)
    if (op == Op::MFSPR || op == Op::MFCR || op == Op::MFOCRF || op == Op::MFTB) return static_cast<int>(i.rd());
    return -1;
}

class Analyzer {
public:
    Analyzer(const loader::Elf& elf, const Options& opts) : elf_(elf), opts_(opts) {
        for (const auto& r : elf.exec_ranges()) ranges_.push_back(r);
    }

    Program run();

private:
    bool in_code(u32 a) const {
        for (const auto& r : ranges_)
            if (a >= r.start && a + 4 <= r.end) return true;
        return false;
    }
    u32 word(u32 a) const { return elf_.read_u32(a); }

    void add_seed(u32 a, u32 toc) {
        if (!in_code(a) || (a & 3)) return;
        if (starts_.emplace(a, toc).second) pending_.push_back(a);
    }

    Function walk(u32 start, u32 toc, bool discover);
    std::optional<JumpTable> resolve_jump_table(u32 bctr, u32 func_start, u32 toc) const;
    std::optional<u32> const_value(int reg, u32 before, u32 floor, u32 toc, int depth) const;
    std::optional<u32> unique_def_value(int reg, u32 func_start, u32 before, u32 toc) const;

    const loader::Elf& elf_;
    const Options& opts_;
    std::vector<loader::Elf::Range> ranges_;
    std::map<u32, u32> starts_;  // start -> toc
    std::deque<u32> pending_;
};

// Very small backward constant propagation, enough for the jump-table idiom
// `lwz rT, off(r2)` / `lis+addi` / `ld rT, off(r2)`.
std::optional<u32> Analyzer::const_value(int reg, u32 before, u32 floor, u32 toc, int depth) const {
    if (depth > 8) return std::nullopt;
    if (reg == 2) return toc;
    for (u32 a = before; a >= floor && a + 256 > before; a -= 4) {
        const u32 raw = word(a);
        const Insn i(raw);
        const Op op = ppu::decode(raw);
        const int dst = dest_gpr(op, i);
        if (ppu::is_branch(op)) {
            // On the fall-through path a definition above a conditional branch
            // still holds; a call keeps non-volatile registers (r2, r13-r31).
            const bool conditional = op == Op::BC && !i.lk();
            const bool call_keeps = i.lk() && (reg == 2 || reg >= 13);
            if (!conditional && !call_keeps) return std::nullopt;
            continue;
        }
        if (dst != reg) continue;
        const u32 prev = a - 4;
        switch (op) {
        case Op::ADDI:
            if (i.ra() == 0) return static_cast<u32>(i.simm16());
            if (auto b = const_value(static_cast<int>(i.ra()), prev, floor, toc, depth + 1))
                return *b + static_cast<u32>(i.simm16());
            return std::nullopt;
        case Op::ADDIS:
            if (i.ra() == 0) return static_cast<u32>(i.simm16()) << 16;
            if (auto b = const_value(static_cast<int>(i.ra()), prev, floor, toc, depth + 1))
                return *b + (static_cast<u32>(i.simm16()) << 16);
            return std::nullopt;
        case Op::ORI:
            if (auto b = const_value(static_cast<int>(i.rs()), prev, floor, toc, depth + 1)) return *b | i.uimm16();
            return std::nullopt;
        case Op::ORIS:
            if (auto b = const_value(static_cast<int>(i.rs()), prev, floor, toc, depth + 1)) return *b | (i.uimm16() << 16);
            return std::nullopt;
        case Op::OR:
            if (i.rs() == i.rb()) return const_value(static_cast<int>(i.rs()), prev, floor, toc, depth + 1);
            return std::nullopt;
        case Op::LWZ:
        case Op::LD: {
            auto b = i.ra() == 0 ? std::optional<u32>(0) : const_value(static_cast<int>(i.ra()), prev, floor, toc, depth + 1);
            if (!b) return std::nullopt;
            const s32 off = op == Op::LD ? i.ds() : i.simm16();
            const u32 ea = *b + static_cast<u32>(off);
            if (!elf_.ptr(ea, op == Op::LD ? 8 : 4)) return std::nullopt;
            return op == Op::LD ? elf_.read_u32(ea + 4) : elf_.read_u32(ea);
        }
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

// Fallback for loop-hoisted table bases: if `reg` is written exactly once
// between the function start and `before`, evaluate that definition.
std::optional<u32> Analyzer::unique_def_value(int reg, u32 func_start, u32 before, u32 toc) const {
    if (before - func_start > 0x8000) func_start = before - 0x8000;
    u32 def = 0;
    int defs = 0;
    for (u32 a = func_start; a < before; a += 4) {
        const Insn i(word(a));
        const Op op = ppu::decode(i.raw);
        if (dest_gpr(op, i) == reg) {
            def = a;
            if (++defs > 1) return std::nullopt;
        }
    }
    if (defs != 1) return std::nullopt;
    return const_value(reg, def, func_start, toc, 0);
}

// Recognises:
//     lwz   rT, off(r2)         (table base; any constant source works)
//     rldic/rlwinm rX, rI, 2    (index * 4)
//     lwzx  rY, rX, rT          (or lwzx rY, rT, rX)
//     extsw rY, rY              (optional)
//     add   rY, rY, rT          (entries relative to the table; optional)
//     mtctr rY
//     bctr
// The entry count comes from a preceding `cmplwi rI, N`; for tables placed
// inline after the bctr the count is additionally capped where code resumes.
std::optional<JumpTable> Analyzer::resolve_jump_table(u32 bctr, u32 func_start, u32 toc) const {
    const u32 floor = std::max(func_start, bctr >= 64 ? bctr - 64 : 0u);
    // mtctr rY
    int ry = -1;
    u32 a = bctr - 4;
    for (; a >= floor; a -= 4) {
        const Insn i(word(a));
        if (ppu::decode(i.raw) == Op::MTSPR && i.spr() == 9) {
            ry = static_cast<int>(i.rs());
            break;
        }
    }
    if (ry < 0) return std::nullopt;

    bool relative = false;
    int rel_base_reg = -1;
    int idx4_reg = -1, base_reg = -1;
    u32 load_at = 0;
    int add_a = -1, add_b = -1;  // operands of `add ry, a, b`: one is the loaded entry
    for (u32 b = a - 4; b >= floor; b -= 4) {
        const Insn i(word(b));
        const Op op = ppu::decode(i.raw);
        const auto is_value = [&](int r) { return add_a >= 0 ? (r == add_a || r == add_b) : r == ry; };
        if (op == Op::ADD && static_cast<int>(i.rd()) == ry && add_a < 0 && !relative) {
            relative = true;
            add_a = static_cast<int>(i.ra());
            add_b = static_cast<int>(i.rb());
            continue;
        }
        if (op == Op::EXTSW && is_value(static_cast<int>(i.ra()))) {
            const int dst = static_cast<int>(i.ra()), src = static_cast<int>(i.rs());
            if (add_a >= 0) {
                rel_base_reg = dst == add_a ? add_b : add_a;
                add_a = add_b = -1;
            }
            ry = src;
            continue;
        }
        if ((op == Op::LWZX || op == Op::LWAX) && is_value(static_cast<int>(i.rd()))) {
            if (add_a >= 0) rel_base_reg = static_cast<int>(i.rd()) == add_a ? add_b : add_a;
            load_at = b;
            idx4_reg = static_cast<int>(i.rb());
            base_reg = static_cast<int>(i.ra());
            break;
        }
        if (ppu::is_branch(op)) break;
    }
    if (!load_at) return std::nullopt;

    // Figure out which operand of lwzx is the constant table base.
    const u32 cfloor = std::max(func_start, bctr >= 512 ? bctr - 512 : 0u);
    std::optional<u32> base = const_value(base_reg, load_at - 4, cfloor, toc, 0);
    if (!base) {
        std::swap(base_reg, idx4_reg);
        base = const_value(base_reg, load_at - 4, cfloor, toc, 0);
    }
    if (!base) {
        // Try both operands again against a function-wide unique definition.
        base = unique_def_value(base_reg, func_start, load_at, toc);
        if (!base) {
            std::swap(base_reg, idx4_reg);
            base = unique_def_value(base_reg, func_start, load_at, toc);
        }
    }
    // Last resort for self-relative tables (target = table + entry) whose base
    // register was set far away or went through a stack spill: SNC places
    // such tables right after the bctr. Accepted only if, with a known bound,
    // every entry lands on aligned code after the end of the table.
    bool guessed_inline = false;
    if (!base && relative && rel_base_reg == base_reg) {
        base = bctr + 4;
        guessed_inline = true;
    } else if (!base && relative && rel_base_reg == idx4_reg) {
        std::swap(base_reg, idx4_reg);
        base = bctr + 4;
        guessed_inline = true;
    }
    if (!base) return std::nullopt;
    u32 rel_base = *base;
    if (relative && rel_base_reg != base_reg) {
        auto rb = const_value(rel_base_reg, load_at - 4, cfloor, toc, 0);
        if (!rb) rb = unique_def_value(rel_base_reg, func_start, load_at, toc);
        if (!rb) return std::nullopt;
        rel_base = *rb;
    }

    // Index register feeding idx4 (rldic/rlwinm rX, rI, 2, ...).
    int index_reg = -1;
    for (u32 b = load_at - 4; b >= floor; b -= 4) {
        const Insn i(word(b));
        const Op op = ppu::decode(i.raw);
        if ((op == Op::RLDIC || op == Op::RLWINM || op == Op::RLDICR) && static_cast<int>(i.ra()) == idx4_reg) {
            index_reg = static_cast<int>(i.rs());
            break;
        }
        if (ppu::is_branch(op)) break;
    }

    // Bound: nearest preceding cmplwi/cmpldi on the index register or a
    // register it was copied from (`mr rI, rJ` after the compare).
    u32 count = 0;
    const u32 search_floor = std::max(func_start, bctr >= 512 ? bctr - 512 : 0u);
    std::set<int> aliases;
    if (index_reg >= 0) aliases.insert(index_reg);
    for (u32 b = load_at - 4; b >= search_floor && !aliases.empty(); b -= 4) {  // copies in either direction
        const Insn i(word(b));
        if (ppu::decode(i.raw) == Op::OR && i.rs() == i.rb()) {
            if (aliases.count(static_cast<int>(i.ra()))) aliases.insert(static_cast<int>(i.rs()));
            else if (aliases.count(static_cast<int>(i.rs()))) aliases.insert(static_cast<int>(i.ra()));
        }
        if (ppu::is_branch(ppu::decode(i.raw)) && !(ppu::decode(i.raw) == Op::BC)) break;
    }
    for (u32 b = load_at - 4; b >= search_floor && !aliases.empty(); b -= 4) {
        const Insn i(word(b));
        if (ppu::decode(i.raw) == Op::CMPLI && aliases.count(static_cast<int>(i.ra()))) {
            count = i.uimm16() + 1;
            break;
        }
    }

    const u32 table = *base;
    const bool inline_table = table > bctr && table < bctr + 0x100000;
    if (count == 0 && !inline_table) return std::nullopt;
    if (guessed_inline) {
        if (count == 0 || count > 4096) return std::nullopt;
        const u32 table_end = table + 4 * count;
        for (u32 k = 0; k < count; ++k) {
            if (!elf_.ptr(table + 4 * k, 4)) return std::nullopt;
            const u32 off = elf_.read_u32(table + 4 * k);
            const u32 target = table + off;
            const bool near = static_cast<s32>(off) > -0x100000 && static_cast<s32>(off) < 0x100000;
            if (!near || !in_code(target) || (target & 3) || (target >= table && target < table_end))
                return std::nullopt;
        }
    }
    if (count == 0 || count > 4096) count = 4096;

    JumpTable jt;
    jt.bctr = bctr;
    jt.table = table;
    u32 min_target = 0xFFFFFFFF;
    for (u32 k = 0; k < count; ++k) {
        const u32 entry = table + 4 * k;
        if (inline_table && entry >= min_target) break;
        if (!elf_.ptr(entry, 4)) break;
        const u32 v = elf_.read_u32(entry);
        const u32 target = relative ? rel_base + v : v;
        if (!in_code(target) || (target & 3)) break;
        jt.targets.push_back(target);
        if (target > bctr) min_target = std::min(min_target, target);
    }
    if (jt.targets.empty()) return std::nullopt;
    return jt;
}

Function Analyzer::walk(u32 start, u32 toc, bool discover) {
    Function f;
    f.start = start;
    f.toc = toc;

    std::unordered_set<u32> seen;
    std::vector<u32> work{start};
    auto is_other_start = [&](u32 a) { return a != start && starts_.count(a); };

    while (!work.empty()) {
        u32 pc = work.back();
        work.pop_back();
        while (true) {
            if (!in_code(pc) || !seen.insert(pc).second) break;
            if (pc != start && is_other_start(pc)) {
                seen.erase(pc);  // belongs to the other function; fallthrough = tail call
                break;
            }
            f.insns.push_back(pc);
            const u32 raw = word(pc);
            const Insn i(raw);
            const Op op = ppu::decode(raw);
            if (op == Op::INVALID) {
                f.invalid.insert(pc);
                break;
            }
            const u32 next = pc + 4;

            if (op == Op::B || op == Op::BC) {
                const s32 disp = op == Op::B ? i.li() : i.bd();
                const u32 target = i.aa() ? static_cast<u32>(disp) : pc + static_cast<u32>(disp);
                const bool conditional = op == Op::BC && (i.bo() & 0x14) != 0x14;
                if (i.lk()) {
                    if (target != next) {
                        f.callees.insert(target);
                        if (discover) add_seed(target, toc);
                    }
                    pc = next;
                    continue;
                }
                if (is_other_start(target)) {
                    f.callees.insert(target);  // tail call
                } else if (in_code(target)) {
                    f.labels.insert(target);
                    work.push_back(target);
                }
                if (!conditional) break;
                pc = next;
                continue;
            }
            if (op == Op::BCLR) {
                if (i.lk() || (i.bo() & 0x14) != 0x14) {
                    pc = next;
                    continue;
                }
                break;  // blr
            }
            if (op == Op::BCCTR) {
                if (i.lk()) {
                    pc = next;
                    continue;
                }
                if (auto jt = resolve_jump_table(pc, start, toc)) {
                    for (u32 t : jt->targets) {
                        if (is_other_start(t)) {
                            f.callees.insert(t);
                        } else {
                            f.labels.insert(t);
                            work.push_back(t);
                        }
                    }
                    f.jump_tables.emplace(pc, std::move(*jt));
                } else {
                    f.unresolved_bctr.insert(pc);
                }
                if ((i.bo() & 0x14) != 0x14) {
                    pc = next;
                    continue;
                }
                break;
            }
            pc = next;
        }
    }
    std::sort(f.insns.begin(), f.insns.end());
    return f;
}

Program Analyzer::run() {
    Program prog;
    const auto entry = elf_.entry_opd();
    prog.toc = entry.toc;

    for (const auto& o : elf_.scan_opds()) add_seed(o.func, o.toc);
    add_seed(entry.func, entry.toc);
    for (const auto& im : elf_.imports()) {
        if (im.is_variable) continue;
        add_seed(im.initial, entry.toc);
        prog.import_stubs[im.initial] = im.library + "::" + loader::nid_display_name(im.library, im.nid);
    }
    for (u32 a : opts_.extra_entries) add_seed(a, entry.toc);
    // PRX modules: every exported function and the module start/stop entries.
    for (const auto& ex : elf_.exports())
        if (!ex.is_variable && elf_.ptr(ex.addr, 8)) add_seed(elf_.read_u32(ex.addr), elf_.read_u32(ex.addr + 4));
    for (u32 opd : {elf_.module_start_opd(), elf_.module_stop_opd()})
        if (opd && elf_.ptr(opd, 8)) add_seed(elf_.read_u32(opd), elf_.read_u32(opd + 4));

    // Phase 1: discover all function starts.
    std::size_t rounds = 0;
    while (!pending_.empty()) {
        const u32 s = pending_.front();
        pending_.pop_front();
        walk(s, starts_[s], /*discover=*/true);
        ++rounds;
    }
    if (opts_.verbose) std::fprintf(stderr, "analysis: %zu function starts after %zu walks\n", starts_.size(), rounds);

    // Phase 2: final bodies with the complete start set.
    for (const auto& [s, toc] : starts_) prog.functions.emplace(s, walk(s, toc, /*discover=*/false));
    return prog;
}

}  // namespace

std::size_t Program::total_insns() const {
    std::size_t n = 0;
    for (const auto& [_, f] : functions) n += f.insns.size();
    return n;
}

Program analyze(const loader::Elf& elf, const Options& opts) { return Analyzer(elf, opts).run(); }

std::vector<u32> read_address_list(const std::string& path, std::string* error) {
    std::vector<u32> out;
    std::ifstream f(path);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return out;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (auto h = line.find('#'); h != std::string::npos) line.resize(h);
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) out.push_back(static_cast<u32>(std::stoul(tok, nullptr, 16)));
    }
    return out;
}

}  // namespace opentit::analysis
