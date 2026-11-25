// openTIT - Stubs for the calls the camera code makes into unported systems.
#include "sly/camera/CameraHooks.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/Entity.h"

#include "sly/camera/CameraSettings.h"
#include "sly/entity/ActorEntities.h"

namespace sly {
namespace camera {

// Entity virtual slot 19; forward once Entity declares it.
const char* EntityName(entity::Entity* entity) { (void)entity; return "<none>"; }
// Entity virtual slot 39; forward once Entity declares it.
ape::Vec3 EntityFocusPoint(entity::Entity* entity) { (void)entity; return {}; }
void EntitySetWorldTransform(entity::Entity* entity, const ape::Matrix34& transform) { (void)entity; (void)transform; }
ape::Vec3 SpotlightCameraOffset(entity::Entity* spotlight)
{
    return static_cast<entity::SearchlightActor*>(spotlight)->CameraOffset();
}
bool IsCharacterEntity(const entity::Entity* entity) { (void)entity; return false; }
bool IsVehicleEntity(const entity::Entity* entity) { (void)entity; return false; }
bool Component03Flag31(entity::Entity* entity) { (void)entity; return false; }
const CameraSettings* CharacterCameraSettings(entity::Entity* character) { (void)character; return nullptr; }
const CameraSettings* VehicleCameraSettings(entity::Entity* vehicle) { (void)vehicle; return nullptr; }
const OrthoCameraDesc* VehicleOrthoCamera(entity::Entity* vehicle) { (void)vehicle; return nullptr; }

float CameraInstanceFov(ape::world::ApeObj* camera) { (void)camera; return 0.0f; }

void QueueEntityTransformCallback(entity::Entity* entity)
{
    entity::GameWorldQueueCallback(0, &entity::QueuedUpdateTransform, entity, entity);
}

void DeleteAnimController(void* controller) { (void)controller; }
void AnimControllerUpdate(void* controller, float dt) { (void)controller; (void)dt; }
void AnimControllerStopAll(void* controller) { (void)controller; }
void AnimPrepareClip(void* controller, uint32_t clip) { (void)controller; (void)clip; }
void AnimStartClip(float blend, void* controller, void* clip, bool loop) { (void)blend; (void)controller; (void)clip; (void)loop; }
void AnimCrossfadeClip(float blend, void* playing, void* clip, bool loop) { (void)blend; (void)playing; (void)clip; (void)loop; }
bool AnimReachedEnd(void* playing) { (void)playing; return false; }

void ResetObjectPivot(ape::world::ApeObj* object) { (void)object; }

const VehicleCameraDesc* VehicleCameraData(entity::Entity* vehicle) { (void)vehicle; return nullptr; }

void* CreateSpherePhantom(float radius, uint32_t filterInfo) { (void)radius; (void)filterInfo; return nullptr; }
void DestroyPhantom(void* phantom) { (void)phantom; }
void DestroyHavokEntity(void* entity) { (void)entity; }
void RemoveHavokEntityFromWorld(void* entity) { (void)entity; }
void HavokRemoveReference(void* object) { (void)object; }
bool EntityCollisionFilter(entity::Entity* entity, uint32_t& filter) { (void)entity; (void)filter; return false; }
void SetEntityCollisionFilter(void* entity, uint32_t filter) { (void)entity; (void)filter; }
void SetPhantomCollisionFilter(void* phantom, uint32_t filter) { (void)phantom; (void)filter; }
uint32_t CharacterCollisionGroup(entity::Entity* entity) { (void)entity; return 0; }
void ConstructVehicleFollowCamera(void* storage) { (void)storage; }
void DestroyVehicleFollowCamera(void* storage) { (void)storage; }

void SetDofEffectParams(agl::AglPostProcEffect* effect, float focus, float range, float blend, uint8_t iterations)
{
    (void)effect; (void)focus; (void)range; (void)blend; (void)iterations;
}
void SetFilterEffectParams(agl::AglPostProcEffect* effect, float blend, const uint8_t color[4])
{
    (void)effect; (void)blend; (void)color;
}
bool BinocucomForcesDof() { return false; }
bool WorldForcesDof() { return false; }
float ScreenWidth() { return 1280.0f; }
float ScreenHeight() { return 720.0f; }
float SafeAreaWidth() { return -1.0f; }
float SafeAreaHeight() { return -1.0f; }
bool IsDisplay4x3() { return false; }

agl::AglRenderTarget* MainRenderTarget() { return nullptr; }
void* PlayerPad(entity::Entity* entity) { (void)entity; return nullptr; }
float PadAxis(void* pad, int axis) { (void)pad; (void)axis; return 0.0f; }
bool PadButtonPressed(void* pad, uint32_t mask) { (void)pad; (void)mask; return false; }

bool IsGameTimeFrozen() { return false; }
bool IsShooterSplineForcedToEnd() { return false; }

void DebugDrawSpline(ape::world::ApeObj* spline, const uint8_t color[4]) { (void)spline; (void)color; }

} // namespace camera
} // namespace sly
