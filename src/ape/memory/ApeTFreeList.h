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
// T's constructor/destructor.
template <class T>
class ApeTFreeList : public ApeFreeList {
public:
    ApeTFreeList() = default;
    ApeTFreeList(uint32_t count, ApeAllocator* allocator = nullptr, bool permanentFirstBlock = false)
    {
        ApeFreeList::Init(sizeof(T), count, allocator, permanentFirstBlock);
    }

    using ApeFreeList::Init;
    void Init(uint32_t count, ApeAllocator* allocator = nullptr, bool permanentFirstBlock = false)
    {
        ApeFreeList::Init(sizeof(T), count, allocator, permanentFirstBlock);
    }

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
            state.list->Init(state.initialCount, nullptr, false);
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

} // namespace ape
