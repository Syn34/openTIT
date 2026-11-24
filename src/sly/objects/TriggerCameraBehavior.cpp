// openTIT - Camera behaviour trigger volumes.
#include "sly/objects/TriggerCameraBehavior.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

namespace sly {
namespace objects {

namespace {
constexpr uint32_t kFlags2_Character = 0x08;
}

// @func 0x00218980 CameraBehaviorTriggerDef::Create
ObjEntity* CameraBehaviorTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kDefClass_CameraBehaviorTrigger) != 0))
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    CameraBehaviorTrigger* trigger = new CameraBehaviorTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// @func 0x00218D30 CameraBehaviorTrigger::~CameraBehaviorTrigger (deleting)
CameraBehaviorTrigger::~CameraBehaviorTrigger() = default;

// @func 0x00218D78 CameraBehaviorTrigger::Update
void CameraBehaviorTrigger::Update(float dt)
{
    // Disabled volumes do not even process their reports.
    if (IsEnabled())
        TriggerBase::Update(dt);
}

// @func 0x0021941C CameraBehaviorTrigger::OnEnter
// Only player characters (or player vehicles, class2 flag 0x400) count.
void CameraBehaviorTrigger::OnEnter(ObjEntity* entity)
{
    if (!entity)
        return;
    const bool character = (entity->flags2 & kFlags2_Character) != 0;
    const bool vehicle = (EntityClass2(entity) & 0x400) != 0;
    if ((!character && !vehicle) || !entity->IsPlayer())
        return;
    TriggerBase::OnEnter(entity);
    ApplyBehavior(entity);
}

// @func 0x00219864 CameraBehaviorTrigger::IsDebugDisplayed
bool CameraBehaviorTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerCameraBehavior.Get());
}

} // namespace objects
} // namespace sly
