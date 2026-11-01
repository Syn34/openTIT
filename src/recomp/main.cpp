// opentit-recomp: analysis and static recompilation driver.
//
//   opentit-recomp info      EBOOT.elf
//   opentit-recomp imports   EBOOT.elf
//   opentit-recomp funcs     EBOOT.elf [--entries extra.txt] [--list]
//   opentit-recomp disasm    EBOOT.elf <hex addr> [count]
//   opentit-recomp recompile EBOOT.elf -o <dir> [--entries extra.txt] [--no-annotate] [--per-file N]
//   opentit-recomp sfo       PARAM.SFO
#include <set>
#include "spu/spu_analysis.h"
#include "recomp/spu_emitter.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "analysis/functions.h"
#include "loader/elf.h"
#include "loader/nid.h"
#include "loader/sfo.h"
#include "loader/spu_images.h"
#include "ppu/ppu_decoder.h"
#include "ppu/ppu_disasm.h"
#include "recomp/emitter.h"
#include "spu/spu_decoder.h"

using namespace opentit;

namespace {

int usage() {
    std::fprintf(stderr,
                 "opentit-recomp - Sly Cooper: Thieves in Time static recompiler\n\n"
                 "usage:\n"
                 "  opentit-recomp info      <EBOOT.elf>\n"
                 "  opentit-recomp imports   <EBOOT.elf>\n"
                 "  opentit-recomp funcs     <EBOOT.elf> [--entries file] [--list]\n"
                 "  opentit-recomp disasm    <EBOOT.elf> <hex addr> [count]\n"
                 "  opentit-recomp recompile <EBOOT.elf> -o <dir> [--entries file] [--no-annotate] [--no-gpr-cache] [--per-file N]\n"
                 "  opentit-recomp recompile <module.prx> -o <dir> [--base hex] [--name name]   (decrypted firmware PRX)\n"
                 "  opentit-recomp sfo       <PARAM.SFO>\n"
                 "  opentit-recomp spu-list   <EBOOT.elf>\n"
                 "  opentit-recomp spu-disasm <EBOOT.elf> <image #> <hex LS addr> [count]\n"
                 "  opentit-recomp spu-recompile <EBOOT.elf|*.prx|*.spurec ...> -o <dir>\n\n"
                 "EBOOT.elf must be the decrypted executable from your own disc dump\n"
                 "(PS3_GAME/USRDIR/EBOOT.BIN decrypted with RPCS3).\n");
    return 2;
}

// PRX modules (firmware libraries) are relocated to a fixed base before
// analysis; the runtime later loads the same file at the same base.
constexpr u32 kDefaultPrxBase = 0x01800000;
u32 g_prx_base = kDefaultPrxBase;
std::string g_prx_name;

bool is_prx_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    u8 h[18] = {};
    f.read(reinterpret_cast<char*>(h), sizeof(h));
    return f && std::memcmp(h, "\x7f" "ELF", 4) == 0 && h[16] == 0xFF && h[17] == 0xA4;
}

std::optional<loader::Elf> open_elf(const char* path) {
    std::string err;
    std::optional<loader::Elf> e;
    if (is_prx_file(path)) {
        e = loader::Elf::load_prx(path, g_prx_base, &err);
        if (e && g_prx_name.empty()) g_prx_name = std::filesystem::path(path).stem().string();
        if (e) std::fprintf(stderr, "PRX module '%s' (file name '%s') relocated to 0x%08X..0x%08X\n",
                            e->module_name().c_str(), g_prx_name.c_str(), g_prx_base, e->image_end());
    } else {
        e = loader::Elf::load(path, &err);
    }
    if (!e) std::fprintf(stderr, "error: %s\n", err.c_str());
    return e;
}

const char* seg_name(u32 t) {
    switch (t) {
    case loader::PT_LOAD: return "LOAD";
    case loader::PT_TLS: return "TLS";
    case loader::PT_PROC_PARAM: return "PROC_PARAM";
    case loader::PT_PROC_PRX_PARAM: return "PRX_PARAM";
    default: return "?";
    }
}

