// openTIT - Goal type 0x22, see GoalSpawnerHunt.h.
#include "sly/game/goals/GoalSpawnerHunt.h"

#include "sly/entity/Entity.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudManager.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

#include <cstdio>

namespace sly {
namespace goals {

namespace {

namespace attr {
constexpr uint32_t kField_A2C = 0x35107D59u;
constexpr uint32_t kSpawnerBase = 0x268C74EFu;
constexpr uint32_t kField_AB0 = 0x0A1A79FBu;
constexpr uint32_t kMarkerBase = 0xBFB1D754u;
constexpr uint32_t kField_A4C = 0x6E6EDD6Bu;
constexpr uint32_t kOffset = 0x4A4D4D4Du;
constexpr uint32_t kField_A60 = 0x9446A097u;
} // namespace attr

constexpr uint32_t kDefaultField_A2C = 0x35107ACFu;
constexpr uint32_t kDefaultSpawnerBase = 0xBE4B81CBu;
constexpr int kSpawnerCount = 5;
constexpr int kMaxMarkerObjects = 32;

Spawner* FindHuntSpawner(uint32_t base, int index)
{
    return g_spawnManager ? g_spawnManager->FindSpawner(base + uint32_t(index)) : nullptr;
}

} // namespace

// @func 0x003373E8 GoalSpawnerHunt::Create
Goal* GoalSpawnerHunt::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t fieldA2C = 0, spawnerBase = 0, fieldA4C = 0;
    float fieldAB0 = 0.0f;
    float offset[3] = {};
    const float zero[3] = {0.0f, 0.0f, 0.0f};
    bool fieldA60 = false;
    xml->GetHash(attr::kField_A2C, &fieldA2C, kDefaultField_A2C);
    xml->GetHash(attr::kSpawnerBase, &spawnerBase, kDefaultSpawnerBase);
    xml->GetFloat(attr::kField_AB0, &fieldAB0, 8.0f);
    const char* markerBase = xml->GetString(attr::kMarkerBase);
    xml->GetHash(attr::kField_A4C, &fieldA4C, 0);
    xml->GetVec3(attr::kOffset, offset, zero);
    xml->GetBool(attr::kField_A60, &fieldA60, false);

    auto* goal = new GoalSpawnerHunt(desc);
    goal->spawnerBase = spawnerBase;
    goal->offset = {offset[0], offset[1], offset[2], 0.0f};
    goal->field_A24 = -1;
    goal->field_A28 = false;
    goal->field_A2C = fieldA2C;
    goal->field_A48 = false;
    goal->field_A49 = false;
    goal->field_A4A = false;
    goal->field_A4C = fieldA4C;
    goal->hudElement = nullptr;
    goal->caught = nullptr;
    goal->hudTaken = false;
    goal->markerCount = 0;
    goal->field_AB0 = fieldAB0;
    goal->field_A60 = fieldA60;
    for (int i = 0; i < kSpawnerCount; ++i) {
        goal->spawned[i] = nullptr;
        goal->spawners[i] = FindHuntSpawner(spawnerBase, i);
    }
    if (markerBase)
        ape::StrCopyBounded(goal->markerBase, markerBase, sizeof goal->markerBase);
    else
        goal->markerBase[0] = '\0';
    return goal;
}

void GoalSpawnerHunt::GiveHudBack()
{
    if (!hudTaken)
        return;
    if (game::JobManager* jobs = game::GetJobManager())
        jobs->SetGoalCountersSuppressed(true);
    hud::SetAllEnabled(true, false);
    PlayerTargeting29F428(false);
    hudTaken = false;
}

// @func 0x003366F0 GoalSpawnerHunt::~GoalSpawnerHunt
// @func 0x00336A80 GoalSpawnerHunt::~GoalSpawnerHunt (deleting)
GoalSpawnerHunt::~GoalSpawnerHunt()
{
    for (entity::Entity*& entity : spawned) {
        if (entity)
            entity->Release(true);
        entity = nullptr;
    }
    GoalStopListening(this);
    GiveHudBack();
}

// @func 0x0033876C GoalSpawnerHunt::ResetHints
void GoalSpawnerHunt::ResetHints(bool stopHintScript)
{
    if (caught) {
        EntityComponent16Set(caught, true);
        caught->Release(false);
        caught = nullptr;
    }
    for (entity::Entity*& entity : spawned) {
        if (entity)
            entity->Release(true);
        entity = nullptr;
    }
    GiveHudBack();
    Goal::ResetHints(stopHintScript);
}

// @func 0x00338560 GoalSpawnerHunt::CollectMarkerObjects
void GoalSpawnerHunt::CollectMarkerObjects()
{
    markerCount = 0;
    while (markerCount < kMaxMarkerObjects) {
        char name[0x40];
        std::snprintf(name, sizeof name, "%s%02d", markerBase, markerCount + 1);
        void* found = FindLevelTableObject(ape::HashNameNoCase(name));
        if (!found)
            break;
        markerObjects[markerCount++] = found;
    }
}

// @func 0x003386C0 GoalSpawnerHunt::ReapplyHints
void GoalSpawnerHunt::ReapplyHints(Goal* owner)
{
    // The spawners may only exist once the level is loaded.
    for (int i = 0; i < kSpawnerCount; ++i)
        spawners[i] = FindHuntSpawner(spawnerBase, i);
    CollectMarkerObjects();
    Goal::ReapplyHints(owner);
}

// @func 0x0033891C GoalSpawnerHunt::GetProgress
bool GoalSpawnerHunt::GetProgress(GoalProgress* out)
{
    if (progressName == 0)
        return false;
    out->name = progressName;
    out->total = kSpawnerCount;
    out->done = kSpawnerCount - remaining;
    return true;
}

} // namespace goals
} // namespace sly
