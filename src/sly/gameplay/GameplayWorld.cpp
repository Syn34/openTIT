// openTIT - TODO stubs for the world/engine API used by the gameplay systems
// (see GameplayWorld.h). They return safe defaults until the world, camera
// and HUD modules provide the real implementations.
#include "sly/gameplay/GameplayWorld.h"

#include "sly/game/AppRuntime.h"
#include "sly/game/SlyGame.h"
#include "sly/game/WorldInterface.h"

namespace sly {
namespace gameplay {

// -- entities ----------------------------------------------------------------
uint32_t EntityNameHash(const WorldObject* entity) { return WorldObjectKey(entity); }
void EntityAddRef(WorldObject*) {}
void EntityRelease(WorldObject*, bool) {}
bool EntityIsActor(const WorldObject*) { return false; }
bool EntityIsCharacter(const WorldObject*) { return false; }
bool EntityHasController(const WorldObject*) { return false; }
bool EntityShowsWhenControlled(const WorldObject*) { return false; }
bool EntityHasUpdateHook(const WorldObject*) { return false; }
bool EntityIsShown(const WorldObject*) { return true; }
bool EntityHasCollision(const WorldObject*) { return true; }
bool EntityIsInWorld(const WorldObject* entity) { return SpawnedIsAlive(entity); }
void EntitySetShown(WorldObject*, bool) {}
void EntitySetSequenceControlled(WorldObject*, bool) {}
void EntitySetCollision(WorldObject*, bool) {}
void EntitySetVisibility(WorldObject*, bool, bool, bool, bool) {}
void EntitySetUpdating(WorldObject*, bool) {}
void EntityEnableUpdate(WorldObject*, bool) {}
void EntityDespawn(WorldObject*) {}
void EntityPlayAnimation(WorldObject*, uint32_t, bool) {}
bool EntityRenderFlag12(const WorldObject*) { return false; }
bool EntityRenderFlag10(const WorldObject*) { return false; }
void EntitySetRenderFlag12(WorldObject*, bool) {}
void EntitySetRenderFlag10(WorldObject*, bool) {}
void EntityApplySequenceAnimSet(WorldObject*) {}
void EntityResetPose(WorldObject*) {}
void CharacterSuspendAi(WorldObject*, bool) {}
void EntityAttach(WorldObject*, WorldObject*) {}
void EntitySuppressAudio(WorldObject*, bool) {}
WorldObject* EntityFindChild(WorldObject*, uint32_t) { return nullptr; }
bool EntityEnableComponent(WorldObject*, uint32_t, bool) { return false; }
void CharacterEnablePhysics(WorldObject*, bool, bool) {}
void EntityStartMover(WorldObject*, uint32_t, uint32_t, uint32_t, float) {}
bool EntityMoverDone(WorldObject*) { return true; }
bool EntityControllerIsScripted(WorldObject*) { return false; }
void EntitySetEmitterParam(WorldObject*, uint32_t) {}
ape::Vec3 EntityPosition(const WorldObject* entity) { return CharacterPosition(entity); }
WorldObject* FindWorldEntity(uint32_t nameHash) { return FindLevelInstance(nameHash); }
int WorldEntityCount() { return 0; }
WorldObject* WorldEntityAt(int) { return nullptr; }
WorldObject* PlayerCharacter(int player) { return player == 0 && g_game ? g_game->playerObject : nullptr; }
int PlayerPadIndex(int player) { return player; }

// -- game world ----------------------------------------------------------------
WorldFlags& GetWorldFlags()
{
    static WorldFlags flags;
    return flags;
}

int GameMode() { return 0; }
void SetLetterbox(bool) {}
void SetHudVisible(bool) {}
void ShowLevelHeadline() {}
void RequestGameTransition(uint32_t) {}

namespace {
float g_screenFade = 1.0f;   // 0x010BA01C
float g_timeScale = 1.0f;    // 0x00F65B4C
} // namespace

void SetScreenFade(float, float, float alpha, bool) { g_screenFade = alpha; }
float CurrentScreenFade() { return g_screenFade; }
void StartColorFilter(float, float, float, uint32_t, bool) {}
bool ColorFilterBusy() { return false; }
void StartDepthOfField(float, float, float, float, bool) {}
bool DepthOfFieldBusy() { return false; }
void StartFocusPull(const float*, WorldObject*, uint32_t, bool, bool) {}
bool FocusPullBusy() { return false; }
void StartExposureRamp(float, const uint8_t*, const uint8_t*, bool) {}
bool ExposureRampBusy() { return false; }
void SetAmbientTint(float, float, float) {}
void ShowMessageBox(const char*, const int32_t*, const int32_t*) {}
void ShowYesNoPrompt(WorldObject*, const char*, const char*, PromptCallback, PromptCallback onNo,
                     void* user)
{
    // Without a HUD the prompt is answered with "no" at once.
    if (onNo != nullptr)
        onNo(user);
}
void PostScriptMessage(WorldObject*, uint32_t, uint32_t, uint32_t) {}
void SetHudCounterVisible(bool) {}
void SetHudTimer(float) {}
void StartCameraShake(const CameraShakeParams&) {}
bool AnyCameraShakeActive() { return false; }
uint32_t StartCameraEffect(const CameraEffectParams&) { return 0; }
bool CameraEffectActive(uint32_t) { return false; }
void StartCameraMover(uint32_t, uint32_t, uint32_t, float) {}
void SetGameTimeScale(float scale) { g_timeScale = scale; }

bool PadPressed(int padIndex, uint32_t mask)
{
    if (padIndex < 0 || padIndex >= kLocalPlayerCount)
        return false;
    const AppPlayer& player = GetAppRuntime().players[padIndex];
    return (player.inputFlags & player.inputMask & mask) != 0;
}

uint32_t CharacterAcceptedButtons(WorldObject*, uint32_t) { return 0; }

void StopAllSounds() {}
bool IsSoundCategoryPlaying(int) { return false; }
bool IsTrackedVoicePlaying(uint32_t) { return false; }
void PlayMusicStream(audio::SoundName*, bool) {}

// -- sound requests ------------------------------------------------------------
bool StreamSystemIdle() { return true; }

// -- misc ----------------------------------------------------------------------
Minigame* ActiveMinigame() { return nullptr; }
void GuiPlayAnimation(float, void*, const void*, bool) {}
void EntityActivate(WorldObject*) {}
void* FindHudElement(uint32_t) { return nullptr; }
void HudPlayElement(void*) {}
bool HudElementDone(void*) { return true; }
void ShowSpeakerDialog(const DialogSpeaker*, int, const char*, const char*, uint32_t, uint32_t,
                       bool) {}
bool SpeakerDialogBusy() { return false; }
void HudScreenNotifyDialog(uint32_t) {}
void ShowDialogPages(const char (*)[0x80], int, const float*, const float*, bool, bool,
                     const uint8_t*, const uint8_t*) {}
bool EvaluateScriptCondition(const ScriptCondition*) { return true; }
void* FindGuiScreen(const char*) { return nullptr; }
void GuiStopAnimation(const char*, void*) {}
GuiAnimation* GuiStartAnimation(const char*, void*, bool) { return nullptr; }
bool GuiAnimationRunning(const GuiAnimation*) { return false; }
void GuiAnimationDelete(GuiAnimation*) {}

// -- second batch (SequenceCommands2, super attacks, minigames) ---------------
bool EntityIsVehicle(const WorldObject*) { return false; }
bool EntityIsMount(const WorldObject*) { return false; }
bool EntityIsCreature(const WorldObject*) { return false; }
bool EntityIsRiding(const WorldObject*) { return false; }
bool EntityAcceptsAttachments(const WorldObject*) { return false; }
void EntitySetSequenceAnimated(WorldObject*, bool) {}
void EntitySetAnimSetOverridden(WorldObject*, bool) {}
WorldObject* EntityLinkedParent(const WorldObject*) { return nullptr; }
void EntityApplyPlacement(WorldObject*, const EntityPlacement&) {}
void EntityCommitPlacement(WorldObject*) {}
void WalkEntityChildren(WorldObject*, EntityVisitor, void*) {}
void EntitySetRenderFlag8(WorldObject*, bool) {}
void EntitySetActive(WorldObject*, bool, bool) {}
void CharacterActivateState(WorldObject*, CharacterStateSlot) {}
bool CharacterHasActiveState(const WorldObject*) { return false; }
bool CharacterActiveStateIs(const WorldObject*, CharacterStateSlot) { return false; }
void* CharacterStartAction(WorldObject*, int) { return nullptr; }
bool CharacterActionRunning(const WorldObject*) { return false; }
void* EntityStartAction(WorldObject*, int) { return nullptr; }
bool EntityActionRunning(const WorldObject*) { return false; }
bool EntityHasAction(const WorldObject*) { return false; }
void EntityFinishAction(WorldObject*) {}
void CharacterEnableHeadTracking(WorldObject*, bool) {}
void CharacterEnableBody(WorldObject*, bool, bool) {}
void CharacterResetMovement(WorldObject*) {}
void CharacterReturnToIdle(WorldObject*) {}
void CharacterResetAnimState(WorldObject*) {}
int CharacterStateId(WorldObject*) { return 0; }
void CharacterStopAction(WorldObject*) {}
void CharacterSwapAnimSet(WorldObject*, void*, bool) {}
void MountSwapAnimSet(WorldObject*, void*, bool) {}
void* FindAnimSet(uint32_t) { return nullptr; }
void CharacterRefreshBody(WorldObject*) {}
void CharacterEnableDefaultState(WorldObject*, bool) {}
void VehicleEnableControl(WorldObject*, bool) {}
void RiderEnableControl(WorldObject*, bool) {}
void EntityRefresh(WorldObject*) {}
void CharacterApplyHit(WorldObject*, const HitInfo&) {}
void ActorDamage(WorldObject*, float, int32_t) {}
float ActorHealth(const WorldObject*) { return 1.0f; }
float ActorMaxHealth(const WorldObject*) { return 1.0f; }
void ActorSetHealth(WorldObject*, float) {}
bool ActorIsDying(WorldObject*) { return false; }
bool ActorControllerReady(const WorldObject*) { return true; }
ape::Vec3 EntityForward(const WorldObject*) { return {0.0f, 0.0f, 1.0f, 0.0f}; }
void ActorPlayScriptClip(WorldObject*, uint32_t, float, bool) {}
void ActorSetClipSpeed(WorldObject*, float) {}
bool ActorClipFinished(WorldObject*) { return true; }
float EntityAnimTime(WorldObject*) { return 0.0f; }
float EntityAnimLength(WorldObject*) { return 1.0f; }
void ShowButtonHint(uint32_t) {}
float PadAxis(int, int) { return 0.0f; }
EntitySkin* EntityActiveSkin(WorldObject*) { return nullptr; }
EntitySkin* SkinPrevious(const EntitySkin*) { return nullptr; }
bool EntitySwitchSkin(WorldObject*, EntitySkin*) { return false; }
void EntityRemoveSkin(WorldObject*, EntitySkin*) {}
void SkinAttach(EntitySkin*, WorldObject*, uint32_t) {}
bool EntityPlayClip(WorldObject*, uint32_t, uint32_t, float, float, bool, uint32_t) { return false; }
bool EntityClipDone(WorldObject*, uint32_t) { return true; }
void EntityClipFailed(WorldObject*) {}
void EntityResetAnimTime(WorldObject*) {}
GameCamera* TopCamera() { return nullptr; }
GameCamera* NextCamera(GameCamera*) { return nullptr; }
int CameraType(GameCamera*) { return 0; }
void PopCameraBlended(float) {}
void KillCamera(GameCamera*) {}
void ResetCameraManager() {}
void SetCameraView(const CameraView&) {}
const ape::Matrix34* CameraObjectMatrix(const WorldObject*, float*) { return nullptr; }
const ape::Matrix34* LocatorMatrix(const WorldObject*) { return nullptr; }
const uint8_t* DefaultCameraColor()
{
    static const uint8_t color[4] = {0, 0, 0, 0};
    return color;
}
void HudShowAll() {}
void HudHideAll() {}
HudScreen* FindHudScreenByHash(uint32_t) { return nullptr; }
void HudScreenSetPersistent(HudScreen*, bool) {}
void HudScreenRestart(HudScreen*) {}
void HudScreenShow(HudScreen*, bool) {}
void HudScreenSkipToEnd(HudScreen*) {}
HudLayer* FindHudLayerByHash(uint32_t) { return nullptr; }
void HudLayerSetPersistent(HudLayer*, bool) {}
void HudLayerRestart(HudLayer*) {}
void HudLayerShow(HudLayer*, bool) {}

// -- debug network sockets (no socket layer yet: everything fails) -------------
bool NetInitialize() { return false; }
bool NetLocalIpString(char*, size_t) { return false; }
int NetOpen(NetProtocol) { return -1; }
bool NetSetOption(int, NetOption) { return false; }
bool NetBind(int, const NetAddress&) { return false; }
bool NetListen(int, int) { return false; }
int NetAccept(int, NetAddress*) { return -1; }
int NetReceive(int, void*, size_t) { return -1; }
int NetSend(int, const void*, size_t) { return -1; }
int NetSendTo(int, const void*, size_t, const NetAddress&) { return -1; }
void NetClose(int) {}
const char* CurrentLevelName() { return ""; }
uint8_t GoalOwnerState(const WorldObject*) { return 0; }
void EntityMarkForRemoval(WorldObject*) {}
uint32_t PlaySound2D(audio::SoundName*, const audio::SoundParams&) { return 0; }
uint32_t EntityPlaySound(WorldObject*, audio::SoundName*, const audio::SoundParams&,
                         audio::AudioBus*)
{
    return 0;
}
void PrepareSoundRequest() {}
uint32_t PlaySoundRequest(audio::SoundRequest*, const audio::SoundParams&) { return 0; }
uint32_t EntityPlaySoundRequest(WorldObject*, audio::SoundRequest*, const audio::SoundParams&,
                                audio::AudioBus*)
{
    return 0;
}
void EmitterStart(uint32_t) {}
bool EmitterPlaying(uint32_t) { return false; }
bool SubtitlesEnabled() { return false; }
void ShowSubtitle(const char*, uint32_t, audio::SoundRequest*) {}
bool EntityUsesAltSoundBus(const WorldObject*) { return false; }

} // namespace gameplay
} // namespace sly
