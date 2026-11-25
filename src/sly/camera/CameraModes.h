// openTIT - The simpler camera modes (anim, fixed, ortho, shooter).
#pragma once

#include "sly/camera/CameraMode.h"
#include "sly/camera/CameraSprings.h"
#include "sly/entity/EntityInterfaces.h"

namespace ape::world { class ApeCameraDef; class ApeObj; class ApeSplineInstance; }

namespace sly {
namespace camera {

// Builds a camera basis looking along `forward` (normalised), keeping the
// world Y axis up unless the direction is (nearly) vertical, where Z is used.
// The translation of `out` is left alone (0x000FD040).
void LookAlong(const ape::Vec3& forward, ape::Matrix34& out);
// Turns the direction `from` toward `to` by at most `maxAngle` radians
// (clamped to 0..pi); the result is a unit vector (0x001007D0). Used by the
// cameras, the characters and the vehicles.
void RotateTowards(float maxAngle, ape::Vec3& out, const ape::Vec3& from, const ape::Vec3& to);
// Keeps the direction `dir` between two pitch angles (radians, either order,
// limited to +-89 degrees) by turning it away from straight up and straight
// down (0x00101958).
void ClampDirectionPitch(float pitchA, float pitchB, ape::Vec3& out, const ape::Vec3& dir);
// Point of the segment [a, b] closest to `point` (0x000FD1A0).
ape::Vec3 ClosestPointOnSegment(const ape::Vec3& a, const ape::Vec3& b, const ape::Vec3& point);

// "Anim" (type 5, vtable 0x00B13AE0, 0x0C bytes): shows the camera's own
// animated world object (an ApeCameraInstance driven by a cutscene).
class AnimCameraMode : public CameraMode {
public:
    explicit AnimCameraMode(GameCamera* owner) : CameraMode(owner) {}
    // @func 0x00748A88 AnimCameraMode::~AnimCameraMode
    // @func 0x007494C8 AnimCameraMode::~AnimCameraMode (deleting)
    ~AnimCameraMode() override = default;
    // @func 0x00749478 AnimCameraMode::GetType
    int32_t GetType() const override { return 5; }
    // @func 0x00749480 AnimCameraMode::GetName
    const char* GetName() const override { return "Anim"; }
    void Update(float dt, CameraViewState& view) override;
    // The view is taken again once the animation has moved this frame.
    // @func 0x0074948C AnimCameraMode::PostUpdate
    void PostUpdate(float dt, CameraViewState& view) override { Update(dt, view); }

    bool useAnimFov = true;   // 0x09
};

// "Fixed" (type 3, vtable 0x00B13B38, 0x70 bytes): a placed view, optionally
// turned toward a target entity. Built by camera triggers.
class FixedCameraMode : public CameraMode {
public:
    explicit FixedCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~FixedCameraMode() override;
    // @func 0x0074961C FixedCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Fixed; }
    // @func 0x00749624 FixedCameraMode::GetName
    const char* GetName() const override { return "Fixed"; }
    void Update(float dt, CameraViewState& view) override;
    // @func 0x00749630 FixedCameraMode::GetTarget
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
    // @func 0x0074986C OrthoCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Ortho; }
    // @func 0x00749874 OrthoCameraMode::GetName
    const char* GetName() const override { return "Ortho"; }
    void Update(float dt, CameraViewState& view) override;
    // @func 0x00749880 OrthoCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
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
    // @func 0x00749858 ShooterCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Shooter; }
    // @func 0x00749860 ShooterCameraMode::GetName
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
    // @func 0x00748C1C CrawlCameraMode::~CrawlCameraMode
    // @func 0x00749780 CrawlCameraMode::~CrawlCameraMode (deleting)
    ~CrawlCameraMode() override = default;
    // @func 0x0074976C CrawlCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Crawl; }
    // @func 0x00749774 CrawlCameraMode::GetName
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

// "Spotlight" (type 8, vtable 0x00B13EB8, 0xE0 bytes): the player steers a
// searchlight entity with the right stick and the camera rides on it.
// NOTE: the constructor (0x00111FE0) has undecodable instruction data in
// the reference and is not ported.
class SpotlightCameraMode : public CameraMode {
public:
    explicit SpotlightCameraMode(GameCamera* owner) : CameraMode(owner) {}
    // Puts the searchlight back where it was.
    ~SpotlightCameraMode() override;
    // @func 0x0074983C SpotlightCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Spotlight; }
    // @func 0x00749844 SpotlightCameraMode::GetName
    const char* GetName() const override { return "Spotlight"; }
    void Update(float dt, CameraViewState& view) override;
    // Stick steering (slot 5).
    void UpdateStage2(float dt, CameraViewState& view) override;
    // @func 0x00749850 SpotlightCameraMode::GetTarget
    Entity* GetTarget() const override { return field_C0; }

