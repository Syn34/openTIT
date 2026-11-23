// openTIT - ApeAllocator: the allocator interface used by engine containers.
#include "ape/memory/ApeAllocator.h"
#include "ape/memory/ApeBucketAlloc.h"
#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeMemStack.h"

#include <cstring>

namespace ape {

// --- general heap -----------------------------------------------------------

// @func 0x00550F5C ApeDefaultAllocator::Alloc
void* ApeDefaultAllocator::Alloc(size_t size, bool clear, size_t align)
{
    void* ptr = MemAlloc(size, kMemClass_Default, align);
    if (clear && size != 0 && ptr)
        std::memset(ptr, 0, size);
    return ptr;
}

// @func 0x005514E0 ApeDefaultAllocator::Realloc
void* ApeDefaultAllocator::Realloc(void* ptr, size_t size)
{
    return SysRealloc(ptr, size, 0);
}

// @func 0x00551510 ApeDefaultAllocator::Free
void ApeDefaultAllocator::Free(void* ptr, size_t /*size*/)
{
    SysFree(ptr, kMemClass_Default, 0);
}

// --- size-bucket pools ---------------------------------------------------------

// @func 0x005523AC ApeBucketAllocator::Alloc
void* ApeBucketAllocator::Alloc(size_t size, bool clear, size_t /*align*/)
{
    // Buckets hand out 16-byte aligned elements; the alignment request is
    // ignored just like on the console.
    void* ptr = BucketAlloc(size);
    if (clear && size != 0 && ptr)
        std::memset(ptr, 0, size);
    return ptr;
}

// @func 0x00552418 ApeBucketAllocator::Realloc
void* ApeBucketAllocator::Realloc(void* /*ptr*/, size_t /*size*/)
{
    return nullptr;
}

// @func 0x005524D8 ApeBucketAllocator::Free
void ApeBucketAllocator::Free(void* ptr, size_t size)
{
    BucketFree(ptr, size);
}

// --- scratch stack -----------------------------------------------------------

// @func 0x00553074 ApeStackAllocator::Alloc
void* ApeStackAllocator::Alloc(size_t size, bool clear, size_t /*align*/)
{
    MemSystemInit();
    void* ptr = GetMemStack()->Alloc(size);
    if (clear && size != 0 && ptr)
        std::memset(ptr, 0, size);
    return ptr;
}

// @func 0x005530E8 ApeStackAllocator::Realloc
void* ApeStackAllocator::Realloc(void* /*ptr*/, size_t /*size*/)
{
    return nullptr;
}

// @func 0x005530F0 ApeStackAllocator::Free
void ApeStackAllocator::Free(void* ptr, size_t /*size*/)
{
    GetMemStack()->Free(ptr);
}

// --- singletons ------------------------------------------------------------------
// The console builds each one in a function-local static on first use.

// @func 0x00550E70 GetDefaultAllocator
ApeAllocator* GetDefaultAllocator()
{
    static ApeDefaultAllocator s_allocator;
    return &s_allocator;
}

// @func 0x00550EE8 GetBucketAllocator
ApeAllocator* GetBucketAllocator()
{
    static ApeBucketAllocator s_allocator;
    return &s_allocator;
}

// @func 0x00552588 GetStackAllocator
ApeAllocator* GetStackAllocator()
{
    static ApeStackAllocator s_allocator;
    return &s_allocator;
}

} // namespace ape
