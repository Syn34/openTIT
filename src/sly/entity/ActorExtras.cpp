// openTIT - Further actor class methods (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

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

} // namespace entity
} // namespace sly
