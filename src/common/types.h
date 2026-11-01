// openTIT - Sly Cooper: Thieves in Time static recompilation project
// Common fixed-width types and endian helpers shared by the tools and runtime.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace opentit {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using f32 = float;
using f64 = double;

#if defined(_MSC_VER)
#include <stdlib.h>
inline u16 bswap16(u16 v) { return _byteswap_ushort(v); }
inline u32 bswap32(u32 v) { return _byteswap_ulong(v); }
inline u64 bswap64(u64 v) { return _byteswap_uint64(v); }
#else
inline constexpr u16 bswap16(u16 v) { return __builtin_bswap16(v); }
inline constexpr u32 bswap32(u32 v) { return __builtin_bswap32(v); }
inline constexpr u64 bswap64(u64 v) { return __builtin_bswap64(v); }
#endif

// Read a big-endian value from an unaligned host pointer.
template <typename T>
inline T load_be(const void* p) {
    static_assert(std::is_integral_v<T>);
    T v;
    std::memcpy(&v, p, sizeof(T));
    if constexpr (sizeof(T) == 2) return static_cast<T>(bswap16(static_cast<u16>(v)));
    else if constexpr (sizeof(T) == 4) return static_cast<T>(bswap32(static_cast<u32>(v)));
    else if constexpr (sizeof(T) == 8) return static_cast<T>(bswap64(static_cast<u64>(v)));
    else return v;
}

template <typename T>
inline void store_be(void* p, T v) {
    static_assert(std::is_integral_v<T>);
    if constexpr (sizeof(T) == 2) v = static_cast<T>(bswap16(static_cast<u16>(v)));
    else if constexpr (sizeof(T) == 4) v = static_cast<T>(bswap32(static_cast<u32>(v)));
    else if constexpr (sizeof(T) == 8) v = static_cast<T>(bswap64(static_cast<u64>(v)));
    std::memcpy(p, &v, sizeof(T));
}

}  // namespace opentit
