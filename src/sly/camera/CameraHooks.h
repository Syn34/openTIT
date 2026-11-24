// openTIT - Calls from the camera code into systems that are not ported yet.
//
// Each hook names the console function, member or virtual slot it stands
// for; CameraHooks.cpp holds TODO stubs with safe defaults.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; }
namespace agl { class AglPostProcEffect; class AglRenderTarget; }

namespace sly {
namespace entity { class Entity; }

namespace camera {

struct CameraSettings;

// -- entities -----------------------------------------------------------------
// Entity name (virtual slot 19, +0x4C).
const char* EntityName(entity::Entity* entity);
// Position the entity wants to be looked at from (virtual slot 39, +0x9C).
ape::Vec3 EntityFocusPoint(entity::Entity* entity);
// Entity virtual slot 21 (+0x54): takes a new world transform.
void EntitySetWorldTransform(entity::Entity* entity, const ape::Matrix34& transform);
// Camera offset in a searchlight definition (+0x140) (0x001EB0BC).
ape::Vec3 SpotlightCameraOffset(entity::Entity* spotlight);
// Type word 0 bit 3 (+0xC0): a character.
bool IsCharacterEntity(const entity::Entity* entity);
// Type word 2 bit 10 (+0xC8): a vehicle. NOTE: named from the definition data it selects.
bool IsVehicleEntity(const entity::Entity* entity);
// Bit 31 of word +0x28 of the entity's component of type 3 (false without one).
// NOTE: meaning unknown; the crawl camera treats it like stick input.
bool Component03Flag31(entity::Entity* entity);
// Camera settings in a character definition (+0x3F0) or a vehicle definition (+0x1B0).
const CameraSettings* CharacterCameraSettings(entity::Entity* character);
const CameraSettings* VehicleCameraSettings(entity::Entity* vehicle);

// Ortho camera block of a vehicle definition (+0x290 offset, +0x2A0 up, +0x2B0 fov).
struct OrthoCameraDesc {
    ape::Vec3 offset;
    ape::Vec3 up;
    float fov;
};
const OrthoCameraDesc* VehicleOrthoCamera(entity::Entity* vehicle);

// -- world objects ------------------------------------------------------------
// Field of view of an ApeCameraInstance (+0x154).
float CameraInstanceFov(ape::world::ApeObj* camera);

// Base version of entity virtual slot 59 (0x000DD368), animation events.
void EntityAnimEventBase(entity::Entity* entity, const char* name, uint32_t arg0, uint32_t arg1, bool fired);
// Queues the entity's transform callback (0x000DD778) on the game world's
// phase-0 callback list (0x0046B5F8).
void QueueEntityTransformCallback(entity::Entity* entity);

// -- animation controllers ------------------------------------------------------
void DeleteAnimController(void* controller);                    // virtual deleting destructor
void AnimControllerUpdate(void* controller, float dt);           // 0x0034C918
void AnimControllerStopAll(void* controller);                    // 0x0034A1FC (with 0)
// Makes sure the clip is resident before it is started (0x00350764 with 1, 1).
void AnimPrepareClip(void* controller, uint32_t clip);
// Starts a clip on layer 0 (0x003514AC) or cross-fades the playing one (0x00350958).
void AnimStartClip(float blend, void* controller, void* clip, bool loop);
void AnimCrossfadeClip(float blend, void* playing, void* clip, bool loop);
// True once a playing (non-looping) clip has reached its end (0x00353EB0 with 0).
// NOTE: EntityHooks.h names the same function AnimStop.
bool AnimReachedEnd(void* playing);

// -- world objects ------------------------------------------------------------
// Puts the identity into the object's animated pivot (+0xA0) unless it uses the
// shared default (0x01193010), and marks it dirty.
void ResetObjectPivot(ape::world::ApeObj* object);

class BinocucomCameraMode;
// Re-reads the binocucom's focus entity (0x0010DA30).
void BinocucomRefreshFocus(BinocucomCameraMode* mode);

// -- physics ------------------------------------------------------------------------
// Creates a sphere phantom of `radius` with the collision filter word and adds
// it to the game physics world (hkpSphereShape 0x0089FE70, hkpSimpleShapePhantom
// 0x0092A718, addPhantom 0x00931D10).
void* CreateSpherePhantom(float radius, uint32_t filterInfo);
// Takes a phantom out of its world (0x00931F00) and drops the reference (0x007D4EA8).
void DestroyPhantom(void* phantom);
// Removes a Havok entity from its world (0x0092FFC8 via 0x009D19C8) and drops it.
void DestroyHavokEntity(void* entity);
// Collision group of a character (skin component's shape filter word >> 16), 0 otherwise.
uint32_t CharacterCollisionGroup(entity::Entity* entity);
// Havok vehicle follow camera constructed in place (0x009F6BB0 / 0x009F6A60).
void ConstructVehicleFollowCamera(void* storage);
void DestroyVehicleFollowCamera(void* storage);

// -- render ---------------------------------------------------------------------
// Parameters of the depth-of-field effect (+0x18 focus, +0x1C range, +0x20
// blend, byte +0x40 iterations) and of the desaturation/colour filter effect
// (+0x14 blend, +0x20..+0x28 colour).
void SetDofEffectParams(agl::AglPostProcEffect* effect, float focus, float range, float blend, uint8_t iterations);
void SetFilterEffectParams(agl::AglPostProcEffect* effect, float blend, const uint8_t color[4]);
// Depth of field forced on by the binocucom (world +0x1281C0, byte +0x301) or
// by a world flag (world +0x128304 bit 29).
bool BinocucomForcesDof();
bool WorldForcesDof();
// Back buffer size (0x010C5A24 / 0x010C5A26) and the TV safe area
// (0x010B1850 / 0x010B1854; negative when not set).
float ScreenWidth();
float ScreenHeight();
float SafeAreaWidth();
float SafeAreaHeight();
// The display is 4:3 (0x0046C7B0 on the game world).
bool IsDisplay4x3();

// Render target of the main view (game world +0x128168).
agl::AglRenderTarget* MainRenderTarget();

// -- input ------------------------------------------------------------------------
// Pad of the player slot that controls `entity` (the entity's player byte
// +0xAC indexes the world player block, whose pad index picks one of the
// 0x2EC0-byte pad blocks at 0x010B1A90).
void* PlayerPad(entity::Entity* entity);
// Stick axis 0..3 (left X/Y, right X/Y) in -1..1 (0x0045B160).
float PadAxis(void* pad, int axis);
// The button bits were pressed this frame (pad +0x08 & +0x0C).
bool PadButtonPressed(void* pad, uint32_t mask);

// -- game state -------------------------------------------------------------------
// Frame time is frozen (short at +0x128214 of the world object 0x010B19E0).
bool IsGameTimeFrozen();
// NOTE: the level-state object at 0x00F64C20 +0x48 has state 0x17538BF3 at
// +0xA00; the shooter camera then jumps to the end of its spline.
bool IsShooterSplineForcedToEnd();

} // namespace camera
} // namespace sly
