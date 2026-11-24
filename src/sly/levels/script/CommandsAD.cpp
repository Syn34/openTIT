// openTIT - Game script commands, last block (sequence animation start to the deferred visibility command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/DeferredRequests.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"
#include "sly/gameplay/SequenceCommands.h"
#include "sly/gameplay/Sequencer.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrState = AnimHash("State");
constexpr uint32_t kAttrRecursive = 0xF5897C08u;    // NOTE: name unknown
constexpr uint32_t kAttrGroup = AnimHash("group");
constexpr uint32_t kGroupMusic = AnimHash("Music");
constexpr uint32_t kAttrVolume = AnimHash("volume");
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrVarName = AnimHash("VarName");
constexpr uint32_t kAttrSoundGroup = AnimHash("SoundGroup");
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrStopSound14 = 0x06FA4B5Fu;  // NOTE: name unknown
constexpr uint32_t kAttrStopMusicFade = 0x83F172B0u;  // NOTE: name unknown
constexpr uint32_t kAttrDef = AnimHash("def");
constexpr uint32_t kAttrDef14 = 0x00D8E5D6u;        // NOTE: name unknown
constexpr uint32_t kAttrDef15 = 0xA18DAB9Fu;        // NOTE: name unknown
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;      // NOTE: name unknown
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrDistance = AnimHash("distance");
constexpr uint32_t kAttrRamp = AnimHash("Ramp");
constexpr uint32_t kAttrMinValue = AnimHash("MinValue");
constexpr uint32_t kAttrFadeTime = AnimHash("FadeTime");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrState1D = 0xF0CA0A04u;      // NOTE: name unknown
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrObjectHash = AnimHash("ObjectHash");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrVisible16 = 0x6334AE6Fu;    // NOTE: name unknown
constexpr uint32_t kAttrVisible17 = 0x834744B0u;    // NOTE: name unknown
} // namespace

// ---------------------------------------------------------------------------
// Hash 0xDAD61703 (name unknown): starts an entity script state in the sequence.

class SeqStartStateCommand : public ApeAnimScriptCommand {
public:
    SeqStartStateCommand();
    ~SeqStartStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;    // +0x0C "Entity"
    uint32_t m_state = 0;     // +0x10 "State"
    bool m_recursive = true;  // +0x14 attribute 0xF5897C08
};

// @func 0x000A5F08 SeqStartStateCommand::SeqStartStateCommand
SeqStartStateCommand::SeqStartStateCommand() = default;

// @func 0x007424D0 SeqStartStateCommand::~SeqStartStateCommand
// @func 0x00742500 SeqStartStateCommand::~SeqStartStateCommand (deleting)
SeqStartStateCommand::~SeqStartStateCommand() = default;

// @func 0x000A5F58 SeqStartStateCommand::Create
ApeAnimScriptCommand* SeqStartStateCommand::Create(XmlElement* element) const
{
    auto* command = new SeqStartStateCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    element->GetBool(kAttrRecursive, &command->m_recursive, command->m_recursive);
    return command;
}

// @func 0x000A604C SeqStartStateCommand::Execute
void SeqStartStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_entity != 0 && m_state != 0)
        SequenceAddStartAnimation(m_entity, m_state, m_recursive);
}

// ---------------------------------------------------------------------------
// Hash 0x390D5F47 (name unknown): sets a sound group volume in the sequence.

class SeqGroupVolumeCommand : public ApeAnimScriptCommand {
public:
    SeqGroupVolumeCommand();
    ~SeqGroupVolumeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_channel = 1;  // +0x0C 1 = music (the only group the console recognises)
    float m_volume = 0.0f;   // +0x10 "volume"
};

// @func 0x000A60E4 SeqGroupVolumeCommand::SeqGroupVolumeCommand
SeqGroupVolumeCommand::SeqGroupVolumeCommand() = default;

// @func 0x00742558 SeqGroupVolumeCommand::~SeqGroupVolumeCommand
// @func 0x00742588 SeqGroupVolumeCommand::~SeqGroupVolumeCommand (deleting)
SeqGroupVolumeCommand::~SeqGroupVolumeCommand() = default;

