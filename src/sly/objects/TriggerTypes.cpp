// openTIT - Trigger subclasses with little behaviour of their own: alert, crawl,
// filtered (physics state), job, jump direction and update-enable volumes.
#include "sly/objects/TriggerTypes.h"

#include "sly/entity/Abilities.h"

#include "sly/objects/TriggerTuning.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

#include <cstring>

namespace sly {
namespace objects {

namespace {
// Physics states a filtered trigger can require (table 0x00AD3FE4).
constexpr uint32_t kPhysicsStateHashes[7] = {
    0x0024A738,   // "None"
    0x7DD0CD87,   // "Ground"
    0x0000FD2A,   // "Air"
    0x03D3CE95,   // "Climb"
    0x03D66C07,   // "Crawl"
    0x9EA8BD98,   // "PlayDead"
    0x419905DF,   // "ClimbWall"
};

// Builds the instance description for a Create: name, placement data.
template <class Desc>
void PrepareDesc(Desc& desc, EntityPlacement* placement)
{
    placement->FillInstanceDesc(desc);
}
} // namespace

bool TriggerDebugSwitch(bool classSwitch)
{
    return classSwitch || g_debugDisplayTrigger_All.Get();
}

// ===========================================================================
// Alert
// ===========================================================================

AlertTriggerDefDesc::AlertTriggerDefDesc(const EntityDefDesc& loader)
    : TriggerDefDesc(kDefClass_Trigger, loader, "TriggerAlert_")
{
    typeBits = kTrigType_Alert;
}

// @func 0x002124D0 AlertTriggerDefDesc::Parse
void AlertTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    // The console builds a throw-away default definition to read the default from;
    // the value it yields is the description default (false).
    bool defIgnore = false;
    if (parent && (parent->typeBits & kTrigType_Alert))
        defIgnore = static_cast<const AlertTriggerDef*>(parent)->ignoreDisguise;
    elem->GetBool(kAttr_IgnoreDisguise, &ignoreDisguise, defIgnore);
}

AlertTriggerDef::AlertTriggerDef(const AlertTriggerDefDesc& desc)
    : TriggerDef(desc), ignoreDisguise(desc.ignoreDisguise)
{
}

// @func 0x002126B4 CreateAlertTriggerDef
EntityDef* CreateAlertTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent)
{
    AlertTriggerDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    return new AlertTriggerDef(desc);
}

// @func 0x00212828 AlertTriggerDef::Create
ObjEntity* AlertTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_Alert) != 0))
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    PrepareDesc(desc, placement);
    AlertTrigger* trigger = new AlertTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// @func 0x00212BE0 AlertTrigger::~AlertTrigger (deleting)
AlertTrigger::~AlertTrigger() = default;

// @func 0x00212C28 AlertTrigger::OnEnter
void AlertTrigger::OnEnter(ObjEntity* entity)
{
    // Undisguised players (not riding anything: class bit 3) raise the alarm.
    if (entity && (entity->flags2 & kEntFlag2_Character) && entity->IsPlayer() &&
        !(entity->classBits & 8)) {
        const AlertTriggerDef* alertDef = static_cast<const AlertTriggerDef*>(def);
        if (alertDef->ignoreDisguise || !IsCharacterDisguised(entity))
            PostTriggerEvent(kTriggerEvent_Alert, this, entity->playerIndex);
    }
    TriggerBase::OnEnter(entity);
}

// @func 0x00212D58 AlertTrigger::OnExit
void AlertTrigger::OnExit(ObjEntity* entity)
{
    TriggerBase::OnExit(entity);
}

// @func 0x00212D78 AlertTrigger::IsDebugDisplayed
bool AlertTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerAlert.Get());
}

// ===========================================================================
// Crawl
// ===========================================================================

// @func 0x002198A8 CreateCrawlTriggerDef
EntityDef* CreateCrawlTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Trigger, loader, "TriggerCrawl_");
    desc.typeBits = kTrigType_Crawl;
    desc.Parse(root, elem, loader, parent);
    return new CrawlTriggerDef(desc);
}

// @func 0x00219A10 CrawlTriggerDef::Create
ObjEntity* CrawlTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_Crawl) != 0))
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    PrepareDesc(desc, placement);
    CrawlTrigger* trigger = new CrawlTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// @func 0x00219DC8 CrawlTrigger::~CrawlTrigger (deleting)
CrawlTrigger::~CrawlTrigger() = default;

