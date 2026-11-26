// openTIT - Calls from the HUD module into systems that are not ported yet
// (TODO stubs in HudHooks.cpp).
#pragma once

#include <cstdint>

#include "sly/game/AudioSystem.h"

namespace sly {
namespace hud {

// 0x0046C7B0 on the game world: HUD elements may be placed this frame.
// NOTE: meaning inferred from the only caller (HudObject::Update).
bool HudPlacementEnabled();
class HudObject;
}
}
namespace ape::world { class ApeObj; }
namespace sly {
namespace hud {
// 0x0046B5F8 on the game world: queues the element's callback on world
// callback list `list` (8: 0x00F37878, 11: 0x00F37880).
void GameWorldAddHudCallback(HudObject* element, int list);
// A player character exists (game world +0x1078) / its HUD colour
// (0x004883C0 with its definition's slot 4 value).
bool PlayerHudColorAvailable();
void PlayerHudColor(uint8_t out[4]);
// Byte +0xA15 of the object at job manager +0x50: a timed text element
// still has lines to show. NOTE: owner (a minigame goal?) unknown.
void SetMinigameLinePending(bool pending);
// Sound `sound` of the HUD bank (0x00446984 with the bank 0x010B809C).
audio::SoundName* FindHudSound(uint32_t sound);
// Starts a HUD sound (0x0044C474 without position).
void PlayHudSound(audio::SoundName* sound);
// The active player's confirm button (pad bit 0x80) went down this frame
// (application player block 0x010B1A98 + player * 0x2EC0, world byte +0x1071).
bool ConfirmButtonPressed();
// 0x0048970C on the game world: continues after a prompt.
void GameWorldContinue();
// Own copy of an element's template object (ApeObj virtual slot 14 with 5,
// then 0x00590FEC registering dynamic lights).
ape::world::ApeObj* HudCloneObject(ape::world::ApeObj* object);
// 0x004A5C20: render LOD of an object and its hierarchy.
void HudSetObjectLod(ape::world::ApeObj* object, uint32_t lod);
// 0x0046B7A4 on the game world: takes the element's phase-8 callback
// (0x00F37878) off the world's callback list.
void GameWorldRemoveHudCallback(HudObject* element);

} // namespace hud
} // namespace sly

namespace ape { class ApeRefCounted; }
namespace sly {
namespace hud {
// Script reference of a world object for an anim script object variable.
// NOTE: on the console world objects and script objects share one counted
// base; the port keeps two hierarchies, so the bridge is a stub.
ape::ApeRefCounted* ObjectScriptRef(ape::world::ApeObj* object);
// The game camera's scene object (game world +0x1281E0, camera +0xA0) as a
// script reference; null without a camera.
ape::ApeRefCounted* GameCameraScriptRef();
// Game world halfword +0x128214: frame time is frozen.
bool HudFrameTimeFrozen();
// Game world virtual slot 8 with 1 reports state 12. NOTE: taken for "a
// level is loading"; meter B requests wait until it ends.
bool GameWorldInLoadingState();
} // namespace hud
} // namespace sly
