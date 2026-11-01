// Crash reports: what the guest was doing when the host process faulted.
#include <cinttypes>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "opentit/memory.h"
#include "opentit/ppu_context.h"
#include "opentit/spu_recomp.h"
#include "runtime.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <execinfo.h>
#include <unistd.h>
#endif

namespace opentit::rtm {

namespace {

std::string g_dump_path;
std::vector<std::pair<u32, u32>> g_dump_ranges;

void report(const char* what, const void* addr) {
    std::fprintf(stderr, "\n==== openTIT crash: %s ====\n", what);
    const auto a = reinterpret_cast<std::uintptr_t>(addr), base = reinterpret_cast<std::uintptr_t>(mem::g_base);
    if (addr && a >= base && a < base + 0x100000000ull)
        std::fprintf(stderr, "fault address: guest 0x%08X (unmapped or protected guest memory)\n",
                     static_cast<u32>(a - base));
    else if (addr)
        std::fprintf(stderr, "fault address: host %p\n", addr);
    if (const PPUContext* c = current_ctx()) {
        std::fprintf(stderr, "PPU thread 0x%08X  lr 0x%08" PRIX64 "  ctr 0x%08" PRIX64 "\n", c->thread_id, c->lr, c->ctr);
        for (int r = 0; r < 32; r += 4)
            std::fprintf(stderr, "  r%-2d %016" PRIX64 "  r%-2d %016" PRIX64 "  r%-2d %016" PRIX64 "  r%-2d %016" PRIX64 "\n", r,
                         c->gpr[r], r + 1, c->gpr[r + 1], r + 2, c->gpr[r + 2], r + 3, c->gpr[r + 3]);
    } else {
        std::fprintf(stderr, "(not on a PPU thread: SPU, RSX or a host thread)\n");
    }
    std::fflush(stderr);
}

#if defined(_WIN32)
LONG WINAPI on_exception(EXCEPTION_POINTERS* e) {
    const auto* r = e->ExceptionRecord;
    char what[64];
    std::snprintf(what, sizeof what, "exception 0x%08lX at %p", r->ExceptionCode, r->ExceptionAddress);
    const void* addr = r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2
                           ? reinterpret_cast<const void*>(r->ExceptionInformation[1]) : nullptr;
    report(what, addr);
    void* frames[32];
    const USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
    HMODULE self = GetModuleHandleW(nullptr);
    std::fprintf(stderr, "host stack (offsets into opentit.exe):\n");
    for (USHORT k = 0; k < n; ++k)
        std::fprintf(stderr, "  #%u +0x%llX\n", k,
                     static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(frames[k]) -
                                                     reinterpret_cast<std::uintptr_t>(self)));
    spu::flush_recording();
    write_exit_dump();
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void on_signal(int sig, siginfo_t* info, void*) {
    report(sig == SIGSEGV ? "SIGSEGV" : sig == SIGBUS ? "SIGBUS" : sig == SIGFPE ? "SIGFPE" : sig == SIGILL ? "SIGILL" : "SIGABRT",
           info ? info->si_addr : nullptr);
    void* frames[32];
    const int n = backtrace(frames, 32);
    std::fprintf(stderr, "host stack:\n");
    backtrace_symbols_fd(frames, n, 2);
    spu::flush_recording();
    write_exit_dump();
    _exit(128 + sig);
}
#endif

}  // namespace

void set_exit_dump(const std::string& path, std::vector<std::pair<u32, u32>> ranges) {
    g_dump_path = path;
    g_dump_ranges = std::move(ranges);
}

void write_exit_dump() {
    if (g_dump_path.empty()) return;
    std::FILE* f = std::fopen(g_dump_path.c_str(), "wb");
    if (!f) return;
    for (const auto& [addr, size] : g_dump_ranges) std::fwrite(mem::ptr(addr), 1, size, f);
    std::fclose(f);
    std::fprintf(stderr, "guest memory written to %s\n", g_dump_path.c_str());
    g_dump_path.clear();
}

void install_crash_handler() {
#if defined(_WIN32)
    SetUnhandledExceptionFilter(on_exception);
#else
    struct sigaction sa{};
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    // An alternate stack so a guest stack overflow can still be reported.
    static char altstack[64 * 1024];
    stack_t ss{};
    ss.ss_sp = altstack;
    ss.ss_size = sizeof altstack;
    sigaltstack(&ss, nullptr);
    for (int s : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT}) sigaction(s, &sa, nullptr);
#endif
}

}  // namespace opentit::rtm
