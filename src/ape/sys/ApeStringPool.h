// openTIT - Packed string pool (0x00565CC8-0x00566F1C).
//
// Strings are appended back to back (each with its terminator) into one
// buffer; the pool can optionally double its buffer when it runs out.
// Storage comes from the default heap, or from memory class 1 when
// `altHeap` is set. The total capacity of all pools is tracked for memory
// reports.
#pragma once

#include <cstdint>

namespace ape {

class ApeStringPool {
public:
    ApeStringPool(int capacity, bool growable, bool altHeap);
    ~ApeStringPool() { Release(); }

    ApeStringPool(const ApeStringPool&) = delete;
    ApeStringPool& operator=(const ApeStringPool&) = delete;

    // Empties the pool; reallocates when the capacity changes.
    void Reset(int capacity);
    void Release();
    // Grows the buffer to `capacity` bytes, keeping the contents.
    void Grow(int capacity);
    // Appends a copy of `str` and returns it, or null when it did not fit
    // (the truncated text is still terminated at the end of the buffer).
    char* Add(const char* str);

    const char* Data() const { return m_data; }
    int Used() const { return m_used; }
    int Capacity() const { return m_capacity; }
    int Count() const { return m_count; }

    // Sum of the capacities of all pools (0x01164DFC).
    static int TotalCapacity();

private:
    char* Allocate(int size) const;
    void Free(char* data) const;

    char* m_data = nullptr;   // +0x00
    int m_used = 0;           // +0x04
    int m_capacity = 0;       // +0x08
    bool m_growable = false;  // +0x0C
    int m_count = 0;          // +0x10 strings added
    bool m_altHeap = false;   // +0x14
};

} // namespace ape
