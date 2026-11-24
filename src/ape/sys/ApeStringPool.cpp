// openTIT - Packed string pool.
#include "ape/sys/ApeStringPool.h"

#include "ape/memory/ApeMem.h"

#include <cstring>

namespace ape {

namespace {
int s_totalCapacity = 0;
}

int ApeStringPool::TotalCapacity()
{
    return s_totalCapacity;
}

char* ApeStringPool::Allocate(int size) const
{
    if (m_altHeap)
        return static_cast<char*>(SysAlloc(16, (size_t)size, 1, 0));
    return static_cast<char*>(MemAlloc((size_t)size, 0, 16));
}

void ApeStringPool::Free(char* data) const
{
    SysFree(data, m_altHeap ? 1 : 0, 0);
}

// @func 0x00565CC8 ApeStringPool::ApeStringPool
ApeStringPool::ApeStringPool(int capacity, bool growable, bool altHeap)
    : m_capacity(capacity), m_growable(growable), m_altHeap(altHeap)
{
    m_data = Allocate(capacity);
    s_totalCapacity += m_capacity;
}

// @func 0x00566318 ApeStringPool::Reset
void ApeStringPool::Reset(int capacity)
{
    if (m_data) {
        if (m_capacity == capacity) {
            m_used = 0;
            m_count = 0;
            return;
        }
        Free(m_data);
        m_data = nullptr;
        s_totalCapacity -= m_capacity;
    }
    m_capacity = capacity;
    m_used = 0;
    m_count = 0;
    m_data = Allocate(capacity);
    s_totalCapacity += m_capacity;
}

// NOTE: unlike Reset, the console does not take the pool out of the total.
// @func 0x005662A0 ApeStringPool::Release
void ApeStringPool::Release()
{
    if (!m_data)
        return;
    m_capacity = 0;
    Free(m_data);
    m_data = nullptr;
}

// NOTE: with altHeap the console allocates the old capacity instead of the
// new one (and then copies the old contents), so the new buffer is too small;
// the requested size is used here.
// @func 0x00566950 ApeStringPool::Grow
void ApeStringPool::Grow(int capacity)
{
    if (capacity <= m_capacity)
        return;
    char* data = Allocate(capacity);
    std::memcpy(data, m_data, (size_t)m_capacity);
    Free(m_data);
    m_data = data;
    m_capacity = capacity;
}

// @func 0x00566F1C ApeStringPool::Add
char* ApeStringPool::Add(const char* str)
{
    const int start = m_used;
    int pos = m_used;
    for (; *str; ++str) {
        if (pos >= m_capacity - 1) {
            if (!m_growable)
                break;
            Grow(m_capacity * 2);
            pos = m_used;
        }
        m_data[pos] = *str;
        pos = ++m_used;
    }

    if (pos < m_capacity) {
        m_data[pos] = '\0';
        ++m_used;
        ++m_count;
        return m_data + start;
    }
    m_data[m_capacity - 1] = '\0';
    return nullptr;
}

} // namespace ape
