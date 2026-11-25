// openTIT - ApeTHandleList<T>: doubly linked list stored in a node pool.
//
// Nodes live in one array (grown by doubling when allowed); unused nodes are
// chained in a free list. Insertions return a 32-bit handle combining the
// node index (high 16 bits) and a global 16-bit serial number (low 16 bits),
// so stale handles can be detected after a node is reused.
#pragma once

#include "ape/memory/ApeMem.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace ape {

// Global serial shared by every handle list (0x010C29DC); 0 is never used.
uint32_t NextHandleListSerial();

template <class T>
class ApeTHandleList {
public:
    using Handle = uint32_t;
    static constexpr int32_t kNone = -1;

    explicit ApeTHandleList(int32_t capacity = 0) { Init(capacity); }
    ~ApeTHandleList() { MemFree(m_nodes, 0); }
    ApeTHandleList(const ApeTHandleList&) = delete;
    ApeTHandleList& operator=(const ApeTHandleList&) = delete;

    void Init(int32_t capacity);
    void Grow(int32_t capacity);
    // Appends `value`; returns 0 when the list is full and may not grow.
    Handle PushBack(const T& value);
    // Removes the first element (if any).
    void PopFront();
    // Value of a live handle, or null when the handle is stale or invalid.
    T* Get(Handle handle);
    // Removes the element of a live handle (stale handles are ignored).
    void Remove(Handle handle) { Erase(handle); }
    // Removes the element of a live handle and returns the handle of the
    // element that followed it (index -1 with serial 0 after the last one);
    // 0 for a stale or invalid handle.
    Handle Erase(Handle handle);
    // Puts every node back on the free list (storage is kept).
    void Clear();
    int32_t Capacity() const { return m_capacity; }

    int32_t Count() const { return m_count; }
    bool Empty() const { return m_count == 0; }
    T& Front() { return m_nodes[m_head].value; }
    void SetGrowable(bool growable) { m_growable = growable; }

    // Iteration by node index: for (i = FirstIndex(); i != kNone; i = NextIndex(i)).
    int32_t FirstIndex() const { return m_head; }
    int32_t NextIndex(int32_t i) const { return m_nodes[i].next; }
    T& At(int32_t i) { return m_nodes[i].value; }

private:
    struct Node {
        int32_t prev;
        int32_t next;
        T value;
        uint32_t flags;   // bit 31: free, bits 15..30: serial
    };

    void MarkFree(Node& n) { n.flags = (n.flags & 0x7FFFu) | 0x80000000u; }
    bool IsLive(Handle handle) const
    {
        const int32_t index = (int32_t)(handle >> 16);
        if ((handle & 0xFFFF) == 0 || index >= m_capacity)
            return false;
        const Node& n = m_nodes[index];
        return ((n.flags >> 15) & 0xFFFF) == (handle & 0xFFFF) && (n.flags & 0x80000000u) == 0;
    }
    void Unlink(int32_t index);
    void SetSerial(Node& n, uint32_t serial) { n.flags = (n.flags & 0x80007FFFu) | (serial << 15); }
    void LinkFreeRange(int32_t first, int32_t end);

    int32_t m_freeHead = 0;
    int32_t m_freeTail = 0;
    int32_t m_head = 0;
    int32_t m_tail = 0;
    int32_t m_count = 0;
    int32_t m_freeCount = 0;
    int32_t m_capacity = 0;
    Node* m_nodes = nullptr;
    bool m_growable = true;
};

template <class T>
void ApeTHandleList<T>::LinkFreeRange(int32_t first, int32_t end)
{
    for (int32_t i = first; i < end; ++i) {
        m_nodes[i].prev = i - 1;
        m_nodes[i].next = i + 1;
        MarkFree(m_nodes[i]);
        m_nodes[i].flags &= 0x80007FFFu;
    }
}

// Class element types (the anim set slots, the knowledge library's lists)
// get one heap-allocated element per node on the console, constructed with
// the node array and copied by pointer on growth; here the elements are
// stored in the nodes and value-initialised the same way.
// @func 0x00788EF4 ApeTHandleList<AnimSetSlot>::Init
template <class T>
void ApeTHandleList<T>::Init(int32_t capacity)
{
    m_freeHead = m_freeTail = m_head = m_tail = m_count = m_freeCount = 0;
    m_capacity = capacity;
    m_nodes = capacity ? static_cast<Node*>(MemAlloc((size_t)capacity * sizeof(Node), 0, 0x10)) : nullptr;
    m_growable = true;
    if (!capacity)
        return;
    for (int32_t i = 0; i < capacity; ++i)
        new (&m_nodes[i].value) T();
    LinkFreeRange(0, capacity);
    m_nodes[0].prev = kNone;
    m_nodes[capacity - 1].next = kNone;
    m_head = m_tail = kNone;
    m_freeHead = 0;
    m_freeTail = capacity > 1 ? capacity - 1 : kNone;
    m_count = 0;
    m_freeCount = capacity;
}

