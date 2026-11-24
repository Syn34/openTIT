// openTIT - Game script commands, tenth block (screen colour fade to the hash 0xF22A1F1A command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/game/AudioSystem.h"
#include "sly/game/Upgrades.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"
#include "sly/levels/ai/AIPriorityTargets.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrStartColor = AnimHash("StartColor");
constexpr uint32_t kAttrEndColor = AnimHash("EndColor");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrFade18 = 0x0AA56D8Bu;  // NOTE: name unknown
constexpr uint32_t kAttrSoundNameVar = AnimHash("SoundNameVar");
constexpr uint32_t kAttrSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrAttachType = AnimHash("AttachType");
constexpr uint32_t kAttrAttachTypePlayer = AnimHash("AttachTypePlayer");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrOffsetPos = AnimHash("OffsetPos");
constexpr uint32_t kAttrVolume = AnimHash("volume");
constexpr uint32_t kAttrSound44 = 0x3404783Cu;  // NOTE: name unknown
constexpr uint32_t kAttrSound48 = 0x9A0F95CAu;  // NOTE: name unknown
constexpr uint32_t kAttrSound4C = 0xD547CE43u;  // NOTE: name unknown
constexpr uint32_t kAttrSound4D = 0xFFB20224u;  // NOTE: name unknown
constexpr uint32_t kAttrSound4E = 0x6334AE6Fu;  // NOTE: name unknown
constexpr uint32_t kAttrUseSurfaceData = AnimHash("UseSurfaceData");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrExclusivityGroupPriority = AnimHash("ExclusivityGroupPriority");
constexpr uint32_t kAttrSound5D = 0xC6C71707u;  // NOTE: name unknown
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrTarget = AnimHash("Target");
constexpr uint32_t kAttrTargetVar = AnimHash("TargetVar");
constexpr uint32_t kAttrOp = AnimHash("Op");
constexpr uint32_t kOpAdd = AnimHash("Add");
constexpr uint32_t kOpRemove = AnimHash("Remove");
constexpr uint32_t kOpClear = AnimHash("Clear");
constexpr uint32_t kAttrScript = AnimHash("Script");
constexpr uint32_t kAttrScriptVar = AnimHash("ScriptVar");
constexpr uint32_t kAttrScriptLib = AnimHash("ScriptLib");
constexpr uint32_t kVarSpawnPt = AnimHash("SpawnPT");
constexpr uint32_t kAttrGroup = AnimHash("group");
constexpr uint32_t kEntityThis = AnimHash("this");
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrTargetObject = AnimHash("TargetObject");
constexpr uint32_t kAttrTargetEntity = AnimHash("TargetEntity");
constexpr uint32_t kAttrIgnoreWarnings = AnimHash("IgnoreWarnings");

constexpr ape::xml::Color8 kTransparentBlack = {0, 0, 0, 0};
constexpr ape::xml::Color8 kOpaqueBlack = {0, 0, 0, 0xFF};
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};  // NOTE: console global 0x01164B40

// Text capacity of the entity message command (fixed on the console).
constexpr size_t kEntityMessageCapacity = 0x10;
// Text capacity of QueueCutscene. NOTE: read from the global 0x00F65C7C at
// construction; its value is not known.
constexpr size_t kQueueCutsceneTextCapacity = 0x10;

// Int variable override used by several commands: the variable's value when
// it is an int variable with a non-zero value.
uint32_t HashFromIntVar(ApeAnimScriptInstance* instance, uint32_t varHash, uint32_t fallback)
{
    if (varHash == 0)
        return fallback;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (var && var->GetType() == AnimScriptVarType::Int) {
        uint32_t value = static_cast<uint32_t>(var->GetInt());
        if (value != 0)
            return value;
    }
    return fallback;
}
} // namespace

// ---------------------------------------------------------------------------
// Hash 0xEB05E07E (name unknown): fades the screen between two colours.

class ScreenColorFadeCommand : public ApeAnimScriptCommand {
public:
    ScreenColorFadeCommand();
    ~ScreenColorFadeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::xml::Color8 m_startColor = kTransparentBlack;  // +0x0C "StartColor"
    ape::xml::Color8 m_endColor = kOpaqueBlack;         // +0x10 "EndColor"
    float m_duration = 1.0f;                            // +0x14 "duration"
    bool m_flag18 = false;                              // +0x18 attribute 0x0AA56D8B
};

