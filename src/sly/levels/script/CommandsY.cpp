// openTIT - Game script commands, twenty-third block (SeqEnableComponent to the depth-of-field fade).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrComponent = AnimHash("component");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrPhysics11 = 0x53B32BA4u;   // NOTE: name unknown
constexpr uint32_t kAttrReset = AnimHash("reset");
constexpr uint32_t kAttrSpawner = AnimHash("Spawner");
constexpr uint32_t kAttrSpawnerGroup = AnimHash("SpawnerGroup");
constexpr uint32_t kAttrSpawnDisable = 0x6557DBDDu;  // NOTE: name unknown, defaults to !enable
constexpr uint32_t kAttrSpawn12 = 0x03A6C100u;       // NOTE: name unknown
constexpr uint32_t kAttrSpawn13 = 0xA92B6296u;       // NOTE: name unknown
constexpr uint32_t kAttrAutoDelete = AnimHash("AutoDelete");
constexpr uint32_t kAttrState = AnimHash("State");
constexpr uint32_t kAttrAnim14 = 0xF5897C08u;        // NOTE: name unknown
constexpr uint32_t kAttrAnim15 = 0x466FBEF5u;        // NOTE: name unknown
constexpr uint32_t kAttrSuppress = AnimHash("Suppress");
constexpr uint32_t kAttrSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrExclusivityGroupPriority = AnimHash("ExclusivityGroupPriority");
constexpr uint32_t kAttrSound20 = 0x6D18DE7Du;       // NOTE: name unknown
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;       // NOTE: name unknown
constexpr uint32_t kAttrSound22 = 0xDB1F009Eu;       // NOTE: name unknown
constexpr uint32_t kAttrStartIntensity = AnimHash("StartIntensity");
constexpr uint32_t kAttrEndIntensity = AnimHash("EndIntensity");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrBlur18 = 0x30BBD29Cu;        // NOTE: name unknown (int)
constexpr uint32_t kAttrBlur1C = 0x26873C74u;        // NOTE: name unknown
constexpr uint32_t kAttrDof0C = 0x653B2EE9u;         // NOTE: names unknown for the depth-of-field values
constexpr uint32_t kAttrDof10 = 0x4B39ED22u;
constexpr uint32_t kAttrDof18 = 0xBE37CBE1u;
constexpr uint32_t kAttrDof1C = 0x8FE1FDC1u;
} // namespace

// ---------------------------------------------------------------------------
// "SeqEnableComponent" (NOTE: name from a hash match)

class SeqEnableComponentCommand : public ApeAnimScriptCommand {
public:
    SeqEnableComponentCommand();
    ~SeqEnableComponentCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;     // +0x0C "Entity"
    uint32_t m_component = 0;  // +0x10 "component"
    bool m_enable = true;      // +0x14 "enable"
};

// @func 0x0009BE48 SeqEnableComponentCommand::SeqEnableComponentCommand
SeqEnableComponentCommand::SeqEnableComponentCommand() = default;

// @func 0x00740588 SeqEnableComponentCommand::~SeqEnableComponentCommand
// @func 0x007405B8 SeqEnableComponentCommand::~SeqEnableComponentCommand (deleting)
SeqEnableComponentCommand::~SeqEnableComponentCommand() = default;

// @func 0x0009BE98 SeqEnableComponentCommand::Create
ApeAnimScriptCommand* SeqEnableComponentCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnableComponentCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrComponent, &command->m_component, command->m_component);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// @func 0x0009BF88 SeqEnableComponentCommand::Execute
void SeqEnableComponentCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_entity != 0)
        SequenceAddEnableComponent(m_entity, m_component, m_enable);
}

// ---------------------------------------------------------------------------
// "EnablePhysics" (NOTE: name from a hash match): enables physics of an entity in the sequence.

class SeqEnablePhysicsCommand : public ApeAnimScriptCommand {
public:
    SeqEnablePhysicsCommand();
    ~SeqEnablePhysicsCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    bool m_enable = true;   // +0x10 "enable"
    bool m_flag11 = false;  // +0x11 attribute 0x53B32BA4
};

// @func 0x0009C254 SeqEnablePhysicsCommand::SeqEnablePhysicsCommand
SeqEnablePhysicsCommand::SeqEnablePhysicsCommand() = default;

// @func 0x00740698 SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand
// @func 0x007406C8 SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand (deleting)
SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand() = default;

