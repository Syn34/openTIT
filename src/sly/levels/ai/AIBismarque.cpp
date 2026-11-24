// openTIT - Bismarque boss behaviour (vtable 0x00B0E888), partial.
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/world/ApeObj.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AITuning.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

// @func 0x00042D3C BismarqueAI::GetTargetPosition
ape::Vec3 BismarqueAI::GetTargetPosition()
{
    return AIController::GetTargetPosition();
}

// @func 0x0004377C BismarqueAI::Think
bool BismarqueAI::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x0004399C BismarqueAI::UpdateMarkers
void BismarqueAI::UpdateMarkers()
{
    for (int i = 0; i < 2; ++i) {
        if (!markers[i] || !anchors[i])
            continue;
        ape::Matrix34 local = markers[i]->GetLocal();
        const ape::Vec3& at = anchors[i]->GetWorld().translation;
        local.translation.x = at.x;
        local.translation.y = at.y;
        local.translation.z = at.z;
        markers[i]->SetLocal(local);
        markers[i]->MarkDirty();
        markers[i]->UpdateWorld(false, false);
    }
}

// @func 0x00043A80 BismarqueAI::SetMarkersVisible
void BismarqueAI::SetMarkersVisible(bool first, bool second)
{
    if (markers[0])
        markers[0]->SetVisible(first, true);
    if (markers[1])
        markers[1]->SetVisible(second, true);
}

// @func 0x00044318 BismarqueAI::ScoreTargetPoint
float BismarqueAI::ScoreTargetPoint(const ape::Vec3& point, const ape::Vec3& direction) const
{
    const ape::Vec3& pos = transform.translation;
    const ape::Vec3& forward = transform.zAxis;
    const float dx = point.x - pos.x, dy = point.y - pos.y, dz = point.z - pos.z;
    const float dist = std::sqrt(dz * dz + dx * dx + dy * dy);
    const float inv = 1.0f / dist;
    const float turn = 1.0f - (direction.z * forward.z + direction.x * forward.x + direction.y * forward.y);
    const float aim = 1.0f - (forward.z * inv * dz + forward.x * inv * dx + forward.y * inv * dy);
    return turn * g_bismarqueTargetPointDeltaOrientationWeight + aim * g_bismarqueTargetPointAngleWeight +
           dist * g_bismarqueTargetPointDistanceWeight + 0.0f;
}

// @func 0x00046638 BismarqueAI::CounterAttackReady
bool BismarqueAI::CounterAttackReady(float dt)
{
    Entity* player = PlayerCharacter();
    if (!player)
        return false;
    counterCooldown -= dt;
    if (counterCooldown > 0.0f)
        return false;
    counterCooldown = 0.0f;
    const float dx = player->Position().x - transform.translation.x;
    const float dz = player->Position().z - transform.translation.z;
    const float range = g_bismarqueCounterAttackDistMin;
    return dz * dz + dx * dx + 0.0f < range * range;
}

// @func 0x00048CE0 BismarqueAI::GravitySpeed
float BismarqueAI::GravitySpeed()
{
    if (!(character->classBits & 0x4000u) && BodyMotionType(body) != 1)
        return -10.0f;
    return 0.0f;
}

} // namespace ai
} // namespace levels
} // namespace sly
