// openTIT - Stubs of the services used by the third batch of level script code.
#include "sly/levels/LevelServices.h"

namespace sly {
namespace levels {

// TODO(0x00399000 laser manager) the laser module is not ported yet.
LaserGroup* FindLaserGroup(uint32_t nameHash) { (void)nameHash; return nullptr; }
Laser* FindLaser(uint32_t nameHash) { (void)nameHash; return nullptr; }
LaserField* FindLaserField(uint32_t nameHash) { (void)nameHash; return nullptr; }
void LaserGroupReset(LaserGroup* group) { (void)group; }
void LaserGroupSetEnabled(LaserGroup* group, bool enable) { (void)group; (void)enable; }
void LaserReset(Laser* laser) { (void)laser; }
void LaserSetEnabled(Laser* laser, bool enable) { (void)laser; (void)enable; }
void LaserFieldSetEnabled(LaserField* field, bool enable, bool immediate) { (void)field; (void)enable; (void)immediate; }

// TODO(0x000D951C EntityInteraction) interaction components live in the entity module.
void* EntityInteractionComponent(WorldObject* entity) { (void)entity; return nullptr; }
uint32_t InteractionComponentId(const void* interaction) { (void)interaction; return 0; }
void InteractionActivate(void* interaction, WorldObject* player) { (void)interaction; (void)player; }
void InteractionDeactivate(void* interaction) { (void)interaction; }
void InteractionSetCancelAllowed(void* interaction, bool allowed) { (void)interaction; (void)allowed; }
void CharacterBeginInteraction(WorldObject* character, uint32_t interactionId) { (void)character; (void)interactionId; }

ape::ApeRefCounted* ObjectFindDescendant(ape::ApeRefCounted* object, uint32_t nameHash) { (void)object; (void)nameHash; return nullptr; }
bool ObjectIsGeometryInstance(const ape::ApeRefCounted* object) { (void)object; return false; }
ape::Vec3 ObjectWorldPosition(const ape::ApeRefCounted* object) { (void)object; return ape::Vec3{}; }
void* ObjectVarAnimController(const ape::ApeAnimScriptObjectVar* var) { (void)var; return nullptr; }
void BinocucomSetObjectMaterial(void* binocucom, ape::ApeRefCounted* geometry, uint32_t material) { (void)binocucom; (void)geometry; (void)material; }

void* AnimControllerFindLayer(void* controller, int32_t layer) { (void)controller; (void)layer; return nullptr; }
float AnimLength(const void* anim) { (void)anim; return 0.0f; }
void AnimSetTime(void* anim, float time) { (void)anim; (void)time; }
void CharacterBeginAnimEdit(void* character) { (void)character; }
void CharacterSetAnimTime(void* character, float time, bool normalized, int32_t layer) { (void)character; (void)time; (void)normalized; (void)layer; }
void CharacterEndAnimEdit(void* character) { (void)character; }
void EntitySetAnimElapsed(WorldObject* entity, float time, bool normalized, int32_t anim) { (void)entity; (void)time; (void)normalized; (void)anim; }

void CameraAddShake(const SequenceShakeParams& params) { (void)params; }

void* CharacterSpecialComponent(WorldObject* character) { (void)character; return nullptr; }
void SpecialSetDirection(void* special, const ape::Vec3& direction) { (void)special; (void)direction; }
void SpecialSetAngle(void* special, float angle) { (void)special; (void)angle; }
void SpecialSetEnabled(void* special, bool enabled) { (void)special; (void)enabled; }
void CharacterSetSkipAnim(WorldObject* character, bool skip) { (void)character; (void)skip; }

bool EntityIsSentryLight(const WorldObject* entity) { (void)entity; return false; }
void SentryLightSetEnabled(WorldObject* light, bool enable) { (void)light; (void)enable; }
void SentryLightSetLocked(WorldObject* light, bool locked) { (void)light; (void)locked; }
void FxKillEntityEffects(WorldObject* entity) { (void)entity; }

void* PhysicsFindRigidBody(const char* name) { (void)name; return nullptr; }
bool RigidBodyIsInWorld(const void* body) { (void)body; return false; }
void PhysicsAddRigidBody(void* body) { (void)body; }
void PhysicsRemoveRigidBody(void* body) { (void)body; }
void EntitySetCollisionDisabled(WorldObject* entity, bool disabled, int32_t index) { (void)entity; (void)disabled; (void)index; }
void* EntityFindComponentById(WorldObject* entity, uint32_t id) { (void)entity; (void)id; return nullptr; }
bool ComponentIsEnabled(const void* component) { (void)component; return false; }
void ComponentSetEnabled(void* component, bool enabled) { (void)component; (void)enabled; }
const char* EntityDebugName(WorldObject* entity) { (void)entity; return ""; }

CharacterCollectInfo CharacterDefCollectInfo(const WorldObject* character) { (void)character; return {}; }
void EntitySetEnabledState(WorldObject* entity, bool enabled) { (void)entity; (void)enabled; }
void EntitySetVisibilityDeferred(WorldObject* entity, bool visible, bool deferred) { (void)entity; (void)visible; (void)deferred; }
int EntityChildCount(const WorldObject* entity) { (void)entity; return 0; }
WorldObject* EntityChildAt(const WorldObject* entity, int index) { (void)entity; (void)index; return nullptr; }
uint32_t EntityScriptState(WorldObject* entity) { (void)entity; return 0; }
void EntitySetScriptState(WorldObject* entity, uint32_t state, bool flag) { (void)entity; (void)state; (void)flag; }
bool EntityIsGadgetObject(const WorldObject* entity) { (void)entity; return false; }
void GadgetObjectSetState(WorldObject* entity, uint32_t state) { (void)entity; (void)state; }
void EntitySetNoTarget(WorldObject* entity, bool noTarget) { (void)entity; (void)noTarget; }
void EntitySetVisible(WorldObject* entity, bool visible, bool immediate, bool children, bool components) { (void)entity; (void)visible; (void)immediate; (void)children; (void)components; }

int GameWorldAllocScreenFilter()
{
    // TODO: the game world's filter table; one shared record until it exists.
    return 0;
}
ScreenFilterFade& GameWorldScreenFilter(int index)
{
    static ScreenFilterFade s_fallback;
    (void)index;
    return s_fallback;
}
void EntitySetAlpha(WorldObject* entity, uint8_t alpha, bool flag) { (void)entity; (void)alpha; (void)flag; }
bool ObjectIsLight(const ape::ApeRefCounted* object) { (void)object; return false; }
void LightSetIntensity(ape::ApeRefCounted* light, float intensity) { (void)light; (void)intensity; }
void LightSetColor(ape::ApeRefCounted* light, const ape::xml::Color8& color) { (void)light; (void)color; }
void GeometrySetAlpha(ape::ApeRefCounted* geometry, uint8_t alpha, bool flag) { (void)geometry; (void)alpha; (void)flag; }
void* GeometryFindMaterial(ape::ApeRefCounted* geometry, uint32_t materialName) { (void)geometry; (void)materialName; return nullptr; }
void GeometrySetTint(ape::ApeRefCounted* geometry, const ape::xml::Color8& color) { (void)geometry; (void)color; }
void* FindLiveMaterial(uint32_t nameHash) { (void)nameHash; return nullptr; }
void MaterialSetColor(void* material, const ape::xml::Color8& color) { (void)material; (void)color; }
void GuiNodeSetColor(void* node, const ape::xml::Color8& color) { (void)node; (void)color; }
void CharacterSetOverrideColor(WorldObject* character, bool active, const ape::xml::Color8& color) { (void)character; (void)active; (void)color; }

} // namespace levels
} // namespace sly
