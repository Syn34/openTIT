// openTIT - HUD module hooks (stubs).
#include "sly/game/hud/HudHooks.h"

#include "sly/game/Upgrades.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"

#include "ape/world/ApeObj.h"

namespace sly {
namespace hud {

// TODO(0x0046C7B0 game world query)
bool HudPlacementEnabled() { return true; }
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
// TODO(job manager +0x50 object byte +0xA15)
void SetMinigameLinePending(bool) {}
// TODO(0x00446984 sound lookup in the HUD bank)
audio::SoundName* FindHudSound(uint32_t) { return nullptr; }
// TODO(0x0044C474)
void PlayHudSound(audio::SoundName*) {}
// TODO(application pad state)
bool ConfirmButtonPressed() { return false; }
void GameWorldContinue()
{
    if (g_game)
        g_game->ContinueAfterPrompt();
}
// TODO(ApeObj slot 14 clone / 0x00590FEC): world objects cannot be cloned
// yet; the element shares the template.
ape::world::ApeObj* HudCloneObject(ape::world::ApeObj* object)
{
    object->AddRef();
    return object;
}
void HudSetObjectLod(ape::world::ApeObj* object, uint32_t lod) { SetHierarchyLod(object, lod); }
// TODO(world object / script object bridge)
ape::ApeRefCounted* ObjectScriptRef(ape::world::ApeObj*) { return nullptr; }
// TODO(game world +0x1281E0 camera object)
ape::ApeRefCounted* GameCameraScriptRef() { return nullptr; }
// TODO(game world +0x128214)
bool HudFrameTimeFrozen() { return false; }
bool GameWorldInLoadingState() { return game::GameWorldState(1) == 12; }

int ProfileCoinCount(uint8_t profile) { (void)profile; return 0; }
// TODO(game world +0x70)
int GameWorldCoins() { return 0; }
// TODO(0x0049E0DC)
bool UpgradeBuy(uint32_t id) { (void)id; return false; }
// TODO(0x0049E268)
bool UpgradeAffordable(uint32_t id) { (void)id; return false; }
// TODO(game world +0x1AC bit 1)
void GameWorldMarkAbilitiesChanged() {}

} // namespace hud
} // namespace sly

namespace sly {
namespace hud {
// TODO(0x0049E628 upgrade table query)
bool UpgradeUnlocked(uint32_t kind, uint32_t item) { (void)kind; (void)item; return true; }
// TODO(game world +0x178/+0x17C/+0x180)
uint32_t EquippedUpgrade(uint32_t kind) { (void)kind; return 0; }
// TODO(game world +0x128164 object, +0x15C4 bit 27)
bool CostumeChoiceFree() { return true; }
// TODO(application player block word +0xB4 bit 15)
static bool s_appInputHeld = false;
bool AppInputHeld() { return s_appInputHeld; }
void SetAppInputHeld(bool held) { s_appInputHeld = held; }
} // namespace hud
} // namespace sly

namespace sly {
namespace hud {
bool UpgradeIsNew(uint32_t index) { return (UpgradeState(index) & kUpgrade_New) != 0; }
void UpgradeClearNew(uint32_t index) { UpgradeState(index) &= uint8_t(~kUpgrade_New); }
} // namespace hud
} // namespace sly

namespace sly {
namespace hud {
// TODO(0x004A6CF8 world draw call buffer)
void HudQueueObjectDraw(ape::world::ApeObj* object, bool recurse) { (void)object; (void)recurse; }
} // namespace hud
} // namespace sly

namespace sly {
namespace hud {
// TODO(0x004A6DF0 world draw call buffer)
void HudQueueObjectDraw2(ape::world::ApeObj* object, bool recurse) { (void)object; (void)recurse; }
} // namespace hud
} // namespace sly