// @func 0x000A6134 SeqGroupVolumeCommand::Create
ApeAnimScriptCommand* SeqGroupVolumeCommand::Create(XmlElement* element) const
{
    auto* command = new SeqGroupVolumeCommand();
    uint32_t group = 0;
    element->GetHash(kAttrGroup, &group, 0);
    if (group == kGroupMusic)
        command->m_channel = 1;
    element->GetFloat(kAttrVolume, &command->m_volume, command->m_volume);
    return command;
}

// @func 0x000A6220 SeqGroupVolumeCommand::Execute
void SeqGroupVolumeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddGroupVolume(m_volume, m_channel);
}

// ---------------------------------------------------------------------------
// Hash 0x7EA392FC (name unknown): shows the level headline in the sequence.

class SeqLevelHeadlineCommand : public ApeAnimScriptCommand {
public:
    SeqLevelHeadlineCommand();
    ~SeqLevelHeadlineCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x000A629C SeqLevelHeadlineCommand::SeqLevelHeadlineCommand
SeqLevelHeadlineCommand::SeqLevelHeadlineCommand() = default;

// @func 0x007425E0 SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand
// @func 0x00742610 SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand (deleting)
SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand() = default;

// @func 0x000A62DC SeqLevelHeadlineCommand::Create
ApeAnimScriptCommand* SeqLevelHeadlineCommand::Create(XmlElement*) const
{
    return new SeqLevelHeadlineCommand();
}

// @func 0x000A635C SeqLevelHeadlineCommand::Execute
void SeqLevelHeadlineCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddLevelHeadline();
}

// TODO(0x000A63C0 SeqSpawnEntityCommand) "SeqSpawnEntity" (#260): parser 0x000A6478,
// Execute 0x000A67E4.

// ---------------------------------------------------------------------------
// "StartSequence" (NOTE: name from a hash match)

class StartSequenceCommand : public ApeAnimScriptCommand {
public:
    StartSequenceCommand();
    ~StartSequenceCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;     // +0x0C "name"
    uint32_t m_varName = 0;  // +0x10 "VarName"
};

// @func 0x000A6A3C StartSequenceCommand::StartSequenceCommand
StartSequenceCommand::StartSequenceCommand() = default;

// @func 0x007426F0 StartSequenceCommand::~StartSequenceCommand
// @func 0x00742720 StartSequenceCommand::~StartSequenceCommand (deleting)
StartSequenceCommand::~StartSequenceCommand() = default;

// @func 0x000A6A84 StartSequenceCommand::Create
ApeAnimScriptCommand* StartSequenceCommand::Create(XmlElement* element) const
{
    auto* command = new StartSequenceCommand();
    element->GetHash(kAttrName, &command->m_name, 0);
    element->GetHash(kAttrVarName, &command->m_varName, command->m_varName);
    return command;
}

// TODO(0x000A6B4C StartSequenceCommand::Execute) starts or queues the named sequence
// (0x0042BAE0 / 0x0042BD0C / 0x0042BF50).

// ---------------------------------------------------------------------------
// Hash 0xB89DA664 (name unknown): stops sounds in the sequence.

class SeqStopSoundCommand : public ApeAnimScriptCommand {
public:
    SeqStopSoundCommand();
    ~SeqStopSoundCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_soundGroup = 0;        // +0x0C "SoundGroup"
    uint32_t m_exclusivityGroup = 0;  // +0x10 "ExclusivityGroup"
    bool m_flag14 = false;            // +0x14 attribute 0x06FA4B5F
};

// @func 0x000A6CA4 SeqStopSoundCommand::SeqStopSoundCommand
SeqStopSoundCommand::SeqStopSoundCommand() = default;

// @func 0x00742778 SeqStopSoundCommand::~SeqStopSoundCommand
// @func 0x007427A8 SeqStopSoundCommand::~SeqStopSoundCommand (deleting)
SeqStopSoundCommand::~SeqStopSoundCommand() = default;

// @func 0x000A6CF4 SeqStopSoundCommand::Create
ApeAnimScriptCommand* SeqStopSoundCommand::Create(XmlElement* element) const
{
    auto* command = new SeqStopSoundCommand();
    element->GetHash(kAttrSoundGroup, &command->m_soundGroup, 0);
    element->GetHash(kAttrExclusivityGroup, &command->m_exclusivityGroup, command->m_exclusivityGroup);
    element->GetBool(kAttrStopSound14, &command->m_flag14, command->m_flag14);
    return command;
}

