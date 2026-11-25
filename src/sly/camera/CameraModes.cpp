// openTIT - The simpler camera modes (see CameraModes.h).
#include "sly/camera/CameraModes.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameEntityHooks.h"
#include "ape/level/ApeGui.h"

#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "agl/AglRenderTarget.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "crt/libc/Libc.h"
#include "sly/camera/CameraHooks.h"
#include "sly/game/GameMath.h"
#include "sly/entity/GameCamera.h"
#include "sly/tuning/CameraTuning.h"

#include <cmath>

namespace sly {
namespace camera {

namespace {

ape::Vec3 Cross(const ape::Vec3& a, const ape::Vec3& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f };
}

ape::Vec3 Normalized(const ape::Vec3& v)
{
    const float inv = 1.0f / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x * inv, v.y * inv, v.z * inv, 0.0f };
}

void SetXYZ(ape::Vec3& out, const ape::Vec3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
}

// Takes a counted reference on `entity` for `slot`, dropping the old one.
void AssignEntity(Entity*& slot, Entity* entity)
{
    if (slot)
        slot->Release(false);
    slot = entity;
    if (entity)
        entity->AddRef();
}

} // namespace

// @func 0x000FD040 LookAlong
void LookAlong(const ape::Vec3& forward, ape::Matrix34& out)
{
    const float lenSq = forward.x * forward.x + forward.y * forward.y + forward.z * forward.z;
    if (lenSq < 1e-05f)
        return;
    const ape::Vec3 z = Normalized(forward);
    const ape::Vec3 worldY = { 0.0f, 1.0f, 0.0f, 0.0f };
    const ape::Vec3 worldZ = { 0.0f, 0.0f, 1.0f, 0.0f };
    const ape::Vec3& up = std::fabs(z.y) >= 0.999f ? worldZ : worldY;
    const ape::Vec3 x = Normalized(Cross(up, z));
    SetXYZ(out.xAxis, x);
    SetXYZ(out.yAxis, Cross(z, x));
    SetXYZ(out.zAxis, z);
}

// @func 0x000FD1A0 ClosestPointOnSegment
ape::Vec3 ClosestPointOnSegment(const ape::Vec3& a, const ape::Vec3& b, const ape::Vec3& point)
{
    ape::Vec3 dir = { b.x - a.x, b.y - a.y, b.z - a.z, 0.0f };
    const float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length >= 1e-05f) {
        dir = { dir.x / length, dir.y / length, dir.z / length, 0.0f };
        const float fromA = dir.x * (point.x - a.x) + dir.y * (point.y - a.y) + dir.z * (point.z - a.z);
        if (fromA > 0.0f) {
            const float fromB = dir.x * (point.x - b.x) + dir.y * (point.y - b.y) + dir.z * (point.z - b.z);
            if (fromB >= 0.0f)
                return { b.x, b.y, b.z, 0.0f };
            return { b.x + dir.x * fromB, b.y + dir.y * fromB, b.z + dir.z * fromB, 0.0f };
        }
    }
    return { a.x, a.y, a.z, 0.0f };
}

// ===========================================================================
// Anim
// ===========================================================================

// @func 0x000FD2D0 AnimCameraMode::Update
void AnimCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    if (camera->cameraFlags & entity::kCamFlag_AnimHold)
        return;
    ape::world::ApeObj* object = camera->object;
    if (!object || object->GetType() != ape::world::kObjType_CameraInstance)
        return;

    object->UpdateWorld(false, false);
    const ape::Matrix34& world = object->GetWorld();
    // The camera object uses the renderer's convention (x and z mirrored).
    view.transform.xAxis = { -world.xAxis.x, -world.xAxis.y, -world.xAxis.z, view.transform.xAxis.pad };
    view.transform.yAxis = { world.yAxis.x, world.yAxis.y, world.yAxis.z, view.transform.yAxis.pad };
    view.transform.zAxis = { -world.zAxis.x, -world.zAxis.y, -world.zAxis.z, view.transform.zAxis.pad };
    SetXYZ(view.transform.translation, world.translation);
    if (useAnimFov && CameraInstanceFov(object) > 0.0f)
        view.fov = CameraInstanceFov(object);
}

// ===========================================================================
// Fixed
// ===========================================================================

// @func 0x0074950C FixedCameraMode::~FixedCameraMode
// @func 0x00749588 FixedCameraMode::~FixedCameraMode (deleting)
FixedCameraMode::~FixedCameraMode()
{
    AssignEntity(target, nullptr);
}

// @func 0x000FD408 FixedCameraMode::Update
void FixedCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    if (fov >= 0.0f)
        view.fov = fov;
    view.dofBlendAlpha = dofBlendAlpha;
    view.dofRange = dofRange;
    view.dofEnabled = dofEnabled;
    view.focusDistance = focusDistance;
    view.dofIterations = dofIterations;

    ape::Matrix34 result = transform;
    if (target) {
        // Turn toward the target; the focus follows its distance.
        const ape::Vec3 aim = EntityFocusPoint(target);
        const ape::Vec3 pos = transform.translation;
        const ape::Vec3 dir = { aim.x - pos.x, aim.y - pos.y, aim.z - pos.z, 0.0f };
        const float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        view.focusDistance = distance;
        const ape::Vec3 z = { dir.x / distance, dir.y / distance, dir.z / distance, 0.0f };
        const ape::Vec3 x = Normalized(Cross({ 0.0f, 1.0f, 0.0f, 0.0f }, z));
        SetXYZ(result.xAxis, x);
        SetXYZ(result.yAxis, Cross(z, x));
        SetXYZ(result.zAxis, z);
    }
    SetXYZ(view.transform.xAxis, result.xAxis);
    SetXYZ(view.transform.yAxis, result.yAxis);
    SetXYZ(view.transform.zAxis, result.zAxis);
    SetXYZ(view.transform.translation, result.translation);
}

// ===========================================================================
// Ortho
// ===========================================================================

