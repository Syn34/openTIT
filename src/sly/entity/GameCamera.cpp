// openTIT - The game camera entity (see GameCamera.h).
#include "sly/entity/GameCamera.h"

#include "sly/entity/EntityHooks.h"

namespace sly {
namespace entity {

// @func 0x000FA5D0 GameCamera::GetView
void GameCamera::GetView(CameraView& out) const
{
    out.transform.xAxis = { transform.xAxis.x, transform.xAxis.y, transform.xAxis.z, out.transform.xAxis.pad };
    out.transform.yAxis = { transform.yAxis.x, transform.yAxis.y, transform.yAxis.z, out.transform.yAxis.pad };
    out.transform.zAxis = { transform.zAxis.x, transform.zAxis.y, transform.zAxis.z, out.transform.zAxis.pad };
    out.transform.translation = { transform.translation.x, transform.translation.y, transform.translation.z,
                                  out.transform.translation.pad };
    out.field_40 = field_164;
    out.field_44 = field_168;
}

// @func 0x000FCBC8 GameCamera::SetAttachParent
void GameCamera::SetAttachParent(Entity* parent)
{
    attachParent = parent;
}

// @func 0x000FC5DC GameCamera::HasTarget
bool GameCamera::HasTarget(uint32_t target) const
{
    for (int i = targetCount - 1; i >= 0; --i)
        if (targets[i] == target)
            return true;
    return false;
}

// @func 0x000FCBD0 GameCamera::TargetBelow
uint32_t GameCamera::TargetBelow(uint32_t target) const
{
    for (int i = targetCount - 1; i >= 0; --i)
        if (targets[i] == target)
            return i > 0 ? targets[i - 1] : 0;
    return 0;
}

// @func 0x000FCFDC GameCamera::SetAnimPaused
void GameCamera::SetAnimPaused(bool paused)
{
    if (!cameraAnim)
        return;
    if (void* playing = AnimFindPlaying(cameraAnim, 0))
        AnimSetPaused(playing, paused);
}

} // namespace entity
} // namespace sly
