// openTIT - Game script commands, twenty-fifth block (Pause to the sequence message box).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"

#include "sly/game/AudioSystem.h"
#include "sly/game/Upgrades.h"
#include "sly/levels/GameServices.h"
#include "sly/gameplay/SequenceCommands.h"
#include "sly/gameplay/Sequencer.h"
#include "sly/gameplay/SequenceCommands2.h"
#include "sly/gameplay/SequenceUiCommands.h"
#include "sly/levels/script/ScriptVars.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrTime = AnimHash("Time");
constexpr uint32_t kAttrTimeVar = AnimHash("TimeVar");
constexpr uint32_t kAttrPause = AnimHash("pause");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrAnimation = AnimHash("animation");
constexpr uint32_t kAttrAnimationVar = AnimHash("AnimationVar");
constexpr uint32_t kAttrLayer = AnimHash("layer");
constexpr uint32_t kAttrSpeed = AnimHash("Speed");
constexpr uint32_t kAttrBlend = AnimHash("Blend");
constexpr uint32_t kAttrSkeletonMap = AnimHash("SkeletonMap");
constexpr uint32_t kAttrAnim2C = 0x941382D8u;   // NOTE: names unknown for the next attributes
constexpr uint32_t kAttrAnim30 = 0xC1B28F57u;
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;
constexpr uint32_t kAttrAnim32 = 0x81D8B678u;
constexpr uint32_t kAttrAnim33 = 0x0CA4259Bu;
constexpr uint32_t kAttrAnimMode = 0x96997C96u;  // NOTE: name unknown; value "Loop" loops
constexpr uint32_t kValueLoop = AnimHash("Loop");
constexpr uint32_t kAttrSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrScript = 0x849B0778u;    // NOTE: name unknown (script name)
constexpr uint32_t kAttrScript24 = 0x74EB708Fu;  // NOTE: name unknown
constexpr uint32_t kAttrScript28 = 0x74EB4BEDu;  // NOTE: name unknown
constexpr uint32_t kAttrIgnoreWarnings = AnimHash("IgnoreWarnings");
constexpr uint32_t kAttrScale = AnimHash("Scale");
constexpr uint32_t kAttrOffset = AnimHash("Offset");

// Text capacity of the sequence script command (fixed on the console).
constexpr size_t kSeqScriptTextCapacity = 0x10;
} // namespace

// ---------------------------------------------------------------------------
// "Pause" (NOTE: name from a hash match): sequence wait.

class SeqPauseCommand : public ApeAnimScriptCommand {
public:
    SeqPauseCommand();
    ~SeqPauseCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_time = 0.0f;     // +0x0C "Time"
    uint32_t m_timeVar = 0;  // +0x10 "TimeVar"
};

// @func 0x0009F750 SeqPauseCommand::SeqPauseCommand
SeqPauseCommand::SeqPauseCommand() = default;

// @func 0x00741134 SeqPauseCommand::~SeqPauseCommand
// @func 0x00741164 SeqPauseCommand::~SeqPauseCommand (deleting)
SeqPauseCommand::~SeqPauseCommand() = default;

// @func 0x0009F7B0 SeqPauseCommand::Create
ApeAnimScriptCommand* SeqPauseCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPauseCommand();
    element->GetFloat(kAttrTime, &command->m_time, 0.0f);
    element->GetHash(kAttrTimeVar, &command->m_timeVar, command->m_timeVar);
    return command;
}

// @func 0x0009F888 SeqPauseCommand::Execute
// Appends a wait of "Time" seconds (or the float variable "TimeVar").
void SeqPauseCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    const float time = ResolveFloat(instance, m_timeVar, m_time);
    if (time > 0.0f)
        gameplay::AddSequenceCommand(new gameplay::SeqWaitCommand(time));
}

// ---------------------------------------------------------------------------
// The sequence switches that take a single "pause" flag. Each builds a
// different sequence command.

// Hash 0xEC00CC0F (name unknown): sequence HUD counter switch.
class SeqHudCounterCommand : public ApeAnimScriptCommand {
public:
    SeqHudCounterCommand();
    ~SeqHudCounterCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_pause = true;  // +0x0C "pause"
};

