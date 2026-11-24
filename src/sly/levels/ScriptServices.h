// openTIT - More game services used by the script commands #83-#269.
//
// Same idea as GameServices.h: each free function stands for a console
// function or field in a system outside this module, and ScriptServices.cpp
// holds TODO stubs returning the "missing" value. The console function or
// field is named in the comment of each declaration.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>
#include <vector>

namespace ape { class ApeRefCounted; class ApeAnimScriptInstance; class ApeAnimScriptVar; namespace xml { struct Color8; } }

namespace sly {

class WorldObject;
struct TextField;
namespace audio { struct SoundName; struct MixState; }

namespace levels {

// ---------------------------------------------------------------------------
// Players (the game world 0x00F65CA0 keeps one 0x20C80-byte record per
// player; the player entity is at +0x1078 of the record).

// Entity of player `index`, may be null.
WorldObject* PlayerEntityByIndex(int index);

// Button mask of a player's controller component (character +0x17C when that
// component is of kind 5): all buttons on or off (0x0027C8C8).
void PlayerEnableAllButtons(WorldObject* player, bool enable);
// One button by name (button table 0x00F64A78, 31 entries; 0x0027C914).
void PlayerEnableButton(WorldObject* player, uint32_t button, bool enable);
// Player control switch: character bit 7 of +0x3C8; other entities bit 29
// of +0x180 when class flag 0x400 (+0xC8) is set.
void PlayerSetControlEnabled(WorldObject* player, bool enable);

// Virtual slot 7 of entities (two integer arguments).
// NOTE: OpenBinocucom calls it with (0, 0) then (1, 7); meaning unknown.
void EntityVirtualCommand(WorldObject* entity, int a, int b);

// ---------------------------------------------------------------------------
// Game world (0x00F65CA0) and game state (0x00F64C20)

// 0x00489EAC: if the world object +0x12847C is flagged (bit 31 of +0xA4),
// runs 0x00425E54 on it. NOTE: purpose unknown.
void GameWorldResumeObject12847C();
// Byte +0xA15 of the game-state object +0x50 (if present). NOTE: a pause
// permission flag.
void GameStateSetPauseFlag(bool value);
// 0x00488DF4: shows an overlay picture by name with a 2D scale and offset.
void GameWorldShowOverlay(const char* name, const float scale[2], const float offset[2]);

// 0x00468C40 on the world's screen fader (+0x128290): fades from `start` to
// `end` over `duration` seconds. NOTE: `flag` meaning unknown.
void ScreenColorFade(const ape::xml::Color8& start, const ape::xml::Color8& end, float duration, bool flag);
// 0x0046C3B8: queues the script `script` of library `library` as a cut scene,
// with optional texts and a spawn point index.
void GameWorldQueueCutscene(uint32_t script, uint32_t library, bool flag,
                            const std::vector<TextField>& texts, int32_t spawnPoint,
                            ape::ApeAnimScriptInstance* instance);

// ---------------------------------------------------------------------------
// Entities

// 0x000E1288: hands text messages to a killable entity.
void EntityShowMessages(WorldObject* entity, const std::vector<TextField>& texts,
                        ape::ApeAnimScriptInstance* instance);
// 0x0048FA54: adds the entity to an event listener group.
void EntityRegisterListener(WorldObject* entity, uint32_t group, bool flag);
// 0x000D949C: the entity's targeting gadget (null if it has none).
void* EntityTargetingGadget(WorldObject* entity);
// 0x0029F7A0: points the gadget at a target entity.
void GadgetSetTarget(void* gadget, WorldObject* target);

// User tag "Entity" (some world objects carry their entity under this tag
// instead of "pEntity").
constexpr uint32_t kUserTag_EntityAlt = 0x7A30D7E3u;  // HashNameNoCase("Entity")
// Second class flag word of entities (+0xC8).
bool EntityHasClassFlag2(const WorldObject* entity, uint32_t mask);
constexpr uint32_t kEntityClass2_Character = 0x08;  // NOTE: inferred from use
// Character resets used by script command 0x8D0B7E84 (0x001F4460, 0x001ED9D4).
// NOTE: what each resets is unknown.
void CharacterReset1F4460(WorldObject* character);
void CharacterReset1ED9D4(WorldObject* character);
// Sets bits of the game world option byte +0x2D.
void SetGameWorldFlag2D(uint8_t mask, bool value);
// 0x002ABD08: sets a mode of the binocucom gadget.
void BinocucomSetMode(void* binocucom, uint32_t mode);

// Binocucom gadget (0x000D94CC gives the player's).
// Bit 11 of +0xF4: the binocucom is locked.
void BinocucomSetLocked(void* binocucom, bool locked);
// 0x002AB428 (static on the binocucom class).
void BinocucomSetPower(float power);
// +0x998: view type index (0 normal, 1 thermal).
void BinocucomSetViewType(void* binocucom, int type);
// 0x002AA9C8 (static): script name the binocucom runs.
void BinocucomSetScript(uint32_t scriptName);
// Console global 0x010592F4. NOTE: what the rate drives is unknown.
void SetGlobalRate010592F4(float rate);
// World objects of kind 8 (bits 10-15 of +4): render instances.
bool ObjectIsRenderInstance(const ape::ApeRefCounted* object);
// Virtual slot 13 of render instances: alpha 0-255. NOTE: `flag` meaning unknown.
void RenderInstanceSetAlpha(ape::ApeRefCounted* object, uint8_t alpha, bool flag);
// Character abilities: byte offset of an ability record by name (0x001205AC),
// -1 if unknown; bit 6 (blocked) / bit 7 (enabled) of record +0x290.
int CharacterFindAbility(uint32_t abilityName);
void CharacterSetAbilityBlocked(WorldObject* character, int ability, bool blocked);
void CharacterSetAbilityEnabled(WorldObject* character, int ability, bool enabled);
// Character controller: +0x17C = default controller +0x180; 0x00128EE0;
// virtual slot 11 of the current controller.
void CharacterRestoreDefaultController(WorldObject* character);
void CharacterResetController(WorldObject* character);
void CharacterEnableController(WorldObject* character, bool enable);

// Components: entity component by id (0x000D92B0), its kind (+0x0C).
void* EntityFindComponent(WorldObject* entity, uint32_t componentId);
int ComponentKind(const void* component);
// 0x002B78F0: script components run the named script.
void ScriptComponentSetScript(void* component, uint32_t script);
// Virtual slot 13 of components (visible, flag).
void ComponentSetVisible(void* component, bool visible, bool flag);
// Debug names: entity definition name text (definition +0x38) and the
// entity's name (virtual slot 19).
const char* EntityDefNameString(WorldObject* entity);
const char* EntityNameString(WorldObject* entity);
// 0x00464488 on the controller record 0x010B1A90 + index * 0x2EC0.
void ControllerRumble(int controllerIndex, float strength, float duration);
// 0x0049E8E4: fills a text from a script variable.
void TextFieldApplyVar(TextField* text, ape::ApeAnimScriptVar* var);
// 0x002D98DC on the game state's text display (+0x28).
void TextDisplayShowMessage(const TextField& text);
// Float +0xA0C of the game-state object +0x50 (if present). NOTE: a limit, meaning unknown.
void GameStateSetLimit(float limit);
// The world's nine player character slots (+0x1280F0).
constexpr int kPlayerCharacterSlots = 9;
WorldObject* PlayerCharacterSlot(int index);
// Character +0x490 time, +0x494, +0x495, then 0x001231B8(character, 1).
void CharacterSetTimer(WorldObject* character, float time, bool forceDisplay, bool flag);

// Entity component by kind (0x000D1288), null if none.
void* EntityGetComponent(WorldObject* entity, int kind);
// Spline follower components (kind 0x22): 0x00282408 attaches a spline (0
// detaches); +0x54 speed; +0x98/+0x9C axis and part, bit 21 of +0x94
// "DoTarget"; +0x84 value and +0x80 rate after detaching.
void SplineFollowerSetSpline(void* follower, uint32_t spline);
void SplineFollowerSetSpeed(void* follower, float speed);
void SplineFollowerSetAxis(void* follower, uint32_t axis, uint32_t part, bool doTarget);
void SplineFollowerSetFreeMotion(void* follower, float value, float rate);
// Sets or clears bits of the 64-bit entity flags +0xB8.
void EntitySetFlags64(WorldObject* entity, uint64_t mask, bool set);
// Entity virtual slots 44 and 54 (one bool each). NOTE: on/off switches,
// exact meaning unknown.
void EntityVirtualSwitch44(WorldObject* entity, bool value);
void EntityVirtualSwitch54(WorldObject* entity, bool value);
// Entity behaviour object (+0xF4), may be null.
void* EntityBrain(WorldObject* entity);
// 0x0005BA08 on the MsDecibel boss behaviour: two point instances by name
// projected onto its spline. NOTE: part of the level entity classes, not ported.
void MsDecibelSetRange(void* brain, uint32_t point0, uint32_t point1);

// Health of killable entities: +0x158 maximum, +0x15C current; virtual
// slot 79 heals by an amount; +0x178 status bits.
float EntityMaxHealth(WorldObject* entity);
void EntitySetHealth(WorldObject* entity, float health);
void EntityHeal(WorldObject* entity, float amount);
void EntityClearStatus(WorldObject* entity, uint32_t mask);
// Game world 0x0048C82C (value) and 0x0048C840 (reset). NOTE: meaning unknown.
void GameWorldSetValue48C82C(float value);
void GameWorldResetValue48C840();
// 0x0035B908 on the global 0x00F64D48 and bit 20 of the result's +0x3C0.
// NOTE: probably a HUD object; meaning unknown.
void* FindObject35B908();
void Object35B908SetFlag(void* object, bool value);
// Interaction component of an entity (0x000D93FC), bit 12 of its +0x28 enables it.
void* EntityInteraction(WorldObject* entity);
void InteractionSetEnabled(void* interaction, bool enabled);
// 0x007348D8: sets a state flag of a killable entity (flag, value, extra).
void EntitySetStateFlag(WorldObject* entity, uint32_t flag, bool value, bool extra);
// World objects of kind 12 (point instances).
WorldObject* PointInstanceFromObject(ape::ApeRefCounted* object);
// Player leash (class flag 0x1000 on +0xC8): 0x00244900 sets the point
// (squared radius, duration, point position +0x80), 0x002448C0 enables it.
constexpr uint32_t kEntityClass2_Leashable = 0x1000;
void PlayerSetLeash(WorldObject* player, float radiusSquared, float duration, WorldObject* point);
void PlayerSetLeashEnabled(WorldObject* player, bool enabled);
// Light group mask of render instances (+0x124; setter 0x00580AF0).
uint32_t RenderInstanceLightGroupMask(const ape::ApeRefCounted* object);
void RenderInstanceSetLightGroupMask(ape::ApeRefCounted* object, uint32_t mask);
// 0x002AB5F8 (static on the binocucom class). NOTE: a second power value.
void BinocucomSetPower2(float power);

// 0x00449E48: starts a music track (two flags, meanings unknown).
void MusicPlay(audio::SoundName* music, bool flag18, bool flag19);
// 0x0044A8A8: sets an interactive music register (value, blend time).
void MusicSetRegister(int registerIndex, float value, float time);
// Sets or clears bits of the killable entity status word +0x178.
void EntitySetStatusBits(WorldObject* entity, uint32_t mask, bool set);
// Outline highlight: entity virtual slot 30 (thickness, colour, true);
// plain world objects 0x000D6F90.
void EntitySetOutline(WorldObject* entity, const ape::xml::Color8& color, float thickness);
void ObjectSetOutline(ape::ApeRefCounted* object, const ape::xml::Color8& color, float thickness);
// 0x00488118 on the game world: gives player `playerIndex` the character
// definition `entityDef`. NOTE: `flag` meaning unknown.
void GameWorldSetPlayerEntity(int playerIndex, uint32_t entityDef, bool flag);
// Third class flag word of entities (+0xC4).
bool EntityHasClassFlag3(const WorldObject* entity, uint32_t mask);
// Mountable entities: 0x001E0F4C attaches a player, 0x001E4688 detaches.
void MountableAttachPlayer(WorldObject* mount, WorldObject* player);
void MountableDetachPlayer(WorldObject* mount, WorldObject* player);

// Player profile abilities: index by name (0x00465E10, -1 if unknown),
// blocked (0x00465EA0) and enabled (0x00465D88) on the player block +0x1070.
int ProfileFindAbility(uint32_t abilityName);
void ProfileSetAbilityBlocked(int playerIndex, int ability, bool blocked);
void ProfileSetAbilityEnabled(int playerIndex, int ability, bool enabled);
// 0x001B7974 on lockable entities (flag, true).
void LockableSetLocked(WorldObject* entity, bool flag, bool value);
// Targeting gadget durations (0x0029F6E8 active, 0x0029F724 refresh).
void GadgetSetActiveDuration(void* gadget, float duration);
void GadgetSetRefreshDuration(void* gadget, float duration);
// Physics entities: bit 13 of the halfword +0x18A ("no gravity");
// 0x001BD540 and 0x001BD450 (flag). NOTE: meanings unknown.
void EntitySetNoGravity(WorldObject* entity, bool noGravity);
void PhysicsEntityReset1BD540(WorldObject* entity);
void PhysicsEntitySet1BD450(WorldObject* entity, bool flag);
// Render instances: bit 7 of +0x142 ("shown") and virtual slot 9 to apply it.
bool RenderInstanceShown(const ape::ApeRefCounted* object);
void RenderInstanceSetShown(ape::ApeRefCounted* object, bool shown);
void RenderInstanceRefresh(ape::ApeRefCounted* object);
// Skin component of an entity (0x000D94F4) and its part scripts (0x002973B8).
void* EntitySkin(WorldObject* entity);
void SkinSetScript(void* skin, uint32_t script, int partIndex);

// Console global 0x0108562C. NOTE: meaning unknown.
void SetGlobalValue0108562C(float value);
// Entity time scale (0x000DAB90 with the entity tag).
void EntitySetTimeScale(WorldObject* entity, float scale);
// Game world time channels (+0x128588 + index * 12: scale, duration, flag).
void GameWorldSetTimeScale(int channel, float scale, float duration, bool flag);
// Render instance material by name hash (0x004A5A48); its texture animation
// data (+0x80) with U (+0x08) and V (+0x38) scroll speeds.
void* RenderInstanceFindMaterial(ape::ApeRefCounted* object, uint32_t materialHash);
void* MaterialAnimData(void* material);
void MaterialAnimSetUSpeed(void* anim, float speed);
void MaterialAnimSetVSpeed(void* anim, float speed);
// 0x000E13D0: runs a script on a killable entity.
void EntityRunScript(WorldObject* entity, uint32_t script);
// Bit 28 of +0x4F4 of the component 0x000D94F4 returns. NOTE: "shooting".
void WeaponSetShooting(void* weapon, bool shooting);
// 0x00489A60 on the game world: shows a prompt for the player.
void GameWorldShowPrompt(WorldObject* player, const char* message, const char* buttons);
// 0x00482B20 on the game world. NOTE: meaning unknown.
void GameWorldCall482B20(bool force);

// 0x00363A3C on a HUD object: shows a text (null keeps the current one) for
// `displayTime` seconds. NOTE: flag meanings partly unknown; the console also
// passes the default colour 0x01164CA4.
void HudShowText(void* hud, const char* text, float displayTime, bool forceStart, bool flag);
// 0x0036742C: stops the HUD object with that name.
void HudStopByName(uint32_t hudName, bool flag, bool forceStop);
// Audio stops: one tracked voice (0x0044D1E8), a category (0x0044D168) or
// everything (0x00449950).
void AudioStopTrackedVoice(uint32_t exclusivityGroup);
void AudioStopCategory(uint32_t category, bool flag);
void AudioStopAll();

// Script variables of type 13 hold a playing voice (+0x14).
constexpr int kAnimScriptVarType_Sound = 13;
// 0x00443424 on the variable's voice, then clears it.
void SoundVarStop(ape::ApeAnimScriptVar* var, bool flag);
// Sound registers: 0x0044A78C (register 1-4 of a voice) and 0x0044CEB0
// (global register 1-64); the music voice is the console global 0x010AFEA4.
void AudioSetVoiceRegister(uint32_t voice, int32_t reg, int8_t value);
void AudioSetGlobalRegister(int32_t reg, int8_t value);
uint32_t AudioMusicVoice();
// 0x00449E10: stops the music. NOTE: flag meaning unknown (probably fade).
void MusicStop(bool flag);
// Component swaps: binocucom (characters, 0x0012E534), inventory
// (0x000D99D8), marker (0x000D9C88), receptacle (0x000D9B30).
void CharacterSwapBinocucom(WorldObject* character, uint32_t componentDef);
void EntitySwapInventory(WorldObject* entity, uint32_t componentDef);
void EntitySwapMarker(WorldObject* entity, uint32_t componentDef);
void EntitySwapReceptacle(WorldObject* entity, uint32_t componentDef);

// Game state (0x00F64C20): 0x002E26A8 (unlock disguise) and 0x002E2874
// (unlock a movie by name).
void GameStateUnlockDisguise(bool value);
void GameStateUnlockMovie(uint32_t movieName);
// World objects of kind 10: flag byte +0x45 of +0xD0 -> +0x70; hierarchy
// links +0x110 (first child) and +0x10C (next sibling).
void ObjectSetFlag45(ape::ApeRefCounted* object, uint8_t value);
ape::ApeRefCounted* ObjectFirstChild(ape::ApeRefCounted* object);
ape::ApeRefCounted* ObjectNextSibling(ape::ApeRefCounted* object);

// ---------------------------------------------------------------------------
// Cut-scene sequence commands (sly/gameplay sequencer): each call builds the
// sequence command named and appends it to the running sequence (0x0042C138).

// Camera shake parameters of SeqCameraShake (0x60 bytes, copied by the
// sequence command 0x0042D72C).
struct SequenceShakeParams {
    float duration = 1.0f;          // +0x00
    float magnitude = 1.0f;         // +0x04
    float startMagnitude = 0.0f;    // +0x08
    uint32_t falloffMode = 0x85D98325u;  // +0x0C "Linear"
    float falloffStartTime = 0.0f;  // +0x10
    float pitch = 1.0f;             // +0x14
    float yaw = 0.0f;               // +0x18
    float roll = 1.0f;              // +0x1C
    float pitchFrequency = 4.0f;    // +0x20
    float yawFrequency = 6.5f;      // +0x24
    float rollFrequency = 2.5f;     // +0x28
    float radius = 0.0f;            // +0x30
    ape::Vec3 vector40 = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x40 NOTE: never parsed
    float value50 = -1.0f;          // +0x50 NOTE: never parsed
};
// SeqMixStateCommand (0x0042CC10).
void SequenceAddMixState(float crossfade, bool activate, const audio::MixState* mixState);
// SeqPostMessageCommand (0x0042D13C).
void SequenceAddPostEvent(uint32_t group, uint32_t event, uint32_t data);
// SeqCameraReleaseCommand (0x0042D464).
void SequenceAddCameraRelease(float time, ape::ApeAnimScriptInstance* instance, bool wait);
// SeqCameraShakeCommand (0x0042D72C).
void SequenceAddCameraShake(uint32_t entity, const SequenceShakeParams& params, bool wait);
// SeqCharacterAction4Command (0x0042D8BC): moves a character to a point.
void SequenceAddMoveToPoint(uint32_t entity, WorldObject* point, bool wait);
// SeqAttachCommand (0x0042DDDC).
void SequenceAddAttach(uint32_t parentEntity, uint32_t childEntity);

// SeqEnableComponentCommand (0x0042DED0).
void SequenceAddEnableComponent(uint32_t entity, uint32_t component, bool enable);
// SeqEnablePhysicsCommand (0x0042E20C).
void SequenceAddEnablePhysics(uint32_t entity, bool enable, bool flag);
// SeqPlayerControlCommand (0x0042E3AC).
void SequenceAddPlayerControl(bool enable, bool reset);
// Spawner switches (0x0042E618 for one spawner, 0x0042E868 for a group).
void SequenceAddEnableSpawner(uint32_t spawner, bool enable, bool disable, bool flag12, bool flag13,
                              bool reset, bool flag15);
void SequenceAddEnableSpawnerGroup(uint32_t group, bool enable, bool disable, bool flag12, bool reset,
                                   bool flag14);
// SeqEndSequenceCommand (0x0042E9F0) and the immediate sequence end (0x0042BC24).
void SequenceAddEndSequence();
void SequenceEnd();
// SeqPlayAnimationCommand (0x0042EA6C).
void SequenceAddPlayAnimation(uint32_t entity, uint32_t state, bool flag14, bool flag15);
// SeqSuppressAudioCommand (0x0042EBC0).
void SequenceAddSuppressAudio(uint32_t entity, bool suppress);
// SeqEnqueueStreamCommand (0x0042ECB8).
void SequenceAddEnqueueStream(audio::SoundName* sound, uint32_t exclusivityGroup, int32_t priority,
                              bool flag20, bool wait, bool flag22);
// SeqColorFilterCommand (0x0042F144).
void SequenceAddColorFilter(float start, float end, float duration, int32_t value, bool flag, bool wait);
// SeqDepthOfFieldCommand (0x0042F264).
void SequenceAddDepthOfField(float value0, float value1, float duration, float value3, bool flag, bool wait);

// SeqFocusPullCommand (0x0042F3BC): seven focus values (the last is the duration).
void SequenceAddFocusPull(uint32_t target, const float values[7], int32_t iterations, bool flag30, bool wait, bool flag32);
// SeqSkinAttachCommand (0x0042F588). NOTE: a shooter firing at a target.
void SequenceAddShoot(uint32_t shooter, uint32_t target, uint32_t hash, int32_t index, bool flag);
// SeqAmbientTintCommand (0x0042FA20). NOTE: blends the camera weight.
void SequenceAddCameraWeight(float weight, float duration, float exponent);
// SeqCharacterAction3Command (0x0042FACC): jump to a point.
void SequenceAddJumpTo(uint32_t entity, WorldObject* point, uint32_t anim, bool flag, bool wait);
// SeqRemoveEntityCommand (0x0042FDC8).
void SequenceAddRemoveEntity(uint32_t entity, bool flag);
// Sequence look-at command (0x0042FF28).
void SequenceAddLookAt(float duration, float value, float fov, uint32_t entity, uint32_t target, uint32_t entity2, uint32_t target2);
// Level spline by name (sorted map at 0x00F8F158), null if none.
void* FindLevelSpline(uint32_t splineName);
// SeqCharacterAction2Command (0x004306F4): follow a spline.
void SequenceAddMoveAlong(uint32_t entity, void* spline, uint32_t anim, float startOffset, float speed, bool forward, bool sprint, bool wait);
// Sequence camera move (0x00430A64).
void SequenceAddMoveCamera(float fov, float value18, float value1C, float moveTime, float value28, uint32_t startPoint, uint32_t endPoint);

// Sequence switches with one flag: SeqHudCounterCommand (0x00432134),
// SeqHideHudCommand (0x00431FA8, used by "PauseEntities"), SeqShowHudCommand
// (0x0043202C), SeqLockInputCommand (0x004320B0, used by "PauseHavok") and
// SeqHudElementCommand (0x00433424). NOTE: the class names come from the
// sequencer port and may not match the script names.
void SequenceAddHudCounter(bool pause);
void SequenceAddPauseEntities(bool pause);
void SequenceAddShowHud(bool pause);
void SequenceAddPauseHavok(bool pause);
void SequenceAddHudElement(bool wait);
// SeqMessageBoxCommand (0x00433938).
void SequenceAddMessageBox(const char* name, const float scale[2], const float offset[2], bool wait);

// SeqExposureRampCommand (0x004340D8): screen colour ramp.
void SequenceAddScreenTint(float duration, const ape::xml::Color8& start, const ape::xml::Color8& end, ape::ApeAnimScriptInstance* instance, bool flag, bool wait);
// SeqWaitForButtonCommand (0x00434A10).
void SequenceAddWaitForButton(float waitTime, int32_t playerNum, uint32_t button, uint32_t hash, uint32_t failEvent);
// SeqTransitionPromptCommand (0x00434CA8).
void SequenceAddTransitionPrompt(int32_t playerNum);
// Sequence arc QTE (0x00434E0C).
void SequenceAddArcQte(float value, float arcAngle, float lookTime, int32_t playerNum);
// Sequence Paris chase QTE (0x004360CC).
void SequenceAddParisChaseQte(uint32_t hash, bool flag10, bool flag11);
// 0x0043554C (100-byte sequence command, not ported in sly/gameplay).
void SequenceAddActorQte(int32_t player, uint32_t button, float value28, float value2C, float value30,
                         float value34);
// SeqEnableScriptedEntitiesCommand (0x00437C0C).
void SequenceAddEnableScriptedEntities(bool enabled);
// SeqMinigameControlCommand (0x00437D58): six switches.
void SequenceAddMinigameControl(const bool flags[6]);
// Sequence HUD type switch (0x00437F24).
void SequenceAddHudType(uint32_t type, bool visible, bool force, uint32_t left, uint32_t right, bool wait, bool flag);

// SeqHudTimerCommand (0x00438498): sequence time limit.
void SequenceAddTimeLimit(float limit);
// SeqCutsceneFlagCommand (0x00438508): skippable flag.
void SequenceAddCutsceneFlag(bool skippable);
// SeqSetEntityUpdateCommand (0x00438F3C).
void SequenceAddSetEntityUpdate(uint32_t entity, bool update);
// SeqSetEntityVisibleCommand (0x00438D14).
void SequenceAddSetEntityVisible(uint32_t entity, uint32_t child, bool visible, bool recursive, bool flag16, bool processComponents, bool flag18);
// SeqEmitterParamCommand (0x004388A0).
void SequenceAddEmitterParam(uint32_t entity, int32_t value);
// SeqLetterboxCommand (0x004389D4).
void SequenceAddLetterbox(bool visible, bool wait);
// Sequence music command (0x00438B84).
void SequenceAddSetMusic(audio::SoundName* music, bool flag18, bool flag19);
// SeqPauseAiCommand (0x00438630) and SeqPausePlayerCommand (0x0043859C).
void SequenceAddPauseAi(bool allowed);
void SequenceAddPausePlayer(bool allowed);

// SeqStartAnimationCommand (0x00439C28).
void SequenceAddStartAnimation(uint32_t entity, uint32_t state, bool recursive);
// SeqTimeScaleCommand (0x00439D48). NOTE: sets a sound group volume (channel 1 = music).
void SequenceAddGroupVolume(float volume, uint32_t channel);
// SeqLevelHeadlineCommand (0x00439DC4).
void SequenceAddLevelHeadline();
// SeqStopSoundCommand (0x0043A6DC).
void SequenceAddStopSound(uint32_t soundGroup, uint32_t exclusivityGroup, bool flag);
// SeqStopMusicCommand (0x0043A7BC).
void SequenceAddStopMusic(bool flag);
// SeqSwapAnimSetCommand (0x0043A84C).
void SequenceAddSwapAnimSet(uint32_t entity, uint32_t def, bool flag14, bool flag15, bool wait);
// SeqWaitForSoundCommand (0x0043AD8C).
void SequenceAddWaitForSound(uint32_t group, uint32_t exclusivityGroup);
// SeqScreenFadeCommand (0x0042E040). NOTE: the dark screen effect.
void SequenceAddScreenFade(float distance, float ramp, float minValue, float fadeTime, bool enable);

} // namespace levels
} // namespace sly
