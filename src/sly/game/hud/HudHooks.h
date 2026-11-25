// openTIT - Calls from the HUD module into systems that are not ported yet
// (TODO stubs in HudHooks.cpp).
#pragma once

namespace sly {
namespace hud {

// 0x0046C7B0 on the game world: HUD elements may be placed this frame.
// NOTE: meaning inferred from the only caller (HudObject::Update).
bool HudPlacementEnabled();
// 0x0035B470 on the anchor stacks (0x0106C330): re-stacks the visible elements.
void RefreshAnchorStacks();
class HudObject;
// 0x0046B7A4 on the game world: takes the element's phase-8 callback
// (0x00F37878) off the world's callback list.
void GameWorldRemoveHudCallback(HudObject* element);

} // namespace hud
} // namespace sly
