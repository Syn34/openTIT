// openTIT - Level AI hooks: TODO stubs returning "nothing there".
#include "sly/levels/ai/AIHooks.h"

#include "sly/entity/Entity.h"

namespace sly {
namespace levels {
namespace ai {

const char* PropString(const PropertyBlock*, AIProp) { return nullptr; }
uint32_t PropHash(const PropertyBlock*, AIProp) { return 0; }
bool PropBool(const PropertyBlock*, AIProp) { return false; }
float PropFloat(const PropertyBlock*, AIProp) { return 0.0f; }

const PropertyBlock* FindSettingObject(uint32_t) { return nullptr; }
bool CharacterSpawnerSettings(const Entity*, uint32_t[3], const PropertyBlock*[3]) { return false; }

void EntityTick(Entity*, float) {}
void EntitySetTransform(Entity* entity, const ape::Matrix34& transform)
{
    if (entity)
        entity->transform = transform;
}
void EntitySetEngaged(Entity*, bool) {}
void EntityResetAnimMode(Entity*) {}
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

bool CharacterPlayClip(EntityComponent*, uint32_t) { return false; }
bool CharacterClipFinished(EntityComponent*) { return false; }
float CharacterHealth(const Entity*) { return 0.0f; }

bool NavPointIsValid(void* point) { return point != nullptr; }
void NavPointRelease(void*) {}
void NavAgentDestroy(NavAgent*) {}

void ReleaseAnimScript(ape::ApeAnimScriptInstance*) {}

Entity* PlayerCharacter() { return nullptr; }
bool RenderZoomActive() { return false; }
float RenderZoom() { return 1.0f; }
bool AITargetingSuppressed() { return false; }
void GameStateNotify() {}
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
