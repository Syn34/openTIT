// openTIT - ApeAllocator: the allocator interface used by engine containers.
//
// Containers (ApeTArray, ApeTFreeList, ...) take an ApeAllocator* so they can
// draw memory from the general heap, the size-bucket pools or the scratch
// stack. Three process-wide allocators exist:
//   GetDefaultAllocator()  general heap (MemAlloc, memory class 0)
//   GetBucketAllocator()   ApeMem::_BucketAlloc size-class pools
//   GetStackAllocator()    the global ApeMemStack
#pragma once

#include <cstddef>

namespace ape {

class ApeAllocator {
public:
    // Returns 'size' bytes aligned to 'align'; zero-filled when 'clear' is set.
    virtual void* Alloc(size_t size, bool clear, size_t align) = 0;
    // None of the engine allocators support this; they return nullptr.
    virtual void* Realloc(void* ptr, size_t size) = 0;
    // 'size' must be the size passed to Alloc (the bucket allocator needs it).
    virtual void Free(void* ptr, size_t size) = 0;
    virtual ~ApeAllocator() = default;
};

class ApeDefaultAllocator final : public ApeAllocator {
public:
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

class ApeBucketAllocator final : public ApeAllocator {
public:
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

class ApeStackAllocator final : public ApeAllocator {
public:
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

ApeAllocator* GetDefaultAllocator();
ApeAllocator* GetBucketAllocator();
ApeAllocator* GetStackAllocator();

} // namespace ape
