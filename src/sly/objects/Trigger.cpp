// openTIT - Trigger volumes: the base of every trigger entity class.
#include "sly/objects/Trigger.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/entity/ScriptedEntity.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>

namespace sly {
namespace objects {

namespace {
// Message hashes understood by every trigger.
constexpr uint32_t kMsg_Disable = 0x8E69AEC8;   // "Disable"
constexpr uint32_t kMsg_Enable  = 0x7A281983;   // "Enable"

// Collision filter words for the "ColFlags" layers 0..5 (table 0x00AD364C).
constexpr uint8_t kColFlagsFilter[6] = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
} // namespace

bool EntityIsDead(Entity* entity)
{
    return entity->HasTypeBit(kType_Scripted) && static_cast<entity::ScriptedEntity*>(entity)->IsDead();
}

uint32_t EntityId(const Entity* entity)
{
    return entity->nameHash;
}

// ===========================================================================
// Definition
// ===========================================================================

// @func 0x0020BFE8 TriggerDefDesc::Parse
void TriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                           const EntityDefDesc& loader, const EntityDef* parent)
{
    (void)root;   // only used for a warning about a non-trigger parent (stripped)
    InheritDefDesc(loader, *this);

    bool defStartEnabled = true;
    bool defPlayersOnly = false;
    bool defAllEvents = false;
    const char* defPrefix = "Trigger_";
    if (parent && parent->IsClass(kDefClass_Trigger)) {
        const TriggerDef* from = static_cast<const TriggerDef*>(parent);
        defStartEnabled = from->startEnabled;
        defPrefix = from->objPrefix;
        defPlayersOnly = from->playersOnly;
        defAllEvents = from->allEvents;
    }

    const char* prefix = elem->GetString("objPrefix");
    objPrefix = prefix ? prefix : defPrefix;
    elem->GetBool(kAttr_StartEnabled, &startEnabled, defStartEnabled);
    elem->GetBool(kAttr_TriggerPlayersOnly, &playersOnly, defPlayersOnly);
    elem->GetBool(kAttr_TriggerAllEvents, &allEvents, defAllEvents);
    elem->GetBool(kAttr_NavObstacle, &navObstacle, navObstacle);
}

TriggerDef::TriggerDef(const TriggerDefDesc& desc)
    : EntityDef(desc),
      startEnabled(desc.startEnabled),
      playersOnly(desc.playersOnly),
      allEvents(desc.allEvents),
      navObstacle(desc.navObstacle)
{
    ape::StrNCopy(objPrefix, desc.objPrefix, sizeof(objPrefix) - 1);
}

// The trigger subclass definitions without members of their own share this
// destructor (byte-identical console copies).
// @func 0x0020BF60 TriggerDef::~TriggerDef
// @func 0x0075AE14 TriggerDef::~TriggerDef (copy 2)
// @func 0x0075AE9C TriggerDef::~TriggerDef (copy 3)
// @func 0x0075B1E0 TriggerDef::~TriggerDef (copy 4)
// @func 0x0075B2F0 TriggerDef::~TriggerDef (copy 5)
// @func 0x0075B378 TriggerDef::~TriggerDef (copy 6)
// @func 0x0075B488 TriggerDef::~TriggerDef (copy 7)
// @func 0x0075B598 TriggerDef::~TriggerDef (copy 8)
// @func 0x0075B6A8 TriggerDef::~TriggerDef (copy 9)
// @func 0x0075B86C TriggerDef::~TriggerDef (copy 10)
// @func 0x0075B97C TriggerDef::~TriggerDef (copy 11)
// @func 0x0075BA04 TriggerDef::~TriggerDef (copy 12)
// @func 0x0075BA8C TriggerDef::~TriggerDef (copy 13)
// @func 0x0075BDB0 TriggerDef::~TriggerDef (copy 14)
// @func 0x0075BE38 TriggerDef::~TriggerDef (copy 15)
// @func 0x0020BF90 TriggerDef::~TriggerDef (deleting)
// @func 0x0075B6D8 TriggerDef::~TriggerDef (deleting, copy 2)
TriggerDef::~TriggerDef() = default;

// @func 0x0020C164 CreateTriggerDef
EntityDef* CreateTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                            const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Trigger, loader, "Trigger_");
    desc.Parse(root, elem, loader, parent);
    return new TriggerDef(desc);
}

