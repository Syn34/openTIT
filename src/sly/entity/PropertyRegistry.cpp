// openTIT - The definition registry of the game property system: every
// entity definition, and the definitions of six property types in their own
// lists (seven sorted maps at 0x010BB754, 0x18 bytes apart, keyed by the
// definition; the type of each list is in the table 0x00AE9490).
#include "sly/entity/EntityHooks.h"
#include "sly/entity/EntityDef.h"

#include <algorithm>
#include <vector>

namespace sly {
namespace entity {

namespace {

constexpr int kRegistryCount = 7;
// Property type of each list; list 0 holds every definition.
constexpr uint32_t kRegistryTypes[kRegistryCount] = {
    0, 0x0353CB09u, 0x3F2F1B0Cu, 0x34FD66B9u, 0xC53F7E26u, 0x00259B63u, 0x79B9AD91u,
};
// NOTE: the type names are unknown.

// Bits that keep a definition out of the registry: triggers (class bit 22)
// and definitions flagged so (registry flags bit 1).
constexpr uint32_t kDefClass_Trigger = 0x400000u;
constexpr uint32_t kRegistry_Skip = 0x2u;

std::vector<EntityDef*> g_registries[kRegistryCount];

bool Registrable(const EntityDef* def)
{
    return def && !(def->classBits & kDefClass_Trigger) && !(def->registryFlags & kRegistry_Skip);
}

void AddUnique(std::vector<EntityDef*>& list, EntityDef* def)
{
    if (std::find(list.begin(), list.end(), def) == list.end())
        list.push_back(def);
}

} // namespace

// @func 0x00490360 PropertySystemRegisterDef
void PropertySystemRegisterDef(EntityDef* def, uint32_t type)
{
    if (!Registrable(def))
        return;
    AddUnique(g_registries[0], def);
    if (type == 0)
        return;
    for (int i = 0; i < kRegistryCount; ++i) {
        if (kRegistryTypes[i] == type) {
            AddUnique(g_registries[i], def);
            return;
        }
    }
}

// @func 0x00490518 PropertySystemForgetDef
void PropertySystemForgetDef(EntityDef* def)
{
    if (!Registrable(def))
        return;
    for (std::vector<EntityDef*>& list : g_registries)
        list.erase(std::remove(list.begin(), list.end(), def), list.end());
}

const std::vector<EntityDef*>& PropertyRegistry(int index)
{
    return g_registries[index];
}

} // namespace entity
} // namespace sly
