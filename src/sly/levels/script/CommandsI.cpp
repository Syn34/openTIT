// openTIT - Game script commands, seventh block (conditional commands If...).
//
// Each runs its nested commands (the command children of its element) when
// its condition holds.
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrCollected = AnimHash("Collected");
constexpr uint32_t kAttrEntityDef = AnimHash("EntityDef");
constexpr uint32_t kAttrEntityInst = AnimHash("EntityInst");
constexpr uint32_t kAttrReverse = AnimHash("Reverse");
constexpr uint32_t kAttrCheckEntityAlive = AnimHash("CheckEntityAlive");
constexpr uint32_t kAttrJob = AnimHash("Job");
constexpr uint32_t kAttrCharacterDef = AnimHash("CharacterDef");
constexpr uint32_t kAttrIsSelectedJob = AnimHash("IsSelectedJob");
constexpr uint32_t kAttrState = AnimHash("state");
constexpr uint32_t kAttrNotifyFlag = AnimHash("NotifyFlag");
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrIsSet = AnimHash("IsSet");

constexpr uint32_t kCollectType_Mask = AnimHash("mask");
} // namespace

// ---------------------------------------------------------------------------
// IfCollected: tests whether a collectable has been picked up.

class IfCollectedCommand : public ApeAnimScriptCommand {
public:
    enum CollectType : int32_t { kType_None = 0, kType_Mask = 1 };

    IfCollectedCommand();
    explicit IfCollectedCommand(XmlElement* element);
    ~IfCollectedCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    XmlElement* CreateDebugElement() const override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;            // +0x0C "object": collectable name
    int32_t m_type = kType_None;      // +0x10 "Type"
    bool m_collected = false;         // +0x14 "Collected"
    ape::AnimScriptCommandList m_commands;  // +0x18
};

// @func 0x0007D9B4 IfCollectedCommand::IfCollectedCommand
IfCollectedCommand::IfCollectedCommand() = default;

// @func 0x0007DA1C IfCollectedCommand::IfCollectedCommand
IfCollectedCommand::IfCollectedCommand(XmlElement* element)
{
    element->GetHash(kAttrObject, &m_object, 0);
    uint32_t type = 0;
    element->GetHash(kAttrType, &type, 0);
    element->GetBool(kAttrCollected, &m_collected, false);
    m_type = (type == kCollectType_Mask) ? kType_Mask : kType_None;
    ape::LoadCommandList(m_commands, element);
}

// @func 0x0007DC44 IfCollectedCommand::~IfCollectedCommand
// @func 0x0007DD24 IfCollectedCommand::~IfCollectedCommand (deleting)
IfCollectedCommand::~IfCollectedCommand()
{
    ape::ReleaseCommandList(m_commands);
}

// @func 0x0007DE20 IfCollectedCommand::Link
void IfCollectedCommand::Link()
{
    LinkCommands(m_commands);
}

// @func 0x0007DEA8 IfCollectedCommand::Create
ApeAnimScriptCommand* IfCollectedCommand::Create(XmlElement* element) const
{
    return new IfCollectedCommand(element);
}

// @func 0x0007DF18 IfCollectedCommand::Execute
void IfCollectedCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (m_type == kType_None)
        return;
    bool collected = false;
    if (m_type == kType_Mask) {
        if (uint32_t id = FindCollectableId(m_object))
            collected = GameStateCollectableQuery(id) == 0;
    }
    if (collected == m_collected)
        RunCommands(m_commands, instance, state);
}

// @func 0x0007E03C IfCollectedCommand::CreateDebugElement
XmlElement* IfCollectedCommand::CreateDebugElement() const
{
    return NewDebugElement("IfCollected");
}

// ---------------------------------------------------------------------------
// IfEntity: tests an entity's definition or instance name.

class IfEntityCommand : public ApeAnimScriptCommand {
public:
    IfEntityCommand();
    explicit IfEntityCommand(XmlElement* element);
    ~IfEntityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    XmlElement* CreateDebugElement() const override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // +0x0C "object"
    uint32_t m_entity = 0;        // +0x10 "Entity"
    uint32_t m_entityDef = 0;     // +0x14 "EntityDef"
    uint32_t m_entityInst = 0;    // +0x18 "EntityInst"
    ape::AnimScriptCommandList m_commands;  // +0x1C
    bool m_reverse = false;       // +0x30 "Reverse"
    bool m_checkAlive = false;    // +0x31 "CheckEntityAlive"
};

