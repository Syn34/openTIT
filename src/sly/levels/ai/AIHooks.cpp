// openTIT - Level AI hooks: TODO stubs returning "nothing there".
#include "sly/levels/ai/AIHooks.h"

#include "sly/entity/Entity.h"
#include "sly/game/goals/JobManager.h"
#include "sly/knowledge/Knowledge.h"

namespace sly {
namespace levels {
namespace ai {

const char* PropString(const PropertyBlock*, AIProp) { return nullptr; }
void SettingClassAddProperty(AISettingClass*, AIProp) {}
uint32_t PropHash(const PropertyBlock*, AIProp) { return 0; }
bool PropBool(const PropertyBlock*, AIProp) { return false; }
float PropFloat(const PropertyBlock*, AIProp) { return 0.0f; }

const PropertyBlock* FindSettingObject(uint32_t) { return nullptr; }
bool CharacterSpawnerSettings(const Entity*, uint32_t[3], const PropertyBlock*[3]) { return false; }

bool SpawnerAltObjectBounds(const Entity*, ape::Vec3&, ape::Vec3&) { return false; }
bool SpawnerIsSpawnField(const Entity*) { return false; }
bool HasSpawner(const Entity* entity) { return entity && entity->field_DC; }
SpawnerRef* EntitySpawner(const Entity*) { return nullptr; }
int SpawnerLiveCount(const SpawnerRef*) { return 0; }
Entity* SpawnerLive(const SpawnerRef*, int) { return nullptr; }
void SpawnerSwapLive(SpawnerRef*, int, int) {}
RaceTracker* SpawnerRaceTracker(const SpawnerRef*) { return nullptr; }
void AddToListenerGroup(Entity*, uint32_t, bool) {}

Entity* MessageSender(const entity::EntityMessage&) { return nullptr; }

void EntityTick(Entity*, float) {}
void EntitySetTransform(Entity* entity, const ape::Matrix34& transform)
{
    if (entity)
        entity->transform = transform;
}
void EntitySetEngaged(Entity*, bool) {}
void EntityResetAnimMode(Entity*) {}
ape::Matrix34 EntityBoneTransform(Entity* entity, uint32_t boneHash)
{
    return entity ? entity->BoneTransformByName(boneHash, true, true) : ape::Matrix34{};
}
void EntityRootMotion(Entity*, float, ape::Matrix34&) {}
ape::Vec3 EntityObjectScale(const Entity*) { return {1.0f, 1.0f, 1.0f, 0.0f}; }
void AnimControllerSetEngaged(Entity*, bool) {}
void CharacterClearAIHold(Entity* character)
{
    if (character)
        character->classBits &= ~uint64_t(0x4000);
}

bool BodyIsEnabled(EntityComponent*) { return false; }
void BodySetEnabled(EntityComponent*, bool) {}
void BodySync(EntityComponent*) {}
ape::Vec3 BodyPosition(EntityComponent*) { return {}; }
ape::Matrix34 BodyRotation(EntityComponent*) { return {}; }
int BodyMotionType(EntityComponent*) { return 0; }
void BodyRequestVelocity(EntityComponent*, const ape::Vec3&) {}
void BodyRequestStop(EntityComponent*) {}
void BodyRequestHold(EntityComponent*) {}
ape::Vec3 BodyGroundNormal(EntityComponent*) { return {0.0f, 1.0f, 0.0f, 0.0f}; }
void SendGameEvent(Entity*, Entity*, uint32_t, uint32_t, bool) {}
void BroadcastInRange(Entity*, float, uint32_t, uint32_t, uint32_t) {}

void EntitySetScriptState(Entity*, uint32_t) {}
bool EntityHoldsAttention(const Entity*) { return false; }
bool RacerHasVehicle(const Entity*) { return false; }
void CharacterSetEventListener(EntityComponent*, AIController*) {}
bool AttackerComponentFlag(Entity*, bool* hasComponent)
{
    if (hasComponent)
        *hasComponent = false;
    return false;
}
void EntityDamage(Entity*, float, Entity*, int) {}
void Component24Reset(EntityComponent*) {}
void BodyClearRequestBit31(EntityComponent*) {}

bool CharacterPlayClip(EntityComponent*, uint32_t) { return false; }
bool CharacterClipFinished(EntityComponent*) { return false; }
bool CharacterHasClip(EntityComponent*, uint32_t) { return false; }
float CharacterHealth(const Entity*) { return 0.0f; }

bool NavPointIsValid(void* point) { return point != nullptr; }
void NavPointRelease(void*) {}
void NavAgentDestroy(NavAgent*) {}
void NavAgentRequestGoal(NavAgent*, const ape::Vec3&) {}
ape::Vec3 NavAgentGoal(NavAgent*) { return {}; }

void KnowledgeLevelStart() { knowledge::KnowledgeLevelStart(); }
void KnowledgeLevelEnd() { knowledge::KnowledgeLevelEnd(); }
void KnowledgeReset() { knowledge::KnowledgeReset(); }
void AIObjectPoolLevelStart() {}
void AIObjectPoolLevelEnd() {}
void AIObjectPoolReset() {}
void ReleaseCameraMode(AICameraMode*) {}
void GameWorldSetBossFlag(bool) {}
void HudSetContext(Entity*, const char*) {}
void HudSetContext(Entity*, uint32_t) {}
bool HudObjectPausesAI(const hud::HudObject*) { return false; }

ape::ApeRefCounted* EntityScriptObject(const Entity*) { return nullptr; }
void ReleaseAnimScript(ape::ApeAnimScriptInstance*) {}

Entity* PlayerCharacter() { return nullptr; }
bool RenderZoomActive() { return false; }
float RenderZoom() { return 1.0f; }
bool AITargetingSuppressed() { return false; }
void GameStateNotify()
{
    if (game::JobManager* manager = game::GetJobManager())
        manager->FlushPendingRequest();
}
void PostScriptMessage(Entity*, uint32_t, uint32_t, uint32_t) {}
uint32_t ProxyPointMessageKey() { return 0; }
uint32_t WorldObjectKeyOf(const WorldObject*) { return 0; }
WorldObject* WorldObjectFirstChild(const WorldObject*) { return nullptr; }
WorldObject* WorldObjectNextSibling(const WorldObject*) { return nullptr; }
AIProxyPoint* ProxyPointOf(const WorldObject*) { return nullptr; }
int ProxyPointIndexFromSpawner(const Entity*) { return -1; }
void EntityApplyRenderZoom(Entity*, float) {}
float LevelFloorDepth() { return 0.0f; }

} // namespace ai
} // namespace levels
} // namespace sly
