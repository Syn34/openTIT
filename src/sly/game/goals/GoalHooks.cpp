// openTIT - TODO stubs for the goal module's outside calls (see GoalHooks.h).
#include "sly/gameplay/Lasers.h"
#include "sly/game/goals/GoalHooks.h"

#include "sly/game/hud/HudType8000.h"

#include "sly/game/goals/JobManager.h"
#include "sly/game/SlyGame.h"
#include "sly/gameplay/Minigame.h"

namespace sly {
namespace goals {

// Jobs / job manager
uint32_t JobGoalCount(const Job*) { return 0; }
Goal* JobGoalAt(const Job*, uint32_t) { return nullptr; }
uint32_t JobGoalSaveWord(const Job*, int) { return 0; }
void JobSetGoalSaveWord(Job*, int, uint32_t) {}
int JobTakeGoalSaveSlot(Job*) { return 0; }
uint8_t JobOwnerFlag(const Job*) { return 0; }
bool IsGoalInLiveJob(const Goal* goal)
{
    game::JobManager* manager = game::GetJobManager();
    return manager && manager->IsGoalLive(goal);
}
void JobManagerGoalFailed(Goal* goal, uint8_t ownerFlag)
{
    if (game::JobManager* manager = game::GetJobManager())
        manager->OnGoalFailed(goal, ownerFlag);
}
void JobManagerBeforeFailSequence()
{
    if (game::JobManager* manager = game::GetJobManager())
        manager->FlushPendingRequest();
}
Goal* JobManagerTrackedGoal()
{
    game::JobManager* manager = game::GetJobManager();
    return manager ? manager->trackedGoal : nullptr;
}
Goal* JobManagerGoalAfterTracked()
{
    game::JobManager* manager = game::GetJobManager();
    return manager ? manager->GoalAfterTracked() : nullptr;
}
const GoalScriptBinding* JobManagerJobBindings(size_t* count)
{
    *count = 0;
    return nullptr;
}
void JobManagerSetTimerGoal(Goal*) {}
void JobManagerGroupAdvanced() { game::SetPlayersTargeting(true); }
uint16_t GameHudLockCount(SlyGame*) { return 0; }
bool PlayerPosition(float out[3])
{
    out[0] = out[1] = out[2] = 0.0f;
    return false;
}
void EntityPosition(const void*, float out[3]) { out[0] = out[1] = out[2] = 0.0f; }

// Sequences and scripts
void SequenceStreamRequest(uint32_t) {}
bool SequenceStreamRelease(uint32_t) { return false; }
void SequenceStreamLoadNow(uint32_t) {}
bool GameIsSequenceQueued(SlyGame*, uint32_t) { return false; }
void GameQueueSequence(SlyGame*, uint32_t, uint32_t, const GoalScriptBinding*, size_t, bool) {}
void GameMarkStreamsChanged(SlyGame*) {}
void BindScriptVariables(ape::ApeAnimScriptInstance*, const GoalScriptBinding*, size_t) {}

// Hint targets
void* HintFindEntity(uint32_t) { return nullptr; }
bool HintEntityTargetable(const void*) { return false; }
bool HintEntityNeedsRefresh(const void*) { return false; }
void HintEntityRefresh(void*) {}
bool HintEntityInteractable(const void*) { return false; }
void* HintEntityInteraction(void*) { return nullptr; }
void* HintFindLaserGroup(uint32_t name) { return GetLaserManager() ? GetLaserManager()->FindGroup(name) : nullptr; }
void* HintFindLaser(uint32_t name) { return GetLaserManager() ? GetLaserManager()->FindLaser(name) : nullptr; }
void* HintFindLaserField(uint32_t name) { return GetLaserManager() ? GetLaserManager()->FindField(name) : nullptr; }
void HintEntitySetEnabled(void*, bool) {}
void HintInteractableSetEnabled(void*, bool) {}
int HintInteractionState(const void*) { return 0; }
void HintInteractionSetState(void*, int) {}
void HintLaserGroupEnable(void* group, bool enable) { static_cast<LaserGroup*>(group)->SetEnabled(enable, false, false); }
void HintLaserEnable(void* laser, bool enable) { static_cast<Laser*>(laser)->SetEnabled(enable, false, false); }
void HintLaserFieldEnable(void* field, bool enable) { static_cast<LaserField*>(field)->SetEnabled(enable, false, false); }

// Players / entities
void* PlayerCharacter(int) { return nullptr; }
int ObjectPlayerIndex(const void*) { return 0; }
bool EntityIsCharacter(const void*) { return false; }
uint32_t EntityGroupName(const void*) { return 0; }
uint32_t EntityGroupField_C0(const void*) { return 0; }
bool EntityIsSlyClass(const void*) { return false; }
uint8_t CharacterAbilityBits(const void*, int) { return 0; }

void CharacterSetState(void*, int, int) {}
uint32_t CharacterActionButtons(void*, uint32_t) { return 0; }
const void* PlayerPad(int) { return nullptr; }
uint32_t PadButtonsDown(const void*) { return 0; }
float PadStickAxis(const void*, int) { return 0.0f; }
float BinocucomPowerLevel() { return 0.0f; }
void GoalAddEntityMarker(GoalMarkerList*, void*) {}
void* GoalFindEntity(uint32_t) { return nullptr; }
void* EntityFindDescendant(void*, uint32_t) { return nullptr; }

bool GoalEntityAlive(const void*) { return false; }
float GoalEntityHealth(const void*) { return 0.0f; }
float GoalEntityMaxHealth(const void*) { return 0.0f; }
bool GoalEntityIsActor(const void*) { return false; }
void JobOwnerUntarget(const Job*, uint32_t) {}
void JobOwnerAddTarget(const Job*, void*) {}
void JobOwnerRemoveTarget(const Job*, void*) {}
void GoalReportStat(void*, uint32_t, uint32_t, int) {}
void GoalListenTo(Goal*, uint32_t) {}
void GoalStopListening(Goal*) {}
void* SpawnMousetrapHelper() { return nullptr; }
void* GoalEntityParent(const void*) { return nullptr; }
bool GoalEntityMarkerSuppressed(const void*) { return false; }
bool GoalEntityAllowsMarker(const void*) { return false; }
void HudSetFollowWarning(bool on)
{
    if (hud::HudType8000* element = hud::HudType8000::FollowElement())
        element->SetAlert(on);
}
bool GoalEntityPhotographable(const void*) { return false; }
void GoalEntityAddRef(void*) {}
void GoalEntityRelease(void*) {}
uint32_t GoalEntityName(const void*) { return 0; }
bool PlayerBinocucomActive() { return false; }
bool GameCameraFrame(float position[3], float forward[3])
{
    position[0] = position[1] = position[2] = 0.0f;
    forward[0] = forward[1] = forward[2] = 0.0f;
    return false;
}
bool GameWorldToScreen(const float*, float screen[2])
{
    screen[0] = screen[1] = 0.0f;
    return false;
}
bool InteractionFlag22(const void*) { return false; }
void* InteractionOwner(const void*) { return nullptr; }
void InteractionTouch(void*) {}

// HUD
void* HudFind(uint32_t) { return nullptr; }
bool HudIsShown(const void*) { return false; }
void HudHide(void*, bool, bool, bool) {}
void HudSetText(void*, const char*, bool) {}
void HudShowText(void*, const char*, bool) {}
bool HudCountdownPaused() { return false; }
bool HudTimerSlotBusy() { return false; }
uint32_t HudCountdownName() { return 0; }

// Minigames
gameplay::Minigame* GameActiveMinigame(SlyGame* game) { return game ? game->minigame : nullptr; }
void GameInstallMinigame(SlyGame* game, gameplay::Minigame* minigame) { if (game) game->SetMinigame(minigame); }
void GameEndActiveMinigame(SlyGame* game) { if (game) game->EndMinigame(); }
void MinigameStart(gameplay::Minigame* minigame) { if (minigame) minigame->Begin(); }
void MinigameCollectMarkers(gameplay::Minigame*, GoalMarkerList*) {}
void GoalAwardTrophy(uint32_t) {}
gameplay::Minigame* CreateMinigame(MinigameKind, SlyGame*, const void*, Goal*) { return nullptr; }

void* JobOwnerCharacter(const Job*) { return nullptr; }
void* CharacterBinocucom(void*) { return nullptr; }
bool BinocucomViewUp(void*) { return false; }
void BinocucomSetPhotoFilter(void*, void*) {}
void BinocucomSetPhotoFlag(void*, bool) {}
void* FindPhotoFilter(uint32_t) { return nullptr; }
void CharacterSetPhotoFlag(void*, bool) {}
void CharacterInputMapSetBit(void*, uint32_t) {}
ape::Matrix34 GoalEntityBoneTransform(void*, uint32_t)
{
    ape::Matrix34 m{};
    m.xAxis.x = m.yAxis.y = m.zAxis.z = 1.0f;
    return m;
}

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {
// TODO(0x00F8F154 +0x10 named object table)
void* FindLevelTableObject(uint32_t name) { (void)name; return nullptr; }
void PlayerTargeting29F428(bool value) { hud::HudType8000::SetFollowElementBlocked(value); }
// TODO(entity slot 16 / 0x00260C14)
void EntityComponent16Set(void* entity, bool value) { (void)entity; (void)value; }
} // namespace goals
} // namespace sly
