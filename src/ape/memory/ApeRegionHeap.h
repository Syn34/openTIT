// openTIT - A heap carved out of a caller-supplied memory region.
//
// On the console this wraps a dlmalloc mspace built inside [base, base+size)
// and keeps byte statistics; the audio system uses it for its sample banks,
// and the two engine heaps of memory classes 4/5 are built the same way.
// The PC version keeps the interface and the statistics but serves blocks
// from the host allocator (the region itself is only remembered).
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

class ApeRegionHeap {
public:
    ApeRegionHeap() = default;
    ApeRegionHeap(const ApeRegionHeap&) = delete;
    ApeRegionHeap& operator=(const ApeRegionHeap&) = delete;

    // Builds the heap over [base, base + size). `forget` drops a heap that was
    // set up before without tearing it down (as the console does).
    void Init(void* base, size_t size, bool forget);
    // Forgets the heap; the region belongs to the caller.
    void Destroy();
    // Aligned allocation; null when the heap is not set up or exhausted.
    void* Alloc(size_t align, size_t size);
    void Free(void* ptr);

    bool IsValid() const { return m_valid; }
    size_t Used() const { return m_used; }
    size_t Available() const { return m_available; }
    size_t Peak() const { return m_peak; }

private:
    void* m_base = nullptr;       // +0x00
    size_t m_size = 0;            // +0x04
    bool m_valid = false;         // +0x08 the mspace handle on the console
    size_t m_used = 0;            // +0x0C
    size_t m_available = 0;       // +0x10
    size_t m_peak = 0;            // +0x14 high-water mark of m_used
};

} // namespace ape