// @func 0x000835A8 ScreenColorFadeCommand::ScreenColorFadeCommand
ScreenColorFadeCommand::ScreenColorFadeCommand() = default;

// @func 0x0073DAA0 ScreenColorFadeCommand::~ScreenColorFadeCommand
// @func 0x0073DAD0 ScreenColorFadeCommand::~ScreenColorFadeCommand (deleting)
ScreenColorFadeCommand::~ScreenColorFadeCommand() = default;

// @func 0x00083620 ScreenColorFadeCommand::Create
ApeAnimScriptCommand* ScreenColorFadeCommand::Create(XmlElement* element) const
{
    auto* command = new ScreenColorFadeCommand();
    element->GetColor(kAttrStartColor, &command->m_startColor, &command->m_startColor);
    element->GetColor(kAttrEndColor, &command->m_endColor, &command->m_endColor);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetBool(kAttrFade18, &command->m_flag18, command->m_flag18);
    return command;
}

// @func 0x0008375C ScreenColorFadeCommand::Execute
void ScreenColorFadeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    ScreenColorFade(m_startColor, m_endColor, m_duration, m_flag18);
}

// ---------------------------------------------------------------------------
// "PlaySound"

class PlaySoundCommand : public ApeAnimScriptCommand {
public:
    PlaySoundCommand();
    explicit PlaySoundCommand(XmlElement* element);
    ~PlaySoundCommand() override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    // Looks the sound up again unless the held reference is still valid
    // (only for sounds named directly, not through a variable).
    void RefreshSound();

    uint32_t m_soundName = 0;                 // +0x0C "SoundName"
    uint32_t m_bank = 0;                      // +0x10 "Bank"
    bool m_flag14 = false;                    // +0x14
    audio::SoundName* m_sound = nullptr;      // +0x18
    uint32_t m_soundNameVar = 0;              // +0x1C "SoundNameVar"
    uint32_t m_attachType = 0;                // +0x20 "AttachType"
    uint32_t m_attachTypePlayer = 0;          // +0x24 "AttachTypePlayer" (defaults to AttachType)
    uint32_t m_refObject = 0;                 // +0x28 "RefObject"
    ape::Vec3 m_offsetPos = kZero;            // +0x30 "OffsetPos"
    float m_volume = 1.0f;                    // +0x40 "volume"
    uint32_t m_hash44 = 0;                    // +0x44 attribute 0x3404783C
    uint32_t m_hash48 = 0;                    // +0x48 attribute 0x9A0F95CA
    bool m_flag4C = false;                    // +0x4C attribute 0xD547CE43
    bool m_flag4D = false;                    // +0x4D attribute 0xFFB20224
    bool m_flag4E = false;                    // +0x4E attribute 0x6334AE6F
    bool m_useSurfaceData = true;             // +0x4F "UseSurfaceData"
    uint32_t m_part = 0;                      // +0x50 "part"
    uint32_t m_exclusivityGroup = 0;          // +0x54 "ExclusivityGroup"
    int32_t m_exclusivityPriority = 0;        // +0x58 "ExclusivityGroupPriority"
    bool m_playing = false;                   // +0x5C bit 31 (runtime)
    bool m_flag5D = false;                    // +0x5D attribute 0xC6C71707
};

// @func 0x000837B8 PlaySoundCommand::PlaySoundCommand
PlaySoundCommand::PlaySoundCommand() = default;

void PlaySoundCommand::RefreshSound()
{
    if (m_soundNameVar == 0 && m_soundName != 0 && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_soundName, m_bank);
    }
}

// @func 0x00083880 PlaySoundCommand::Link
void PlaySoundCommand::Link()
{
    RefreshSound();
}