// @func 0x00113608 OrthoCameraMode::OrthoCameraMode
OrthoCameraMode::OrthoCameraMode(float fovDegrees, GameCamera* owner, Entity* entity)
    : CameraMode(owner), fov(fovDegrees)
{
    SetTarget(entity);
}

// @func 0x001136E0 OrthoCameraMode::~OrthoCameraMode
// @func 0x0011375C OrthoCameraMode::~OrthoCameraMode (deleting)
OrthoCameraMode::~OrthoCameraMode()
{
    AssignEntity(target, nullptr);
}

void OrthoCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
    if (entity)
        LoadFromTarget();
}

// @func 0x001134D8 OrthoCameraMode::LoadFromTarget
void OrthoCameraMode::LoadFromTarget()
{
    if (!target || !IsVehicleEntity(target))
        return;
    const OrthoCameraDesc* desc = VehicleOrthoCamera(target);
    if (!desc)
        return;
    offset = desc->offset;
    up = desc->up;
    fov = desc->fov;

    // Look back from the offset toward the target.
    const ape::Vec3 z = Normalized({ -offset.x, -offset.y, -offset.z, 0.0f });
    const ape::Vec3 x = Normalized(Cross(up, z));
    SetXYZ(transform.xAxis, x);
    SetXYZ(transform.yAxis, Cross(z, x));
    SetXYZ(transform.zAxis, z);
}

// @func 0x00113800 OrthoCameraMode::Update
void OrthoCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    if (target) {
        const ape::Vec3& pos = target->Position();
        transform.translation.x = pos.x + offset.x;
        transform.translation.y = pos.y + offset.y;
        transform.translation.z = pos.z + offset.z;
        SetXYZ(view.transform.xAxis, transform.xAxis);
        SetXYZ(view.transform.yAxis, transform.yAxis);
        SetXYZ(view.transform.zAxis, transform.zAxis);
        SetXYZ(view.transform.translation, transform.translation);
    }
    if (fov >= 0.0f)
        view.fov = fov;
}

// ===========================================================================
// Shooter
// ===========================================================================

// @func 0x00112BC8 ShooterCameraMode::ShooterCameraMode
ShooterCameraMode::ShooterCameraMode(GameCamera* owner) : CameraMode(owner) {}

// @func 0x00112C58 ShooterCameraMode::~ShooterCameraMode
// @func 0x00112C68 ShooterCameraMode::~ShooterCameraMode (deleting)
// (the destructor is defaulted in the header)

// @func 0x00113458 ShooterCameraMode::ComputeOffset
void ShooterCameraMode::ComputeOffset()
{
    if (!spline || !cameraDef)
        return;
    const ape::Vec3& from = spline->GetWorld().translation;
    const ape::Vec3& to = cameraDef->m_transform.translation;
    // Only the height difference is kept.
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = to.z - from.z;
}

// @func 0x00112CC8 ShooterCameraMode::Update
void ShooterCameraMode::Update(float dt, CameraViewState& view)
{
    if (followCamera) {
        // Ride along with the game camera, drifting toward the aim offset.
        GameCamera* owner = camera;
        view.dofEnabled = false;
        view.dofRange = 0.0f;
        view.dofBlendAlpha = 0.0f;
        view.dofIterations = 0;
        view.filterEnabled = false;
        view.filterBlend = 0.0f;
        view.filterColor[0] = 0;
        view.filterColor[1] = 0;
        view.filterColor[2] = 0;
        view.filterColor[3] = 0xFF;
        view.focusDistance = owner->focusDistance;
        view.fov = IsDisplay4x3() ? g_cameraShooter43FOV.Get() : owner->fov;
        SetXYZ(view.transform.xAxis, owner->transform.xAxis);
        SetXYZ(view.transform.yAxis, owner->transform.yAxis);
        SetXYZ(view.transform.zAxis, owner->transform.zAxis);
        ape::Vec3& pos = view.transform.translation;
        pos = { owner->transform.translation.x, owner->transform.translation.y,
                owner->transform.translation.z, pos.pad };
        pos.z = pos.z * 0.9f + 0.6f;
        pos.x = (aimOffset.x - 2.0f) * 0.1f + pos.x * 0.9f;
        pos.y = aimOffset.y * 0.1f + pos.y * 0.9f;
        return;
    }

    if (IsGameTimeFrozen())
        dt = 0.0f;
    if (IsShooterSplineForcedToEnd())
        progress = 1.0f;
    if (!cameraDef)
        return;

    // Orientation from the camera definition (stored in the renderer's convention).
    const ape::Matrix34& placed = cameraDef->m_transform;
    view.transform.xAxis = { -placed.xAxis.x, -placed.xAxis.y, -placed.xAxis.z, view.transform.xAxis.pad };
    SetXYZ(view.transform.yAxis, placed.yAxis);
    view.transform.zAxis = { -placed.zAxis.x, -placed.zAxis.y, -placed.zAxis.z, view.transform.zAxis.pad };
    SetXYZ(view.transform.translation, placed.translation);
    view.fov = IsDisplay4x3() ? g_cameraShooter43FOV.Get() : cameraDef->m_fov;

    // Find the spline points around the current progress.
    const ape::ApeSpline& path = spline->GetSplineDef()->m_spline;
    const ape::SplinePoint* points = path.GetPoints();
    const uint32_t count = path.GetNumPoints();
    while (segment < count - 1 && points[segment].param <= progress)
        ++segment;
    while (segment != 0 && points[segment].param >= progress)
        --segment;

    // Move at `speed` metres per second along the current segment.
    const ape::SplinePoint& a = points[segment];
    const ape::SplinePoint& b = points[segment + 1];
    const float dx = a.position.x - b.position.x;
    const float dy = a.position.y - b.position.y;
    const float dz = a.position.z - b.position.z;
    float step = (b.param - a.param) * speed * speedScale * dt / std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!rewinding) {
        const float timer = rampTimer;
        if (timer > 0.0f) {
            rampTimer = timer - dt;
            step *= 1.0f - timer / rampTime;
        }
    } else {
        rampTimer = rampTime;
        step *= -5.0f;
    }

    progress = ape::Clamp(progress + step, 0.0f, 1.0f);
    if (progress == 0.0f)
        rewinding = false;

    ape::Vec3 pos;
    spline->EvaluatePosition(progress, pos, false);
    view.transform.translation.x = pos.x + offset.x;
    view.transform.translation.y = pos.y + offset.y;
    view.transform.translation.z = pos.z + offset.z;
    if (rewinding && progress <= rewindStop)
        rewinding = false;
}

