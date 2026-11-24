// openTIT - El Jefe boss behaviour (vtable 0x00B0E9C0), partial.
#include "sly/levels/ai/AIBehaviours.h"

#include "sly/game/WorldInterface.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AIProxyPoints.h"

namespace sly {
namespace levels {
namespace ai {

// @func 0x0004B0D0 ElJefeAI::Detach
void ElJefeAI::Detach()
{
    AIController::Detach();
    while (spawnedCount > 0) {
        --spawnedCount;
        spawned[spawnedCount]->Release(false);
    }
    GameWorldSetBossFlag(false);
    if (camera) {
        ReleaseCameraMode(camera);
        camera = nullptr;
    }
}

// @func 0x0004E660 ElJefeAI::GetTargetPosition
ape::Vec3 ElJefeAI::GetTargetPosition()
{
    // Outside the special phase El Jefe aims at his stage's point.
    if (!field_713 || field_41C == 0) {
        const Stage& stage = stages[stageIndex];
        if (stage.point)
            return InstancePosition(stage.point->object);
    }
    return AIController::GetTargetPosition();
}

// @func 0x0004E9D4 ElJefeAI::OnTauntChance
void ElJefeAI::OnTauntChance() {}

} // namespace ai
} // namespace levels
} // namespace sly
