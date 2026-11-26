// openTIT - Stubs for CharacterHooks.h.
#include "sly/entity/Awards.h"
#include "sly/entity/EntityHits.h"
#include "sly/game/Upgrades.h"

#include <algorithm>
#include <vector>
#include "sly/entity/CharacterHooks.h"

#include "sly/components/ReceptacleComponent.h"

#include "sly/entity/EntityHits.h"

namespace sly {
namespace entity {

uint32_t CharacterCompModel(EntityComponent* component) { (void)component; return 0; }
uint32_t CharacterCompDefaultModel(EntityComponent* component) { (void)component; return 0; }
void CharacterCompSetModel(EntityComponent* component, uint32_t model) { (void)component; (void)model; }
void CharacterCompResetPose(EntityComponent* component) { (void)component; }
void CharacterCompSetAnimController(EntityComponent* component, void* controller) { (void)component; (void)controller; }
uint32_t PhysicsCompCollisionGroup(EntityComponent* component) { (void)component; return 0; }
void* PhysicsCompRigidBody(EntityComponent* component) { (void)component; return nullptr; }
void RigidBodySetCollisionGroup(void* body, uint32_t group) { (void)body; (void)group; }
ape::Vec3 CharacterCompVelocity(EntityComponent* component) { (void)component; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
float CharacterCompTurnRate(EntityComponent* component) { (void)component; return 0.0f; }
float RigidBodyGravityFactor(void* body) { (void)body; return 0.0f; }
void RigidBodySetGravityFactor(void* body, float factor) { (void)body; (void)factor; }

void PhysicsCompReset(EntityComponent* component) { (void)component; }
void PhysicsCompResetAfterSwap(EntityComponent* component) { (void)component; }
void PhysicsCompSetDisabled(EntityComponent* component, bool disabled) { (void)component; (void)disabled; }
void CharacterCompSendAnimEvent(EntityComponent* component, uint32_t event) { (void)component; (void)event; }
void Component10SetHidden(EntityComponent* component, bool hidden) { (void)component; (void)hidden; }
void Component24Reset(EntityComponent* component) { (void)component; }
void Component15Reset(EntityComponent* component) { (void)component; }
void EntityAttachChild(Entity* parent, const EntityChildLink& link) { (void)parent; (void)link; }
void EntitySetState(Entity* entity, int layer, int state) { (void)entity; (void)layer; (void)state; }
int PhysicsCompMovementMode(const EntityComponent* component) { (void)component; return 0; }
bool ListenerIsHeavy(EntityListener* listener) { (void)listener; return false; }
void ListenerOnDropped(EntityListener* listener, const uint8_t* dropSettings) { (void)listener; (void)dropSettings; }
void ObjectApplyAlphaBuffer(ape::world::ApeObj* object, const void* buffer, uint8_t alpha) { (void)object; (void)buffer; (void)alpha; }
int32_t RenderObjectLodField(const ape::world::ApeObj* object) { (void)object; return 0; }
void ListenerOnContact(EntityListener* listener, const void* event) { (void)listener; (void)event; }
void RenderObjectSetLodField(ape::world::ApeObj* object, int32_t value) { (void)object; (void)value; }
void GameWorldQueueCallback(int list, WorldCallback fn, void* owner, Entity* entity) { (void)list; (void)fn; (void)owner; (void)entity; }
void GameWorldDequeueCallbacks(int list, void* owner) { (void)list; (void)owner; }
// TODO(0x003F2990): minigame code outside the entity module.
void MinigameTrackRewound() {}
Entity* HitSourceOwner(Entity* source, HitOwnerKind kind) { (void)source; (void)kind; return nullptr; }
bool ThrownBySelf(Entity* thrown, Entity* entity) { (void)thrown; (void)entity; return false; }
void* HitOtherBody(const EntityHitRecord& hit) { (void)hit; return nullptr; }
bool RigidBodyIsKeyframed(void* body) { (void)body; return false; }
uint32_t HitOtherCollisionLayer(const EntityHitRecord& hit) { (void)hit; return 0; }
float RigidBodyMass(void* body) { (void)body; return 0.0f; }
// @func 0x000B2AE0 RecordMeleeHit
// Hits a player lands on an entity with a listener (an AI) count towards a
// trophy: either of the two damage bits awards it. NOTE: the trophy's name
// (hash 0x67DEAB5E) is not recovered.
void RecordMeleeHit(Entity* victim, const EntityHitRecord& hit)
{
    constexpr uint32_t kTrophy_MeleeHit = 0x67DEAB5Eu;
    if (!victim->listener || !hit.source || !(hit.source->classBits & 1))
        return;
    if (hit.damageBits & 0x0400)
        AwardTrophyByName(kTrophy_MeleeHit);
    if (hit.damageBits & 0x0800)
        AwardTrophyByName(kTrophy_MeleeHit);
}
bool ListenerIgnoresDamage(EntityListener* listener) { (void)listener; return false; }
void GameWorldPlayerDamaged(uint32_t player) { (void)player; }
void GameWorldNpcDamaged(Entity* entity) { (void)entity; }
ape::Vec3 PhysicsCompUpVector(EntityComponent* component) { (void)component; return { 0.0f, 1.0f, 0.0f, 0.0f }; }
ape::Vec3 PhysicsCompSideVector(EntityComponent* component) { (void)component; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void* AnimMapFindClip(EntityComponent* animMap, uint32_t name) { (void)animMap; (void)name; return nullptr; }
void EntityRootMotionDelta(Entity* entity, ape::Matrix34& out, bool consume)
{
    (void)entity; (void)consume;
    out = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };
}
void EntitySetWorldTransform(Entity* entity, const ape::Matrix34& transform) { (void)entity; (void)transform; }
ape::ApeAnimScriptInstance* EntityStartScript(Entity* entity, ape::ApeAnimScript* script, bool flag, bool attach)
{
    (void)entity; (void)script; (void)flag; (void)attach;
    return nullptr;
}
void AttachAnimScript(ape::ApeAnimScriptInstance* instance) { (void)instance; }
void PlayerBlockDisguiseEnded(uint32_t player) { (void)player; }
Entity* AttackTargetOf(Entity* attacker) { (void)attacker; return nullptr; }
float ScoringValue(EntityComponent* scoring) { (void)scoring; return 0.0f; }
void ScoringAward(EntityComponent* scoring, int points, int multiplier) { (void)scoring; (void)points; (void)multiplier; }
bool ScoringPaid(EntityComponent* scoring) { (void)scoring; return false; }
void ScoringSetPaid(EntityComponent* scoring) { (void)scoring; }
void AddPlayerScore(uint32_t player, int points) { (void)player; (void)points; }
void MinigameFigureRemoved(uint32_t model) { (void)model; }
float EntityStateTime(Entity* entity, uint32_t layer) { (void)entity; (void)layer; return 0.0f; }
void EntitySetStateTime(Entity* entity, uint32_t layer, float time) { (void)entity; (void)layer; (void)time; }
void InteractionSetOffer(EntityComponent* interaction, uint32_t kinds) { (void)interaction; (void)kinds; }
Entity* InteractionUser(EntityComponent* interaction) { (void)interaction; return nullptr; }
int32_t InteractionUsers(EntityComponent* interaction) { (void)interaction; return 0; }
Entity* CharacterCarried(Entity* character) { (void)character; return nullptr; }
uint32_t PlayerBlockValue10D8(uint32_t player) { (void)player; return 0; }
int CarryCompFreeSlot(EntityComponent* carry)
{
    return static_cast<components::ReceptacleComponent*>(carry)->FreeSlot();
}
void GroupVarSetValue(void* var, float value) { (void)var; (void)value; }
void PlayerBlockSetValue10D8(uint32_t player, uint32_t value) { (void)player; (void)value; }
void InteractionWithdraw(EntityComponent*) {}
bool CharacterWantsToLeave(Entity*) { return false; }
bool CharacterCompHasController(EntityComponent* component) { (void)component; return false; }
void CharacterCompSetPoseHook(EntityComponent* component, void (*hook)(void*), void* arg)
{
    (void)component; (void)hook; (void)arg;
}
float PlayerBlockBusyTime(uint32_t player) { (void)player; return 0.0f; }
void GameWorldDequeueSafeCallback(Entity* safe) { (void)safe; }
void GameWorldSetHudHidden(bool hidden) { (void)hidden; }
void GameWorldSetPlayerControl(bool enabled) { (void)enabled; }
void InteractionClearFlag20(EntityComponent* interaction) { (void)interaction; }
void PlayCharacterFx(EntityComponent* component, uint32_t fx, float length, float fade) { (void)component; (void)fx; (void)length; (void)fade; }
float SkinRate(EntityComponent* skin) { (void)skin; return 0.0f; }
void SkinSetRate(EntityComponent* skin, float rate) { (void)skin; (void)rate; }
void SkinSetScale(EntityComponent* skin, float scale) { (void)skin; (void)scale; }
float SkinShotInterval(EntityComponent* skin) { (void)skin; return 0.0f; }
float SkinComputeRate(EntityComponent* skin) { (void)skin; return 0.0f; }
void SkinSetLevel(EntityComponent* skin, uint32_t level) { (void)skin; (void)level; }
int32_t SkinVariant(EntityComponent* skin) { (void)skin; return 0; }
void SkinSetVariant(EntityComponent* skin, int32_t variant) { (void)skin; (void)variant; }
ape::ApeRefCounted* WorldObjectRef(ape::world::ApeObj* object) { (void)object; return nullptr; }
void SetAnimScriptKeepAlive(void* script, bool keep) { (void)script; (void)keep; }
bool IgnoresKillOrder(Entity* entity) { (void)entity; return false; }
void* EntityPlayDirectorScriptInstance(Entity* entity, uint32_t script) { (void)entity; (void)script; return nullptr; }
void CullManagerRelease(uint32_t handle) { (void)handle; }
void PhysicsCompSetTarget(EntityComponent* component, const ape::Vec3& point) { (void)component; (void)point; }
void* GadgetAbilityList() { return nullptr; }
int GadgetAbilityCount(void* list) { (void)list; return 0; }
uint32_t GadgetAbility(void* list, int index) { (void)list; (void)index; return 0; }
void* FindHudElement(uint32_t name) { (void)name; return nullptr; }
void HudElementHide(void* element) { (void)element; }
void HudElementShow(void* element, float time, const char* text) { (void)element; (void)time; (void)text; }
bool IsGameTimeFrozen() { return false; }
void Component11Refresh(EntityComponent* component) { (void)component; }
void Component10SetAlternate(EntityComponent* component, bool alternate) { (void)component; (void)alternate; }
uint32_t Component15ObjectCount(EntityComponent* component) { (void)component; return 0; }
ape::world::ApeObj* Component15Object(EntityComponent* component, uint32_t index) { (void)component; (void)index; return nullptr; }
bool Component03Busy(EntityComponent* component) { (void)component; return false; }

ape::world::ApeObj* CreateGeomInstance(void* geomDef, bool flag) { (void)geomDef; (void)flag; return nullptr; }
void SetGeomMaterialColor(ape::world::ApeObj* object, const uint8_t color[4]) { (void)object; (void)color; }
void CharacterHudColor(const EntityDef* def, uint8_t out[4])
{
    (void)def;
    out[0] = out[1] = out[2] = out[3] = 0xFF;
}
float EaseTint(float t) { return t; }
void AnimControllerSetFlag(void* controller, uint16_t bit, bool on) { (void)controller; (void)bit; (void)on; }
void AnimControllerSetEventSink(void* controller, Entity* owner) { (void)controller; (void)owner; }

uint8_t PlayerProfileIndex(uint32_t player) { (void)player; return 0; }
bool ProfileAlternateCostume(uint8_t profile, uint32_t costume) { (void)profile; (void)costume; return false; }
uint32_t ProfileSelection(uint8_t profile, uint32_t offset) { (void)profile; (void)offset; return 0; }
bool LevelPlayerHasAbility(uint32_t player, int ability) { (void)player; (void)ability; return false; }
uint32_t PurchasedAbilities(uint32_t character, uint32_t* out, uint32_t max)
{
    std::vector<uint32_t> items;
    CollectBoughtItems(character, items);
    const uint32_t count = std::min<uint32_t>(uint32_t(items.size()), max);
    std::copy_n(items.begin(), count, out);
    return count;
}
bool WorldFlag(uint32_t name) { (void)name; return false; }
bool PhysicsWorldPresent() { return false; }

void EntityPlayDirectorScript(Entity* entity, uint32_t script) { (void)entity; (void)script; }
void EntityAttachSpawned(Entity* parent, EntityPlacement& placement, Entity* child) { (void)parent; (void)placement; (void)child; }

// TODO(0x0012C968 / 0x00129D94): character placement helpers.
void CharacterSettleAfterMove(Entity*, float, int32_t, int32_t) {}
void CharacterStoreRestorePoint(Entity*) {}

} // namespace entity
} // namespace sly
