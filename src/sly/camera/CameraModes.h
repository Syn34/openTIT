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
    void Finish() override;
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

// "Crawl" (type 10, vtable 0x00B13D48, 0x80 bytes): the close camera of the
// crawl spaces. Shows a placed view with a shallow depth of field and lets the
// right stick pan it; left alone for a while it glances around on its own.
class CrawlCameraMode : public CameraMode {
public:
    explicit CrawlCameraMode(GameCamera* owner) : CameraMode(owner) {}
    int32_t GetType() const override { return kCamMode_Crawl; }
    const char* GetName() const override { return "Crawl"; }
    void Update(float dt, CameraViewState& view) override;
    void UpdateStage2(float dt, CameraViewState& view) override;
    // Pulls the near plane in while active and restores it afterwards.
    void Activate() override;
    void Deactivate() override;
    void Finish() override;

    // Pan states (flags bits 29-30).
    enum PanState : uint8_t { kPan_Stick = 0, kPan_Glance = 1 };

    ape::Matrix34 transform{};       // 0x10
    float panTargetPitch = 0.0f;     // 0x50 degrees
    float panTargetYaw = 0.0f;       // 0x54 degrees
    float panPitch = 0.0f;           // 0x58
    float panYaw = 0.0f;             // 0x5C
    Entity* player = nullptr;        // 0x60
    float savedFov = 0.0f;           // 0x64 put back on the camera when deactivated
    float savedNear = 0.0f;          // 0x68 near plane of the main view before activation
    float idleLimit = 0.0f;          // 0x6C
    float idleTime = 0.0f;           // 0x70
    bool lookLocked = false;         // 0x74 bit 31 (toggled by the pad)
    PanState panState = kPan_Stick;  // 0x74 bits 29-30
};

// "Track" (type 15, vtable 0x00B13FC8, 0xA0 bytes): follows a target at an
// offset. NOTE: the update (0x001139C0) is not ported yet.
class TrackCameraMode : public CameraMode {
public:
    explicit TrackCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~TrackCameraMode() override;
    int32_t GetType() const override { return kCamMode_Track; }
    const char* GetName() const override { return "Track"; }
    void Update(float dt, CameraViewState& view) override;
    const char* GetDebugText() override;
    // 0x001138D8
    void SetTarget(Entity* entity);

    ape::Vec3 offsetPos{};          // 0x60
    float offsetYaw = 0.0f;         // 0x78 radians
    float offsetPitch = 0.0f;       // 0x7C radians
    Entity* target = nullptr;       // 0x88
    bool invertDir = false;         // 0x94 bit 31
};

// Orbit camera styles (tuning "Camera.Orbit.Mode.<style>.*").
enum OrbitStyle : uint8_t { kOrbit_Standard = 0, kOrbit_Sprint = 1 };

// "Orbit" (type 16, vtable 0x00B14020, 0xB0 bytes): orbits a target under
// stick control. NOTE: the update (0x00114650) is not ported yet.
class OrbitCameraMode : public CameraMode {
public:
    OrbitCameraMode(GameCamera* owner, Entity* target, OrbitStyle style);   // 0x00114298
    ~OrbitCameraMode() override;
    int32_t GetType() const override { return kCamMode_Orbit; }
    const char* GetName() const override { return "Orbit"; }
    void Update(float dt, CameraViewState& view) override;
    // Snaps to the target on the next update (0x001158E4).
    void Activate() override { snap = true; }
    // 0x00114208
    void SetTarget(Entity* entity);

    Entity* target = nullptr;            // 0x0C
    Entity* field_10 = nullptr;          // 0x10
    ape::Vec3 field_20{};                // 0x20
    float field_30[2] = {};              // 0x30
    float field_38[2] = {};              // 0x38
    float field_40 = 0.0f;               // 0x40
    float field_44 = 0.0f;               // 0x44
    ape::Vec3 gimbalLimit{};             // 0x60 radians (min pitch, max pitch, default pitch)
    float yaw = 0.0f;                    // 0x70 radians, starts at the target's heading
    float pitch = 0.0f;                  // 0x74 radians
    float field_78[2] = {};              // 0x78
    float field_80[2] = {};              // 0x80
    ape::Vec3 gimbalOffset{};            // 0x90
    float radius = 0.0f;                 // 0xA0
    float fov = 0.0f;                    // 0xA4
    bool snap = true;                    // 0xA8 bit 31
    OrbitStyle style = kOrbit_Standard;  // 0xA8 bits 29-30
};

// Blended settings of the strafe camera (console block of 0x40 bytes).
struct StrafeSettings {
    void LoadDefaults(float height);
    // Eases toward `target` at the target's rates (0x00115900).
    void BlendToward(float dt, const StrafeSettings& target);

    float interp = 0.0f;          // 0x00
    float fovInterp = 0.0f;       // 0x04
    float fov = 0.0f;             // 0x08
    ape::Vec3 posAdjust{};        // 0x10
    float gimbalRadius = 0.0f;    // 0x20
    float angleHome = 0.0f;       // 0x24
    float angleMin = 0.0f;        // 0x28
    float angleMax = 0.0f;        // 0x2C
    float height = 0.0f;          // 0x30 NOTE: camera height, from the creator
};

// "Strafe" (type 17, vtable 0x00B14078, 0xE0 bytes): keeps two entities in
// view side by side. NOTE: the update (0x00115FB0) has undecodable
// instruction data in the reference and is not ported yet.
class StrafeCameraMode : public CameraMode {
public:
    StrafeCameraMode(float height, GameCamera* owner, Entity* subject, Entity* opponent);   // 0x00115B90
    ~StrafeCameraMode() override;
    int32_t GetType() const override { return kCamMode_Strafe; }
    const char* GetName() const override { return "Strafe"; }
    void Update(float dt, CameraViewState& view) override;
    bool Tick(float dt, CameraViewState& view) override;
    // Slot 17 (strafe only); empty.
    // @func 0x00116434 StrafeCameraMode::Slot17
    virtual void Slot17() {}
    // 0x00115B0C
    void SetSubject(Entity* entity);

    Entity* subject = nullptr;           // 0x0C
    Entity* opponent = nullptr;          // 0x10
    ape::Matrix34 transform = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };   // 0x20
    StrafeSettings current;              // 0x60
    StrafeSettings target;               // 0xA0
};

} // namespace camera
} // namespace sly
