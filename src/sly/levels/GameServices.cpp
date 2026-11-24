// openTIT - TODO stubs for the game systems used by the level and script code.
// Each returns what the original returns for a missing object.
#include "sly/levels/GameServices.h"
#include "sly/levels/script/ScriptVars.h"

#include "sly/levels/PlayerNotifyFlags.h"

namespace sly {
namespace levels {

// TODO(0x000D6A28 FindEntity)
WorldObject* FindEntity(uint32_t nameHash) { (void)nameHash; return nullptr; }
uint32_t EntityName(const WorldObject* entity) { (void)entity; return 0; }
uint32_t EntityDefName(const WorldObject* entity) { (void)entity; return 0; }
bool EntityIsAlive(const WorldObject* entity) { (void)entity; return false; }
bool EntityIsCharacter(const WorldObject* entity) { return EntityHasClassFlag(entity, kEntityClass_Character); }
bool EntityHasClassFlag(const WorldObject* entity, uint32_t mask) { (void)entity; (void)mask; return false; }
ape::ApeRefCounted* EntityWorldObject(const WorldObject* entity) { (void)entity; return nullptr; }
// TODO(0x000D6AA0 EntityFromWorldObject)
WorldObject* EntityFromWorldObject(ape::ApeRefCounted* object, bool searchParents, bool searchLinks)
{
    (void)object; (void)searchParents; (void)searchLinks;
    return nullptr;
}
WorldObject* FindPointInstance(uint32_t nameHash) { (void)nameHash; return nullptr; }
void WorldObjectSetUserData(WorldObject* object, uint32_t tag, void* data) { (void)object; (void)tag; (void)data; }
uint32_t WorldObjectUserTag(const ape::ApeRefCounted* object) { (void)object; return 0; }
void* WorldObjectUserData(const ape::ApeRefCounted* object) { (void)object; return nullptr; }

void* EntityCharacter(WorldObject* entity) { (void)entity; return nullptr; }
void CharacterSetAnimLayerWeight(void* character, float weight, int32_t layer) { (void)character; (void)weight; (void)layer; }
void EntitySetAnimLayerWeight(WorldObject* entity, float weight, int32_t layer) { (void)entity; (void)weight; (void)layer; }
bool CharacterCanAttach(const WorldObject* character) { (void)character; return false; }
void CharacterAttachTo(WorldObject* character, WorldObject* target, const uint32_t* part,
                       const ape::Vec3* offset, const ape::Vec4* rotation, bool ignoreRotation)
{
    (void)character; (void)target; (void)part; (void)offset; (void)rotation; (void)ignoreRotation;
}
void EntitySetAttachMode(WorldObject* entity, int mode) { (void)entity; (void)mode; }
void CharacterBlendMotion(WorldObject* character, float value, bool flag) { (void)character; (void)value; (void)flag; }

WorldObject* PlayerEntity() { return nullptr; }
WorldObject* GameCamera() { return nullptr; }
void CameraSetEase(WorldObject* camera, float value) { (void)camera; (void)value; }
void CameraSetEaseRate(WorldObject* camera, float rate) { (void)camera; (void)rate; }
void CameraPlayAnim(WorldObject* camera, uint32_t anim, float speed, bool loop, const void* owner, bool flag)
{
    (void)camera; (void)anim; (void)speed; (void)loop; (void)owner; (void)flag;
}
void CameraStopAnim(WorldObject* camera, bool removeAnimation) { (void)camera; (void)removeAnimation; }
void* PlayerBinocucom(WorldObject* player) { (void)player; return nullptr; }
void BinocucomClose(void* binocucom) { (void)binocucom; }
WorldObject* EntityAttachParent(const WorldObject* entity) { (void)entity; return nullptr; }
void SetWorldFlag12828D(bool value) { (void)value; }
void SetGlobalSwitch010AF1BC(bool value) { (void)value; }
void SetDarkScreen(bool enable, float distance, float ramp, float minValue) { (void)enable; (void)distance; (void)ramp; (void)minValue; }
void EntityDetach(WorldObject* parent, WorldObject* child) { (void)parent; (void)child; }
void GameStateNotifyEntity(uint32_t entityName) { (void)entityName; }
void ClearGameCounter() {}
uint8_t GameWorldFlags2C() { return 0; }
uint8_t GameWorldFlags2D() { return 0; }
// TODO(0x0049EC08 TextFieldParse)
bool TextFieldParse(ape::xml::XmlElement* element, TextField* text) { (void)element; (void)text; return false; }
bool TextDisplayAvailable() { return false; }
void TextDisplayShow(const TextField& text) { (void)text; }
void TextFieldNotify(ape::ApeAnimScriptInstance* instance, const TextField& text) { (void)instance; (void)text; }
void AwardTrophyByName(uint32_t awardName) { (void)awardName; }
void* FindHudObject(uint32_t nameHash) { (void)nameHash; return nullptr; }
void HudPlayAnim(void* hud, int mode, uint32_t anim, bool loop) { (void)hud; (void)mode; (void)anim; (void)loop; }
void HudSetAnimMode(void* hud, uint32_t mode) { (void)hud; (void)mode; }
void HudSetAnimRate(void* hud, float rate) { (void)hud; (void)rate; }
ape::ApeAnimScriptInstance* HudScriptInstance(void* hud) { (void)hud; return nullptr; }
bool EntityIsDead(WorldObject* entity) { (void)entity; return false; }
uint32_t FindCollectableId(uint32_t nameHash) { (void)nameHash; return 0; }
int GameStateCollectableQuery(uint32_t id) { (void)id; return 1; }
bool PlayerNotifyFlagIsSet(int playerIndex, uint32_t flag)
{
    const PlayerNotifyFlags* flags = PlayerNotifyFlagsOf(playerIndex);
    return flags && flags->IsSet(flag);
}
void CameraBlendWeight(float weight, float duration, float exponent) { (void)weight; (void)duration; (void)exponent; }
void KillSequence(uint32_t name) { (void)name; }
uint32_t CurrentLevelName() { return 0; }
void RestartLevel() {}
void ChangeLevel(uint32_t levelName) { (void)levelName; }
void SendGameEvent(WorldObject* sender, WorldObject* receiver, uint32_t event, uintptr_t data, bool immediate)
{
    (void)sender; (void)receiver; (void)event; (void)data; (void)immediate;
}
void BroadcastGameEvent(WorldObject* sender, uint32_t group, uint32_t event, uintptr_t data)
{
    (void)sender; (void)group; (void)event; (void)data;
}
void BroadcastGameEventInRange(WorldObject* sender, uint32_t group, uint32_t event, uintptr_t data, float range)
{
    (void)sender; (void)group; (void)event; (void)data; (void)range;
}
WorldObject* EntityFindChild(WorldObject* entity, uint32_t childName) { (void)entity; (void)childName; return nullptr; }
WorldObject* EntityVarValue(ape::ApeAnimScriptVar* var)
{
    // The caller has checked the type (kVarType_Trigger).
    return static_cast<ScriptTriggerVar*>(var)->GetValue();
}
void GoalSetComplete(uint32_t goal, bool complete, bool notify) { (void)goal; (void)complete; (void)notify; }
void GlobalToggleOn() {}

// TODO(0x0044C43C FindMixState)
const audio::MixState* FindMixState(uint32_t nameHash) { (void)nameHash; return nullptr; }
// TODO(0x00446984 FindSoundName)
audio::SoundName* FindSoundName(uint32_t soundHash, uint32_t bankHash) { (void)soundHash; (void)bankHash; return nullptr; }

} // namespace levels
} // namespace sly
