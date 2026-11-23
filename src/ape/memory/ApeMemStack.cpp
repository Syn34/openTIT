// openTIT - ApeMemStack: LIFO scratch allocator over one contiguous buffer.
#include "ape/memory/ApeMemStack.h"
#include "platform/Platform.h"

namespace ape {

namespace {

ApeMemStack* s_globalStack = nullptr;

constexpr size_t kGlobalStackSize = 0x80000;

uint8_t* AlignUp16(uint8_t* p)
{
    return reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(p) + 15) & ~uintptr_t(15));
}

uint8_t* AlignDown16(uint8_t* p)
{
    return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(p) & ~uintptr_t(15));
}

} // namespace

// @func 0x0054ECA0 ApeMemStack::ApeMemStack
ApeMemStack::ApeMemStack(size_t size, int memClass, bool fallbackToHeap)
    : m_size(size)
    , m_memClass(memClass)
    , m_fallbackToHeap(fallbackToHeap)
    , m_minFree(size)
{
    m_base = static_cast<uint8_t*>(MemAlloc(size, memClass, 16));
    if (m_base) {
        m_end = m_base + size;
        m_top = m_base;
    }
}

// @func 0x00553140 ApeMemStack::~ApeMemStack
// @func 0x005531A4 ApeMemStack::~ApeMemStack (deleting)
ApeMemStack::~ApeMemStack()
{
    if (m_base)
        MemFree(m_base, m_memClass);
    m_top = nullptr;
    m_end = nullptr;
    m_base = nullptr;
}

bool ApeMemStack::Owns(const void* ptr) const
{
    const uint8_t* p = static_cast<const uint8_t*>(ptr);
    return p >= m_base && p <= m_end;
}

// @func 0x0055260C ApeMemStack::Alloc
void* ApeMemStack::Alloc(size_t size)
{
    MemSystemInit();

    const size_t rounded = (size + 15) & ~size_t(15);
    uint8_t* block = m_top;
    uint8_t* newTop = block + kHeaderSize + rounded;

    if (!block || newTop > m_end) {
        if (!m_fallbackToHeap)
            return nullptr;
        plat::Log(plat::LogLevel::Warning, "ApeMemStack::Alloc using malloc:%d\n", (int)size);
        return SysAlloc(16, size, kMemClass_Default, 0);
    }

    m_top = newTop;
    const size_t freeBytes = (size_t)(m_end - newTop);
    if (freeBytes < m_minFree)
        m_minFree = freeBytes;

    // The header remembers the block size (debug aid; Free does not need it).
    *reinterpret_cast<size_t*>(block) = rounded;
    return block + kHeaderSize;
}

// @func 0x0055322C ApeMemStack::Free
void ApeMemStack::Free(void* ptr)
{
    uint8_t* p = static_cast<uint8_t*>(ptr);
    if (!Owns(p)) {
        // Overflow block that came from the heap.
        SysFree(ptr, kMemClass_Default, 0);
        return;
    }
    uint8_t* block = p - kHeaderSize;
    if (block < m_top)
        m_top = AlignDown16(p - 1);
}

// @func 0x0055329C ApeMemStack::Reset
void ApeMemStack::Reset()
{
    if (m_base < m_top)
        m_top = AlignUp16(m_base);
    m_upperLimit = AlignDown16(m_end);
}

// @func 0x005525FC GetMemStack
ApeMemStack* GetMemStack()
{
    return s_globalStack;
}

namespace detail {

void CreateGlobalMemStack()
{
    if (!s_globalStack)
        s_globalStack = new ApeMemStack(kGlobalStackSize, kMemClass_Stack, true);
}

} // namespace detail

} // namespace ape
