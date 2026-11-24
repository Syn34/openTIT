// openTIT - Trigger volumes: the base of every trigger entity class.
//
// A trigger is an entity wrapped around a physics phantom from the level data.
// Entities whose collidables overlap the phantom are reported (through the
// phantom listener, or by polling the overlap list) and become the trigger's
// occupants; derived classes react in OnEnter/OnExit. Triggers can also
// register their volume as a navigation obstacle and post "entered"/"left"
// game events for scripts.
//
// Every trigger class comes as three console objects:
//   * a definition (TriggerDef, vtable 0x00B19510) parsed from the entity XML,
//   * a placement (TriggerPlacement, vtable 0x00B19540) parsed per placed instance,
//   * the entity (TriggerBase, vtable 0x00B19558; plain Trigger 0x00B19698).
#pragma once

#include "sly/objects/ObjectsBase.h"
#include "sly/objects/ObjectsHooks.h"

#include <cstdint>
#include <map>
#include <vector>

namespace sly {
namespace objects {

// Trigger attribute name hashes (HashNameNoCase).
constexpr uint32_t kAttr_StartEnabled = 0xBEF25C9F;   // "StartEnabled"
constexpr uint32_t kAttr_NavObstacle  = 0x778A1C92;   // "NavObstacle"
// NOTE: the two flags below have no recovered name. The first restricts the
// trigger to players, the second posts the enter/leave events for every entity
// (not only players) with player index 0.
constexpr uint32_t kAttr_TriggerPlayersOnly = 0x1EF4F9F5;
constexpr uint32_t kAttr_TriggerAllEvents   = 0x2D407057;

// ---------------------------------------------------------------------------
// Definition
// ---------------------------------------------------------------------------
struct TriggerDefDesc : EntityDefDesc {
    const char* objPrefix;         // 0x4C prefix of the level objects of this class
    bool startEnabled = true;      // 0x50
    bool playersOnly = false;      // 0x51
    bool allEvents = false;        // 0x52
    bool navObstacle = false;      // 0x53

    TriggerDefDesc(uint32_t bits, const EntityDefDesc& loader, const char* prefix)
        : EntityDefDesc(bits, loader), objPrefix(prefix) {}

    // Reads the trigger attributes. Defaults come from a "from" trigger definition
    // when there is one (0x0020BFE8).
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class TriggerDef : public EntityDef {
public:
    explicit TriggerDef(const TriggerDefDesc& desc);
    ~TriggerDef() override;
    ObjEntity* Create(EntityPlacement* placement) override;

    char objPrefix[0x20] = {};     // 0x8C
    bool startEnabled = true;      // 0xAC
    bool playersOnly = false;      // 0xAD
    bool allEvents = false;        // 0xAE
    bool navObstacle = false;      // 0xAF
};

// Definition loader entry for <Trigger> (0x0020C164).
EntityDef* CreateTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                            const EntityDefDesc& loader, const EntityDef* parent);

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------
class TriggerPlacement : public EntityPlacement {
public:
    // The flag defaults come from the definition when one is given,
    // otherwise from the arguments.
    TriggerPlacement(const char* placementName, EntityDef* ownerDef, TriggerVolume* volume,
                     bool aabbVolume, bool ownsVolume, bool startEnabled, bool playersOnly,
                     bool allEvents, bool skipOptional);
    // @func 0x00746060 TriggerPlacement::~TriggerPlacement
    // @func 0x00759DA8 TriggerPlacement::~TriggerPlacement (deleting)
    ~TriggerPlacement() override = default;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    TriggerVolume* volume = nullptr;   // 0xB4 phantom assigned by the level loader
    // 0xB8 bit field (console MSB first).
    bool aabbVolume = false;           // bit 7: the volume is an AABB phantom (listener reports)
    bool ownsVolume = false;           // bit 6: the trigger adds the phantom to the world itself
    bool startEnabled = true;          // bit 5
    bool playersOnly = false;          // bit 4
    bool allEvents = false;            // bit 3
    bool skipOptional = false;         // bit 2: NOTE: optional sound triggers skip creation
    bool navObstacle = false;          // bit 1
};

// Placement loader entry for <Trigger>/<TriggerWeather> (0x0020D0C8).
EntityPlacement* ParseTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                       const char* name);
// Reads the four trigger flags of a placement element (shared by the loaders).
void ParseTriggerPlacementFlags(ape::xml::XmlElement* elem, TriggerPlacement& placement);

// Instance description of triggers (base description + 0x0A bytes).
struct TriggerInstanceDesc : EntityInstanceDesc {
    // The flag defaults come from the definition (inlined in every Create).
    TriggerInstanceDesc(const char* instanceName, TriggerDef* ownerDef);
    TriggerVolume* volume = nullptr;   // 0x90
    bool aabbVolume = false;           // 0x94
    bool ownsVolume = false;           // 0x95
    bool startEnabled = true;          // 0x96
    bool playersOnly = false;          // 0x97
    bool allEvents = false;            // 0x98
    bool navObstacle = false;          // 0x99
};

// ---------------------------------------------------------------------------
// Entity
// ---------------------------------------------------------------------------
class TriggerBase : public ObjEntity {
public:
    // `capacity` sizes the occupant list; `applyColFlags` takes the collision
    // layer from the volume's "ColFlags" property (0x0020C868).
    TriggerBase(const TriggerInstanceDesc& desc, uint16_t capacity, bool applyColFlags);
    ~TriggerBase() override;

