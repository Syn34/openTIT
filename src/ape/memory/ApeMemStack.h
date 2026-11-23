// openTIT - ApeMemStack: LIFO scratch allocator over one contiguous buffer.
//
// Each allocation is preceded by a 16-byte header holding its rounded size.
// Freeing a block rewinds the stack top to that block, which also releases
// everything allocated after it, so the usual pattern is
//     void* p = ape::GetMemStack()->Alloc(n);  ...  ape::GetMemStack()->Free(p);
// When the buffer is exhausted the stack can fall back to the general heap
// (blocks outside the buffer are recognised and released to the heap by Free).
#pragma once

#include "ape/memory/ApeMem.h"

#include <cstddef>
#include <cstdint>

namespace ape {

class ApeMemStack : public ApeMemObject {
public:
    ApeMemStack(size_t size, int memClass, bool fallbackToHeap);
    virtual ~ApeMemStack();

    void* Alloc(size_t size);
    virtual void Free(void* ptr);

    // Rewinds the stack to empty.
    void Reset();

    size_t Capacity() const { return m_size; }
    size_t BytesUsed() const { return (size_t)(m_top - m_base); }
    size_t MinFree() const { return m_minFree; }
    bool Owns(const void* ptr) const;

private:
    static constexpr size_t kHeaderSize = 16;

    uint8_t* m_base = nullptr;   // start of the buffer
    uint8_t* m_end = nullptr;    // one past the end of the buffer
    uint8_t* m_top = nullptr;    // next free byte (always 16-byte aligned)
    // NOTE: only written by Reset() (end of buffer rounded down to 16); probably
    // the limit of a second, downward-growing end that this build never uses.
    uint8_t* m_upperLimit = nullptr;
    size_t m_size = 0;           // requested buffer size
    int m_memClass = 0;          // memory class the buffer was allocated from
    bool m_fallbackToHeap = false;
    size_t m_minFree = 0;        // lowest amount of free space seen
};

// The global scratch stack (512 KiB, created on the first MemAlloc call).
ApeMemStack* GetMemStack();

namespace detail {
// Creates the global scratch stack; called from MemSystemInit().
void CreateGlobalMemStack();
}

} // namespace ape
