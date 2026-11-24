// openTIT - TODO stubs for the goal module's outside calls (see GoalHooks.h).
#include "sly/game/goals/GoalHooks.h"

namespace sly {
namespace goals {

// Jobs / job manager
uint32_t JobGoalCount(const Job*) { return 0; }
Goal* JobGoalAt(const Job*, uint32_t) { return nullptr; }
uint32_t JobGoalSaveWord(const Job*, int) { return 0; }
void JobSetGoalSaveWord(Job*, int, uint32_t) {}
uint8_t JobOwnerFlag(const Job*) { return 0; }
bool IsGoalInLiveJob(const Goal*) { return false; }
void JobManagerGoalFailed(Goal*, uint8_t) {}
void JobManagerBeforeFailSequence() {}
Goal* JobManagerTrackedGoal() { return nullptr; }
Goal* JobManagerGoalAfterTracked() { return nullptr; }
const GoalScriptBinding* JobManagerJobBindings(size_t* count)
{
    *count = 0;
    return nullptr;
}
void JobManagerSetTimerGoal(Goal*) {}
void JobManagerGroupAdvanced() {}
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
void* HintFindLaserGroup(uint32_t) { return nullptr; }
void* HintFindLaser(uint32_t) { return nullptr; }
void* HintFindLaserField(uint32_t) { return nullptr; }
void HintEntitySetEnabled(void*, bool) {}
void HintInteractableSetEnabled(void*, bool) {}
int HintInteractionState(const void*) { return 0; }
void HintInteractionSetState(void*, int) {}
void HintLaserGroupEnable(void*, bool) {}
void HintLaserEnable(void*, bool) {}
void HintLaserFieldEnable(void*, bool) {}

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
void GoalListenTo(Goal*, uint32_t) {}
void GoalStopListening(Goal*) {}
void* SpawnMousetrapHelper() { return nullptr; }
void* GoalEntityParent(const void*) { return nullptr; }
bool GoalEntityMarkerSuppressed(const void*) { return false; }
bool GoalEntityAllowsMarker(const void*) { return false; }
void HudSetFollowWarning(bool) {}
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
gameplay::Minigame* GameActiveMinigame(SlyGame*) { return nullptr; }
void GameInstallMinigame(SlyGame*, gameplay::Minigame*) {}
void GameEndActiveMinigame(SlyGame*) {}
void MinigameStart(gameplay::Minigame*) {}
void MinigameCollectMarkers(gameplay::Minigame*, GoalMarkerList*) {}
gameplay::Minigame* CreateMinigame(MinigameKind, SlyGame*, const void*, Goal*) { return nullptr; }

} // namespace goals
} // namespace sly
