// openTIT - Game script commands, twenty-sixth block (sequence screen tint to the HUD element switch).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrStartColor = AnimHash("StartColor");
constexpr uint32_t kAttrEndColor = AnimHash("EndColor");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;    // NOTE: name unknown
constexpr uint32_t kAttrTint19 = 0x0AA56D8Bu;     // NOTE: name unknown
constexpr uint32_t kAttrSoundNameVar = AnimHash("SoundNameVar");
constexpr uint32_t kAttrSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrVolume = AnimHash("volume");
constexpr uint32_t kAttrSoundInst = AnimHash("SoundInst");
constexpr uint32_t kAttrSound24 = 0x9A0F95CAu;    // NOTE: name unknown
constexpr uint32_t kAttrSound29 = 0x6D18DE7Du;    // NOTE: name unknown
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrAttachType = AnimHash("AttachType");
constexpr uint32_t kAttrAttachTypePlayer = AnimHash("AttachTypePlayer");
constexpr uint32_t kAttrSubtitleType = 0x1050F752u;  // NOTE: name unknown
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrExclusivityGroupPriority = AnimHash("ExclusivityGroupPriority");
constexpr uint32_t kAttrSubtitle = AnimHash("subtitle");
constexpr uint32_t kAttrPlayerNum = AnimHash("PlayerNum");
constexpr uint32_t kAttrButton = AnimHash("Button");
constexpr uint32_t kAttrQte14 = 0x0F7D01C3u;      // NOTE: name unknown
constexpr uint32_t kAttrFailEvent = AnimHash("FailEvent");
constexpr uint32_t kAttrWaitTime = AnimHash("WaitTime");
constexpr uint32_t kAttrArc10 = 0x6D8F483Au;      // NOTE: name unknown
constexpr uint32_t kAttrArcAngle = AnimHash("ArcAngle");
constexpr uint32_t kAttrLookTime = AnimHash("LookTime");
constexpr uint32_t kAttrChase0C = 0x14929F25u;    // NOTE: names unknown for ParisChaseQTE
constexpr uint32_t kAttrChase10 = 0x2EC28132u;
constexpr uint32_t kAttrChase11 = 0xBCDD3C19u;
constexpr uint32_t kAttrEnabled = AnimHash("enabled");
constexpr uint32_t kAttrMinigame0C = 0xF19B0C5Au; // NOTE: names unknown for the minigame switches
constexpr uint32_t kAttrMinigame0D = 0xFA4A62D8u;
constexpr uint32_t kAttrMinigame0E = 0xCE13953Du;
constexpr uint32_t kAttrMinigame0F = 0x00814A44u;
constexpr uint32_t kAttrMinigame10 = 0xFE2AAC3Cu;
constexpr uint32_t kAttrMinigame11 = 0x6C06B3AFu;
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrForce = AnimHash("force");
constexpr uint32_t kAttrLeft = AnimHash("left");
constexpr uint32_t kAttrRight = AnimHash("Right");
constexpr uint32_t kAttrHud1F = 0x822AA3ABu;      // NOTE: name unknown

constexpr ape::xml::Color8 kTransparentBlack = {0, 0, 0, 0};
constexpr ape::xml::Color8 kOpaqueBlack = {0, 0, 0, 0xFF};

// "PlayerNum" of the sequence QTE commands: -1 (missing) means player 0.
int32_t ReadPlayerNum(XmlElement* element, uint32_t attr)
{
    int32_t value = -1;
    element->GetInt(attr, &value, -1);
    return value == -1 ? 0 : value;
}
} // namespace

// ---------------------------------------------------------------------------
// Hash 0x1FE37F1C (name unknown): sequence screen colour ramp.