// "PauseEntities" (NOTE: name from a hash match)
class SeqPauseEntitiesCommand : public ApeAnimScriptCommand {
public:
    SeqPauseEntitiesCommand();
    ~SeqPauseEntitiesCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_pause = true;  // +0x0C "pause"
};

// Hash 0x2D43CE4C (name unknown)
class SeqShowHudCommand : public ApeAnimScriptCommand {
public:
    SeqShowHudCommand();
    ~SeqShowHudCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_pause = true;  // +0x0C "pause"
};

// "PauseHavok" (NOTE: name from a hash match)
class SeqPauseHavokCommand : public ApeAnimScriptCommand {
public:
    SeqPauseHavokCommand();
    ~SeqPauseHavokCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_pause = true;  // +0x0C "pause"
};

// @func 0x0009FDB0 SeqHudCounterCommand::SeqHudCounterCommand
SeqHudCounterCommand::SeqHudCounterCommand() = default;

// @func 0x00741354 SeqHudCounterCommand::~SeqHudCounterCommand
// @func 0x00741384 SeqHudCounterCommand::~SeqHudCounterCommand (deleting)
SeqHudCounterCommand::~SeqHudCounterCommand() = default;

// @func 0x0009FDF0 SeqHudCounterCommand::Create
ApeAnimScriptCommand* SeqHudCounterCommand::Create(XmlElement* element) const
{
    auto* command = new SeqHudCounterCommand();
    element->GetBool(kAttrPause, &command->m_pause, true);
    return command;
}

// @func 0x0009FE98 SeqHudCounterCommand::Execute
void SeqHudCounterCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddHudCounter(m_pause);
}

// @func 0x0009F994 SeqPauseEntitiesCommand::SeqPauseEntitiesCommand
SeqPauseEntitiesCommand::SeqPauseEntitiesCommand() = default;

// @func 0x007411BC SeqPauseEntitiesCommand::~SeqPauseEntitiesCommand
// @func 0x007411EC SeqPauseEntitiesCommand::~SeqPauseEntitiesCommand (deleting)
SeqPauseEntitiesCommand::~SeqPauseEntitiesCommand() = default;

// @func 0x0009F9D4 SeqPauseEntitiesCommand::Create
ApeAnimScriptCommand* SeqPauseEntitiesCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPauseEntitiesCommand();
    element->GetBool(kAttrPause, &command->m_pause, true);
    return command;
}

// @func 0x0009FA7C SeqPauseEntitiesCommand::Execute
void SeqPauseEntitiesCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPauseEntities(m_pause);
}

// @func 0x0009FAF0 SeqShowHudCommand::SeqShowHudCommand
SeqShowHudCommand::SeqShowHudCommand() = default;

// @func 0x00741244 SeqShowHudCommand::~SeqShowHudCommand
// @func 0x00741274 SeqShowHudCommand::~SeqShowHudCommand (deleting)
SeqShowHudCommand::~SeqShowHudCommand() = default;

// @func 0x0009FB38 SeqShowHudCommand::Create
ApeAnimScriptCommand* SeqShowHudCommand::Create(XmlElement* element) const
{
    auto* command = new SeqShowHudCommand();
    element->GetBool(kAttrPause, &command->m_pause, true);
    return command;
}

// @func 0x0009FBE0 SeqShowHudCommand::Execute
void SeqShowHudCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddShowHud(m_pause);
}

// @func 0x0009FC54 SeqPauseHavokCommand::SeqPauseHavokCommand
SeqPauseHavokCommand::SeqPauseHavokCommand() = default;

// @func 0x007412CC SeqPauseHavokCommand::~SeqPauseHavokCommand
// @func 0x007412FC SeqPauseHavokCommand::~SeqPauseHavokCommand (deleting)
SeqPauseHavokCommand::~SeqPauseHavokCommand() = default;

// @func 0x0009FC94 SeqPauseHavokCommand::Create
ApeAnimScriptCommand* SeqPauseHavokCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPauseHavokCommand();
    element->GetBool(kAttrPause, &command->m_pause, true);
    return command;
}