// @func 0x00219E10 CrawlTrigger::OnEnter
void CrawlTrigger::OnEnter(ObjEntity* entity)
{
    if (!entity || !(entity->flags2 & kEntFlag2_Character))
        return;
    TriggerBase::OnEnter(entity);
}

// @func 0x00219E84 CrawlTrigger::OnExit
void CrawlTrigger::OnExit(ObjEntity* entity)
{
    // A character still in the crawl idle (state 0/10/2) is moved on to the
    // stand-up state when it leaves.
    ObjEntity* character = (entity && (entity->flags2 & kEntFlag2_Character)) ? entity : nullptr;
    if (character && character->GetState(0) == 0 && character->GetState(1) == 10 &&
        character->GetState(2) == 2)
        character->SetState(2, 3);
    TriggerBase::OnExit(entity);
}

// @func 0x00219FB0 CrawlTrigger::IsDebugDisplayed
bool CrawlTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerCrawl.Get());
}

// ===========================================================================
// Filtered
// ===========================================================================

int PhysicsStateIndex(uint32_t nameHash)
{
    for (int i = 0; i < 7; ++i)
        if (kPhysicsStateHashes[i] == nameHash)
            return i;
    return -1;
}

// @func 0x0021D3CC CreateFilteredTriggerDef
EntityDef* CreateFilteredTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Trigger, loader, "TriggerFiltered_");
    desc.typeBits = kTrigType_Filtered;
    desc.Parse(root, elem, loader, parent);
    uint32_t stateHash = 0;
    elem->GetHash(kAttr_PhysicsState, &stateHash, 0);
    return new FilteredTriggerDef(desc, PhysicsStateIndex(stateHash));
}

// @func 0x0021D5B0 FilteredTriggerDef::Create
ObjEntity* FilteredTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_Filtered) != 0))
        return nullptr;
    FilteredTriggerInstanceDesc desc(GetInstanceName(placement), this);
    PrepareDesc(desc, placement);
    FilteredTrigger* trigger = new FilteredTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

FilteredTriggerPlacement::FilteredTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    typeBits |= kTrigType_Filtered;
}

// @func 0x0021D838 ReadFilteredTriggerPlacement
static void ReadFilteredTriggerPlacement(ape::xml::XmlElement* elem, FilteredTriggerPlacement& placement)
{
    ParseTriggerPlacementFlags(elem, placement);
    uint32_t stateHash = 0;
    elem->GetHash(kAttr_PhysicsState, &stateHash, 0);
    int state = PhysicsStateIndex(stateHash);
    if (state >= 0)
        placement.physicsState = state;
}

// @func 0x0021D9C8 ParseFilteredTriggerPlacement
EntityPlacement* ParseFilteredTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                               const char* name)
{
    (void)root;
    FilteredTriggerPlacement* placement = new FilteredTriggerPlacement(name);
    ReadFilteredTriggerPlacement(elem, *placement);
    return placement;
}

// @func 0x0021DAA0 FilteredTriggerPlacement::FillInstanceDesc
void FilteredTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    static_cast<FilteredTriggerInstanceDesc&>(baseDesc).physicsState = physicsState;
}

// @func 0x0021DB80 FilteredTrigger::~FilteredTrigger
// @func 0x0021DCC0 FilteredTrigger::~FilteredTrigger (deleting)
FilteredTrigger::~FilteredTrigger() = default;

int CharacterPhysicsState(ObjEntity* entity)
{
    // The physics component keeps the state in bits 28..30 of its word +0x1A0.
    const uint8_t* component = static_cast<const uint8_t*>(entity->GetPhysicsComponent());
    if (!component)
        return -1;
    uint32_t word;
    std::memcpy(&word, component + 0x1A0, sizeof(word));
    return (int)((word >> 28) & 7);
}

bool FilteredTrigger::Accepts(ObjEntity* entity)
{
    if (m_physicsState == -1)
        return true;
    int state = CharacterPhysicsState(entity);
    return state == -1 || state == m_physicsState;
}

// @func 0x0021DD08 FilteredTrigger::Update
void FilteredTrigger::Update(float dt)
{
    if (m_physicsState != -1) {
        size_t i = 0;
        while (i < m_occupants.size()) {
            ObjEntity* occupant = m_occupants[i];
            if (Accepts(occupant))
                ++i;
            else
                OnExit(occupant);
        }
    }
    TriggerBase::Update(dt);
}