class SeqScreenTintCommand : public ApeAnimScriptCommand {
public:
    SeqScreenTintCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::xml::Color8 m_startColor = kTransparentBlack;  // +0x0C "StartColor"
    ape::xml::Color8 m_endColor = kOpaqueBlack;         // +0x10 "EndColor"
    float m_duration = 1.0f;                            // +0x14 "duration"
    bool m_wait = true;                                 // +0x18 attribute 0x8488565B
    bool m_flag19 = false;                              // +0x19 attribute 0x0AA56D8B
};

// @func 0x000A220C SeqScreenTintCommand::SeqScreenTintCommand
SeqScreenTintCommand::SeqScreenTintCommand() = default;

// @func 0x000A2284 SeqScreenTintCommand::Create
ApeAnimScriptCommand* SeqScreenTintCommand::Create(XmlElement* element) const
{
    auto* command = new SeqScreenTintCommand();
    element->GetColor(kAttrStartColor, &command->m_startColor, &command->m_startColor);
    element->GetColor(kAttrEndColor, &command->m_endColor, &command->m_endColor);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    element->GetBool(kAttrTint19, &command->m_flag19, command->m_flag19);
    return command;
}

// @func 0x000A23F0 SeqScreenTintCommand::Execute
void SeqScreenTintCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    SequenceAddScreenTint(m_duration, m_startColor, m_endColor, instance, m_flag19, m_wait);
}

// ---------------------------------------------------------------------------
// "SeqPlaySound"

class SeqPlaySoundCommand : public ApeAnimScriptCommand {
public:
    SeqPlaySoundCommand();
    explicit SeqPlaySoundCommand(XmlElement* element);
    ~SeqPlaySoundCommand() override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    void RefreshSound();

    uint32_t m_soundNameVar = 0;          // +0x0C "SoundNameVar"
    uint32_t m_soundName = 0;             // +0x10 "SoundName"
    uint32_t m_bank = 0;                  // +0x14 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x18
    float m_volume = 1.0f;                // +0x1C "volume"
    uint32_t m_soundInst = 0;             // +0x20 "SoundInst"
    uint32_t m_hash24 = 0;                // +0x24 attribute 0x9A0F95CA
    bool m_wait = true;                   // +0x28 attribute 0x8488565B
    bool m_flag29 = false;                // +0x29 attribute 0x6D18DE7D
    uint32_t m_entity = 0;                // +0x2C "Entity"
    uint32_t m_attachType = 0;            // +0x30 "AttachType"
    uint32_t m_attachTypePlayer = 0;      // +0x34 "AttachTypePlayer" (defaults to AttachType)
    char m_subtitle[0x40] = {};           // +0x38 "subtitle"
    uint32_t m_subtitleType = 0;          // +0x7C attribute 0x1050F752
    uint32_t m_exclusivityGroup = 0;      // +0x80 "ExclusivityGroup"
    int32_t m_exclusivityPriority = 0;    // +0x84 "ExclusivityGroupPriority"
};

// @func 0x000A2480 SeqPlaySoundCommand::SeqPlaySoundCommand
SeqPlaySoundCommand::SeqPlaySoundCommand() = default;

void SeqPlaySoundCommand::RefreshSound()
{
    if (m_soundName != 0 && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_soundName, m_bank);
    }
}

// @func 0x000A2500 SeqPlaySoundCommand::Link
void SeqPlaySoundCommand::Link()
{
    RefreshSound();
}

