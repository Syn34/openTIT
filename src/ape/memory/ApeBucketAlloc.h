// openTIT - ApeMem bucket allocator: size-class pools for small allocations.
//
// Requests up to 18 KiB are served from one of ten ApeFreeList pools whose
// element sizes go from 64 bytes to 0x4800 bytes; anything larger goes to
// the general heap. A pool is created the first time its size class is used
// and its first block is permanent. The caller must pass the same size to
// BucketFree() that it passed to BucketAlloc(), since the size picks the pool.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

constexpr int kNumMemBuckets = 10;

// ApeMem::_BucketAlloc in the original.
void* BucketAllocInternal(size_t size);
void* BucketAlloc(size_t size);
void  BucketFree(void* ptr, size_t size);

struct MemBucketInfo {
    uint32_t elementSize;    // largest request served by this bucket
    uint32_t initialCount;   // elements in the first (permanent) block
    bool created;
    uint32_t usedCount;
    uint32_t peakUsed;
    int64_t wastedBytes;     // sum of (elementSize - requested size) for live blocks
};

MemBucketInfo GetBucketInfo(int bucket);
int64_t GetBucketWastedTotal();
// Overrides the size of a bucket's first block; must be called before the
// bucket is first used. 0 restores the default, negative values mean 0.
void SetBucketInitialCount(int bucket, int count);

} // namespace ape