// @func 0x0021DE08 FilteredTrigger::OnEnter
void FilteredTrigger::OnEnter(ObjEntity* entity)
{
    if (!Accepts(entity))
        return;
    TriggerBase::OnEnter(entity);
}

// @func 0x0021DE9C FilteredTrigger::IsDebugDisplayed
bool FilteredTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerFiltered.Get());
}

// ===========================================================================
// Job
// ===========================================================================

// @func 0x002203F0 CreateJobTriggerDef
EntityDef* CreateJobTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                               const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(0x40000000u | kDefClass_Trigger, loader, "TriggerJob_");
    desc.Parse(root, elem, loader, parent);
    return new JobTriggerDef(desc);
}

// @func 0x00220560 JobTriggerDef::Create
ObjEntity* JobTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kPlace_Job) != 0))
        return nullptr;
    JobTriggerInstanceDesc desc(GetInstanceName(placement), this);
    PrepareDesc(desc, placement);
    JobTrigger* trigger = new JobTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

JobTriggerPlacement::JobTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    flags |= kPlace_Job;
}

// @func 0x0022083C ReadJobTriggerPlacement
static void ReadJobTriggerPlacement(ape::xml::XmlElement* elem, JobTriggerPlacement& p)
{
    ParseTriggerPlacementFlags(elem, p);
    elem->GetHash(kAttr_JobName, &p.jobName, p.jobName);
    elem->GetHash(kAttr_StartGoal, &p.startGoal, p.startGoal);
    elem->GetBool(kAttr_Job_C4, &p.flagC4, p.flagC4);
    elem->GetBool(kAttr_Job_C5, &p.flagC5, p.flagC5);
    elem->GetHash(kAttr_Job_C8, &p.promptId, p.promptId);
    if (const char* text = elem->GetString(kAttr_Job_Text)) {
        ape::StrNCopy(p.promptText, text, sizeof(p.promptText));
        p.promptText[0xFF] = '\0';
    }
}

// @func 0x00220A64 ParseJobTriggerPlacement
EntityPlacement* ParseJobTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name)
{
    (void)root;
    JobTriggerPlacement* placement = new JobTriggerPlacement(name);
    ReadJobTriggerPlacement(elem, *placement);
    return placement;
}

// @func 0x00220B48 JobTriggerPlacement::FillInstanceDesc
void JobTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    JobTriggerInstanceDesc& desc = static_cast<JobTriggerInstanceDesc&>(baseDesc);
    desc.jobName = jobName;
    desc.startGoal = startGoal;
    desc.flagA4 = flagC4;
    desc.flagA5 = flagC5;
    desc.promptId = promptId;
    ape::StrNCopy(desc.promptText, promptText, sizeof(desc.promptText));
}

JobTrigger::JobTrigger(const JobTriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x20, true),
      m_startGoal(desc.startGoal),
      m_flag1B4(desc.flagA4),
      m_flag1B5(desc.flagA5),
      m_promptId(desc.promptId)
{
    m_job = FindJob(desc.jobName);
    ape::StrNCopy(m_promptText, desc.promptText, sizeof(m_promptText));
}

// @func 0x00220DA8 JobTrigger::~JobTrigger (deleting)
JobTrigger::~JobTrigger() = default;

// @func 0x00220DF0 JobTrigger::Release
void JobTrigger::Release(bool immediate)
{
    TriggerBase::Release(immediate);
}

uint32_t JobField1E8(const void* job)
{
    uint32_t value;
    std::memcpy(&value, static_cast<const uint8_t*>(job) + 0x1E8, sizeof(value));
    return value;
}

// @func 0x00221068 JobTrigger::GetJob
void* JobTrigger::GetJob() const
{
    return m_job;
}

// @func 0x00220E14 JobTrigger::IsJobAvailable
bool JobTrigger::IsJobAvailable() const
{
    return m_job && JobState(m_job) == 1;
}

// @func 0x00220E64 JobTrigger::OnEnter
// Offers the job to a living entity when none is running: either through a
// yes/no prompt (when the trigger names an ability the character has), or by
// starting it straight away.
void JobTrigger::OnEnter(ObjEntity* entity)
{
    TriggerBase::OnEnter(entity);
    if (!IsJobAvailable() || GameStateHasActiveJob())
        return;
    if (!GameStateCanStartJob(entity, m_job) || GameStateJobsBlocked())
        return;
    if ((entity->flags2 & 0x02) && entity->IsEventSuppressed())
        return;
    if (m_promptId != 0 && (entity->flags2 & 0x08)) {
        const int ability = entity::FindAbility(m_promptId);
        if (ability != -1) {
            const uint8_t state = CharacterAbilityState(entity, ability);
            if (!(state & 0x80) || (state & 0x40)) {
                ShowJobPrompt(entity, m_promptText, "$BUTTON_X $STR_YES $BUTTON_CIRCLE $STR_NO");
                return;
            }
        }
    }
    GameStateStartJob(entity, m_job, m_startGoal);
}