// @func 0x000838F4 PlaySoundCommand::PlaySoundCommand
PlaySoundCommand::PlaySoundCommand(XmlElement* element)
{
    if (element->GetHash(kAttrSoundNameVar, &m_soundNameVar, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrSoundName, &m_soundName, 0) != ape::xml::kQuery_Ok)
        return;
    element->GetHash(kAttrBank, &m_bank, 0);
    element->GetHash(kAttrAttachType, &m_attachType, 0);
    element->GetHash(kAttrAttachTypePlayer, &m_attachTypePlayer, m_attachType);
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetVec3(kAttrOffsetPos, &m_offsetPos.x, &kZero.x);
    element->GetFloat(kAttrVolume, &m_volume, m_volume);
    element->GetHash(kAttrSound44, &m_hash44, 0);
    element->GetHash(kAttrSound48, &m_hash48, 0);
    element->GetBool(kAttrSound4C, &m_flag4C, false);
    element->GetBool(kAttrSound4D, &m_flag4D, false);
    element->GetBool(kAttrSound4E, &m_flag4E, false);
    element->GetBool(kAttrUseSurfaceData, &m_useSurfaceData, m_useSurfaceData);
    element->GetHash(kAttrPart, &m_part, 0);
    element->GetHash(kAttrExclusivityGroup, &m_exclusivityGroup, m_exclusivityGroup);
    element->GetInt(kAttrExclusivityGroupPriority, &m_exclusivityPriority, m_exclusivityPriority);
    element->GetBool(kAttrSound5D, &m_flag5D, m_flag5D);
    RefreshSound();
}

// @func 0x00083C8C PlaySoundCommand::~PlaySoundCommand
// @func 0x00083CE0 PlaySoundCommand::~PlaySoundCommand (deleting)
PlaySoundCommand::~PlaySoundCommand()
{
    audio::SoundNameRelease(m_sound);
    m_sound = nullptr;
}

// @func 0x00083D54 PlaySoundCommand::Create
ApeAnimScriptCommand* PlaySoundCommand::Create(XmlElement* element) const
{
    return new PlaySoundCommand(element);
}

// TODO(0x00083DC4 PlaySoundCommand::Execute) "PlaySound fails to find sound %s (in bank %s)
// for script %s state %s": 3D/attached sound start through the audio system.

// ---------------------------------------------------------------------------
// Hash 0xF7E87759 (name unknown): hands a list of text messages to a
// killable entity.

class EntityMessageCommand : public ApeAnimScriptCommand {
public:
    EntityMessageCommand();
    ~EntityMessageCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    TextFieldList m_texts;  // +0x0C array of up to 16 texts
    uint32_t m_object = 0;  // +0x20 "object"
};

// @func 0x00084410 EntityMessageCommand::EntityMessageCommand
EntityMessageCommand::EntityMessageCommand()
{
    m_texts.reserve(kEntityMessageCapacity);
}

// @func 0x0073DB28 EntityMessageCommand::~EntityMessageCommand
// @func 0x0073DB90 EntityMessageCommand::~EntityMessageCommand (deleting)
EntityMessageCommand::~EntityMessageCommand() = default;

// @func 0x00084490 EntityMessageCommand::Create
ApeAnimScriptCommand* EntityMessageCommand::Create(XmlElement* element) const
{
    auto* command = new EntityMessageCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    ParseTextFieldList(element, command->m_texts, kEntityMessageCapacity);
    return command;
}

// @func 0x00084584 EntityMessageCommand::Execute
void EntityMessageCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = EntityFromObject(instance->ResolveObject(m_object, false));
    if (entity && EntityHasClassFlag(entity, kEntityClass_Killable))
        EntityShowMessages(entity, m_texts, instance);
}

// ---------------------------------------------------------------------------
// "PriorityTarget": adds, removes or clears AI priority targets.

class PriorityTargetCommand : public ApeAnimScriptCommand {
public:
    PriorityTargetCommand();
    ~PriorityTargetCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_target = 0;      // +0x0C "Target"
    uint32_t m_targetVar = 0;   // +0x10 "TargetVar" (int variable holding a name)
    uint32_t m_op = kOpAdd;     // +0x14 "Op": Add, Remove or Clear
};

// @func 0x00084624 PriorityTargetCommand::PriorityTargetCommand
PriorityTargetCommand::PriorityTargetCommand() = default;

// @func 0x0073DC10 PriorityTargetCommand::~PriorityTargetCommand
// @func 0x0073DC40 PriorityTargetCommand::~PriorityTargetCommand (deleting)
PriorityTargetCommand::~PriorityTargetCommand() = default;

// @func 0x00084684 PriorityTargetCommand::Create
ApeAnimScriptCommand* PriorityTargetCommand::Create(XmlElement* element) const
{
    auto* command = new PriorityTargetCommand();
    element->GetHash(kAttrTarget, &command->m_target, 0);
    element->GetHash(kAttrTargetVar, &command->m_targetVar, command->m_targetVar);
    element->GetHash(kAttrOp, &command->m_op, command->m_op);
    return command;
}

