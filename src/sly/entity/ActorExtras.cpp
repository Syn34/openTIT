// openTIT - Further actor class methods (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "sly/camera/CameraMode.h"
#include "sly/entity/ActorDefs.h"
#include "sly/entity/GameCamera.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/tuning/TargetRangeTuning.h"

#include <cmath>

namespace sly {
namespace entity {

// -- InteractivePropC (the player turret) ------------------------------------------

// @func 0x001DEC60 InteractivePropC::ReleaseCamera
void InteractivePropC::ReleaseCamera()
{
    GameCamera* camera = MainGameCamera();
    if (!cameraMode || !camera)
        return;
    camera->RemoveMode(g_playerTurretCameraEaseOutTime, cameraMode);
    delete cameraMode;
    cameraMode = nullptr;
}

// @func 0x001DC988 InteractivePropC::UpdateAimFrame
void InteractivePropC::UpdateAimFrame()
{
    ape::Matrix34 frame = baseFrame;
    ape::Matrix34 rotation;
    ape::Matrix34RotationY(yaw, rotation);
    ape::world::Matrix34Mul(frame, frame, rotation);
    ape::Matrix34RotationX(pitch, rotation);
    ape::Matrix34PreMultiply(frame, rotation);
    aimFrame = ape::Matrix34NormalizeAxes(frame);
}

// -- SoundActorB17270 (the gadget turret) ------------------------------------------

// @func 0x001CAF1C SoundActorB17270::FindAimParts
void SoundActorB17270::FindAimParts()
{
    const auto* turretDef = static_cast<const SoundActorB17270Def*>(def);
    if (turretDef->aimPartName) {
        aimBone = GetBoneByName(turretDef->aimPartName);
        if (!aimBone)
            aimObject = EntityFindObject(this, turretDef->aimPartName);
        else
            aimParentBone = GetBone(aimBone->def->parent);
    }
    if (turretDef->field_194)
        object_344 = EntityFindObject(this, turretDef->field_194);
    if (turretDef->field_198)
        object_348 = EntityFindObject(this, turretDef->field_198);
}

// @func 0x001CAAEC SoundActorB17270::UpdateAimFrames
void SoundActorB17270::UpdateAimFrames()
{
    if (!object_344 && !object_348) {
        if (aimBone)
            ape::world::Matrix34Mul(aimFrame, *aimBone->model, GetLocalTransform());
        else
            aimFrame = aimObject ? aimObject->GetWorld() : GetLocalTransform();
        return;
    }
    baseTransform = object_344 ? object_344->GetWorld() : GetLocalTransform();
    aimFrame = object_348 ? object_348->GetWorld() : GetLocalTransform();
}

// @func 0x001E1EE4 InteractivePropC::DefField140
ape::Vec3 InteractivePropC::DefField140(const Entity* prop)
{
    if (!prop->def)
        return {0.0f, 0.0f, 0.0f, 0.0f};
    const ape::Vec3& v = static_cast<const InteractivePropADef*>(prop->def)->field_140;
    return {v.x, v.y, v.z, 0.0f};
}

// -- TweenActor ------------------------------------------------------------------------

// @func 0x001F01A8 TweenActor::ScaleUserPropTime
void TweenActor::ScaleUserPropTime()
{
    if (!field_358)
        return;
    if (Entity* prop = field_358->FindDescendant(0x366FD481u))
        prop->SetTimeScale(0x7B2728A1u, true);
}

// @func 0x001EF878 TweenActor::SetAiming
void TweenActor::SetAiming(bool aiming, bool immediate)
{
    const bool wasAiming = (tweenFlags & 0x00400000u) != 0;
    if ((!wasAiming && aiming) || immediate) {
        if (immediate)
            aimAngles[0] = aimAngles[1] = 0.0f;
        aimSpeed[0] = aimSpeed[1] = 0.0f;
        tween_220.from = immediate ? 1.0f : 0.0f;
        tween_220.to = 1.0f;
        tween_220.elapsed = 0.0f;
        tween_220.SetDuration(immediate ? 0.0f : float(g_tgtRangeAimRotAccelRampTime));
    }
    tweenFlags = aiming ? (tweenFlags | 0x00400000u) : (tweenFlags & ~0x00400000u);
}

// @func 0x001EF728 TweenActor::UserReady
bool TweenActor::UserReady()
{
    if (tweenFlags & 0x00020000u) {
        tweenFlags &= ~0x00020000u;
        return false;
    }
    return field_358 && (field_358->flags & 0x00040000u) && EntityGetState(field_358, 0) == 0 &&
           EntityGetState(field_358, 1) == 6;
}

// @func 0x001F0218 TweenActor::AimPitch
float TweenActor::AimPitch() const
{
    return std::asin(field_304) + float(g_tgtRangeAimOffsetVisual) * 0.01745329238474369f;
}

} // namespace entity
} // namespace sly