    Entity* player = nullptr;          // 0x0C
    Entity* spotlight = nullptr;       // 0x10
    ape::Matrix34 savedSpotlight = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };   // 0x20
    ape::Vec3 field_60{};              // 0x60
    ape::Matrix34 transform = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };        // 0x70
    float pitchSpeed = 0.0f;           // 0xB0
    float yawSpeed = 0.0f;             // 0xB4
    Entity* field_C0 = nullptr;        // 0xC0 reported as the target. NOTE: role unknown
};

// "Default" (type 11, vtable 0x00B13B90 + 0x00B13BE4 at +0x0C, 0x410 bytes;
// it mixes in the entities' +0x17C listener interface at +0x0C):
// the follow camera of the player characters. It keeps two sets of
// CameraSettings (current and target, blended), a stack of setting
// overrides pushed by character states and triggers, and Havok probe
// bodies/phantoms for collision.
// NOTE: the constructor (0x000FF0D8) and the update stages are not ported yet.
class DefaultCameraMode : public CameraMode, public entity::EntityInterface17C {
public:
    static constexpr int kMaxOverrides = 17;

    explicit DefaultCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~DefaultCameraMode() override;
    // @func 0x00749638 DefaultCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Default; }
    // @func 0x00749640 DefaultCameraMode::GetName
    const char* GetName() const override { return "Default"; }
    void Deactivate() override;
    void ResetSettings() override;
    bool Tick(float dt, CameraViewState& view) override;
    const char* GetDebugText() override;
    // @func 0x0074964C DefaultCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
    // Slot 17: empty.
    // @func 0x001066FC DefaultCameraMode::Slot17
    virtual void Slot17() {}
    // Slot 18, also reached through the listener interface (0x00106B80).
    void OnStep(const void* info) override;

    // Follows `entity` and takes its camera settings (0x000FEEBC).
    void SetTarget(Entity* entity);
    // Target settings from the followed character or vehicle definition, or
    // the tuning defaults (0x000FEACC).
    void LoadTargetSettings();
    // Gives the probe bodies and phantoms the target's collision group (0x000FE828).
    void UpdateCollisionFilter();
    // Secondary target (lock-on): switches the target field of view (0x00106700).
    void SetSecondaryTarget(Entity* entity);
    // Settings overrides; entries flagged keepOnTop (CameraSettings::field_78)
    // stay above unflagged ones (0x000FEF54 / 0x001071CC). The bottom entry is
    // never removed.
    void PushOverride(CameraSettings* settings, bool keepOnTop);
    CameraSettings* RemoveOverride(CameraSettings* settings);
    // The target settings with every override applied in stack order (0x000FE3E8).
    void ResolveSettings(CameraSettings& out) const;
    // 0x001071C4
    void SetField214(bool value) { field_214 = value; }

