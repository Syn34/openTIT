// openTIT - ApeTFreeList: fixed-size element pool with intrusive used/free lists.
//
// Elements are carved out of blocks requested from an ApeAllocator. Every
// element is preceded by a 16-byte node header that links it into either the
// "used" list or the "free" list, so live elements can be iterated without any
// extra bookkeeping:
//     for (auto* n = list.First(); n; n = n->next) Use(list.NodeData(n));
// Blocks are never returned while the pool lives, except by
// ReleaseUnusedBlocks()/CompactAll() which free blocks with no live element.
// The first block can be marked permanent so it survives compaction.
//
// Every initialised pool registers itself in a global registry so that
// ApeFreeList::CompactAll() can trim all pools at once.
#pragma once

#include "ape/memory/ApeAllocator.h"
#include "ape/memory/ApeMem.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

namespace ape {

class ApeFreeList : public ApeMemObject {
public:
    struct alignas(16) Node {
        Node* next;
        Node* prev;
    };

    ApeFreeList() = default;
    virtual ~ApeFreeList();

    ApeFreeList(const ApeFreeList&) = delete;
    ApeFreeList& operator=(const ApeFreeList&) = delete;

    // Sets up the pool for elements of 'elemSize' bytes and pre-allocates one
    // block of 'count' elements (none when count is 0). Later blocks hold
    // count/4 elements (at least 16) for big pools, 'count' otherwise.
    // A null allocator selects GetDefaultAllocator().
    virtual void Init(size_t elemSize, uint32_t count, ApeAllocator* allocator, bool permanentFirstBlock);
    // Returns a zero-filled element (grows the pool if needed).
    virtual void* Alloc();
    // Returns an element obtained from Alloc() to the pool.
    virtual void Free(void* element);

    // Releases every block and unregisters the pool. Outstanding elements
    // become invalid. The pool must be Init()ed again before reuse.
    void Destroy();
    // Frees all non-permanent blocks that hold no live element.
    void ReleaseUnusedBlocks();
    // ReleaseUnusedBlocks() on every registered pool.
    static void CompactAll();

    // Head of the list of live elements (most recently allocated first).
    Node* First() const;
    // Oldest live element (the tail of the used list); walk with node->prev.
    Node* Last() const { return m_used.tail; }
    static void* NodeData(Node* node) { return node ? node + 1 : nullptr; }
    static Node* NodeOf(void* element) { return static_cast<Node*>(element) - 1; }

    uint32_t UsedCount() const { return m_used.count; }
    uint32_t FreeCount() const { return m_free.count; }
    uint32_t BlockCount() const { return m_blocks.count; }
    uint32_t PeakUsed() const { return m_peakUsed; }
    size_t ElementSize() const { return m_elemSize; }
    const char* Name() const { return m_name; }
    void SetName(const char* name) { m_name = name; }

private:
    struct alignas(16) Block {
        Block* next;
        Block* prev;
        size_t size;     // bytes from the block start to the end of its last node
        uint32_t flags;  // kBlockPermanent
    };
    static constexpr uint32_t kBlockPermanent = 1;

    template <class T>
    struct List {
        T* head = nullptr;
        T* tail = nullptr;
        uint32_t count = 0;

        void PushFront(T* item)
        {
            item->prev = nullptr;
            item->next = head;
            if (head)
                head->prev = item;
            head = item;
            if (!tail)
                tail = item;
            ++count;
        }
        void PushBack(T* item)
        {
            item->next = nullptr;
            item->prev = tail;
            if (tail)
                tail->next = item;
            tail = item;
            if (!head)
                head = item;
            ++count;
        }
        void Remove(T* item)
        {
            if (item->prev)
                item->prev->next = item->next;
            else
                head = item->next;
            if (item->next)
                item->next->prev = item->prev;
            else
                tail = item->prev;
            item->prev = nullptr;
            --count;
        }
        void Reset() { head = tail = nullptr; count = 0; }
    };

    size_t NodeStride() const;
    void AddBlock(bool permanent);
    static void Register(ApeFreeList* list);
    static void Unregister(ApeFreeList* list);