// @func 0x00113350 ShooterCameraMode::Finish
void ShooterCameraMode::Finish()
{
    GameCamera* owner = camera;
    CameraMode* below = owner->ModeBelow(this);
    owner->RemoveMode(-1.0f, this);
    // NOTE: the mode underneath is told to finish as well.
    if (below)
        below->Finish();
    delete this;
}

// ===========================================================================
// Crawl
// ===========================================================================

// @func 0x0010B6A8 CrawlCameraMode::Update
void CrawlCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    SetXYZ(view.transform.xAxis, transform.xAxis);
    SetXYZ(view.transform.yAxis, transform.yAxis);
    SetXYZ(view.transform.zAxis, transform.zAxis);
    SetXYZ(view.transform.translation, transform.translation);
    if (g_cameraCrawlFOV >= 0.0f)
        view.fov = g_cameraCrawlFOV;
    view.dofBlendAlpha = g_cameraCrawlDOFIntensity;
    view.dofEnabled = g_cameraCrawlDOFEnable;
    view.dofRange = g_cameraCrawlDOFRange;
    view.dofIterations = 1;
    view.focusDistance = g_cameraCrawlDOFFocalDist;
}

// @func 0x0010B790 CrawlCameraMode::UpdateStage2
void CrawlCameraMode::UpdateStage2(float dt, CameraViewState& view)
{
    if (!player)
        return;
    void* pad = PlayerPad(player);
    constexpr uint32_t kLookToggleButton = 0x2000;
    if (PadButtonPressed(pad, kLookToggleButton))
        lookLocked = !lookLocked;
    const float moveX = PadAxis(pad, 0);
    const float moveY = -PadAxis(pad, 1);
    const float lookX = PadAxis(pad, 2);
    const float lookY = -PadAxis(pad, 3);

    // Any input (or a busy character) resets the idle timer.
    const float* idleRange = g_cameraCrawlIdleTimeMinMax.Get();
    if (lookLocked || moveX * moveX + moveY * moveY >= 0.0625f || lookX * lookX + lookY * lookY >= 0.0625f ||
        Component03Flag31(player)) {
        panState = kPan_Stick;
        idleTime = 0.0f;
        idleLimit = ape::RandomRange(idleRange[0], idleRange[1], ape::g_gameRandom);
    } else {
        idleTime += dt;
    }

    const float* panMax = g_cameraCrawlPanMax.Get();
    if (idleLimit <= idleTime) {
        // Glance somewhere at random for a while.
        const float range = g_cameraCrawlLookRandMax;
        panTargetYaw = ape::RandomRange(-range, range, ape::g_gameRandom) * panMax[0];
        panTargetPitch = ape::RandomRange(-range, range, ape::g_gameRandom) * panMax[1];
        idleTime = 0.0f;
        panState = kPan_Glance;
        const float* lookTime = g_cameraCrawlLookTimeMinMax.Get();
        idleLimit = ape::RandomRange(lookTime[0], lookTime[1], ape::g_gameRandom);
    } else if (panState == kPan_Stick && !lookLocked) {
        panTargetYaw = lookX * panMax[0];
        panTargetPitch = lookY * panMax[1];
    }

    panPitch = ApproachExp(panPitch, panTargetPitch, g_cameraCrawlPanInterp, dt);
    panYaw = ApproachExp(panYaw, panTargetYaw, g_cameraCrawlPanInterp, dt);

    // Turn the view about its own position: yaw, then pitch.
    constexpr float kDegToRad = 0.017453292f;
    const ape::Vec3 pos = view.transform.translation;
    ape::Matrix34 turn;
    ape::Matrix34RotationY(-panYaw * kDegToRad, turn);
    ape::Matrix34 yawed;
    ape::Matrix34MulRotation(yawed, view.transform, turn);
    ape::Matrix34RotationX(-panPitch * kDegToRad, turn);
    ape::Matrix34MulRotation(view.transform, yawed, turn);
    SetXYZ(view.transform.translation, pos);
}

// @func 0x0010BCD8 CrawlCameraMode::Activate
void CrawlCameraMode::Activate()
{
    agl::AglRenderTarget* target = MainRenderTarget();
    savedNear = target->zNear;
    target->SetNear(0.02f);
}

// @func 0x0010BD28 CrawlCameraMode::Deactivate
void CrawlCameraMode::Deactivate()
{
    MainRenderTarget()->SetNear(savedNear);
    camera->fov = savedFov;
}

// @func 0x0010BD7C CrawlCameraMode::Finish
void CrawlCameraMode::Finish()
{
    savedNear = 0.0f;
}

// ===========================================================================
// Spotlight
// ===========================================================================

// @func 0x001125D8 SpotlightCameraMode::~SpotlightCameraMode
// @func 0x00112648 SpotlightCameraMode::~SpotlightCameraMode (deleting)
SpotlightCameraMode::~SpotlightCameraMode()
{
    EntitySetWorldTransform(spotlight, savedSpotlight);
}

