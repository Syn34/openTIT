// openTIT - Draw buckets: the render items of one kind (opaque, translucent,
// decals ...) collected for a render target, sorted by a 32-bit key and
// drawn through their materials' techniques.
//
// A bucket holds (key, item) pairs, a list of callbacks run before its items
// and one run after them, and the stencil write mask its draws use. The
// console keeps several bucket classes (vtables 0x00B233E4..0x00B23CA4)
// that differ in how they compute keys and draw; the shared parts are here.
#pragma once

#include "agl/AglRenderTarget.h"

#include <cstdint>
#include <vector>

namespace agl {

class AglMaterial;
class AglTechnique;
struct AglRenderItem;

struct AglBucketEntry {
    uint32_t key;
    AglRenderItem* item;
};

class AglDrawBucket {
public:
    explicit AglDrawBucket(uint32_t capacity = 0x1000);
    virtual ~AglDrawBucket() = default;

    // Slot 1: adds an item with its sort key.
    virtual void Add(AglRenderItem* item, uint32_t key);
    // Slot 2 of the plain bucket: adds an item that sorts first.
    virtual void AddUnsorted(AglRenderItem* item);
    // Slot 3: sorts the items by key. A bucket that overflowed its capacity
    // drops all of its items for this frame.
    virtual void Sort();
    // Slot 4: runs the "before" callbacks, draws the items and runs the
    // "after" callbacks.
    virtual void Draw();

    // Ends the item list with a terminator that sorts last.
    void Terminate();
    // Stable byte-wise radix sort used for large buckets.
    void RadixSort();

    uint32_t Count() const { return static_cast<uint32_t>(m_items.size()); }

    std::vector<AglBucketEntry> m_items;    // 0x04 begin / 0x08 cursor
    uint8_t m_flags = 0;                    // 0x0C bit 7: bucket disabled
    uint8_t m_stencilWriteMask = 0;         // 0x0D
    AglRenderCallbackList m_before;         // 0x14
    AglRenderCallbackList m_after;          // 0x58
    uint32_t m_capacity = 0;                // 0x9C
    uint32_t m_peakCount = 0;               // 0xA0
};

// Draw state shared by the item loop: the material and technique last
// applied (0x010DC840 / 0x010DC844).
struct AglItemDrawState {
    AglMaterial* material = nullptr;
    AglTechnique* technique = nullptr;
    uint32_t field_8 = 0;                   // 0x010DC848 NOTE: cleared with the others, use unknown
};
AglItemDrawState& GetItemDrawState();

// Last blend and depth words applied from a material (0x010DD598 /
// 0x010DD5A0); anything that changes that state behind the material code
// resets them.
void InvalidateMaterialStateCache();

// Draws one render item with the technique its material selects (or the
// render target's override technique).
void DrawRenderItem(AglRenderItem* item);
// Item stencil set-up of a bucket: draws write their reference into the
// bits of 'writeMask' (stencil test always passes).
void BeginBucketDraw(uint8_t writeMask);
// Ends the current technique and restores the default stencil state.
void EndBucketDraw();
// Number of buckets drawn so far (0x010C5A6C); per-bucket caches compare
// against it.
uint32_t BucketDrawCounter();

} // namespace agl