// @func 0x0022104C JobTrigger::GetJobField1E8
uint32_t JobTrigger::GetJobField1E8() const
{
    return m_job ? JobField1E8(m_job) : 0;
}

// @func 0x00221090 JobTrigger::IsDebugDisplayed
bool JobTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerJob.Get());
}

// ===========================================================================
// JumpDir
// ===========================================================================

const char* TriggerPlacementObjectName(const EntityPlacement* placement)
{
    if (placement->name[0] != '\0')
        return placement->name;
    if (placement->flags & kPlace_Trigger) {
        const TriggerPlacement* trig = static_cast<const TriggerPlacement*>(placement);
        if (trig->volume)
            return VolumeGetName(trig->volume);
    }
    return placement->name;
}

const char* TriggerNameSuffix(const char* name)
{
    const char* p = name;
    while (*p != '\0') {
        if (*p++ == '_')
            break;
    }
    return p;
}

// @func 0x002210C8 CreateJumpDirTriggerDef
EntityDef* CreateJumpDirTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Trigger, loader, "TriggerJumpDir_");
    desc.typeBits = kTrigType_JumpDir;
    desc.Parse(root, elem, loader, parent);
    return new JumpDirTriggerDef(desc);
}

// @func 0x00221238 JumpDirTriggerDef::Create
ObjEntity* JumpDirTriggerDef::Create(EntityPlacement* placement)
{
    bool typed = (placement->typeBits & kTrigType_JumpDir) != 0;
    if (TriggerPlacementSkips(placement, typed))
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    // The direction spline is "Spline_JumpDir_<suffix of the object name>".
    uint32_t splineHash = 0;
    if (!typed) {
        const char* suffix = TriggerNameSuffix(TriggerPlacementObjectName(placement));
        splineHash = ape::HashNameNoCase(ape::TempFormat("Spline_JumpDir_%s", suffix));
    }
    PrepareDesc(desc, placement);
    JumpDirTrigger* trigger = new JumpDirTrigger(desc, splineHash);
    trigger->OnCreated();
    return trigger;
}

JumpDirTrigger::JumpDirTrigger(const TriggerInstanceDesc& desc, uint32_t splineHash)
    : TriggerBase(desc, 0x20, true)
{
    m_spline = FindSplineDef(splineHash);
}

// @func 0x002216D8 JumpDirTrigger::~JumpDirTrigger (deleting)
JumpDirTrigger::~JumpDirTrigger() = default;

// @func 0x00221720 JumpDirTrigger::IsDebugDisplayed
bool JumpDirTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerJumpDir.Get());
}

// ===========================================================================
// UpdateEnable
// ===========================================================================

namespace {
struct UpdateEnableTriggerDefDesc : TriggerDefDesc {
    explicit UpdateEnableTriggerDefDesc(const EntityDefDesc& loader)
        : TriggerDefDesc(kDefClass_Trigger, loader, "TriggerUpdateEnable_")
    {
        typeBits = kTrigType_UpdateEnable;
    }
};
} // namespace

// @func 0x0022515C UpdateEnableTriggerDefDesc::Parse
static void ParseUpdateEnableTriggerDesc(UpdateEnableTriggerDefDesc& desc, ape::xml::XmlElement* root,
                                         ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                         const EntityDef* parent)
{
    desc.Parse(root, elem, loader, parent);
    desc.field_20 = 1;
    // A parent that is not an update-enable trigger only triggers a warning (stripped).
}

// @func 0x002251DC CreateUpdateEnableTriggerDef
EntityDef* CreateUpdateEnableTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                        const EntityDefDesc& loader, const EntityDef* parent)
{
    UpdateEnableTriggerDefDesc desc(loader);
    ParseUpdateEnableTriggerDesc(desc, root, elem, loader, parent);
    return new UpdateEnableTriggerDef(desc);
}

// @func 0x00225348 UpdateEnableTriggerDef::Create
ObjEntity* UpdateEnableTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_UpdateEnable) != 0))
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    PrepareDesc(desc, placement);
    UpdateEnableTrigger* trigger = new UpdateEnableTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