// @func 0x0007E0A0 IfEntityCommand::IfEntityCommand
IfEntityCommand::IfEntityCommand() = default;

// @func 0x0007E110 IfEntityCommand::IfEntityCommand
IfEntityCommand::IfEntityCommand(XmlElement* element)
{
    element->GetHash(kAttrObject, &m_object, 0);
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrEntityDef, &m_entityDef, 0);
    element->GetHash(kAttrEntityInst, &m_entityInst, 0);
    element->GetBool(kAttrReverse, &m_reverse, false);
    element->GetBool(kAttrCheckEntityAlive, &m_checkAlive, false);
    ape::LoadCommandList(m_commands, element);
}

// @func 0x0007E388 IfEntityCommand::~IfEntityCommand
// @func 0x0007E468 IfEntityCommand::~IfEntityCommand (deleting)
IfEntityCommand::~IfEntityCommand()
{
    ape::ReleaseCommandList(m_commands);
}

// @func 0x0007E564 IfEntityCommand::Link
void IfEntityCommand::Link()
{
    LinkCommands(m_commands);
}

// @func 0x0007E5EC IfEntityCommand::Create
ApeAnimScriptCommand* IfEntityCommand::Create(XmlElement* element) const
{
    return new IfEntityCommand(element);
}

// @func 0x0007E65C IfEntityCommand::Execute
void IfEntityCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (m_checkAlive) {
        // A missing, inactive or dead entity matches neither way.
        if (!entity || !EntityIsAlive(entity))
            return;
        if (EntityHasClassFlag(entity, kEntityClass_Killable) && EntityIsDead(entity))
            return;
    }
    bool match = false;
    if (entity) {
        match = (m_entityDef && m_entityDef == EntityDefName(entity)) ||
                (m_entityInst && m_entityInst == EntityName(entity));
    }
    if (match != m_reverse)
        RunCommands(m_commands, instance, state);
}

// @func 0x0007E870 IfEntityCommand::CreateDebugElement
XmlElement* IfEntityCommand::CreateDebugElement() const
{
    return NewDebugElement("IfEntity");
}

// ---------------------------------------------------------------------------
// IfJob: tests the state of a job (mission).

class IfJobCommand : public ApeAnimScriptCommand {
public:
    enum JobState : int32_t { kJob_Unavailable = 0, kJob_Available = 1, kJob_Complete = 2 };

    IfJobCommand();
    explicit IfJobCommand(XmlElement* element);
    ~IfJobCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_job = 0;                 // +0x0C "Job"
    uint32_t m_characterDef = 0;        // +0x10 "CharacterDef"
    int32_t m_state = kJob_Available;   // +0x14 "state"
    ape::AnimScriptCommandList m_commands;  // +0x18
    bool m_isSelectedJob = false;       // +0x2C "IsSelectedJob"
    bool m_testState = true;            // +0x2D false when "IsSelectedJob" is given
};

// @func 0x0007E8D4 IfJobCommand::IfJobCommand
IfJobCommand::IfJobCommand() = default;

// @func 0x0007E944 IfJobCommand::IfJobCommand
IfJobCommand::IfJobCommand(XmlElement* element)
{
    element->GetHash(kAttrJob, &m_job, m_job);
    element->GetHash(kAttrCharacterDef, &m_characterDef, m_characterDef);
    if (element->GetBool(kAttrIsSelectedJob, &m_isSelectedJob, m_isSelectedJob) != ape::xml::kQuery_Missing)
        m_testState = false;

    uint32_t state = 0;
    if (element->GetHash(kAttrState, &state, 0) == ape::xml::kQuery_Ok) {
        switch (state) {
        case AnimHash("available"): m_state = kJob_Available; break;
        case AnimHash("unavailable"): m_state = kJob_Unavailable; break;
        case AnimHash("Complete"): m_state = kJob_Complete; break;
        default: break;
        }
    }
    ape::LoadCommandList(m_commands, element);
}

