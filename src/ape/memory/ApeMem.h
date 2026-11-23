// openTIT - Ape engine low-level memory API (memory classes, system pools, stats).
//
// On the PS3 every engine allocation ends up in one of three pools:
//   * the SN libc heap (memalign/free) for most memory classes,
//   * two dlmalloc "mspaces" carved out of fixed memory regions for memory
//     classes 4 and 5.
// Each pool keeps byte counters (in use / still available / lowest available)
// plus a per-category breakdown; the category is a small 0..7 tag that some
// callers pass to group allocations in memory reports.
//
// On PC all pools are backed by the host allocator. The pools, memory classes
// and statistics are kept so that game code and debug reports behave the same,
// but there is no fixed capacity: a pool's "available" figure is only a budget
// that can be configured with SetPoolBudget().
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

// Memory classes passed to MemAlloc/MemFree. Only the values with a visible
// effect on the console are named; any other value behaves like Default.
enum MemClass : int {
    kMemClass_Default = 0,
    // NOTE: class 3 is used by the engine scratch stack (ApeMemStack) buffer and a
    // couple of other large one-off buffers; it is served from the system pool.
    kMemClass_Stack   = 3,
    // NOTE: classes 4 and 5 go to the two dlmalloc mspaces (globals 0x0116B340 and
    // 0x0116B328). Their purpose (which memory region they were carved from) is
    // not known yet.
    kMemClass_HeapA   = 4,
    kMemClass_HeapB   = 5,
};

// The pools a memory class maps to.
enum MemPool : int {
    kMemPool_System = 0,
    kMemPool_HeapA  = 1,   // memory class 4
    kMemPool_HeapB  = 2,   // memory class 5
    kMemPool_Count
};

constexpr int kMemCategoryCount = 8;
constexpr size_t kMemDefaultAlign = 16;

struct MemPoolStats {
    int64_t used;           // bytes currently allocated from this pool
    int64_t available;      // budget left (can go negative on PC)
    int64_t minAvailable;   // low-water mark of 'available'
    int64_t categoryUsed[kMemCategoryCount];
};

MemPool MemClassToPool(int memClass);

// --- system pool back-end -------------------------------------------------

// Allocates 'size' bytes aligned to 'align' from the pool selected by
// memClass, charging the bytes to 'category'. Returns nullptr on failure.
void* SysAlloc(size_t align, size_t size, int memClass, int category);
// Releases a block obtained from SysAlloc; memClass/category must match the
// allocation. Null is ignored.
void SysFree(void* ptr, int memClass, int category);
// The engine never implemented reallocation; always returns nullptr.
void* SysRealloc(void* ptr, size_t size, int category);
// Usable size of a block returned by SysAlloc (0 for null).
size_t SysBlockSize(const void* ptr);

// --- engine entry points ---------------------------------------------------

// Main engine allocation entry (what operator new and most containers use).
// The first call also brings up the memory system and the global scratch
// stack (see ApeMemStack.h).
void* MemAlloc(size_t size, int memClass = kMemClass_Default, size_t align = kMemDefaultAlign);
void  MemFree(void* ptr, int memClass = kMemClass_Default);

// malloc-style helpers (default class, 16-byte alignment).
void* Malloc(size_t size);
void  FreeIfNotNull(void* ptr);
// Duplicates a C string into engine memory; free with MemFree.
char* StrDup(const char* str);

// zlib allocation callbacks (zalloc/zfree signatures).
void* ZlibAlloc(void* opaque, unsigned items, unsigned size);
void  ZlibFree(void* opaque, void* ptr);

// --- statistics -------------------------------------------------------------

// Sets the nominal capacity of a pool. Resets 'available' and 'minAvailable'
// to the budget minus what is currently in use.
void SetPoolBudget(MemPool pool, int64_t bytes);
MemPoolStats GetPoolStats(MemPool pool);
// Total bytes in use per category across all pools.
int64_t GetCategoryTotal(int category);

// One-time memory system bring-up (idempotent). MemAlloc calls it lazily.
void MemSystemInit();

// Engine objects that the original allocated with the engine's global
// operator new derive from this to keep their storage in engine memory.
struct ApeMemObject {
    static void* operator new(size_t size);
    static void  operator delete(void* ptr);
};

} // namespace ape
