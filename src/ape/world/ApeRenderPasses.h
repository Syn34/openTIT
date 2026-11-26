// openTIT - Instance render passes and the camera instance list of the world
// module.
//
// Geometry instances with a reflection (or texture warp) tag get their own
// render pass (0x005A9DB0): an off-screen render target, three textures of
// decreasing size and the material that shows the result. The passes live
// in a table (0x01194ED8, 200-byte records, count 0x01194FA0); the views the
// render thread produced are kept per frame (0x01194FB0, two banks of
// 0x70-byte records, counts 0x01195090, current bank 0x01195098).
#pragma once

#include <cstdint>
#include <vector>

namespace agl { class AglMaterial; class AglResource; class AglTexture; }

namespace ape {
namespace world {

class ApeCameraInstance;

struct ApeInstanceRenderPass {
    static constexpr int kMaxTextures = 3;

    uint32_t field_00[3] = {};                  // +0x00
    agl::AglMaterial* material = nullptr;       // +0x0C shows the pass result (texture slot 1)
    uint32_t field_10[2] = {};                  // +0x10
    agl::AglResource* target = nullptr;         // +0x18 the pass render target
    agl::AglTexture* textures[kMaxTextures] = {}; // +0x1C
    uint32_t textureCount = 0;                  // +0x28
};

// A view the render thread produced for a pass (0x70 bytes; only the first
// two words are read here).
struct ApeRenderPassView {
    uint32_t passKey = 0;                       // +0x00 0: the main pass
    void* object = nullptr;                     // +0x04
};

struct ApeRenderPassTable {
    static constexpr uint32_t kMaxPasses = 1;

    ApeInstanceRenderPass passes[kMaxPasses];   // 0x01194ED8
    uint32_t passCount = 0;                     // 0x01194FA0
    ApeRenderPassView views[2];                 // 0x01194FB0 one per frame bank
    uint32_t viewCounts[2] = {};                // 0x01195090
    uint32_t bank = 0;                          // 0x01195098 bank being written
};

ApeRenderPassTable& GetRenderPassTable();

// Releases every pass (targets, textures, the material's pass texture) and
// forgets the views (level unload, 0x005AC64C).
void ClearRenderPasses();
// Calls fn(view.object, arg) for each view of the last finished frame made
// for pass `passKey` (0x005AC470).
void ForEachPassView(uint32_t passKey, void (*fn)(void* object, bool arg), bool arg);

// Camera instances of the loaded level (0x01193088, count 0x0119308C).
std::vector<ApeCameraInstance*>& CameraInstanceList();
// Level unload (0x005A5864).
void ClearCameraInstanceList();

} // namespace world
} // namespace ape
