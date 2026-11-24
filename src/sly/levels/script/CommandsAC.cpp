// openTIT - Game script commands, twenty-seventh block (sequence time limit to SetPosition).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrLimit = AnimHash("limit");
constexpr uint32_t kAttrSkippable = AnimHash("Skippable");
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrParent = AnimHash("Parent");
constexpr uint32_t kAttrVar = AnimHash("var");
constexpr uint32_t kAttrNameOverride = AnimHash("nameOverride");
constexpr uint32_t kAttrSetEntity1C = 0x12E3611Bu;  // NOTE: names unknown for the SetEntity values
constexpr uint32_t kSetEntityFlagAttrs[9] = {
    0x3D1FC26Cu, 0x2CC1B490u, 0x5D4879B3u, 0x0ADE6CD2u, 0xF1559E55u,
    0x466FBEF5u, 0x9304C8C5u, 0x05D5430Fu, 0x971C59D9u,
};
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrUpdate = AnimHash("update");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrRecursive = 0xF5897C08u;    // NOTE: name unknown
constexpr uint32_t kAttrVisible16 = 0xCD9F562Bu;    // NOTE: name unknown
constexpr uint32_t kAttrProcessComponents = AnimHash("ProcessComponents");
constexpr uint32_t kAttrVisible18 = 0x80116C17u;    // NOTE: name unknown
constexpr uint32_t kAttrEmitterParam = 0x39908DCAu; // NOTE: name unknown (int)
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;      // NOTE: name unknown
constexpr uint32_t kAttrMusic = AnimHash("Music");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrMusic18 = 0x83F172B0u;      // NOTE: name unknown, default true
constexpr uint32_t kAttrMusic19 = 0x0138B87Fu;      // NOTE: name unknown
constexpr uint32_t kAttrAllowed = AnimHash("allowed");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrPosition = 0xCD8B05D5u;     // NOTE: name unknown (position vector)
constexpr uint32_t kAttrOffsetRot = AnimHash("OffsetRot");
constexpr uint32_t kAttrPtName = 0x8D47DA8Fu;       // NOTE: name unknown (point instance name)
constexpr uint32_t kAttrPtVarName = 0xA4304B8Eu;    // NOTE: name unknown
constexpr uint32_t kAttrPos3C = 0x4BA42764u;        // NOTE: names unknown for the next attributes
constexpr uint32_t kAttrPos40 = 0x04DF3157u;
constexpr uint32_t kAttrPos44 = 0x2FA79D7Fu;
constexpr uint32_t kAttrPos45 = 0xA18DAB9Fu;
constexpr uint32_t kAttrPos46 = 0x3FC9B95Au;

constexpr float kDegToRad = 0.017453292f;
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};  // NOTE: console global 0x01164B40
} // namespace

// ---------------------------------------------------------------------------
// Hash 0xCF110608 (name unknown): sequence time limit.

class SeqTimeLimitCommand : public ApeAnimScriptCommand {
public:
    SeqTimeLimitCommand();
    ~SeqTimeLimitCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_limit = 0.0f;  // +0x0C "limit"
};

// @func 0x000A42C0 SeqTimeLimitCommand::SeqTimeLimitCommand
SeqTimeLimitCommand::SeqTimeLimitCommand() = default;

// @func 0x00741E44 SeqTimeLimitCommand::~SeqTimeLimitCommand
// @func 0x00741E74 SeqTimeLimitCommand::~SeqTimeLimitCommand (deleting)
SeqTimeLimitCommand::~SeqTimeLimitCommand() = default;

// @func 0x000A4318 SeqTimeLimitCommand::Create
ApeAnimScriptCommand* SeqTimeLimitCommand::Create(XmlElement* element) const
{
    auto* command = new SeqTimeLimitCommand();
    element->GetFloat(kAttrLimit, &command->m_limit, 0.0f);
    return command;
}

// @func 0x000A43C4 SeqTimeLimitCommand::Execute
void SeqTimeLimitCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddTimeLimit(m_limit);
}

// ---------------------------------------------------------------------------
// Hash 0xE61291FE (name unknown): marks the sequence skippable.

