// openTIT - The slice of the world/engine API used by the gameplay systems
// (sequencer commands, lasers, minigames).
//
// Game entities, the camera manager and several game-world members are being
// reconstructed by other modules at the same time. Gameplay code only talks
// to them through these free functions; GameplayWorld.cpp holds TODO stubs
// with safe defaults until the real implementations exist. Every hook names
// the console member or virtual slot it stands for.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/math/ApeMatrixOps.h"

#include <cstddef>
#include <cstdint>

namespace ape { class ApeAnimScriptVar; }

namespace sly {

class WorldObject;   // game entity (sly/game/WorldInterface.h)
namespace audio { struct SoundName; struct SoundParams; struct SoundRequest; class AudioBus; }

namespace gameplay {

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------
// Name hash an entity registers under (+0xA8).
uint32_t EntityNameHash(const WorldObject* entity);
// Reference count at +0xB2.
void EntityAddRef(WorldObject* entity);
// Virtual slot 10: drops a reference; with 'destroy' the entity is also
// removed from the world.
void EntityRelease(WorldObject* entity, bool destroy);

// Type bits of field_C0.
bool EntityIsActor(const WorldObject* entity);       // bit 1 (0x02)
bool EntityIsCharacter(const WorldObject* entity);   // bit 3 (0x08)
bool EntityHasController(const WorldObject* entity); // bit 4 (0x10)
// field_C8 bit 8: the entity should be shown when a sequence takes it over.
bool EntityShowsWhenControlled(const WorldObject* entity);
// field_C0 bit 22: the entity implements virtual slot 76 (SetUpdating).
bool EntityHasUpdateHook(const WorldObject* entity);

// field_AC bit 1 / bit 3 / bit 18.
bool EntityIsShown(const WorldObject* entity);
bool EntityHasCollision(const WorldObject* entity);
bool EntityIsInWorld(const WorldObject* entity);

// Virtual slot 44: shows or hides the entity.
void EntitySetShown(WorldObject* entity, bool shown);
// Virtual slot 45: marks the entity as driven by a sequence.
void EntitySetSequenceControlled(WorldObject* entity, bool controlled);
// Virtual slot 24: enables or disables collision.
void EntitySetCollision(WorldObject* entity, bool enabled);
// Virtual slot 22 (0x58): visibility of the entity and its parts.
void EntitySetVisibility(WorldObject* entity, bool visible, bool shadows,
                         bool children, bool effects);
// Virtual slot 76 (0x130) and 0x000DD02C: toggles per-frame updating.
void EntitySetUpdating(WorldObject* entity, bool updating);
void EntityEnableUpdate(WorldObject* entity, bool enable);
// Virtual slot 88 (0x160): plays the death/despawn animation, then removes.
void EntityDespawn(WorldObject* entity);
// Virtual slot 32 (0x80): plays an animation by name hash.
void EntityPlayAnimation(WorldObject* entity, uint32_t animHash, bool loop);

// Render node flags (+0xA4 -> +0x28): bit 12 "sequence lit", bit 10.
bool EntityRenderFlag12(const WorldObject* entity);
bool EntityRenderFlag10(const WorldObject* entity);
void EntitySetRenderFlag12(WorldObject* entity, bool value);
void EntitySetRenderFlag10(WorldObject* entity, bool value);

// Extra set-up done when a sequence takes over an entity
// (0x000C9FC8/0x00124D58 animation-set swap, 0x000DC314/0x00254814 pose reset,
// 0x001243D0/0x00124370 character AI suspend).
void EntityApplySequenceAnimSet(WorldObject* entity);
void EntityResetPose(WorldObject* entity);
void CharacterSuspendAi(WorldObject* entity, bool clearState);

// 0x000D72C0: attaches 'child' to 'parent'.
void EntityAttach(WorldObject* child, WorldObject* parent);
// 0x000DB180: mutes or unmutes the entity's sounds.
void EntitySuppressAudio(WorldObject* entity, bool suppress);
// 0x000DB290: named child entity (part) of an entity.
WorldObject* EntityFindChild(WorldObject* entity, uint32_t childHash);
// 0x000D92B0 and component virtual slot 11: enables a component by name.
// Returns false if the component does not exist.
bool EntityEnableComponent(WorldObject* entity, uint32_t componentHash, bool enable);
// Character virtual slot 16 (physics body) + slot 11 + 0x00260C14.
void CharacterEnablePhysics(WorldObject* character, bool enable, bool alsoBody);
// +0xF4 -> virtual slots 41/40: the entity's scripted mover.
void EntityStartMover(WorldObject* entity, uint32_t a, uint32_t b, uint32_t c, float time);
bool EntityMoverDone(WorldObject* entity);
// +0xF4 -> virtual slot 30: true if the controller accepts sequence control.
bool EntityControllerIsScripted(WorldObject* entity);
// 0x000D1288 (component of type 0x10) + 0x00287260.
void EntitySetEmitterParam(WorldObject* entity, uint32_t value);
// World position (+0x90).
ape::Vec3 EntityPosition(const WorldObject* entity);
// World transform: axes from +0x60, translation +0x90.
ape::Matrix34 EntityTransform(const WorldObject* entity);

// Looks up a named world entity (0x000D6A28).
WorldObject* FindWorldEntity(uint32_t nameHash);

// Every entity of the level (0x01013B34, count 0x01013B3C).
int WorldEntityCount();
WorldObject* WorldEntityAt(int index);

// The controlled player character of a local player (+0x1078, stride
// 0x20C80 per player in the game world).
WorldObject* PlayerCharacter(int player = 0);
// Index of the app player that drives a game player (+0x1071).
int PlayerPadIndex(int player = 0);

// ---------------------------------------------------------------------------
// Game world members (SlyGame) not modelled by sly/game/SlyGame.h yet
// ---------------------------------------------------------------------------
struct WorldFlags {
    uint32_t flags28 = 0;   // +0x28 bit 1: HUD hidden, bit 2: input locked
    uint8_t flags2D = 0;    // +0x2D bit 5: cutscene running
    uint8_t flags2E = 0;    // +0x2E bit 6/7: player/AI pause requests
};
WorldFlags& GetWorldFlags();

// Pause-menu style state requested by the game (virtual slot 8 of the game
// world returns the current mode; 12 means a modal dialog is open).
int GameMode();
constexpr int kGameModeModalDialog = 12;
// 0x00470590: letterbox / cutscene bars.
void SetLetterbox(bool on);
// 0x004897AC: shows or hides the HUD.
void SetHudVisible(bool visible);
// 0x00482B20: shows the level headline.
void ShowLevelHeadline();
// 0x0048A0E4 + 0x00472208 + 0x00365C24 + 0x00371968: switches the game to
// another level/mode.
void RequestGameTransition(uint32_t target);

// Screen colour fade (0x0048C6EC) and the current fade level (0x010BA01C).
void SetScreenFade(float r, float g, float alpha, bool fadeOut);
float CurrentScreenFade();
// Colour filter (0x00470560 on +0x1283A8); its busy flag is +0x1283BC bit 30.
void StartColorFilter(float r, float g, float b, uint32_t mode, bool hold);
bool ColorFilterBusy();
// Depth-of-field blends (0x0048EF50 on +0x1282A4 / +0x1282C0, busy flag
// +0x1282BC bit 31).
void StartDepthOfField(float nearStart, float nearEnd, float farStart, float farEnd, bool both);
bool DepthOfFieldBusy();
// Focus-pull (0x0048EF9C on +0x1282DC, busy flag +0x128304 bit 29).
void StartFocusPull(const float params[7], WorldObject* target, uint32_t mode, bool flag30,
                    bool flag31);
bool FocusPullBusy();
// Bloom/exposure ramp (0x00468C40 on +0x128290, busy flag +0x1282A0 bit 30).
void StartExposureRamp(float time, const uint8_t from[4], const uint8_t to[4], bool hold);
bool ExposureRampBusy();
// Sky/lighting tint (0x00484488).
void SetAmbientTint(float r, float g, float b);
// Text-box messages (0x00488DF4).
void ShowMessageBox(const char* text, const int32_t pos[2], const int32_t size[2]);
// Yes/No prompt (0x00489A60). 'onYes'/'onNo' are invoked with 'user'.
using PromptCallback = void (*)(void* user);
void ShowYesNoPrompt(WorldObject* player, const char* text, const char* buttons,
                     PromptCallback onYes, PromptCallback onNo, void* user);
// Script messages (0x00470660): queued for every listener registered under
// 'listenerKey' (at most 0x80 per frame).
void PostScriptMessage(WorldObject* sender, uint32_t listenerKey, uint32_t message,
                       uint32_t argument);

// Player profile/HUD state reached through 0x00F64C20 -> +0x50.
void SetHudCounterVisible(bool visible);   // +0xA15
void SetHudTimer(float seconds);            // +0xA0C

// Camera shakes (0x002F2140 on 0x00F64C38) and their active count (+0x2C).
// NOTE: the meaning of the individual values is not decoded.
struct CameraShakeParams {
    float values[13] = {};                         // +0x00
    ape::Vec3 position = {0.0f, 0.0f, 0.0f, 0.0f}; // +0x40 centre (from an entity)
    float radius = 0.0f;                           // +0x50
};
void StartCameraShake(const CameraShakeParams& params);
bool AnyCameraShakeActive();

// Camera effects (0x002F0E0C on 0x00F64C38). The returned handle packs the
// effect slot (high 16 bits) and a serial (low 16 bits); 0 means none.
struct CameraEffectParams {
    uint32_t words[29] = {};   // 0x74 bytes, word 2 must be non-zero
};
uint32_t StartCameraEffect(const CameraEffectParams& params);
bool CameraEffectActive(uint32_t handle);
// Scripted camera mover (0x0001B3BC on 0x00F557F0).
void StartCameraMover(uint32_t a, uint32_t b, uint32_t c, float time);

// Global time scale applied to gameplay (0x00F65B4C).
void SetGameTimeScale(float scale);

// Is the given button mask pressed this frame by the pad of a local player?
bool PadPressed(int padIndex, uint32_t mask);
// Character state (+0x17C) virtual slot 16: the subset of 'mask' the current
// state lets the player use.
uint32_t CharacterAcceptedButtons(WorldObject* character, uint32_t mask);

// Audio helpers not modelled by sly/game/AudioSystem.h yet.
void StopAllSounds();                          // 0x00449950
bool IsSoundCategoryPlaying(int category);     // 0x0044D2E0
bool IsTrackedVoicePlaying(uint32_t key);      // 0x0044D4C8
void PlayMusicStream(audio::SoundName* name, bool play);   // 0x00449E48

// ---------------------------------------------------------------------------
// Sound requests
// ---------------------------------------------------------------------------
// 0x004431A0: the request's voice has started buffering (audio::IsRequestReady).
bool StreamSystemIdle(audio::SoundRequest* request);

// ---------------------------------------------------------------------------
// The active game-mode object of the level (+0x1281F4, see Minigame.h).
// ---------------------------------------------------------------------------
class Minigame;
Minigame* ActiveMinigame();

// GUI helpers (0x00363A3C): plays an animation of a GUI object.
void GuiPlayAnimation(float speed, void* gui, const void* animation, bool restart);
// HUD elements: lookup by name hash (0x00368DB0), play (0x003679C8) and
// "animation finished" (0x003814A8).
void* FindHudElement(uint32_t nameHash);
void HudPlayElement(void* element);
bool HudElementDone(void* element);
// Speaker dialogs (0x00489328) and their busy flag (+0x128474 bit 30).
struct DialogSpeaker;
void ShowSpeakerDialog(const DialogSpeaker* speakers, int count, const char* textKey,
                       const char* soundKey, uint32_t screenHash, uint32_t param, bool pauseGame);
bool SpeakerDialogBusy();
// 0x00368FA0 + 0x004A674C: tells a HUD screen a dialog is starting.
void HudScreenNotifyDialog(uint32_t screenHash);
// Text pages (0x004890A0).
void ShowDialogPages(const char (*pages)[0x80], int count, const float pos[2],
                     const float size[2], bool flagA, bool flagB, const uint8_t colorA[4],
                     const uint8_t colorB[4]);
// Script condition objects: int script variables (virtual slot 6, GetInt, non-zero when true).
using ScriptCondition = ape::ApeAnimScriptVar;
bool EvaluateScriptCondition(const ScriptCondition* condition);
// GUI screens and their animations (0x004D5544, 0x004E3168, 0x004E2EF0).
struct GuiAnimation;
void* FindGuiScreen(const char* name);
void GuiStopAnimation(const char* name, void* screen);
GuiAnimation* GuiStartAnimation(const char* name, void* screen, bool loop);
bool GuiAnimationRunning(const GuiAnimation* animation);
void GuiAnimationDelete(GuiAnimation* animation);

// Entity virtual slot 6 after setting field_B8 bit 31.
// NOTE: used to wake up the minigame's player entity; meaning is a guess.
void EntityActivate(WorldObject* entity);

// ---------------------------------------------------------------------------
// Additional hooks used by the second batch of sequencer commands
// (SequenceCommands2.*) and the super attack / minigame code.
// ---------------------------------------------------------------------------

// field_C8 type bits of other entity kinds. NOTE: names are guesses from the
// systems that check them.
bool EntityIsVehicle(const WorldObject* entity);    // field_C8 bit 10 (0x400)
bool EntityIsMount(const WorldObject* entity);      // field_C8 bit 11 (0x800)
bool EntityIsCreature(const WorldObject* entity);   // field_C8 bit 13 (0x2000)
bool EntityIsRiding(const WorldObject* entity);     // field_C0 bit 7 (0x80)
// field_C0 bit 0: the entity can carry attachments (used by skin attach).
bool EntityAcceptsAttachments(const WorldObject* entity);
// field_AC bit 13: the entity's animation is driven by a sequence.
void EntitySetSequenceAnimated(WorldObject* entity, bool value);
// field_AC bit 14 (0x4000): the animation set is overridden by a sequence.
void EntitySetAnimSetOverridden(WorldObject* entity, bool value);
// Entity +0x150: the entity this one is linked to (see SeqPlaceEntityCommand).
WorldObject* EntityLinkedParent(const WorldObject* entity);

// Placement of an entity relative to another (0x000D15C0 + 0x000D7A00).
struct EntityPlacement {
    ape::Vec3 position = {0.0f, 0.0f, 0.0f, 0.0f};   // +0x00 local offset
    ape::Vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};   // +0x10 quaternion
    uint32_t nodeHash = 0;                           // +0x20 node of 'relativeTo'
    uint32_t field_28 = 0;                           // +0x28 NOTE: meaning unknown
    bool flag31 = false;                             // +0x30 bits copied from the script
    bool flag25 = false;
    bool flag28 = false;
    bool flag29 = false;
    WorldObject* relativeTo = nullptr;               // +0x40
};
void EntityApplyPlacement(WorldObject* entity, const EntityPlacement& placement);
void EntityCommitPlacement(WorldObject* entity);

// Walks an entity's child hierarchy (0x0057F5D8); 'visit' returns false to
// stop the walk.
using EntityVisitor = bool (*)(WorldObject* node, void* user);
void WalkEntityChildren(WorldObject* root, EntityVisitor visit, void* user);
// Render node flag word (+0xA4) bit 8.
void EntitySetRenderFlag8(WorldObject* entity, bool value);
// Virtual slot 10 with two flags (activate/deactivate).
// NOTE: the same slot is EntityRelease() for pooled objects; the sequencer
// uses it to switch an entity on or off.
void EntitySetActive(WorldObject* entity, bool active, bool children);

// Characters keep several behaviour states: +0x17C is the active one,
// +0x180 the default, +0x184 the idle one and +0x188 the scripted one.
enum class CharacterStateSlot { Default, Idle, Scripted };
void CharacterActivateState(WorldObject* character, CharacterStateSlot slot);
bool CharacterHasActiveState(const WorldObject* character);
bool CharacterActiveStateIs(const WorldObject* character, CharacterStateSlot slot);
// Starts an action of the active state's sub-machine (state +0x50 virtual
// slot 7) and returns its parameter block (+0x64), or null without an
// active state.
void* CharacterStartAction(WorldObject* character, int action);
// State +0x50 virtual slot 8: true while the action still runs.
bool CharacterActionRunning(const WorldObject* character);
// Mounts (0x00230990, action holder +0x3B8) and creatures (0x00246D50,
// holder +0x190) have action machines of their own; same contract.
void* EntityStartAction(WorldObject* entity, int action);
bool EntityActionRunning(const WorldObject* entity);
bool EntityHasAction(const WorldObject* entity);
void EntityFinishAction(WorldObject* entity);   // 0x0023DF6C / 0x0024DDBC
// Character virtual slot 94: head tracking component (slot 11 enables it).
void CharacterEnableHeadTracking(WorldObject* character, bool enable);
// Character virtual slot 16: physics body (slot 11 + 0x00260C14); 'wake'
// also calls the body's slot 4.
void CharacterEnableBody(WorldObject* character, bool enable, bool wake);
// 0x00121E8C, +0x1B0, field_B8 bits 2/19, 0x0011F0E4: stops any movement.
void CharacterResetMovement(WorldObject* character);
// 0x00128EE0: returns the character to its idle state.
void CharacterReturnToIdle(WorldObject* character);
// 0x00129C18: makes the AI forget its current animation.
void CharacterResetAnimState(WorldObject* character);
// Character virtual slot 8 (0x20): current behaviour state id.
int CharacterStateId(WorldObject* character);
// Character virtual slot 7 (0x1C) with two zero arguments.
void CharacterStopAction(WorldObject* character);
// 0x00124D58 / 0x00238270: replaces the animation set of a character or
// mount; a temporary set is dropped again when the sequence ends.
void CharacterSwapAnimSet(WorldObject* character, void* animSet, bool temporary);
void MountSwapAnimSet(WorldObject* mount, void* animSet, bool temporary);
// 0x000C9FC8: animation set resource by name hash.
void* FindAnimSet(uint32_t nameHash);
// Character slot 16 body + 0x00261328.
void CharacterRefreshBody(WorldObject* character);

// Player control (character +0x180 slot 11/4, vehicle slot 99, riding
// slot 36 + 0x0091D470) and entity virtual slot 11.
void CharacterEnableDefaultState(WorldObject* character, bool enable);
void VehicleEnableControl(WorldObject* vehicle, bool enable);
void RiderEnableControl(WorldObject* rider, bool enable);
void EntityRefresh(WorldObject* entity);

// Hits (character virtual slot 86 with a hit record, vtable 0x00B0DADC).
struct HitInfo {
    uint32_t source = 0;                             // +0x04
    int32_t mode = 0;                                // +0x08
    uint16_t field_0C = 0;                           // +0x0C
    uint16_t kind = 3;                               // +0x0E bits 13-15
    ape::Vec3 direction = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x10
    float force = 0.0f;                              // +0x20
    float lift = 0.0f;                               // +0x24
};
void CharacterApplyHit(WorldObject* character, const HitInfo& hit);
// Actor virtual slot 88 (0x160): applies damage (no source) of a type.
void ActorDamage(WorldObject* actor, float amount, int32_t mode);
// Actor health (+0x15C) and maximum (+0x158).
float ActorHealth(const WorldObject* actor);
float ActorMaxHealth(const WorldObject* actor);
void ActorSetHealth(WorldObject* actor, float health);
// Actor virtual slot 80 (0x140): true while the actor is dying.
bool ActorIsDying(WorldObject* actor);
// +0xF4: true if the actor has no controller or its controller is set up.
bool ActorControllerReady(const WorldObject* actor);
// Facing direction (+0x80).
ape::Vec3 EntityForward(const WorldObject* entity);
// Anim script clips (anim script virtual slot 15 + 0x002775A8 with a 0.25s
// blend), their speed (0x00276E4C) and completion (0x0029879C).
void ActorPlayScriptClip(WorldObject* actor, uint32_t clipHash, float speed, bool loop);
void ActorSetClipSpeed(WorldObject* actor, float speed);
bool ActorClipFinished(WorldObject* actor);
// Current animation time and length (0x000DC488 / 0x000DC418).
float EntityAnimTime(WorldObject* entity);
float EntityAnimLength(WorldObject* entity);
// Button hint popup (0x004A5FC8).
void ShowButtonHint(uint32_t hintHash);
// Analogue stick axis of a pad (0x0045B160; 0/1 left X/Y, 2/3 right X/Y).
float PadAxis(int padIndex, int axis);

// Skins (0x000D94F4 active skin, +4 previous, slot 18 switch, 0x000D91BC
// removal, 0x00296810 attachment).
struct EntitySkin;
EntitySkin* EntityActiveSkin(WorldObject* entity);
EntitySkin* SkinPrevious(const EntitySkin* skin);
bool EntitySwitchSkin(WorldObject* entity, EntitySkin* skin);
void EntityRemoveSkin(WorldObject* entity, EntitySkin* skin);
void SkinAttach(EntitySkin* skin, WorldObject* child, uint32_t socket);

// Animation clips (anim script virtual slot 15 + 0x002775A8, otherwise
// entity virtual slot 50) and their completion (0x000D8760).
bool EntityPlayClip(WorldObject* entity, uint32_t clipHash, uint32_t nextHash, float blendIn,
                    float speed, bool loop, uint32_t param);
bool EntityClipDone(WorldObject* entity, uint32_t clipHash);
void EntityClipFailed(WorldObject* entity);   // virtual slot 19
void EntityResetAnimTime(WorldObject* entity); // render node +0x74 -> +0xA8/+0xAC

// Camera manager (game world +0x1281E0).
struct GameCamera;
GameCamera* TopCamera();
GameCamera* NextCamera(GameCamera* camera);
int CameraType(GameCamera* camera);              // camera virtual slot 2
constexpr int kCameraTypeScripted = 11;
void PopCameraBlended(float blendTime);          // manager slot 53 with blend +0x250
void KillCamera(GameCamera* camera);             // 0x000FEACC + camera slot 7
void ResetCameraManager();                       // 0x000FCD1C
// The camera manager exists, its current transform (axes +0x60,
// translation +0x90) and field of view (+0x164).
bool HasCameraManager();
ape::Matrix34 CameraManagerTransform();
float CameraManagerFov();
struct CameraView {
    ape::Matrix34 transform;                     // axes as stored by camera objects
    float fov = 53.5f;
    float nearClip = 7.5f;
    uint8_t color[4] = {};
};
void SetCameraView(const CameraView& view);      // 0x000FB31C
// Camera definition (+0x70 matrix, +0xB0 fov) and locator (+0x50 matrix).
const ape::Matrix34* CameraObjectMatrix(const WorldObject* camera, float* fov);
const ape::Matrix34* LocatorMatrix(const WorldObject* locator);
const uint8_t* DefaultCameraColor();             // 0x01164CA8

// HUD screens and layers (GUI manager 0x00F65BE8).
struct HudScreen;
struct HudLayer;
void HudShowAll();                               // 0x00454DBC
void HudHideAll();                               // 0x00454E88
HudScreen* FindHudScreenByHash(uint32_t nameHash);
void HudScreenSetPersistent(HudScreen* screen, bool value);   // 0x00453578
void HudScreenRestart(HudScreen* screen);                     // 0x0045477C
void HudScreenShow(HudScreen* screen, bool show);             // slots 6 / 7
void HudScreenSkipToEnd(HudScreen* screen);                   // 0x004536C0
HudLayer* FindHudLayerByHash(uint32_t nameHash);
void HudLayerSetPersistent(HudLayer* layer, bool value);      // 0x004563A0
void HudLayerRestart(HudLayer* layer);                        // 0x0045488C
void HudLayerShow(HudLayer* layer, bool show);                // 0x00454220 / 0x004562F4

// ---------------------------------------------------------------------------
// Sockets for the debug network server (sys_net / cellNetCtl on the console).
// NOTE: these belong in src/platform once it has a socket layer.
// ---------------------------------------------------------------------------
struct NetAddress {
    uint8_t ip[4] = {};
    uint16_t port = 0;
};
enum class NetProtocol { Udp, Tcp };
enum class NetOption { ReuseAddress, Broadcast, NoDelay };
// Loads the network modules (sys_net_initialize_network_ex + cellNetCtlInit).
bool NetInitialize();
// Local IP address as "a.b.c.d" (cellNetCtlGetInfo 16); false if offline.
bool NetLocalIpString(char* out, size_t outSize);
int NetOpen(NetProtocol protocol);              // socket(); negative on failure
bool NetSetOption(int socket, NetOption option);
bool NetBind(int socket, const NetAddress& address);
bool NetListen(int socket, int backlog);
int NetAccept(int socket, NetAddress* peer);    // negative/zero on failure
int NetReceive(int socket, void* buffer, size_t size);
int NetSend(int socket, const void* buffer, size_t size);
int NetSendTo(int socket, const void* buffer, size_t size, const NetAddress& to);
void NetClose(int socket);
// +0xB0 bit 12: asks the world to remove the entity at the end of the frame.
void EntityMarkForRemoval(WorldObject* entity);
// 0x0048F6B0 (event listener base destructor): takes a listener out of
// every listener group it joined.
void ListenerLeaveAllGroups(void* listener);
// Sound playback helpers of the sequencer (audio module, not modelled yet).
// The VoiceId results are audio::VoiceId values.
uint32_t PlaySound2D(audio::SoundName* sound, const audio::SoundParams& params);   // 0x0044A374
uint32_t EntityPlaySound(WorldObject* entity, audio::SoundName* sound,
                         const audio::SoundParams& params, audio::AudioBus* bus);  // 0x000DB348
void PrepareSoundRequest();                                                        // 0x004440F4
uint32_t PlaySoundRequest(audio::SoundRequest* request,
                          const audio::SoundParams& params);                       // 0x00443EC0
uint32_t EntityPlaySoundRequest(WorldObject* entity, audio::SoundRequest* request,
                                const audio::SoundParams& params,
                                audio::AudioBus* bus);                             // 0x000DBA84
// Sound emitters placed in the level (0x000F5FFC start, 0x000F60DC busy).
void EmitterStart(uint32_t emitter);
bool EmitterPlaying(uint32_t emitter);
// Subtitles option (game world +0x1030 bit 27) and display (0x004A8CD0).
bool SubtitlesEnabled();
void ShowSubtitle(const char* textKey, uint32_t param, audio::SoundRequest* request);
// Entity field_B8 bit 0: sounds of this entity use the alternative bus.
bool EntityUsesAltSoundBus(const WorldObject* entity);
// State byte (+0x12) of the level object that owns a goal.
uint8_t GoalOwnerState(const WorldObject* owner);
// Name of the current level (0x0047C46C on the game world).
const char* CurrentLevelName();

} // namespace gameplay
} // namespace sly