// NOTE: in the class-element instantiations the console copies the old
// nodes (element pointers included) over the new ones and then deletes the
// old elements, leaving the copies dangling; both such lists are created
// non-growable, so it never matters. Here the elements are copied by value.
// @func 0x007893B0 ApeTHandleList<AnimSetSlot>::Grow
template <class T>
void ApeTHandleList<T>::Grow(int32_t capacity)
{
    if (capacity == m_capacity)
        return;
    Node* nodes = static_cast<Node*>(MemAlloc((size_t)capacity * sizeof(Node), 0, 0x10));
    if (m_nodes)
        std::memcpy(nodes, m_nodes, (size_t)m_capacity * sizeof(Node));
    MemFree(m_nodes, 0);
    m_nodes = nodes;
    for (int32_t i = m_capacity; i < capacity; ++i)
        new (&m_nodes[i].value) T();

    const int32_t oldCapacity = m_capacity;
    LinkFreeRange(oldCapacity, capacity);
    m_nodes[oldCapacity].prev = m_freeTail;
    if (m_freeCount)
        m_nodes[m_freeTail].next = oldCapacity;
    else
        m_freeHead = oldCapacity;
    m_nodes[capacity - 1].next = kNone;
    m_freeTail = capacity - 1;
    m_freeCount += capacity - oldCapacity;
    m_capacity = capacity;
}

template <class T>
typename ApeTHandleList<T>::Handle ApeTHandleList<T>::PushBack(const T& value)
{
    if (m_count == m_capacity) {
        if (!m_growable)
            return 0;
        Grow(m_capacity * 2);
    }

    const int32_t index = m_freeHead;
    Node& node = m_nodes[index];
    if (m_count == 0) {
        if (m_capacity != 1)
            m_nodes[node.next].prev = kNone;
        m_head = m_tail = index;
        m_freeHead = node.next;
        node.prev = kNone;
        node.next = kNone;
    } else {
        m_freeHead = (m_freeCount == 1) ? kNone : node.next;
        if (m_freeCount == 1)
            m_freeTail = kNone;
        node.prev = m_tail;
        node.next = kNone;
        m_nodes[m_tail].next = index;
        m_tail = index;
    }
    node.value = value;
    node.flags &= 0x7FFFFFFFu;
    ++m_count;
    --m_freeCount;

    const uint32_t serial = NextHandleListSerial();
    SetSerial(node, serial);
    return (serial & 0xFFFF) | ((uint32_t)index << 16);
}

template <class T>
void ApeTHandleList<T>::PopFront()
{
    if (!m_count)
        return;
    Unlink(m_head);
}

template <class T>
T* ApeTHandleList<T>::Get(Handle handle)
{
    return IsLive(handle) ? &m_nodes[handle >> 16].value : nullptr;
}

template <class T>
typename ApeTHandleList<T>::Handle ApeTHandleList<T>::Erase(Handle handle)
{
    if (!m_count || !IsLive(handle))
        return 0;
    const int32_t index = (int32_t)(handle >> 16);
    const Node& node = m_nodes[index];
    Handle next;
    if (index == m_tail)
        next = 0xFFFF0000u;
    else
        next = ((uint32_t)node.next << 16) | ((m_nodes[node.next].flags >> 15) & 0xFFFFu);
    Unlink(index);
    return next;
}

template <class T>
void ApeTHandleList<T>::Clear()
{
    if (!m_capacity)
        return;
    LinkFreeRange(0, m_capacity);
    m_nodes[0].prev = kNone;
    m_nodes[m_capacity - 1].next = kNone;
    m_head = m_tail = kNone;
    m_freeHead = 0;
    m_freeTail = m_capacity > 1 ? m_capacity - 1 : kNone;
    m_count = 0;
    m_freeCount = m_capacity;
}

template <class T>
void ApeTHandleList<T>::Unlink(int32_t index)
{
    Node& node = m_nodes[index];
    // Unlink from the used list.
    if (index == m_head)
        m_head = node.next;
    else
        m_nodes[node.prev].next = node.next;
    if (index == m_tail)
        m_tail = node.prev;
    else
        m_nodes[node.next].prev = node.prev;

    // Append to the free list.
    if (m_freeTail == kNone) {
        node.prev = kNone;
        node.next = kNone;
        m_freeHead = index;
    } else {
        m_nodes[m_freeTail].next = index;
        node.prev = m_freeTail;
        node.next = kNone;
        if (m_capacity == 1)
            m_freeHead = index;
    }
    MarkFree(node);
    m_freeTail = index;

    --m_count;
    ++m_freeCount;
    if (m_count == 0)
        m_head = m_tail = kNone;
}

} // namespace ape
