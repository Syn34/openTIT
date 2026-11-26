// openTIT - Trigger subclasses with little behaviour of their own: alert, crawl,
// filtered (physics state), job, jump direction and update-enable volumes.
//
// Each class has a definition loader entry (Create<Name>TriggerDef), sometimes a
// placement loader entry, and the definition's Create that spawns the entity.
// Type bits (EntityDef::typeBits / EntityPlacement::typeBits) let a placement made
// for one trigger class be skipped by the others.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>
#include <vector>

namespace sly {
namespace objects {

enum TriggerTypeBits : uint32_t {
    kTrigType_PushBlock    = 0x01,
    kTrigType_Crawl        = 0x04,
    kTrigType_Alert        = 0x08,
    kTrigType_Filtered     = 0x10,
    kTrigType_JumpDir      = 0x20,
    kTrigType_UpdateEnable = 0x80,
};

// Placement flags of the trigger subclasses that have their own placement class.
enum TriggerPlacementFlags : uint32_t {
    kPlace_Camera = 0x00800000,
    kPlace_Job    = 0x40000000,
};

// ---------------------------------------------------------------------------
// Alert: alerts the guards when an undisguised player walks in.
// ---------------------------------------------------------------------------
constexpr uint32_t kAttr_IgnoreDisguise = 0x4E6A50D1;   // "IgnoreDisguise"

struct AlertTriggerDefDesc : TriggerDefDesc {
    bool ignoreDisguise = false;    // 0x54
    AlertTriggerDefDesc(const EntityDefDesc& loader);
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class AlertTriggerDef : public TriggerDef {
public:
    explicit AlertTriggerDef(const AlertTriggerDefDesc& desc);
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075AE44 AlertTriggerDef::~AlertTriggerDef (deleting)
    ~AlertTriggerDef() override = default;
    bool ignoreDisguise = false;    // 0xB0
};

class AlertTrigger : public TriggerBase {
public:
    explicit AlertTrigger(const TriggerInstanceDesc& desc) : TriggerBase(desc, 0x20, true) {}
    ~AlertTrigger() override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    bool IsDebugDisplayed() const override;
};

EntityDef* CreateAlertTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent);

// ---------------------------------------------------------------------------
// Crawl: characters inside must crawl.
// ---------------------------------------------------------------------------
class CrawlTriggerDef : public TriggerDef {
public:
    using TriggerDef::TriggerDef;
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075B320 CrawlTriggerDef::~CrawlTriggerDef (deleting)
    ~CrawlTriggerDef() override = default;
};

class CrawlTrigger : public TriggerBase {
public:
    explicit CrawlTrigger(const TriggerInstanceDesc& desc) : TriggerBase(desc, 0x20, true) {}
    ~CrawlTrigger() override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    bool IsDebugDisplayed() const override;
};

EntityDef* CreateCrawlTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent);

// ---------------------------------------------------------------------------
// Filtered: only accepts characters in one physics state.
// ---------------------------------------------------------------------------
constexpr uint32_t kAttr_PhysicsState = 0x51609AAA;     // "PhysicsState"

// Index of a physics state name in the state table (0x00AD3FE4), or -1.
int PhysicsStateIndex(uint32_t nameHash);

class FilteredTriggerDef : public TriggerDef {
public:
    FilteredTriggerDef(const TriggerDefDesc& desc, int state) : TriggerDef(desc), physicsState(state) {}
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075B5C8 FilteredTriggerDef::~FilteredTriggerDef (deleting)
    ~FilteredTriggerDef() override = default;
    int physicsState = -1;          // 0xB0
};

class FilteredTriggerPlacement : public TriggerPlacement {
public:
    explicit FilteredTriggerPlacement(const char* placementName);
    // @func 0x00759AB0 FilteredTriggerPlacement::~FilteredTriggerPlacement
    // @func 0x0075B620 FilteredTriggerPlacement::~FilteredTriggerPlacement (deleting)
    ~FilteredTriggerPlacement() override = default;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;
    int physicsState = -1;          // 0xBC
};

struct FilteredTriggerInstanceDesc : TriggerInstanceDesc {
    using TriggerInstanceDesc::TriggerInstanceDesc;
    int physicsState = -1;          // 0x9C
};

class FilteredTrigger : public TriggerBase {
public:
    explicit FilteredTrigger(const FilteredTriggerInstanceDesc& desc)
        : TriggerBase(desc, 0x20, true), m_physicsState(-1) {}
    ~FilteredTrigger() override;
    bool Update(float dt) override;
    void OnEnter(Entity* entity) override;
    bool IsDebugDisplayed() const override;

private:
    bool Accepts(Entity* entity);
    int m_physicsState;             // 0x1AC
};

EntityDef* CreateFilteredTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseFilteredTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                               const char* name);
// Physics state component word of a character (+0x1A0 bits 28..30).
int CharacterPhysicsState(Entity* entity);

// ---------------------------------------------------------------------------
// Job: offers / starts a job (mission) when the player walks in.
// ---------------------------------------------------------------------------
constexpr uint32_t kAttr_JobName   = 0xD5227BA8;   // "JobName"
constexpr uint32_t kAttr_StartGoal = 0x27365255;   // "StartGoal"
// NOTE: names of these job placement attributes were not recovered.
constexpr uint32_t kAttr_Job_C4    = 0x95098150;
constexpr uint32_t kAttr_Job_C5    = 0x9446A097;
constexpr uint32_t kAttr_Job_C8    = 0x45BF4CC0;   // hash: prompt confirmation id
constexpr uint32_t kAttr_Job_Text  = 0xF735351B;   // string: prompt text

