// openTIT - ApeTArray: growable array of trivially copyable elements.
//
// The untyped part (ApeArrayBase) owns the storage and talks to an
// ApeAllocator; ApeTArray<T> adds typed access. Elements are moved with
// memcpy when the array grows, so T must be trivially copyable. Counts are
// 16-bit like on the console (at most 65535 elements).
#pragma once

#include "ape/memory/ApeAllocator.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ape {

class ApeArrayBase {
public:
    explicit ApeArrayBase(uint16_t elemSize = 0);
    // Does not release the storage; ApeTArray<T> does that in its own
    // destructor.
    ~ApeArrayBase();

    ApeArrayBase(const ApeArrayBase&) = delete;
    ApeArrayBase& operator=(const ApeArrayBase&) = delete;

    // Allocates zeroed storage for 'capacity' elements (any previous storage
    // is not released). A null allocator selects GetBucketAllocator().
    void Init(uint32_t capacity, ApeAllocator* allocator, bool flag);
    // Grows or shrinks the storage. Never shrinks below the element count.
    // On an array without storage this is Init(capacity ? capacity : 16).
    void SetCapacity(uint32_t capacity);
    // Returns the storage to the allocator and empties the array.
    void Release();

    uint16_t Count() const { return m_count; }
    uint16_t Capacity() const { return m_capacity; }
    uint16_t PeakCount() const { return m_peak; }
    uint16_t ElementSize() const { return m_elemSize; }
    ApeAllocator* Allocator() const { return m_allocator; }

protected:
    void* m_data = nullptr;
    uint16_t m_capacity = 0;
    uint16_t m_elemSize = 0;
    uint16_t m_count = 0;
    uint16_t m_peak = 0;          // highest count reached
    uint32_t m_byteSize = 0;      // size of the storage block in bytes
    // NOTE: bit 2 holds the 'flag' argument of Init(); its meaning is unknown.
    uint8_t m_flags = 0;
    ApeAllocator* m_allocator = nullptr;
};

template <class T>
class ApeTArray : public ApeArrayBase {
    static_assert(std::is_trivially_copyable<T>::value, "ApeTArray elements are moved with memcpy");

public:
    ApeTArray() : ApeArrayBase(uint16_t(sizeof(T))) {}
    ApeTArray(uint32_t capacity, ApeAllocator* allocator = nullptr) : ApeArrayBase(uint16_t(sizeof(T)))
    {
        Init(capacity, allocator, false);
    }
    ~ApeTArray() { Release(); }

    T* Data() { return static_cast<T*>(m_data); }
    const T* Data() const { return static_cast<const T*>(m_data); }
    T& operator[](size_t i) { return Data()[i]; }
    const T& operator[](size_t i) const { return Data()[i]; }
    T* begin() { return Data(); }
    T* end() { return Data() + m_count; }
    const T* begin() const { return Data(); }
    const T* end() const { return Data() + m_count; }

    // Out-of-line instantiations (most for 4-byte elements):
    // @func 0x0074E8DC ApeTArray<T>::PushBack (copy 1)
    // @func 0x00761428 ApeTArray<T>::PushBack (copy 2)
    // @func 0x00767F94 ApeTArray<T>::PushBack (copy 3)
    // @func 0x00768020 ApeTArray<T>::PushBack (copy 4)
    // @func 0x007680AC ApeTArray<T>::PushBack (copy 5)
    // @func 0x00768138 ApeTArray<T>::PushBack (copy 6)
    // @func 0x0077214C ApeTArray<T>::PushBack (copy 7)
    // @func 0x00735530 ApeTArray<T>::PushBack (copy 8, 0x30-byte elements)
    T& PushBack(const T& value)
    {
        if (m_count == m_capacity)
            SetCapacity(uint32_t(m_count) * 2);
        T& slot = Data()[m_count++];
        slot = value;
        if (m_count > m_peak)
            m_peak = m_count;
        return slot;
    }

    // Removes element i by moving the last element into its place.
    void RemoveAtSwap(size_t i)
    {
        --m_count;
        if (i != m_count)
            Data()[i] = Data()[m_count];
    }

    // Index of the last element equal to value, or -1.
    int FindLast(const T& value) const
    {
        for (int i = int(m_count) - 1; i >= 0; --i)
            if (Data()[i] == value)
                return i;
        return -1;
    }

    void Clear() { m_count = 0; }
    void PopBack() { --m_count; }
};

} // namespace ape
