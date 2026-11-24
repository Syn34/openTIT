// openTIT - The simpler camera modes (see CameraModes.h).
#include "sly/camera/CameraModes.h"

#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
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

// @func 0x00113350 ShooterCameraMode::Release
void ShooterCameraMode::Release()
{
    GameCamera* owner = camera;
    CameraMode* below = owner->ModeBelow(this);
    owner->RemoveMode(-1.0f, this);
    // NOTE: the mode underneath is released together with this one.
    if (below)
        below->Release();
    delete this;
}

} // namespace camera
} // namespace sly