TriggerInstanceDesc::TriggerInstanceDesc(const char* instanceName, TriggerDef* ownerDef)
    : EntityInstanceDesc(instanceName, ownerDef),
      aabbVolume(true),
      startEnabled(ownerDef && ownerDef->startEnabled),
      playersOnly(ownerDef && ownerDef->playersOnly),
      allEvents(ownerDef && ownerDef->allEvents),
      navObstacle(ownerDef && ownerDef->navObstacle)
{
}

bool TriggerPlacementSkips(const EntityPlacement* placement, bool typeMatches)
{
    if (!(placement->flags & kPlace_Trigger))
        return false;
    const TriggerPlacement* trig = static_cast<const TriggerPlacement*>(placement);
    return trig->skipOptional && !typeMatches;
}

// @func 0x0020CEA0 TriggerDef::Spawn
Entity* TriggerDef::Spawn(EntityPlacement* placement)
{
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    Trigger* trigger = new Trigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// ===========================================================================
// Placement
// ===========================================================================

// The definition's flags override the arguments.
// @func 0x00746148 TriggerPlacement::TriggerPlacement
TriggerPlacement::TriggerPlacement(const char* placementName, EntityDef* ownerDef,
                                   TriggerVolume* triggerVolume, bool aabb, bool owns,
                                   bool start, bool players, bool all, bool skip)
    : EntityPlacement(placementName, ownerDef),
      volume(triggerVolume),
      aabbVolume(aabb),
      ownsVolume(owns),
      startEnabled(start),
      playersOnly(players),
      allEvents(all),
      skipOptional(skip)
{
    if (ownerDef) {
        const TriggerDef* trigDef = static_cast<const TriggerDef*>(ownerDef);
        startEnabled = trigDef->startEnabled;
        playersOnly = trigDef->playersOnly;
        allEvents = trigDef->allEvents;
        navObstacle = trigDef->navObstacle;
    }
    flags |= kPlace_Trigger;
}

void ParseTriggerPlacementFlags(ape::xml::XmlElement* elem, TriggerPlacement& placement)
{
    elem->GetBool(kAttr_StartEnabled, &placement.startEnabled, placement.startEnabled);
    elem->GetBool(kAttr_TriggerPlayersOnly, &placement.playersOnly, placement.playersOnly);
    elem->GetBool(kAttr_TriggerAllEvents, &placement.allEvents, placement.allEvents);
    elem->GetBool(kAttr_NavObstacle, &placement.navObstacle, placement.navObstacle);
}

// @func 0x0020D0C8 ParseTriggerPlacement
EntityPlacement* ParseTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                       const char* name)
{
    (void)root;
    TriggerPlacement* placement =
        new TriggerPlacement(name, nullptr, nullptr, true, false, true, false, false, false);
    ParseTriggerPlacementFlags(elem, *placement);
    return placement;
}

// @func 0x0020D264 TriggerPlacement::FillInstanceDesc
void TriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    TriggerInstanceDesc& desc = static_cast<TriggerInstanceDesc&>(baseDesc);
    desc.volume = volume;
    desc.aabbVolume = aabbVolume;
    desc.ownsVolume = ownsVolume;
    desc.startEnabled = startEnabled;
    desc.playersOnly = playersOnly;
    desc.allEvents = allEvents;
    desc.navObstacle = navObstacle;
}

// ===========================================================================
// Entity
// ===========================================================================

// @func 0x0020DB54 CollidableOwner
Entity* CollidableOwner(const void* collidable)
{
    int type = 0;
    TriggerVolume* object = CollidableWorldObject(collidable, &type);
    if ((type != 1 && type != 2) || !object)
        return nullptr;
    return reinterpret_cast<Entity*>(
        (uintptr_t)VolumeGetProperty(object, kVolumeProp_OwnerPtr, 0));
}

