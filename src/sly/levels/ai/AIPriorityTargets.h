// openTIT - Priority targets of the level AI.
//
// Scripts ("PriorityTarget" command) mark up to eight entities the AI should
// prefer as targets. The list lives in a level AI object reached through the
// console pointer 0x00F557F0 (count at +0x14, entries at +0xE74); only this
// part of that object is reconstructed here.
#pragma once

#include <cstdint>

namespace sly {

class WorldObject;

namespace levels {

// NOTE: the class name is not known.
class AIPriorityTargets {
public:
    static constexpr int kMaxTargets = 8;

    // Appends a target; ignored once the list is full (0x0001B238).
    void Add(WorldObject* entity);
    // Removes every occurrence of `entity`, filling holes with the last entry
    // (0x0001B274).
    void Remove(WorldObject* entity);
    // Empties the list (0x0001B2E8).
    void Clear();

    int Count() const { return m_count; }
    WorldObject* At(int index) const { return (index >= 0 && index < m_count) ? m_targets[index] : nullptr; }

private:
    int32_t m_count = 0;                           // +0x14
    WorldObject* m_targets[kMaxTargets] = {};      // +0xE74
};

// The level's AI object (console pointer 0x00F557F0), may be null.
AIPriorityTargets* GetAIPriorityTargets();
void SetAIPriorityTargets(AIPriorityTargets* targets);

} // namespace levels
} // namespace sly