// @func 0x0009C2A4 SeqEnablePhysicsCommand::Create
ApeAnimScriptCommand* SeqEnablePhysicsCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnablePhysicsCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    element->GetBool(kAttrPhysics11, &command->m_flag11, command->m_flag11);
    return command;
}

// @func 0x0009C394 SeqEnablePhysicsCommand::Execute
void SeqEnablePhysicsCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_entity != 0)
        SequenceAddEnablePhysics(m_entity, m_enable, m_flag11);
}

// ---------------------------------------------------------------------------
// Hash 0xB164C699 (name unknown): player control during the sequence.

class SeqPlayerControlCommand : public ApeAnimScriptCommand {
public:
    SeqPlayerControlCommand();
    ~SeqPlayerControlCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = true;  // +0x0C "enable"
    bool m_reset = false;  // +0x0D "reset"
};

// @func 0x0009C420 SeqPlayerControlCommand::SeqPlayerControlCommand
SeqPlayerControlCommand::SeqPlayerControlCommand() = default;

// @func 0x00740720 SeqPlayerControlCommand::~SeqPlayerControlCommand
// @func 0x00740750 SeqPlayerControlCommand::~SeqPlayerControlCommand (deleting)
SeqPlayerControlCommand::~SeqPlayerControlCommand() = default;

// @func 0x0009C470 SeqPlayerControlCommand::Create
ApeAnimScriptCommand* SeqPlayerControlCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPlayerControlCommand();
    element->GetBool(kAttrEnable, &command->m_enable, true);
    element->GetBool(kAttrReset, &command->m_reset, command->m_reset);
    return command;
}

// @func 0x0009C53C SeqPlayerControlCommand::Execute
void SeqPlayerControlCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPlayerControl(m_enable, m_reset);
}

// ---------------------------------------------------------------------------
// "SeqEnableSpawner" / "SeqEnableSpawnerGroup" (NOTE: names from hash matches)

class SeqEnableSpawnerCommand : public ApeAnimScriptCommand {
public:
    SeqEnableSpawnerCommand();
    ~SeqEnableSpawnerCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_spawner = 0;  // +0x0C "Spawner"
    bool m_enable = true;    // +0x10 "enable"
    bool m_disable = true;   // +0x11 attribute 0x6557DBDD (default !enable)
    bool m_flag12 = false;   // +0x12 attribute 0x03A6C100
    bool m_flag13 = false;   // +0x13 attribute 0xA92B6296
    bool m_reset = false;    // +0x14 "reset"
    bool m_flag15 = false;   // +0x15 (never parsed)
};

// @func 0x0009C5B4 SeqEnableSpawnerCommand::SeqEnableSpawnerCommand
SeqEnableSpawnerCommand::SeqEnableSpawnerCommand() = default;

// @func 0x007407A8 SeqEnableSpawnerCommand::~SeqEnableSpawnerCommand
// @func 0x007407D8 SeqEnableSpawnerCommand::~SeqEnableSpawnerCommand (deleting)
SeqEnableSpawnerCommand::~SeqEnableSpawnerCommand() = default;

// @func 0x0009C614 SeqEnableSpawnerCommand::Create
ApeAnimScriptCommand* SeqEnableSpawnerCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnableSpawnerCommand();
    element->GetHash(kAttrSpawner, &command->m_spawner, 0);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    command->m_disable = !command->m_enable;
    element->GetBool(kAttrSpawnDisable, &command->m_disable, command->m_disable);
    element->GetBool(kAttrSpawn12, &command->m_flag12, command->m_flag12);
    element->GetBool(kAttrReset, &command->m_reset, command->m_reset);
    element->GetBool(kAttrSpawn13, &command->m_flag13, command->m_flag13);
    return command;
}

// @func 0x0009C788 SeqEnableSpawnerCommand::Execute
void SeqEnableSpawnerCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddEnableSpawner(m_spawner, m_enable, m_disable, m_flag12, m_flag13, m_reset, m_flag15);
}

class SeqEnableSpawnerGroupCommand : public ApeAnimScriptCommand {
public:
    SeqEnableSpawnerGroupCommand();
    ~SeqEnableSpawnerGroupCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_group = 0;   // +0x0C "SpawnerGroup"
    bool m_enable = true;   // +0x10 "enable"
    bool m_disable = true;  // +0x11 attribute 0x6557DBDD (default !enable)
    bool m_flag12 = false;  // +0x12 attribute 0x03A6C100
    bool m_reset = false;   // +0x13 "reset"
    bool m_flag14 = false;  // +0x14 (never parsed)
};

