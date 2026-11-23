// openTIT - Compact arrays and sorted key/value maps.
//
// ApeCompactArrayBase is the 16-bit counted array the engine embeds in many
// objects: capacity/count/peak and the element size are packed into four
// halfwords, storage comes from an ApeAllocator. ApeTSortedMap<V> builds a
// uint32 -> V dictionary on top of it: entries are appended unsorted and the
// array is sorted lazily (qsort) before the next lookup.
#pragma once

#include "ape/memory/ApeAllocator.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ape {

class ApeCompactArrayBase {
public:
    ApeCompactArrayBase(uint16_t elemSize, uint16_t capacity, ApeAllocator* allocator = nullptr);
    ~ApeCompactArrayBase();
    ApeCompactArrayBase(const ApeCompactArrayBase&) = delete;
    ApeCompactArrayBase& operator=(const ApeCompactArrayBase&) = delete;

    // Grows the storage to at least `minCapacity` (and by at least 16 elements).
    void Grow(uint32_t minCapacity);
    void Release();

    uint16_t Count() const { return m_count; }
    uint16_t Capacity() const { return m_capacity; }
    uint16_t PeakCount() const { return m_peak; }
    uint16_t ElementSize() const { return (uint16_t)(m_elemSizeAndDirty >> 1); }

protected:
    // Empty hook the original calls right after initialising an array.
    void OnInit();
    void* AppendRaw();
    bool IsDirty() const { return (m_elemSizeAndDirty & 1) != 0; }
    void SetDirty(bool dirty)
    {
        m_elemSizeAndDirty = (uint16_t)((m_elemSizeAndDirty & ~1u) | (dirty ? 1u : 0u));
    }

    uint16_t m_capacity = 0;
    uint16_t m_count = 0;
    uint16_t m_peak = 0;
    uint16_t m_elemSizeAndDirty = 0;   // element size << 1 | "needs sorting"
    void* m_data = nullptr;
    ApeAllocator* m_allocator = nullptr;
};

// Default key ordering used by every sorted map instantiation.
struct ApeSortedMapKey {
    uint32_t key;
};
int CompareSortedMapKeys(const void* a, const void* b);

template <class V>
class ApeTSortedMap : public ApeCompactArrayBase {
public:
    struct Pair {
        uint32_t key;
        V value;
    };
    using CompareFn = int (*)(const void*, const void*);

    explicit ApeTSortedMap(uint16_t capacity = 0, ApeAllocator* allocator = nullptr, V defaultValue = V())
        : ApeCompactArrayBase((uint16_t)sizeof(Pair), capacity, allocator), m_default(defaultValue)
    {
    }

    void Add(uint32_t key, V value)
    {
        Pair* p = static_cast<Pair*>(AppendRaw());
        p->key = key;
        p->value = value;
        SetDirty(true);
    }
    // Returns the value for `key`, or the default value's address if absent.
    V* Find(uint32_t key);
    // Removes `key` (every match the search meets, like the original).
    void Remove(uint32_t key);

    Pair* Data() { return static_cast<Pair*>(m_data); }
    void SetCompare(CompareFn compare) { m_compare = compare; }

private:
    void SortIfDirty();

    V m_default;
    CompareFn m_compare = &CompareSortedMapKeys;
};

template <class V>
void ApeTSortedMap<V>::SortIfDirty()
{
    if (!IsDirty())
        return;
    SetDirty(false);
    if (m_count)
        std::qsort(m_data, m_count, sizeof(Pair), m_compare);
}

template <class V>
V* ApeTSortedMap<V>::Find(uint32_t key)
{
    SortIfDirty();
    Pair* data = Data();
    int lo = 0, hi = (int)m_count - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) >> 1;
        int order;
        if (m_compare == &CompareSortedMapKeys) {
            order = key < data[mid].key ? -1 : (key > data[mid].key ? 1 : 0);
        } else {
            const ApeSortedMapKey probe = { key };
            order = -m_compare(&data[mid], &probe);
        }
        if (order < 0)
            hi = mid - 1;
        else if (order > 0)
            lo = mid + 1;
        else
            return &data[mid].value;
    }
    return &m_default;
}

template <class V>
void ApeTSortedMap<V>::Remove(uint32_t key)
{
    SortIfDirty();
    int lo = 0, hi = (int)m_count - 1;
    while (lo <= hi) {
        Pair* data = Data();
        const int mid = (lo + hi) >> 1;
        int order;
        if (m_compare == &CompareSortedMapKeys) {
            order = key < data[mid].key ? -1 : (key > data[mid].key ? 1 : 0);
        } else {
            const ApeSortedMapKey probe = { key };
            order = -m_compare(&data[mid], &probe);
        }
        if (order < 0) {
            hi = mid - 1;
            continue;
        }
        if (order == 0) {
            std::memmove(&data[mid], &data[mid + 1], (size_t)(m_count - 1 - mid) * sizeof(Pair));
            --m_count;
        }
        lo = mid + 1;
    }
}

} // namespace ape