// @func 0x000A2568 SeqPlaySoundCommand::SeqPlaySoundCommand
SeqPlaySoundCommand::SeqPlaySoundCommand(XmlElement* element)
{
    element->GetHash(kAttrSoundNameVar, &m_soundNameVar, 0);
    element->GetHash(kAttrSoundName, &m_soundName, 0);
    element->GetHash(kAttrBank, &m_bank, 0);
    element->GetFloat(kAttrVolume, &m_volume, m_volume);
    element->GetHash(kAttrSoundInst, &m_soundInst, m_soundInst);
    element->GetHash(kAttrSound24, &m_hash24, m_hash24);
    element->GetBool(kAttrSeqWait, &m_wait, m_wait);
    element->GetBool(kAttrSound29, &m_flag29, m_flag29);
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrAttachType, &m_attachType, 0);
    element->GetHash(kAttrAttachTypePlayer, &m_attachTypePlayer, m_attachType);
    element->GetHash(kAttrSubtitleType, &m_subtitleType, m_subtitleType);
    element->GetHash(kAttrExclusivityGroup, &m_exclusivityGroup, m_exclusivityGroup);
    element->GetInt(kAttrExclusivityGroupPriority, &m_exclusivityPriority, m_exclusivityPriority);
    const char* subtitle = element->GetString(kAttrSubtitle);
    if (subtitle && *subtitle) {
        std::strncpy(m_subtitle, subtitle, sizeof(m_subtitle) - 1);
        m_subtitle[sizeof(m_subtitle) - 1] = '\0';
    }
    RefreshSound();
}

// The destructors live outside this module (0x00741908 / 0x00741954).
SeqPlaySoundCommand::~SeqPlaySoundCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x000A2850 SeqPlaySoundCommand::Create
ApeAnimScriptCommand* SeqPlaySoundCommand::Create(XmlElement* element) const
{
    return new SeqPlaySoundCommand(element);
}

// TODO(0x000A28C8 SeqPlaySoundCommand::Execute) resolves the sound (variable or name) and
// builds the sequence sound command (0x004342DC).

// ---------------------------------------------------------------------------
// "SeqQTE" (NOTE: name from a hash match): waits for a button press.

class SeqQteCommand : public ApeAnimScriptCommand {
public:
    SeqQteCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;   // +0x0C "PlayerNum"
    uint32_t m_button = 0;     // +0x10 "Button"
    uint32_t m_hash14 = 0;     // +0x14 attribute 0x0F7D01C3
    uint32_t m_failEvent = 0;  // +0x18 "FailEvent"
    float m_waitTime = 0.0f;   // +0x1C "WaitTime"
};

// @func 0x000A2ADC SeqQteCommand::SeqQteCommand
SeqQteCommand::SeqQteCommand() = default;

// @func 0x000A2B34 SeqQteCommand::Create
ApeAnimScriptCommand* SeqQteCommand::Create(XmlElement* element) const
{
    auto* command = new SeqQteCommand();
    command->m_playerNum = ReadPlayerNum(element, kAttrPlayerNum);
    element->GetHash(kAttrButton, &command->m_button, command->m_button);
    element->GetHash(kAttrQte14, &command->m_hash14, command->m_hash14);
    element->GetHash(kAttrFailEvent, &command->m_failEvent, command->m_failEvent);
    element->GetFloat(kAttrWaitTime, &command->m_waitTime, command->m_waitTime);
    return command;
}

// @func 0x000A2C88 SeqQteCommand::Execute
void SeqQteCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddWaitForButton(m_waitTime, m_playerNum, m_button, m_hash14, m_failEvent);
}

// ---------------------------------------------------------------------------
// Hash 0x556D9F3B (name unknown): sequence transition prompt for a player.

class SeqTransitionPromptCommand : public ApeAnimScriptCommand {
public:
    SeqTransitionPromptCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;  // +0x0C "PlayerNum"
};

// @func 0x000A2D14 SeqTransitionPromptCommand::SeqTransitionPromptCommand
SeqTransitionPromptCommand::SeqTransitionPromptCommand() = default;

// @func 0x000A2D5C SeqTransitionPromptCommand::Create
ApeAnimScriptCommand* SeqTransitionPromptCommand::Create(XmlElement* element) const
{
    auto* command = new SeqTransitionPromptCommand();
    command->m_playerNum = ReadPlayerNum(element, kAttrPlayerNum);
    return command;
}

// @func 0x000A2E14 SeqTransitionPromptCommand::Execute
void SeqTransitionPromptCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddTransitionPrompt(m_playerNum);
}

// ---------------------------------------------------------------------------
// Hash 0xB34002B5 (name unknown): aiming arc QTE for a player.