// @func 0x0009C818 SeqEnableSpawnerGroupCommand::SeqEnableSpawnerGroupCommand
SeqEnableSpawnerGroupCommand::SeqEnableSpawnerGroupCommand() = default;

// @func 0x00740830 SeqEnableSpawnerGroupCommand::~SeqEnableSpawnerGroupCommand
// @func 0x00740860 SeqEnableSpawnerGroupCommand::~SeqEnableSpawnerGroupCommand (deleting)
SeqEnableSpawnerGroupCommand::~SeqEnableSpawnerGroupCommand() = default;

// @func 0x0009C878 SeqEnableSpawnerGroupCommand::Create
ApeAnimScriptCommand* SeqEnableSpawnerGroupCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnableSpawnerGroupCommand();
    element->GetHash(kAttrSpawnerGroup, &command->m_group, 0);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    command->m_disable = !command->m_enable;
    element->GetBool(kAttrSpawnDisable, &command->m_disable, command->m_disable);
    element->GetBool(kAttrSpawn12, &command->m_flag12, command->m_flag12);
    element->GetBool(kAttrReset, &command->m_reset, command->m_reset);
    return command;
}

// @func 0x0009C9C4 SeqEnableSpawnerGroupCommand::Execute
void SeqEnableSpawnerGroupCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddEnableSpawnerGroup(m_group, m_enable, m_disable, m_flag12, m_reset, m_flag14);
}

// ---------------------------------------------------------------------------
// "EndSequence" (NOTE: name from a hash match)

class EndSequenceCommand : public ApeAnimScriptCommand {
public:
    EndSequenceCommand();
    ~EndSequenceCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_autoDelete = true;  // +0x0C "AutoDelete"
};

// @func 0x0009CA50 EndSequenceCommand::EndSequenceCommand
EndSequenceCommand::EndSequenceCommand() = default;

// @func 0x007408B8 EndSequenceCommand::~EndSequenceCommand
// @func 0x007408E8 EndSequenceCommand::~EndSequenceCommand (deleting)
EndSequenceCommand::~EndSequenceCommand() = default;

// @func 0x0009CA98 EndSequenceCommand::Create
ApeAnimScriptCommand* EndSequenceCommand::Create(XmlElement* element) const
{
    auto* command = new EndSequenceCommand();
    element->GetBool(kAttrAutoDelete, &command->m_autoDelete, true);
    return command;
}

// @func 0x0009CB40 EndSequenceCommand::Execute
void EndSequenceCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_autoDelete)
        SequenceAddEndSequence();
    SequenceEnd();
}

// ---------------------------------------------------------------------------
// Hash 0x9F872A66 (name unknown): changes an entity's script state in the sequence.

class SeqSetStateCommand : public ApeAnimScriptCommand {
public:
    SeqSetStateCommand();
    ~SeqSetStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_state = 0;   // +0x10 "State"
    bool m_flag14 = false;  // +0x14 attribute 0xF5897C08
    bool m_flag15 = false;  // +0x15 attribute 0x466FBEF5
};

// @func 0x0009CBB8 SeqSetStateCommand::SeqSetStateCommand
SeqSetStateCommand::SeqSetStateCommand() = default;

// @func 0x00740940 SeqSetStateCommand::~SeqSetStateCommand
// @func 0x00740970 SeqSetStateCommand::~SeqSetStateCommand (deleting)
SeqSetStateCommand::~SeqSetStateCommand() = default;

// @func 0x0009CC08 SeqSetStateCommand::Create
ApeAnimScriptCommand* SeqSetStateCommand::Create(XmlElement* element) const
{
    auto* command = new SeqSetStateCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    element->GetBool(kAttrAnim14, &command->m_flag14, command->m_flag14);
    element->GetBool(kAttrAnim15, &command->m_flag15, command->m_flag15);
    return command;
}

// @func 0x0009CD1C SeqSetStateCommand::Execute
void SeqSetStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPlayAnimation(m_entity, m_state, m_flag14, m_flag15);
}

// ---------------------------------------------------------------------------
// Hash 0x631AC87F (name unknown): suppresses an entity's audio in the sequence.

class SeqSuppressAudioCommand : public ApeAnimScriptCommand {
public:
    SeqSuppressAudioCommand();
    ~SeqSuppressAudioCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;   // +0x0C "Entity"
    bool m_suppress = true;  // +0x10 "Suppress"
};