class SeqSkippableCommand : public ApeAnimScriptCommand {
public:
    SeqSkippableCommand();
    ~SeqSkippableCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_skippable = true;  // +0x0C "Skippable"
};

// @func 0x000A443C SeqSkippableCommand::SeqSkippableCommand
SeqSkippableCommand::SeqSkippableCommand() = default;

// @func 0x00741ECC SeqSkippableCommand::~SeqSkippableCommand
// @func 0x00741EFC SeqSkippableCommand::~SeqSkippableCommand (deleting)
SeqSkippableCommand::~SeqSkippableCommand() = default;

// @func 0x000A4484 SeqSkippableCommand::Create
ApeAnimScriptCommand* SeqSkippableCommand::Create(XmlElement* element) const
{
    auto* command = new SeqSkippableCommand();
    element->GetBool(kAttrSkippable, &command->m_skippable, true);
    return command;
}

// @func 0x000A452C SeqSkippableCommand::Execute
void SeqSkippableCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddCutsceneFlag(m_skippable);
}

// ---------------------------------------------------------------------------
// "SetEntity" (NOTE: name from a hash match): binds an entity to a sequence
// variable.

class SetEntityCommand : public ApeAnimScriptCommand {
public:
    SetEntityCommand();
    explicit SetEntityCommand(XmlElement* element);
    ~SetEntityCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;          // +0x0C "name"
    uint32_t m_parent = 0;        // +0x10 "Parent"
    uint32_t m_var = 0;           // +0x14 "var"
    uint32_t m_nameOverride = 0;  // +0x18 "nameOverride"
    uint32_t m_hash1C = 0;        // +0x1C attribute 0x12E3611B
    bool m_flags[9] = {};         // +0x20..+0x28 (attributes in kSetEntityFlagAttrs)
};

// @func 0x000A4878 SetEntityCommand::SetEntityCommand
SetEntityCommand::SetEntityCommand() = default;

// @func 0x00742064 SetEntityCommand::~SetEntityCommand
// @func 0x00742094 SetEntityCommand::~SetEntityCommand (deleting)
SetEntityCommand::~SetEntityCommand() = default;

// @func 0x000A48F0 SetEntityCommand::SetEntityCommand
SetEntityCommand::SetEntityCommand(XmlElement* element)
{
    element->GetHash(kAttrName, &m_name, 0);
    element->GetHash(kAttrParent, &m_parent, m_parent);
    element->GetHash(kAttrVar, &m_var, m_var);
    element->GetHash(kAttrNameOverride, &m_nameOverride, m_nameOverride);
    element->GetHash(kAttrSetEntity1C, &m_hash1C, m_hash1C);
    for (int i = 0; i < 9; ++i)
        element->GetBool(kSetEntityFlagAttrs[i], &m_flags[i], m_flags[i]);
}

// @func 0x000A4B48 SetEntityCommand::Create
ApeAnimScriptCommand* SetEntityCommand::Create(XmlElement* element) const
{
    return new SetEntityCommand(element);
}

// TODO(0x000A4BB8 SetEntityCommand::Execute) resolves the entity and builds the sequence
// entity binding command (0x004386C4).

// ---------------------------------------------------------------------------
// "SetEntityUpdate" (NOTE: name from a hash match)

class SetEntityUpdateCommand : public ApeAnimScriptCommand {
public:
    SetEntityUpdateCommand();
    ~SetEntityUpdateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    bool m_update = false;  // +0x10 "update"
};

// @func 0x000A536C SetEntityUpdateCommand::SetEntityUpdateCommand
SetEntityUpdateCommand::SetEntityUpdateCommand() = default;

// @func 0x00742284 SetEntityUpdateCommand::~SetEntityUpdateCommand
// @func 0x007422B4 SetEntityUpdateCommand::~SetEntityUpdateCommand (deleting)
SetEntityUpdateCommand::~SetEntityUpdateCommand() = default;

// @func 0x000A53B4 SetEntityUpdateCommand::Create
ApeAnimScriptCommand* SetEntityUpdateCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntityUpdateCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetBool(kAttrUpdate, &command->m_update, command->m_update);
    return command;
}

