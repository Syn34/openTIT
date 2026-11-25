// openTIT - HUD module hooks (stubs).
#include "sly/game/hud/HudHooks.h"

#include "ape/world/ApeObj.h"

namespace sly {
namespace hud {

// TODO(0x0046C7B0 game world query)
bool HudPlacementEnabled() { return true; }
// TODO(0x0035B470 anchor stack refresh)
void RefreshAnchorStacks() {}
// TODO(0x0046B7A4 game world callback list)
void GameWorldRemoveHudCallback(HudObject*) {}
// TODO(0x0046B5F8 game world callback list)
void GameWorldAddHudCallback(HudObject*, int) {}
// TODO(game world player block +0x1078 / 0x004883C0)
bool PlayerHudColorAvailable() { return false; }
void PlayerHudColor(uint8_t out[4])
{
    out[0] = out[1] = out[2] = out[3] = 0xFF;
}
// TODO(ApeObj slot 14 clone / 0x00590FEC): world objects cannot be cloned
// yet; the element shares the template.
ape::world::ApeObj* HudCloneObject(ape::world::ApeObj* object)
{
    object->AddRef();
    return object;
}
// TODO(0x004A5C20 render LOD)
void HudSetObjectLod(ape::world::ApeObj*, uint32_t) {}

} // namespace hud
} // namespace sly
