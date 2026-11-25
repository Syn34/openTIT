// openTIT - Character spawner: a spawner whose spawns are AI characters.
//
// Built for "Sly_AISpawner" setting objects (vtable 0x00B0ED68, derived from
// sly::Spawner). Besides the base spawner values the setting names up to
// three AI settings objects; each spawn uses one of them (and the entity
// definition that AI setting names), picked at random.
#pragma once

#include "sly/game/SpawnManager.h"

#include <cstdint>

namespace sly {
namespace levels {
namespace ai {

class PropertyBlock;

// Values of a "Sly_AISpawner" setting object (base spawner values plus the
// AI spawner parameters; descriptors named in the comments).
struct AISpawnerSettings : SpawnerSettings {
    const char* spawnObjName = nullptr;          // 0x010AAFF8 "SpawnObjName"
    const char* aiSettingName = nullptr;         // 0x010AB388 "SpawnAISettingName"
    const char* aiSettingNameAlt1 = nullptr;     // 0x010AB3A0 "AISettingNameAlt1"
    const char* aiSettingNameAlt2 = nullptr;     // 0x010AB3B8 "AISettingNameAlt2"
    const char* patrolProxyPointName = nullptr;  // 0x010AB3D0 "PartrolProxyPointName" (sic)
    uint32_t projectorType = 0;                  // 0x010AB3E8 "ProjectorType"
};

class CharacterSpawner : public Spawner {
public:
    static constexpr int kSettingCount = 3;

    CharacterSpawner(WorldObject* spawnPoint, const AISpawnerSettings* settings);
    ~CharacterSpawner() override;

    void Reset() override;
    bool IsCharacterSpawner() const override { return true; }   // 0x00739870 (shared "return 1")
    const char* SpawnDefName() const override;
    const char* SpawnObjectName() const override;

    const AISpawnerSettings* aiSettings() const
    {
        return static_cast<const AISpawnerSettings*>(settings);
    }

    const char* settingNames[kSettingCount] = {};          // +0x5C..+0x64
    uint32_t defNames[kSettingCount] = {};                 // +0x68..+0x70 EntityDefName hash of each AI setting
    int16_t patrolPoint = -1;                              // +0x74 proxy point index
    const PropertyBlock* aiSettingObjects[kSettingCount] = {};  // +0x78..+0x80
    uint32_t projectorType = 0x91CB25B2u;                  // +0x88 NOTE: default hash name unknown
};

} // namespace ai
} // namespace levels
} // namespace sly
