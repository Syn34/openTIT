// openTIT - C++ runtime support of the statically linked SN toolchain library.
//
// The console executable carries the language support routines of its C++
// library next to the C runtime: the global allocation functions, the
// terminate machinery, the pure virtual call trap and the report printed
// when an exception escapes (the game is built without exception handling,
// so a failed allocation reports and aborts instead of throwing).
//
// On the PC the host C++ runtime provides the language-level versions of
// these; the functions here are the named equivalents that ported code (and
// documentation) can refer to. They are built on the crt:: allocator so that
// memory obtained here is released with crt::Free like on the console.
#include "crt/libc/Libc.h"

#include <cstring>
#include <exception>
#include <new>

namespace crt {

[[noreturn]] void DefaultTerminateHandler();

namespace {
// Current terminate handler (console 0x00F94720, initially the default one
// that aborts).
std::terminate_handler g_terminateHandler = &DefaultTerminateHandler;
} // namespace

// @func 0x00731800 ReportUncaughtException
// Prints "exception: <what>" on stdout and aborts. Used where the library
// would have thrown (allocation failure).
[[noreturn]] void ReportUncaughtException(const std::exception& exception)
{
    const char* what = exception.what();
    std::fflush(nullptr);
    std::fputs("exception: ", stdout);
    std::fputs(what ? what : "unknown", stdout);
    std::fputs("\n", stdout);
    Abort();
}

// @func 0x00731648 OperatorNewAligned
// operator new(size, alignment): retries through the new handler while the
// heap is exhausted; without a handler the bad_alloc report ends the game.
void* OperatorNewAligned(size_t size, size_t alignment)
{
    for (;;) {
        if (void* block = MemAlign(alignment, size))
            return block;
        std::new_handler handler = std::get_new_handler();
        if (!handler)
            ReportUncaughtException(std::bad_alloc());
        handler();
    }
}

// @func 0x00731620 OperatorNewArrayAligned
// operator new[](size, alignment) is the scalar version.
void* OperatorNewArrayAligned(size_t size, size_t alignment)
{
    return OperatorNewAligned(size, alignment);
}

// @func 0x007315EC OperatorDelete
// operator delete: null is ignored.
void OperatorDelete(void* block)
{
    if (block)
        Free(block);
}

// @func 0x00732D84 DefaultTerminateHandler
[[noreturn]] void DefaultTerminateHandler()
{
    Abort();
}

std::terminate_handler SetTerminateHandler(std::terminate_handler handler)
{
    std::terminate_handler previous = g_terminateHandler;
    g_terminateHandler = handler;
    return previous;
}

// @func 0x00732D44 Terminate
// std::terminate(): runs the installed handler, then aborts in case it
// returns.
[[noreturn]] void Terminate()
{
    if (g_terminateHandler)
        g_terminateHandler();
    Abort();
}

// @func 0x007318A4 PureVirtualCalled
// Target of every pure virtual slot (__cxa_pure_virtual).
[[noreturn]] void PureVirtualCalled()
{
    Terminate();
}

// @func 0x00732BCC MemCpyAligned
// Block copy used for doubleword aligned buffers (copies 64, 32, 16, 8, 4
// byte pieces, then single bytes). Returns the destination.
void* MemCpyAligned(void* dst, const void* src, size_t bytes)
{
    return std::memcpy(dst, src, bytes);
}

} // namespace crt
