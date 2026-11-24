// openTIT - Game script commands, twenty-sixth block (sequence screen tint to the HUD element switch).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"
#include "sly/gameplay/SequenceCommands.h"
#include "sly/gameplay/Sequencer.h"
#include "sly/gameplay/SequenceUiCommands.h"

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
// Hash 0x9F8D96A1 (#232, name unknown): shows up to four text pages (the
// element's own "name" and its <MOVIE name=...> children) in the sequence
// (sequence command 0x00433B2C).

namespace {
constexpr uint32_t kTagPageMovie = AnimHash("MOVIE");
constexpr uint32_t kAttrPagesCondition = 0x42E874AAu;   // NOTE: name unknown (int variable)
constexpr uint32_t kAttrPagesColorA = 0x3060AABFu;      // NOTE: name unknown
constexpr uint32_t kAttrPagesColorB = 0xBF1E5566u;      // NOTE: name unknown
constexpr uint32_t kAttrPagesBlocking = 0x8488565Bu;    // NOTE: name unknown
constexpr uint32_t kAttrPagesLetterbox = 0x9C6B7DABu;   // NOTE: name unknown
constexpr uint32_t kAttrPagesPrompt = AnimHash("PROMPT");
constexpr uint32_t kAttrPagesFade = AnimHash("fade");
constexpr uint32_t kAttrPagesFlagB = 0x4DFD4BFEu;       // NOTE: name unknown
constexpr uint32_t kAttrPagesFlag0 = 0x367CF240u;       // NOTE: name unknown
} // namespace

class SeqDialogPagesScriptCommand : public ApeAnimScriptCommand {
public:
    SeqDialogPagesScriptCommand();
    explicit SeqDialogPagesScriptCommand(XmlElement* element);
    ~SeqDialogPagesScriptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    static constexpr int kMaxPages = 4;
    void Reset();
    void AddPage(const char* text);

    char m_pages[kMaxPages][0x80];   // +0x0C
    int32_t m_pageCount;             // +0x20C
    float m_scale[2];                // +0x210 "Scale"
    float m_offset[2];               // +0x218 "Offset"
    uint32_t m_condition;            // +0x220 int variable
    uint8_t m_colorA[4];             // +0x224
    uint8_t m_colorB[4];             // +0x228
    bool m_blocking;                 // +0x22C
    bool m_letterbox;                // +0x22D
    bool m_letterboxGiven;           // +0x22E the letterbox attribute was present
    bool m_prompt;                   // +0x22F "PROMPT"
    bool m_fade;                     // +0x230 "fade"
    bool m_flagB;                    // +0x231
    bool m_flag0;                    // +0x232
};

// @func 0x000A1C28 SeqDialogPagesScriptCommand::Reset
void SeqDialogPagesScriptCommand::Reset()
{
    m_pageCount = 0;
    m_scale[0] = m_scale[1] = 1.0f;     // console global 0x01164AF4
    m_offset[0] = m_offset[1] = 0.0f;   // console global 0x01164AFC
    m_condition = 0;
    m_colorA[0] = m_colorA[1] = m_colorA[2] = 0;
    m_colorA[3] = 0xFF;
    m_colorB[0] = m_colorB[1] = m_colorB[2] = m_colorB[3] = 0;
    m_blocking = true;
    m_letterbox = false;
    m_letterboxGiven = false;
    m_prompt = true;
    m_fade = true;
    m_flagB = true;
    m_flag0 = true;
    for (char* page : m_pages)
        page[0] = '\0';
}

void SeqDialogPagesScriptCommand::AddPage(const char* text)
{
    // NOTE: the console does not bound the page count (four pages fit).
    if (text && text[0] != '\0' && m_pageCount < kMaxPages)
        ape::StrCopyBounded(m_pages[m_pageCount++], text, 0x7F);
}

// @func 0x000A1CEC SeqDialogPagesScriptCommand::SeqDialogPagesScriptCommand
SeqDialogPagesScriptCommand::SeqDialogPagesScriptCommand() { Reset(); }