// @func 0x0007EBF4 IfJobCommand::~IfJobCommand
// @func 0x0007ECC0 IfJobCommand::~IfJobCommand (deleting)
IfJobCommand::~IfJobCommand()
{
    // Unlike the other conditionals this one releases first to last.
    for (ApeAnimScriptCommand* command : m_commands) {
        if (command)
            command->Release();
    }
    m_commands.clear();
}

// @func 0x0007EDB4 IfJobCommand::Create
ApeAnimScriptCommand* IfJobCommand::Create(XmlElement* element) const
{
    return new IfJobCommand(element);
}

// @func 0x0007EE24 IfJobCommand::Execute
// Runs the nested commands when the job matches: either "is (not) the
// selected job", or "is in `state`" (optionally only while the player is the
// given character definition).
void IfJobCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    void* job = GameStateFindJob(m_job);
    if (!job)
        return;
    if (!m_testState) {
        if ((CurrentJob() == job) != m_isSelectedJob)
            return;
    } else {
        if (levels::JobState(job) != m_state)
            return;
        if (m_characterDef != 0) {
            WorldObject* player = PlayerEntity();
            if (!player || EntityDefName(player) != m_characterDef)
                return;
        }
    }
    // NOTE: unlike the other nested-command commands this one runs them in
    // list order through the command's own list.
    RunCommands(m_commands, instance, state);
}

// ---------------------------------------------------------------------------
// "IfNotifyFlag" (hash 0xBFB3BF12; NOTE: name from a hash match): tests a
// notification flag of a player.

class IfNotifyFlagCommand : public ApeAnimScriptCommand {
public:
    IfNotifyFlagCommand();
    explicit IfNotifyFlagCommand(XmlElement* element);
    ~IfNotifyFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::AnimScriptCommandList m_commands;  // +0x0C
    uint32_t m_notifyFlag = 0;  // +0x20 "NotifyFlag"
    int32_t m_playerIdx = 0;    // +0x24 "PlayerIdx" (1-based in data)
    bool m_isSet = false;       // +0x28 "IsSet"
};

// @func 0x0007EF7C IfNotifyFlagCommand::IfNotifyFlagCommand
IfNotifyFlagCommand::IfNotifyFlagCommand() = default;

// @func 0x0007EFE4 IfNotifyFlagCommand::IfNotifyFlagCommand
IfNotifyFlagCommand::IfNotifyFlagCommand(XmlElement* element)
{
    element->GetHash(kAttrNotifyFlag, &m_notifyFlag, 0);
    if (element->GetInt(kAttrPlayerIdx, &m_playerIdx, m_playerIdx) == ape::xml::kQuery_Ok && m_playerIdx > 0)
        m_playerIdx -= 1;
    element->GetBool(kAttrIsSet, &m_isSet, m_isSet);
    ape::LoadCommandList(m_commands, element);
}

// @func 0x0007F218 IfNotifyFlagCommand::~IfNotifyFlagCommand
// @func 0x0007F2F8 IfNotifyFlagCommand::~IfNotifyFlagCommand (deleting)
IfNotifyFlagCommand::~IfNotifyFlagCommand()
{
    ape::ReleaseCommandList(m_commands);
}

// @func 0x0007F3F4 IfNotifyFlagCommand::Link
void IfNotifyFlagCommand::Link()
{
    LinkCommands(m_commands);
}

// @func 0x0007F47C IfNotifyFlagCommand::Create
ApeAnimScriptCommand* IfNotifyFlagCommand::Create(XmlElement* element) const
{
    return new IfNotifyFlagCommand(element);
}

// @func 0x0007F4EC IfNotifyFlagCommand::Execute
void IfNotifyFlagCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (m_notifyFlag == 0)
        return;
    if (PlayerNotifyFlagIsSet(m_playerIdx, m_notifyFlag) == m_isSet)
        RunCommands(m_commands, instance, state);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsI()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("IfCollected"), new IfCollectedCommand());
    prototypes.Add(AnimHash("IfEntity"), new IfEntityCommand());
    prototypes.Add(AnimHash("IfJob"), new IfJobCommand());
    prototypes.Add(AnimHash("IfNotifyFlag"), new IfNotifyFlagCommand());
}

} // namespace levels
} // namespace sly
