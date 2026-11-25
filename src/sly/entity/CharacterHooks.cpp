// openTIT - Stubs for CharacterHooks.h.
#include "sly/entity/CharacterHooks.h"

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
ape::Vec3 CharacterCompAimPosition(EntityComponent* component) { (void)component; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
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
void EntitySetFadeAlpha(Entity* entity, bool flag, uint8_t alpha, bool immediate) { (void)entity; (void)flag; (void)alpha; (void)immediate; }
void EntityApplyFadeBuffer(Entity* entity) { (void)entity; }
bool ListenerIsHeavy(EntityListener* listener) { (void)listener; return false; }
void ListenerOnDropped(EntityListener* listener, const uint8_t* dropSettings) { (void)listener; (void)dropSettings; }
void ObjectApplyAlphaBuffer(ape::world::ApeObj* object, const void* buffer, uint8_t alpha) { (void)object; (void)buffer; (void)alpha; }
void RenderObjectSetLodField(ape::world::ApeObj* object, int32_t value) { (void)object; (void)value; }
void GameWorldQueueCallback(int list, WorldCallback fn, void* owner, Entity* entity) { (void)list; (void)fn; (void)owner; (void)entity; }
Entity* HitSourceOwner(Entity* source, HitOwnerKind kind) { (void)source; (void)kind; return nullptr; }
bool ThrownBySelf(Entity* thrown, Entity* entity) { (void)thrown; (void)entity; return false; }
void* HitOtherBody(const EntityHitRecord& hit) { (void)hit; return nullptr; }
bool RigidBodyIsKeyframed(void* body) { (void)body; return false; }
float RigidBodyMass(void* body) { (void)body; return 0.0f; }
void RecordMeleeHit(Entity* victim, const EntityHitRecord& hit) { (void)victim; (void)hit; }
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
void Component11Refresh(EntityComponent* component) { (void)component; }
void Component10SetAlternate(EntityComponent* component, bool alternate) { (void)component; (void)alternate; }
uint32_t Component15ObjectCount(EntityComponent* component) { (void)component; return 0; }
ape::world::ApeObj* Component15Object(EntityComponent* component, uint32_t index) { (void)component; (void)index; return nullptr; }
bool Component03Busy(EntityComponent* component) { (void)component; return false; }

void ObjectSetTint(float amount, ape::world::ApeObj* object, const uint8_t color[4]) { (void)amount; (void)object; (void)color; }
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
uint32_t PurchasedAbilities(uint32_t character, uint32_t* out, uint32_t max) { (void)character; (void)out; (void)max; return 0; }
bool WorldFlag(uint32_t name) { (void)name; return false; }
bool PhysicsWorldPresent() { return false; }

void EntityPlayDirectorScript(Entity* entity, uint32_t script) { (void)entity; (void)script; }
void EntityAttachSpawned(Entity* parent, SpawnParams& params, Entity* child) { (void)parent; (void)params; (void)child; }
void EntityDefBaseBounds(const EntityDef* def, ape::Aabb& out)
{
    (void)def;
    out.min = { 0.0f, 0.0f, 0.0f, 0.0f };
    out.max = { 0.0f, 0.0f, 0.0f, 0.0f };
}

} // namespace entity
} // namespace sly