// @func 0x000A5480 SetEntityUpdateCommand::Execute
void SetEntityUpdateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_entity != 0)
        SequenceAddSetEntityUpdate(m_entity, m_update);
}

// ---------------------------------------------------------------------------
// "SetEntityVisible" (NOTE: name from a hash match)

class SetEntityVisibleCommand : public ApeAnimScriptCommand {
public:
    SetEntityVisibleCommand();
    ~SetEntityVisibleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;            // +0x0C "Entity"
    uint32_t m_child = 0;             // +0x10 "Child"
    bool m_visible = false;           // +0x14 "visible"
    bool m_recursive = true;          // +0x15 attribute 0xF5897C08
    bool m_flag16 = true;             // +0x16 attribute 0xCD9F562B
    bool m_processComponents = true;  // +0x17 "ProcessComponents"
    bool m_flag18 = false;            // +0x18 attribute 0x80116C17
};

// @func 0x000A50E4 SetEntityVisibleCommand::SetEntityVisibleCommand
SetEntityVisibleCommand::SetEntityVisibleCommand() = default;

// @func 0x007421FC SetEntityVisibleCommand::~SetEntityVisibleCommand
// @func 0x0074222C SetEntityVisibleCommand::~SetEntityVisibleCommand (deleting)
SetEntityVisibleCommand::~SetEntityVisibleCommand() = default;

// @func 0x000A5144 SetEntityVisibleCommand::Create
ApeAnimScriptCommand* SetEntityVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntityVisibleCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    element->GetBool(kAttrRecursive, &command->m_recursive, command->m_recursive);
    element->GetBool(kAttrVisible16, &command->m_flag16, command->m_flag16);
    element->GetBool(kAttrProcessComponents, &command->m_processComponents, command->m_processComponents);
    element->GetBool(kAttrVisible18, &command->m_flag18, command->m_flag18);
    return command;
}

// @func 0x000A52D0 SetEntityVisibleCommand::Execute
void SetEntityVisibleCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_entity != 0)
        SequenceAddSetEntityVisible(m_entity, m_child, m_visible, m_recursive, m_flag16, m_processComponents,
                                    m_flag18);
}

// ---------------------------------------------------------------------------
// Hash 0x7BF7E38C (name unknown): sets an emitter parameter of an entity.

class SeqEmitterParamCommand : public ApeAnimScriptCommand {
public:
    SeqEmitterParamCommand();
    ~SeqEmitterParamCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    int32_t m_value = 1;    // +0x10 attribute 0x39908DCA
};

// @func 0x000A4DB4 SeqEmitterParamCommand::SeqEmitterParamCommand
SeqEmitterParamCommand::SeqEmitterParamCommand() = default;

// @func 0x007420EC SeqEmitterParamCommand::~SeqEmitterParamCommand
// @func 0x0074211C SeqEmitterParamCommand::~SeqEmitterParamCommand (deleting)
SeqEmitterParamCommand::~SeqEmitterParamCommand() = default;

// @func 0x000A4E04 SeqEmitterParamCommand::Create
ApeAnimScriptCommand* SeqEmitterParamCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEmitterParamCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetInt(kAttrEmitterParam, &command->m_value, command->m_value);
    return command;
}

// @func 0x000A4ED4 SeqEmitterParamCommand::Execute
void SeqEmitterParamCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddEmitterParam(m_entity, m_value);
}

// ---------------------------------------------------------------------------
// Hash 0x2CB7712F (name unknown): sequence letterbox bars.

class SeqLetterboxCommand : public ApeAnimScriptCommand {
public:
    SeqLetterboxCommand();
    ~SeqLetterboxCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_visible = false;  // +0x0C "visible"
    bool m_wait = true;      // +0x0D attribute 0x8488565B
};

// @func 0x000A4F4C SeqLetterboxCommand::SeqLetterboxCommand
SeqLetterboxCommand::SeqLetterboxCommand() = default;

