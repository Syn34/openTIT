// openTIT - Spawners, spawner groups and the spawn manager.
#include "sly/game/SpawnManager.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMath.h"
#include "sly/game/SlyGame.h"
#include "sly/game/WorldInterface.h"
#include "sly/tuning/AiSpawnTuning.h"

#include <algorithm>

namespace sly {

SpawnManager* g_spawnManager = nullptr;

namespace {

// Message sent to spawners by triggers; carries the trigger's name hash.
constexpr uint32_t kMsg_StopSpawner = 0x879F2BDAu;

SpawnPlacement PlacementFromHash(uint32_t hash)
{
    if (hash == 0x74BC7BADu)
        return kPlacement_74BC7BAD;
    if (hash == 0x95E51416u)
        return kPlacement_95E51416;
    return kPlacement_Default;
}

const SpawnerSettings& DefaultSpawnerSettings()
{
    static const SpawnerSettings s_defaults{};
    return s_defaults;
}

bool ManagerBusy()
{
    return g_spawnManager && g_spawnManager->busy;
}

} // namespace

// ---------------------------------------------------------------------------
// Spawner
// ---------------------------------------------------------------------------

// @func 0x004558BC Spawner::Spawner
Spawner::Spawner()
    : settings(&DefaultSpawnerSettings())
{
}

// @func 0x004551C8 Spawner::Spawner
Spawner::Spawner(WorldObject* point, const SpawnerSettings* setting)
    : settings(setting ? setting : &DefaultSpawnerSettings()), spawnPoint(point)
{
    placement = PlacementFromHash(settings->placementHash);
    flags = 0;

    maxAlive = settings->maxAlive;
    int32_t limit = settings->spawnLimit;
    if (limit == 0 && !settings->infinite)
        limit = 1;
    if (limit == 1 && settings->infinite)
        limit = 0;
    else if (limit > 0 && limit < maxAlive)
        limit = maxAlive;
    spawnLimit = limit;

    spawned.reserve(maxAlive > 0 ? (size_t)maxAlive : 0);
    triggerHash = ape::HashNameNoCase(settings->triggerName);
    secondHash = ape::HashNameNoCase(settings->secondName);
    if (settings->linkedObjectName)
        linkedObject = FindLevelInstance(ape::HashNameNoCase(settings->linkedObjectName));
}

// @func 0x00455974 Spawner::~Spawner
// @func 0x00455A94 Spawner::~Spawner (copy 2)
// @func 0x004559F8 Spawner::~Spawner (deleting)
Spawner::~Spawner() = default;

// TODO(0x00452058 Spawner::SpawnOne): creates the object from the spawn
// point's template, places it and registers it with the group.
WorldObject* Spawner::SpawnOne()
{
    return nullptr;
}

// NOTE: the base Reset/Start/IsCharacterSpawner bodies live in other
// translation units on the console; these are the observable defaults.
void Spawner::Reset()
{
    ClearCounters();
}

void Spawner::Start() {}

bool Spawner::IsCharacterSpawner() const
{
    return false;
}

// @func 0x00451E08 Spawner::AltSpawnObject
WorldObject* Spawner::AltSpawnObject() const
{
    if (!settings->useAltSpawnPoint)
        return nullptr;
    if (settings->useGroupSpawnPoint && group && group->levelObject)
        return group->levelObject;
    return spawnPoint;
}

// @func 0x00452024 Spawner::SpawnTransform
// NOTE: the console returns the address of a block 0x50 bytes into the
// spawn point object; on PC the spawn point itself identifies it.
const void* Spawner::SpawnTransform() const
{
    return spawnPoint;
}

// @func 0x00453350 Spawner::IsPlayerInRange
bool Spawner::IsPlayerInRange() const
{
    float minDist = settings->minPlayerDistance;
    const float maxDist = settings->maxPlayerDistance;
    if (minDist == 0.0f && IsCharacterSpawner() && spawnLimit != 1)
        minDist = g_defaultNoSpawnDist;

    if (maxDist <= 0.0f && minDist <= 0.0f)
        return true;

    WorldObject* player = g_game ? g_game->playerObject : nullptr;
    if (!player)
        return false;

    WorldObject* point = AltSpawnObject();
    if (!point)
        point = spawnPoint;

    const ape::Vec3 p = CharacterPosition(player);
    const ape::Vec3 s = InstancePosition(point);
    const float dx = p.x - s.x;
    const float dy = p.y - s.y;
    const float dz = p.z - s.z;
    const float distSq = dx * dx + dy * dy + dz * dz;

    if (maxDist > 0.0f && distSq >= maxDist * maxDist)
        return false;
    if (minDist > 0.0f && distSq <= minDist * minDist)
        return false;
    return true;
}

// @func 0x00453578 Spawner::DespawnAll
void Spawner::DespawnAll(bool kill)
{
    if (!settings->trackSpawned)
        return;
    std::vector<WorldObject*> objects;
    objects.swap(spawned);
    for (WorldObject* object : objects) {
        if (!SpawnedIsAlive(object))
            continue;
        if (kill)
            SpawnedKill(object);
        --totalSpawned;
        SpawnedRelease(object);
    }
}

// @func 0x004536C0 Spawner::Update
bool Spawner::Update(float dt)
{
    if (flags & kFlag_ResetQueued) {
        flags &= ~kFlag_ResetQueued;
        DespawnAll(false);
    }
    if (!(flags & kFlag_Active))
        return false;

    if (delayTimer > 0.0f) {
        delayTimer -= dt;
        return true;
    }
    if (respawnTimer > 0.0f) {
        respawnTimer -= dt;
        return true;
    }
    // A group with an active-spawner budget decides when we fire.
    if (group && group->settings && group->settings->maxActive >= 1)
        return true;

    const bool roomForMore = (int32_t)spawned.size() < maxAlive || !settings->trackSpawned;
    const bool underLimit = spawnLimit == 0 || totalSpawned < spawnLimit;
    if (!roomForMore || !underLimit)
        return true;

    if ((flags & kFlag_OffscreenOnly) && !ManagerBusy()) {
        WorldObject* point = spawnPoint;
        if (point && IsPointInCameraView(InstancePosition(point)))
            return true;
    }

    if (SpawnGateAllows() && IsPlayerInRange()) {
        SpawnOne();
        respawnTimer = settings->respawnDelay;
    }
    return true;
}

// @func 0x0045477C Spawner::DespawnAndReset
// Used as the Reset override of tracking spawners.
void SpawnerDespawnAndReset(Spawner& spawner)
{
    spawner.DespawnAll(false);
    spawner.Reset();
}

// @func 0x00455B7C Spawner::ClearCounters
void Spawner::ClearCounters()
{
    delayTimer = 0.0f;
    respawnTimer = 0.0f;
    spawned.clear();
    totalSpawned = 0;
    flags &= 0x73;  // clears Active, Pending and Suspended
}

// @func 0x00455F34 Spawner::QueueReset
void Spawner::QueueReset(bool autoReset)
{
    flags = (uint8_t)((flags & 0x3F) | kFlag_Suspended | (autoReset ? kFlag_ResetQueued : 0));
}

// @func 0x00455C78 Spawner::ChooseInitialDelay
void Spawner::ChooseInitialDelay()
{
    float delay = 0.0f;
    if (settings->initialDelay > 0.0f)
        delay = settings->initialDelay;
    delayTimer = delay;
    if (settings->randomDelay > 0.0f)
        delayTimer = settings->randomDelay * ape::RandomUnit(ape::g_gameRandom) + delay;
}

// @func 0x00455D68 Spawner::Activate
bool Spawner::Activate()
{
    if (flags & kFlag_Active)
        return false;
    if (settings->activateChance < 100.0f) {
        const float roll = ape::RandomUnit(ape::g_gameRandom) * 100.0f;
        if (settings->activateChance < roll)
            return false;
    }
    flags = (uint8_t)((flags & ~kFlag_Suspended) | kFlag_Active);
    ChooseInitialDelay();
    if (g_spawnManager && !g_spawnManager->AddActive(this))
        ape::TempFormat("Cannot activate spawner (%s) because out of active spawner space!",
                        WorldObjectName(spawnPoint));
    return true;
}

void Spawner::Deactivate(bool immediate)
{
    (void)immediate;
    flags &= ~kFlag_Active;
}

// @func 0x00455B18 Spawner::OnMessage
void Spawner::OnMessage(uint32_t messageHash, uint32_t messageArg)
{
    if (messageHash == kMsg_StopSpawner && messageArg == triggerHash)
        Deactivate(false);
}

// @func 0x00456258 Spawner::NotifyOwner
void Spawner::NotifyOwner()
{
    if (group && group->onSpawn)
        group->onSpawn(this, indexInGroup, group->onSpawnData);
}

// ---------------------------------------------------------------------------
// SpawnGroup
// ---------------------------------------------------------------------------

// @func 0x004555F4 SpawnGroup::SpawnGroup
SpawnGroup::SpawnGroup(const SpawnGroupSettings* setting, WorldObject* object)
    : settings(setting), levelObject(object)
{
    static const SpawnGroupSettings s_defaults{};
    if (!settings)
        settings = &s_defaults;

    if (settings->selectionHash == 0x13234CFFu)
        selection = 1;
    else if (settings->selectionHash == 0xFD81u)
        selection = 2;
    else
        selection = 0;
    if (settings->maxActive == 0)
        selection = 2;

    placement = PlacementFromHash(settings->placementHash);
}

void SpawnGroup::AddSpawner(Spawner* spawner)
{
    spawner->group = this;
    spawner->nextInGroup = firstSpawner;
    spawner->indexInGroup = firstSpawner ? firstSpawner->indexInGroup + 1 : 0;
    firstSpawner = spawner;
    ++spawnerCount;
}

// TODO(0x00453A68 SpawnGroup::PickSpawner): weighted/random choice of the
// next spawner according to 'selection'.
Spawner* SpawnGroup::PickSpawner(bool initial)
{
    (void)initial;
    return nullptr;
}

// @func 0x00454220 SpawnGroup::Activate
void SpawnGroup::Activate(bool tick)
{
    if (spawnerCount == 0)
        ape::TempFormat("Spawn group %s contains no spawners", WorldObjectName(levelObject));
    active = true;
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup) {
        s->Activate();
        if (tick)
            s->Update(0.0f);
    }
}