// @func 0x000A6DE0 SeqStopSoundCommand::Execute
void SeqStopSoundCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddStopSound(m_soundGroup, m_exclusivityGroup, m_flag14);
}

// ---------------------------------------------------------------------------
// "SeqStopMusic" (NOTE: name from a hash match)

class SeqStopMusicCommand : public ApeAnimScriptCommand {
public:
    SeqStopMusicCommand();
    ~SeqStopMusicCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_flag = false;  // +0x0C attribute 0x83F172B0
};

// @func 0x000A6E60 SeqStopMusicCommand::SeqStopMusicCommand
SeqStopMusicCommand::SeqStopMusicCommand() = default;

// @func 0x00740090 SeqStopMusicCommand::~SeqStopMusicCommand
// @func 0x00742800 SeqStopMusicCommand::~SeqStopMusicCommand (deleting)
SeqStopMusicCommand::~SeqStopMusicCommand() = default;

// @func 0x000A6EA8 SeqStopMusicCommand::Create
ApeAnimScriptCommand* SeqStopMusicCommand::Create(XmlElement* element) const
{
    auto* command = new SeqStopMusicCommand();
    element->GetBool(kAttrStopMusicFade, &command->m_flag, false);
    return command;
}

// @func 0x000A6F4C SeqStopMusicCommand::Execute
void SeqStopMusicCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddStopMusic(m_flag);
}

// ---------------------------------------------------------------------------
// "SwitchCostume" (NOTE: name from a hash match)

class SwitchCostumeCommand : public ApeAnimScriptCommand {
public:
    SwitchCostumeCommand();
    ~SwitchCostumeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_def = 0;     // +0x10 "def"
    bool m_flag14 = false;  // +0x14 attribute 0x00D8E5D6
    bool m_flag15 = false;  // +0x15 attribute 0xA18DAB9F
    bool m_wait = true;     // +0x16 attribute 0x8488565B
};

// @func 0x000A6FC0 SwitchCostumeCommand::SwitchCostumeCommand
SwitchCostumeCommand::SwitchCostumeCommand() = default;

// @func 0x00742858 SwitchCostumeCommand::~SwitchCostumeCommand
// @func 0x00742888 SwitchCostumeCommand::~SwitchCostumeCommand (deleting)
SwitchCostumeCommand::~SwitchCostumeCommand() = default;

// @func 0x000A7018 SwitchCostumeCommand::Create
ApeAnimScriptCommand* SwitchCostumeCommand::Create(XmlElement* element) const
{
    auto* command = new SwitchCostumeCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrDef, &command->m_def, command->m_def);
    element->GetBool(kAttrDef14, &command->m_flag14, command->m_flag14);
    element->GetBool(kAttrDef15, &command->m_flag15, command->m_flag15);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x000A7158 SwitchCostumeCommand::Execute
void SwitchCostumeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_def != 0)
        SequenceAddSwapAnimSet(m_entity, m_def, m_flag14, m_flag15, m_wait);
}

// ---------------------------------------------------------------------------
// "SyncSequence" (#265): waits in the running sequence until every sequence
// listed by the <Seq Name=...> children has finished.

class SyncSequenceCommand : public ApeAnimScriptCommand {
public:
    SyncSequenceCommand();
    explicit SyncSequenceCommand(XmlElement* element);
    ~SyncSequenceCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    std::vector<uint32_t> m_sequences;   // +0x0C (console ApeArray of hashes, capacity 4)
};

// @func 0x000A71EC SyncSequenceCommand::SyncSequenceCommand
SyncSequenceCommand::SyncSequenceCommand() { m_sequences.reserve(4); }

// @func 0x000A7244 SyncSequenceCommand::SyncSequenceCommand (parsing)
SyncSequenceCommand::SyncSequenceCommand(XmlElement* element) : SyncSequenceCommand()
{
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != AnimHash("Seq"))
            continue;
        uint32_t name = 0;
        child->GetHash(AnimHash("Name"), &name, 0);
        m_sequences.push_back(name);
    }
}

// NOTE: the destructors are at 0x007428E0 / 0x00742948 (outside this range).
SyncSequenceCommand::~SyncSequenceCommand() = default;

// @func 0x000A7394 SyncSequenceCommand::Create
ApeAnimScriptCommand* SyncSequenceCommand::Create(XmlElement* element) const
{
    return new SyncSequenceCommand(element);
}