// @func 0x0009FD3C SeqPauseHavokCommand::Execute
void SeqPauseHavokCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPauseHavok(m_pause);
}

// ---------------------------------------------------------------------------
// "SeqPlayAnim" (NOTE: name from a hash match)

class SeqPlayAnimCommand : public ApeAnimScriptCommand {
public:
    SeqPlayAnimCommand();
    explicit SeqPlayAnimCommand(XmlElement* element);
    ~SeqPlayAnimCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;        // +0x0C "Entity"
    uint32_t m_animation = 0;     // +0x10 "animation"
    uint32_t m_animationVar = 0;  // +0x14 "AnimationVar"
    bool m_loop = false;          // +0x18 attribute 0x96997C96 == "Loop"
    uint32_t m_layer = 0;         // +0x1C "layer"
    float m_speed = 1.0f;         // +0x20 "Speed"
    float m_blend = 0.0f;         // +0x24 "Blend"
    uint32_t m_skeletonMap = 0;   // +0x28 "SkeletonMap"
    uint32_t m_hash2C = 0;        // +0x2C attribute 0x941382D8
    bool m_flag30 = true;         // +0x30 attribute 0xC1B28F57
    bool m_wait = true;           // +0x31 attribute 0x8488565B
    bool m_flag32 = true;         // +0x32 attribute 0x81D8B678
    bool m_flag33 = false;        // +0x33 attribute 0x0CA4259B
};

// @func 0x0009FF10 SeqPlayAnimCommand::SeqPlayAnimCommand
SeqPlayAnimCommand::SeqPlayAnimCommand() = default;

// @func 0x007413DC SeqPlayAnimCommand::~SeqPlayAnimCommand
// @func 0x0074140C SeqPlayAnimCommand::~SeqPlayAnimCommand (deleting)
SeqPlayAnimCommand::~SeqPlayAnimCommand() = default;

// @func 0x0009FF90 SeqPlayAnimCommand::SeqPlayAnimCommand
SeqPlayAnimCommand::SeqPlayAnimCommand(XmlElement* element)
{
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrAnimation, &m_animation, m_animation);
    element->GetHash(kAttrAnimationVar, &m_animationVar, m_animationVar);
    element->GetHash(kAttrLayer, &m_layer, m_layer);
    element->GetFloat(kAttrSpeed, &m_speed, m_speed);
    element->GetFloat(kAttrBlend, &m_blend, m_blend);
    element->GetHash(kAttrSkeletonMap, &m_skeletonMap, m_skeletonMap);
    element->GetHash(kAttrAnim2C, &m_hash2C, m_hash2C);
    element->GetBool(kAttrAnim30, &m_flag30, m_flag30);
    element->GetBool(kAttrSeqWait, &m_wait, m_wait);
    element->GetBool(kAttrAnim32, &m_flag32, m_flag32);
    element->GetBool(kAttrAnim33, &m_flag33, m_flag33);
    uint32_t mode = 0;
    if (element->GetHash(kAttrAnimMode, &mode, 0) == ape::xml::kQuery_Ok)
        m_loop = (mode == kValueLoop);
}

// @func 0x000A0200 SeqPlayAnimCommand::Create
ApeAnimScriptCommand* SeqPlayAnimCommand::Create(XmlElement* element) const
{
    return new SeqPlayAnimCommand(element);
}

// TODO(0x000A0270 SeqPlayAnimCommand::Execute) resolves the animation variable and builds
// the sequence animation command (0x004321A4).

// ---------------------------------------------------------------------------
// Hash 0x780316BB (name unknown): sequence subtitle/dialogue line with an
// optional button prompt.