class JobTriggerDef : public TriggerDef {
public:
    using TriggerDef::TriggerDef;
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075B89C JobTriggerDef::~JobTriggerDef (deleting)
    ~JobTriggerDef() override = default;
};

class JobTriggerPlacement : public TriggerPlacement {
public:
    explicit JobTriggerPlacement(const char* placementName);
    // @func 0x00759BBC JobTriggerPlacement::~JobTriggerPlacement
    // @func 0x0075B8F4 JobTriggerPlacement::~JobTriggerPlacement (deleting)
    ~JobTriggerPlacement() override = default;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;
    uint32_t jobName = 0;           // 0xBC
    uint32_t startGoal = 0;         // 0xC0
    bool flagC4 = false;            // 0xC4
    bool flagC5 = false;            // 0xC5
    uint32_t promptId = 0;          // 0xC8
    char promptText[0x100] = {};    // 0xCC
};

struct JobTriggerInstanceDesc : TriggerInstanceDesc {
    using TriggerInstanceDesc::TriggerInstanceDesc;
    uint32_t jobName = 0;           // 0x9C
    uint32_t startGoal = 0;         // 0xA0
    bool flagA4 = false;            // 0xA4
    bool flagA5 = false;            // 0xA5
    uint32_t promptId = 0;          // 0xA8
    char promptText[0x100] = {};    // 0xAC
};

class JobTrigger : public TriggerBase {
public:
    explicit JobTrigger(const JobTriggerInstanceDesc& desc);
    ~JobTrigger() override;
    bool Release(bool immediate) override;
    void OnEnter(Entity* entity) override;
    bool IsDebugDisplayed() const override;
    // The job this trigger belongs to (0x00221068) and its field +0x1E8 (0x0022104C).
    void* GetJob() const;
    // True while the job can be started (state 1, "available").
    bool IsJobAvailable() const;
    uint32_t GetJobField1E8() const;

private:
    void* m_job = nullptr;          // 0x1AC
    uint32_t m_startGoal = 0;       // 0x1B0
    bool m_flag1B4 = false;         // 0x1B4
    bool m_flag1B5 = false;         // 0x1B5
    uint32_t m_promptId = 0;        // 0x1B8
    char m_promptText[0x100] = {};  // 0x1BC
};

EntityDef* CreateJobTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                               const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseJobTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name);
// Job field +0x1E8 accessor used by JobTrigger (the job object is opaque here).
uint32_t JobField1E8(const void* job);

// ---------------------------------------------------------------------------
// JumpDir: gives jumps inside a direction spline ("Spline_JumpDir_<suffix>").
// ---------------------------------------------------------------------------
class JumpDirTriggerDef : public TriggerDef {
public:
    using TriggerDef::TriggerDef;
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075B9AC JumpDirTriggerDef::~JumpDirTriggerDef (deleting)
    ~JumpDirTriggerDef() override = default;
};

class JumpDirTrigger : public TriggerBase {
public:
    JumpDirTrigger(const TriggerInstanceDesc& desc, uint32_t splineHash);
    ~JumpDirTrigger() override;
    bool IsDebugDisplayed() const override;
    void* GetSpline() const { return m_spline; }

private:
    void* m_spline = nullptr;       // 0x1AC
};

EntityDef* CreateJumpDirTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent);
// Name of the placement's level object, or of its volume when it has none.
const char* TriggerPlacementObjectName(const EntityPlacement* placement);
// Part of an object name after its first '_' ("Trigger_Foo" -> "Foo").
const char* TriggerNameSuffix(const char* name);

// ---------------------------------------------------------------------------
// UpdateEnable: entities inside are updated; others may be put to sleep.
// ---------------------------------------------------------------------------
class UpdateEnableTriggerDef : public TriggerDef {
public:
    using TriggerDef::TriggerDef;
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075BE68 UpdateEnableTriggerDef::~UpdateEnableTriggerDef (deleting)
    ~UpdateEnableTriggerDef() override = default;
};

class UpdateEnableTrigger : public TriggerBase {
public:
    explicit UpdateEnableTrigger(const TriggerInstanceDesc& desc);
    ~UpdateEnableTrigger() override;
    bool Update(float dt) override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    bool IsDebugDisplayed() const override;

private:
    // Characters and players inside, with whether each was a player when last
    // seen (a character can become the player).
    struct Tracked {
        Entity* entity = nullptr;
        bool isPlayer = false;
    };
    // Wakes every occupant that sleeps without a player near.
    void WakeOccupants();
    // A player left: when none remain, the sleeping occupants (except `leaving`)
    // are put back to sleep (0x002257B4).
    void PlayerLeft(Entity* leaving);

    uint32_t m_playerCount = 0;          // 0x1AC players inside
    std::vector<Tracked> m_tracked;      // 0x1B0 (capacity 2)
};

EntityDef* CreateUpdateEnableTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                        const EntityDefDesc& loader, const EntityDef* parent);

// Shared "debug display" switch test: the class switch or "Debug.Display.Trigger. All".
bool TriggerDebugSwitch(bool classSwitch);

} // namespace objects
} // namespace sly
