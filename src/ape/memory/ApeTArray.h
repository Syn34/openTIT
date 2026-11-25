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
    // Out-of-line instantiations:
    // @func 0x00762CC0 ApeTArray<T>::ApeTArray (copy 1, 4-byte elements)
    // @func 0x00786658 ApeTArray<T>::ApeTArray (copy 2, 4-byte elements)
    // @func 0x00742DD8 ApeTArray<T>::ApeTArray (copy 3, 0x4C-byte elements)
    ApeTArray(uint32_t capacity, ApeAllocator* allocator = nullptr, bool flag = false)
        : ApeArrayBase(uint16_t(sizeof(T)))
    {
        Init(capacity, allocator, flag);
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

    // Appends `value` and returns its index.
    // @func 0x0076A800 ApeTArray<T>::Add (copy 1)
    // @func 0x0076A914 ApeTArray<T>::Add (copy 2)
    // @func 0x0076AF44 ApeTArray<T>::Add (copy 3)
    // @func 0x00733964 ApeTArray<T>::Add (copy 4, 0xC0-byte elements)
    // @func 0x007450E4 ApeTArray<T>::Add (copy 5, 0xF0-byte elements)
    // @func 0x00745564 ApeTArray<T>::Add (copy 6, 0xA0-byte elements)
    // @func 0x00745720 ApeTArray<T>::Add (copy 7, 0xA0-byte elements)
    // @func 0x0075C8F4 ApeTArray<T>::Add (copy 8, 0x160-byte elements)
    // @func 0x0075EBFC ApeTArray<T>::Add (copy 9, 0x80-byte elements)
    // @func 0x0075FF20 ApeTArray<T>::Add (copy 10, 0x90-byte elements)
    // @func 0x007617C0 ApeTArray<T>::Add (copy 11, 0x100-byte elements)
    // @func 0x0076A618 ApeTArray<T>::Add (copy 12, 0x90-byte elements)
    // @func 0x0076AD3C ApeTArray<T>::Add (copy 13, 0x30-byte elements)
    // @func 0x0076E400 ApeTArray<T>::Add (copy 14, 0xE0-byte elements)
    // @func 0x0076E704 ApeTArray<T>::Add (copy 15, 0x140-byte elements)
    // @func 0x0077AF70 ApeTArray<T>::Add (copy 16, 0x80-byte elements)
    // @func 0x00788A00 ApeTArray<T>::Add (copy 17, 0x80-byte elements)
    uint32_t Add(const T& value)
    {
        PushBack(value);
        return uint32_t(m_count) - 1;
    }

    // Removes element i by moving the last element into its place.
    // @func 0x0076AE8C ApeTArray<T>::RemoveAtSwap (copy 1, 0x20-byte elements)
    // @func 0x00735874 ApeTArray<T>::RemoveAtSwap (copy 2, 0x20-byte elements)
    // @func 0x00735A20 ApeTArray<T>::RemoveAtSwap (copy 3, 8-byte elements)
    // @func 0x00735D10 ApeTArray<T>::RemoveAtSwap (copy 4, 0x70-byte elements)
    // @func 0x0074534C ApeTArray<T>::RemoveAtSwap (copy 5, 0xF0-byte elements)
    // @func 0x0074E2FC ApeTArray<T>::RemoveAtSwap (copy 6, 0x10-byte elements)
    // @func 0x0076E5A8 ApeTArray<T>::RemoveAtSwap (copy 7, 0xE0-byte elements)
    // @func 0x0076E8F4 ApeTArray<T>::RemoveAtSwap (copy 8, 0x140-byte elements)
    void RemoveAtSwap(size_t i)
    {
        --m_count;
        if (i != m_count)
            Data()[i] = Data()[m_count];
    }

    // Removes element i keeping the order of the others.
    // @func 0x00755C34 ApeTArray<T>::RemoveAt (copy 1, 8-byte elements)
    // @func 0x0075DC24 ApeTArray<T>::RemoveAt (copy 2, 4-byte elements)
    void RemoveAt(size_t i)
    {
        for (size_t j = i; j + 1 < m_count; ++j)
            Data()[j] = Data()[j + 1];
        --m_count;
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