// @func 0x000A7404 SyncSequenceCommand::Execute
void SyncSequenceCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!m_sequences.empty())
        gameplay::AddSequenceCommand(new gameplay::SeqWaitForSequencesCommand(m_sequences));
}

// ---------------------------------------------------------------------------
// Hash 0x2D2232FC (name unknown): waits for a sound group in the sequence.

class SeqWaitForSoundCommand : public ApeAnimScriptCommand {
public:
    SeqWaitForSoundCommand();
    ~SeqWaitForSoundCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_group = 0;             // +0x0C "group"
    uint32_t m_exclusivityGroup = 0;  // +0x10 "ExclusivityGroup"
};

// @func 0x000A7488 SeqWaitForSoundCommand::SeqWaitForSoundCommand
SeqWaitForSoundCommand::SeqWaitForSoundCommand() = default;

// @func 0x007429C8 SeqWaitForSoundCommand::~SeqWaitForSoundCommand
// @func 0x007429F8 SeqWaitForSoundCommand::~SeqWaitForSoundCommand (deleting)
SeqWaitForSoundCommand::~SeqWaitForSoundCommand() = default;

// @func 0x000A74D0 SeqWaitForSoundCommand::Create
ApeAnimScriptCommand* SeqWaitForSoundCommand::Create(XmlElement* element) const
{
    auto* command = new SeqWaitForSoundCommand();
    element->GetHash(kAttrGroup, &command->m_group, 0);
    element->GetHash(kAttrExclusivityGroup, &command->m_exclusivityGroup, command->m_exclusivityGroup);
    return command;
}

// @func 0x000A7598 SeqWaitForSoundCommand::Execute
void SeqWaitForSoundCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddWaitForSound(m_group, m_exclusivityGroup);
}

// ---------------------------------------------------------------------------
// Hash 0x7DE1DF64 (name unknown): sequence variant of the dark screen effect.

class SeqDarkScreenCommand : public ApeAnimScriptCommand {
public:
    SeqDarkScreenCommand();
    ~SeqDarkScreenCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = false;    // +0x0C "enable"
    float m_distance = 0.0f;  // +0x10 "distance"
    float m_ramp = 1.0f;      // +0x14 "Ramp" (default when parsed: 0)
    float m_minValue = 0.0f;  // +0x18 "MinValue"
    float m_fadeTime = 0.0f;  // +0x1C "FadeTime"
};

// @func 0x0009C014 SeqDarkScreenCommand::SeqDarkScreenCommand
SeqDarkScreenCommand::SeqDarkScreenCommand() = default;

// @func 0x00740610 SeqDarkScreenCommand::~SeqDarkScreenCommand
// @func 0x00740640 SeqDarkScreenCommand::~SeqDarkScreenCommand (deleting)
SeqDarkScreenCommand::~SeqDarkScreenCommand() = default;

// @func 0x0009C078 SeqDarkScreenCommand::Create
ApeAnimScriptCommand* SeqDarkScreenCommand::Create(XmlElement* element) const
{
    auto* command = new SeqDarkScreenCommand();
    // As in EnableDarkScreen, every getter gets the default 0.
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetFloat(kAttrDistance, &command->m_distance, 0.0f);
    element->GetFloat(kAttrRamp, &command->m_ramp, 0.0f);
    element->GetFloat(kAttrMinValue, &command->m_minValue, 0.0f);
    element->GetFloat(kAttrFadeTime, &command->m_fadeTime, 0.0f);
    return command;
}