// @func 0x00084778 PriorityTargetCommand::Execute
void PriorityTargetCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    AIPriorityTargets* targets = GetAIPriorityTargets();
    if (!targets)
        return;
    if (m_op == kOpClear) {
        targets->Clear();
        return;
    }
    uint32_t name = HashFromIntVar(instance, m_targetVar, m_target);
    WorldObject* entity = FindEntity(name);
    if (!entity)
        return;
    if (m_op == kOpRemove)
        targets->Remove(entity);
    else
        targets->Add(entity);
}

// ---------------------------------------------------------------------------
// "QueueCutscene": queues a script as a cut scene, with optional texts.

class QueueCutsceneCommand : public ApeAnimScriptCommand {
public:
    QueueCutsceneCommand();
    ~QueueCutsceneCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    TextFieldList m_texts;      // +0x0C
    uint32_t m_script = 0;      // +0x20 "Script"
    uint32_t m_scriptVar = 0;   // +0x24 "ScriptVar" (int variable holding a name)
    uint32_t m_scriptLib = 0;   // +0x28 "ScriptLib"
};

// @func 0x000848CC QueueCutsceneCommand::QueueCutsceneCommand
QueueCutsceneCommand::QueueCutsceneCommand()
{
    m_texts.reserve(kQueueCutsceneTextCapacity);
}

// @func 0x0073DC98 QueueCutsceneCommand::~QueueCutsceneCommand
// @func 0x0073DD00 QueueCutsceneCommand::~QueueCutsceneCommand (deleting)
QueueCutsceneCommand::~QueueCutsceneCommand() = default;

// @func 0x0008496C QueueCutsceneCommand::Create
ApeAnimScriptCommand* QueueCutsceneCommand::Create(XmlElement* element) const
{
    auto* command = new QueueCutsceneCommand();
    element->GetHash(kAttrScript, &command->m_script, 0);
    element->GetHash(kAttrScriptVar, &command->m_scriptVar, 0);
    element->GetHash(kAttrScriptLib, &command->m_scriptLib, 0);
    ParseTextFieldList(element, command->m_texts, kQueueCutsceneTextCapacity);
    return command;
}

// @func 0x00084ABC QueueCutsceneCommand::Execute
void QueueCutsceneCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_script == 0 && m_scriptVar == 0)
        return;
    uint32_t script = m_script;
    if (m_scriptVar != 0) {
        ApeAnimScriptVar* var = instance->FindVar(m_scriptVar);
        if (var && var->GetType() == AnimScriptVarType::Int)
            script = static_cast<uint32_t>(var->GetInt());
    }
    int32_t spawnPoint = ResolveInt(instance, kVarSpawnPt, 0);
    GameWorldQueueCutscene(script, m_scriptLib, true, m_texts, spawnPoint, instance);
}

// @func 0x00084C5C QueueCutsceneCommand::CreateDebugElement
XmlElement* QueueCutsceneCommand::CreateDebugElement() const
{
    return NewDebugElement("QueueCutscene");
}

// ---------------------------------------------------------------------------
// "RegisterListener": makes an entity listen to an event group.

class RegisterListenerCommand : public ApeAnimScriptCommand {
public:
    RegisterListenerCommand();
    ~RegisterListenerCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = kEntityThis;  // +0x0C "Entity"
    uint32_t m_group = 0;             // +0x10 "group"
};

// @func 0x00084CC0 RegisterListenerCommand::RegisterListenerCommand
RegisterListenerCommand::RegisterListenerCommand() = default;

// @func 0x0073DD80 RegisterListenerCommand::~RegisterListenerCommand
// @func 0x0073DDB0 RegisterListenerCommand::~RegisterListenerCommand (deleting)
RegisterListenerCommand::~RegisterListenerCommand() = default;

// @func 0x00084D10 RegisterListenerCommand::Create
ApeAnimScriptCommand* RegisterListenerCommand::Create(XmlElement* element) const
{
    auto* command = new RegisterListenerCommand();
    element->GetHash(kAttrEntity, &command->m_entity, kEntityThis);
    element->GetHash(kAttrGroup, &command->m_group, command->m_group);
    return command;
}

// @func 0x00084DE4 RegisterListenerCommand::Execute
void RegisterListenerCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (WorldObject* entity = ResolveEventEntity(instance, m_entity))
        EntityRegisterListener(entity, m_group, true);
}

// ---------------------------------------------------------------------------
// Hash 0x7C3AAA8B (name unknown): points a gadget of a player (or entity) at
// a target entity.