class SeqArcQteCommand : public ApeAnimScriptCommand {
public:
    SeqArcQteCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;   // +0x0C "PlayerNum"
    float m_value10 = 0.0f;    // +0x10 attribute 0x6D8F483A
    float m_arcAngle = 0.0f;   // +0x14 "ArcAngle"
    float m_lookTime = 0.0f;   // +0x18 "LookTime"
};

// @func 0x000A2E88 SeqArcQteCommand::SeqArcQteCommand
SeqArcQteCommand::SeqArcQteCommand() = default;

// @func 0x000A2ED8 SeqArcQteCommand::Create
ApeAnimScriptCommand* SeqArcQteCommand::Create(XmlElement* element) const
{
    auto* command = new SeqArcQteCommand();
    command->m_playerNum = ReadPlayerNum(element, kAttrPlayerNum);
    element->GetFloat(kAttrArc10, &command->m_value10, command->m_value10);
    element->GetFloat(kAttrArcAngle, &command->m_arcAngle, command->m_arcAngle);
    element->GetFloat(kAttrLookTime, &command->m_lookTime, command->m_lookTime);
    return command;
}

// @func 0x000A300C SeqArcQteCommand::Execute
void SeqArcQteCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddArcQte(m_value10, m_arcAngle, m_lookTime, m_playerNum);
}

// TODO(0x000A3108 SeqActorQteCommand::SeqActorQteCommand) hash 0x58A0F34C (#238): QTE with
// a list of actor records; Execute 0x000A3434.

// ---------------------------------------------------------------------------
// "ParisChaseQTE" (NOTE: name from a hash match)

class ParisChaseQteCommand : public ApeAnimScriptCommand {
public:
    ParisChaseQteCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hash0C = 0;  // +0x0C attribute 0x14929F25
    bool m_flag10 = true;   // +0x10 attribute 0x2EC28132
    bool m_flag11 = true;   // +0x11 attribute 0xBCDD3C19
};

// @func 0x000A34C8 ParisChaseQteCommand::ParisChaseQteCommand
ParisChaseQteCommand::ParisChaseQteCommand() = default;

// @func 0x000A3518 ParisChaseQteCommand::Create
ApeAnimScriptCommand* ParisChaseQteCommand::Create(XmlElement* element) const
{
    auto* command = new ParisChaseQteCommand();
    element->GetHash(kAttrChase0C, &command->m_hash0C, 0);
    element->GetBool(kAttrChase10, &command->m_flag10, command->m_flag10);
    element->GetBool(kAttrChase11, &command->m_flag11, command->m_flag11);
    return command;
}

// @func 0x000A3608 ParisChaseQteCommand::Execute
void ParisChaseQteCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddParisChaseQte(m_hash0C, m_flag10, m_flag11);
}

// TODO(0x000A3688 SeqEntityStateCommand) hash 0xB39D2C46 (#240): entity state/clip with a
// target; parser 0x000A3700, Execute 0x000A38C4.

// ---------------------------------------------------------------------------
// Hash 0x5226D5B6 (name unknown): enables scripted entities in the sequence.

class SeqEnableScriptedEntitiesCommand : public ApeAnimScriptCommand {
public:
    SeqEnableScriptedEntitiesCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enabled = false;  // +0x0C "enabled"
};

// @func 0x000A39EC SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand
SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand() = default;

// @func 0x000A3A34 SeqEnableScriptedEntitiesCommand::Create
ApeAnimScriptCommand* SeqEnableScriptedEntitiesCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEnableScriptedEntitiesCommand();
    element->GetBool(kAttrEnabled, &command->m_enabled, false);
    return command;
}

// @func 0x000A3ADC SeqEnableScriptedEntitiesCommand::Execute
void SeqEnableScriptedEntitiesCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddEnableScriptedEntities(m_enabled);
}