    Entity* target = nullptr;            // 0x10 counted
    void* probeBodyA = nullptr;          // 0x14 Havok entity
    void* probeBodyB = nullptr;          // 0x18 Havok entity
    void* probeShape = nullptr;          // 0x1C
    uint32_t collisionFilter = 0;        // 0x30
    float field_A4 = 0.0f;               // 0xA4 cleared after every tick
    bool field_C1 = false;               // 0xC1 cleared after every tick
    void* phantomA = nullptr;            // 0x110
    Entity* secondaryTarget = nullptr;   // 0x1B8 (not counted)
    float angleInterp = 0.0f;            // 0x1F0
    bool field_214 = false;              // 0x214
    int32_t overrideCount = 0;           // 0x218
    CameraSettings* overrides[kMaxOverrides] = {};   // 0x21C
    CameraSettings current;              // 0x260
    CameraSettings targetSettings;       // 0x2E0
    SpringFloat dtSpring;                // 0x360 smooths the frame time
    float smoothingTime = 0.0f;          // 0x380
    SpringVec3 field_390;                // 0x390
    void* phantomB = nullptr;            // 0x404
};

// Settings of the over-the-shoulder camera (Camera.Shoulder.*).
struct ShoulderSettings {
    void LoadDefaults();               // 0x001072BC
    ape::Vec3 offset{};                // 0x00
    ape::Vec3 colOffset{};             // 0x10 collision probe position
    float fov = 0.0f;                  // 0x20
};

// "Shoulder" (type 14, vtable 0x00B13C18, 0x60 bytes): over the target's
// shoulder, with a sphere phantom probing for walls.
class ShoulderCameraMode : public CameraMode {
public:
    ShoulderCameraMode(GameCamera* owner, Entity* target);   // 0x00107320
    ~ShoulderCameraMode() override;
    // @func 0x00749654 ShoulderCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Shoulder; }
    // @func 0x0074965C ShoulderCameraMode::GetName
    const char* GetName() const override { return "Shoulder"; }
    void Update(float dt, CameraViewState& view) override;
    void Activate() override;
    void ActivateFrom(const CameraViewState& previous) override;
    bool Tick(float dt, CameraViewState& view) override;

    ShoulderSettings settings;         // 0x10
    Entity* target = nullptr;          // 0x40 (not counted)
    bool blocked = false;              // 0x44 NOTE: set while the probe hits something
    float blockTime = 0.0f;            // 0x48
    void* phantom = nullptr;           // 0x4C Havok sphere phantom
    bool justActivated = false;        // 0x50 cleared after the first tick
};

// "Spline" (type 2, vtable 0x00B13CF0, 0x150 bytes): moves along a position
// spline while looking at a target, both tunable per spline through tags.
// NOTE: construction (0x00108AF8) and the update (0x00109B38) are not ported yet.
class SplineCameraMode : public CameraMode {
public:
    explicit SplineCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~SplineCameraMode() override;
    // @func 0x00749714 SplineCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Spline; }
    // @func 0x0074971C SplineCameraMode::GetName
    const char* GetName() const override { return "Spline"; }
    void Finish() override;
    const char* GetDebugText() override;
    // @func 0x00749764 SplineCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
    // Starts at the spline point closest to the target (0x0010B0E4).
    void Activate() override;
    // Lets the player look around with the right stick (0x0010AD50).
    void UpdateStage2(float dt, CameraViewState& view) override;
    // Draws the splines (flag bit 0) (0x0010B348).
    void DebugDraw(uint32_t flags) override;
    // A blend-in starts the same way as a cut.
    // @func 0x00749728 SplineCameraMode::ActivateFrom
    void ActivateFrom(const CameraViewState& previous) override { (void)previous; Activate(); }
    // A null target also drops the look-at entity (0x00108A34).
    void SetTarget(Entity* entity);
    // 0x001089A8
    void SetLookAt(Entity* entity);

