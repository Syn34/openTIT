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

void GadgetObjectSetFlag(WorldObject* entity, bool value) { (void)entity; (void)value; }
bool EntityIsGadgetTurret(const WorldObject* entity) { (void)entity; return false; }
void TurretAddTarget(WorldObject* turret, WorldObject* target) { (void)turret; (void)target; }
void TurretRemoveTarget(WorldObject* turret, WorldObject* target) { (void)turret; (void)target; }
bool BinocucomIsVisible(void* binocucom) { (void)binocucom; return false; }
void* CharacterAnimController(void* character) { (void)character; return nullptr; }
void* ScriptObjectAnimController(ape::ApeAnimScriptInstance* instance, uint32_t varName) { (void)instance; (void)varName; return nullptr; }
float AnimSpeed(const void* anim) { (void)anim; return 0.0f; }

uint64_t EntityClassBits(const WorldObject* entity) { (void)entity; return 0; }
bool EntityHasEffectOfType(const WorldObject* entity, int type) { (void)entity; (void)type; return false; }
bool HasCurrentJob() { return false; }
uint32_t CurrentJobName() { return 0; }
void* GameStateFindJob(uint32_t jobName) { (void)jobName; return nullptr; }
int JobState(void* job) { (void)job; return 0; }

CharacterRespawnInfo CharacterRespawn(const WorldObject* character) { (void)character; return {}; }
void* GameStateCurrentSafe() { return nullptr; }
int SafeState(void* safe) { (void)safe; return -1; }

void HudSetPlayerHealthMeter(int player, const char* text) { (void)player; (void)text; }
void HudShowHealthMeter(WorldObject* entity, uint32_t meter) { (void)entity; (void)meter; }
void HudHideHealthMeter(WorldObject* entity) { (void)entity; }
void HudSetText(void* hud, const char* text, int32_t index) { (void)hud; (void)text; (void)index; }
void* CurrentJob() { return nullptr; }

bool EntityIsInputMover(const WorldObject* entity) { (void)entity; return false; }
void InputMoverSetPoints(WorldObject* mover, float points, bool flag) { (void)mover; (void)points; (void)flag; }
void InputMoverReset(WorldObject* mover) { (void)mover; }
bool EntityHasListener(const WorldObject* entity) { (void)entity; return false; }
void DecibelMeterTrigger(WorldObject* meter) { (void)meter; }
float EntityHealth(const WorldObject* entity) { (void)entity; return 0.0f; }
void EntityApplyDamage(WorldObject* entity, float amount) { (void)entity; (void)amount; }
void EntityRelease(WorldObject* entity, bool kill) { (void)entity; (void)kill; }

void EntityApplyOverrideColor(WorldObject* entity, bool enabled, const ape::xml::Color8& color) { (void)entity; (void)enabled; (void)color; }

void BinocucomScanObject(void* binocucom, ape::ApeRefCounted* geometry) { (void)binocucom; (void)geometry; }
void AnimSetSpeed(void* anim, float speed) { (void)anim; (void)speed; }
bool AnimIsLooping(const void* anim) { (void)anim; return false; }
void AnimJumpToEnd(void* anim) { (void)anim; }
uint8_t EntityPlayerByte(const WorldObject* entity) { (void)entity; return 0; }
void GameWorldPostEvent(uint8_t player, const GameEvent& event) { (void)player; (void)event; }

void EntityListenerSetState(WorldObject* entity, uint32_t state, uint32_t clip, uint32_t target, float value) { (void)entity; (void)state; (void)clip; (void)target; (void)value; }
void* EntityFindComponentOfType(WorldObject* entity, uint32_t type) { (void)entity; (void)type; return nullptr; }
ChainComponentParams& ChainParams(void* chainComponent)
{
    // TODO: chain components are not ported; hand out a scratch record.
    static ChainComponentParams s_scratch;
    (void)chainComponent;
    return s_scratch;
}
ape::Matrix34 PointInstanceTransform(const WorldObject* point) { (void)point; return IdentityTransform(); }
void GameWorldSetPlayerSpawn(int player, const ape::Matrix34& transform) { (void)player; (void)transform; }

void CharacterPauseAnimLayer(void* character, uint32_t layer) { (void)character; (void)layer; }
void EntitySetAnimPaused(WorldObject* entity, uint32_t layer, bool paused) { (void)entity; (void)layer; (void)paused; }
void* ObjectDef(const ape::ApeRefCounted* object) { (void)object; return nullptr; }
void* FindLevelDef(uint32_t nameHash) { (void)nameHash; return nullptr; }
int DefKind(const void* def) { (void)def; return -1; }
void ObjectSetDef(ape::ApeRefCounted* object, void* def) { (void)object; (void)def; }
void EntityRefreshRenderComponents(WorldObject* entity) { (void)entity; }
void EntitySwapWeapon(WorldObject* entity, uint32_t component, float speed, float interval1, float interval2, float angle) { (void)entity; (void)component; (void)speed; (void)interval1; (void)interval2; (void)angle; }
void TurretResetAim(WorldObject* turret) { (void)turret; }

void* FindEntityDefByName(uint32_t nameHash) { (void)nameHash; return nullptr; }
uint32_t EntityDefNameOf(const void* def) { (void)def; return 0; }
bool EntityHasClass2Flag(const WorldObject* entity, uint32_t mask) { (void)entity; (void)mask; return false; }
void CharacterSwapDef(WorldObject* character, void* def, bool deferred) { (void)character; (void)def; (void)deferred; }
void VehicleSwapDef800(WorldObject* vehicle, void* def, bool deferred) { (void)vehicle; (void)def; (void)deferred; }
void VehicleSwapDef2000(WorldObject* vehicle, void* def, bool deferred) { (void)vehicle; (void)def; (void)deferred; }
void CharacterResetAfterSwap(WorldObject* character) { (void)character; }

WorldObject* FindCameraInstance(uint32_t nameHash) { (void)nameHash; return nullptr; }

void EntityAttach(WorldObject* parent, const EntityAttachDesc& desc) { (void)parent; (void)desc; }
bool EntityForwardsAnimation(const WorldObject* entity) { (void)entity; return false; }
void EntityUpdateTransform(WorldObject* entity) { (void)entity; }
void* EntityCarryComponent(WorldObject* entity) { (void)entity; return nullptr; }
bool CarryCanTake(void* carry, WorldObject* entity) { (void)carry; (void)entity; return false; }
void CarryTake(void* carry, WorldObject* entity) { (void)carry; (void)entity; }

void* WorldTrackPlayer() { return nullptr; }
void TrackPlayerSelect(void* player, uint32_t track) { (void)player; (void)track; }
void TrackPlayerReset(void* player, uint32_t track) { (void)player; (void)track; }
void TrackPlayerSetValue(void* player, float value) { (void)player; (void)value; }
void* FindSplineDefByName(uint32_t nameHash) { (void)nameHash; return nullptr; }
void SentryLightSetSpline(WorldObject* light, void* spline) { (void)light; (void)spline; }
void SentryLightReverse(WorldObject* light) { (void)light; }
void LevelAIAddSmoke(WorldObject* owner, float radius, float duration, int32_t mode, const ape::Vec3& position) { (void)owner; (void)radius; (void)duration; (void)mode; (void)position; }

SunLightSettings* GetSunLightSettings() { return nullptr; }
void SunLightSetRange(SunLightSettings& light, float range) { (void)light; (void)range; }
void SunLightRestoreDefaults() {}

TargetingMarker* TargetingAddMarker(void* targeting) { (void)targeting; return nullptr; }

} // namespace levels
} // namespace sly