// @func 0x000A1D34 SeqDialogPagesScriptCommand::SeqDialogPagesScriptCommand (parsing)
SeqDialogPagesScriptCommand::SeqDialogPagesScriptCommand(XmlElement* element)
{
    Reset();
    AddPage(element->GetString("name"));
    for (XmlElement* child = element->FirstChildElement(kTagPageMovie); child;
         child = child->NextSiblingElement(kTagPageMovie))
        AddPage(child->GetString("name"));
    element->GetVec2(AnimHash("Scale"), m_scale, m_scale);
    element->GetVec2(AnimHash("Offset"), m_offset, m_offset);
    element->GetHash(kAttrPagesCondition, &m_condition, m_condition);
    element->GetColor(kAttrPagesColorA, reinterpret_cast<ape::xml::Color8*>(m_colorA),
                      reinterpret_cast<const ape::xml::Color8*>(m_colorA));
    element->GetColor(kAttrPagesColorB, reinterpret_cast<ape::xml::Color8*>(m_colorB),
                      reinterpret_cast<const ape::xml::Color8*>(m_colorB));
    element->GetBool(kAttrPagesBlocking, &m_blocking, m_blocking);
    m_letterboxGiven =
        element->GetBool(kAttrPagesLetterbox, &m_letterbox, m_letterbox) == ape::xml::kQuery_Ok;
    element->GetBool(kAttrPagesPrompt, &m_prompt, m_prompt);
    element->GetBool(kAttrPagesFade, &m_fade, m_fade);
    element->GetBool(kAttrPagesFlagB, &m_flagB, m_flagB);
    element->GetBool(kAttrPagesFlag0, &m_flag0, m_flag0);
}

// NOTE: the destructors live at 0x0074xxxx (outside this range).
SeqDialogPagesScriptCommand::~SeqDialogPagesScriptCommand() = default;

// @func 0x000A2028 SeqDialogPagesScriptCommand::Create
ApeAnimScriptCommand* SeqDialogPagesScriptCommand::Create(XmlElement* element) const
{
    return new SeqDialogPagesScriptCommand(element);
}

// @func 0x000A2098 SeqDialogPagesScriptCommand::Execute
void SeqDialogPagesScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ApeAnimScriptVar* condition = nullptr;
    if (m_condition != 0) {
        ApeAnimScriptVar* var = instance->FindVar(m_condition);
        if (var && var->GetType() == ape::AnimScriptVarType::Int)
            condition = var;
    }
    gameplay::AddSequenceCommand(new gameplay::SeqDialogPagesCommand(
        m_pages, m_pageCount, m_scale, m_offset, condition, m_blocking, m_letterbox, m_colorA, m_colorB,
        m_letterboxGiven, m_prompt, m_fade, m_flagB, m_flag0));
}

// ---------------------------------------------------------------------------
// Hash 0x1FE37F1C (name unknown): sequence screen colour ramp.

class SeqScreenTintCommand : public ApeAnimScriptCommand {
public:
    SeqScreenTintCommand();
    ~SeqScreenTintCommand() override;
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

// @func 0x00741880 SeqScreenTintCommand::~SeqScreenTintCommand
// @func 0x007418B0 SeqScreenTintCommand::~SeqScreenTintCommand (deleting)
SeqScreenTintCommand::~SeqScreenTintCommand() = default;

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

// @func 0x00741908 SeqPlaySoundCommand::~SeqPlaySoundCommand
// @func 0x00741954 SeqPlaySoundCommand::~SeqPlaySoundCommand (deleting)
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
    ~SeqQteCommand() override;
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

// @func 0x0073FF68 SeqQteCommand::~SeqQteCommand
// @func 0x007419BC SeqQteCommand::~SeqQteCommand (deleting)
SeqQteCommand::~SeqQteCommand() = default;

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
    ~SeqTransitionPromptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;  // +0x0C "PlayerNum"
};

// @func 0x000A2D14 SeqTransitionPromptCommand::SeqTransitionPromptCommand
SeqTransitionPromptCommand::SeqTransitionPromptCommand() = default;

// @func 0x0073FF98 SeqTransitionPromptCommand::~SeqTransitionPromptCommand
// @func 0x00741A14 SeqTransitionPromptCommand::~SeqTransitionPromptCommand (deleting)
SeqTransitionPromptCommand::~SeqTransitionPromptCommand() = default;

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
    ~SeqArcQteCommand() override;
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

// @func 0x0073FFC8 SeqArcQteCommand::~SeqArcQteCommand
// @func 0x00741A6C SeqArcQteCommand::~SeqArcQteCommand (deleting)
SeqArcQteCommand::~SeqArcQteCommand() = default;

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

// ---------------------------------------------------------------------------
// Hash 0x58A0F34C (#238, name unknown): button QTE over a list of actors in the
// sequence. NOTE: the console parses the <0x286A440C> actor records but the
// sequence command it builds (0x0043554C) only receives the scalar fields.

namespace {
constexpr uint32_t kTagQteActor = 0x286A440Cu;         // NOTE: element name unknown
constexpr uint32_t kAttrQteActor08 = 0x3EE71C7Au;      // NOTE: name unknown
constexpr uint32_t kAttrQteActor0C = 0x31F8E7E5u;      // NOTE: name unknown
constexpr uint32_t kAttrQtePlayer = 0x368328E5u;       // NOTE: name unknown (-1 = 0)
constexpr uint32_t kAttrQteButton = AnimHash("Button");
constexpr uint32_t kAttrQte28 = 0xBD79437Du;           // NOTE: name unknown
constexpr uint32_t kAttrQte2C = 0x3EE9A5E8u;           // NOTE: name unknown
constexpr uint32_t kAttrQte30 = 0x3EEC218Cu;           // NOTE: name unknown
constexpr uint32_t kAttrQte34 = 0x32261AEBu;           // NOTE: name unknown
} // namespace