// @func 0x00454320 SpawnGroup::Start
void SpawnGroup::Start()
{
    if (!active || placement != kPlacement_Default)
        return;
    if (spawnerCount == 0)
        ape::TempFormat("Spawn group %s contains no spawners", WorldObjectName(levelObject));
    active = true;
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup)
        s->Activate();

    int32_t count = settings->maxActive;
    if (settings->maxTotal == 0 && count == 0)
        count = spawnerCount;
    for (int32_t i = 0; i < count; ++i) {
        if (Spawner* s = PickSpawner(true))
            s->SpawnOne();
    }
}

// @func 0x0045488C SpawnGroup::Reset
void SpawnGroup::Reset()
{
    totalSpawned = 0;
    aliveCount = 0;
    active = true;
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup)
        SpawnerDespawnAndReset(*s);
}

// @func 0x004562F4 SpawnGroup::Deactivate
void SpawnGroup::Deactivate(bool immediate)
{
    active = false;
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup)
        s->Deactivate(immediate);
}

// @func 0x004563A0 SpawnGroup::DespawnAll
void SpawnGroup::DespawnAll(bool kill)
{
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup)
        s->DespawnAll(kill);
}

// @func 0x00456500 SpawnGroup::ActivateSpawner
void SpawnGroup::ActivateSpawner(int32_t index)
{
    if (Spawner* s = FindSpawner(index))
        s->Activate();
}