class SeqDialogueCommand : public ApeAnimScriptCommand {
public:
    SeqDialogueCommand();
    explicit SeqDialogueCommand(XmlElement* element);
    ~SeqDialogueCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_subtitle[0x44] = {};           // +0x0C "subtitle"
    uint32_t m_hash50 = 0;                // +0x50 attribute 0x1050F752. NOTE: name unknown
    uint32_t m_soundName = 0;             // +0x54 "SoundName"
    uint32_t m_bank = 0;                  // +0x58 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x5C
    float m_time = 0.0f;                  // +0x60 attribute 0xC05CEEB3 (negative = until the button)
    bool m_blocking = true;               // +0x64
    uint32_t m_hash68 = 0;                // +0x68 attribute 0x9518F698. NOTE: name unknown
    uint32_t m_hash6C = 0;                // +0x6C attribute 0x4897260D. NOTE: name unknown
    float m_displayRate = -1.0f;          // +0x70 "DisplayRate"
    uint32_t m_buttonMask = 0;            // +0x74 button to continue (NOTE: unknown names leave it unset on the console)
    bool m_flag78 = false;                // +0x78 attribute 0xD06EE4FE. NOTE: name unknown
    bool m_flag79 = false;                // +0x79 attribute 0x91BAC895. NOTE: name unknown
    uint32_t m_hash7C = 0xB7E98FE4u;      // +0x7C attribute 0xE2F075ED (default: console global 0x00F64D08)
    char m_promptText[0x40] = {};         // +0x80 "promptText" or "<button> $STR_CONTINUE"
    bool m_flagC0 = false;                // +0xC0 attribute 0x6D18DE7D. NOTE: name unknown
    uint32_t m_exclusivityGroup = 0;      // +0xC4 "ExclusivityGroup"
    int32_t m_exclusivityPriority = 0;    // +0xC8 "ExclusivityGroupPriority"
    uint32_t m_entity = 0;                // +0xCC "Entity"
};

// @func 0x000A045C SeqDialogueCommand::SeqDialogueCommand
SeqDialogueCommand::SeqDialogueCommand() = default;

// @func 0x000A04E4 SeqDialogueCommand::Link
void SeqDialogueCommand::Link()
{
    if (m_soundName != 0 && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_soundName, m_bank);
    }
}

// @func 0x00741464 SeqDialogueCommand::~SeqDialogueCommand
// @func 0x007414B0 SeqDialogueCommand::~SeqDialogueCommand (deleting)
SeqDialogueCommand::~SeqDialogueCommand()
{
    audio::SoundNameRelease(m_sound);
}

namespace {
constexpr uint32_t kAttrDialogueSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrDialogueBank = AnimHash("Bank");
constexpr uint32_t kAttrDialogue50 = 0x1050F752u;        // NOTE: name unknown
constexpr uint32_t kAttrDialogueTime = 0xC05CEEB3u;      // NOTE: name unknown
constexpr uint32_t kAttrDialogueBlocking = 0x8488565Bu;  // NOTE: name unknown
constexpr uint32_t kAttrDialogue68 = 0x9518F698u;        // NOTE: name unknown
constexpr uint32_t kAttrDialogue6C = 0x4897260Du;        // NOTE: name unknown
constexpr uint32_t kAttrDialogue78 = 0xD06EE4FEu;        // NOTE: name unknown
constexpr uint32_t kAttrDialogue79 = 0x91BAC895u;        // NOTE: name unknown
constexpr uint32_t kAttrDialogueBoth = 0x23D89F8Bu;      // NOTE: name unknown (sets both flags)
constexpr uint32_t kAttrDialogueButton = 0x051086F8u;    // NOTE: name unknown
constexpr uint32_t kAttrDialogue7C = 0xE2F075EDu;        // NOTE: name unknown
constexpr uint32_t kAttrDialogueC0 = 0x6D18DE7Du;        // NOTE: name unknown

// Continue buttons by name: mask bit and prompt text.
struct DialogueButton {
    uint32_t nameHash;
    uint32_t mask;
    const char* text;
};
constexpr DialogueButton kDialogueButtons[] = {
    {AnimHash("UP"), 0x01, "$BUTTON_UP"},           {AnimHash("DOWN"), 0x02, "$BUTTON_DOWN"},
    {AnimHash("LEFT"), 0x04, "$BUTTON_LEFT"},       {AnimHash("RIGHT"), 0x08, "$BUTTON_RIGHT"},
    {AnimHash("TRIANGLE"), 0x10, "$BUTTON_TRIANGLE"}, {AnimHash("SQUARE"), 0x20, "$BUTTON_SQUARE"},
    {AnimHash("CIRCLE"), 0x40, "$BUTTON_CIRCLE"},   {AnimHash("X"), 0x80, "$BUTTON_X"},
    {AnimHash("CROSS"), 0x80, "$BUTTON_X"},
};
} // namespace