// @func 0x0009CDA0 SeqSuppressAudioCommand::SeqSuppressAudioCommand
SeqSuppressAudioCommand::SeqSuppressAudioCommand() = default;

// @func 0x0073FF38 SeqSuppressAudioCommand::~SeqSuppressAudioCommand
// @func 0x007409C8 SeqSuppressAudioCommand::~SeqSuppressAudioCommand (deleting)
SeqSuppressAudioCommand::~SeqSuppressAudioCommand() = default;

// @func 0x0009CDF0 SeqSuppressAudioCommand::Create
ApeAnimScriptCommand* SeqSuppressAudioCommand::Create(XmlElement* element) const
{
    auto* command = new SeqSuppressAudioCommand();
    element->GetBool(kAttrSuppress, &command->m_suppress, true);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    return command;
}

// @func 0x0009CEBC SeqSuppressAudioCommand::Execute
void SeqSuppressAudioCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddSuppressAudio(m_entity, m_suppress);
}

// ---------------------------------------------------------------------------
// "SeqEnqueueSound" (NOTE: name from a hash match)

class SeqEnqueueSoundCommand : public ApeAnimScriptCommand {
public:
    SeqEnqueueSoundCommand();
    ~SeqEnqueueSoundCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    void RefreshSound();

    uint32_t m_soundName = 0;             // +0x0C "SoundName"
    uint32_t m_bank = 0;                  // +0x10 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x14
    uint32_t m_exclusivityGroup = 0;      // +0x18 "ExclusivityGroup"
    int32_t m_exclusivityPriority = 0;    // +0x1C "ExclusivityGroupPriority"
    bool m_flag20 = false;                // +0x20 attribute 0x6D18DE7D
    bool m_wait = false;                  // +0x21 attribute 0x8488565B
    bool m_flag22 = false;                // +0x22 attribute 0xDB1F009E
};

// @func 0x0009CF34 SeqEnqueueSoundCommand::SeqEnqueueSoundCommand
SeqEnqueueSoundCommand::SeqEnqueueSoundCommand() = default;

void SeqEnqueueSoundCommand::RefreshSound()
{
    if (m_soundName != 0 && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_soundName, m_bank);
    }
}

// @func 0x0009CF8C SeqEnqueueSoundCommand::Link
// (byte-identical to EnqueueSoundCommand::Link 0x000740B0 and SeqSetMusicCommand::Link)
void SeqEnqueueSoundCommand::Link()
{
    RefreshSound();
}

// @func 0x00740A20 SeqEnqueueSoundCommand::~SeqEnqueueSoundCommand
// @func 0x00740A6C SeqEnqueueSoundCommand::~SeqEnqueueSoundCommand (deleting)
SeqEnqueueSoundCommand::~SeqEnqueueSoundCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x0009CFF4 SeqEnqueueSoundCommand::Create
ApeAnimScriptCommand* SeqEnqueueSoundCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnqueueSoundCommand();
    element->GetHash(kAttrSoundName, &command->m_soundName, 0);
    element->GetHash(kAttrBank, &command->m_bank, command->m_bank);
    element->GetHash(kAttrExclusivityGroup, &command->m_exclusivityGroup, command->m_exclusivityGroup);
    element->GetInt(kAttrExclusivityGroupPriority, &command->m_exclusivityPriority,
                    command->m_exclusivityPriority);
    element->GetBool(kAttrSound20, &command->m_flag20, command->m_flag20);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    element->GetBool(kAttrSound22, &command->m_flag22, command->m_flag22);
    command->RefreshSound();
    return command;
}

// @func 0x0009D1C0 SeqEnqueueSoundCommand::Execute
void SeqEnqueueSoundCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    // Only streamed sounds with variants can be queued; anything else only
    // warns in debug builds.
    if (!audio::SoundNameHasDef(m_sound) || !audio::SoundNameHasVariants(&m_sound))
        return;
    SequenceAddEnqueueStream(m_sound, m_exclusivityGroup, m_exclusivityPriority, m_flag20, m_wait, m_flag22);
}

// ---------------------------------------------------------------------------
// "FadeBlur" (NOTE: name from a hash match): sequence colour filter fade.