    List<Node> m_used;
    List<Node> m_free;
    List<Block> m_blocks;
    ApeAllocator* m_allocator = nullptr;
    size_t m_elemSize = 0;
    uint32_t m_peakUsed = 0;
    uint16_t m_growCount = 0;
    // NOTE: the console object also has a flag (bit 15 of the halfword after
    // the grow count) that every constructor clears and nothing here reads;
    // it is not reproduced.
    const char* m_name = "";
};

// Typed pool. Alloc() returns raw zeroed storage, New()/Delete() also run
// T's constructor/destructor. The element size is fixed by T: the untyped
// Init() of the base class is disabled and Init(count, allocator) replaces it.
//
// The console has one copy of every member per element type (43 pools);
// the engine-side instantiations release the pool object through
// ApeMemObject's operator delete (SysFree), the game-side ones through
// the global one (MemFree). Here they all use ApeMemObject's.
template <class T>
class ApeTFreeList : public ApeFreeList {
public:
    ApeTFreeList() = default;
    explicit ApeTFreeList(uint32_t count, ApeAllocator* allocator = nullptr)
    {
        ApeFreeList::Init(sizeof(T), count, allocator, false);
    }
    ~ApeTFreeList() override;

    void Init(size_t elemSize, uint32_t count, ApeAllocator* allocator, bool permanentFirstBlock) override;
    void* Alloc() override;
    void Free(void* element) override;
    // Sets up the pool for elements of type T (see ApeFreeList::Init).
    virtual void Init(uint32_t count, ApeAllocator* allocator = nullptr);

    T* AllocT() { return static_cast<T*>(ApeFreeList::Alloc()); }

    template <class... Args>
    T* New(Args&&... args)
    {
        void* storage = ApeFreeList::Alloc();
        return storage ? new (storage) T(std::forward<Args>(args)...) : nullptr;
    }

    void Delete(T* object)
    {
        if (!object)
            return;
        object->~T();
        ApeFreeList::Free(object);
    }

    static T* Data(Node* node) { return static_cast<T*>(NodeData(node)); }

    template <class Fn>
    void ForEach(Fn&& fn)
    {
        for (Node* node = First(); node;) {
            Node* next = node->next;   // fn may free the element
            fn(Data(node));
            node = next;
        }
    }
};

// Per-type shared pool, created on first use (ApeTListAllocator<T> in the
// original). SetInitialCount() must be called before the first
// GetAllocator() to change the size of the first block.
template <class T>
class ApeTListAllocator {
public:
    static ApeTFreeList<T>& GetAllocator()
    {
        State& state = GetState();
        if (!state.list) {
            state.list = new ApeTFreeList<T>();
            state.list->SetName("ApeTListAllocator<T>::GetAllocator()");
            state.list->Init(state.initialCount);
        }
        return *state.list;
    }