int cmd_info(const loader::Elf& e) {
    const auto opd = e.entry_opd();
    std::printf("entry descriptor 0x%08X -> code 0x%08X, toc 0x%08X\n", opd.addr, opd.func, opd.toc);
    std::printf("segments:\n");
    for (const auto& s : e.segments())
        std::printf("  %-10s vaddr 0x%08llX filesz 0x%08llX memsz 0x%08llX %c%c%c\n", seg_name(s.type),
                    (unsigned long long)s.vaddr, (unsigned long long)s.filesz, (unsigned long long)s.memsz,
                    (s.flags & 4) ? 'r' : '-', (s.flags & 2) ? 'w' : '-', (s.flags & 1) ? 'x' : '-');
    std::printf("code ranges:\n");
    u64 code = 0;
    for (const auto& r : e.exec_ranges()) {
        std::printf("  0x%08llX - 0x%08llX\n", (unsigned long long)r.start, (unsigned long long)r.end);
        code += r.end - r.start;
    }
    std::printf("  %llu instructions\n", (unsigned long long)(code / 4));
    const auto& pp = e.process_param();
    std::printf("process param: sdk 0x%08X, primary prio %d, primary stack 0x%X, malloc page 0x%X\n", pp.sdk_version,
                pp.primary_prio, pp.primary_stacksize, pp.malloc_pagesize);
    if (auto t = e.tls()) std::printf("TLS: vaddr 0x%08llX filesz 0x%llX memsz 0x%llX\n", (unsigned long long)t->vaddr,
                                      (unsigned long long)t->filesz, (unsigned long long)t->memsz);
    std::printf("function descriptors (.opd): %zu\n", e.scan_opds().size());
    std::printf("imports: %zu\n", e.imports().size());
    if (e.is_prx()) {
        std::printf("PRX module '%s': toc 0x%08X, module_start OPD 0x%08X, %zu exports\n", e.module_name().c_str(),
                    e.prx_toc(), e.module_start_opd(), e.exports().size());
        std::map<std::string, std::size_t> libs;
        for (const auto& x : e.exports()) libs[x.library]++;
        for (const auto& [lib, n] : libs) std::printf("  exports %-24s %zu\n", lib.c_str(), n);
    }
    return 0;
}

int cmd_imports(const loader::Elf& e) {
    std::map<std::string, std::vector<const loader::Import*>> by_lib;
    for (const auto& im : e.imports()) by_lib[im.library].push_back(&im);
    std::size_t named = 0;
    for (const auto& [lib, list] : by_lib) {
        std::printf("%s (%zu)\n", lib.c_str(), list.size());
        for (const auto* im : list) {
            const char* n = loader::nid_name(im->nid);
            named += n != nullptr;
            std::printf("  %s 0x%08X  slot 0x%08X  stub 0x%08X  %s\n", im->is_variable ? "var " : "func", im->nid,
                        im->slot, im->initial, n ? n : "(unknown)");
        }
    }
    std::printf("\n%zu imports, %zu identified\n", e.imports().size(), named);
    return 0;
}

analysis::Options parse_analysis_opts(int argc, char** argv, int from, bool* ok) {
    analysis::Options o;
    *ok = true;
    for (int k = from; k < argc; ++k) {
        if (!std::strcmp(argv[k], "--entries") && k + 1 < argc) {
            std::string err;
            auto v = analysis::read_address_list(argv[++k], &err);
            if (!err.empty()) {
                std::fprintf(stderr, "error: %s\n", err.c_str());
                *ok = false;
            }
            o.extra_entries.insert(o.extra_entries.end(), v.begin(), v.end());
        } else if (!std::strcmp(argv[k], "-v")) {
            o.verbose = true;
        }
    }
    return o;
}

int cmd_funcs(const loader::Elf& e, int argc, char** argv) {
    bool ok;
    auto opts = parse_analysis_opts(argc, argv, 3, &ok);
    if (!ok) return 1;
    bool list = false;
    for (int k = 3; k < argc; ++k) list |= !std::strcmp(argv[k], "--list");
    const auto prog = analysis::analyze(e, opts);
    std::size_t jt = 0, jt_targets = 0, unresolved = 0, invalid = 0;
    for (const auto& [a, f] : prog.functions) {
        jt += f.jump_tables.size();
        for (const auto& [_, t] : f.jump_tables) jt_targets += t.targets.size();
        unresolved += f.unresolved_bctr.size();
        invalid += f.invalid.size();
        if (list)
            std::printf("0x%08X  %6zu insns%s%s\n", a, f.insns.size(), f.jump_tables.empty() ? "" : "  [switch]",
                        prog.import_stubs.count(a) ? ("  -> " + prog.import_stubs.at(a)).c_str() : "");
    }
    std::printf("functions:            %zu\n", prog.functions.size());
    std::printf("instructions emitted: %zu\n", prog.total_insns());
    std::printf("jump tables:          %zu (%zu targets)\n", jt, jt_targets);
    std::printf("dynamic bctr:         %zu (%zu are import stubs)\n", unresolved, prog.import_stubs.size());
    std::printf("reachable invalid:    %zu\n", invalid);
    for (const auto& [a, f] : prog.functions)
        for (u32 b : f.unresolved_bctr)
            if (!prog.import_stubs.count(a)) std::printf("  unresolved bctr at 0x%08X in f_%08X\n", b, a);
    return 0;
}