// @func 0x000A0558 SeqDialogueCommand::SeqDialogueCommand (parsing)
SeqDialogueCommand::SeqDialogueCommand(XmlElement* element)
{
    element->GetHash(kAttrDialogueSoundName, &m_soundName, 0);
    element->GetHash(kAttrDialogueBank, &m_bank, 0);
    element->GetHash(kAttrDialogue50, &m_hash50, m_hash50);
    element->GetFloat(kAttrDialogueTime, &m_time, m_time);
    element->GetBool(kAttrDialogueBlocking, &m_blocking, m_blocking);
    element->GetFloat(AnimHash("DisplayRate"), &m_displayRate, -1.0f);
    const char* subtitle = element->GetString("subtitle");
    if (subtitle && subtitle[0] != '\0')
        ape::StrCopyBounded(m_subtitle, subtitle, 0x40);
    element->GetHash(kAttrDialogue68, &m_hash68, m_hash68);
    element->GetHash(kAttrDialogue6C, &m_hash6C, m_hash6C);
    element->GetBool(kAttrDialogue78, &m_flag78, false);
    element->GetBool(kAttrDialogue79, &m_flag79, false);
    bool both = false;
    element->GetBool(kAttrDialogueBoth, &both, false);
    if (both)
        m_flag78 = m_flag79 = true;

    const char* buttonText = "";
    uint32_t button = 0;
    if (element->GetHash(kAttrDialogueButton, &button, 0) == ape::xml::kQuery_Ok) {
        for (const DialogueButton& entry : kDialogueButtons) {
            if (entry.nameHash == button) {
                m_buttonMask = entry.mask;
                buttonText = entry.text;
                break;
            }
        }
    } else {
        m_buttonMask = 0;
    }
    element->GetHash(kAttrDialogue7C, &m_hash7C, m_hash7C);
    const char* prompt = element->GetString("promptText");
    if (!prompt)
        prompt = ape::TempFormat("%s $STR_CONTINUE", buttonText);
    ape::StrNCopy(m_promptText, prompt, sizeof(m_promptText));
    m_promptText[sizeof(m_promptText) - 1] = '\0';
    element->GetBool(kAttrDialogueC0, &m_flagC0, m_flagC0);
    element->GetHash(AnimHash("ExclusivityGroup"), &m_exclusivityGroup, m_exclusivityGroup);
    element->GetInt(AnimHash("ExclusivityGroupPriority"), &m_exclusivityPriority, m_exclusivityPriority);
    element->GetHash(kAttrEntity, &m_entity, m_entity);
    // A line that waits for nothing has no time limit either.
    if (m_time < 0.0f && m_buttonMask == 0)
        m_time = 0.0f;
    Link();
}

// @func 0x000A0AAC SeqDialogueCommand::Create
ApeAnimScriptCommand* SeqDialogueCommand::Create(XmlElement* element) const
{
    return new SeqDialogueCommand(element);
}

// @func 0x000A0B1C SeqDialogueCommand::Execute
void SeqDialogueCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_soundName != 0 && !audio::SoundNameHasDef(m_sound)) {
        // Compiled-out warning naming the missing sound and bank.
        (void)ape::TempHexString(m_soundName);
        (void)ape::TempHexString(m_bank);
    }
    // NOTE: the console passes 19 values to 0x0043267C; the sequencer port
    // models 17 of them (the prompt text pointer goes in twice and the
    // blocking byte has no slot there). The mapping below is positional.
    gameplay::AddSequenceCommand(new gameplay::SeqDialogStreamCommand(
        m_time, m_displayRate, m_sound, m_subtitle, m_hash50, m_hash68, m_hash6C, m_blocking, m_buttonMask,
        m_flag78, m_flag79, m_hash7C, m_promptText, m_flagC0, m_exclusivityGroup, uint32_t(m_exclusivityPriority),
        m_entity));
}

