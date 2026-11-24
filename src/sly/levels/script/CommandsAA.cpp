// openTIT - Game script commands, twenty-fifth block (Pause to the sequence message box).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/game/AudioSystem.h"
#include "sly/game/Upgrades.h"
#include "sly/levels/GameServices.h"
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

// TODO(0x0009F888 SeqPauseCommand::Execute) resolves the time variable and appends the
// sequence wait command (0x00431EEC).

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
    ~SeqDialogueCommand() override;
    void Link() override;

private:
    uint32_t m_soundName = 0;             // +0x54 "SoundName"
    uint32_t m_bank = 0;                  // +0x58 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x5C
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

// TODO(0x000A0558 SeqDialogueCommand::SeqDialogueCommand) parser (button prompt table,
// subtitle text, timing) and TODO(0x000A0AAC SeqDialogueCommand::Create),
// TODO(0x000A0B1C SeqDialogueCommand::Execute).

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

// TODO(0x000A116C SeqTextPanelCommand) hash 0x661993C7 (#230): text panel with up to six
// named lines; parser 0x000A1224, Execute 0x000A16C0.

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
    // #227 (0x780316BB) and #230 (0x661993C7) are not registered until their
    // parsers are ported.
    prototypes.Add(0xE37F3CE5u, new SeqRunScriptCommand());
    prototypes.Add(0x93C95D39u, new SeqHudElementCommand());
    prototypes.Add(0x226C3AFDu, new SeqMessageBoxCommand());
}

} // namespace levels
} // namespace sly
