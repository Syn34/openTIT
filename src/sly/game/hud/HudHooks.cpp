// openTIT - HUD module hooks (stubs).
#include "sly/game/hud/HudHooks.h"

namespace sly {
namespace hud {

// TODO(0x0046C7B0 game world query)
bool HudPlacementEnabled() { return true; }
// TODO(0x0035B470 anchor stack refresh)
void RefreshAnchorStacks() {}
// TODO(0x0046B7A4 game world callback list)
void GameWorldRemoveHudCallback(HudObject*) {}

} // namespace hud
} // namespace sly