// ---------------------------------------------------------------------------
// Hash 0xE37F3CE5 (name unknown): runs a script as part of the sequence,
// with optional texts.

class SeqRunScriptCommand : public ApeAnimScriptCommand {
public:
    SeqRunScriptCommand();
    ~SeqRunScriptCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    TextFieldList m_texts;          // +0x0C
    uint32_t m_script = 0;          // +0x20 attribute 0x849B0778
    uint32_t m_hash24 = 0;          // +0x24 attribute 0x74EB708F
    uint32_t m_hash28 = 0;          // +0x28 attribute 0x74EB4BED
    bool m_wait = true;             // +0x2C attribute 0x8488565B
    bool m_ignoreWarnings = false;  // +0x2D "IgnoreWarnings"
};

// @func 0x000A0C5C SeqRunScriptCommand::SeqRunScriptCommand
SeqRunScriptCommand::SeqRunScriptCommand()
{
    m_texts.reserve(kSeqScriptTextCapacity);
}

// @func 0x00741518 SeqRunScriptCommand::~SeqRunScriptCommand
// @func 0x00741580 SeqRunScriptCommand::~SeqRunScriptCommand (deleting)
SeqRunScriptCommand::~SeqRunScriptCommand() = default;

// @func 0x000A0CF4 SeqRunScriptCommand::Create
ApeAnimScriptCommand* SeqRunScriptCommand::Create(XmlElement* element) const
{
    auto* command = new SeqRunScriptCommand();
    element->GetHash(kAttrScript, &command->m_script, 0);
    element->GetHash(kAttrScript24, &command->m_hash24, command->m_hash24);
    element->GetHash(kAttrScript28, &command->m_hash28, command->m_hash28);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    element->GetBool(kAttrIgnoreWarnings, &command->m_ignoreWarnings, command->m_ignoreWarnings);
    ParseTextFieldList(element, command->m_texts, kSeqScriptTextCapacity);
    return command;
}

// TODO(0x000A0E80 SeqRunScriptCommand::Execute) builds the sequence script command (0x00433120).

// ---------------------------------------------------------------------------
// Hash 0x93C95D39 (name unknown): sequence HUD element switch.

class SeqHudElementCommand : public ApeAnimScriptCommand {
public:
    SeqHudElementCommand();
    ~SeqHudElementCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_wait = true;  // +0x0C bit 31, attribute 0x8488565B
};

// @func 0x000A0FAC SeqHudElementCommand::SeqHudElementCommand
SeqHudElementCommand::SeqHudElementCommand() = default;

// @func 0x00741600 SeqHudElementCommand::~SeqHudElementCommand
// @func 0x00741630 SeqHudElementCommand::~SeqHudElementCommand (deleting)
SeqHudElementCommand::~SeqHudElementCommand() = default;

// @func 0x000A0FFC SeqHudElementCommand::Create
ApeAnimScriptCommand* SeqHudElementCommand::Create(XmlElement* element) const
{
    auto* command = new SeqHudElementCommand();
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x000A10E4 SeqHudElementCommand::Execute
void SeqHudElementCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddHudElement(m_wait);
}

// ---------------------------------------------------------------------------
// Hash 0x661993C7 (#230, name unknown): speaker dialog in the sequence: two
// text keys (literal or string variables), up to six speakers and the HUD
// screen to notify (sequence command 0x004335C0).

