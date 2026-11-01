// Guest (PS3) address space.
//
// The full 32-bit guest address space is reserved as one contiguous host
// region, so a guest effective address translates with a single add.
// All guest memory is big endian; these helpers byte-swap on access.
#pragma once

#include <atomic>
#include <cstring>
#include <string>

#include "common/types.h"

namespace opentit {

namespace mem {

extern u8* g_base;  // host pointer for guest address 0

// PS3 address map used by the runtime.
inline constexpr u32 kMainMemBase = 0x00010000;    // ELF image is loaded here
inline constexpr u32 kHeapBase = 0x20000000;       // _sys_malloc / sys_memory_allocate
inline constexpr u32 kHeapSize = 0x10000000;       // 256 MiB user memory
inline constexpr u32 kMmapperBase = 0x30000000;    // sys_mmapper
inline constexpr u32 kHleBase = 0x7F000000;        // fake code addresses for HLE functions
inline constexpr u32 kRsxLocalBase = 0xC0000000;   // RSX local memory (256 MiB)
inline constexpr u32 kStackBase = 0xD0000000;      // PPU thread stacks
inline constexpr u32 kStackRegionSize = 0x10000000;

bool init();       // reserve the 4 GiB guest region
void shutdown();
bool commit(u32 addr, u32 size);  // make a range readable/writable

inline u8* ptr(u32 ea) { return g_base + ea; }

inline u8 r8(u32 ea) { return g_base[ea]; }
inline u16 r16(u32 ea) { return load_be<u16>(g_base + ea); }
inline u32 r32(u32 ea) { return load_be<u32>(g_base + ea); }
inline u64 r64(u32 ea) { return load_be<u64>(g_base + ea); }
inline void w8(u32 ea, u8 v) { g_base[ea] = v; }
inline void w16(u32 ea, u16 v) { store_be<u16>(g_base + ea, v); }
inline void w32(u32 ea, u32 v) { store_be<u32>(g_base + ea, v); }
inline void w64(u32 ea, u64 v) { store_be<u64>(g_base + ea, v); }
inline void copy(u32 dst, u32 src, u32 n) { std::memmove(g_base + dst, g_base + src, n); }

inline f32 rf32(u32 ea) {
    const u32 b = r32(ea);
    f32 f;
    std::memcpy(&f, &b, 4);
    return f;
}
inline f64 rf64(u32 ea) {
    const u64 b = r64(ea);
    f64 f;
    std::memcpy(&f, &b, 8);
    return f;
}
inline void wf32(u32 ea, f32 f) {
    u32 b;
    std::memcpy(&b, &f, 4);
    w32(ea, b);
}
inline void wf64(u32 ea, f64 f) {
    u64 b;
    std::memcpy(&b, &f, 8);
    w64(ea, b);
}

// Atomic compare-and-swap on big-endian guest words (used by stwcx./stdcx.).
inline bool cas32(u32 ea, u32 expected_be_value, u32 new_value) {
    std::atomic_ref<u32> ref(*reinterpret_cast<u32*>(g_base + ea));
    u32 exp = bswap32(expected_be_value);
    return ref.compare_exchange_strong(exp, bswap32(new_value));
}
inline bool cas64(u32 ea, u64 expected_be_value, u64 new_value) {
    std::atomic_ref<u64> ref(*reinterpret_cast<u64*>(g_base + ea));
    u64 exp = bswap64(expected_be_value);
    return ref.compare_exchange_strong(exp, bswap64(new_value));
}

// Convenience for HLE code.
inline void write_bytes(u32 ea, const void* src, std::size_t n) { std::memcpy(g_base + ea, src, n); }
inline void read_bytes(void* dst, u32 ea, std::size_t n) { std::memcpy(dst, g_base + ea, n); }
std::string read_string(u32 ea, std::size_t limit = 4096);

}  // namespace mem

}  // namespace opentit
