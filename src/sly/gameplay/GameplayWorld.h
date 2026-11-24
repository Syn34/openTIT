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

#include <cstdint>

namespace sly {

class WorldObject;   // game entity (sly/game/WorldInterface.h)
namespace audio { struct SoundName; }

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
// 0x004431A0. NOTE: purpose unknown; the stream wait of SeqEnqueueStream is
// skipped while it returns true.
bool StreamSystemIdle();

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
void ShowDialogPages(const char (*pages)[0x80], int count, const int32_t pos[2],
                     const int32_t size[2], bool flagA, bool flagB, const uint8_t colorA[4],
                     const uint8_t colorB[4]);
// Script condition objects (virtual slot 6 returns non-zero when true).
class ScriptCondition;
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

} // namespace gameplay
} // namespace sly