namespace {
constexpr uint32_t kAttrPanelMessageVar = AnimHash("messageVar");
constexpr uint32_t kAttrPanelMessage2Var = AnimHash("message2Var");
constexpr uint32_t kAttrPanelScreen = 0x7A73437Du;       // NOTE: name unknown
constexpr uint32_t kAttrPanelScreenVar = 0xAB5BFA6Au;    // NOTE: name unknown (int variable)
constexpr uint32_t kAttrPanelParam = 0xE7CEA5A4u;        // NOTE: name unknown
constexpr uint32_t kAttrPanelParamVar = 0xA8BF12E3u;     // NOTE: name unknown (int variable)
constexpr uint32_t kAttrPanelBlocking = 0x8488565Bu;     // NOTE: name unknown
constexpr uint32_t kAttrPanelPause = 0x1D373FEBu;        // NOTE: name unknown
constexpr uint32_t kAttrPanelNotify = 0x92DE619Au;       // NOTE: name unknown
constexpr uint32_t kTagPanelSpeaker = 0xDB7A793Au;       // NOTE: element name unknown
constexpr uint32_t kAttrPanelSpeakerFlag = 0xF5897C08u;  // NOTE: name unknown

// Text of a string variable, or null.
const char* StringVarText(ApeAnimScriptInstance* instance, uint32_t varHash)
{
    if (varHash == 0)
        return nullptr;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (!var || static_cast<int>(var->GetType()) != kVarType_String)
        return nullptr;
    return static_cast<ScriptStringVar*>(var)->GetString();
}
} // namespace

class SeqSpeakerDialogScriptCommand : public ApeAnimScriptCommand {
public:
    SeqSpeakerDialogScriptCommand();
    explicit SeqSpeakerDialogScriptCommand(XmlElement* element);
    ~SeqSpeakerDialogScriptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    static constexpr size_t kMaxSpeakers = 6;
    char m_message[0x41] = {};                    // +0x0C "message"
    char m_message2[0x41] = {};                   // +0x4D "message2"
    std::vector<gameplay::DialogSpeaker> m_speakers;   // +0x90 (console ApeArray, capacity 6)
    uint32_t m_messageVar = 0;                    // +0xA4
    uint32_t m_message2Var = 0;                   // +0xA8
    uint32_t m_screen = 0;                        // +0xAC
    uint32_t m_screenVar = 0;                     // +0xB0
    int32_t m_param = 0;                          // +0xB4
    uint32_t m_paramVar = 0;                      // +0xB8
    // +0xBC flag bits 31..29.
    bool m_blocking = true;
    bool m_pauseGame = false;
    bool m_notifyScreen = false;
};

// @func 0x000A116C SeqSpeakerDialogScriptCommand::SeqSpeakerDialogScriptCommand
SeqSpeakerDialogScriptCommand::SeqSpeakerDialogScriptCommand() { m_speakers.reserve(kMaxSpeakers); }

// @func 0x000A1224 SeqSpeakerDialogScriptCommand::SeqSpeakerDialogScriptCommand (parsing)
SeqSpeakerDialogScriptCommand::SeqSpeakerDialogScriptCommand(XmlElement* element) : SeqSpeakerDialogScriptCommand()
{
    element->GetHash(kAttrPanelMessageVar, &m_messageVar, 0);
    const char* text = m_messageVar == 0 ? element->GetString("message") : nullptr;
    if (text && text[0] != '\0')
        ape::StrCopyBounded(m_message, text, 0x40);
    element->GetHash(kAttrPanelMessage2Var, &m_message2Var, m_message2Var);
    text = m_message2Var == 0 ? element->GetString("message2") : nullptr;
    if (text && text[0] != '\0')
        ape::StrCopyBounded(m_message2, text, 0x40);
    element->GetHash(kAttrPanelScreen, &m_screen, m_screen);
    element->GetHash(kAttrPanelScreenVar, &m_screenVar, m_screenVar);
    element->GetInt(kAttrPanelParam, &m_param, m_param);
    element->GetHash(kAttrPanelParamVar, &m_paramVar, m_paramVar);
    element->GetBool(kAttrPanelBlocking, &m_blocking, m_blocking);
    element->GetBool(kAttrPanelPause, &m_pauseGame, m_pauseGame);
    element->GetBool(kAttrPanelNotify, &m_notifyScreen, m_notifyScreen);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != kTagPanelSpeaker || m_speakers.size() >= kMaxSpeakers)
            continue;
        gameplay::DialogSpeaker speaker;
        bool flag = true;
        child->GetHash(AnimHash("Name"), &speaker.nameHash, 0);
        child->GetBool(kAttrPanelSpeakerFlag, &flag, flag);
        speaker.flags = flag ? 1 : 0;
        if (speaker.nameHash != 0)
            m_speakers.push_back(speaker);
    }
}

