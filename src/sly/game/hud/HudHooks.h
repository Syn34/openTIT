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
// Coins of a profile (profile record +0x70, read by 0x00465D5C).
int ProfileCoinCount(uint8_t profile);
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

namespace sly {
namespace hud {
// Wheel element hooks (type bit 0x400).
// 0x0049E628: upgrade `item` of character/kind `kind` is bought and enabled
// (game world flags +0xFBC bits 0 and 1 of the upgrade table entry); true
// while no job runs.
bool UpgradeUnlocked(uint32_t kind, uint32_t item);
// The upgrade a character currently uses (game world +0x178 Murray, +0x17C
// Bentley, +0x180 for kind 0xDF0611F9); 0 for other kinds.
uint32_t EquippedUpgrade(uint32_t kind);
// The running job does not tie the player to its costume: no level object
// (game world +0x128164) or its flag +0x15C4 bit 27 is set.
bool CostumeChoiceFree();
// Word +0xB4 bit 15 of the first local player's block (0x010B1A94): input
// held by a modal HUD element. NOTE: meaning inferred from the wheel, which
// sets it while shown and restores the previous value when it hides.
bool AppInputHeld();
void SetAppInputHeld(bool held);
} // namespace hud
} // namespace sly

namespace sly {
namespace hud {
// Upgrade table entry `index` (game world flag byte +0xFBC + index) is
// marked new (bit 2) / the mark is cleared.
bool UpgradeIsNew(uint32_t index);
void UpgradeClearNew(uint32_t index);
} // namespace hud
} // namespace sly