// @func 0x001126D4 SpotlightCameraMode::Update
void SpotlightCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    SetXYZ(view.transform.xAxis, transform.xAxis);
    SetXYZ(view.transform.yAxis, transform.yAxis);
    SetXYZ(view.transform.zAxis, transform.zAxis);
    SetXYZ(view.transform.translation, transform.translation);

    // The light hangs below the camera by the offset in its definition.
    ape::Matrix34 light = transform;
    const ape::Vec3 offset = SpotlightCameraOffset(spotlight);
    light.translation.x -= transform.xAxis.x * offset.x + transform.yAxis.x * offset.y + transform.zAxis.x * offset.z;
    light.translation.y -= transform.xAxis.y * offset.x + transform.yAxis.y * offset.y + transform.zAxis.y * offset.z;
    light.translation.z -= transform.xAxis.z * offset.x + transform.yAxis.z * offset.y + transform.zAxis.z * offset.z;
    EntitySetWorldTransform(spotlight, light);
}

// @func 0x00112870 SpotlightCameraMode::UpdateStage2
void SpotlightCameraMode::UpdateStage2(float dt, CameraViewState& /*view*/)
{
    if (!(player->classBits & 1))   // only a player steers
        return;
    void* pad = PlayerPad(player);
    const float stickY = PadAxis(pad, 3);
    const float stickX = PadAxis(pad, 2);
    const float* accel = g_cameraSpotlightRotationalAcceleration.Get();
    const float* damping = g_cameraSpotlightRotationalDamping.Get();
    const float* maxSpeed = g_cameraSpotlightRotationalMaxSpeed.Get();

    // Squared stick response, then drag back toward rest, then a speed cap.
    pitchSpeed += accel[0] * (stickY * std::fabs(stickY)) * dt;
    pitchSpeed = ApproachExp(pitchSpeed, 0.0f, damping[0], dt);
    if (std::fabs(pitchSpeed) > maxSpeed[0])
        pitchSpeed *= maxSpeed[0] / std::fabs(pitchSpeed);
    yawSpeed -= accel[1] * (stickX * std::fabs(stickX)) * dt;
    yawSpeed = ApproachExp(yawSpeed, 0.0f, damping[1], dt);
    if (std::fabs(yawSpeed) > maxSpeed[1])
        yawSpeed *= maxSpeed[1] / std::fabs(yawSpeed);

    // Turn about the camera's own position.
    const ape::Vec3 pos = transform.translation;
    ape::Matrix34 turn;
    ape::Matrix34RotationY(yawSpeed * dt, turn);
    ape::Matrix34 yawed;
    ape::Matrix34MulRotation(yawed, transform, turn);
    ape::Matrix34RotationX(pitchSpeed * dt, turn);
    ape::Matrix34MulRotation(transform, yawed, turn);
    SetXYZ(transform.translation, pos);
}

// ===========================================================================
// Default
// ===========================================================================

// @func 0x001002C4 DefaultCameraMode::~DefaultCameraMode
// @func 0x00100470 DefaultCameraMode::~DefaultCameraMode (deleting)
// @func 0x00100504 DefaultCameraMode::~DefaultCameraMode (thunk)
// @func 0x001004B8 DefaultCameraMode::~DefaultCameraMode (deleting thunk)
DefaultCameraMode::~DefaultCameraMode()
{
    AssignEntity(target, nullptr);
    if (phantomB) {
        DestroyPhantom(phantomB);
        phantomB = nullptr;
    }
    if (probeShape) {
        HavokRemoveReference(probeShape);
        probeShape = nullptr;
    }
    if (phantomA)
        DestroyPhantom(phantomA);
    if (probeBodyB)
        DestroyHavokEntity(probeBodyB);
    if (probeBodyA)
        DestroyHavokEntity(probeBodyA);
}

// @func 0x000FE828 DefaultCameraMode::UpdateCollisionFilter
void DefaultCameraMode::UpdateCollisionFilter()
{
    if (!target)
        return;
    uint32_t filter;
    if (!EntityCollisionFilter(target, filter))
        return;
    // Same group as the target (so it never collides with it), layer 0x10.
    filter = (filter & 0xFFFF0000u) | 0x10;
    if (filter == collisionFilter)
        return;
    collisionFilter = filter;
    if (probeBodyA)
        SetEntityCollisionFilter(probeBodyA, filter);
    if (probeBodyB)
        SetEntityCollisionFilter(probeBodyB, filter);
    if (phantomA)
        SetPhantomCollisionFilter(phantomA, filter);
    if (phantomB)
        SetPhantomCollisionFilter(phantomB, filter);
}

// @func 0x000FEACC DefaultCameraMode::LoadTargetSettings
void DefaultCameraMode::LoadTargetSettings()
{
    const CameraSettings* source = nullptr;
    if (target) {
        if (IsCharacterEntity(target))
            source = CharacterCameraSettings(target);
        else if (IsVehicleEntity(target))
            source = VehicleCameraSettings(target);
    }
    if (source)
        targetSettings = *source;
    else
        targetSettings.LoadDefaults();
}

// @func 0x000FEEBC DefaultCameraMode::SetTarget
void DefaultCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
    if (entity) {
        LoadTargetSettings();
        UpdateCollisionFilter();
    }
}

// @func 0x000FEF54 DefaultCameraMode::PushOverride
void DefaultCameraMode::PushOverride(CameraSettings* settings, bool keepOnTop)
{
    settings->field_78 = keepOnTop;
    if (!keepOnTop && overrideCount != 0 && overrides[overrideCount - 1]->field_78) {
        // Slide in below the entries that stay on top.
        for (int i = overrideCount; i > 0; --i) {
            if (!overrides[i - 1]->field_78) {
                overrides[i] = settings;
                ++overrideCount;
                return;
            }
            overrides[i] = overrides[i - 1];
        }
        // NOTE: with every entry flagged the console drops the new one.
        return;
    }
    overrides[overrideCount++] = settings;
}

// @func 0x001071CC DefaultCameraMode::RemoveOverride
CameraSettings* DefaultCameraMode::RemoveOverride(CameraSettings* settings)
{
    if (overrideCount >= 2 && overrides[overrideCount - 1] == settings) {
        overrides[--overrideCount] = nullptr;
        return settings;
    }
    for (int i = overrideCount - 1; i > 0; --i) {
        if (overrides[i] != settings)
            continue;
        --overrideCount;
        for (int j = i; j < overrideCount; ++j)
            overrides[j] = overrides[j + 1];
        overrides[overrideCount] = nullptr;
        return settings;
    }
    return nullptr;
}