// NOTE: the destructors live at 0x0074xxxx (outside this range).
SeqSpeakerDialogScriptCommand::~SeqSpeakerDialogScriptCommand() = default;

// @func 0x000A1650 SeqSpeakerDialogScriptCommand::Create
ApeAnimScriptCommand* SeqSpeakerDialogScriptCommand::Create(XmlElement* element) const
{
    return new SeqSpeakerDialogScriptCommand(element);
}

// @func 0x000A16C0 SeqSpeakerDialogScriptCommand::Execute
void SeqSpeakerDialogScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // String variables replace the stored texts (the console keeps the copy).
    const char* text = StringVarText(instance, m_messageVar);
    if (text && text[0] != '\0')
        ape::StrCopyBounded(m_message, text, 0x40);
    text = StringVarText(instance, m_message2Var);
    if (text && text[0] != '\0')
        ape::StrCopyBounded(m_message2, text, 0x40);
    const uint32_t screen = uint32_t(ResolveInt(instance, m_screenVar, int32_t(m_screen)));
    const uint32_t param = uint32_t(ResolveInt(instance, m_paramVar, m_param));
    gameplay::AddSequenceCommand(new gameplay::SeqSpeakerDialogCommand(
        m_speakers, m_message, m_message2, screen, param, m_notifyScreen, m_blocking, m_pauseGame));
}

// ---------------------------------------------------------------------------
// Hash 0x226C3AFD (name unknown): sequence message box with a picture.

class SeqMessageBoxCommand : public ApeAnimScriptCommand {
public:
    SeqMessageBoxCommand();
    ~SeqMessageBoxCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_name[0x80] = {};            // +0x0C "name"
    float m_scale[2] = {1.0f, 1.0f};   // +0x8C "Scale" (NOTE: default global 0x01164AF4, assumed 1,1)
    float m_offset[2] = {0.0f, 0.0f};  // +0x94 "Offset" (NOTE: default global 0x01164AFC, assumed 0,0)
    bool m_wait = true;                // +0x9C attribute 0x8488565B
};

// @func 0x000A19B0 SeqMessageBoxCommand::SeqMessageBoxCommand
SeqMessageBoxCommand::SeqMessageBoxCommand() = default;

// @func 0x00741770 SeqMessageBoxCommand::~SeqMessageBoxCommand
// @func 0x007417A0 SeqMessageBoxCommand::~SeqMessageBoxCommand (deleting)
SeqMessageBoxCommand::~SeqMessageBoxCommand() = default;

// @func 0x000A1A30 SeqMessageBoxCommand::Create
ApeAnimScriptCommand* SeqMessageBoxCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMessageBoxCommand();
    const char* name = element->GetString("name");
    if (name && *name)
        std::strncpy(command->m_name, name, 0x7E);
    element->GetVec2(kAttrScale, command->m_scale, command->m_scale);
    element->GetVec2(kAttrOffset, command->m_offset, command->m_offset);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x000A1B98 SeqMessageBoxCommand::Execute
void SeqMessageBoxCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddMessageBox(m_name, m_scale, m_offset, m_wait);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsAA()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("Pause"), new SeqPauseCommand());
    prototypes.Add(0xEC00CC0Fu, new SeqHudCounterCommand());
    prototypes.Add(AnimHash("PauseEntities"), new SeqPauseEntitiesCommand());
    prototypes.Add(0x2D43CE4Cu, new SeqShowHudCommand());
    prototypes.Add(AnimHash("PauseHavok"), new SeqPauseHavokCommand());
    prototypes.Add(AnimHash("SeqPlayAnim"), new SeqPlayAnimCommand());
    prototypes.Add(0x780316BBu, new SeqDialogueCommand());
    prototypes.Add(0xE37F3CE5u, new SeqRunScriptCommand());
    prototypes.Add(0x93C95D39u, new SeqHudElementCommand());
    prototypes.Add(0x661993C7u, new SeqSpeakerDialogScriptCommand());
    prototypes.Add(0x226C3AFDu, new SeqMessageBoxCommand());
}

} // namespace levels
} // namespace sly
