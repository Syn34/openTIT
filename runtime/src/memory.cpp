#include "opentit/memory.h"

#include "log.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace opentit::mem {

u8* g_base = nullptr;

namespace {
constexpr u64 kSpace = 0x100000000ull;  // 4 GiB
}

bool init() {
    if (g_base) return true;
#if defined(_WIN32)
    g_base = static_cast<u8*>(VirtualAlloc(nullptr, kSpace, MEM_RESERVE, PAGE_NOACCESS));
#else
    void* p = mmap(nullptr, kSpace, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    g_base = p == MAP_FAILED ? nullptr : static_cast<u8*>(p);
#endif
    if (!g_base) {
        LOG_ERROR("mem", "failed to reserve the 4 GiB guest address space");
        return false;
    }
    LOG_INFO("mem", "guest address space reserved at host %p", static_cast<void*>(g_base));
    return true;
}

void shutdown() {
    if (!g_base) return;
#if defined(_WIN32)
    VirtualFree(g_base, 0, MEM_RELEASE);
#else
    munmap(g_base, kSpace);
#endif
    g_base = nullptr;
}

bool commit(u32 addr, u32 size) {
    if (!size) return true;
    const u64 page = 0x10000;
    const u64 start = addr & ~(page - 1);
    const u64 end = (u64(addr) + size + page - 1) & ~(page - 1);
#if defined(_WIN32)
    const bool ok = VirtualAlloc(g_base + start, end - start, MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    const bool ok = mprotect(g_base + start, end - start, PROT_READ | PROT_WRITE) == 0;
#endif
    if (!ok) LOG_ERROR("mem", "commit failed for 0x%08llX..0x%08llX", (unsigned long long)start, (unsigned long long)end);
    return ok;
}

std::string read_string(u32 ea, std::size_t limit) {
    std::string s;
    if (!ea) return s;
    for (std::size_t i = 0; i < limit; ++i) {
        const char c = static_cast<char>(g_base[ea + i]);
        if (!c) break;
        s.push_back(c);
    }
    return s;
}

}  // namespace opentit::mem