// @func 0x00106700 DefaultCameraMode::SetSecondaryTarget
void DefaultCameraMode::SetSecondaryTarget(Entity* entity)
{
    if (secondaryTarget != entity)
        targetSettings.fov = entity ? g_cameraDefaultTargetFOV : g_cameraDefaultFOV;
    secondaryTarget = entity;
}

// @func 0x001062B8 DefaultCameraMode::Deactivate
void DefaultCameraMode::Deactivate()
{
    if (probeShape) {
        HavokRemoveReference(probeShape);
        probeShape = nullptr;
    }
    if (probeBodyB)
        RemoveHavokEntityFromWorld(probeBodyB);
    if (probeBodyA)
        RemoveHavokEntityFromWorld(probeBodyA);
}

// @func 0x00106350 DefaultCameraMode::ResetSettings
void DefaultCameraMode::ResetSettings()
{
    targetSettings.LoadDefaults();
    const bool flag = targetSettings.field_78;
    const float extra = targetSettings.field_7C;
    current = targetSettings;
    current.field_78 = flag;
    current.field_7C = extra;
    angleInterp = g_cameraDefaultAngleInterp;
}

// @func 0x00100570 DefaultCameraMode::Tick
bool DefaultCameraMode::Tick(float dt, CameraViewState& view)
{
    if (std::fabs(dt) >= 1e-05f) {
        // Smooth the frame time, fading the smoothing in over five seconds.
        if (smoothingTime <= 0.1f) {
            dtSpring.target = dt;
            dtSpring.targetVelocity = 0.0f;
            dtSpring.value = dt;
            dtSpring.velocity = 0.0f;
        }
        smoothingTime += dt;
        dtSpring.timeLeft = 2.0f;
        dtSpring.Update(dt, dt);
        const float weight = ape::Saturate(smoothingTime / 5.0f);
        dt = (dtSpring.value - dt) * weight + dt;
    }
    UpdateCollisionFilter();
    Update(dt, view);
    UpdateStage2(dt, view);
    UpdateStage3(dt, view);

    // Optional roll.
    ape::Matrix34 roll;
    ape::Matrix34RotationZ(g_cameraDefaultRotationZ * 0.017453292f, roll);
    const ape::Matrix34 rolled = view.transform;
    ape::Matrix34MulRotationKeepTranslation(view.transform, rolled, roll);
    field_C1 = false;
    field_A4 = 0.0f;
    return true;
}

// @func 0x0010660C DefaultCameraMode::GetDebugText
const char* DefaultCameraMode::GetDebugText()
{
    return ape::TempFormat("Target: %s\nSec Tgt: %s\nGimbal Radius: %5.2f", target ? EntityName(target) : "<none>",
                           secondaryTarget ? EntityName(secondaryTarget) : "<none>", current.gimbalRadius);
}

// TODO(0x00106B80 DefaultCameraMode::OnStep)
void DefaultCameraMode::OnStep(const void* /*info*/) {}

// ===========================================================================
// Shoulder
// ===========================================================================

// @func 0x001072BC ShoulderSettings::LoadDefaults
void ShoulderSettings::LoadDefaults()
{
    const float* o = g_cameraShoulderOffset.Get();
    offset = { o[0], o[1], o[2], offset.pad };
    const float* c = g_cameraShoulderColOffset.Get();
    colOffset = { c[0], c[1], c[2], colOffset.pad };
    fov = g_cameraShoulderFOV;
}

// @func 0x00107320 ShoulderCameraMode::ShoulderCameraMode
ShoulderCameraMode::ShoulderCameraMode(GameCamera* owner, Entity* entity) : CameraMode(owner), target(entity)
{
    settings.LoadDefaults();
    const uint32_t group = (entity && IsCharacterEntity(entity)) ? CharacterCollisionGroup(entity) : 0;
    phantom = CreateSpherePhantom(g_cameraShoulderColRadius, (group << 16) | 0x10);
}

// @func 0x00107618 ShoulderCameraMode::~ShoulderCameraMode
// @func 0x00107698 ShoulderCameraMode::~ShoulderCameraMode (deleting)
ShoulderCameraMode::~ShoulderCameraMode()
{
    if (phantom) {
        DestroyPhantom(phantom);
        phantom = nullptr;
    }
}

// TODO(0x00107890 ShoulderCameraMode::Update)
void ShoulderCameraMode::Update(float /*dt*/, CameraViewState& /*view*/) {}

// @func 0x001082F0 ShoulderCameraMode::Activate
// A cut starts like a blend from a default view.
void ShoulderCameraMode::Activate()
{
    CameraViewState view;
    ActivateFrom(view);
}

// @func 0x00108468 ShoulderCameraMode::ActivateFrom
void ShoulderCameraMode::ActivateFrom(const CameraViewState& /*previous*/)
{
    blocked = false;
    blockTime = 0.0f;
    justActivated = true;
}

// @func 0x00107734 ShoulderCameraMode::Tick
bool ShoulderCameraMode::Tick(float dt, CameraViewState& view)
{
    Update(dt, view);
    UpdateStage2(dt, view);
    UpdateStage3(dt, view);
    justActivated = false;
    return true;
}

// ===========================================================================
// Spline
// ===========================================================================

// @func 0x0010B0E4 SplineCameraMode::Activate
// With a target spline the camera tracks a point between that spline and
// the target; otherwise it starts where the position spline comes closest
// to the target.
void SplineCameraMode::Activate()
{
    if (!target)
        return;
    const ape::Vec3 focus = EntityFocusPoint(target);
    ape::Vec3 point = focus;
    float param;
    if (!targetSpline) {
        param = static_cast<ape::world::ApeSplineInstance*>(positionSpline)->FindClosestParam(point, nullptr, nullptr);
    } else {
        auto* spline = static_cast<ape::world::ApeSplineInstance*>(targetSpline);
        ape::Vec3 onSpline{};
        spline->EvaluatePosition(spline->FindClosestParam(focus, nullptr, nullptr), onSpline, false);
        point = { targetBlend * (focus.x - onSpline.x) + onSpline.x, targetBlend * (focus.y - onSpline.y) + onSpline.y,
                  targetBlend * (focus.z - onSpline.z) + onSpline.z, 0.0f };
        param = spline->FindClosestParam(point, nullptr, nullptr);
    }
    t = param;
    trackPoint = { point.x, point.y, point.z, trackPoint.pad };
    splineFlags |= 1u << 29;
}

