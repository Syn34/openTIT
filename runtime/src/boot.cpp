#include "opentit/boot.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

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
                 "  --log <level>         trace | debug | info | warn | error\n"
                 "  --strict              abort on the first unimplemented HLE function\n");
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

// argv / envp for the main thread, laid out in kernel memory.
u32 build_argv(const std::string& arg0, u32* envp) {
    const u32 str = rtm::alloc_guest(static_cast<u32>(arg0.size() + 1), 16);
    mem::write_bytes(str, arg0.c_str(), arg0.size() + 1);
    const u32 argv = rtm::alloc_guest(16, 16);
    mem::w32(argv, str);
    mem::w32(argv + 4, 0);
    *envp = rtm::alloc_guest(16, 16);
    return argv;
}

}  // namespace

int boot_main(int argc, char** argv) {
    if (argc < 2) return usage();
    auto& cfg = rtm::config();
    cfg.game_root = argv[1];
    cfg.hdd0 = (fs::current_path() / "dev_hdd0").string();
    for (int k = 2; k < argc; ++k) {
        const std::string a = argv[k];
        if (a == "--eboot" && k + 1 < argc) cfg.eboot = argv[++k];
        else if (a == "--hdd0" && k + 1 < argc) cfg.hdd0 = argv[++k];
        else if (a == "--strict") cfg.strict = true;
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
    rtm::hle_bind_imports(*elf);

    // Main PPU thread.
    static PPUContext ctx;
    const auto& pp = elf->process_param();
    const u32 stack_size = std::max<u32>(pp.primary_stacksize, 0x100000);
    ctx.stack_size = stack_size;
    ctx.stack_addr = rtm::alloc_stack(stack_size);
    kernel::register_main_thread(ctx, 0x01000000);

    u32 envp = 0;
    const u32 argvp = build_argv("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", &envp);
    const auto tls = elf->tls();
    const auto entry = elf->entry_opd();

    // Register state at process entry (LV2 ABI).
    ctx.gpr[1] = ctx.stack_addr + stack_size - 0x70;
    ctx.gpr[3] = 1;  // argc
    ctx.gpr[4] = argvp;
    ctx.gpr[5] = envp;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = ctx.thread_id;
    ctx.gpr[8] = tls ? tls->vaddr : 0;
    ctx.gpr[9] = tls ? tls->filesz : 0;
    ctx.gpr[10] = tls ? tls->memsz : 0;
    ctx.gpr[11] = elf->entry();
    ctx.gpr[12] = pp.malloc_pagesize;

    LOG_INFO("boot", "entry 0x%08X toc 0x%08X, stack 0x%08X+0x%X", entry.func, entry.toc, ctx.stack_addr, stack_size);
    try {
        rtm::call_code(ctx, entry.func, entry.toc);
    } catch (const rtm::ThreadExit& e) {
        LOG_INFO("boot", "main thread exited via sys_ppu_thread_exit(0x%llX)", (unsigned long long)e.code);
    }
    rtm::exit_process(static_cast<int>(ctx.gpr[3]));
}

}  // namespace opentit
