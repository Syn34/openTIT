// openTIT - Small engine utilities: byte hashing and key sorting.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

// h = h * 31 + byte over `size` bytes (0 for a null pointer).
uint32_t HashBytes31(const void* data, uint32_t size);

// (key, value) pair sorted by SortPairsByKey.
struct KeyValuePair {
    uint32_t key;
    uint32_t value;
};
// Sorts `count` pairs by ascending key, in place.
void SortPairsByKey(KeyValuePair* pairs, int count);

} // namespace ape