// @func 0x0020C868 TriggerBase::TriggerBase
TriggerBase::TriggerBase(const TriggerInstanceDesc& desc, uint16_t capacity, bool applyColFlags)
    : Entity(desc)
{
    m_occupants.reserve(capacity);
    m_enabled = true;   // so that the start state below is applied as a change
    m_playersOnly = desc.playersOnly;
    m_allEvents = desc.allEvents;
    m_navObstacle = desc.navObstacle;
    m_aabbVolume = desc.aabbVolume;
    m_ownsVolume = desc.ownsVolume;
    m_volume = desc.volume;

    ChangeEnabled(desc.startEnabled);
    if (!m_ownsVolume)
        VolumeAddRef(m_volume);

    if (applyColFlags) {
        uint64_t layer = VolumeGetProperty(m_volume, kVolumeProp_ColFlags, 0);
        if ((uint32_t)layer < 6)
            VolumeSetCollisionFilter(m_volume, kColFlagsFilter[(uint32_t)layer]);
    }

    // Replace whatever listeners the level installed with ours, and tag the
    // phantom so that overlap queries can find the trigger.
    VolumeInstallListener(m_volume, this);
    m_listenerInstalled = true;
    if (VolumeHasProperty(m_volume, kVolumeProp_OwnerType))
        VolumeRemoveProperty(m_volume, kVolumeProp_OwnerType);
    VolumeAddProperty(m_volume, kVolumeProp_OwnerType, kVolumeOwner_Trigger);
    if (VolumeHasProperty(m_volume, kVolumeProp_OwnerPtr))
        VolumeRemoveProperty(m_volume, kVolumeProp_OwnerPtr);
    VolumeAddProperty(m_volume, kVolumeProp_OwnerPtr, (uint64_t)(uintptr_t)this);

    if (VolumeIsInWorld(m_volume)) {
        ape::Aabb box = VolumeGetAabb(m_volume);
        ape::AabbCenter(box, transform.translation);
    }
    RegisterNavObstacle();
}

// Trigger classes without members of their own got byte-identical copies of
// this destructor.
// @func 0x0020D338 TriggerBase::~TriggerBase
// @func 0x00212AA0 TriggerBase::~TriggerBase (copy 2, alert)
// @func 0x00215C00 TriggerBase::~TriggerBase (copy 3, camera)
// @func 0x00218BF0 TriggerBase::~TriggerBase (copy 4, camera behavior)
// @func 0x00219C88 TriggerBase::~TriggerBase (copy 5, crawl)
// @func 0x0021AF48 TriggerBase::~TriggerBase (copy 6, damage)
// @func 0x00220C68 TriggerBase::~TriggerBase (copy 7, job)
// @func 0x00221598 TriggerBase::~TriggerBase (copy 8, jump direction)
// @func 0x00759E74 TriggerBase::~TriggerBase (copy 9)
// @func 0x0075A110 TriggerBase::~TriggerBase (copy 10)
// @func 0x0075BB14 TriggerBase::~TriggerBase (copy 11)
// @func 0x0020D478 TriggerBase::~TriggerBase (deleting)
// @func 0x0075A250 TriggerBase::~TriggerBase (deleting, copy 2)
TriggerBase::~TriggerBase()
{
    NavRemoveObstacle(m_navHandle);
}

int TriggerBase::FindOccupant(const Entity* entity) const
{
    auto it = std::find(m_occupants.begin(), m_occupants.end(), entity);
    return it == m_occupants.end() ? -1 : (int)(it - m_occupants.begin());
}

void TriggerBase::QueueEnter(Entity* entity)
{
    if (!entity->IsAlive() || m_enterQueue.count(EntityId(entity)))
        return;
    entity->AddRef();
    if (!m_enabled && entity->HasTypeBit(kType_Character))
        EntityTrackTrigger(entity, this);
    m_enterQueue[EntityId(entity)] = entity;
}

