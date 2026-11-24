// openTIT - Bump allocator over a caller supplied memory block.
#include "ape/sys/ApeLinearAllocator.h"

namespace ape {

// @func 0x0056DF80 ApeLinearAllocator::Init
void ApeLinearAllocator::Init(void* base, size_t size)
{
    m_base = static_cast<uint8_t*>(base);
    m_cursor = m_base;
    m_size = size;
}

// @func 0x0056DF9C ApeLinearAllocator::Alloc
void* ApeLinearAllocator::Alloc(size_t align, size_t size)
{
    const uintptr_t cursor = reinterpret_cast<uintptr_t>(m_cursor);
    const uintptr_t aligned = (cursor + align - 1) & ~(uintptr_t)(align - 1);
    m_cursor = reinterpret_cast<uint8_t*>(aligned + size);
    return reinterpret_cast<void*>(aligned);
}

// @func 0x0056DFC8 ApeLinearAllocator::Reset
void ApeLinearAllocator::Reset()
{
    m_cursor = m_base;
}

} // namespace ape
