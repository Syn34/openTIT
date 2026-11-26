// openTIT - The game's memory router for the physics middleware.
#include "sly/game/HavokMemory.h"

#include "ape/memory/ApeRegionHeap.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace sly {

namespace {

// Every block starts with this header (16 bytes, so the payload stays
// aligned). NOTE: the console reads the size back from the allocator's own
// chunk header and tells the two heaps apart by address; the PC region heap
// does not own a real address range, so the header records both.
struct alignas(16) BlockHeader {
    uint32_t charged;      // bytes counted against the totals
    bool fromRegion;
};

struct HavokMemoryState {
    plat::Mutex mutex;                    // 0x0106BFC8
    ape::ApeRegionHeap* region = nullptr; // 0x0106BFE0
    bool regionActive = false;            // 0x0106BFE4
    size_t totalUsed = 0;                 // 0x0106BFE8
    size_t levelUsed = 0;                 // 0x0106BFEC
    size_t totalPeak = 0;                 // 0x0106BFF0
    size_t levelPeak = 0;                 // 0x0106BFF4
};

HavokMemoryState& State()
{
    static HavokMemoryState state;
    return state;
}

size_t RegionMinFree(const ape::ApeRegionHeap& region)
{
    return region.Available() + region.Used() - region.Peak();
}

} // namespace

// @func 0x00355080 HavokMemory::Instance
HavokMemory& HavokMemory::Instance()
{
    static HavokMemory instance;
    return instance;
}

// @func 0x00354F60 HavokMemory::Init
void HavokMemory::Init()
{
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    // NOTE: the console allocates the 22 MiB region here; the PC region heap
    // only keeps the budget and takes its blocks from the host heap.
    state.region = new ape::ApeRegionHeap();
    state.region->Init(nullptr, kRegionSize, false);
    state.regionActive = false;
    state.totalUsed = 0;
    state.levelUsed = 0;
    state.totalPeak = 0;
    state.levelPeak = 0;
}

// @func 0x003550BC HavokMemory::Shutdown
void HavokMemory::Shutdown()
{
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    if (state.region) {
        state.region->Destroy();
        delete state.region;
        state.region = nullptr;
    }
}

// @func 0x00355174 HavokMemory::LevelStart
void HavokMemory::LevelStart()
{
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    if (state.region)
        state.region->Init(nullptr, kRegionSize, true);
    state.regionActive = true;
    state.levelUsed = 0;
    state.levelPeak = 0;
}

// @func 0x00355204 HavokMemory::LevelShutdown
void HavokMemory::LevelShutdown()
{
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    if (!state.region)
        return;
    state.regionActive = false;
    std::printf("gmHkMemory: LevelShutdown: Total:   InUse: %9dk\n", int(state.totalUsed >> 10));
    std::printf("gmHkMemory: LevelShutdown: Total: PeakUse: %9dk\n", int(state.totalPeak >> 10));
    std::printf("gmHkMemory: LevelShutdown: Heap:  MinFree: %9dk\n", int(RegionMinFree(*state.region) >> 10));
    std::printf("gmHkMemory: LevelShutdown: Level: PeakUse: %9dk\n", int(state.levelPeak >> 10));
    std::printf("\n");
}

// @func 0x00355308 HavokMemory::BlockAlloc
void* HavokMemory::BlockAlloc(int32_t size)
{
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    const size_t total = sizeof(BlockHeader) + (size > 0 ? size_t(size) : 0);
    void* raw = nullptr;
    bool fromRegion = false;
    if (size >= 0 && state.regionActive && state.region) {
        raw = state.region->Alloc(16, total);
        fromRegion = raw != nullptr;
    }
    if (!raw)
        raw = std::malloc(total);
    if (!raw) {
        std::printf("gmHkMemory: blockAlloc: failed to alloc:%d\n", size);
        std::printf("gmHkMemory: blockAlloc: Level:    Used:%9dk PeakUse: %dk\n", int(state.levelUsed >> 10),
                    int(state.levelPeak >> 10));
        std::printf("gmHkMemory: blockAlloc: Total:    Used:%9dk PeakUse: %dk\n", int(state.totalPeak >> 10),
                    int(state.totalPeak >> 10));
        if (state.region)
            std::printf("gmHkMemory: blockAlloc: Heap:  MinFree:%9dk\n", int(RegionMinFree(*state.region) >> 10));
        std::printf("\n");
        return nullptr;
    }
    auto* header = static_cast<BlockHeader*>(raw);
    // Charged like a heap chunk: the payload rounded to 16 plus a header.
    header->charged = uint32_t(((total - sizeof(BlockHeader) + 15) & ~size_t(15)) + 16);
    header->fromRegion = fromRegion;
    state.totalUsed += header->charged;
    state.totalPeak = std::max(state.totalPeak, state.totalUsed);
    state.levelUsed += header->charged;
    state.levelPeak = std::max(state.levelPeak, state.levelUsed);
    return header + 1;
}

// @func 0x003554D4 HavokMemory::BlockFree
void HavokMemory::BlockFree(void* block, int32_t /*size*/)
{
    if (!block)
        return;
    HavokMemoryState& state = State();
    std::lock_guard<plat::Mutex> lock(state.mutex);
    BlockHeader* header = static_cast<BlockHeader*>(block) - 1;
    state.totalUsed -= header->charged;
    state.levelUsed -= header->charged;
    // NOTE: the console frees region blocks to the region only while it is
    // active (outside a level they leak to the C heap's free); the header
    // makes the right owner known either way.
    if (header->fromRegion && state.region)
        state.region->Free(header);
    else
        std::free(header);
}

} // namespace sly