void TriggerBase::QueueExit(Entity* entity)
{
    if (!entity->IsAlive() || m_exitQueue.count(EntityId(entity)))
        return;
    entity->AddRef();
    m_exitQueue[EntityId(entity)] = entity;
}

// @func 0x0020C2BC TriggerBase::RegisterNavObstacle
void TriggerBase::RegisterNavObstacle()
{
    if (!m_navObstacle || !m_volume)
        return;

    ape::Matrix34 frame;
    ape::Vec3 size;
    if (m_aabbVolume) {
        // AABB phantoms: the trigger's own orientation, centred on its bounds.
        ape::Aabb box = GetBounds(true);
        frame = transform;
        ape::AabbCenter(box, frame.translation);
        size = { box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z, 0.0f };
    } else {
        // Shape phantoms: only boxes become obstacles.
        if (VolumeGetShapeKind(m_volume) != kVolumeShape_Box)
            return;
        frame = VolumeGetTransform(m_volume);
        ape::Vec3 half = VolumeGetBoxHalfExtents(m_volume);
        size = { half.x * 2.0f, half.y * 2.0f, half.z * 2.0f, 0.0f };
    }
    NavRemoveObstacle(m_navHandle);
    m_navHandle = NavAddObstacle(frame, size, GetDebugName());
}

void TriggerBase::ChangeEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!enabled)
        NavRemoveObstacle(m_navHandle);
    else
        RegisterNavObstacle();
    if (!enabled) {
        if (!m_processing)
            ExitAll();
        SetBusyFlag(false);
    }
}

// @func 0x0020C7A0 TriggerBase::SetEnabled
void TriggerBase::SetEnabled(bool enabled)
{
    ChangeEnabled(enabled);
}

// @func 0x0020C670 TriggerBase::ExitAll
void TriggerBase::ExitAll()
{
    // Occupants that are still alive are queued to enter again, so that they
    // are reported once the trigger is enabled again.
    while (!m_occupants.empty()) {
        Entity* entity = m_occupants.front();
        OnExit(entity);
        if (entity->IsAlive() && !m_enterQueue.count(EntityId(entity))) {
            entity->AddRef();
            if (!m_enabled && entity->HasTypeBit(kType_Character))
                EntityTrackTrigger(entity, this);
            m_enterQueue[EntityId(entity)] = entity;
        }
        // OnExit removes known occupants; guard against an override that does not.
        if (!m_occupants.empty() && m_occupants.front() == entity)
            m_occupants.erase(m_occupants.begin());
    }
    m_occupants.clear();
}

// @func 0x0020D5D4 TriggerBase::Release
bool TriggerBase::Release(bool immediate)
{
    if (immediate) {
        // Pending reports are dropped without their references (the entities
        // are going away with the level).
        m_exitQueue.clear();
        m_enterQueue.clear();
        while (!m_occupants.empty()) {
            Entity* entity = m_occupants.front();
            OnExit(entity);
            if (!m_occupants.empty() && m_occupants.front() == entity)
                m_occupants.erase(m_occupants.begin());
        }
        if (VolumeIsInWorld(m_volume) && m_ownsVolume)
            VolumeRemoveFromWorld(m_volume);
        if (VolumeHasProperty(m_volume, kVolumeProp_OwnerType))
            VolumeRemoveProperty(m_volume, kVolumeProp_OwnerType);
        if (VolumeHasProperty(m_volume, kVolumeProp_OwnerPtr))
            VolumeRemoveProperty(m_volume, kVolumeProp_OwnerPtr);
        if (m_listenerInstalled) {
            VolumeRemoveListener(m_volume, this);
            m_listenerInstalled = false;
        }
        VolumeRelease(m_volume);
    }
    return Entity::Release(immediate);
}