// @func 0x00456568 SpawnGroup::SpawnInitial
void SpawnGroup::SpawnInitial()
{
    if (settings->maxActive == 0) {
        for (Spawner* s = firstSpawner; s; s = s->nextInGroup) {
            s->Activate();
            s->Update(0.0f);
        }
        return;
    }
    for (int32_t i = 0; i < settings->maxActive; ++i) {
        if (Spawner* s = PickSpawner(true))
            s->SpawnOne();
    }
}

// @func 0x0045666C SpawnGroup::FindSpawner
Spawner* SpawnGroup::FindSpawner(int32_t index) const
{
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup) {
        if (s->indexInGroup == index)
            return s;
    }
    return nullptr;
}

// @func 0x0045669C SpawnGroup::SetSpawnerValues
void SpawnGroup::SetSpawnerValues(uint32_t a, uint32_t b)
{
    for (Spawner* s = firstSpawner; s; s = s->nextInGroup) {
        s->ownerValueA = a;
        s->ownerValueB = b;
    }
}

// ---------------------------------------------------------------------------
// SpawnManager
// ---------------------------------------------------------------------------

SpawnManager::~SpawnManager()
{
    Clear();
}

// @func 0x004544B0 SpawnManager::Start
void SpawnManager::Start()
{
    busy = true;
    for (Entry& e : spawners)
        e.spawner->Start();
    for (GroupEntry& g : groups)
        g.group->Start();
    Update(0.0f);
    busy = false;
}

// @func 0x004545B0 SpawnManager::Clear
void SpawnManager::Clear()
{
    for (GroupEntry& g : groups)
        delete g.group;
    groups.clear();
    for (Entry& e : spawners)
        delete e.spawner;
    spawners.clear();
    active.clear();
}

// @func 0x004549D0 SpawnManager::Reset
void SpawnManager::Reset()
{
    busy = true;
    for (Entry& e : spawners)
        e.spawner->Reset();
    for (GroupEntry& g : groups)
        g.group->Reset();
    active.clear();
    busy = false;
}

bool SpawnManager::AddSpawner(Spawner* spawner)
{
    if ((int)spawners.size() >= kMaxSpawners) {
        ape::TempFormat("Number of spawner is more than %d, please increase the max number of "
                        "the spawner in SpawnManager",
                        kMaxSpawners);
        return false;
    }
    spawners.push_back(Entry{WorldObjectKey(spawner->spawnPoint), spawner});
    return true;
}