class SeqActorQteCommand : public ApeAnimScriptCommand {
public:
    SeqActorQteCommand();
    explicit SeqActorQteCommand(XmlElement* element);
    ~SeqActorQteCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    struct Actor {
        uint32_t field00 = 0;    // +0x00 NOTE: never set
        uint32_t entity = 0;     // +0x04 "Entity"
        uint32_t hash08 = 0;     // +0x08
        uint32_t hash0C = 0;     // +0x0C
    };
    std::vector<Actor> m_actors;     // +0x0C (console ApeArray, capacity 16)
    int32_t m_player = 0;            // +0x20
    uint32_t m_button = 0;           // +0x24 "Button"
    float m_value28 = 1.0f;          // +0x28
    float m_value2C = 0.0f;          // +0x2C
    float m_value30 = 0.0f;          // +0x30
    float m_value34 = -1.0f;         // +0x34
};

// @func 0x000A3090 SeqActorQteCommand::SeqActorQteCommand
SeqActorQteCommand::SeqActorQteCommand() = default;

// @func 0x000A3108 SeqActorQteCommand::SeqActorQteCommand (parsing)
SeqActorQteCommand::SeqActorQteCommand(XmlElement* element)
{
    m_actors.reserve(16);
    for (XmlElement* child = element->FirstChildElement(kTagQteActor); child;
         child = child->NextSiblingElement(kTagQteActor)) {
        Actor actor;
        child->GetHash(kAttrEntity, &actor.entity, 0);
        child->GetHash(kAttrQteActor0C, &actor.hash0C, 0);
        child->GetHash(kAttrQteActor08, &actor.hash08, 0);
        m_actors.push_back(actor);
    }
    int32_t player = -1;
    element->GetInt(kAttrQtePlayer, &player, -1);
    m_player = player == -1 ? 0 : player;
    element->GetHash(kAttrQteButton, &m_button, m_button);
    element->GetFloat(kAttrQte28, &m_value28, m_value28);
    element->GetFloat(kAttrQte2C, &m_value2C, m_value2C);
    element->GetFloat(kAttrQte30, &m_value30, m_value30);
    element->GetFloat(kAttrQte34, &m_value34, m_value34);
}

// NOTE: the destructors live at 0x0074xxxx (outside this range).
SeqActorQteCommand::~SeqActorQteCommand() = default;

// @func 0x000A33C4 SeqActorQteCommand::Create
ApeAnimScriptCommand* SeqActorQteCommand::Create(XmlElement* element) const
{
    return new SeqActorQteCommand(element);
}

// @func 0x000A3434 SeqActorQteCommand::Execute
void SeqActorQteCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddActorQte(m_player, m_button, m_value28, m_value2C, m_value30, m_value34);
}

// ---------------------------------------------------------------------------
// "ParisChaseQTE" (NOTE: name from a hash match)

class ParisChaseQteCommand : public ApeAnimScriptCommand {
public:
    ParisChaseQteCommand();
    ~ParisChaseQteCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hash0C = 0;  // +0x0C attribute 0x14929F25
    bool m_flag10 = true;   // +0x10 attribute 0x2EC28132
    bool m_flag11 = true;   // +0x11 attribute 0xBCDD3C19
};

// @func 0x000A34C8 ParisChaseQteCommand::ParisChaseQteCommand
ParisChaseQteCommand::ParisChaseQteCommand() = default;

// @func 0x00740060 ParisChaseQteCommand::~ParisChaseQteCommand
// @func 0x00741B44 ParisChaseQteCommand::~ParisChaseQteCommand (deleting)
ParisChaseQteCommand::~ParisChaseQteCommand() = default;

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

// ---------------------------------------------------------------------------
// Hash 0xB39D2C46 (#240, name unknown): moves an entity to a state/clip
// towards a target in the sequence (sequence command 0x00437A1C).

namespace {
constexpr uint32_t kAttrEntityStateBlocking = 0x8488565Bu;   // NOTE: name unknown
constexpr uint32_t kAttrEntityStateQuiet = 0x80116C17u;      // NOTE: name unknown
} // namespace

