// openTIT - ApeTFreeList: fixed-size element pool with intrusive used/free lists.
#include "ape/memory/ApeTFreeList.h"
#include "ape/memory/ApeTArray.h"

#include <algorithm>
#include <cstring>

namespace ape {

namespace {

constexpr size_t kBlockHeaderSize = 32;   // sizeof(Block) rounded to 16

// Registry of every initialised pool (console: an ApeTArray of 64 entries,
// created on first use with the default allocator).
ApeTArray<ApeFreeList*>& Registry()
{
    // Intentionally never destroyed: pools with static storage duration may
    // unregister themselves after the registry would have been torn down.
    static ApeTArray<ApeFreeList*>* s_registry = new ApeTArray<ApeFreeList*>(0x40, GetDefaultAllocator());
    return *s_registry;
}

size_t RoundUp16(size_t value)
{
    return (value + 15) & ~size_t(15);
}

} // namespace

ApeFreeList::~ApeFreeList()
{
    Destroy();
}

size_t ApeFreeList::NodeStride() const
{
    return sizeof(Node) + RoundUp16(m_elemSize);
}

void ApeFreeList::Register(ApeFreeList* list)
{
    Registry().PushBack(list);
}

void ApeFreeList::Unregister(ApeFreeList* list)
{
    ApeTArray<ApeFreeList*>& registry = Registry();
    int index = registry.FindLast(list);
    if (index >= 0)
        registry.RemoveAtSwap(size_t(index));
}

// @func 0x005518FC ApeFreeList::AddBlock
void ApeFreeList::AddBlock(bool permanent)
{
    static_assert(sizeof(Block) <= kBlockHeaderSize, "block header does not fit");

    const size_t stride = NodeStride();
    const size_t nodeBytes = stride * m_growCount;
    void* memory = m_allocator->Alloc(kBlockHeaderSize + nodeBytes, false, 16);
    if (!memory)
        return;

    Block* block = static_cast<Block*>(memory);
    block->size = kBlockHeaderSize + nodeBytes;
    block->flags = permanent ? kBlockPermanent : 0;
    m_blocks.PushBack(block);

    uint8_t* nodes = static_cast<uint8_t*>(memory) + kBlockHeaderSize;
    for (uint32_t i = 0; i < m_growCount; ++i)
        m_free.PushFront(reinterpret_cast<Node*>(nodes + i * stride));
}

// @func 0x00551A58 ApeFreeList::Init
void ApeFreeList::Init(size_t elemSize, uint32_t count, ApeAllocator* allocator, bool permanentFirstBlock)
{
    m_elemSize = elemSize;
    m_allocator = allocator ? allocator : GetDefaultAllocator();
    m_peakUsed = 0;
    m_growCount = uint16_t(std::max<uint32_t>(count, 1));

    if (count != 0) {
        AddBlock(permanentFirstBlock);
        // Big pools grow in quarter-size steps (never less than 16 elements).
        if (count > 16)
            m_growCount = uint16_t(std::max<uint32_t>(count / 4, 16));
    }

    Register(this);
}

// @func 0x00556BF0 ApeFreeList::First
ApeFreeList::Node* ApeFreeList::First() const
{
    return m_used.head;
}

// @func 0x00556C04 ApeFreeList::Alloc
void* ApeFreeList::Alloc()
{
    if (!m_free.head)
        AddBlock(false);
    Node* node = m_free.head;
    if (!node)
        return nullptr;

    m_free.Remove(node);
    std::memset(node, 0, NodeStride());
    m_used.PushFront(node);
    m_peakUsed = std::max(m_peakUsed, m_used.count);
    return NodeData(node);
}

// @func 0x00556D18 ApeFreeList::Free
void ApeFreeList::Free(void* element)
{
    Node* node = NodeOf(element);
    m_used.Remove(node);
    m_free.PushFront(node);
}

// @func 0x00556774 ApeFreeList::ReleaseUnusedBlocks
void ApeFreeList::ReleaseUnusedBlocks()
{
    Block* block = m_blocks.head;
    while (block) {
        Block* nextBlock = block->next;
        if (!(block->flags & kBlockPermanent)) {
            const uint8_t* begin = reinterpret_cast<const uint8_t*>(block);
            const uint8_t* end = begin + block->size;
            auto inBlock = [&](const Node* node) {
                const uint8_t* p = reinterpret_cast<const uint8_t*>(node);
                return p >= begin && p < end;
            };

            bool inUse = false;
            for (Node* node = m_used.head; node; node = node->next) {
                if (inBlock(node)) {
                    inUse = true;
                    break;
                }
            }

            if (!inUse) {
                for (Node* node = m_free.head; node;) {
                    Node* nextNode = node->next;
                    if (inBlock(node))
                        m_free.Remove(node);
                    node = nextNode;
                }
                m_blocks.Remove(block);
                // NOTE: the console passes 0 as the size here; the real size
                // keeps size-tracking allocators (buckets) correct.
                m_allocator->Free(block, block->size);
            }
        }
        block = nextBlock;
    }
}

// @func 0x005568FC ApeFreeList::Destroy
void ApeFreeList::Destroy()
{
    if (!m_allocator)
        return;

    // Live and free nodes all live inside the blocks; just forget them.
    m_used.Reset();
    m_free.Reset();
    while (Block* block = m_blocks.head) {
        m_blocks.Remove(block);
        m_allocator->Free(block, block->size);
    }
    m_allocator = nullptr;

    Unregister(this);
}

// @func 0x00556DB4 ApeFreeList::CompactAll
void ApeFreeList::CompactAll()
{
    ApeTArray<ApeFreeList*>& registry = Registry();
    for (int i = int(registry.Count()) - 1; i >= 0; --i)
        registry[size_t(i)]->ReleaseUnusedBlocks();
}

} // namespace ape
