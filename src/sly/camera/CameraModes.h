// openTIT - The simpler camera modes (anim, fixed, ortho, shooter).
#pragma once

#include "sly/camera/CameraMode.h"

namespace ape::world { class ApeCameraDef; class ApeSplineInstance; }

namespace sly {
namespace camera {

// Builds a camera basis looking along `forward` (normalised), keeping the
// world Y axis up unless the direction is (nearly) vertical, where Z is used.
// The translation of `out` is left alone (0x000FD040).
void LookAlong(const ape::Vec3& forward, ape::Matrix34& out);
// Point of the segment [a, b] closest to `point` (0x000FD1A0).
ape::Vec3 ClosestPointOnSegment(const ape::Vec3& a, const ape::Vec3& b, const ape::Vec3& point);

// "Anim" (type 5, vtable 0x00B13AE0, 0x0C bytes): shows the camera's own
// animated world object (an ApeCameraInstance driven by a cutscene).
class AnimCameraMode : public CameraMode {
public:
    explicit AnimCameraMode(GameCamera* owner) : CameraMode(owner) {}
    int32_t GetType() const override { return 5; }
    const char* GetName() const override { return "Anim"; }
    void Update(float dt, CameraViewState& view) override;

    bool useAnimFov = true;   // 0x09
};

// "Fixed" (type 3, vtable 0x00B13B38, 0x70 bytes): a placed view, optionally
// turned toward a target entity. Built by camera triggers.
class FixedCameraMode : public CameraMode {
public:
    explicit FixedCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~FixedCameraMode() override;
    int32_t GetType() const override { return kCamMode_Fixed; }
    const char* GetName() const override { return "Fixed"; }
    void Update(float dt, CameraViewState& view) override;
    Entity* GetTarget() const override { return target; }

    ape::Matrix34 transform{};       // 0x10
    float fov = -1.0f;               // 0x50 negative: keep the current one
    Entity* target = nullptr;        // 0x54 counted reference
    bool dofEnabled = false;         // 0x58 bit 31
    uint8_t dofIterations = 0;       // 0x58 bits 23-30
    float dofBlendAlpha = 0.0f;      // 0x5C
    float dofRange = 0.0f;           // 0x60
    float focusDistance = 0.0f;      // 0x64
};

// "Ortho" (type 13, vtable 0x00B13F70, 0x80 bytes): a fixed offset from a
// vehicle, oriented by the vehicle definition's ortho camera block.
class OrthoCameraMode : public CameraMode {
public:
    // 0x00113608
    OrthoCameraMode(float fov, GameCamera* owner, Entity* target);
    ~OrthoCameraMode() override;
    int32_t GetType() const override { return kCamMode_Ortho; }
    const char* GetName() const override { return "Ortho"; }
    void Update(float dt, CameraViewState& view) override;
    // Replaces the followed entity (0x001138D8 is the Track mode's copy).
    void SetTarget(Entity* entity);
    // Reads offset, up vector and field of view from the target (0x001134D8).
    void LoadFromTarget();

    ape::Matrix34 transform{};                  // 0x10
    float fov = -1.0f;                          // 0x50
    Entity* target = nullptr;                   // 0x54
    ape::Vec3 offset{ 0.0f, 2.0f, 0.0f, 0.0f }; // 0x60 from the target to the camera
    ape::Vec3 up{ 0.0f, 0.0f, 1.0f, 0.0f };     // 0x70
};

// "Shooter" (type 12, vtable 0x00B13F18, 0x70 bytes): the camera of the
// on-rails shooting sections, carried along a spline with the orientation of
// a placed camera definition.
class ShooterCameraMode : public CameraMode {
public:
    explicit ShooterCameraMode(GameCamera* owner);   // 0x00112BC8
    ~ShooterCameraMode() override = default;
    int32_t GetType() const override { return kCamMode_Shooter; }
    const char* GetName() const override { return "Shooter"; }
    void Update(float dt, CameraViewState& view) override;
    void Release() override;
    // Offset from the spline start to the camera definition (height only) (0x00113458).
    void ComputeOffset();

    ape::world::ApeSplineInstance* spline = nullptr;   // 0x0C
    ape::world::ApeCameraDef* cameraDef = nullptr;     // 0x10
    ape::Vec3 offset{};                  // 0x20
    float progress = 0.0f;               // 0x30 0..1 along the spline
    float field_34 = -1.0f;              // 0x34 NOTE: never read here
    float speed = 5.0f;                  // 0x38
    uint32_t segment = 0;                // 0x3C point index bracketing the progress
    ape::Vec3 aimOffset{};               // 0x40 used while following the game camera
    bool followCamera = false;           // 0x50
    float speedScale = 1.0f;             // 0x54
    float rewindStop = 0.0f;             // 0x58 rewinding ends at this progress
    bool rewinding = false;              // 0x5C
    float rampTimer = 0.0f;              // 0x60 speed ramps up while positive
    float rampTime = 5.0f;               // 0x64
};

} // namespace camera
} // namespace sly
