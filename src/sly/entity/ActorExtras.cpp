// openTIT - Further actor class methods (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraMode.h"
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

} // namespace entity
} // namespace sly