    Entity* target = nullptr;                          // 0x0C
    Entity* lookAt = nullptr;                          // 0x10
    ape::world::ApeObj* positionSpline = nullptr;      // 0x14
    ape::world::ApeObj* targetSpline = nullptr;        // 0x18
    float targetBlend = 0.0f;                          // 0x40 between the target spline and the target
    ape::Vec3 trackPoint{};                            // 0x50
    float t = 0.0f;                                    // 0x60
    float panH = 0.0f;                                 // 0x90
    float panV = 0.0f;                                 // 0x94
    float stickPitch = 0.0f;                           // 0xA0 degrees, from the right stick
    float stickYaw = 0.0f;                             // 0xA4 degrees
    // 0xB0: bit 31 stick panning locked, bit 30/28 stick panning allowed,
    // bit 29 placed.
    uint32_t splineFlags = 0;
};

// "Binocucom" (type 6, vtable 0x00B13DA0, 0x2B0 bytes): Bentley's binocucom
// view. NOTE: only the small members are ported.
class BinocucomCameraMode : public CameraMode {
public:
    explicit BinocucomCameraMode(GameCamera* owner) : CameraMode(owner) {}
    // Drops the photo component, hides the photo GUI and turns the player
    // to where the binocucom looked (0x0010C3C8).
    ~BinocucomCameraMode() override;
    // @func 0x007497C4 BinocucomCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Binocucom; }
    // @func 0x007497CC BinocucomCameraMode::GetName
    const char* GetName() const override { return "Binocucom"; }
    void Deactivate() override;
    // @func 0x007497D8 BinocucomCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
    // Becoming the top mode, by cut or blend, flags a restart of the view.
    // NOTE: meaning of zoomFlags bit 13 assumed.
    // @func 0x007497F0 BinocucomCameraMode::Activate
    void Activate() override { zoomFlags |= kZoom_Restart; }
    // @func 0x007497E0 BinocucomCameraMode::ActivateFrom
    void ActivateFrom(const CameraViewState& previous) override { (void)previous; zoomFlags |= kZoom_Restart; }
    // Counts requests to hold the zoom; returns true while any is held (0x0010D9B0).
    bool HoldZoom(bool hold);

    static constexpr uint16_t kZoom_Restart = 0x2000;
    Entity* player = nullptr;            // 0x0C
    ape::Vec3 lookDirection{};           // 0x50 horizontal look direction
    Entity* target = nullptr;            // 0x90 entity being looked at
    int16_t zoomHolds = 0;               // 0xB4
    uint16_t zoomFlags = 0;              // 0xB6 bits 14-15 cleared by a new hold
    float savedFov = 0.0f;               // 0xB8
    entity::EntityComponent* photoComponent = nullptr;   // 0xC0 added to the player
};

// Settings of the vehicle chase camera (Camera.Vehicle.*; vehicle definitions
// carry their own copy at +0x100). Console block of 0xA8 bytes.
struct VehicleChaseSettings {
    // From the tuning variables; the heading-return fields are left alone.
    void LoadTuningDefaults();

    float fov = 0.0f;                        // 0x00
    ape::Vec3 localLookAtOffset{};           // 0x10
    float horzInputRotScalar = 0.0f;         // 0x20
    float horzInputRotInterp = 0.0f;         // 0x24
    float lookAtXZInterp = 0.0f;             // 0x28
    float lookAtYUpInterp = 0.0f;            // 0x2C
    float lookAtYDownInterp = 0.0f;          // 0x30
    float gimbalRadius = 0.0f;               // 0x34
    float vertGimbalMin = 0.0f;              // 0x38
    float vertGimbalMax = 0.0f;              // 0x3C
    float vertGimbalRestShift = 0.0f;        // 0x40
    ape::Vec3 gimbalOffset{};                // 0x50
    float vertInputRotScalar = 0.0f;         // 0x60
    float vertInputRotAccelInterp = 0.0f;    // 0x64
    float vertInputRotVelInterp = 0.0f;      // 0x68
    float gimbalReturnInterp = 0.0f;         // 0x6C
    float colRadius = 0.0f;                  // 0x70
    float colFractHS = 0.0f;                 // 0x74
    float colInterp = 0.0f;                  // 0x78
    float colConeMin = 0.0f;                 // 0x7C
    float colMomentMax = 0.0f;               // 0x80
    float colRetract = 0.0f;                 // 0x84
    float headingReturnMinTimeDefault = 0.0f;   // 0x88
    float headingReturnMinTimeFwd = 0.0f;       // 0x8C
    float headingReturnMinTimeReverse = 0.0f;   // 0x90
    float headingReturnLowMaxRotSpeed = 0.0f;   // 0x94
    float headingReturnHighMaxRotSpeed = 0.0f;  // 0x98
    float headingReturnLowSin = 0.0f;           // 0x9C sine of the low angle
    float headingReturnHighSin = 0.0f;          // 0xA0 sine of the high angle
    float headingReturnFullSpeedElapsedTime = 0.0f;   // 0xA4
};
// Vehicle definition data the camera reads (definition +0x100, angles in
// degrees at +0x19C/+0x1A0 where the settings keep their sines).
struct VehicleCameraDesc {
    float fov;
    float pad04[3];
    ape::Vec3 localLookAtOffset;
    float values20[12];          // 0x20..0x4C
    ape::Vec3 gimbalOffset;      // 0x50
    float values60[15];          // 0x60..0x98
    float headingReturnLowAngle;    // 0x9C
    float headingReturnHighAngle;   // 0xA0
    float headingReturnFullSpeedElapsedTime;   // 0xA4
};

// "Vehicle" (type 7, vtable 0x00B13E00, 0x260 bytes): chase camera of the
// driving sections. NOTE: only teardown and the blend helpers are ported.
class VehicleCameraMode : public CameraMode {
public:
    explicit VehicleCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~VehicleCameraMode() override;
    // @func 0x00749800 VehicleCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Vehicle; }
    // @func 0x00749808 VehicleCameraMode::GetName
    const char* GetName() const override { return "Vehicle"; }
    // @func 0x00749814 VehicleCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
    // Follows `entity` and takes its camera settings (0x0010DD60).
    void SetTarget(Entity* entity);
    // Target settings from the vehicle definition, or the tuning defaults (0x0010DA30).
    void LoadTargetSettings();