    void Update(float dt) override;
    void Release(bool immediate) override;
    const char* GetName() const override;
    ape::Aabb GetBounds(bool world) const override;
    void HandleMessage(uint32_t id, const void* data) override;
    void ApplyVisibility(bool visible) override;

    // -- trigger virtual interface (slots 71..77) ---------------------------
    virtual void OnEnter(ObjEntity* entity);
    virtual void OnExit(ObjEntity* entity);
    virtual void OnStay(ObjEntity* entity) { (void)entity; }
    // Removes an occupant without the leave notifications.
    virtual void RemoveOccupant(ObjEntity* entity);
    // Drops pending enter/leave reports and the entity's nav registration.
    virtual void ForgetEntity(ObjEntity* entity);
    virtual void SetEnabled(bool enabled);
    virtual bool IsDebugDisplayed() const;

    // Reports queued by the phantom listener (console code 0x0075A6xx).
    void QueueEnter(ObjEntity* entity);
    void QueueExit(ObjEntity* entity);

    bool IsEnabled() const { return m_enabled; }
    int OccupantCount() const { return (int)m_occupants.size(); }
    ObjEntity* Occupant(int index) const { return m_occupants[(size_t)index]; }
    TriggerVolume* Volume() const { return m_volume; }

protected:
    // Registers the volume (or the trigger bounds) as a nav obstacle (0x0020C2BC).
    void RegisterNavObstacle();
    // Sends OnExit for every occupant (0x0020C670).
    void ExitAll();
    // Applies the queued reports (0x0020DC6C enters, 0x0020DE3C leaves).
    void ProcessEnterQueue();
    void ProcessExitQueue();
    // Drops queued reports of entities that died meanwhile.
    void PruneQueues();
    // Leaves the trigger slot: OnExit's bookkeeping shared by RemoveOccupant.
    bool RemoveFromList(ObjEntity* entity, bool notifyEntity);
    // Shared start of OnEnter/OnExit/RemoveOccupant/ForgetEntity.
    void DropPendingReports(ObjEntity* entity);
    // Applies a new enabled state (shared by the constructor and SetEnabled).
    void ChangeEnabled(bool enabled);
    int FindOccupant(const ObjEntity* entity) const;

    std::vector<ObjEntity*> m_occupants;        // 0x158
    std::map<uint32_t, ObjEntity*> m_enterQueue; // 0x16C keyed by entity id (+0xA8)
    std::map<uint32_t, ObjEntity*> m_exitQueue;  // 0x184
    bool m_enabled = false;          // 0x19C bit 31
    bool m_playersOnly = false;      // bit 30
    bool m_allEvents = false;        // bit 29
    bool m_navObstacle = false;      // bit 28
    bool m_aabbVolume = false;       // bit 27: AABB phantom fed by the listener; else the overlaps are polled
    bool m_ownsVolume = false;       // bit 26: no extra reference; the trigger adds the phantom to the world
    bool m_processing = false;       // bit 25: inside ProcessEnterQueue/ProcessExitQueue
    TriggerVolume* m_volume = nullptr;           // 0x1A0
    bool m_listenerInstalled = false;            // 0x1A4 (listener object)
    NavObstacleHandle m_navHandle;               // 0x1A8
};

// The generic trigger (vtable 0x00B19698); behaviour is all in the base.
class Trigger : public TriggerBase {
public:
    explicit Trigger(const TriggerInstanceDesc& desc) : TriggerBase(desc, 0x20, true) {}
    // @func 0x00759FB4 Trigger::~Trigger (deleting)
    ~Trigger() override = default;
};

// Whether a Create should skip a placement: placements flagged "optional" are only
// used by the trigger class they were made for (`typeMatches`).
bool TriggerPlacementSkips(const EntityPlacement* placement, bool typeMatches);

// Entity id used to key the report queues (+0xA8).
uint32_t EntityId(const ObjEntity* entity);

// Owner entity of a collidable overlapping a volume (0x0020DB54).
ObjEntity* CollidableOwner(const void* collidable);

} // namespace objects
} // namespace sly
