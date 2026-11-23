// openTIT - ApeMem bucket allocator: size-class pools for small allocations.
#include "ape/memory/ApeBucketAlloc.h"
#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeTFreeList.h"
#include "platform/Platform.h"

#include <cstdio>

namespace ape {

namespace {

struct MemBucket {
    uint32_t elementSize;
    uint32_t defaultCount;
    ApeFreeList* pool;
    int64_t wastedBytes;
    char name[64];
};

// Size classes and first-block element counts from the console build.
MemBucket s_buckets[kNumMemBuckets] = {
    { 0x0040, 4788, nullptr, 0, {} },
    { 0x0080, 2176, nullptr, 0, {} },
    { 0x0100, 1032, nullptr, 0, {} },
    { 0x0200, 1053, nullptr, 0, {} },
    { 0x0400,  154, nullptr, 0, {} },
    { 0x0800,   84, nullptr, 0, {} },
    { 0x1000,  248, nullptr, 0, {} },
    { 0x1800,   44, nullptr, 0, {} },
    { 0x2000,   18, nullptr, 0, {} },
    { 0x4800,  114, nullptr, 0, {} },
};

// Per-bucket overrides of defaultCount (0 = use the default).
int16_t s_countOverride[kNumMemBuckets] = {};
int64_t s_totalWasted = 0;

plat::Mutex& BucketLock()
{
    static plat::Mutex s_lock;
    return s_lock;
}

MemBucket* FindBucket(size_t size)
{
    for (MemBucket& bucket : s_buckets)
        if (size <= bucket.elementSize)
            return &bucket;
    return nullptr;
}

ApeFreeList* CreatePool(MemBucket& bucket, int index)
{
    const int16_t override = s_countOverride[index];
    uint32_t count = bucket.defaultCount;
    if (override != 0)
        count = override > 0 ? uint32_t(override) : 0;

    ApeFreeList* pool = new ApeFreeList();
    pool->Init(bucket.elementSize, count, nullptr, true);
    std::snprintf(bucket.name, sizeof(bucket.name), "%s:%5d",
                  "void *ApeMem::_BucketAlloc(unsigned int)", (int)bucket.elementSize);
    pool->SetName(bucket.name);
    return pool;
}

} // namespace

// @func 0x00551CC0 ApeMem::_BucketAlloc
void* BucketAllocInternal(size_t size)
{
    {
        plat::ScopedLock guard(BucketLock());
        if (MemBucket* bucket = FindBucket(size)) {
            if (!bucket->pool)
                bucket->pool = CreatePool(*bucket, int(bucket - s_buckets));
            const int64_t waste = int64_t(bucket->elementSize) - int64_t(size);
            bucket->wastedBytes += waste;
            s_totalWasted += waste;
            return bucket->pool->Alloc();
        }
    }
    // Too big for any bucket.
    return MemAlloc(size, kMemClass_Default, 16);
}

// @func 0x0055238C BucketAlloc
void* BucketAlloc(size_t size)
{
    return BucketAllocInternal(size);
}

// @func 0x00552420 BucketFree
void BucketFree(void* ptr, size_t size)
{
    if (!ptr)
        return;
    {
        plat::ScopedLock guard(BucketLock());
        if (MemBucket* bucket = FindBucket(size)) {
            if (!bucket->pool) {
                plat::Log(plat::LogLevel::Error, "BucketFree: %p (size %u) has no bucket pool\n",
                          ptr, (unsigned)size);
                return;
            }
            const int64_t waste = int64_t(bucket->elementSize) - int64_t(size);
            bucket->wastedBytes -= waste;
            s_totalWasted -= waste;
            bucket->pool->Free(ptr);
            return;
        }
    }
    SysFree(ptr, kMemClass_Default, 0);
}

MemBucketInfo GetBucketInfo(int index)
{
    MemBucketInfo info = {};
    if (index < 0 || index >= kNumMemBuckets)
        return info;
    plat::ScopedLock guard(BucketLock());
    const MemBucket& bucket = s_buckets[index];
    info.elementSize = bucket.elementSize;
    info.initialCount = bucket.defaultCount;
    info.created = bucket.pool != nullptr;
    info.usedCount = bucket.pool ? bucket.pool->UsedCount() : 0;
    info.peakUsed = bucket.pool ? bucket.pool->PeakUsed() : 0;
    info.wastedBytes = bucket.wastedBytes;
    return info;
}

int64_t GetBucketWastedTotal()
{
    plat::ScopedLock guard(BucketLock());
    return s_totalWasted;
}

void SetBucketInitialCount(int index, int count)
{
    if (index < 0 || index >= kNumMemBuckets)
        return;
    plat::ScopedLock guard(BucketLock());
    s_countOverride[index] = int16_t(count);
}

} // namespace ape