    // Settings that are still easing are flagged in a bit set; these step one
    // flagged value toward its target and clear the flag once it arrived
    // (0x0010EB20 / 0x0010EBE8).
    static void EaseFlagged(float target, float rate, float dt, uint32_t* dirtyBits, uint32_t bit, float& value);
    static void EaseFlagged(float rate, float dt, uint32_t* dirtyBits, uint32_t bit, ape::Vec3& value,
                            const ape::Vec3& target);

    VehicleChaseSettings targetSettings;    // 0x10
    VehicleChaseSettings current;           // 0xB8
    Entity* target = nullptr;          // 0x250
    void* phantom = nullptr;           // 0x254
    void* body = nullptr;              // 0x258 NOTE: a Havok entity used for camera collision
};

// "Havok vehicle" (type 9, vtable 0x00B13E58, 0x110 bytes): uses Havok's
// vehicle follow camera (constructed in place at +0x10).
class HavokVehicleCameraMode : public CameraMode {
public:
    explicit HavokVehicleCameraMode(GameCamera* owner);
    ~HavokVehicleCameraMode() override;
    // @func 0x0074981C HavokVehicleCameraMode::GetType
    int32_t GetType() const override { return kCamMode_HavokVehicle; }
    // @func 0x00749824 HavokVehicleCameraMode::GetName
    const char* GetName() const override { return "Havok vehicle"; }
    // @func 0x00749834 HavokVehicleCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
    // Nothing to do in the second stage.
    // @func 0x00749830 HavokVehicleCameraMode::UpdateStage2
    void UpdateStage2(float dt, CameraViewState& view) override { (void)dt; (void)view; }
    // 0x0011135C
    void SetTarget(Entity* entity);

    Entity* target = nullptr;                       // 0x0C
    alignas(16) uint8_t followCamera[0xC0] = {};    // 0x10 Havok vehicle follow camera
    ape::Matrix34 transform = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };   // 0xD0
};

// "Track" (type 15, vtable 0x00B13FC8, 0xA0 bytes): follows a target at an
// offset. NOTE: the update (0x001139C0) is not ported yet.
class TrackCameraMode : public CameraMode {
public:
    explicit TrackCameraMode(GameCamera* owner) : CameraMode(owner) {}
    ~TrackCameraMode() override;
    // @func 0x00749998 TrackCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Track; }
    // @func 0x007499A0 TrackCameraMode::GetName
    const char* GetName() const override { return "Track"; }
    void Update(float dt, CameraViewState& view) override;
    const char* GetDebugText() override;
    // @func 0x007499AC TrackCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
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
    // @func 0x007499B4 OrbitCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Orbit; }
    // @func 0x007499BC OrbitCameraMode::GetName
    const char* GetName() const override { return "Orbit"; }
    void Update(float dt, CameraViewState& view) override;
    // @func 0x007499C8 OrbitCameraMode::GetTarget
    Entity* GetTarget() const override { return target; }
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
    // @func 0x007499D0 StrafeCameraMode::GetType
    int32_t GetType() const override { return kCamMode_Strafe; }
    // @func 0x007499D8 StrafeCameraMode::GetName
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