// @func 0x0020DC6C TriggerBase::ProcessEnterQueue
void TriggerBase::ProcessEnterQueue()
{
    if (m_enterQueue.empty())
        return;
    m_processing = true;
    // Work on a copy: the callbacks may queue new reports.
    std::vector<Entity*> pending;
    pending.reserve(m_enterQueue.size());
    for (auto& entry : m_enterQueue)
        pending.push_back(entry.second);
    m_enterQueue.clear();

    for (Entity* entity : pending) {
        entity->Release(false);
        if (FindOccupant(entity) != -1)
            OnExit(entity);
        if (!m_playersOnly || EntityIsPlayer(entity))
            OnEnter(entity);
    }
    m_processing = false;
    if (!m_enabled)
        ExitAll();
}

// @func 0x0020DE3C TriggerBase::ProcessExitQueue
void TriggerBase::ProcessExitQueue()
{
    if (m_exitQueue.empty())
        return;
    bool wasEnabled = m_enabled;
    m_processing = true;
    std::vector<Entity*> pending;
    pending.reserve(m_exitQueue.size());
    for (auto& entry : m_exitQueue)
        pending.push_back(entry.second);
    m_exitQueue.clear();

    for (Entity* entity : pending) {
        entity->Release(false);
        if (FindOccupant(entity) != -1)
            OnExit(entity);
    }
    m_processing = false;
    if (!m_enabled && wasEnabled)
        ExitAll();
}

void TriggerBase::PruneQueues()
{
    for (auto it = m_enterQueue.begin(); it != m_enterQueue.end();)
        it = it->second->IsAlive() ? std::next(it) : m_enterQueue.erase(it);
    for (auto it = m_exitQueue.begin(); it != m_exitQueue.end();)
        it = it->second->IsAlive() ? std::next(it) : m_exitQueue.erase(it);
}

// @func 0x0020DFD8 TriggerBase::Update
bool TriggerBase::Update(float dt)
{
    if (m_aabbVolume) {
        PruneQueues();
        ProcessExitQueue();
    }
    if (m_enabled) {
        if (m_ownsVolume && !VolumeIsInWorld(m_volume)) {
            VolumeAddToWorld(m_volume);
            ape::Aabb box = VolumeGetAabb(m_volume);
            ape::AabbCenter(box, transform.translation);
        }
        if (!m_aabbVolume) {
            // Shape phantoms are polled: compare the overlap list with the occupants.
            const void* overlaps[64];
            int count = VolumeGetOverlaps(m_volume, overlaps, 64);
            // NOTE: the console walks the occupants by index while OnExit removes
            // entries (swap with last), so an occupant can be skipped for a frame.
            for (int i = 0; i < (int)m_occupants.size(); ++i) {
                Entity* occupant = m_occupants[(size_t)i];
                bool found = false;
                for (int j = 0; j < count && !found; ++j)
                    found = CollidableOwner(overlaps[j]) == occupant;
                if (!found)
                    OnExit(occupant);
            }
            for (int j = 0; j < count; ++j) {
                Entity* owner = CollidableOwner(overlaps[j]);
                if (!owner || FindOccupant(owner) >= 0)
                    continue;
                if (!m_playersOnly || EntityIsPlayer(owner))
                    OnEnter(owner);
            }
        } else {
            ProcessEnterQueue();
        }
    }
    return Entity::Update(dt);
}

// Name of the trigger: the phantom's name (a Havok string pointer whose
// low bit is a flag; VolumeGetName returns it with the bit cleared).
// @func 0x00759E30 TriggerBase::GetName
const char* TriggerBase::GetDebugName()
{
    return VolumeGetName(m_volume);
}

// @func 0x0020E560 TriggerBase::GetBounds
ape::Aabb TriggerBase::GetBounds(bool world)
{
    ape::Aabb box;
    ape::AabbFromPoint(box, ape::world::kWorldZero);
    if (m_volume && VolumeIsInWorld(m_volume)) {
        int type = VolumeGetType(m_volume);
        // NOTE: the console copies an uninitialised box for other phantom types.
        if (type >= 0 && type < 3)
            box = VolumeGetAabb(m_volume);
        if (!world) {
            const ape::Vec3& p = Position();
            ape::Vec3 offset = { -p.x, -p.y, -p.z, 0.0f };
            ape::AabbTranslate(box, offset);
        }
    }
    return box;
}