// @func 0x0010B348 SplineCameraMode::DebugDraw
void SplineCameraMode::DebugDraw(uint32_t flags)
{
    if (!(flags & 1))
        return;
    // Both splines in the same half-transparent colour. NOTE: the console
    // also evaluates the target spline at t; what it draws there is not
    // visible in the reference.
    static const uint8_t kTargetColor[4] = { 0xB0, 0x90, 0x40, 0xC0 };
    static const uint8_t kPositionColor[4] = { 0xB0, 0x90, 0x40, 0xC0 };
    if (targetSpline) {
        DebugDrawSpline(targetSpline, kTargetColor);
        ape::Vec3 unused{};
        static_cast<ape::world::ApeSplineInstance*>(targetSpline)->EvaluatePosition(t, unused, false);
    }
    DebugDrawSpline(positionSpline, kPositionColor);
}

// @func 0x001096C0 SplineCameraMode::~SplineCameraMode
// @func 0x00109774 SplineCameraMode::~SplineCameraMode (deleting)
SplineCameraMode::~SplineCameraMode()
{
    AssignEntity(target, nullptr);
    AssignEntity(lookAt, nullptr);
}

// @func 0x00108A34 SplineCameraMode::SetTarget
void SplineCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
    if (!entity)
        AssignEntity(lookAt, nullptr);
}

// @func 0x001089A8 SplineCameraMode::SetLookAt
void SplineCameraMode::SetLookAt(Entity* entity)
{
    if (lookAt != entity)
        AssignEntity(lookAt, entity);
}

// @func 0x0010B240 SplineCameraMode::Finish
void SplineCameraMode::Finish()
{
    GameCamera* owner = camera;
    CameraMode* below = owner->ModeBelow(this);
    owner->RemoveMode(-1.0f, this);
    if (below)
        below->Finish();
    delete this;
}

// @func 0x0010B574 SplineCameraMode::GetDebugText
const char* SplineCameraMode::GetDebugText()
{
    return ape::TempFormat("Target: %s\nTgt Spline: %s\nPos Spline: %s\nT: %5.3f\npan: (%5.1f,%5.1f)",
                           target ? EntityName(target) : "<none>",
                           targetSpline ? targetSpline->GetName() : "<none>", positionSpline->GetName(), t, panH,
                           panV);
}

// ===========================================================================
// Binocucom
// ===========================================================================

// @func 0x0010C3C8 BinocucomCameraMode::~BinocucomCameraMode
// @func 0x0010C60C BinocucomCameraMode::~BinocucomCameraMode (deleting)
BinocucomCameraMode::~BinocucomCameraMode()
{
    if (photoComponent) {
        player->RemoveComponent(photoComponent);
        photoComponent = nullptr;
    }
    if (ape::ApeGuiObject* photo = ape::FindGuiObject(".InGame.BinocucomPhoto", nullptr))
        photo->SetVisible(false);
    // Unless the player has already left the binocucom stance, it turns to
    // face where the binocucom looked, keeping its up axis.
    if (entity::EntityGetState(player, 0) == 3 && entity::EntityGetState(player, 1) != 1)
        return;
    ape::Matrix34 m = player->transform;
    const ape::Vec3& up = m.yAxis;
    ape::Vec3 side;
    ape::Vec3Cross(side, up, lookDirection);
    ape::Vec3Normalize(side);
    ape::Vec3 forward;
    ape::Vec3Cross(forward, side, up);
    ape::Vec3Normalize(forward);
    m.xAxis = { side.x, side.y, side.z, m.xAxis.pad };
    m.zAxis = { forward.x, forward.y, forward.z, m.zAxis.pad };
    EntitySetWorldTransform(player, m);
}

// @func 0x0010D9A0 BinocucomCameraMode::Deactivate
void BinocucomCameraMode::Deactivate()
{
    camera->fov = savedFov;
}

// @func 0x0010D9B0 BinocucomCameraMode::HoldZoom
bool BinocucomCameraMode::HoldZoom(bool hold)
{
    if (hold) {
        ++zoomHolds;
        zoomFlags &= 0x3FFF;
    } else if (zoomHolds > 0) {
        --zoomHolds;
    }
    return zoomHolds > 0;
}


// ===========================================================================
// Vehicle
// ===========================================================================

void VehicleChaseSettings::LoadTuningDefaults()
{
    fov = g_cameraVehicleFOV;
    const float* look = g_cameraVehicleLocalLookAtOffset.Get();
    localLookAtOffset = { look[0], look[1], look[2], localLookAtOffset.pad };
    horzInputRotScalar = g_cameraVehicleHorzInputRotScalar;
    horzInputRotInterp = g_cameraVehicleHorzInputRotInterp;
    lookAtXZInterp = g_cameraVehicleLookAtXZInterp;
    lookAtYUpInterp = g_cameraVehicleLookAtYUpInterp;
    lookAtYDownInterp = g_cameraVehicleLookAtYDownInterp;
    gimbalRadius = g_cameraVehicleGimbalRadius;
    vertGimbalMin = g_cameraVehicleVertGimbalMin;
    vertGimbalMax = g_cameraVehicleVertGimbalMax;
    vertGimbalRestShift = g_cameraVehicleVertGimbalRestShift;
    const float* offset = g_cameraVehicleGimbalOffset.Get();
    gimbalOffset = { offset[0], offset[1], offset[2], gimbalOffset.pad };
    vertInputRotScalar = g_cameraVehicleVertInputRotScalar;
    vertInputRotAccelInterp = g_cameraVehicleVertInputRotAccelInterp;
    vertInputRotVelInterp = g_cameraVehicleVertInputRotVelInterp;
    gimbalReturnInterp = g_cameraVehicleGimbalReturnInterp;
    colRadius = g_cameraVehicleColRadius;
    colFractHS = g_cameraVehicleColFractHS;
    colInterp = g_cameraVehicleColInterp;
    colConeMin = g_cameraVehicleColConeMin;
    colMomentMax = g_cameraVehicleColMomentMax;
    colRetract = g_cameraVehicleColRetract;
}

