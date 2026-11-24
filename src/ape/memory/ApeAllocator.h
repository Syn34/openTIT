// openTIT - ApeAllocator: the allocator interface used by engine containers.
//
// Containers (ApeTArray, ApeTFreeList, ...) take an ApeAllocator* so they can
// draw memory from the general heap, the size-bucket pools or the scratch
// stack. Three process-wide allocators exist:
//   GetDefaultAllocator()  general heap (MemAlloc, memory class 0)
//   GetBucketAllocator()   ApeMem::_BucketAlloc size-class pools
//   GetStackAllocator()    the global ApeMemStack
#pragma once

#include "ape/memory/ApeMem.h"

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

// The general heap allocator; the bucket and stack allocators derive from
// it (their destructors restore its vtable on the console). The allocator
// objects themselves live in engine memory (ApeMemObject).
class ApeDefaultAllocator : public ApeAllocator, public ApeMemObject {
public:
    // @func 0x007A4B50 ApeDefaultAllocator::~ApeDefaultAllocator
    // @func 0x007A4B60 ApeDefaultAllocator::~ApeDefaultAllocator (deleting)
    ~ApeDefaultAllocator() override = default;
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

class ApeBucketAllocator final : public ApeDefaultAllocator {
public:
    // @func 0x007A3DB8 ApeBucketAllocator::~ApeBucketAllocator
    // @func 0x007A4BA8 ApeBucketAllocator::~ApeBucketAllocator (deleting)
    ~ApeBucketAllocator() override = default;
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

class ApeStackAllocator final : public ApeDefaultAllocator {
public:
    // @func 0x007A3DC8 ApeStackAllocator::~ApeStackAllocator
    // @func 0x007A4BF0 ApeStackAllocator::~ApeStackAllocator (deleting)
    ~ApeStackAllocator() override = default;
    void* Alloc(size_t size, bool clear, size_t align) override;
    void* Realloc(void* ptr, size_t size) override;
    void Free(void* ptr, size_t size) override;
};

ApeAllocator* GetDefaultAllocator();
ApeAllocator* GetBucketAllocator();
ApeAllocator* GetStackAllocator();

} // namespace ape
