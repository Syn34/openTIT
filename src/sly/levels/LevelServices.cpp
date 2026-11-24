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

} // namespace levels
} // namespace sly