// @func 0x0010DA30 VehicleCameraMode::LoadTargetSettings
void VehicleCameraMode::LoadTargetSettings()
{
    const VehicleCameraDesc* desc = (target && IsVehicleEntity(target)) ? VehicleCameraData(target) : nullptr;
    if (!desc) {
        targetSettings.LoadTuningDefaults();
        return;
    }
    constexpr float kDegToRad = 0.017453292f;
    VehicleChaseSettings& t = targetSettings;
    t.fov = desc->fov;
    t.localLookAtOffset = desc->localLookAtOffset;
    float* const values20[] = { &t.horzInputRotScalar, &t.horzInputRotInterp, &t.lookAtXZInterp,
                                &t.lookAtYUpInterp, &t.lookAtYDownInterp, &t.gimbalRadius, &t.vertGimbalMin,
                                &t.vertGimbalMax, &t.vertGimbalRestShift };
    for (int i = 0; i < 9; ++i)
        *values20[i] = desc->values20[i];
    t.gimbalOffset = desc->gimbalOffset;
    float* const values60[] = { &t.vertInputRotScalar, &t.vertInputRotAccelInterp, &t.vertInputRotVelInterp,
                                &t.gimbalReturnInterp, &t.colRadius, &t.colFractHS, &t.colInterp, &t.colConeMin,
                                &t.colMomentMax, &t.colRetract, &t.headingReturnMinTimeDefault,
                                &t.headingReturnMinTimeFwd, &t.headingReturnMinTimeReverse,
                                &t.headingReturnLowMaxRotSpeed, &t.headingReturnHighMaxRotSpeed };
    for (int i = 0; i < 15; ++i)
        *values60[i] = desc->values60[i];
    t.headingReturnLowSin = ape::FastSin(desc->headingReturnLowAngle * kDegToRad);
    t.headingReturnHighSin = ape::FastSin(desc->headingReturnHighAngle * kDegToRad);
    t.headingReturnFullSpeedElapsedTime = desc->headingReturnFullSpeedElapsedTime;
}

// @func 0x0010DD60 VehicleCameraMode::SetTarget
void VehicleCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
    if (entity)
        LoadTargetSettings();
}

// @func 0x0010E9A4 VehicleCameraMode::~VehicleCameraMode
// @func 0x0010EAD0 VehicleCameraMode::~VehicleCameraMode (deleting)
VehicleCameraMode::~VehicleCameraMode()
{
    AssignEntity(target, nullptr);
    DestroyPhantom(phantom);
    phantom = nullptr;
    if (body)
        DestroyHavokEntity(body);
}

// @func 0x0010EB20 VehicleCameraMode::EaseFlagged
void VehicleCameraMode::EaseFlagged(float goal, float rate, float dt, uint32_t* dirtyBits, uint32_t bit, float& value)
{
    uint32_t& word = dirtyBits[bit >> 5];
    const uint32_t mask = 1u << (bit & 31);
    if (!(word & mask))
        return;
    if (std::fabs(value - goal) < 1e-05f)
        word &= ~mask;
    else
        value = ApproachExp(value, goal, rate, dt);
}

// @func 0x0010EBE8 VehicleCameraMode::EaseFlagged
void VehicleCameraMode::EaseFlagged(float rate, float dt, uint32_t* dirtyBits, uint32_t bit, ape::Vec3& value,
                                    const ape::Vec3& goal)
{
    uint32_t& word = dirtyBits[bit >> 5];
    const uint32_t mask = 1u << (bit & 31);
    if (!(word & mask))
        return;
    if (std::fabs(value.x - goal.x) >= 1e-05f || std::fabs(value.y - goal.y) >= 1e-05f ||
        std::fabs(value.z - goal.z) >= 1e-05f) {
        ape::Vec3 eased;
        ApproachExp3(eased, rate, dt, value, goal);
        SetXYZ(value, eased);
    } else {
        word &= ~mask;
    }
}

// ===========================================================================
// Havok vehicle
// ===========================================================================

HavokVehicleCameraMode::HavokVehicleCameraMode(GameCamera* owner) : CameraMode(owner)
{
    ConstructVehicleFollowCamera(followCamera);
}

// @func 0x00111840 HavokVehicleCameraMode::~HavokVehicleCameraMode
// @func 0x001118C8 HavokVehicleCameraMode::~HavokVehicleCameraMode (deleting)
HavokVehicleCameraMode::~HavokVehicleCameraMode()
{
    AssignEntity(target, nullptr);
    DestroyVehicleFollowCamera(followCamera);
}

// @func 0x0011135C HavokVehicleCameraMode::SetTarget
void HavokVehicleCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
}

// ===========================================================================
// Track
// ===========================================================================

// @func 0x00749888 TrackCameraMode::~TrackCameraMode
// @func 0x00749904 TrackCameraMode::~TrackCameraMode (deleting)
TrackCameraMode::~TrackCameraMode()
{
    AssignEntity(target, nullptr);
}

// @func 0x001138D8 TrackCameraMode::SetTarget
void TrackCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
}

// TODO(0x001139C0 TrackCameraMode::Update)
void TrackCameraMode::Update(float /*dt*/, CameraViewState& /*view*/) {}

