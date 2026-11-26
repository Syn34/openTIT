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

} // namespace entity
} // namespace sly