// @func 0x00742174 SeqLetterboxCommand::~SeqLetterboxCommand
// @func 0x007421A4 SeqLetterboxCommand::~SeqLetterboxCommand (deleting)
SeqLetterboxCommand::~SeqLetterboxCommand() = default;

// @func 0x000A4F9C SeqLetterboxCommand::Create
ApeAnimScriptCommand* SeqLetterboxCommand::Create(XmlElement* element) const
{
    auto* command = new SeqLetterboxCommand();
    element->GetBool(kAttrVisible, &command->m_visible, false);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x000A506C SeqLetterboxCommand::Execute
void SeqLetterboxCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddLetterbox(m_visible, m_wait);
}

// ---------------------------------------------------------------------------
// "SeqSetMusic"

class SeqSetMusicCommand : public ApeAnimScriptCommand {
public:
    SeqSetMusicCommand();
    ~SeqSetMusicCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    void RefreshSound();

    uint32_t m_music = 0;                 // +0x0C "Music"
    uint32_t m_bank = 0;                  // +0x10 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x14
    bool m_flag18 = true;                 // +0x18 attribute 0x83F172B0
    bool m_flag19 = false;                // +0x19 attribute 0x0138B87F
};

// @func 0x000A5504 SeqSetMusicCommand::SeqSetMusicCommand
SeqSetMusicCommand::SeqSetMusicCommand() = default;

void SeqSetMusicCommand::RefreshSound()
{
    if (m_music != 0 && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_music, m_bank);
    }
}

// @func 0x000A555C SeqSetMusicCommand::Link
// (byte-identical to EnqueueSoundCommand::Link 0x000740B0)
void SeqSetMusicCommand::Link()
{
    RefreshSound();
}

// @func 0x0074230C SeqSetMusicCommand::~SeqSetMusicCommand
// @func 0x00742358 SeqSetMusicCommand::~SeqSetMusicCommand (deleting)
SeqSetMusicCommand::~SeqSetMusicCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x000A55C4 SeqSetMusicCommand::Create
ApeAnimScriptCommand* SeqSetMusicCommand::Create(XmlElement* element) const
{
    auto* command = new SeqSetMusicCommand();
    element->GetHash(kAttrMusic, &command->m_music, 0);
    element->GetHash(kAttrBank, &command->m_bank, 0);
    element->GetBool(kAttrMusic18, &command->m_flag18, command->m_flag18);
    element->GetBool(kAttrMusic19, &command->m_flag19, command->m_flag19);
    command->RefreshSound();
    return command;
}

// @func 0x000A571C SeqSetMusicCommand::Execute
void SeqSetMusicCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!audio::SoundNameHasDef(m_sound)) {
        ape::TempFormat("SeqSetMusic fails to find sound %s (in bank %s)", ape::TempHexString(m_music),
                        ape::TempHexString(m_bank));
        return;
    }
    SequenceAddSetMusic(m_sound, m_flag18, m_flag19);
}

// ---------------------------------------------------------------------------
// The sequence switches with an "allowed" flag: AI and player pause.

// Hash 0xF6F2D4D6 (name unknown): SeqPauseAiCommand.
class SeqPauseAiCommand : public ApeAnimScriptCommand {
public:
    SeqPauseAiCommand();
    ~SeqPauseAiCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_allowed = true;  // +0x0C "allowed"
};

// @func 0x000A4710 SeqPauseAiCommand::SeqPauseAiCommand
SeqPauseAiCommand::SeqPauseAiCommand() = default;

// @func 0x00741FDC SeqPauseAiCommand::~SeqPauseAiCommand
// @func 0x0074200C SeqPauseAiCommand::~SeqPauseAiCommand (deleting)
SeqPauseAiCommand::~SeqPauseAiCommand() = default;

// @func 0x000A4758 SeqPauseAiCommand::Create
ApeAnimScriptCommand* SeqPauseAiCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPauseAiCommand();
    element->GetBool(kAttrAllowed, &command->m_allowed, true);
    return command;
}

// @func 0x000A4804 SeqPauseAiCommand::Execute
void SeqPauseAiCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPauseAi(m_allowed);
}

