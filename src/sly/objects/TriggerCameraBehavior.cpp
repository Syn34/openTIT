// openTIT - Camera behaviour trigger volumes.
#include "sly/objects/TriggerCameraBehavior.h"

#include "ape/memory/ApeMem.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

namespace sly {
namespace objects {

namespace {
}

// @func 0x002172A8 CameraBehaviorTriggerDef::~CameraBehaviorTriggerDef
// @func 0x00217374 CameraBehaviorTriggerDef::~CameraBehaviorTriggerDef (deleting)
CameraBehaviorTriggerDef::~CameraBehaviorTriggerDef()
{
    for (void*& block : m_behaviorBlocks) {
        if (block) {
            ape::MemFree(block, 0);
            block = nullptr;
        }
    }
}

// @func 0x00218980 CameraBehaviorTriggerDef::Spawn
Entity* CameraBehaviorTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kDefClass_CameraBehaviorTrigger) != 0))
        return nullptr;
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    CameraBehaviorTrigger* trigger = new CameraBehaviorTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// @func 0x00218D30 CameraBehaviorTrigger::~CameraBehaviorTrigger (deleting)
CameraBehaviorTrigger::~CameraBehaviorTrigger() = default;

// @func 0x00218D78 CameraBehaviorTrigger::Update
bool CameraBehaviorTrigger::Update(float dt)
{
    // Disabled volumes do not even process their reports.
    if (IsEnabled())
        return TriggerBase::Update(dt);
    return true;
}

// @func 0x0021941C CameraBehaviorTrigger::OnEnter
// Only player characters (or player vehicles, class2 flag 0x400) count.
void CameraBehaviorTrigger::OnEnter(Entity* entity)
{
    if (!entity)
        return;
    const bool character = entity->HasTypeBit(kType_Character);
    const bool vehicle = (EntityClass2(entity) & 0x400) != 0;
    if ((!character && !vehicle) || !EntityIsPlayer(entity))
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
