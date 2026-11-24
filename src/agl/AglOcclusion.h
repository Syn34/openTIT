// openTIT - Agl occlusion queries: how much of a sphere is visible, measured by the GPU.
//
// The game asks "how visible is this point" (lens flares, light glows) by
// drawing a small sphere twice with pixel counting on: once with the
// current depth test (visible pixels) and once without (total pixels). The
// GPU writes both counts to report slots a frame or more later, so every
// query keeps a ring of four frames of results and the game reads the one
// from a few frames back. There are at most 16 queries (0x010DFAD0).
#pragma once

#include "ape/math/ApeMatrixOps.h"

#include <cstdint>

namespace agl {

class AglSubMesh;

class AglOcclusionQueryBase {
public:
    AglOcclusionQueryBase();
    virtual ~AglOcclusionQueryBase();

    AglOcclusionQueryBase(const AglOcclusionQueryBase&) = delete;
    AglOcclusionQueryBase& operator=(const AglOcclusionQueryBase&) = delete;

    bool active = false;                // 0x04: slot in use
};

class AglOcclusionQuery : public AglOcclusionQueryBase {
public:
    static constexpr int kFrames = 4;

    enum SlotFlags : uint8_t {
        kSlot_Issued       = 1,
        kSlot_TotalReady   = 2,
        kSlot_VisibleReady = 4,
        kSlot_Complete     = kSlot_Issued | kSlot_TotalReady | kSlot_VisibleReady,
    };

    struct Slot {
        uint32_t totalPixels = 0;       // pixels drawn without depth test
        uint32_t visiblePixels = 0;     // pixels passing the depth test
        uint32_t totalReport = 0;       // GPU report indices
        uint32_t visibleReport = 0;
        uint8_t flags = 0;              // SlotFlags
    };

    AglOcclusionQuery();

    // Sets the sphere and its transform and clears the results.
    void Init(AglSubMesh* mesh, const ape::Matrix34& transform);
    // Draws the sphere twice with pixel counting (render thread).
    virtual void Issue();
    // Results of the frame 'framesBack' frames ago; true when both counts
    // have arrived.
    virtual bool GetResult(uint32_t out[2], int framesBack) const;
    // Collects arrived GPU reports and moves to the next frame slot.
    void Update();
    // Drops the sphere mesh.
    void ReleaseMesh();

    AglSubMesh* mesh = nullptr;         // 0x08
    ape::Matrix34 transform;            // 0x10
    Slot slots[kFrames];                // 0x50
    uint16_t current = 0;               // 0xA0
};

// System start-up and shutdown (the shared query material and report memory).
void InitOcclusionQueries();
void ShutdownOcclusionQueries();
// A query drawing a rings x segments sphere with 'transform' (null when all
// 16 are in use).
AglOcclusionQuery* CreateOcclusionQuery(const ape::Matrix34& transform, int rings, int segments);
// Frees a query slot immediately (see QueueEndOfFrameRelease for the
// deferred version).
bool FreeOcclusionQuery(AglOcclusionQuery* query);
// Releases a query queued for the end of the frame.
void ReleaseOcclusionQuery(AglOcclusionQuery* query);
// Draws every active query (render thread; skipped for the right eye).
void DrawOcclusionQueries();
void DrawOcclusionQueriesForEye();
// End-of-frame collection of the GPU results.
void UpdateOcclusionQueries();
void SetOcclusionQueriesEnabled(bool enabled);

} // namespace agl