// @func 0x00454CAC SpawnManager::AddGroup
bool SpawnManager::AddGroup(SpawnGroup* group)
{
    if ((int)groups.size() >= kMaxGroups) {
        ape::TempFormat("Run out of space for spawner group (%d), spawner group (%s) will not "
                        "add to the system!",
                        kMaxGroups, WorldObjectName(group->levelObject));
        return false;
    }
    groups.push_back(GroupEntry{WorldObjectKey(group->levelObject), group});
    return true;
}

// @func 0x00454DBC SpawnManager::ReactivatePending
void SpawnManager::ReactivatePending()
{
    busy = true;
    for (Entry& e : spawners) {
        Spawner* s = e.spawner;
        if (s->flags & Spawner::kFlag_Pending) {
            s->flags &= ~Spawner::kFlag_Pending;
            s->Activate();
        }
    }
    busy = false;
}

// @func 0x00454E88 SpawnManager::DeactivateOutOfRange
void SpawnManager::DeactivateOutOfRange()
{
    for (Entry& e : spawners) {
        Spawner* s = e.spawner;
        if (!(s->flags & Spawner::kFlag_Active))
            continue;
        if (!s->IsCharacterSpawner() && !(s->flags & Spawner::kFlag_AutoReset))
            continue;

        std::vector<WorldObject*> objects;
        objects.swap(s->spawned);
        s->flags |= Spawner::kFlag_Pending;
        for (WorldObject* object : objects) {
            if (SpawnedIsAlive(object) && !SpawnedIsPersistent(object)) {
                --s->totalSpawned;
                SpawnedRelease(object);
            }
        }
        s->Deactivate(false);
    }
}

// @func 0x00453FC4 SpawnManager::Update
void SpawnManager::Update(float dt)
{
    // Groups with a budget fire one spawner per tick while under budget.
    for (GroupEntry& entry : groups) {
        SpawnGroup* g = entry.group;
        if (!g || g->aliveCount >= g->settings->maxActive)
            continue;
        if (g->settings->maxTotal != 0 && g->totalSpawned >= g->settings->maxTotal)
            continue;
        Spawner* s = g->PickSpawner(false);
        if (s && s->IsPlayerInRange())
            s->SpawnOne();
    }

    // Tick the active spawners; drop the ones that are done. Spawners added
    // while ticking are picked up next frame.
    const size_t count = active.size();
    std::vector<Spawner*> keep;
    keep.reserve(active.size());
    for (size_t i = 0; i < count; ++i) {
        Spawner* s = active[i];
        if (s->Update(dt))
            keep.push_back(s);
    }
    for (size_t i = count; i < active.size(); ++i)
        keep.push_back(active[i]);
    active.swap(keep);
}

bool SpawnManager::AddActive(Spawner* spawner)
{
    active.push_back(spawner);
    return true;
}

Spawner* SpawnManager::FindSpawner(uint32_t key) const
{
    for (const Entry& e : spawners) {
        if (e.key == key)
            return e.spawner;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Level loaders
// ---------------------------------------------------------------------------

// TODO(0x00455038 LoadCharacterSpawnerSetting): same as LoadSpawnerSetting
// but constructs the character spawner subclass (constructor at 0x00061810).

// @func 0x00455468 LoadSpawnerSetting
void LoadSpawnerSetting(const SpawnerSettings* settings, const char* settingName,
                        const char* spawnPointName)
{
    if (!spawnPointName || !g_spawnManager)
        return;
    WorldObject* point = FindLevelInstance(ape::HashNameNoCase(spawnPointName));
    if (!point) {
        ape::TempFormat("Cannot find the spawn point object instance (%s) to associate with "
                        "Spawner Setting (%s)",
                        spawnPointName, settingName);
        return;
    }
    Spawner* spawner = new Spawner(point, settings);
    if (!g_spawnManager->AddSpawner(spawner))
        delete spawner;
}

// @func 0x00455728 LoadSpawnGroupSetting
void LoadSpawnGroupSetting(const SpawnGroupSettings* settings, const char* settingName,
                           const char* groupObjectName, const std::vector<uint32_t>& spawnerKeys)
{
    if (!groupObjectName || !g_spawnManager)
        return;
    WorldObject* object = FindLevelInstance(ape::HashNameNoCase(groupObjectName));
    if (!object) {
        ape::TempFormat("Cannot find the spawn group object instance (%s) to associate with "
                        "Entity Spawn Group Setting (%s)",
                        groupObjectName, settingName);
        return;
    }
    SpawnGroup* group = new SpawnGroup(settings, object);
    g_spawnManager->AddGroup(group);
    for (uint32_t key : spawnerKeys) {
        Spawner* spawner = g_spawnManager->FindSpawner(key);
        if (spawner && !spawner->group)
            group->AddSpawner(spawner);
    }
}

} // namespace sly
