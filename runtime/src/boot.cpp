#include <cstdlib>
#include "opentit/spu_recomp.h"
#include "opentit/boot.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "kernel.h"
#include "log.h"
#include "opentit/memory.h"
#include "opentit/recomp.h"
#include "runtime.h"

namespace opentit {

namespace fs = std::filesystem;

namespace {

int usage() {
    std::fprintf(stderr,
                 "openTIT - Sly Cooper: Thieves in Time (recompiled)\n\n"
                 "usage: opentit <path to disc dump> [options]\n"
                 "  <path to disc dump>   folder that contains PS3_GAME/\n"
                 "  --eboot <file>        decrypted executable (default: PS3_GAME/USRDIR/EBOOT.elf)\n"
                 "  --hdd0 <dir>          host folder for /dev_hdd0 (default: ./dev_hdd0)\n"
                 "  --firmware <dir>      decrypted firmware modules, e.g. libsre.prx (default: <dump>/firmware)\n"
                 "  --log <level>         trace | debug | info | warn | error\n"
                 "  --strict              abort on the first unimplemented HLE function\n"
                 "  --dump-shaders <dir>  write every distinct vertex/fragment program's disassembly\n"
                 "  --renderer <name>     auto (default) | vulkan | null\n"
                 "  --spu-record <file>   save SPU code the build has not compiled (feed it to spu-recompile)\n"
                 "  --headless            no window\n"
                 "  --scale <n>           window size: 1 = 640x360, 2 = 1280x720, 3 = 1920x1080\n"
                 "  -- <args...>          pass the remaining arguments to the game (argv)\n");
    return 2;
}

bool load_segments(const loader::Elf& e) {
    for (const auto& s : e.segments()) {
        if (s.type != loader::PT_LOAD || !s.memsz) continue;
        const u32 va = static_cast<u32>(s.vaddr);
        if (!mem::commit(va, static_cast<u32>(s.memsz))) return false;
        std::memcpy(mem::ptr(va), e.data().data() + s.offset, s.filesz);
        std::memset(mem::ptr(va) + s.filesz, 0, s.memsz - s.filesz);
        LOG_INFO("boot", "loaded segment 0x%08X (0x%llX bytes, 0x%llX in file)", va, (unsigned long long)s.memsz,
                 (unsigned long long)s.filesz);
    }
    return true;
}

// argv / envp for the main thread, laid out in kernel memory (32-bit pointers).
// Games launched from the XMB receive no arguments; Sly 4's own parser treats
// anything starting with '-' or '/' as a switch, so argv[0] is not supplied.
u32 build_argv(const std::vector<std::string>& args, u32* envp) {
    const u32 argv = rtm::alloc_guest(static_cast<u32>(4 * (args.size() + 1)), 16);
    for (std::size_t i = 0; i < args.size(); ++i) {
        const u32 str = rtm::alloc_guest(static_cast<u32>(args[i].size() + 1), 16);
        mem::write_bytes(str, args[i].c_str(), args[i].size() + 1);
        mem::w32(argv + 4 * static_cast<u32>(i), str);
    }
    mem::w32(argv + 4 * static_cast<u32>(args.size()), 0);
    *envp = rtm::alloc_guest(16, 16);
    return argv;
}

}  // namespace

int boot_main(int argc, char** argv) {
    if (argc < 2) return usage();
    auto& cfg = rtm::config();
    cfg.game_root = argv[1];
    cfg.hdd0 = (fs::current_path() / "dev_hdd0").string();
    std::vector<std::string> game_args;
    for (int k = 2; k < argc; ++k) {
        const std::string a = argv[k];
        if (a == "--") {
            game_args.assign(argv + k + 1, argv + argc);
            break;
        }
        if (a == "--eboot" && k + 1 < argc) cfg.eboot = argv[++k];
        else if (a == "--hdd0" && k + 1 < argc) cfg.hdd0 = argv[++k];
        else if (a == "--firmware" && k + 1 < argc) cfg.firmware = argv[++k];
        else if (a == "--strict") cfg.strict = true;
        else if (a == "--dump-shaders" && k + 1 < argc) cfg.shader_dump = argv[++k];
        else if (a == "--spu-record" && k + 1 < argc) cfg.spu_record = argv[++k];
        else if (a == "--renderer" && k + 1 < argc) cfg.renderer = argv[++k];
        else if (a == "--headless") cfg.headless = true;
        else if (a == "--scale" && k + 1 < argc) cfg.scale = std::atoi(argv[++k]);
        else if (a == "--log" && k + 1 < argc) {
            const std::string l = argv[++k];
            log::set_level(l == "trace" ? log::Level::Trace
                           : l == "debug" ? log::Level::Debug
                           : l == "warn"  ? log::Level::Warn
                           : l == "error" ? log::Level::Error
                                          : log::Level::Info);
        } else {
            return usage();
        }
    }
    if (cfg.eboot.empty()) cfg.eboot = (fs::path(cfg.game_root) / "PS3_GAME" / "USRDIR" / "EBOOT.elf").string();
    if (cfg.firmware.empty()) cfg.firmware = (fs::path(cfg.game_root) / "firmware").string();

    std::string err;
    static std::optional<loader::Elf> elf = loader::Elf::load(cfg.eboot, &err);
    if (!elf) {
        LOG_ERROR("boot", "%s", err.c_str());
        return 1;
    }
    rtm::set_elf(&*elf);

    if (!mem::init()) return 1;
    if (!load_segments(*elf)) return 1;
    rtm::kernel_init();
    rtm::init_dispatch(g_recompiled_functions, g_recompiled_function_count);

    // Recompiled firmware modules (LLE). Each must be loaded at the base it was recompiled for.
    static std::vector<std::unique_ptr<loader::Elf>> modules;
    for (const auto& mt : rtm::module_tables()) {
        fs::path path = fs::path(cfg.firmware) / (std::string(mt.name) + ".prx");
        std::error_code ec;
        if (!fs::exists(path, ec)) path = fs::path(cfg.game_root) / (std::string(mt.name) + ".prx");  // dump root
        std::string merr;
        auto m = loader::Elf::load_prx(path.string(), mt.base, &merr);
        if (!m) {
            LOG_WARN("boot", "module '%s' was recompiled but %s could not be loaded (%s) - using HLE instead", mt.name,
                     path.string().c_str(), merr.c_str());
            continue;
        }
        if (!load_segments(*m)) return 1;
        rtm::init_dispatch(mt.functions, mt.count);
        LOG_INFO("boot", "module '%s' (%s) at 0x%08X, %zu exports", mt.name, m->module_name().c_str(), mt.base,
                 m->exports().size());
        modules.push_back(std::make_unique<loader::Elf>(std::move(*m)));
    }
    std::map<std::pair<std::string, u32>, u32> exports;
    for (const auto& m : modules)
        for (const auto& x : m->exports()) exports[{x.library, x.nid}] = x.addr;
    auto resolve = [&](const loader::Import& im) -> u32 {
        auto it = exports.find({im.library, im.nid});
        return it == exports.end() ? 0 : it->second;
    };
    rtm::hle_bind_imports(*elf, resolve);
    for (const auto& m : modules) rtm::hle_bind_imports(*m, resolve);

    // Main PPU thread.
    static PPUContext ctx;
    const auto& pp = elf->process_param();
    const u32 stack_size = std::max<u32>(pp.primary_stacksize, 0x100000);
    ctx.stack_size = stack_size;
    ctx.stack_addr = rtm::alloc_stack(stack_size);
    kernel::register_main_thread(ctx, 0x01000000);

    u32 envp = 0;
    const u32 argvp = build_argv(game_args, &envp);
    const auto tls = elf->tls();
    const auto entry = elf->entry_opd();

    // Register state at process entry (LV2 ABI).
    ctx.gpr[1] = ctx.stack_addr + stack_size - 0x70;
    ctx.gpr[3] = game_args.size();  // argc
    ctx.gpr[4] = argvp;
    ctx.gpr[5] = envp;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = ctx.thread_id;
    ctx.gpr[8] = tls ? tls->vaddr : 0;
    ctx.gpr[9] = tls ? tls->filesz : 0;
    ctx.gpr[10] = tls ? tls->memsz : 0;
    ctx.gpr[11] = elf->entry();
    ctx.gpr[12] = pp.malloc_pagesize;

    // module_start(argc = 0, argv = 0) for each LLE module, on its own context.
    for (const auto& m : modules) {
        if (!m->module_start_opd()) continue;
        PPUContext mctx;
        mctx.stack_size = 0x10000;
        mctx.stack_addr = rtm::alloc_stack(mctx.stack_size);
        mctx.gpr[1] = mctx.stack_addr + mctx.stack_size - 0x70;
        mctx.gpr[13] = rtm::alloc_tls() + 0x7030;
        mctx.thread_id = ctx.thread_id;
        kernel::register_main_thread(mctx, ctx.thread_id);
        rtm::call_opd(mctx, m->module_start_opd());
        LOG_INFO("boot", "module_start(%s) returned 0x%llX", m->module_name().c_str(), (unsigned long long)mctx.gpr[3]);
    }
    kernel::register_main_thread(ctx, 0x01000000);

    LOG_INFO("boot", "%zu recompiled SPU regions", spu::registered_region_count());
    if (!cfg.spu_record.empty()) spu::start_recording(cfg.spu_record);
    LOG_INFO("boot", "entry 0x%08X toc 0x%08X, stack 0x%08X+0x%X", entry.func, entry.toc, ctx.stack_addr, stack_size);
    try {
        rtm::call_code(ctx, entry.func, entry.toc);
    } catch (const rtm::ThreadExit& e) {
        LOG_INFO("boot", "main thread exited via sys_ppu_thread_exit(0x%llX)", (unsigned long long)e.code);
    }
    rtm::exit_process(static_cast<int>(ctx.gpr[3]));
}

}  // namespace opentit