// Hash 0xAC54A838 (name unknown): SeqPausePlayerCommand.
class SeqPausePlayerCommand : public ApeAnimScriptCommand {
public:
    SeqPausePlayerCommand();
    ~SeqPausePlayerCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_allowed = true;  // +0x0C "allowed"
};

// @func 0x000A45A4 SeqPausePlayerCommand::SeqPausePlayerCommand
SeqPausePlayerCommand::SeqPausePlayerCommand() = default;

// @func 0x00741F54 SeqPausePlayerCommand::~SeqPausePlayerCommand
// @func 0x00741F84 SeqPausePlayerCommand::~SeqPausePlayerCommand (deleting)
SeqPausePlayerCommand::~SeqPausePlayerCommand() = default;

// @func 0x000A45EC SeqPausePlayerCommand::Create
ApeAnimScriptCommand* SeqPausePlayerCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPausePlayerCommand();
    element->GetBool(kAttrAllowed, &command->m_allowed, true);
    return command;
}

// @func 0x000A4698 SeqPausePlayerCommand::Execute
void SeqPausePlayerCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPausePlayer(m_allowed);
}

// ---------------------------------------------------------------------------
// Hash 0x9EAE7138 (name unknown): enables a part of an object in the sequence.

class SeqEnablePartCommand : public ApeAnimScriptCommand {
public:
    SeqEnablePartCommand();
    ~SeqEnablePartCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    uint32_t m_part = 0;       // +0x10 "part"
    bool m_enable = false;     // +0x14 "enable"
    bool m_recursive = false;  // +0x15 attribute 0xF5897C08
};

// @func 0x000A4078 SeqEnablePartCommand::SeqEnablePartCommand
SeqEnablePartCommand::SeqEnablePartCommand() = default;

// @func 0x00741DBC SeqEnablePartCommand::~SeqEnablePartCommand
// @func 0x00741DEC SeqEnablePartCommand::~SeqEnablePartCommand (deleting)
SeqEnablePartCommand::~SeqEnablePartCommand() = default;

// @func 0x000A40C8 SeqEnablePartCommand::Create
ApeAnimScriptCommand* SeqEnablePartCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnablePartCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrPart, &command->m_part, command->m_part);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    element->GetBool(kAttrRecursive, &command->m_recursive, command->m_recursive);
    return command;
}

// TODO(0x000A41E0 SeqEnablePartCommand::Execute) resolves the object and builds the
// sequence part command (0x00438348).

// ---------------------------------------------------------------------------
// Hash 0x2C6A515D (name unknown): shows a part of an entity in the sequence.

class SeqShowPartCommand : public ApeAnimScriptCommand {
public:
    SeqShowPartCommand();
    ~SeqShowPartCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;       // +0x0C "name"
    uint32_t m_part = 0;       // +0x10 "part"
    bool m_visible = false;    // +0x14 "visible"
    bool m_recursive = false;  // +0x15 attribute 0xF5897C08
};

// @func 0x000A57EC SeqShowPartCommand::SeqShowPartCommand
SeqShowPartCommand::SeqShowPartCommand() = default;

// @func 0x007423C0 SeqShowPartCommand::~SeqShowPartCommand
// @func 0x007423F0 SeqShowPartCommand::~SeqShowPartCommand (deleting)
SeqShowPartCommand::~SeqShowPartCommand() = default;

// @func 0x000A583C SeqShowPartCommand::Create
ApeAnimScriptCommand* SeqShowPartCommand::Create(XmlElement* element) const
{
    auto* command = new SeqShowPartCommand();
    element->GetHash(kAttrName, &command->m_name, 0);
    element->GetHash(kAttrPart, &command->m_part, command->m_part);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    element->GetBool(kAttrRecursive, &command->m_recursive, command->m_recursive);
    return command;
}

// TODO(0x000A5950 SeqShowPartCommand::Execute) builds the sequence part visibility command
// (0x004390AC).

// ---------------------------------------------------------------------------
// "SetPosition" (NOTE: name from a hash match): places an entity in the sequence.