int cmd_disasm(const loader::Elf& e, int argc, char** argv) {
    if (argc < 4) return usage();
    u32 a = static_cast<u32>(std::strtoul(argv[3], nullptr, 16));
    const u32 n = argc > 4 ? static_cast<u32>(std::strtoul(argv[4], nullptr, 0)) : 32;
    for (u32 k = 0; k < n; ++k, a += 4) {
        if (!e.ptr(a, 4)) break;
        const u32 w = e.read_u32(a);
        std::printf("%08X  %08X  %s\n", a, w, ppu::disasm(w, a).c_str());
    }
    return 0;
}

int cmd_recompile(const loader::Elf& e, int argc, char** argv) {
    recomp::EmitOptions eo;
    for (int k = 3; k < argc; ++k) {
        if (!std::strcmp(argv[k], "-o") && k + 1 < argc) eo.out_dir = argv[++k];
        else if (!std::strcmp(argv[k], "--no-annotate")) eo.annotate = false;
        else if (!std::strcmp(argv[k], "--no-gpr-cache")) eo.cache_gprs = false;
        else if (!std::strcmp(argv[k], "--per-file") && k + 1 < argc) eo.insns_per_file = std::strtoul(argv[++k], nullptr, 0);
    }
    if (e.is_prx()) {
        eo.module_name = g_prx_name;
        eo.module_base = g_prx_base;
    }
    if (eo.out_dir.empty()) return usage();
    bool ok;
    auto opts = parse_analysis_opts(argc, argv, 3, &ok);
    if (!ok) return 1;

    const auto t0 = std::chrono::steady_clock::now();
    const auto prog = analysis::analyze(e, opts);
    recomp::EmitStats stats;
    std::string err;
    if (!recomp::emit_program(e, prog, eo, &stats, &err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }
    const auto secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("recompiled %zu functions / %zu instructions into %zu files in %.1fs -> %s\n", stats.functions,
                stats.instructions, stats.files, secs, eo.out_dir.c_str());
    if (!stats.fallback_ops.empty()) {
        std::printf("instructions emitted as runtime fallbacks / no-ops:\n");
        for (const auto& [op, n] : stats.fallback_ops) std::printf("  %-10s %zu\n", ppu::op_name(op), n);
    }
    return 0;
}

int cmd_spu_list(const loader::Elf& e) {
    const auto imgs = loader::find_spu_images(e);
    std::printf("%zu embedded SPU images\n", imgs.size());
    for (std::size_t k = 0; k < imgs.size(); ++k) {
        const auto& im = imgs[k];
        u32 code = 0, total = 0;
        for (const auto& s : im.segments) {
            total += s.memsz;
            if (s.flags & 1) code += s.filesz;
        }
        std::printf("  #%zu  ea 0x%08X  file+0x%08X  %7u bytes  entry 0x%05X  code %6u  LS %6u  %s\n", k, im.vaddr,
                    im.file_offset, im.size, im.entry, code, total, im.hint.c_str());
    }
    return 0;
}

int cmd_spu_disasm(const loader::Elf& e, int argc, char** argv) {
    if (argc < 5) return usage();
    const auto imgs = loader::find_spu_images(e);
    const std::size_t idx = std::strtoul(argv[3], nullptr, 0);
    if (idx >= imgs.size()) {
        std::fprintf(stderr, "error: image #%zu does not exist (%zu images)\n", idx, imgs.size());
        return 1;
    }
    const auto& im = imgs[idx];
    u32 a = static_cast<u32>(std::strtoul(argv[4], nullptr, 16));
    const u32 n = argc > 5 ? static_cast<u32>(std::strtoul(argv[5], nullptr, 0)) : 32;
    for (u32 k = 0; k < n; ++k, a += 4) {
        const loader::SpuSegment* seg = nullptr;
        for (const auto& s : im.segments)
            if (a >= s.ls_addr && a + 4 <= s.ls_addr + s.filesz) seg = &s;
        if (!seg) break;
        const u32 w = load_be<u32>(e.data().data() + im.file_offset + seg->file_offset + (a - seg->ls_addr));
        std::printf("%05X  %08X  %s\n", a, w, spu::disasm(w, a).c_str());
    }
    return 0;
}

// spu-recompile <inputs...> -o <dir>
//   inputs: PPU ELFs/PRXs (their embedded SPU images are compiled), SPU ELFs,
//   and .spurec files written by `opentit --spu-record`.
int cmd_spu_recompile(int argc, char** argv) {
    std::vector<std::string> inputs;
    std::string out;
    for (int k = 2; k < argc; ++k) {
        if (!std::strcmp(argv[k], "-o") && k + 1 < argc) out = argv[++k];
        else inputs.push_back(argv[k]);
    }
    if (out.empty() || inputs.empty()) return usage();
    std::vector<spu::Region> regions;
    std::vector<u8> ls(0x40000);
    auto compile_image = [&](const u8* file, std::size_t size, const std::vector<loader::SpuSegment>& segs, u32 entry,
                             const std::string& what) {
        std::fill(ls.begin(), ls.end(), 0);
        for (const auto& sg : segs) {
            if (sg.ls_addr + sg.memsz > ls.size() || sg.file_offset + sg.filesz > size) continue;
            std::memcpy(ls.data() + sg.ls_addr, file + sg.file_offset, sg.filesz);
        }
        // The entry, then every statically called function.
        std::set<u32> done;
        std::vector<u32> work{entry};
        std::size_t n = 0;
        while (!work.empty() && n < 4096) {
            const u32 a = work.back() & 0x3FFFC;
            work.pop_back();
            if (!done.insert(a).second) continue;
            spu::Region r = spu::discover(ls.data(), a);
            if (r.instruction_count() == 0) continue;
            for (u32 c : r.calls) work.push_back(c);
            regions.push_back(std::move(r));
            ++n;
        }
        std::printf("  %-40s entry %05X: %zu regions\n", what.c_str(), entry, n);
    };
    for (const auto& in : inputs) {
        std::ifstream f(in, std::ios::binary);
        char magic[8] = {};
        f.read(magic, 8);
        if (std::memcmp(magic, "OTSPUREC", 8) == 0) {
            std::string err;
            const std::size_t before = regions.size();
            if (!spu::read_records(in, &regions, &err)) {
                std::fprintf(stderr, "error: %s\n", err.c_str());
                return 1;
            }
            std::printf("  %-40s %zu recorded regions\n", in.c_str(), regions.size() - before);
            continue;
        }
        auto e = open_elf(in.c_str());
        if (!e) return 1;
        const auto imgs = loader::find_spu_images(*e);
        for (std::size_t k = 0; k < imgs.size(); ++k) {
            const auto& im = imgs[k];
            std::vector<loader::SpuSegment> segs = im.segments;
            for (auto& sg : segs) sg.file_offset += im.file_offset;
            compile_image(e->data().data(), e->data().size(), segs, im.entry,
                          in.substr(in.find_last_of("/\\") + 1) + " #" + std::to_string(k) + " " + im.hint.substr(0, 20));
        }
    }
    std::string err;
    recomp::SpuEmitOptions opt;
    opt.out_dir = out;
    if (!recomp::emit_spu(regions, opt, &err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }
    u64 insns = 0;
    for (const auto& r : regions) insns += r.instruction_count();
    std::printf("%zu SPU regions (%llu instructions) -> %s\n", regions.size(), (unsigned long long)insns, out.c_str());
    return 0;
}

int cmd_sfo(const char* path) {
    std::string err;
    auto sfo = loader::read_sfo(path, &err);
    if (!sfo) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }
    for (const auto& [k, v] : *sfo) {
        if (const auto* s = std::get_if<std::string>(&v)) std::printf("%-20s %s\n", k.c_str(), s->c_str());
        else std::printf("%-20s 0x%08X\n", k.c_str(), std::get<u32>(v));
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) return usage();
    const std::string cmd = argv[1];
    for (int k = 3; k < argc; ++k) {
        if (!std::strcmp(argv[k], "--base") && k + 1 < argc) g_prx_base = static_cast<u32>(std::strtoul(argv[k + 1], nullptr, 16));
        if (!std::strcmp(argv[k], "--name") && k + 1 < argc) g_prx_name = argv[k + 1];
    }
    if (cmd == "sfo") return cmd_sfo(argv[2]);
    if (cmd == "spu-recompile") return cmd_spu_recompile(argc, argv);
    auto e = open_elf(argv[2]);
    if (!e) return 1;
    if (cmd == "info") return cmd_info(*e);
    if (cmd == "imports") return cmd_imports(*e);
    if (cmd == "funcs") return cmd_funcs(*e, argc, argv);
    if (cmd == "disasm") return cmd_disasm(*e, argc, argv);
    if (cmd == "recompile") return cmd_recompile(*e, argc, argv);
    if (cmd == "spu-list") return cmd_spu_list(*e);
    if (cmd == "spu-disasm") return cmd_spu_disasm(*e, argc, argv);
    return usage();
}
