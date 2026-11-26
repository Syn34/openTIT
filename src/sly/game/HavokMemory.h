// openTIT - The game's memory router for the physics middleware
// ("gmHkMemory", vtable 0x00B1F5D0).
//
// The physics library asks it for raw blocks. During a level the blocks come
// from a dedicated 22 MiB region heap (reset at every level start); outside
// a level, or once the region is full, they come from the C heap. The
// router keeps running totals for the level shut-down report.
#pragma once

#include <cstddef>
#include <cstdint>

namespace sly {

class HavokMemory {
public:
    static constexpr size_t kRegionSize = 0x1600000;

    virtual ~HavokMemory() = default;
    // Slot 2 (0x00355308): a block of `size` bytes (null on failure, with a
    // report of the totals).
    virtual void* BlockAlloc(int32_t size);
    // Slot 3 (0x003554D4).
    virtual void BlockFree(void* block, int32_t size);

    // 0x00355080: the router handed to the physics library.
    static HavokMemory& Instance();

    // 0x00354F60 / 0x003550BC: creates / frees the region heap.
    static void Init();
    static void Shutdown();
    // 0x00355174: the region heap starts empty and takes the level's blocks.
    static void LevelStart();
    // 0x00355204: stops using the region and prints the level's totals.
    static void LevelShutdown();
};

} // namespace sly
