// openTIT - ApeTArray: growable array of trivially copyable elements.
#include "ape/memory/ApeTArray.h"

#include <algorithm>
#include <cstring>

namespace ape {

// @func 0x005516E0 ApeArrayBase::ApeArrayBase
ApeArrayBase::ApeArrayBase(uint16_t elemSize)
    : m_elemSize(elemSize)
{
}

// @func 0x0054EC18 ApeArrayBase::~ApeArrayBase
ApeArrayBase::~ApeArrayBase()
{
}

// @func 0x00551598 ApeArrayBase::Init
void ApeArrayBase::Init(uint32_t capacity, ApeAllocator* allocator, bool flag)
{
    m_capacity = uint16_t(capacity);
    m_byteSize = uint32_t(m_elemSize) * m_capacity;
    m_allocator = allocator ? allocator : GetBucketAllocator();
    m_data = capacity ? m_allocator->Alloc(m_byteSize, true, 16) : nullptr;
    m_count = 0;
    m_flags = uint8_t((m_flags & ~4u) | (flag ? 4u : 0u));
}

// @func 0x00551718 ApeArrayBase::SetCapacity
void ApeArrayBase::SetCapacity(uint32_t capacity)
{
    if (!m_data) {
        Init(capacity ? capacity : 16, nullptr, false);
        return;
    }

    // NOTE: the console sizes a shrink from the requested capacity even when
    // it is below the element count (which would drop live elements); here
    // the clamped capacity is used for both decisions.
    capacity = std::max<uint32_t>(capacity, m_count);
    const uint32_t oldBytes = m_byteSize;
    uint32_t newBytes = capacity * m_elemSize;
    uint32_t copyBytes;

    if (capacity < m_capacity) {
        copyBytes = newBytes;
    } else {
        // Grow by at least 16 elements to avoid reallocating on every push.
        newBytes = std::max<uint32_t>(oldBytes + uint32_t(m_elemSize) * 16, newBytes);
        copyBytes = oldBytes;
    }

    void* newData = m_allocator->Alloc(newBytes, true, 16);
    std::memcpy(newData, m_data, copyBytes);
    m_allocator->Free(m_data, oldBytes);

    m_data = newData;
    m_capacity = uint16_t(newBytes / m_elemSize);
    m_byteSize = newBytes;
}

// @func 0x0054EC1C ApeArrayBase::Release
void ApeArrayBase::Release()
{
    if (!m_data)
        return;
    m_allocator->Free(m_data, m_byteSize);
    m_capacity = 0;
    m_count = 0;
    m_data = nullptr;
}

} // namespace ape
