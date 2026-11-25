// openTIT - Spawners, spawner groups and the spawn manager.
//
// A Spawner is created from a "Spawner Setting" level object and bound to a
// spawn point instance. It creates world objects (AI characters, pickups...)
// at that point, limited by how many may be alive at once and how many may
// be spawned in total. Spawners can be collected into a SpawnGroup that
// decides which of its spawners fires. The SpawnManager (global at
// 0x00F65BE8) owns every spawner and group and ticks the active ones.
#pragma once

#include <cstdint>
#include <vector>

namespace sly {

class WorldObject;
class SpawnGroup;

// How a spawner/group places its objects (setting property hashed by name).
// NOTE: the two non-default modes are only known by their hashes
// (0x74BC7BAD and 0x95E51416).
enum SpawnPlacement : int32_t {
    kPlacement_Default = 0,
    kPlacement_74BC7BAD = 1,
    kPlacement_95E51416 = 2,
};

// Values of the "Spawner Setting" level object. The console reads them from
// the setting's property block on demand; the property descriptors are the
// globals named in the comments. NOTE: names are inferred from use.
struct SpawnerSettings {
    bool useAltSpawnPoint = false;      // 0x010AB0F8 spawn at a linked object instead
    bool useGroupSpawnPoint = false;    // 0x010AB128 ...preferring the group's object
    bool flatSpawn = false;             // 0x010AB110 clears the spawn height offset
    float maxPlayerDistance = 0.0f;     // 0x010AB170 0 = no limit
    float minPlayerDistance = 0.0f;     // 0x010AB188 0 = use the AI tuning default
    bool trackSpawned = true;           // 0x010AB1B8 keep a list of live objects
    float respawnDelay = 0.0f;          // 0x010AB0B0
    float initialDelay = 0.0f;          // 0x010AB038
    float randomDelay = 0.0f;           // 0x010AB050 extra random delay range
    float activateChance = 100.0f;      // 0x010AB068 percent
    uint32_t placementHash = 0;         // 0x010AB0E0
    int32_t maxAlive = 1;               // 0x010AB080
    int32_t spawnLimit = 0;             // 0x010AB098 total spawns, 0 = see 'infinite'
    bool infinite = false;              // 0x010AB140
    const char* triggerName = nullptr;  // 0x010AB2F0 NOTE: hashed into nameHash
    const char* secondName = nullptr;   // 0x010AB308
    const char* linkedObjectName = nullptr; // 0x010AB0C8
    const char* entityDefName = nullptr;    // 0x010AB310 definition to spawn
};

// Values of the "Entity Spawn Group Setting" level object.
struct SpawnGroupSettings {
    uint32_t selectionHash = 0;         // 0x010ABD38
    int32_t maxActive = 0;              // 0x010ABD80 spawners firing at once, 0 = all
    int32_t maxTotal = 0;               // 0x010ABD68 total spawns, 0 = unlimited
    uint32_t placementHash = 0;         // 0x010ABD50
};

// Called when a spawned object goes away; installed by the group owner.
using SpawnCallback = void (*)(class Spawner* spawner, int32_t spawnerIndex, void* userData);

class Spawner {
public:
    enum Flags : uint8_t {
        kFlag_Suspended   = 0x04,  // set by default; cleared when activated
        kFlag_Pending     = 0x08,  // deactivated by the manager, reactivate later
        kFlag_OffscreenOnly = 0x10, // only spawn while the point is not in view
        kFlag_AutoReset   = 0x20,
        kFlag_ResetQueued = 0x40,
        kFlag_Active      = 0x80,
    };

    Spawner();
    // 'spawnPoint' is the placed instance, 'settings' the setting object.
    Spawner(WorldObject* spawnPoint, const SpawnerSettings* settings);
    virtual ~Spawner();

    // Virtual interface (console vtable 0x00B21988).
    virtual void Reset();                       // slot 3
    virtual void Start();                       // slot 4
    virtual bool IsCharacterSpawner() const;    // slot 5 NOTE: guessed meaning
    virtual bool Activate();                    // slot 6
    virtual void Deactivate(bool immediate);    // slot 7
    // Slot 10: name of the entity definition to spawn (base 0x004562B8: the
    // setting's EntityDefName).
    virtual const char* SpawnDefName() const { return settings ? settings->entityDefName : nullptr; }
    // Slot 11: name of an object to spawn instead (base 0x0077E8A4: none).
    virtual const char* SpawnObjectName() const { return nullptr; }

    // Object the spawn is placed relative to (null: use the spawn point).
    WorldObject* AltSpawnObject() const;
    // The spawn point's transform block (spawnPoint + 0x50).
    const void* SpawnTransform() const;
    // Player distance gate (min/max distance settings).
    bool IsPlayerInRange() const;
    // Removes every tracked live object; optionally kills them first.
    void DespawnAll(bool kill);
    // Per-frame tick while in the manager's active list. Returns false once
    // the spawner can be dropped from the list.
    bool Update(float dt);
    // Resets the spawn counters and clears the transient flags.
    void ClearCounters();
    // Queues a reset (bit 6) and marks the spawner suspended.
    void QueueReset(bool autoReset);
    // Picks the delay before the first spawn after activation.
    void ChooseInitialDelay();
    // Handles a message; message 0x879F2BDA with our trigger hash stops us.
    void OnMessage(uint32_t messageHash, uint32_t messageArg);
    // Runs the owner callback for this spawner.
    void NotifyOwner();
    // Spawns one object now. TODO(0x00452058).
    WorldObject* SpawnOne();