UpdateEnableTrigger::UpdateEnableTrigger(const TriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x100, true)
{
    m_tracked.reserve(2);
}

// @func 0x00225600 UpdateEnableTrigger::~UpdateEnableTrigger
// @func 0x0022576C UpdateEnableTrigger::~UpdateEnableTrigger (deleting)
UpdateEnableTrigger::~UpdateEnableTrigger() = default;

namespace {
constexpr uint8_t kDefOption_SleepsAway = 0x08;   // EntityDef::options bit 3
constexpr uint32_t kFlags2_Killable = 0x02;
constexpr uint32_t kFlags2_Character = 0x08;
constexpr uint32_t kFlags2_10 = 0x10;              // NOTE: excludes characters from tracking

bool SleepsAway(const ObjEntity* entity)
{
    return entity->def && (entity->def->options & kDefOption_SleepsAway) != 0;
}

// Characters that could turn into the player are tracked even when they are not one.
bool IsTrackedCharacter(const ObjEntity* entity)
{
    return (entity->flags2 & kFlags2_Character) && !(entity->flags2 & kFlags2_10);
}
} // namespace

void UpdateEnableTrigger::WakeOccupants()
{
    for (ObjEntity* occupant : m_occupants) {
        if (SleepsAway(occupant))
            EntitySetVisibilityDeferred(occupant, true, false);
    }
}

// @func 0x002257B4 UpdateEnableTrigger::PlayerLeft
void UpdateEnableTrigger::PlayerLeft(ObjEntity* leaving)
{
    if (--m_playerCount != 0)
        return;
    for (ObjEntity* occupant : m_occupants) {
        if (occupant != leaving && SleepsAway(occupant))
            EntitySetVisibilityDeferred(occupant, false, false);
    }
}

// @func 0x00225870 UpdateEnableTrigger::Update
// Characters that became (or stopped being) the player since last frame enter
// (or leave) the trigger's player bookkeeping.
void UpdateEnableTrigger::Update(float dt)
{
    for (Tracked& tracked : m_tracked) {
        const bool isPlayer = tracked.entity->IsPlayer();
        if (tracked.isPlayer == isPlayer)
            continue;
        tracked.isPlayer = isPlayer;
        if (isPlayer) {
            TriggerBase::OnEnter(tracked.entity);
            if (m_playerCount++ == 0)
                WakeOccupants();
        } else {
            TriggerBase::OnExit(tracked.entity);
            PlayerLeft(tracked.entity);
        }
    }
    TriggerBase::Update(dt);
}

// @func 0x002259EC UpdateEnableTrigger::OnEnter
void UpdateEnableTrigger::OnEnter(ObjEntity* entity)
{
    const bool isPlayer = entity->IsPlayer();
    const bool character = IsTrackedCharacter(entity);
    if (isPlayer || character) {
        bool known = false;
        for (const Tracked& tracked : m_tracked)
            known = known || tracked.entity == entity;
        if (!known)
            m_tracked.push_back(Tracked{entity, isPlayer});
        // Characters wait in the list until they become the player.
        if (!isPlayer)
            return;
    }
    TriggerBase::OnEnter(entity);
    if (!isPlayer) {
        if (SleepsAway(entity)) {
            entity->SetBusy(true);
            EntitySetVisibilityDeferred(entity, m_playerCount != 0, false);
        }
    } else if (m_playerCount++ == 0) {
        WakeOccupants();
    }
}

// @func 0x00225C88 UpdateEnableTrigger::OnExit
void UpdateEnableTrigger::OnExit(ObjEntity* entity)
{
    TriggerBase::OnExit(entity);
    if (IsTrackedCharacter(entity) || entity->IsPlayer()) {
        for (size_t i = 0; i < m_tracked.size(); ++i) {
            if (m_tracked[i].entity == entity) {
                m_tracked[i] = m_tracked.back();
                m_tracked.pop_back();
                break;
            }
        }
    }
    if (entity->IsPlayer()) {
        PlayerLeft(entity);
        return;
    }
    if (!SleepsAway(entity))
        return;
    // Dead killables keep their state.
    if ((entity->flags2 & kFlags2_Killable) && EntityHealth(entity) <= 0.0f)
        return;
    entity->SetBusy(false);
}

// @func 0x00225E50 UpdateEnableTrigger::IsDebugDisplayed
bool UpdateEnableTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerUpdateEnable.Get());
}

} // namespace objects
} // namespace sly
