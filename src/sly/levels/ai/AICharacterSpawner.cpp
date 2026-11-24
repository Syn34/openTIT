// openTIT - Character spawner (0x00061810-0x00061DC4).
#include "sly/levels/ai/AICharacterSpawner.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMath.h"
#include "sly/game/WorldInterface.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AIProxyPoints.h"

namespace sly {
namespace levels {
namespace ai {

namespace {

// The setting manager's settings object of that name, with the console's
// warning when it is missing.
const PropertyBlock* LookUpSetting(const char* name)
{
    const PropertyBlock* setting = nullptr;
    const uint32_t hash = ape::HashNameNoCase(name);
    if (hash != 0)
        setting = FindSettingObject(hash);
    if (!setting)
        ape::TempFormat("Warning! Cannot find the setting (%s) from the setting manager!", name);
    return setting;
}

uint32_t DefNameOf(const PropertyBlock* setting)
{
    return ape::HashNameNoCase(PropString(setting, AIProp::Enemy_EntityDefName));
}

} // namespace

// @func 0x00061810 CharacterSpawner::CharacterSpawner
CharacterSpawner::CharacterSpawner(WorldObject* point, const AISpawnerSettings* aiSettings)
    : Spawner(point, aiSettings)
{
    settingNames[0] = aiSettings->aiSettingName;
    aiSettingObjects[0] = settingNames[0] ? LookUpSetting(settingNames[0]) : nullptr;
    if (aiSettingObjects[0]) {
        defNames[0] = DefNameOf(aiSettingObjects[0]);
    } else {
        ape::TempFormat("Fail to find AI Setting (%s) to spawn for spawn point (%s)", settingNames[0],
                        WorldObjectName(spawnPoint));
    }

    // The alternatives are optional.
    for (int i = 1; i < kSettingCount; ++i) {
        settingNames[i] = i == 1 ? aiSettings->aiSettingNameAlt1 : aiSettings->aiSettingNameAlt2;
        if (!settingNames[i])
            continue;
        aiSettingObjects[i] = LookUpSetting(settingNames[i]);
        if (aiSettingObjects[i])
            defNames[i] = DefNameOf(aiSettingObjects[i]);
    }
}

// @func 0x00061B30 CharacterSpawner::~CharacterSpawner
// @func 0x00061B60 CharacterSpawner::~CharacterSpawner (deleting)
CharacterSpawner::~CharacterSpawner() = default;

// @func 0x00061BB8 CharacterSpawner::Reset
// Picks up the projector type and the patrol start point, then clears the
// spawn counters.
void CharacterSpawner::Reset()
{
    projectorType = aiSettings()->projectorType;
    if (AIProxyPointTable* table = GetAIProxyPointTable()) {
        const char* patrolName = aiSettings()->patrolProxyPointName;
        if (const AIProxyPoint* point = table->Find(ape::HashNameNoCase(patrolName)))
            patrolPoint = point->self;
    }
    ClearCounters();
}

// @func 0x00061C94 CharacterSpawner::SpawnDefName
// One in three spawns use the second alternative (when there is one); the
// rest pick evenly between the main setting and the first alternative.
const char* CharacterSpawner::SpawnDefName() const
{
    const PropertyBlock* setting = nullptr;
    if (aiSettingObjects[2] && ape::RandomInt(ape::g_gameRandom, 0, 2) == 0) {
        setting = aiSettingObjects[2];
    } else {
        setting = aiSettingObjects[0];
        if (aiSettingObjects[1] && ape::RandomInt(ape::g_gameRandom, 0, 1) == 1)
            setting = aiSettingObjects[1];
    }
    return setting ? PropString(setting, AIProp::Enemy_EntityDefName) : nullptr;
}

// @func 0x00061D9C CharacterSpawner::SpawnObjectName
const char* CharacterSpawner::SpawnObjectName() const
{
    return aiSettings()->spawnObjName;
}

} // namespace ai
} // namespace levels
} // namespace sly
