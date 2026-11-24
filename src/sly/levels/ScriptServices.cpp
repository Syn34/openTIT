// openTIT - TODO stubs for the game services used by the script commands #83-#269.
// Each returns what the original returns for a missing object.
#include "sly/levels/ScriptServices.h"

#include "ape/xml/ApeXml.h"
#include "sly/game/Upgrades.h"

namespace sly {
namespace levels {

WorldObject* PlayerEntityByIndex(int index) { (void)index; return nullptr; }
void PlayerEnableAllButtons(WorldObject* player, bool enable) { (void)player; (void)enable; }
void PlayerEnableButton(WorldObject* player, uint32_t button, bool enable) { (void)player; (void)button; (void)enable; }
void PlayerSetControlEnabled(WorldObject* player, bool enable) { (void)player; (void)enable; }
void EntityVirtualCommand(WorldObject* entity, int a, int b) { (void)entity; (void)a; (void)b; }

void GameWorldResumeObject12847C() {}
void GameStateSetPauseFlag(bool value) { (void)value; }
void GameWorldShowOverlay(const char* name, const float scale[2], const float offset[2])
{
    (void)name; (void)scale; (void)offset;
}

void ScreenColorFade(const ape::xml::Color8& start, const ape::xml::Color8& end, float duration, bool flag)
{
    (void)start; (void)end; (void)duration; (void)flag;
}
void GameWorldQueueCutscene(uint32_t script, uint32_t library, bool flag,
                            const std::vector<TextField>& texts, int32_t spawnPoint,
                            ape::ApeAnimScriptInstance* instance)
{
    (void)script; (void)library; (void)flag; (void)texts; (void)spawnPoint; (void)instance;
}
void EntityShowMessages(WorldObject* entity, const std::vector<TextField>& texts,
                        ape::ApeAnimScriptInstance* instance)
{
    (void)entity; (void)texts; (void)instance;
}
void EntityRegisterListener(WorldObject* entity, uint32_t group, bool flag) { (void)entity; (void)group; (void)flag; }
void* EntityTargetingGadget(WorldObject* entity) { (void)entity; return nullptr; }
void GadgetSetTarget(void* gadget, WorldObject* target) { (void)gadget; (void)target; }

bool EntityHasClassFlag2(const WorldObject* entity, uint32_t mask) { (void)entity; (void)mask; return false; }
void CharacterReset1F4460(WorldObject* character) { (void)character; }
void CharacterReset1ED9D4(WorldObject* character) { (void)character; }
void SetGameWorldFlag2D(uint8_t mask, bool value) { (void)mask; (void)value; }
void BinocucomSetMode(void* binocucom, uint32_t mode) { (void)binocucom; (void)mode; }

void BinocucomSetLocked(void* binocucom, bool locked) { (void)binocucom; (void)locked; }
void BinocucomSetPower(float power) { (void)power; }
void BinocucomSetViewType(void* binocucom, int type) { (void)binocucom; (void)type; }
void BinocucomSetScript(uint32_t scriptName) { (void)scriptName; }
void SetGlobalRate010592F4(float rate) { (void)rate; }
bool ObjectIsRenderInstance(const ape::ApeRefCounted* object) { (void)object; return false; }
void RenderInstanceSetAlpha(ape::ApeRefCounted* object, uint8_t alpha, bool flag) { (void)object; (void)alpha; (void)flag; }
int CharacterFindAbility(uint32_t abilityName) { (void)abilityName; return 0; }
void CharacterSetAbilityBlocked(WorldObject* character, int ability, bool blocked) { (void)character; (void)ability; (void)blocked; }
void CharacterSetAbilityEnabled(WorldObject* character, int ability, bool enabled) { (void)character; (void)ability; (void)enabled; }
void CharacterRestoreDefaultController(WorldObject* character) { (void)character; }
void CharacterResetController(WorldObject* character) { (void)character; }
void CharacterEnableController(WorldObject* character, bool enable) { (void)character; (void)enable; }

void* EntityFindComponent(WorldObject* entity, uint32_t componentId) { (void)entity; (void)componentId; return nullptr; }
int ComponentKind(const void* component) { (void)component; return 0; }
void ScriptComponentSetScript(void* component, uint32_t script) { (void)component; (void)script; }
void ComponentSetVisible(void* component, bool visible, bool flag) { (void)component; (void)visible; (void)flag; }
const char* EntityDefNameString(WorldObject* entity) { (void)entity; return nullptr; }
const char* EntityNameString(WorldObject* entity) { (void)entity; return nullptr; }
void ControllerRumble(int controllerIndex, float strength, float duration) { (void)controllerIndex; (void)strength; (void)duration; }
void TextFieldApplyVar(TextField* text, ape::ApeAnimScriptVar* var) { (void)text; (void)var; }
void TextDisplayShowMessage(const TextField& text) { (void)text; }
void GameStateSetLimit(float limit) { (void)limit; }
WorldObject* PlayerCharacterSlot(int index) { (void)index; return nullptr; }
void CharacterSetTimer(WorldObject* character, float time, bool forceDisplay, bool flag) { (void)character; (void)time; (void)forceDisplay; (void)flag; }

void* EntityGetComponent(WorldObject* entity, int kind) { (void)entity; (void)kind; return nullptr; }
void SplineFollowerSetSpline(void* follower, uint32_t spline) { (void)follower; (void)spline; }
void SplineFollowerSetSpeed(void* follower, float speed) { (void)follower; (void)speed; }
void SplineFollowerSetAxis(void* follower, uint32_t axis, uint32_t part, bool doTarget) { (void)follower; (void)axis; (void)part; (void)doTarget; }
void SplineFollowerSetFreeMotion(void* follower, float value, float rate) { (void)follower; (void)value; (void)rate; }
void EntitySetFlags64(WorldObject* entity, uint64_t mask, bool set) { (void)entity; (void)mask; (void)set; }
void EntityVirtualSwitch44(WorldObject* entity, bool value) { (void)entity; (void)value; }
void EntityVirtualSwitch54(WorldObject* entity, bool value) { (void)entity; (void)value; }
void* EntityBrain(WorldObject* entity) { (void)entity; return nullptr; }
void MsDecibelSetRange(void* brain, uint32_t point0, uint32_t point1) { (void)brain; (void)point0; (void)point1; }

float EntityMaxHealth(WorldObject* entity) { (void)entity; return 0.0f; }
void EntitySetHealth(WorldObject* entity, float health) { (void)entity; (void)health; }
void EntityHeal(WorldObject* entity, float amount) { (void)entity; (void)amount; }
void EntityClearStatus(WorldObject* entity, uint32_t mask) { (void)entity; (void)mask; }
void GameWorldSetValue48C82C(float value) { (void)value; }
void GameWorldResetValue48C840() {}
void* FindObject35B908() { return nullptr; }
void Object35B908SetFlag(void* object, bool value) { (void)object; (void)value; }
void* EntityInteraction(WorldObject* entity) { (void)entity; return nullptr; }
void InteractionSetEnabled(void* interaction, bool enabled) { (void)interaction; (void)enabled; }
void EntitySetStateFlag(WorldObject* entity, uint32_t flag, bool value, bool extra) { (void)entity; (void)flag; (void)value; (void)extra; }
WorldObject* PointInstanceFromObject(ape::ApeRefCounted* object) { (void)object; return nullptr; }
void PlayerSetLeash(WorldObject* player, float radiusSquared, float duration, WorldObject* point) { (void)player; (void)radiusSquared; (void)duration; (void)point; }
void PlayerSetLeashEnabled(WorldObject* player, bool enabled) { (void)player; (void)enabled; }
uint32_t RenderInstanceLightGroupMask(const ape::ApeRefCounted* object) { (void)object; return 0; }
void RenderInstanceSetLightGroupMask(ape::ApeRefCounted* object, uint32_t mask) { (void)object; (void)mask; }
void BinocucomSetPower2(float power) { (void)power; }

void MusicPlay(audio::SoundName* music, bool flag18, bool flag19) { (void)music; (void)flag18; (void)flag19; }
void MusicSetRegister(int registerIndex, float value, float time) { (void)registerIndex; (void)value; (void)time; }
void EntitySetStatusBits(WorldObject* entity, uint32_t mask, bool set) { (void)entity; (void)mask; (void)set; }
void EntitySetOutline(WorldObject* entity, const ape::xml::Color8& color, float thickness) { (void)entity; (void)color; (void)thickness; }
void ObjectSetOutline(ape::ApeRefCounted* object, const ape::xml::Color8& color, float thickness) { (void)object; (void)color; (void)thickness; }
void GameWorldSetPlayerEntity(int playerIndex, uint32_t entityDef, bool flag) { (void)playerIndex; (void)entityDef; (void)flag; }
bool EntityHasClassFlag3(const WorldObject* entity, uint32_t mask) { (void)entity; (void)mask; return false; }
void MountableAttachPlayer(WorldObject* mount, WorldObject* player) { (void)mount; (void)player; }
void MountableDetachPlayer(WorldObject* mount, WorldObject* player) { (void)mount; (void)player; }

int ProfileFindAbility(uint32_t abilityName) { (void)abilityName; return 0; }
void ProfileSetAbilityBlocked(int playerIndex, int ability, bool blocked) { (void)playerIndex; (void)ability; (void)blocked; }
void ProfileSetAbilityEnabled(int playerIndex, int ability, bool enabled) { (void)playerIndex; (void)ability; (void)enabled; }
void LockableSetLocked(WorldObject* entity, bool flag, bool value) { (void)entity; (void)flag; (void)value; }
void GadgetSetActiveDuration(void* gadget, float duration) { (void)gadget; (void)duration; }
void GadgetSetRefreshDuration(void* gadget, float duration) { (void)gadget; (void)duration; }
void EntitySetNoGravity(WorldObject* entity, bool noGravity) { (void)entity; (void)noGravity; }
void PhysicsEntityReset1BD540(WorldObject* entity) { (void)entity; }
void PhysicsEntitySet1BD450(WorldObject* entity, bool flag) { (void)entity; (void)flag; }
bool RenderInstanceShown(const ape::ApeRefCounted* object) { (void)object; return false; }
void RenderInstanceSetShown(ape::ApeRefCounted* object, bool shown) { (void)object; (void)shown; }
void RenderInstanceRefresh(ape::ApeRefCounted* object) { (void)object; }
void* EntitySkin(WorldObject* entity) { (void)entity; return nullptr; }
void SkinSetScript(void* skin, uint32_t script, int partIndex) { (void)skin; (void)script; (void)partIndex; }

void SetGlobalValue0108562C(float value) { (void)value; }
void EntitySetTimeScale(WorldObject* entity, float scale) { (void)entity; (void)scale; }
void GameWorldSetTimeScale(int channel, float scale, float duration, bool flag) { (void)channel; (void)scale; (void)duration; (void)flag; }
void* RenderInstanceFindMaterial(ape::ApeRefCounted* object, uint32_t materialHash) { (void)object; (void)materialHash; return nullptr; }
void* MaterialAnimData(void* material) { (void)material; return nullptr; }
void MaterialAnimSetUSpeed(void* anim, float speed) { (void)anim; (void)speed; }
void MaterialAnimSetVSpeed(void* anim, float speed) { (void)anim; (void)speed; }
void EntityRunScript(WorldObject* entity, uint32_t script) { (void)entity; (void)script; }
void WeaponSetShooting(void* weapon, bool shooting) { (void)weapon; (void)shooting; }
void GameWorldShowPrompt(WorldObject* player, const char* message, const char* buttons) { (void)player; (void)message; (void)buttons; }
void GameWorldCall482B20(bool force) { (void)force; }

void HudShowText(void* hud, const char* text, float displayTime, bool forceStart, bool flag) { (void)hud; (void)text; (void)displayTime; (void)forceStart; (void)flag; }
void HudStopByName(uint32_t hudName, bool flag, bool forceStop) { (void)hudName; (void)flag; (void)forceStop; }
void AudioStopTrackedVoice(uint32_t exclusivityGroup) { (void)exclusivityGroup; }
void AudioStopCategory(uint32_t category, bool flag) { (void)category; (void)flag; }
void AudioStopAll() {}

void SoundVarStop(ape::ApeAnimScriptVar* var, bool flag) { (void)var; (void)flag; }
// TODO(0x0044A78C / 0x0044CEB0): sound registers are not modelled by the audio port.
void AudioSetVoiceRegister(uint32_t voice, int32_t reg, int8_t value) { (void)voice; (void)reg; (void)value; }
void AudioSetGlobalRegister(int32_t reg, int8_t value) { (void)reg; (void)value; }
// TODO(0x010AFEA4): the audio port keeps the music voice private.
uint32_t AudioMusicVoice() { return 0; }
void MusicStop(bool flag) { (void)flag; }
void CharacterSwapBinocucom(WorldObject* character, uint32_t componentDef) { (void)character; (void)componentDef; }
void EntitySwapInventory(WorldObject* entity, uint32_t componentDef) { (void)entity; (void)componentDef; }
void EntitySwapMarker(WorldObject* entity, uint32_t componentDef) { (void)entity; (void)componentDef; }
void EntitySwapReceptacle(WorldObject* entity, uint32_t componentDef) { (void)entity; (void)componentDef; }

void GameStateUnlockDisguise(bool value) { (void)value; }
void GameStateUnlockMovie(uint32_t movieName) { (void)movieName; }
void ObjectSetFlag45(ape::ApeRefCounted* object, uint8_t value) { (void)object; (void)value; }
ape::ApeRefCounted* ObjectFirstChild(ape::ApeRefCounted* object) { (void)object; return nullptr; }
ape::ApeRefCounted* ObjectNextSibling(ape::ApeRefCounted* object) { (void)object; return nullptr; }

void SequenceAddMixState(float crossfade, bool activate, const audio::MixState* mixState) { (void)crossfade; (void)activate; (void)mixState; }
void SequenceAddPostEvent(uint32_t group, uint32_t event, uint32_t data) { (void)group; (void)event; (void)data; }
void SequenceAddCameraRelease(float time, ape::ApeAnimScriptInstance* instance, bool wait) { (void)time; (void)instance; (void)wait; }
void SequenceAddCameraShake(uint32_t entity, const SequenceShakeParams& params, bool wait) { (void)entity; (void)params; (void)wait; }
void SequenceAddMoveToPoint(uint32_t entity, WorldObject* point, bool wait) { (void)entity; (void)point; (void)wait; }
void SequenceAddAttach(uint32_t parentEntity, uint32_t childEntity) { (void)parentEntity; (void)childEntity; }

void SequenceAddEnableComponent(uint32_t entity, uint32_t component, bool enable) { (void)entity; (void)component; (void)enable; }
void SequenceAddEnablePhysics(uint32_t entity, bool enable, bool flag) { (void)entity; (void)enable; (void)flag; }
void SequenceAddPlayerControl(bool enable, bool reset) { (void)enable; (void)reset; }
void SequenceAddEndSequence() {}
void SequenceEnd() {}
void SequenceAddPlayAnimation(uint32_t entity, uint32_t state, bool flag14, bool flag15) { (void)entity; (void)state; (void)flag14; (void)flag15; }
void SequenceAddSuppressAudio(uint32_t entity, bool suppress) { (void)entity; (void)suppress; }
void SequenceAddColorFilter(float start, float end, float duration, int32_t value, bool flag, bool wait) { (void)start; (void)end; (void)duration; (void)value; (void)flag; (void)wait; }
void SequenceAddDepthOfField(float value0, float value1, float duration, float value3, bool flag, bool wait) { (void)value0; (void)value1; (void)duration; (void)value3; (void)flag; (void)wait; }

void SequenceAddEnableSpawner(uint32_t spawner, bool enable, bool disable, bool flag12, bool flag13,
                              bool reset, bool flag15)
{
    (void)spawner; (void)enable; (void)disable; (void)flag12; (void)flag13; (void)reset; (void)flag15;
}
void SequenceAddEnableSpawnerGroup(uint32_t group, bool enable, bool disable, bool flag12, bool reset,
                                   bool flag14)
{
    (void)group; (void)enable; (void)disable; (void)flag12; (void)reset; (void)flag14;
}
void SequenceAddEnqueueStream(audio::SoundName* sound, uint32_t exclusivityGroup, int32_t priority,
                              bool flag20, bool wait, bool flag22)
{
    (void)sound; (void)exclusivityGroup; (void)priority; (void)flag20; (void)wait; (void)flag22;
}

void SequenceAddFocusPull(uint32_t target, const float values[7], int32_t iterations, bool flag30, bool wait, bool flag32) { (void)target; (void)values; (void)iterations; (void)flag30; (void)wait; (void)flag32; }
void SequenceAddShoot(uint32_t shooter, uint32_t target, uint32_t hash, int32_t index, bool flag) { (void)shooter; (void)target; (void)hash; (void)index; (void)flag; }
void SequenceAddCameraWeight(float weight, float duration, float exponent) { (void)weight; (void)duration; (void)exponent; }
void SequenceAddJumpTo(uint32_t entity, WorldObject* point, uint32_t anim, bool flag, bool wait) { (void)entity; (void)point; (void)anim; (void)flag; (void)wait; }
void SequenceAddRemoveEntity(uint32_t entity, bool flag) { (void)entity; (void)flag; }
void SequenceAddLookAt(float duration, float value, float fov, uint32_t entity, uint32_t target, uint32_t entity2, uint32_t target2) { (void)duration; (void)value; (void)fov; (void)entity; (void)target; (void)entity2; (void)target2; }
void* FindLevelSpline(uint32_t splineName) { (void)splineName; return nullptr; }
void SequenceAddMoveAlong(uint32_t entity, void* spline, uint32_t anim, float startOffset, float speed, bool forward, bool sprint, bool wait) { (void)entity; (void)spline; (void)anim; (void)startOffset; (void)speed; (void)forward; (void)sprint; (void)wait; }
void SequenceAddMoveCamera(float fov, float value18, float value1C, float moveTime, float value28, uint32_t startPoint, uint32_t endPoint) { (void)fov; (void)value18; (void)value1C; (void)moveTime; (void)value28; (void)startPoint; (void)endPoint; }

void SequenceAddHudCounter(bool pause) { (void)pause; }
void SequenceAddPauseEntities(bool pause) { (void)pause; }
void SequenceAddShowHud(bool pause) { (void)pause; }
void SequenceAddPauseHavok(bool pause) { (void)pause; }
void SequenceAddHudElement(bool wait) { (void)wait; }
void SequenceAddMessageBox(const char* name, const float scale[2], const float offset[2], bool wait) { (void)name; (void)scale; (void)offset; (void)wait; }

void SequenceAddScreenTint(float duration, const ape::xml::Color8& start, const ape::xml::Color8& end, ape::ApeAnimScriptInstance* instance, bool flag, bool wait) { (void)duration; (void)start; (void)end; (void)instance; (void)flag; (void)wait; }
void SequenceAddWaitForButton(float waitTime, int32_t playerNum, uint32_t button, uint32_t hash, uint32_t failEvent) { (void)waitTime; (void)playerNum; (void)button; (void)hash; (void)failEvent; }
void SequenceAddTransitionPrompt(int32_t playerNum) { (void)playerNum; }
void SequenceAddArcQte(float value, float arcAngle, float lookTime, int32_t playerNum) { (void)value; (void)arcAngle; (void)lookTime; (void)playerNum; }
void SequenceAddParisChaseQte(uint32_t hash, bool flag10, bool flag11) { (void)hash; (void)flag10; (void)flag11; }
void SequenceAddSpawnEntity(void* def, const char* entityName, WorldObject* point, uint32_t hash60, uint32_t part,
                            const ape::Vec3& offsetPos, const ape::Vec3& offsetRot, const bool flags[6])
{
    (void)def; (void)entityName; (void)point; (void)hash60; (void)part; (void)offsetPos; (void)offsetRot; (void)flags;
}
void SequenceAddActorQte(int32_t player, uint32_t button, float value28, float value2C, float value30, float value34)
{
    (void)player; (void)button; (void)value28; (void)value2C; (void)value30; (void)value34;
}
void SequenceAddEnableScriptedEntities(bool enabled) { (void)enabled; }
void SequenceAddMinigameControl(const bool flags[6]) { (void)flags; }
void SequenceAddHudType(uint32_t type, bool visible, bool force, uint32_t left, uint32_t right, bool wait, bool flag) { (void)type; (void)visible; (void)force; (void)left; (void)right; (void)wait; (void)flag; }

void SequenceAddTimeLimit(float limit) { (void)limit; }
void SequenceAddCutsceneFlag(bool skippable) { (void)skippable; }
void SequenceAddSetEntityUpdate(uint32_t entity, bool update) { (void)entity; (void)update; }
void SequenceAddSetEntityVisible(uint32_t entity, uint32_t child, bool visible, bool recursive, bool flag16, bool processComponents, bool flag18) { (void)entity; (void)child; (void)visible; (void)recursive; (void)flag16; (void)processComponents; (void)flag18; }
void SequenceAddEmitterParam(uint32_t entity, int32_t value) { (void)entity; (void)value; }
void SequenceAddLetterbox(bool visible, bool wait) { (void)visible; (void)wait; }
void SequenceAddSetMusic(audio::SoundName* music, bool flag18, bool flag19) { (void)music; (void)flag18; (void)flag19; }
void SequenceAddPauseAi(bool allowed) { (void)allowed; }
void SequenceAddPausePlayer(bool allowed) { (void)allowed; }

void SequenceAddStartAnimation(uint32_t entity, uint32_t state, bool recursive) { (void)entity; (void)state; (void)recursive; }
void SequenceAddGroupVolume(float volume, uint32_t channel) { (void)volume; (void)channel; }
void SequenceAddLevelHeadline() {}
void SequenceAddStopSound(uint32_t soundGroup, uint32_t exclusivityGroup, bool flag) { (void)soundGroup; (void)exclusivityGroup; (void)flag; }
void SequenceAddStopMusic(bool flag) { (void)flag; }
void SequenceAddSwapAnimSet(uint32_t entity, uint32_t def, bool flag14, bool flag15, bool wait) { (void)entity; (void)def; (void)flag14; (void)flag15; (void)wait; }
void SequenceAddWaitForSound(uint32_t group, uint32_t exclusivityGroup) { (void)group; (void)exclusivityGroup; }
void SequenceAddScreenFade(float distance, float ramp, float minValue, float fadeTime, bool enable) { (void)distance; (void)ramp; (void)minValue; (void)fadeTime; (void)enable; }

} // namespace levels
} // namespace sly