class FadeBlurCommand : public ApeAnimScriptCommand {
public:
    FadeBlurCommand();
    ~FadeBlurCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_startIntensity = 0.0f;  // +0x0C "StartIntensity"
    float m_endIntensity = 1.0f;    // +0x10 "EndIntensity"
    float m_duration = 1.0f;        // +0x14 "duration"
    int32_t m_value18 = 1;          // +0x18 attribute 0x30BBD29C
    bool m_flag1C = false;          // +0x1C attribute 0x26873C74
    bool m_wait = true;             // +0x1D attribute 0x8488565B
};

// @func 0x0009D2A4 FadeBlurCommand::FadeBlurCommand
FadeBlurCommand::FadeBlurCommand() = default;

// @func 0x00740AD4 FadeBlurCommand::~FadeBlurCommand
// @func 0x00740B04 FadeBlurCommand::~FadeBlurCommand (deleting)
FadeBlurCommand::~FadeBlurCommand() = default;

// @func 0x0009D310 FadeBlurCommand::Create
ApeAnimScriptCommand* FadeBlurCommand::Create(XmlElement* element) const
{
    auto* command = new FadeBlurCommand();
    element->GetFloat(kAttrStartIntensity, &command->m_startIntensity, 0.0f);
    element->GetFloat(kAttrEndIntensity, &command->m_endIntensity, command->m_endIntensity);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetInt(kAttrBlur18, &command->m_value18, command->m_value18);
    element->GetBool(kAttrBlur1C, &command->m_flag1C, command->m_flag1C);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x0009D484 FadeBlurCommand::Execute
void FadeBlurCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddColorFilter(m_startIntensity, m_endIntensity, m_duration, m_value18, m_flag1C, m_wait);
}

// ---------------------------------------------------------------------------
// Hash 0xDDB84C75 (name unknown): sequence depth-of-field fade.

class FadeDofCommand : public ApeAnimScriptCommand {
public:
    FadeDofCommand();
    ~FadeDofCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_value0C = 1.0f;    // +0x0C attribute 0x653B2EE9
    float m_value10 = 0.25f;   // +0x10 attribute 0x4B39ED22
    float m_duration = -1.0f;  // +0x14 "duration"
    float m_value18 = 0.0f;    // +0x18 attribute 0xBE37CBE1
    bool m_flag1C = false;     // +0x1C attribute 0x8FE1FDC1
    bool m_wait = true;        // +0x1D attribute 0x8488565B
};

// @func 0x0009D514 FadeDofCommand::FadeDofCommand
FadeDofCommand::FadeDofCommand() = default;

// @func 0x00740B5C FadeDofCommand::~FadeDofCommand
// @func 0x00740B8C FadeDofCommand::~FadeDofCommand (deleting)
FadeDofCommand::~FadeDofCommand() = default;

// @func 0x0009D580 FadeDofCommand::Create
ApeAnimScriptCommand* FadeDofCommand::Create(XmlElement* element) const
{
    auto* command = new FadeDofCommand();
    element->GetFloat(kAttrDof0C, &command->m_value0C, 1.0f);
    element->GetFloat(kAttrDof10, &command->m_value10, command->m_value10);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetFloat(kAttrDof18, &command->m_value18, command->m_value18);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    element->GetBool(kAttrDof1C, &command->m_flag1C, command->m_flag1C);
    return command;
}

// @func 0x0009D708 FadeDofCommand::Execute
void FadeDofCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_value0C >= 0.0f && m_value10 >= 0.0f)
        SequenceAddDepthOfField(m_value0C, m_value10, m_duration, m_value18, m_flag1C, m_wait);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsY()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SeqEnableComponent"), new SeqEnableComponentCommand());
    prototypes.Add(AnimHash("EnablePhysics"), new SeqEnablePhysicsCommand());
    prototypes.Add(0xB164C699u, new SeqPlayerControlCommand());
    prototypes.Add(AnimHash("SeqEnableSpawner"), new SeqEnableSpawnerCommand());
    prototypes.Add(AnimHash("SeqEnableSpawnerGroup"), new SeqEnableSpawnerGroupCommand());
    prototypes.Add(AnimHash("EndSequence"), new EndSequenceCommand());
    prototypes.Add(0x9F872A66u, new SeqSetStateCommand());
    prototypes.Add(0x631AC87Fu, new SeqSuppressAudioCommand());
    prototypes.Add(AnimHash("SeqEnqueueSound"), new SeqEnqueueSoundCommand());
    prototypes.Add(AnimHash("FadeBlur"), new FadeBlurCommand());
    prototypes.Add(0xDDB84C75u, new FadeDofCommand());
}

} // namespace levels
} // namespace sly
