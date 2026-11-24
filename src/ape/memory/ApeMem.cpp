// openTIT - Ape engine low-level memory API (memory classes, system pools, stats).
#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeMemStack.h"
#include "platform/Platform.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace ape {

namespace {

// Every SysAlloc block is preceded by this header. It replaces the chunk size
// word that dlmalloc / the SN heap kept in front of the user pointer.
struct BlockHeader {
    void* raw;          // pointer returned by std::malloc
    size_t size;        // usable size charged to the pool
    uint32_t magic;
    int32_t pool;
};
constexpr uint32_t kBlockMagic = 0x41504D42; // 'APMB'

struct MemState {
    plat::Mutex lock;
    MemPoolStats pools[kMemPool_Count] = {};
    int64_t categoryTotal[kMemCategoryCount] = {};
};

MemState& State()
{
    static MemState s_state;
    return s_state;
}

bool s_memSystemStarted = false;

size_t RoundUp(size_t value, size_t align)
{
    return (value + align - 1) & ~(align - 1);
}

int ClampCategory(int category)
{
    if (category < 0)
        return 0;
    if (category >= kMemCategoryCount)
        return kMemCategoryCount - 1;
    return category;
}

BlockHeader* HeaderOf(const void* ptr)
{
    return reinterpret_cast<BlockHeader*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(ptr)) - sizeof(BlockHeader));
}

} // namespace

MemPool MemClassToPool(int memClass)
{
    switch (memClass) {
    case kMemClass_HeapA: return kMemPool_HeapA;
    case kMemClass_HeapB: return kMemPool_HeapB;
    default:              return kMemPool_System;
    }
}

// @func 0x0056C514 SysAlloc
void* SysAlloc(size_t align, size_t size, int memClass, int category)
{
    if (align < kMemDefaultAlign)
        align = kMemDefaultAlign;
    // Alignment must be a power of two; round odd requests up.
    while (align & (align - 1))
        align += align & (~align + 1);

    const size_t usable = RoundUp(size ? size : 1, kMemDefaultAlign);
    void* raw = std::malloc(usable + align + sizeof(BlockHeader));
    if (!raw)
        return nullptr;

    uintptr_t user = RoundUp(reinterpret_cast<uintptr_t>(raw) + sizeof(BlockHeader), align);
    void* ptr = reinterpret_cast<void*>(user);
    BlockHeader* header = HeaderOf(ptr);
    header->raw = raw;
    header->size = usable;
    header->magic = kBlockMagic;
    header->pool = MemClassToPool(memClass);

    const int cat = ClampCategory(category);
    MemState& state = State();
    plat::ScopedLock guard(state.lock);
    MemPoolStats& pool = state.pools[header->pool];
    pool.used += (int64_t)usable;
    pool.available -= (int64_t)usable;
    if (pool.available < pool.minAvailable)
        pool.minAvailable = pool.available;
    pool.categoryUsed[cat] += (int64_t)usable;
    state.categoryTotal[cat] += (int64_t)usable;
    return ptr;
}

// @func 0x0056C730 SysFree
void SysFree(void* ptr, int memClass, int category)
{
    if (!ptr)
        return;
    BlockHeader* header = HeaderOf(ptr);
    if (header->magic != kBlockMagic) {
        plat::Log(plat::LogLevel::Error, "SysFree: %p was not allocated by SysAlloc\n", ptr);
        return;
    }

    // The console trusts the caller's memory class to pick the pool; the
    // header lets us use the pool the block really came from.
    (void)memClass;
    const int cat = ClampCategory(category);
    {
        MemState& state = State();
        plat::ScopedLock guard(state.lock);
        MemPoolStats& pool = state.pools[header->pool];
        const int64_t size = (int64_t)header->size;
        pool.used -= size;
        pool.available += size;
        pool.categoryUsed[cat] -= size;
        state.categoryTotal[cat] -= size;
    }
    header->magic = 0;
    std::free(header->raw);
}

// @func 0x0056C89C SysRealloc
void* SysRealloc(void* ptr, size_t size, int category)
{
    // Reallocation was never implemented by the engine; callers must cope
    // with a null result.
    (void)ptr;
    (void)size;
    (void)category;
    return nullptr;
}

size_t SysBlockSize(const void* ptr)
{
    if (!ptr)
        return 0;
    const BlockHeader* header = HeaderOf(ptr);
    return header->magic == kBlockMagic ? header->size : 0;
}

void MemSystemInit()
{
    if (s_memSystemStarted)
        return;
    s_memSystemStarted = true;
    // NOTE: the console version (0x0056C2E4) queries the user memory size,
    // prints it, warns when less than 170 MB is available and pre-touches the
    // whole heap once. None of that is meaningful on PC.
    detail::CreateGlobalMemStack();
}

// @func 0x0054ED38 MemAlloc
void* MemAlloc(size_t size, int memClass, size_t align)
{
    MemSystemInit();
    return SysAlloc(align, size, memClass, 0);
}

// @func 0x0054EE6C MemFree
void MemFree(void* ptr, int memClass)
{
    SysFree(ptr, memClass, 0);
}

// @func 0x0055578C Malloc
void* Malloc(size_t size)
{
    return MemAlloc(size, kMemClass_Default, kMemDefaultAlign);
}

// @func 0x00555CC0 FreeIfNotNull
void FreeIfNotNull(void* ptr)
{
    if (ptr)
        SysFree(ptr, kMemClass_Default, 0);
}

// @func 0x00556038 StrDup
char* StrDup(const char* str)
{
    const size_t length = std::strlen(str);
    char* copy = static_cast<char*>(MemAlloc(length + 1));
    if (copy)
        std::memcpy(copy, str, length + 1);
    return copy;
}

// @func 0x00550908 ZlibAlloc
void* ZlibAlloc(void* opaque, unsigned items, unsigned size)
{
    (void)opaque;
    return MemAlloc((size_t)items * size);
}

// @func 0x00550E40 ZlibFree
void ZlibFree(void* opaque, void* ptr)
{
    (void)opaque;
    SysFree(ptr, kMemClass_Default, 0);
}

void SetPoolBudget(MemPool pool, int64_t bytes)
{
    MemState& state = State();
    plat::ScopedLock guard(state.lock);
    MemPoolStats& stats = state.pools[pool];
    stats.available = bytes - stats.used;
    stats.minAvailable = stats.available;
}

MemPoolStats GetPoolStats(MemPool pool)
{
    MemState& state = State();
    plat::ScopedLock guard(state.lock);
    return state.pools[pool];
}

int64_t GetCategoryTotal(int category)
{
    MemState& state = State();
    plat::ScopedLock guard(state.lock);
    return state.categoryTotal[ClampCategory(category)];
}

// @func 0x007A3DD8 ApeMemObject::operator new
// The console inlines the memory system bring-up (MemSystemInit, global
// scratch stack) in front of the allocation; MemAlloc does the same here.
void* ApeMemObject::operator new(size_t size)
{
    void* ptr = MemAlloc(size);
    if (!ptr)
        throw std::bad_alloc();
    return ptr;
}

void ApeMemObject::operator delete(void* ptr)
{
    SysFree(ptr, kMemClass_Default, 0);
}

} // namespace ape
