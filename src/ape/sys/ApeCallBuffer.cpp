// openTIT - Deferred call buffer.
#include "ape/sys/ApeCallBuffer.h"

#include <cstring>

namespace ape {

void ApeCallBuffer::Init(void* memory, size_t size)
{
    m_capacity = size;
    m_base = static_cast<uint8_t*>(memory);
    m_top = m_base + size;
    m_write = reinterpret_cast<DeferredCall*>(m_base);
    m_data = m_top;
    m_used = 0;
    m_count = 0;
    m_frame = 0;
    m_flags = 0;
}

// @func 0x00564FF8 ApeCallBuffer::Push
void ApeCallBuffer::Push(DeferredFunc func, void* arg)
{
    if (m_used + sizeof(DeferredCall) > m_capacity)
        return;
    *m_write++ = DeferredCall{ func, arg };
    m_used += sizeof(DeferredCall);
    ++m_count;
}

namespace {
uint8_t* AlignDown16(uint8_t* p)
{
    return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(p) & ~(uintptr_t)15);
}
}

// @func 0x00565044 ApeCallBuffer::PushWithData
void ApeCallBuffer::PushWithData(DeferredFunc func, const void* data, size_t size)
{
    uint8_t* copy = AlignDown16(m_data - size);
    const size_t reserved = size_t(m_data - copy);
    if (m_used + reserved + sizeof(DeferredCall) > m_capacity)
        return;
    *m_write++ = DeferredCall{ func, copy };
    std::memcpy(copy, data, size);
    m_data = copy;
    m_used += reserved + sizeof(DeferredCall);
    ++m_count;
}

// @func 0x0056510C ApeCallBuffer::AllocData
void* ApeCallBuffer::AllocData(size_t size)
{
    uint8_t* block = AlignDown16(m_data - size);
    const size_t used = m_used + size_t(m_data - block);
    if (used > m_capacity)
        return nullptr;
    m_data = block;
    m_used = used;
    return block;
}

// @func 0x00565154 ApeCallBuffer::BeginBlock
void ApeCallBuffer::BeginBlock(DeferredBlock& block)
{
    block.first = m_write;
    block.count = 0;
    block.frame = 0;
    m_flags |= kFlag_InBlock;
}

// @func 0x00565178 ApeCallBuffer::EndBlock
void ApeCallBuffer::EndBlock(DeferredBlock& block)
{
    block.frame = m_frame;
    block.count = (uint32_t)(m_write - block.first);
    m_flags = (uint8_t)((m_flags & ~kFlag_InBlock) | kFlag_BlockDone);
}

// @func 0x005651B0 ApeCallBuffer::ExecuteBlock
void ApeCallBuffer::ExecuteBlock(const DeferredBlock& block) const
{
    const DeferredCall* call = block.first;
    for (uint32_t i = block.count; i != 0; --i, ++call)
        call->func(call->arg);
}

// @func 0x00565228 ApeCallBuffer::Execute
void ApeCallBuffer::Execute(bool reset)
{
    const DeferredCall* call = reinterpret_cast<const DeferredCall*>(m_base);
    for (uint32_t i = m_count; i != 0; --i, ++call)
        call->func(call->arg);

    if (reset) {
        m_write = reinterpret_cast<DeferredCall*>(m_base);
        m_data = m_top;
        m_count = 0;
        m_used = 0;
        ++m_frame;
        m_flags &= (uint8_t)~(kFlag_InBlock | kFlag_BlockDone);
    }
}

} // namespace ape
