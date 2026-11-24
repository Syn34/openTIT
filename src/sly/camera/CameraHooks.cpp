// openTIT - Stubs for the calls the camera code makes into unported systems.
#include "sly/camera/CameraHooks.h"

#include "sly/camera/CameraSettings.h"

namespace sly {
namespace camera {

// Entity virtual slot 19; forward once Entity declares it.
const char* EntityName(entity::Entity* entity) { (void)entity; return "<none>"; }
// Entity virtual slot 39; forward once Entity declares it.
ape::Vec3 EntityFocusPoint(entity::Entity* entity) { (void)entity; return {}; }
bool IsCharacterEntity(const entity::Entity* entity) { (void)entity; return false; }
bool IsVehicleEntity(const entity::Entity* entity) { (void)entity; return false; }
const CameraSettings* CharacterCameraSettings(entity::Entity* character) { (void)character; return nullptr; }
const CameraSettings* VehicleCameraSettings(entity::Entity* vehicle) { (void)vehicle; return nullptr; }
const OrthoCameraDesc* VehicleOrthoCamera(entity::Entity* vehicle) { (void)vehicle; return nullptr; }

float CameraInstanceFov(ape::world::ApeObj* camera) { (void)camera; return 0.0f; }

void EntityAnimEventBase(entity::Entity* entity, const char* name, uint32_t arg0, uint32_t arg1, bool fired)
{
    (void)entity; (void)name; (void)arg0; (void)arg1; (void)fired;
}
void QueueEntityTransformCallback(entity::Entity* entity) { (void)entity; }

void DeleteAnimController(void* controller) { (void)controller; }
void AnimControllerUpdate(void* controller, float dt) { (void)controller; (void)dt; }
void AnimControllerStopAll(void* controller) { (void)controller; }
void AnimPrepareClip(void* controller, uint32_t clip) { (void)controller; (void)clip; }
void AnimStartClip(float blend, void* controller, void* clip, bool loop) { (void)blend; (void)controller; (void)clip; (void)loop; }
void AnimCrossfadeClip(float blend, void* playing, void* clip, bool loop) { (void)blend; (void)playing; (void)clip; (void)loop; }
bool AnimReachedEnd(void* playing) { (void)playing; return false; }

void ResetObjectPivot(ape::world::ApeObj* object) { (void)object; }

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

bool IsGameTimeFrozen() { return false; }
bool IsShooterSplineForcedToEnd() { return false; }

} // namespace camera
} // namespace sly
