// openTIT - ApeTFixedArray<T>: a fixed number of elements allocated in one
// block from an ApeAllocator.
//
// The block starts with a 16-byte header (element count, block size, whether
// an index table follows, the allocator), then optionally a table of one
// 32-bit index per element, then the elements. The elements are raw storage:
// the owner initialises them.
#pragma once

#include "ape/memory/ApeAllocator.h"

#include <cstddef>
#include <cstdint>

namespace ape {

template <class T>
class ApeTFixedArray {
public:
    ApeTFixedArray() = default;
    ~ApeTFixedArray() { Release(); }
    ApeTFixedArray(const ApeTFixedArray&) = delete;
    ApeTFixedArray& operator=(const ApeTFixedArray&) = delete;

    // Allocates room for `count` elements (none for 0) and, with
    // `withIndex`, the index table.
    void Init(uint16_t count, bool withIndex, ApeAllocator* allocator);
    // Gives the block back to its allocator.
    void Release();

    uint16_t Count() const { return m_header ? m_header->count : 0; }
    T* Data() { return m_elements; }
    const T* Data() const { return m_elements; }
    T& operator[](size_t i) { return m_elements[i]; }
    const T& operator[](size_t i) const { return m_elements[i]; }
    // The index table (null without one).
    uint32_t* Indices() { return m_indices; }

private:
    struct Header {
        uint16_t count;
        uint16_t field_02;
        uint32_t blockSize;
        uint32_t withIndex;
        ApeAllocator* allocator;
    };

    void* m_block = nullptr;       // +0x00
    Header* m_header = nullptr;    // +0x04
    uint32_t* m_indices = nullptr; // +0x08
    T* m_elements = nullptr;       // +0x0C
};

// One instantiation per element type on the console.
// @func 0x00789280 ApeTFixedArray<T>::Init (AnimClipEntry)
// @func 0x00733DD0 ApeTFixedArray<T>::Init (copy, 0x14-byte elements)
// @func 0x0076EBA0 ApeTFixedArray<T>::Init (copy, 8-byte elements)
// @func 0x0077A9AC ApeTFixedArray<T>::Init (copy, bartender 1)
// @func 0x0077AAE0 ApeTFixedArray<T>::Init (copy, bartender 2)
// @func 0x0077AC14 ApeTFixedArray<T>::Init (copy, bartender 3)
// @func 0x007A7550 ApeTFixedArray<T>::Init (copy, world debug materials)
// @func 0x007A7674 ApeTFixedArray<T>::Init (copy, spline instances)
// @func 0x007A77A0 ApeTFixedArray<T>::Init (copy, emitter definitions)
template <class T>
void ApeTFixedArray<T>::Init(uint16_t count, bool withIndex, ApeAllocator* allocator)
{
    if (count == 0)
        return;
    const size_t indexBytes = withIndex ? size_t(count) * sizeof(uint32_t) : 0;
    const uint32_t blockSize = uint32_t((size_t(count) * sizeof(T) + indexBytes + 0x1F) & ~size_t(0xF));
    m_block = allocator->Alloc(blockSize, false, 0x10);
    m_header = static_cast<Header*>(m_block);
    m_header->count = count;
    m_header->field_02 = 0;
    m_header->blockSize = blockSize;
    m_header->withIndex = withIndex ? 1u : 0u;
    m_header->allocator = allocator;
    uint8_t* afterHeader = static_cast<uint8_t*>(m_block) + sizeof(Header);
    m_indices = withIndex ? reinterpret_cast<uint32_t*>(afterHeader) : nullptr;
    m_elements = reinterpret_cast<T*>(afterHeader + indexBytes);
}

template <class T>
void ApeTFixedArray<T>::Release()
{
    if (!m_block)
        return;
    m_header->allocator->Free(m_block, m_header->blockSize);
    m_block = nullptr;
    m_header = nullptr;
    m_indices = nullptr;
    m_elements = nullptr;
}

} // namespace ape