class SeqEntityStateCommand : public ApeAnimScriptCommand {
public:
    SeqEntityStateCommand();
    ~SeqEntityStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;      // +0x0C "Entity"
    uint32_t m_state = 0;       // +0x10 "state"
    uint32_t m_clip = 0;        // +0x14 "clip"
    uint32_t m_target = 0;      // +0x18 "Target"
    uint32_t m_targetVar = 0;   // +0x1C "TargetVar" (int variable; non-zero overrides Target)
    float m_value = 0.0f;       // +0x20 "Value"
    bool m_blocking = false;    // +0x24
    bool m_quiet = false;       // +0x25
};

// @func 0x000A3688 SeqEntityStateCommand::SeqEntityStateCommand
SeqEntityStateCommand::SeqEntityStateCommand() = default;

// NOTE: the destructors live at 0x0074xxxx (outside this range).
SeqEntityStateCommand::~SeqEntityStateCommand() = default;

// @func 0x000A3700 SeqEntityStateCommand::Create
ApeAnimScriptCommand* SeqEntityStateCommand::Create(XmlElement* element) const
{
    auto* command = new SeqEntityStateCommand();
    element->GetBool(kAttrEntityStateBlocking, &command->m_blocking, false);
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(AnimHash("state"), &command->m_state, 0);
    element->GetHash(AnimHash("clip"), &command->m_clip, 0);
    element->GetHash(AnimHash("Target"), &command->m_target, 0);
    element->GetHash(AnimHash("TargetVar"), &command->m_targetVar, 0);
    element->GetFloat(AnimHash("Value"), &command->m_value, 0.0f);
    element->GetBool(kAttrEntityStateQuiet, &command->m_quiet, command->m_quiet);
    return command;
}

// @func 0x000A38C4 SeqEntityStateCommand::Execute
void SeqEntityStateCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    uint32_t target = m_target;
    if (m_targetVar != 0) {
        ApeAnimScriptVar* var = instance->FindVar(m_targetVar);
        if (var && var->GetType() == ape::AnimScriptVarType::Int && var->GetInt() != 0)
            target = uint32_t(var->GetInt());
    }
    gameplay::AddSequenceCommand(
        new gameplay::SeqMoveEntityCommand(m_value, m_entity, m_state, m_clip, target, m_quiet, m_blocking));
}

// ---------------------------------------------------------------------------
// Hash 0x5226D5B6 (name unknown): enables scripted entities in the sequence.

class SeqEnableScriptedEntitiesCommand : public ApeAnimScriptCommand {
public:
    SeqEnableScriptedEntitiesCommand();
    ~SeqEnableScriptedEntitiesCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enabled = false;  // +0x0C "enabled"
};

// @func 0x000A39EC SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand
SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand() = default;

// @func 0x00741C24 SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand
// @func 0x00741C54 SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand (deleting)
SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand() = default;

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
    ~SeqMinigameControlCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_flags[6] = {};  // +0x0C..+0x11 attributes 0xF19B0C5A, 0xFA4A62D8, 0xCE13953D,
                           // 0x00814A44, 0xFE2AAC3C, 0x6C06B3AF
};

// @func 0x000A3B50 SeqMinigameControlCommand::SeqMinigameControlCommand
SeqMinigameControlCommand::SeqMinigameControlCommand() = default;

// @func 0x00741CAC SeqMinigameControlCommand::~SeqMinigameControlCommand
// @func 0x00741CDC SeqMinigameControlCommand::~SeqMinigameControlCommand (deleting)
SeqMinigameControlCommand::~SeqMinigameControlCommand() = default;

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
    ~SeqHudTypeCommand() override;
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

// @func 0x00741D34 SeqHudTypeCommand::~SeqHudTypeCommand
// @func 0x00741D64 SeqHudTypeCommand::~SeqHudTypeCommand (deleting)
SeqHudTypeCommand::~SeqHudTypeCommand() = default;

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
    prototypes.Add(0x9F8D96A1u, new SeqDialogPagesScriptCommand());
    prototypes.Add(0x1FE37F1Cu, new SeqScreenTintCommand());
    prototypes.Add(AnimHash("SeqPlaySound"), new SeqPlaySoundCommand());
    prototypes.Add(AnimHash("SeqQTE"), new SeqQteCommand());
    prototypes.Add(0x556D9F3Bu, new SeqTransitionPromptCommand());
    prototypes.Add(0xB34002B5u, new SeqArcQteCommand());
    prototypes.Add(0x58A0F34Cu, new SeqActorQteCommand());
    prototypes.Add(AnimHash("ParisChaseQTE"), new ParisChaseQteCommand());
    prototypes.Add(0xB39D2C46u, new SeqEntityStateCommand());
    prototypes.Add(0x5226D5B6u, new SeqEnableScriptedEntitiesCommand());
    prototypes.Add(0xAD4079D1u, new SeqMinigameControlCommand());
    prototypes.Add(0xFD97E64Du, new SeqHudTypeCommand());
}

} // namespace levels
} // namespace sly