class SetPositionCommand : public ApeAnimScriptCommand {
public:
    SetPositionCommand();
    explicit SetPositionCommand(XmlElement* element);
    ~SetPositionCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::Vec3 m_position = kZero;  // +0x10 attribute 0xCD8B05D5
    ape::Vec3 m_rotation = kZero;  // +0x20 "OffsetRot" (degrees in data, radians here)
    uint32_t m_entity = 0;         // +0x30 "Entity"
    uint32_t m_point = 0;          // +0x34 attribute 0x8D47DA8F
    uint32_t m_pointVar = 0;       // +0x38 attribute 0xA4304B8E
    uint32_t m_hash3C = 0;         // +0x3C attribute 0x4BA42764
    uint32_t m_hash40 = 0;         // +0x40 attribute 0x04DF3157
    bool m_flag44 = false;         // +0x44 attribute 0x2FA79D7F
    bool m_flag45 = true;          // +0x45 attribute 0xA18DAB9F
    bool m_flag46 = false;         // +0x46 attribute 0x3FC9B95A
};

// @func 0x000A5A04 SetPositionCommand::SetPositionCommand
SetPositionCommand::SetPositionCommand() = default;

// @func 0x00742448 SetPositionCommand::~SetPositionCommand
// @func 0x00742478 SetPositionCommand::~SetPositionCommand (deleting)
SetPositionCommand::~SetPositionCommand() = default;

// @func 0x000A5AA8 SetPositionCommand::SetPositionCommand
SetPositionCommand::SetPositionCommand(XmlElement* element)
{
    element->GetVec3(kAttrPosition, &m_position.x, &m_position.x);
    if (element->GetVec3(kAttrOffsetRot, &m_rotation.x, &m_rotation.x) == ape::xml::kQuery_Ok) {
        m_rotation.x *= kDegToRad;
        m_rotation.y *= kDegToRad;
        m_rotation.z *= kDegToRad;
    }
    element->GetHash(kAttrEntity, &m_entity, m_entity);
    element->GetHash(kAttrPtName, &m_point, m_point);
    element->GetHash(kAttrPtVarName, &m_pointVar, m_pointVar);
    element->GetHash(kAttrPos3C, &m_hash3C, m_hash3C);
    element->GetHash(kAttrPos40, &m_hash40, m_hash40);
    element->GetBool(kAttrPos44, &m_flag44, m_flag44);
    element->GetBool(kAttrPos45, &m_flag45, m_flag45);
    element->GetBool(kAttrPos46, &m_flag46, m_flag46);
}

// @func 0x000A5CE4 SetPositionCommand::Create
ApeAnimScriptCommand* SetPositionCommand::Create(XmlElement* element) const
{
    return new SetPositionCommand(element);
}

// TODO(0x000A5D54 SetPositionCommand::Execute) resolves the point (name or variable) and
// builds the sequence placement command (0x00439224).

// ---------------------------------------------------------------------------

void RegisterScriptCommandsAC()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xCF110608u, new SeqTimeLimitCommand());
    prototypes.Add(0xE61291FEu, new SeqSkippableCommand());
    prototypes.Add(AnimHash("SetEntity"), new SetEntityCommand());
    prototypes.Add(AnimHash("SetEntityUpdate"), new SetEntityUpdateCommand());
    prototypes.Add(AnimHash("SetEntityVisible"), new SetEntityVisibleCommand());
    prototypes.Add(0x7BF7E38Cu, new SeqEmitterParamCommand());
    prototypes.Add(0x2CB7712Fu, new SeqLetterboxCommand());
    prototypes.Add(AnimHash("SeqSetMusic"), new SeqSetMusicCommand());
    prototypes.Add(0xF6F2D4D6u, new SeqPauseAiCommand());
    prototypes.Add(0x9EAE7138u, new SeqEnablePartCommand());
    prototypes.Add(0x2C6A515Du, new SeqShowPartCommand());
    prototypes.Add(AnimHash("SetPosition"), new SetPositionCommand());
    prototypes.Add(0xAC54A838u, new SeqPausePlayerCommand());
}

} // namespace levels
} // namespace sly