// ---------------------------------------------------------------------------
// Hash 0xAD4079D1 (name unknown): minigame switches in the sequence.

class SeqMinigameControlCommand : public ApeAnimScriptCommand {
public:
    SeqMinigameControlCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_flags[6] = {};  // +0x0C..+0x11 attributes 0xF19B0C5A, 0xFA4A62D8, 0xCE13953D,
                           // 0x00814A44, 0xFE2AAC3C, 0x6C06B3AF
};

// @func 0x000A3B50 SeqMinigameControlCommand::SeqMinigameControlCommand
SeqMinigameControlCommand::SeqMinigameControlCommand() = default;

// @func 0x000A3BA8 SeqMinigameControlCommand::Create
ApeAnimScriptCommand* SeqMinigameControlCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMinigameControlCommand();
    const uint32_t attrs[6] = {kAttrMinigame0C, kAttrMinigame0D, kAttrMinigame0E,
                               kAttrMinigame0F, kAttrMinigame10, kAttrMinigame11};
    for (int i = 0; i < 6; ++i)
        element->GetBool(attrs[i], &command->m_flags[i], command->m_flags[i]);
    return command;
}

// @func 0x000A3D10 SeqMinigameControlCommand::Execute
void SeqMinigameControlCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddMinigameControl(m_flags);
}

// ---------------------------------------------------------------------------
// Hash 0xFD97E64D (name unknown): shows or hides a HUD element type in the
// sequence, with left/right labels.

class SeqHudTypeCommand : public ApeAnimScriptCommand {
public:
    SeqHudTypeCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_type = 0;     // +0x0C "Type"
    uint32_t m_left = 0;     // +0x10 "left"
    uint32_t m_right = 0;    // +0x14 "Right"
    bool m_visible = false;  // +0x1C "visible"
    bool m_force = false;    // +0x1D "force"
    bool m_wait = true;      // +0x1E attribute 0x8488565B
    bool m_flag1F = false;   // +0x1F attribute 0x822AA3AB
};

// @func 0x000A3D9C SeqHudTypeCommand::SeqHudTypeCommand
SeqHudTypeCommand::SeqHudTypeCommand() = default;

// @func 0x000A3E04 SeqHudTypeCommand::Create
ApeAnimScriptCommand* SeqHudTypeCommand::Create(XmlElement* element) const
{
    auto* command = new SeqHudTypeCommand();
    element->GetHash(kAttrType, &command->m_type, 0);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    element->GetBool(kAttrForce, &command->m_force, command->m_force);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    element->GetHash(kAttrLeft, &command->m_left, command->m_left);
    element->GetHash(kAttrRight, &command->m_right, command->m_right);
    element->GetBool(kAttrHud1F, &command->m_flag1F, command->m_flag1F);
    return command;
}

// @func 0x000A3FD8 SeqHudTypeCommand::Execute
void SeqHudTypeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddHudType(m_type, m_visible, m_force, m_left, m_right, m_wait, m_flag1F);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsAB()
{
    auto& prototypes = ape::CommandPrototypes();
    // #232 (0x9F8D96A1) is not ported yet.
    prototypes.Add(0x1FE37F1Cu, new SeqScreenTintCommand());
    prototypes.Add(AnimHash("SeqPlaySound"), new SeqPlaySoundCommand());
    prototypes.Add(AnimHash("SeqQTE"), new SeqQteCommand());
    prototypes.Add(0x556D9F3Bu, new SeqTransitionPromptCommand());
    prototypes.Add(0xB34002B5u, new SeqArcQteCommand());
    prototypes.Add(AnimHash("ParisChaseQTE"), new ParisChaseQteCommand());
    prototypes.Add(0x5226D5B6u, new SeqEnableScriptedEntitiesCommand());
    prototypes.Add(0xAD4079D1u, new SeqMinigameControlCommand());
    prototypes.Add(0xFD97E64Du, new SeqHudTypeCommand());
}

} // namespace levels
} // namespace sly