    static void SetInitialCount(uint32_t count) { GetState().initialCount = count; }

private:
    struct State {
        ApeTFreeList<T>* list = nullptr;
        uint32_t initialCount = 16;
    };
    static State& GetState()
    {
        static State s_state;
        return s_state;
    }
};

// --- ApeTFreeList<T> members ---

// @func 0x00736ADC ApeTFreeList<T>::~ApeTFreeList (copy 1, sizeof(T) 0x18)
// @func 0x00736BD8 ApeTFreeList<T>::~ApeTFreeList (copy 2, sizeof(T) 0x98)
// @func 0x00736CD4 ApeTFreeList<T>::~ApeTFreeList (copy 3, sizeof(T) 0x14)
// @func 0x00736E00 ApeTFreeList<T>::~ApeTFreeList (copy 4, sizeof(T) 0x14)
// @func 0x00736EFC ApeTFreeList<T>::~ApeTFreeList (copy 5, sizeof(T) 0x120)
// @func 0x00737024 ApeTFreeList<T>::~ApeTFreeList (copy 6, sizeof(T) 0x160)
// @func 0x00737120 ApeTFreeList<T>::~ApeTFreeList (copy 7, sizeof(T) 0x80)
// @func 0x0073B938 ApeTFreeList<T>::~ApeTFreeList (copy 8, sizeof(T) 0x14)
// @func 0x0073BA68 ApeTFreeList<T>::~ApeTFreeList (copy 9, sizeof(T) 0x14)
// @func 0x0073BB30 ApeTFreeList<T>::~ApeTFreeList (copy 10, sizeof(T) 0x90)
// @func 0x0073BC28 ApeTFreeList<T>::~ApeTFreeList (copy 11, sizeof(T) 0x160)
// @func 0x00746D88 ApeTFreeList<T>::~ApeTFreeList (copy 12, sizeof(T) 0x120)
// @func 0x00746E50 ApeTFreeList<T>::~ApeTFreeList (copy 13, sizeof(T) 0x190)
// @func 0x007492B4 ApeTFreeList<T>::~ApeTFreeList (copy 14, sizeof(T) 0xC0)
// @func 0x00770910 ApeTFreeList<T>::~ApeTFreeList (copy 15, sizeof(T) 0x14)
// @func 0x0077D82C ApeTFreeList<T>::~ApeTFreeList (copy 16, sizeof(T) 0x14)
// @func 0x00794528 ApeTFreeList<T>::~ApeTFreeList (copy 17, sizeof(T) 0x14)
// @func 0x007945F0 ApeTFreeList<T>::~ApeTFreeList (copy 18, sizeof(T) 0x14)
// @func 0x007946B8 ApeTFreeList<T>::~ApeTFreeList (copy 19, sizeof(T) 0x14)
// @func 0x00794810 ApeTFreeList<T>::~ApeTFreeList (copy 20, sizeof(T) 0x14)
// @func 0x0079A038 ApeTFreeList<T>::~ApeTFreeList (copy 21, sizeof(T) 0x14)
// @func 0x0079A100 ApeTFreeList<T>::~ApeTFreeList (copy 22, sizeof(T) 0x14)
// @func 0x007A23D4 ApeTFreeList<T>::~ApeTFreeList (copy 23, sizeof(T) 0x9C)
// @func 0x007A249C ApeTFreeList<T>::~ApeTFreeList (copy 24, sizeof(T) 0x84)
// @func 0x007A2564 ApeTFreeList<T>::~ApeTFreeList (copy 25, sizeof(T) 0x730)
// @func 0x007A262C ApeTFreeList<T>::~ApeTFreeList (copy 26, sizeof(T) 0x74)
// @func 0x007A26F4 ApeTFreeList<T>::~ApeTFreeList (copy 27, sizeof(T) 0x70)
// @func 0x007A27BC ApeTFreeList<T>::~ApeTFreeList (copy 28, sizeof(T) 0x140)
// @func 0x007A2884 ApeTFreeList<T>::~ApeTFreeList (copy 29, sizeof(T) 0x90)
// @func 0x007A294C ApeTFreeList<T>::~ApeTFreeList (copy 30, sizeof(T) 0xF0)
// @func 0x007A2A14 ApeTFreeList<T>::~ApeTFreeList (copy 31, sizeof(T) 0x8C)
// @func 0x007A2ADC ApeTFreeList<T>::~ApeTFreeList (copy 32, sizeof(T) 0xC)
// @func 0x007A4E80 ApeTFreeList<T>::~ApeTFreeList (copy 33, sizeof(T) 0x128)
// @func 0x007A4F3C ApeTFreeList<T>::~ApeTFreeList (copy 34, sizeof(T) 0x7C)
// @func 0x007A4FF8 ApeTFreeList<T>::~ApeTFreeList (copy 35, sizeof(T) 0x14)
// @func 0x007AA12C ApeTFreeList<T>::~ApeTFreeList (copy 36, sizeof(T) 0x70)
// @func 0x007AA224 ApeTFreeList<T>::~ApeTFreeList (copy 37, sizeof(T) 0x8)
// @func 0x007AA2EC ApeTFreeList<T>::~ApeTFreeList (copy 38, sizeof(T) 0xB0)
// @func 0x007AA3E4 ApeTFreeList<T>::~ApeTFreeList (copy 39, sizeof(T) 0xB0)
// @func 0x007AA4AC ApeTFreeList<T>::~ApeTFreeList (copy 40, sizeof(T) 0x80)
// @func 0x007AA5A4 ApeTFreeList<T>::~ApeTFreeList (copy 41, sizeof(T) 0x70)
// @func 0x007AA69C ApeTFreeList<T>::~ApeTFreeList (copy 42, sizeof(T) 0x160)
// @func 0x007AA764 ApeTFreeList<T>::~ApeTFreeList (copy 43, sizeof(T) 0x40)
// @func 0x00736B30 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 1, sizeof(T) 0x18)
// @func 0x00736C2C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 2, sizeof(T) 0x98)
// @func 0x00736D28 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 3, sizeof(T) 0x14)
// @func 0x00736E54 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 4, sizeof(T) 0x14)
// @func 0x00736F50 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 5, sizeof(T) 0x120)
// @func 0x00737078 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 6, sizeof(T) 0x160)
// @func 0x00737174 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 7, sizeof(T) 0x80)
// @func 0x0073B98C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 8, sizeof(T) 0x14)
// @func 0x0073BABC ApeTFreeList<T>::~ApeTFreeList (deleting, copy 9, sizeof(T) 0x14)
// @func 0x0073BB84 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 10, sizeof(T) 0x90)
// @func 0x0073BC7C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 11, sizeof(T) 0x160)
// @func 0x00746DDC ApeTFreeList<T>::~ApeTFreeList (deleting, copy 12, sizeof(T) 0x120)
// @func 0x00746EA4 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 13, sizeof(T) 0x190)
// @func 0x00749308 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 14, sizeof(T) 0xC0)
// @func 0x00770964 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 15, sizeof(T) 0x14)
// @func 0x0077D880 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 16, sizeof(T) 0x14)
// @func 0x0079457C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 17, sizeof(T) 0x14)
// @func 0x00794644 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 18, sizeof(T) 0x14)
// @func 0x0079470C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 19, sizeof(T) 0x14)
// @func 0x00794864 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 20, sizeof(T) 0x14)
// @func 0x0079A08C ApeTFreeList<T>::~ApeTFreeList (deleting, copy 21, sizeof(T) 0x14)
// @func 0x0079A154 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 22, sizeof(T) 0x14)
// @func 0x007A2428 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 23, sizeof(T) 0x9C)
// @func 0x007A24F0 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 24, sizeof(T) 0x84)
// @func 0x007A25B8 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 25, sizeof(T) 0x730)
// @func 0x007A2680 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 26, sizeof(T) 0x74)
// @func 0x007A2748 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 27, sizeof(T) 0x70)
// @func 0x007A2810 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 28, sizeof(T) 0x140)
// @func 0x007A28D8 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 29, sizeof(T) 0x90)
// @func 0x007A29A0 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 30, sizeof(T) 0xF0)
// @func 0x007A2A68 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 31, sizeof(T) 0x8C)
// @func 0x007A2B30 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 32, sizeof(T) 0xC)
// @func 0x007A4ECC ApeTFreeList<T>::~ApeTFreeList (deleting, copy 33, sizeof(T) 0x128)
// @func 0x007A4F88 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 34, sizeof(T) 0x7C)
// @func 0x007A5044 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 35, sizeof(T) 0x14)
// @func 0x007AA180 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 36, sizeof(T) 0x70)
// @func 0x007AA278 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 37, sizeof(T) 0x8)
// @func 0x007AA340 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 38, sizeof(T) 0xB0)
// @func 0x007AA438 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 39, sizeof(T) 0xB0)
// @func 0x007AA500 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 40, sizeof(T) 0x80)
// @func 0x007AA5F8 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 41, sizeof(T) 0x70)
// @func 0x007AA6F0 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 42, sizeof(T) 0x160)
// @func 0x007AA7B8 ApeTFreeList<T>::~ApeTFreeList (deleting, copy 43, sizeof(T) 0x40)
template <class T>
ApeTFreeList<T>::~ApeTFreeList()
{
    Destroy();
}

// @func 0x00736BA0 ApeTFreeList<T>::Init(untyped) (copy 1, sizeof(T) 0x18)
// @func 0x00736C9C ApeTFreeList<T>::Init(untyped) (copy 2, sizeof(T) 0x98)
// @func 0x00736D98 ApeTFreeList<T>::Init(untyped) (copy 3, sizeof(T) 0x14)
// @func 0x00736EC4 ApeTFreeList<T>::Init(untyped) (copy 4, sizeof(T) 0x14)
// @func 0x00736FC0 ApeTFreeList<T>::Init(untyped) (copy 5, sizeof(T) 0x120)
// @func 0x007370E8 ApeTFreeList<T>::Init(untyped) (copy 6, sizeof(T) 0x160)
// @func 0x007371E4 ApeTFreeList<T>::Init(untyped) (copy 7, sizeof(T) 0x80)
// @func 0x0073B9FC ApeTFreeList<T>::Init(untyped) (copy 8, sizeof(T) 0x14)
// @func 0x0073BB2C ApeTFreeList<T>::Init(untyped) (copy 9, sizeof(T) 0x14)
// @func 0x0073BBF4 ApeTFreeList<T>::Init(untyped) (copy 10, sizeof(T) 0x90)
// @func 0x0073BCEC ApeTFreeList<T>::Init(untyped) (copy 11, sizeof(T) 0x160)
// @func 0x00746E4C ApeTFreeList<T>::Init(untyped) (copy 12, sizeof(T) 0x120)
// @func 0x00746F14 ApeTFreeList<T>::Init(untyped) (copy 13, sizeof(T) 0x190)
// @func 0x00749378 ApeTFreeList<T>::Init(untyped) (copy 14, sizeof(T) 0xC0)
// @func 0x007709D4 ApeTFreeList<T>::Init(untyped) (copy 15, sizeof(T) 0x14)
// @func 0x0077D8F0 ApeTFreeList<T>::Init(untyped) (copy 16, sizeof(T) 0x14)
// @func 0x007945EC ApeTFreeList<T>::Init(untyped) (copy 17, sizeof(T) 0x14)
// @func 0x007946B4 ApeTFreeList<T>::Init(untyped) (copy 18, sizeof(T) 0x14)
// @func 0x0079477C ApeTFreeList<T>::Init(untyped) (copy 19, sizeof(T) 0x14)
// @func 0x007948D4 ApeTFreeList<T>::Init(untyped) (copy 20, sizeof(T) 0x14)
// @func 0x0079A0FC ApeTFreeList<T>::Init(untyped) (copy 21, sizeof(T) 0x14)
// @func 0x0079A1C4 ApeTFreeList<T>::Init(untyped) (copy 22, sizeof(T) 0x14)
// @func 0x007A2498 ApeTFreeList<T>::Init(untyped) (copy 23, sizeof(T) 0x9C)
// @func 0x007A2560 ApeTFreeList<T>::Init(untyped) (copy 24, sizeof(T) 0x84)
// @func 0x007A2628 ApeTFreeList<T>::Init(untyped) (copy 25, sizeof(T) 0x730)
// @func 0x007A26F0 ApeTFreeList<T>::Init(untyped) (copy 26, sizeof(T) 0x74)
// @func 0x007A27B8 ApeTFreeList<T>::Init(untyped) (copy 27, sizeof(T) 0x70)
// @func 0x007A2880 ApeTFreeList<T>::Init(untyped) (copy 28, sizeof(T) 0x140)
// @func 0x007A2948 ApeTFreeList<T>::Init(untyped) (copy 29, sizeof(T) 0x90)
// @func 0x007A2A10 ApeTFreeList<T>::Init(untyped) (copy 30, sizeof(T) 0xF0)
// @func 0x007A2AD8 ApeTFreeList<T>::Init(untyped) (copy 31, sizeof(T) 0x8C)
// @func 0x007A2BA0 ApeTFreeList<T>::Init(untyped) (copy 32, sizeof(T) 0xC)
// @func 0x007A4F38 ApeTFreeList<T>::Init(untyped) (copy 33, sizeof(T) 0x128)
// @func 0x007A4FF4 ApeTFreeList<T>::Init(untyped) (copy 34, sizeof(T) 0x7C)
// @func 0x007A50B0 ApeTFreeList<T>::Init(untyped) (copy 35, sizeof(T) 0x14)
// @func 0x007AA1F0 ApeTFreeList<T>::Init(untyped) (copy 36, sizeof(T) 0x70)
// @func 0x007AA2E8 ApeTFreeList<T>::Init(untyped) (copy 37, sizeof(T) 0x8)
// @func 0x007AA3B0 ApeTFreeList<T>::Init(untyped) (copy 38, sizeof(T) 0xB0)
// @func 0x007AA4A8 ApeTFreeList<T>::Init(untyped) (copy 39, sizeof(T) 0xB0)
// @func 0x007AA570 ApeTFreeList<T>::Init(untyped) (copy 40, sizeof(T) 0x80)
// @func 0x007AA668 ApeTFreeList<T>::Init(untyped) (copy 41, sizeof(T) 0x70)
// @func 0x007AA760 ApeTFreeList<T>::Init(untyped) (copy 42, sizeof(T) 0x160)
// @func 0x007AA828 ApeTFreeList<T>::Init(untyped) (copy 43, sizeof(T) 0x40)
template <class T>
void ApeTFreeList<T>::Init(size_t /*elemSize*/, uint32_t /*count*/, ApeAllocator* /*allocator*/,
                           bool /*permanentFirstBlock*/)
{
    // Intentionally empty: a typed pool cannot be re-initialised with
    // another element size.
}

// @func 0x007338D4 ApeTFreeList<T>::Alloc (copy 1, sizeof(T) 0x18)
// @func 0x0073391C ApeTFreeList<T>::Alloc (copy 2, sizeof(T) 0x98)
// @func 0x00733B84 ApeTFreeList<T>::Alloc (copy 3, sizeof(T) 0x14)
// @func 0x00733BCC ApeTFreeList<T>::Alloc (copy 4, sizeof(T) 0x14)
// @func 0x00733C14 ApeTFreeList<T>::Alloc (copy 5, sizeof(T) 0x120)
// @func 0x00733C5C ApeTFreeList<T>::Alloc (copy 6, sizeof(T) 0x160)
// @func 0x00733CA4 ApeTFreeList<T>::Alloc (copy 7, sizeof(T) 0x80)
// @func 0x00739B2C ApeTFreeList<T>::Alloc (copy 8, sizeof(T) 0x14)
// @func 0x00739B74 ApeTFreeList<T>::Alloc (copy 9, sizeof(T) 0x14)
// @func 0x00739BBC ApeTFreeList<T>::Alloc (copy 10, sizeof(T) 0x90)
// @func 0x00739C04 ApeTFreeList<T>::Alloc (copy 11, sizeof(T) 0x160)
// @func 0x007458DC ApeTFreeList<T>::Alloc (copy 12, sizeof(T) 0x120)
// @func 0x00745924 ApeTFreeList<T>::Alloc (copy 13, sizeof(T) 0x190)
// @func 0x00748A40 ApeTFreeList<T>::Alloc (copy 14, sizeof(T) 0xC0)
// @func 0x0076E3B8 ApeTFreeList<T>::Alloc (copy 15, sizeof(T) 0x14)
// @func 0x0077AD40 ApeTFreeList<T>::Alloc (copy 16, sizeof(T) 0x14)
// @func 0x00793BC4 ApeTFreeList<T>::Alloc (copy 17, sizeof(T) 0x14)
// @func 0x00793C0C ApeTFreeList<T>::Alloc (copy 18, sizeof(T) 0x14)
// @func 0x00793C54 ApeTFreeList<T>::Alloc (copy 19, sizeof(T) 0x14)
// @func 0x00793C9C ApeTFreeList<T>::Alloc (copy 20, sizeof(T) 0x14)
// @func 0x00799F10 ApeTFreeList<T>::Alloc (copy 21, sizeof(T) 0x14)
// @func 0x00799F58 ApeTFreeList<T>::Alloc (copy 22, sizeof(T) 0x14)
// @func 0x0079E9F4 ApeTFreeList<T>::Alloc (copy 23, sizeof(T) 0x9C)
// @func 0x0079EA3C ApeTFreeList<T>::Alloc (copy 24, sizeof(T) 0x84)
// @func 0x0079EA84 ApeTFreeList<T>::Alloc (copy 25, sizeof(T) 0x730)
// @func 0x0079EACC ApeTFreeList<T>::Alloc (copy 26, sizeof(T) 0x74)
// @func 0x0079EB14 ApeTFreeList<T>::Alloc (copy 27, sizeof(T) 0x70)
// @func 0x0079EB5C ApeTFreeList<T>::Alloc (copy 28, sizeof(T) 0x140)
// @func 0x0079EBA4 ApeTFreeList<T>::Alloc (copy 29, sizeof(T) 0x90)
// @func 0x0079EBEC ApeTFreeList<T>::Alloc (copy 30, sizeof(T) 0xF0)
// @func 0x0079EC34 ApeTFreeList<T>::Alloc (copy 31, sizeof(T) 0x8C)
// @func 0x0079EC7C ApeTFreeList<T>::Alloc (copy 32, sizeof(T) 0xC)
// @func 0x007A4A90 ApeTFreeList<T>::Alloc (copy 33, sizeof(T) 0x128)
// @func 0x007A4AD0 ApeTFreeList<T>::Alloc (copy 34, sizeof(T) 0x7C)
// @func 0x007A4B10 ApeTFreeList<T>::Alloc (copy 35, sizeof(T) 0x14)
// @func 0x007A94E8 ApeTFreeList<T>::Alloc (copy 36, sizeof(T) 0x70)
// @func 0x007A9714 ApeTFreeList<T>::Alloc (copy 37, sizeof(T) 0x8)
// @func 0x007A975C ApeTFreeList<T>::Alloc (copy 38, sizeof(T) 0xB0)
// @func 0x007A97A4 ApeTFreeList<T>::Alloc (copy 39, sizeof(T) 0xB0)
// @func 0x007A97EC ApeTFreeList<T>::Alloc (copy 40, sizeof(T) 0x80)
// @func 0x007A9834 ApeTFreeList<T>::Alloc (copy 41, sizeof(T) 0x70)
// @func 0x007A987C ApeTFreeList<T>::Alloc (copy 42, sizeof(T) 0x160)
// @func 0x007A98C4 ApeTFreeList<T>::Alloc (copy 43, sizeof(T) 0x40)
template <class T>
void* ApeTFreeList<T>::Alloc()
{
    return ApeFreeList::Alloc();
}

// @func 0x007338F8 ApeTFreeList<T>::Free (copy 1, sizeof(T) 0x18)
// @func 0x00733940 ApeTFreeList<T>::Free (copy 2, sizeof(T) 0x98)
// @func 0x00733BA8 ApeTFreeList<T>::Free (copy 3, sizeof(T) 0x14)
// @func 0x00733BF0 ApeTFreeList<T>::Free (copy 4, sizeof(T) 0x14)
// @func 0x00733C38 ApeTFreeList<T>::Free (copy 5, sizeof(T) 0x120)
// @func 0x00733C80 ApeTFreeList<T>::Free (copy 6, sizeof(T) 0x160)
// @func 0x00733CC8 ApeTFreeList<T>::Free (copy 7, sizeof(T) 0x80)
// @func 0x00739B50 ApeTFreeList<T>::Free (copy 8, sizeof(T) 0x14)
// @func 0x00739B98 ApeTFreeList<T>::Free (copy 9, sizeof(T) 0x14)
// @func 0x00739BE0 ApeTFreeList<T>::Free (copy 10, sizeof(T) 0x90)
// @func 0x00739C28 ApeTFreeList<T>::Free (copy 11, sizeof(T) 0x160)
// @func 0x00745900 ApeTFreeList<T>::Free (copy 12, sizeof(T) 0x120)
// @func 0x00745948 ApeTFreeList<T>::Free (copy 13, sizeof(T) 0x190)
// @func 0x00748A64 ApeTFreeList<T>::Free (copy 14, sizeof(T) 0xC0)
// @func 0x0076E3DC ApeTFreeList<T>::Free (copy 15, sizeof(T) 0x14)
// @func 0x0077AD64 ApeTFreeList<T>::Free (copy 16, sizeof(T) 0x14)
// @func 0x00793BE8 ApeTFreeList<T>::Free (copy 17, sizeof(T) 0x14)
// @func 0x00793C30 ApeTFreeList<T>::Free (copy 18, sizeof(T) 0x14)
// @func 0x00793C78 ApeTFreeList<T>::Free (copy 19, sizeof(T) 0x14)
// @func 0x00793CC0 ApeTFreeList<T>::Free (copy 20, sizeof(T) 0x14)
// @func 0x00799F34 ApeTFreeList<T>::Free (copy 21, sizeof(T) 0x14)
// @func 0x00799F7C ApeTFreeList<T>::Free (copy 22, sizeof(T) 0x14)
// @func 0x0079EA18 ApeTFreeList<T>::Free (copy 23, sizeof(T) 0x9C)
// @func 0x0079EA60 ApeTFreeList<T>::Free (copy 24, sizeof(T) 0x84)
// @func 0x0079EAA8 ApeTFreeList<T>::Free (copy 25, sizeof(T) 0x730)
// @func 0x0079EAF0 ApeTFreeList<T>::Free (copy 26, sizeof(T) 0x74)
// @func 0x0079EB38 ApeTFreeList<T>::Free (copy 27, sizeof(T) 0x70)
// @func 0x0079EB80 ApeTFreeList<T>::Free (copy 28, sizeof(T) 0x140)
// @func 0x0079EBC8 ApeTFreeList<T>::Free (copy 29, sizeof(T) 0x90)
// @func 0x0079EC10 ApeTFreeList<T>::Free (copy 30, sizeof(T) 0xF0)
// @func 0x0079EC58 ApeTFreeList<T>::Free (copy 31, sizeof(T) 0x8C)
// @func 0x0079ECA0 ApeTFreeList<T>::Free (copy 32, sizeof(T) 0xC)
// @func 0x007A4AB0 ApeTFreeList<T>::Free (copy 33, sizeof(T) 0x128)
// @func 0x007A4AF0 ApeTFreeList<T>::Free (copy 34, sizeof(T) 0x7C)
// @func 0x007A4B30 ApeTFreeList<T>::Free (copy 35, sizeof(T) 0x14)
// @func 0x007A950C ApeTFreeList<T>::Free (copy 36, sizeof(T) 0x70)
// @func 0x007A9738 ApeTFreeList<T>::Free (copy 37, sizeof(T) 0x8)
// @func 0x007A9780 ApeTFreeList<T>::Free (copy 38, sizeof(T) 0xB0)
// @func 0x007A97C8 ApeTFreeList<T>::Free (copy 39, sizeof(T) 0xB0)
// @func 0x007A9810 ApeTFreeList<T>::Free (copy 40, sizeof(T) 0x80)
// @func 0x007A9858 ApeTFreeList<T>::Free (copy 41, sizeof(T) 0x70)
// @func 0x007A98A0 ApeTFreeList<T>::Free (copy 42, sizeof(T) 0x160)
// @func 0x007A98E8 ApeTFreeList<T>::Free (copy 43, sizeof(T) 0x40)
template <class T>
void ApeTFreeList<T>::Free(void* element)
{
    ApeFreeList::Free(element);
}

// @func 0x00736BA4 ApeTFreeList<T>::Init (copy 1, sizeof(T) 0x18)
// @func 0x00736CA0 ApeTFreeList<T>::Init (copy 2, sizeof(T) 0x98)
// @func 0x00736D9C ApeTFreeList<T>::Init (copy 3, sizeof(T) 0x14)
// @func 0x00736EC8 ApeTFreeList<T>::Init (copy 4, sizeof(T) 0x14)
// @func 0x00736A74 ApeTFreeList<T>::Init (copy 5, sizeof(T) 0x120)
// @func 0x007370EC ApeTFreeList<T>::Init (copy 6, sizeof(T) 0x160)
// @func 0x00736AA8 ApeTFreeList<T>::Init (copy 7, sizeof(T) 0x80)
// @func 0x0073BA00 ApeTFreeList<T>::Init (copy 8, sizeof(T) 0x14)
// @func 0x0073BA34 ApeTFreeList<T>::Init (copy 9, sizeof(T) 0x14)
// @func 0x0073B904 ApeTFreeList<T>::Init (copy 10, sizeof(T) 0x90)
// @func 0x0073BCF0 ApeTFreeList<T>::Init (copy 11, sizeof(T) 0x160)
// @func 0x00746CB4 ApeTFreeList<T>::Init (copy 12, sizeof(T) 0x120)
// @func 0x00746F18 ApeTFreeList<T>::Init (copy 13, sizeof(T) 0x190)
// @func 0x0074908C ApeTFreeList<T>::Init (copy 14, sizeof(T) 0xC0)
// @func 0x00770694 ApeTFreeList<T>::Init (copy 15, sizeof(T) 0x14)
// @func 0x0077D7F8 ApeTFreeList<T>::Init (copy 16, sizeof(T) 0x14)
// @func 0x00793D18 ApeTFreeList<T>::Init (copy 17, sizeof(T) 0x14)
// @func 0x00793D90 ApeTFreeList<T>::Init (copy 18, sizeof(T) 0x14)
// @func 0x00793D5C ApeTFreeList<T>::Init (copy 19, sizeof(T) 0x14)
// @func 0x00793CE4 ApeTFreeList<T>::Init (copy 20, sizeof(T) 0x14)
// @func 0x00799FA0 ApeTFreeList<T>::Init (copy 21, sizeof(T) 0x14)
// @func 0x00799FD4 ApeTFreeList<T>::Init (copy 22, sizeof(T) 0x14)
// @func 0x007A1634 ApeTFreeList<T>::Init (copy 23, sizeof(T) 0x9C)
// @func 0x007A104C ApeTFreeList<T>::Init (copy 24, sizeof(T) 0x84)
// @func 0x007A1018 ApeTFreeList<T>::Init (copy 25, sizeof(T) 0x730)
// @func 0x007A1A08 ApeTFreeList<T>::Init (copy 26, sizeof(T) 0x74)
// @func 0x007A0E58 ApeTFreeList<T>::Init (copy 27, sizeof(T) 0x70)
// @func 0x007A0E24 ApeTFreeList<T>::Init (copy 28, sizeof(T) 0x140)
// @func 0x007A2160 ApeTFreeList<T>::Init (copy 29, sizeof(T) 0x90)
// @func 0x007A0FE4 ApeTFreeList<T>::Init (copy 30, sizeof(T) 0xF0)
// @func 0x007A0870 ApeTFreeList<T>::Init (copy 31, sizeof(T) 0x8C)
// @func 0x007A0DF0 ApeTFreeList<T>::Init (copy 32, sizeof(T) 0xC)
// @func 0x007A4D1C ApeTFreeList<T>::Init (copy 33, sizeof(T) 0x128)
// @func 0x007A4C38 ApeTFreeList<T>::Init (copy 34, sizeof(T) 0x7C)
// @func 0x007A4CEC ApeTFreeList<T>::Init (copy 35, sizeof(T) 0x14)
// @func 0x007A9B74 ApeTFreeList<T>::Init (copy 36, sizeof(T) 0x70)
// @func 0x007A9A70 ApeTFreeList<T>::Init (copy 37, sizeof(T) 0x8)
// @func 0x007A9AA4 ApeTFreeList<T>::Init (copy 38, sizeof(T) 0xB0)
// @func 0x007A9AD8 ApeTFreeList<T>::Init (copy 39, sizeof(T) 0xB0)
// @func 0x007A9B0C ApeTFreeList<T>::Init (copy 40, sizeof(T) 0x80)
// @func 0x007A9BA8 ApeTFreeList<T>::Init (copy 41, sizeof(T) 0x70)
// @func 0x007A9B40 ApeTFreeList<T>::Init (copy 42, sizeof(T) 0x160)
// @func 0x007A9930 ApeTFreeList<T>::Init (copy 43, sizeof(T) 0x40)
template <class T>
void ApeTFreeList<T>::Init(uint32_t count, ApeAllocator* allocator)
{
    ApeFreeList::Init(sizeof(T), count, allocator, false);
}

} // namespace ape
