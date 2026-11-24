// openTIT - Small engine utilities: byte hashing and key sorting.
#include "ape/sys/ApeUtil.h"

#include <utility>

namespace ape {

// @func 0x0056A7AC HashBytes31
uint32_t HashBytes31(const void* data, uint32_t size)
{
    if (!data)
        return 0;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 0;
    for (uint32_t i = 0; i < size; ++i)
        hash = hash * 31u + bytes[i];
    return hash;
}

// Iterative quicksort. The partition walks two cursors towards each other;
// whenever a pair is out of order it is swapped and the other cursor starts
// moving, which leaves the first element (the pivot) at its final place.
// The same scheme is kept so equal keys end up in the same order as on the
// console.
// @func 0x0056A81C SortPairsByKey
void SortPairsByKey(KeyValuePair* pairs, int count)
{
    struct Range {
        int lo, hi;
    };
    Range pending[64];
    int depth = 0;
    int lo = 0;
    int hi = count - 1;

    for (;;) {
        do {
            int i = lo;
            int j = hi;
            bool moveRight = true;   // true: advance j downwards
            while (i < j) {
                if (pairs[j].key < pairs[i].key) {
                    std::swap(pairs[i], pairs[j]);
                    moveRight = !moveRight;
                }
                if (moveRight)
                    --j;
                else
                    ++i;
            }
            if (i + 1 < hi)
                pending[depth++] = Range{ i + 1, hi };
            hi = i - 1;
        } while (lo < hi);

        if (depth == 0)
            break;
        --depth;
        lo = pending[depth].lo;
        hi = pending[depth].hi;
    }
}

} // namespace ape
