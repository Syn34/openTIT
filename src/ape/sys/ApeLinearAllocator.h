// openTIT - Bump allocator over a caller supplied memory block
// (0x0056DF80-0x0056DFC8).
//
// NOTE: used by the renderer side (0x0053AC38 sets it up); the original name
// is unknown. Allocation does not check the end of the block, like the
// console code.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

class ApeLinearAllocator {
public:
    void Init(void* base, size_t size);
    // Aligns the cursor to `align` (a power of two), reserves `size` bytes.
    void* Alloc(size_t align, size_t size);
    // Releases everything at once.
    void Reset();

    uint8_t* Base() const { return m_base; }
    uint8_t* Cursor() const { return m_cursor; }
    size_t Size() const { return m_size; }
    size_t Used() const { return size_t(m_cursor - m_base); }

private:
    uint8_t* m_base = nullptr;
    uint8_t* m_cursor = nullptr;
    size_t m_size = 0;
};

} // namespace ape
