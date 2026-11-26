// openTIT - Instance render passes and the camera instance list.
#include "ape/world/ApeRenderPasses.h"

#include "agl/AglResources.h"
#include "agl/AglTexture.h"

namespace ape {
namespace world {

ApeRenderPassTable& GetRenderPassTable()
{
    static ApeRenderPassTable table;
    return table;
}

std::vector<ApeCameraInstance*>& CameraInstanceList()
{
    static std::vector<ApeCameraInstance*> list;
    return list;
}

// @func 0x005AC64C ClearRenderPasses
void ClearRenderPasses()
{
    ApeRenderPassTable& table = GetRenderPassTable();
    for (uint32_t i = 0; i < table.passCount; ++i) {
        ApeInstanceRenderPass& pass = table.passes[i];
        pass.material->SetTexture(nullptr, 1);
        for (uint32_t t = 0; t < pass.textureCount; ++t) {
            if (pass.textures[t]) {
                pass.textures[t]->Release();
                pass.textures[t] = nullptr;
            }
        }
        if (pass.target) {
            pass.target->Release();
            pass.target = nullptr;
        }
    }
    table.passCount = 0;
    table.viewCounts[0] = 0;
    table.viewCounts[1] = 0;
}

// @func 0x005AC470 ForEachPassView
void ForEachPassView(uint32_t passKey, void (*fn)(void* object, bool arg), bool arg)
{
    ApeRenderPassTable& table = GetRenderPassTable();
    const uint32_t finished = table.bank ^ 1u;
    // NOTE: the console steps through the records at 0x70-byte strides from
    // the bank's first record; with the single record per bank the game
    // uses, that is the bank's own record.
    for (uint32_t i = 0; i < table.viewCounts[finished]; ++i) {
        const ApeRenderPassView& view = table.views[finished];
        if (view.passKey == passKey)
            fn(view.object, arg);
    }
}

// @func 0x005A5864 ClearCameraInstanceList
void ClearCameraInstanceList()
{
    std::vector<ApeCameraInstance*>().swap(CameraInstanceList());
}

} // namespace world
} // namespace ape