// @func 0x0009C1CC SeqDarkScreenCommand::Execute
void SeqDarkScreenCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddScreenFade(m_distance, m_ramp, m_minValue, m_fadeTime, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0x83F48558 (name unknown): queues an entity state change.

class QueueEntityStateCommand : public ApeAnimScriptCommand {
public:
    QueueEntityStateCommand();
    ~QueueEntityStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject" (parsed, unused)
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_child = 0;      // +0x14 "Child" (parsed, unused)
    uint32_t m_state = 0;      // +0x18 "State"
    bool m_flag1C = false;     // +0x1C attribute 0xF5897C08
    bool m_flag1D = true;      // +0x1D attribute 0xF0CA0A04 (parsed, unused)
};

// @func 0x00099AFC QueueEntityStateCommand::QueueEntityStateCommand
QueueEntityStateCommand::QueueEntityStateCommand() = default;

// @func 0x0073FEB0 QueueEntityStateCommand::~QueueEntityStateCommand
// @func 0x0073FEE0 QueueEntityStateCommand::~QueueEntityStateCommand (deleting)
QueueEntityStateCommand::~QueueEntityStateCommand() = default;

// @func 0x00099B64 QueueEntityStateCommand::Create
ApeAnimScriptCommand* QueueEntityStateCommand::Create(XmlElement* element) const
{
    auto* command = new QueueEntityStateCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    element->GetBool(kAttrRecursive, &command->m_flag1C, command->m_flag1C);
    element->GetBool(kAttrState1D, &command->m_flag1D, command->m_flag1D);
    return command;
}

// @func 0x00099CC8 QueueEntityStateCommand::Execute
void QueueEntityStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    QueueEntityStateRequest(m_entity, m_state, m_flag1C);
}

// ---------------------------------------------------------------------------
// Hash 0x2C1E0CC1 (name unknown): queues an object visibility change.

class QueueObjectVisibleCommand : public ApeAnimScriptCommand {
public:
    QueueObjectVisibleCommand();
    ~QueueObjectVisibleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object" (or "ObjectHash" as a number)
    uint32_t m_part = 0;       // +0x10 "part" (parsed, unused)
    bool m_visible = false;    // +0x14 "visible"
    bool m_recursive = false;  // +0x15 attribute 0xF5897C08
    bool m_flag16 = false;     // +0x16 attribute 0x6334AE6F (parsed, unused)
    bool m_flag17 = false;     // +0x17 attribute 0x834744B0 (parsed, unused)
};

// @func 0x00099CFC QueueObjectVisibleCommand::QueueObjectVisibleCommand
QueueObjectVisibleCommand::QueueObjectVisibleCommand() = default;

// @func 0x00099D5C QueueObjectVisibleCommand::~QueueObjectVisibleCommand
// @func 0x00099D8C QueueObjectVisibleCommand::~QueueObjectVisibleCommand (deleting)
QueueObjectVisibleCommand::~QueueObjectVisibleCommand() = default;

// @func 0x00099DE4 QueueObjectVisibleCommand::Create
ApeAnimScriptCommand* QueueObjectVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new QueueObjectVisibleCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) != ape::xml::kQuery_Ok) {
        int32_t hash = 0;
        element->GetInt(kAttrObjectHash, &hash, 0);
        command->m_object = static_cast<uint32_t>(hash);
    }
    element->GetHash(kAttrPart, &command->m_part, 0);
    element->GetBool(kAttrVisible, &command->m_visible, false);
    element->GetBool(kAttrRecursive, &command->m_recursive, false);
    element->GetBool(kAttrVisible16, &command->m_flag16, false);
    element->GetBool(kAttrVisible17, &command->m_flag17, false);
    return command;
}

// @func 0x00099F6C QueueObjectVisibleCommand::Execute
void QueueObjectVisibleCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    QueueObjectVisibleRequest(m_object, m_visible, m_recursive);
}

// @func 0x00099FA0 QueueObjectVisibleCommand::CreateDebugElement
XmlElement* QueueObjectVisibleCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsAD()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xDAD61703u, new SeqStartStateCommand());
    prototypes.Add(0x390D5F47u, new SeqGroupVolumeCommand());
    prototypes.Add(0x7EA392FCu, new SeqLevelHeadlineCommand());
    // #260 "SeqSpawnEntity" (0x1BC5155F) is not ported yet.
    prototypes.Add(AnimHash("StartSequence"), new StartSequenceCommand());
    prototypes.Add(0xB89DA664u, new SeqStopSoundCommand());
    prototypes.Add(AnimHash("SeqStopMusic"), new SeqStopMusicCommand());
    prototypes.Add(AnimHash("SwitchCostume"), new SwitchCostumeCommand());
    prototypes.Add(AnimHash("SyncSequence"), new SyncSequenceCommand());
    prototypes.Add(0x2D2232FCu, new SeqWaitForSoundCommand());
    prototypes.Add(0x7DE1DF64u, new SeqDarkScreenCommand());
    prototypes.Add(0x83F48558u, new QueueEntityStateCommand());
    prototypes.Add(0x2C1E0CC1u, new QueueObjectVisibleCommand());
}

} // namespace levels
} // namespace sly
