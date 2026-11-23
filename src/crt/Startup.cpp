// openTIT - Process start-up and termination.
//
// On the PS3 the ELF entry point (_start) sets up the TOC and stack and
// calls the C runtime initialiser, which copies argv/envp out of the
// kernel-provided pair tables, initialises the C library, the SPU
// printf server and the heap, runs the static constructors (.init) and
// finally calls the game's main(). On the PC the host C++ runtime has done
// all of that by the time our main() runs, so only the hand-off remains.
#include "platform/Platform.h"

#include <cstdlib>

namespace sly {
int SlyMain(int argc, char** argv);
}

namespace crt {

// @func 0x0071B570 Exit
// exit(): runs atexit handlers, flushes stdio, runs .fini and terminates.
[[noreturn]] void Exit(int code)
{
    plat::ExitProcess(code);
}

// @func 0x0071F460 ExitImmediate
// _Exit(): terminates without running atexit handlers or flushing streams.
[[noreturn]] void ExitImmediate(int code)
{
    std::_Exit(code);
}

// @func 0x004FC8BC RuntimeStart
// Calls the game's main() with the process arguments and exits with its result.
[[noreturn]] void RuntimeStart(int argc, char** argv)
{
    const int result = sly::SlyMain(argc, argv);
    Exit(result);
}

} // namespace crt

// @func 0x004FC7B4 _start
int main(int argc, char** argv)
{
    crt::RuntimeStart(argc, argv);
}