class GadgetTargetCommand : public ApeAnimScriptCommand {
public:
    GadgetTargetCommand();
    ~GadgetTargetCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerIdx = -1;       // +0x0C "PlayerIdx" (1-based in data)
    uint32_t m_object = 0;          // +0x10 "object"
    uint32_t m_entity = 0;          // +0x14 "Entity"
    uint32_t m_targetObject = 0;    // +0x18 "TargetObject"
    uint32_t m_targetEntity = 0;    // +0x1C "TargetEntity"
    bool m_ignoreWarnings = false;  // +0x20 "IgnoreWarnings"
};

// @func 0x00084E34 GadgetTargetCommand::GadgetTargetCommand
GadgetTargetCommand::GadgetTargetCommand() = default;

// @func 0x0073DE08 GadgetTargetCommand::~GadgetTargetCommand
// @func 0x0073DE38 GadgetTargetCommand::~GadgetTargetCommand (deleting)
GadgetTargetCommand::~GadgetTargetCommand() = default;

// @func 0x00084E9C GadgetTargetCommand::Create
ApeAnimScriptCommand* GadgetTargetCommand::Create(XmlElement* element) const
{
    auto* command = new GadgetTargetCommand();
    if (element->GetInt(kAttrPlayerIdx, &command->m_playerIdx, -1) == ape::xml::kQuery_Ok &&
        command->m_playerIdx > 0)
        --command->m_playerIdx;
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrTargetObject, &command->m_targetObject, command->m_targetObject);
    element->GetHash(kAttrTargetEntity, &command->m_targetEntity, command->m_targetEntity);
    element->GetBool(kAttrIgnoreWarnings, &command->m_ignoreWarnings, command->m_ignoreWarnings);
    return command;
}

// @func 0x00085024 GadgetTargetCommand::Execute
void GadgetTargetCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* owner = m_playerIdx >= 0 ? PlayerEntityByIndex(m_playerIdx) : nullptr;
    if (!owner)
        owner = ResolveEntity(instance, m_object, m_entity);
    void* gadget = owner ? EntityTargetingGadget(owner) : nullptr;
    if (!gadget)
        return;  // the release build's warnings print nothing
    WorldObject* target = ResolveEntity(instance, m_targetObject, m_targetEntity);
    if (target)
        GadgetSetTarget(gadget, target);
}

// ---------------------------------------------------------------------------
// Hash 0xF22A1F1A (name unknown): gives an object (or a part of it) to the
// player's scanner gadget.

class ScanObjectCommand : public ApeAnimScriptCommand {
public:
    ScanObjectCommand();
    ~ScanObjectCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_object = 0;  // +0x10 "object"
    uint32_t m_part = 0;    // +0x14 "part"
};

// @func 0x000852A8 ScanObjectCommand::ScanObjectCommand
ScanObjectCommand::ScanObjectCommand() = default;

// @func 0x000852F8 ScanObjectCommand::~ScanObjectCommand
// @func 0x00085328 ScanObjectCommand::~ScanObjectCommand (deleting)
ScanObjectCommand::~ScanObjectCommand() = default;

// @func 0x00085380 ScanObjectCommand::Create
ApeAnimScriptCommand* ScanObjectCommand::Create(XmlElement* element) const
{
    auto* command = new ScanObjectCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrPart, &command->m_part, 0);
    return command;
}

// TODO(0x0008546C ScanObjectCommand::Execute) part search through the object hierarchy
// (0x0057F5D8) then 0x002A8F18 on the player's binocucom; the part branch is unclear.

// ---------------------------------------------------------------------------

void RegisterScriptCommandsL()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xEB05E07Eu, new ScreenColorFadeCommand());
    prototypes.Add(AnimHash("PlaySound"), new PlaySoundCommand());
    prototypes.Add(0xF7E87759u, new EntityMessageCommand());
    prototypes.Add(AnimHash("PriorityTarget"), new PriorityTargetCommand());
    prototypes.Add(AnimHash("QueueCutscene"), new QueueCutsceneCommand());
    prototypes.Add(AnimHash("RegisterListener"), new RegisterListenerCommand());
    prototypes.Add(0x7C3AAA8Bu, new GadgetTargetCommand());
    prototypes.Add(0xF22A1F1Au, new ScanObjectCommand());
}

} // namespace levels
} // namespace sly