// @func 0x00114130 TrackCameraMode::GetDebugText
const char* TrackCameraMode::GetDebugText()
{
    constexpr float kRadToDeg = 57.295776f;
    return ape::TempFormat("Target: %s\nOffsetPos: {%5.2f,%5.2f,%5.2f}\nOffsetRot: {%5.2f,%5.2f}\nInvertDir: %d",
                           target ? EntityName(target) : "<none>", offsetPos.x, offsetPos.y, offsetPos.z,
                           offsetYaw * kRadToDeg, offsetPitch * kRadToDeg, invertDir ? 1 : 0);
}

// ===========================================================================
// Orbit
// ===========================================================================

// @func 0x00114298 OrbitCameraMode::OrbitCameraMode
OrbitCameraMode::OrbitCameraMode(GameCamera* owner, Entity* entity, OrbitStyle orbitStyle)
    : CameraMode(owner), style(orbitStyle)
{
    constexpr float kDegToRad = 0.017453292f;
    const float* limit = g_cameraOrbitModeStandardGimbalLimit.Get();
    gimbalLimit = { limit[0] * kDegToRad, limit[1] * kDegToRad, limit[2] * kDegToRad, 0.0f };
    yaw = entity ? crt::ATan2F(entity->transform.zAxis.x, entity->transform.zAxis.z) : 0.0f;
    pitch = gimbalLimit.z;

    const bool sprint = orbitStyle == kOrbit_Sprint;
    const float* offset = (sprint ? g_cameraOrbitModeSprintGimbalOffset : g_cameraOrbitModeStandardGimbalOffset).Get();
    gimbalOffset = { offset[0], offset[1], offset[2], 0.0f };
    radius = sprint ? g_cameraOrbitModeSprintRadius : g_cameraOrbitModeStandardRadius;
    fov = sprint ? g_cameraOrbitModeSprintFOV : g_cameraOrbitModeStandardFOV;
    snap = true;
    AssignEntity(target, entity);
}

// @func 0x001144D0 OrbitCameraMode::~OrbitCameraMode
// @func 0x0011454C OrbitCameraMode::~OrbitCameraMode (deleting)
OrbitCameraMode::~OrbitCameraMode()
{
    AssignEntity(target, nullptr);
}

// @func 0x00114208 OrbitCameraMode::SetTarget
void OrbitCameraMode::SetTarget(Entity* entity)
{
    AssignEntity(target, entity);
}

// TODO(0x00114650 OrbitCameraMode::Update)
void OrbitCameraMode::Update(float /*dt*/, CameraViewState& /*view*/) {}

// ===========================================================================
// Strafe
// ===========================================================================

void StrafeSettings::LoadDefaults(float cameraHeight)
{
    interp = g_cameraStrafeInterp;
    fovInterp = g_cameraFovInterpInterp;
    fov = g_cameraStrafeFOV;
    const float* adjust = g_cameraStrafePosAdjust.Get();
    posAdjust = { adjust[0], adjust[1], adjust[2], 0.0f };
    gimbalRadius = g_cameraStrafeGimbalRadius;
    angleHome = g_cameraStrafeAngleHome;
    angleMin = g_cameraStrafeAngleMin;
    angleMax = g_cameraStrafeAngleMax;
    height = cameraHeight;
}

// @func 0x00115900 StrafeSettings::BlendToward
void StrafeSettings::BlendToward(float dt, const StrafeSettings& to)
{
    constexpr float kEpsilon = 1e-05f;
    auto approach = [dt](float& value, float goal, float rate) {
        if (std::fabs(value - goal) >= kEpsilon)
            value = ApproachExp(value, goal, rate, dt);
    };
    approach(fov, to.fov, to.fovInterp);
    if (std::fabs(posAdjust.x - to.posAdjust.x) >= kEpsilon || std::fabs(posAdjust.y - to.posAdjust.y) >= kEpsilon ||
        std::fabs(posAdjust.z - to.posAdjust.z) >= kEpsilon) {
        ape::Vec3 eased;
        ApproachExp3(eased, to.interp, dt, posAdjust, to.posAdjust);
        SetXYZ(posAdjust, eased);
    }
    approach(gimbalRadius, to.gimbalRadius, to.interp);
    approach(angleHome, to.angleHome, to.interp);
    approach(angleMin, to.angleMin, to.interp);
    approach(angleMax, to.angleMax, to.interp);
    height = to.height;
}

// @func 0x00115B90 StrafeCameraMode::StrafeCameraMode
StrafeCameraMode::StrafeCameraMode(float height, GameCamera* owner, Entity* subjectEntity, Entity* opponentEntity)
    : CameraMode(owner)
{
    current.LoadDefaults(height);
    target.LoadDefaults(height);
    AssignEntity(subject, subjectEntity);
    AssignEntity(opponent, opponentEntity);
}

// @func 0x00115D2C StrafeCameraMode::~StrafeCameraMode
// @func 0x00115DE0 StrafeCameraMode::~StrafeCameraMode (deleting)
StrafeCameraMode::~StrafeCameraMode()
{
    AssignEntity(opponent, nullptr);
    AssignEntity(subject, nullptr);
}

// @func 0x00115B0C StrafeCameraMode::SetSubject
void StrafeCameraMode::SetSubject(Entity* entity)
{
    AssignEntity(subject, entity);
}

// @func 0x00115EB0 StrafeCameraMode::Tick
bool StrafeCameraMode::Tick(float dt, CameraViewState& view)
{
    current.BlendToward(dt, target);
    Update(dt, view);
    UpdateStage2(dt, view);
    UpdateStage3(dt, view);
    return true;
}

// TODO(0x00115FB0 StrafeCameraMode::Update)
void StrafeCameraMode::Update(float /*dt*/, CameraViewState& view)
{
    // Without both entities alive the stored transform is shown.
    SetXYZ(view.transform.xAxis, transform.xAxis);
    SetXYZ(view.transform.yAxis, transform.yAxis);
    SetXYZ(view.transform.zAxis, transform.zAxis);
    SetXYZ(view.transform.translation, transform.translation);
}

} // namespace camera
} // namespace sly
