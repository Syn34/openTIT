// openTIT - Priority targets of the level AI.
#include "sly/levels/ai/AIPriorityTargets.h"

#include "sly/game/WorldInterface.h"

namespace sly {
namespace levels {

namespace {
AIPriorityTargets* g_priorityTargets = nullptr;
}

AIPriorityTargets* GetAIPriorityTargets() { return g_priorityTargets; }
void SetAIPriorityTargets(AIPriorityTargets* targets) { g_priorityTargets = targets; }

// @func 0x0001B238 AIPriorityTargets::Add
void AIPriorityTargets::Add(WorldObject* entity)
{
    if (m_count < kMaxTargets)
        m_targets[m_count++] = entity;
}

// @func 0x0001B274 AIPriorityTargets::Remove
void AIPriorityTargets::Remove(WorldObject* entity)
{
    for (int i = 0; i < m_count; ++i) {
        if (m_targets[i] == entity) {
            m_targets[i] = m_targets[m_count - 1];
            --m_count;
            --i;  // look at the entry moved into this slot
        }
    }
}

// @func 0x0001B2E8 AIPriorityTargets::Clear
void AIPriorityTargets::Clear()
{
    m_count = 0;
}

void AIPriorityTargets::RemoveDead()
{
    for (int i = 0; i < m_count; ++i) {
        if (!SpawnedIsAlive(m_targets[i])) {
            m_targets[i] = m_targets[m_count - 1];
            --m_count;
            --i;
        }
    }
}

} // namespace levels
} // namespace sly
