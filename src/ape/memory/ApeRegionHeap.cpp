// openTIT - A heap carved out of a caller-supplied memory region.
#include "ape/memory/ApeRegionHeap.h"

#include <algorithm>
#include <cstdlib>

namespace ape {

namespace {

// dlmalloc refuses regions too small for its bookkeeping (0x208 bytes on the
// console build) and regions that would overflow the address space.
constexpr size_t kMinRegionSize = 0x208;

// Header placed in front of each block so Free knows what to give back.
struct alignas(16) BlockHeader {
    void* raw;
    size_t size;
};

size_t RoundUp16(size_t n)
{
    return (n + 15) & ~size_t(15);
}

BlockHeader* HeaderOf(void* ptr)
{
    return reinterpret_cast<BlockHeader*>(ptr) - 1;
}

} // namespace

// @func 0x00565AA8 ApeRegionHeap::Init
void ApeRegionHeap::Init(void* base, size_t size, bool /*forget*/)
{
    // NOTE: `forget` only clears the old handle, which is overwritten below
    // anyway; an older heap is never torn down.
    m_valid = size > kMinRegionSize;
    m_base = base;
    m_size = size;
    m_used = 0;
    m_available = size;
}

// @func 0x00565A88 ApeRegionHeap::Destroy
void ApeRegionHeap::Destroy()
{
    m_valid = false;
}

// @func 0x00565BD0 ApeRegionHeap::Alloc
void* ApeRegionHeap::Alloc(size_t align, size_t size)
{
    if (!m_valid)
        return nullptr;
    align = std::max<size_t>(align, 16);
    while (align & (align - 1))
        align += align & (~align + 1);
    // The console charges the dlmalloc chunk size (payload + header, 16-byte
    // granular); the payload rounded to 16 stands in for it.
    const size_t charged = RoundUp16(size ? size : 1) + 16;
    if (charged > m_available)
        return nullptr;
    void* raw = std::malloc(charged + align + sizeof(BlockHeader));
    if (!raw)
        return nullptr;
    const uintptr_t user = (reinterpret_cast<uintptr_t>(raw) + sizeof(BlockHeader) + align - 1) & ~(uintptr_t(align) - 1);
    void* ptr = reinterpret_cast<void*>(user);
    HeaderOf(ptr)->raw = raw;
    HeaderOf(ptr)->size = charged;
    m_used += charged;
    m_available -= charged;
    m_peak = std::max(m_peak, m_used);
    return ptr;
}

// @func 0x00565C50 ApeRegionHeap::Free
void ApeRegionHeap::Free(void* ptr)
{
    if (!ptr)
        return;
    BlockHeader* header = HeaderOf(ptr);
    m_used -= header->size;
    m_available += header->size;
    std::free(header->raw);
}

} // namespace ape