    const SpawnerSettings* settings = nullptr;  // +0x30
    SpawnGroup* group = nullptr;                // +0x08
    float delayTimer = 0.0f;                    // +0x0C
    float respawnTimer = 0.0f;                  // +0x10
    std::vector<WorldObject*> spawned;          // +0x14/+0x18 live objects
    int32_t maxAlive = 0;                       // +0x1C
    int32_t totalSpawned = 0;                   // +0x20
    int32_t spawnLimit = 0;                     // +0x24 0 = unlimited
    uint32_t triggerHash = 0;                   // +0x28
    uint32_t secondHash = 0;                    // +0x2C
    SpawnPlacement placement = kPlacement_Default; // +0x34
    WorldObject* linkedObject = nullptr;        // +0x38
    WorldObject* spawnPoint = nullptr;          // +0x3C
    uint32_t ownerValueA = 0;                   // +0x40 set by SpawnGroup::SetSpawnerValues
    uint32_t ownerValueB = 0;                   // +0x44
    Spawner* nextInGroup = nullptr;             // +0x48
    int32_t indexInGroup = -1;                  // +0x4C
    uint8_t flags = 0;                          // +0x58
};

class SpawnGroup {
public:
    SpawnGroup(const SpawnGroupSettings* settings, WorldObject* levelObject);
    ~SpawnGroup() = default;

    // Activates every spawner; with 'tick' also runs one zero-length update.
    void Activate(bool tick);
    // Initial spawn when the level starts.
    void Start();
    // Clears counters and resets every spawner.
    void Reset();
    // Deactivates the group and its spawners.
    void Deactivate(bool immediate);
    // Despawns the live objects of every tracking spawner.
    void DespawnAll(bool kill);
    // Activates the spawner with the given index.
    void ActivateSpawner(int32_t index);
    // Fires the initial set of spawners (all of them, or 'maxActive' picks).
    void SpawnInitial();
    Spawner* FindSpawner(int32_t index) const;
    void SetSpawnerValues(uint32_t a, uint32_t b);
    // Adds a spawner at the head of the list.
    void AddSpawner(Spawner* spawner);
    // Chooses the next spawner to fire. TODO(0x00453A68).
    Spawner* PickSpawner(bool initial);

    const SpawnGroupSettings* settings = nullptr; // +0x00
    int32_t selection = 0;                      // +0x04
    int32_t totalSpawned = 0;                   // +0x08
    int32_t field_0C = 0;                       // +0x0C NOTE: compared with totalSpawned
    int32_t aliveCount = 0;                     // +0x10
    int32_t field_14 = 0;                       // +0x14
    int32_t field_18 = 0;                       // +0x18
    bool active = true;                         // +0x1C
    Spawner* firstSpawner = nullptr;            // +0x20
    int32_t spawnerCount = 0;                   // +0x24
    WorldObject* levelObject = nullptr;         // +0x28
    SpawnPlacement placement = kPlacement_Default; // +0x2C
    SpawnCallback onSpawn = nullptr;            // +0x38 NOTE: called by Spawner::NotifyOwner
    void* onSpawnData = nullptr;                // +0x3C
};

class SpawnManager {
public:
    static constexpr int kMaxSpawners = 0x200;
    static constexpr int kMaxGroups = 0x32;

    SpawnManager() = default;
    ~SpawnManager();

    void Start();
    void Clear();
    void Reset();
    bool AddSpawner(Spawner* spawner);
    bool AddGroup(SpawnGroup* group);
    // Reactivates spawners deactivated by DeactivateOutOfRange().
    void ReactivatePending();
    // Deactivates active spawners that report themselves out of range.
    void DeactivateOutOfRange();
    void Update(float dt);
    // Adds a spawner to the active list.
    bool AddActive(Spawner* spawner);
    Spawner* FindSpawner(uint32_t key) const;
    // Group registered under `key` (sorted map at +0x1C), or null.
    SpawnGroup* FindGroup(uint32_t key) const
    {
        for (const GroupEntry& entry : groups)
            if (entry.key == key)
                return entry.group;
        return nullptr;
    }

    bool busy = false;                          // +0x00 inside a bulk operation
    struct Entry { uint32_t key; Spawner* spawner; };
    struct GroupEntry { uint32_t key; SpawnGroup* group; };
    std::vector<Entry> spawners;                // +0x04
    std::vector<GroupEntry> groups;             // +0x1C
    std::vector<Spawner*> active;               // +0x34 (handle list on the console)
};

// Despawns the live objects of a spawner, then resets it (0x0045477C, the
// Reset override of tracking spawners).
void SpawnerDespawnAndReset(Spawner& spawner);

// The global spawn manager (0x00F65BE8).
extern SpawnManager* g_spawnManager;

// Level loader callbacks: create objects from setting elements.
void LoadSpawnerSetting(const SpawnerSettings* settings, const char* settingName,
                        const char* spawnPointName);
void LoadSpawnGroupSetting(const SpawnGroupSettings* settings, const char* settingName,
                           const char* groupObjectName,
                           const std::vector<uint32_t>& spawnerKeys);

} // namespace sly