// @func 0x0020E6DC TriggerBase::HandleMessage
void TriggerBase::HandleMessage(const entity::EntityMessage& msg)
{
    if (msg.id == kMsg_Disable) {
        SetEnabled(false);
        return;
    }
    if (msg.id == kMsg_Enable) {
        SetEnabled(true);
        return;
    }
    Entity::HandleMessage(msg);
}

// @func 0x0020FA20 TriggerBase::ApplyVisibility
void TriggerBase::ApplyVisibility(bool visible)
{
    SetEnabled(visible);
}

// @func 0x0020FA10 TriggerBase::IsDebugDisplayed
bool TriggerBase::IsDebugDisplayed() const
{
    return g_debugDisplayTrigger_All.Get();
}

void TriggerBase::DropPendingReports(Entity* entity)
{
    if (g_debugDisplayTriggerLog.Get()) {
        // The console formats both names for a log line (stripped).
        (void)GetDebugName();
        (void)entity->GetDebugName();
    }
    if (!m_enterQueue.empty()) {
        auto it = m_enterQueue.find(EntityId(entity));
        if (it != m_enterQueue.end()) {
            Entity* queued = it->second;
            if (queued) {
                if (queued->HasTypeBit(kType_Character))
                    EntityUntrackTrigger(queued, this);
                queued->Release(false);
            }
            m_enterQueue.erase(it);
        }
    }
    if (!m_exitQueue.empty()) {
        auto it = m_exitQueue.find(EntityId(entity));
        if (it != m_exitQueue.end()) {
            if (it->second)
                it->second->Release(false);
            m_exitQueue.erase(it);
        }
    }
}

// @func 0x0020E7A4 TriggerBase::OnEnter
void TriggerBase::OnEnter(Entity* entity)
{
    DropPendingReports(entity);

    m_occupants.push_back(entity);
    SetBusyFlag(true);
    entity->AddRef();
    entity::EntitySetState(this, 0, entity::EntityGetState(this, 0) + 1);

    if (m_allEvents || EntityIsPlayer(entity)) {
        bool suppressed = EntityIsPlayer(entity) && EntityIsDead(entity);
        if (!suppressed) {
            uint8_t player = m_allEvents ? 0 : EntityPlayerIndex(entity);
            PostTriggerEvent(kTriggerEvent_Entered, this, player);
            if ((int)m_occupants.size() == GetPlayerCount())
                PostTriggerEvent(kTriggerEvent_AllPlayersEntered, this, EntityPlayerIndex(entity));
        }
    }
    entity->AddWatcher(this);
}

bool TriggerBase::RemoveFromList(Entity* entity, bool notifyEntity)
{
    int index = FindOccupant(entity);
    if (index < 0)
        return false;
    if (notifyEntity)
        entity->RemoveWatcher(this);
    entity->Release(false);
    m_occupants[(size_t)index] = m_occupants.back();
    m_occupants.pop_back();
    SetBusyFlag(!m_occupants.empty());
    entity::EntitySetState(this, 0, entity::EntityGetState(this, 0) - 1);
    return true;
}

// @func 0x0020EBC8 TriggerBase::OnExit
void TriggerBase::OnExit(Entity* entity)
{
    DropPendingReports(entity);
    if (!RemoveFromList(entity, true))
        return;
    if (!m_allEvents && !EntityIsPlayer(entity))
        return;
    PostTriggerEvent(kTriggerEvent_Left, this, m_allEvents ? 0 : EntityPlayerIndex(entity));
}

// @func 0x0020EF60 TriggerBase::RemoveOccupant
void TriggerBase::RemoveOccupant(Entity* entity)
{
    DropPendingReports(entity);
    RemoveFromList(entity, false);
}

// @func 0x0020F240 TriggerBase::ForgetEntity
void TriggerBase::ForgetEntity(Entity* entity)
{
    DropPendingReports(entity);
    EntityUntrackTrigger(entity, this);
}

} // namespace objects
} // namespace sly
