// openTIT - Game script commands, thirteenth block (SetComponentScript to the player timer command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/Upgrades.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrVar = AnimHash("var");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrComponent = AnimHash("component");
constexpr uint32_t kAttrScript = AnimHash("Script");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrPlayerIndex = AnimHash("PlayerIndex");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrComponentName = AnimHash("ComponentName");
constexpr uint32_t kAttrChain1C = 0x002480E8u;   // NOTE: name unknown
constexpr uint32_t kAttrIterate = AnimHash("iterate");
constexpr uint32_t kAttrSpring = AnimHash("spring");
constexpr uint32_t kAttrChain28 = 0x390C8892u;   // NOTE: name unknown
constexpr uint32_t kAttrChain2C = 0x38714C6Bu;   // NOTE: name unknown
constexpr uint32_t kAttrAngleMax = AnimHash("AngleMax");
constexpr uint32_t kAttrChain60 = 0x724F4DDAu;   // NOTE: name unknown
constexpr uint32_t kAttrGravity = AnimHash("gravity");
constexpr uint32_t kAttrChain50 = 0x34BF0B12u;   // NOTE: name unknown
constexpr uint32_t kAttrControllerIdx = AnimHash("ControllerIdx");
constexpr uint32_t kAttrStrength = AnimHash("strength");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrText58 = 0x98129697u;    // NOTE: name unknown; fills the text from a variable
constexpr uint32_t kAttrLimit = AnimHash("limit");
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrTargetObject = AnimHash("TargetObject");
constexpr uint32_t kAttrTargetEntity = AnimHash("TargetEntity");
constexpr uint32_t kAttrColorOverride = AnimHash("ColorOverride");
constexpr uint32_t kAttrOffset = AnimHash("Offset");
constexpr uint32_t kAttrMarkerUnused = 0x3B66FD69u;  // NOTE: name unknown; read and ignored
constexpr uint32_t kAttrMarker38 = 0x5D28E3FCu;
constexpr uint32_t kAttrMarker39 = 0x9446A097u;
constexpr uint32_t kAttrMarker3A = 0xD9159FE7u;
constexpr uint32_t kAttrMarker3B = 0x09DA53D5u;
constexpr uint32_t kAttrMarkerDef = AnimHash("MarkerDef");
constexpr uint32_t kAttrMarker3C = 0x04656B91u;
constexpr uint32_t kAttrMarker3D = 0x310F51B2u;
constexpr uint32_t kAttrIgnoreWarnings = AnimHash("IgnoreWarnings");
constexpr uint32_t kAttrTime = AnimHash("Time");
constexpr uint32_t kAttrForceDisplay = AnimHash("ForceDisplay");
constexpr uint32_t kAttrTimer11 = 0x6D68B156u;   // NOTE: name unknown

// Component kind of script components.
constexpr int kComponentKind_Script = 0x1B;

const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};  // NOTE: console global 0x01164B40
// NOTE: console global 0x01164CA4 (next to the default white); assumed transparent black.
constexpr ape::xml::Color8 kDefaultMarkerColor = {0, 0, 0, 0};
} // namespace

// ---------------------------------------------------------------------------
// "SetComponentScript" (class name from the warning text): sets the script of
// a script component.

class SetComponentScriptCommand : public ApeAnimScriptCommand {
public:
    SetComponentScriptCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_var = 0;        // +0x0C "var"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_component = 0;  // +0x14 "component"
    uint32_t m_script = 0;     // +0x18 "Script"
};

// @func 0x00087FFC SetComponentScriptCommand::SetComponentScriptCommand
SetComponentScriptCommand::SetComponentScriptCommand() = default;

// @func 0x0008804C SetComponentScriptCommand::Create
ApeAnimScriptCommand* SetComponentScriptCommand::Create(XmlElement* element) const
{
    auto* command = new SetComponentScriptCommand();
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrComponent, &command->m_component, command->m_component);
    element->GetHash(kAttrScript, &command->m_script, command->m_script);
    return command;
}

// @func 0x00088160 SetComponentScriptCommand::Execute
void SetComponentScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_var, m_entity);
    if (!entity)
        return;
    void* component = EntityFindComponent(entity, m_component);
    if (!component) {
        ape::TempFormat("AnimScriptCommandSetComponentScript failed because component (%s) could not be found.",
                        ape::TempHexString(m_component));
        return;
    }
    if (ComponentKind(component) == kComponentKind_Script)
        ScriptComponentSetScript(component, m_script);
}

// ---------------------------------------------------------------------------
// "SetComponentVisible" (class name from the warning text).

class SetComponentVisibleCommand : public ApeAnimScriptCommand {
public:
    SetComponentVisibleCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_var = 0;          // +0x0C "var"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_child = 0;        // +0x14 "Child"
    int32_t m_playerIndex = -1;  // +0x18 "PlayerIndex"
    uint32_t m_component = 0;    // +0x1C "component"
    bool m_visible = false;      // +0x20 "visible"
};

// @func 0x000882C8 SetComponentVisibleCommand::SetComponentVisibleCommand
SetComponentVisibleCommand::SetComponentVisibleCommand() = default;

// @func 0x00088330 SetComponentVisibleCommand::Create
ApeAnimScriptCommand* SetComponentVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new SetComponentVisibleCommand();
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetInt(kAttrPlayerIndex, &command->m_playerIndex, command->m_playerIndex);
    element->GetHash(kAttrComponent, &command->m_component, command->m_component);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    return command;
}

// @func 0x00088494 SetComponentVisibleCommand::Execute
void SetComponentVisibleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = nullptr;
    if (m_playerIndex == -1) {
        entity = EntityFromVar(instance, m_var);
    } else {
        entity = PlayerEntityByIndex(m_playerIndex);
    }
    if (!entity && m_entity != 0)
        entity = FindEntity(m_entity);
    if (!entity)
        return;
    if (m_child != 0) {
        entity = EntityFindChild(entity, m_child);
        if (!entity)
            return;
    }
    void* component = EntityFindComponent(entity, m_component);
    if (!component) {
        ape::TempFormat("AnimScriptCommandSetComponentVisible failed because component (%s) could not be found "
                        "for def '%s' entity '%s'.",
                        ape::TempHexString(m_component), EntityDefNameString(entity), EntityNameString(entity));
        return;
    }
    ComponentSetVisible(component, m_visible, true);
}

// ---------------------------------------------------------------------------
// Hash 0xDF50133E (name unknown): overrides the simulation parameters of a
// chain/spring component; only attributes present in the element are applied
// (mask +0x18).

class ChainParamsCommand : public ApeAnimScriptCommand {
public:
    ChainParamsCommand();
    explicit ChainParamsCommand(XmlElement* element);
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    enum : uint32_t {
        kHas1C = 0x01,
        kHasIterate = 0x02,
        kHasSpring = 0x04,
        kHas28 = 0x08,
        kHas2C = 0x10,
        kHasAngleMax = 0x20,
        kHasGravity = 0x40,
        kHas50 = 0x80,
        kHas60 = 0x100,
    };

private:
    uint32_t m_object = 0;          // +0x0C "object"
    uint32_t m_entity = 0;          // +0x10 "Entity"
    uint32_t m_componentName = 0;   // +0x14 "ComponentName"
    uint32_t m_mask = 0;            // +0x18 attributes present
    bool m_flag1C = false;          // +0x1C attribute 0x002480E8
    int32_t m_iterate = 2;          // +0x20 "iterate"
    float m_spring = 0.05f;         // +0x24 "spring"
    float m_value28 = 0.1f;         // +0x28 attribute 0x390C8892
    float m_value2C = 0.1f;         // +0x2C attribute 0x38714C6B
    float m_angleMax = 30.0f;       // +0x30 "AngleMax"
    ape::Vec3 m_gravity = kZero;    // +0x40 "gravity"
    ape::Vec3 m_vector50 = kZero;   // +0x50 attribute 0x34BF0B12
    bool m_flag60 = false;          // +0x60 attribute 0x724F4DDA
};

// @func 0x0008867C ChainParamsCommand::ChainParamsCommand
ChainParamsCommand::ChainParamsCommand() = default;

// @func 0x00088704 ChainParamsCommand::ChainParamsCommand
ChainParamsCommand::ChainParamsCommand(XmlElement* element)
{
    using ape::xml::kQuery_Ok;
    element->GetHash(kAttrObject, &m_object, 0);
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrComponentName, &m_componentName, m_componentName);
    if (element->GetBool(kAttrChain1C, &m_flag1C, m_flag1C) == kQuery_Ok)
        m_mask |= kHas1C;
    if (element->GetInt(kAttrIterate, &m_iterate, m_iterate) == kQuery_Ok)
        m_mask |= kHasIterate;
    if (element->GetFloat(kAttrSpring, &m_spring, m_spring) == kQuery_Ok)
        m_mask |= kHasSpring;
    if (element->GetFloat(kAttrChain28, &m_value28, m_value28) == kQuery_Ok)
        m_mask |= kHas28;
    if (element->GetFloat(kAttrChain2C, &m_value2C, m_value2C) == kQuery_Ok)
        m_mask |= kHas2C;
    if (element->GetFloat(kAttrAngleMax, &m_angleMax, m_angleMax) == kQuery_Ok)
        m_mask |= kHasAngleMax;
    if (element->GetBool(kAttrChain60, &m_flag60, m_flag60) == kQuery_Ok)
        m_mask |= kHas60;
    ape::Vec3 value = kZero;
    if (element->GetVec3(kAttrGravity, &value.x, &kZero.x) == kQuery_Ok) {
        m_gravity = value;
        m_mask |= kHasGravity;
    }
    if (element->GetVec3(kAttrChain50, &value.x, &kZero.x) == kQuery_Ok) {
        m_vector50 = value;
        m_mask |= kHas50;
    }
}

// @func 0x00088A4C ChainParamsCommand::Create
ApeAnimScriptCommand* ChainParamsCommand::Create(XmlElement* element) const
{
    return new ChainParamsCommand(element);
}

// @func 0x00088ABC ChainParamsCommand::Execute
// Overrides the parameters given in the script on a chain component (named,
// else the entity's first component of type 0x0F).
void ChainParamsCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity) {
        ScriptWarning("ChainParams: no entity %s", m_entity);
        return;
    }
    void* component = m_componentName != 0 ? EntityFindComponentById(entity, m_componentName)
                                           : EntityFindComponentOfType(entity, kComponent_Chain);
    if (!component) {
        ScriptWarning("ChainParams: no component %s", m_componentName);
        return;
    }
    ChainComponentParams& params = ChainParams(component);
    if (m_mask & kHas1C)
        params.flag31 = m_flag1C;
    if (m_mask & kHasIterate)
        params.iterations = static_cast<uint8_t>(m_iterate);
    if (m_mask & kHasSpring)
        params.spring = m_spring;
    if (m_mask & kHas28)
        params.value28 = m_value28;
    if (m_mask & kHas2C)
        params.value2C = m_value2C;
    if (m_mask & kHasAngleMax)
        params.angleMax = m_angleMax;
    if (m_mask & kHasGravity)
        params.gravity = m_gravity;
    if (m_mask & kHas50)
        params.vector50 = m_vector50;
    // NOTE: the console tests kHas1C again here, not kHas60.
    if (m_mask & kHas1C)
        params.flag30 = m_flag60;
}

// ---------------------------------------------------------------------------
// "SetControllerRumble" (NOTE: name from a hash match)

class SetControllerRumbleCommand : public ApeAnimScriptCommand {
public:
    SetControllerRumbleCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_controllerIdx = -1;  // +0x0C "ControllerIdx"
    float m_strength = 0.0f;       // +0x10 "strength"
    float m_duration = 0.0f;       // +0x14 "duration"
};

// @func 0x00088D44 SetControllerRumbleCommand::SetControllerRumbleCommand
SetControllerRumbleCommand::SetControllerRumbleCommand() = default;

// @func 0x00088D94 SetControllerRumbleCommand::Create
ApeAnimScriptCommand* SetControllerRumbleCommand::Create(XmlElement* element) const
{
    auto* command = new SetControllerRumbleCommand();
    if (element->GetInt(kAttrControllerIdx, &command->m_controllerIdx, 0) == ape::xml::kQuery_Ok) {
        element->GetFloat(kAttrStrength, &command->m_strength, command->m_strength);
        element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    }
    return command;
}

// @func 0x00088E98 SetControllerRumbleCommand::Execute
void SetControllerRumbleCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    ControllerRumble(m_controllerIdx, m_strength, m_duration);
}

// ---------------------------------------------------------------------------
// Hash 0xBF6B7FBB (name unknown): shows a text message, optionally filled in
// from the variable named by the text's id.

class ShowVarTextCommand : public ApeAnimScriptCommand {
public:
    ShowVarTextCommand();
    ~ShowVarTextCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    TextField m_text;          // +0x0C first child element the text parser accepts
    bool m_fromVar = false;    // +0x58 attribute 0x98129697
};

// @func 0x00088EE0 ShowVarTextCommand::ShowVarTextCommand
ShowVarTextCommand::ShowVarTextCommand()
{
    m_text.Clear();
}

ShowVarTextCommand::~ShowVarTextCommand() = default;

// @func 0x00088F38 ShowVarTextCommand::Create
ApeAnimScriptCommand* ShowVarTextCommand::Create(XmlElement* element) const
{
    auto* command = new ShowVarTextCommand();
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (TextFieldParse(child, &command->m_text))
            break;
    }
    element->GetBool(kAttrText58, &command->m_fromVar, command->m_fromVar);
    return command;
}

// @func 0x00089048 ShowVarTextCommand::Execute
void ShowVarTextCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // flagA marks a text that was parsed successfully.
    if (!m_text.flagA)
        return;
    if (m_fromVar) {
        if (ApeAnimScriptVar* var = instance->FindVar(m_text.id))
            TextFieldApplyVar(&m_text, var);
    }
    if (TextDisplayAvailable())
        TextDisplayShowMessage(m_text);
}

// ---------------------------------------------------------------------------
// Hash 0x824684A7 (name unknown): sets a limit value of the game state.

class SetGameLimitCommand : public ApeAnimScriptCommand {
public:
    SetGameLimitCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_limit = 0.0f;  // +0x0C "limit"
};

// @func 0x000890DC SetGameLimitCommand::SetGameLimitCommand
SetGameLimitCommand::SetGameLimitCommand() = default;

// @func 0x00089138 SetGameLimitCommand::Create
ApeAnimScriptCommand* SetGameLimitCommand::Create(XmlElement* element) const
{
    auto* command = new SetGameLimitCommand();
    element->GetFloat(kAttrLimit, &command->m_limit, 0.0f);
    return command;
}

// @func 0x000891E4 SetGameLimitCommand::Execute
// NOTE: byte-identical to 0x004384E8 (another module's setter of the same field).
void SetGameLimitCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameStateSetLimit(m_limit);
}

// ---------------------------------------------------------------------------
// Hash 0x270D8A8D (name unknown): places a HUD marker on an entity, optionally
// pointing at a target.

class SetMarkerCommand : public ApeAnimScriptCommand {
public:
    SetMarkerCommand();
    explicit SetMarkerCommand(XmlElement* element);
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::xml::Color8 m_color = kDefaultMarkerColor;  // +0x0C "ColorOverride"
    ape::Vec3 m_offset = kZero;                      // +0x10 "Offset"
    int32_t m_playerIdx = -1;                        // +0x20 "PlayerIdx" (1-based in data)
    uint32_t m_object = 0;                           // +0x24 "object"
    uint32_t m_entity = 0;                           // +0x28 "Entity"
    uint32_t m_targetObject = 0;                     // +0x2C "TargetObject"
    uint32_t m_targetEntity = 0;                     // +0x30 "TargetEntity"
    uint32_t m_markerDef = 0;                        // +0x34 "MarkerDef"
    bool m_flag38 = false;                           // +0x38 attribute 0x5D28E3FC
    bool m_flag39 = false;                           // +0x39 attribute 0x9446A097
    bool m_flag3A = false;                           // +0x3A attribute 0xD9159FE7
    bool m_flag3B = false;                           // +0x3B attribute 0x09DA53D5
    bool m_flag3C = false;                           // +0x3C attribute 0x04656B91
    bool m_flag3D = false;                           // +0x3D attribute 0x310F51B2
    bool m_ignoreWarnings = false;                   // +0x3E "IgnoreWarnings"
};

// @func 0x00089204 SetMarkerCommand::SetMarkerCommand
SetMarkerCommand::SetMarkerCommand() = default;

// @func 0x000892D4 SetMarkerCommand::SetMarkerCommand
SetMarkerCommand::SetMarkerCommand(XmlElement* element)
{
    if (element->GetInt(kAttrPlayerIdx, &m_playerIdx, -1) == ape::xml::kQuery_Ok && m_playerIdx > 0)
        --m_playerIdx;
    element->GetHash(kAttrObject, &m_object, m_object);
    element->GetHash(kAttrEntity, &m_entity, m_entity);
    element->GetHash(kAttrTargetObject, &m_targetObject, m_targetObject);
    element->GetHash(kAttrTargetEntity, &m_targetEntity, m_targetEntity);
    element->GetColor(kAttrColorOverride, &m_color, &m_color);
    element->GetVec3(kAttrOffset, &m_offset.x, &m_offset.x);
    bool unused = false;
    element->GetBool(kAttrMarkerUnused, &unused, false);
    element->GetBool(kAttrMarker38, &m_flag38, false);
    element->GetBool(kAttrMarker39, &m_flag39, false);
    element->GetBool(kAttrMarker3A, &m_flag3A, false);
    element->GetBool(kAttrMarker3B, &m_flag3B, false);
    element->GetHash(kAttrMarkerDef, &m_markerDef, 0);
    element->GetBool(kAttrMarker3C, &m_flag3C, false);
    element->GetBool(kAttrMarker3D, &m_flag3D, false);
    element->GetBool(kAttrIgnoreWarnings, &m_ignoreWarnings, m_ignoreWarnings);
}

// @func 0x00089608 SetMarkerCommand::Create
ApeAnimScriptCommand* SetMarkerCommand::Create(XmlElement* element) const
{
    return new SetMarkerCommand(element);
}

// TODO(0x00089678 SetMarkerCommand::Execute) resolves owner and target like the gadget
// target command, then sets up the marker through 0x0029F760.

// ---------------------------------------------------------------------------
// Hash 0xDB38C585 (name unknown): sets a timer on every player character.

class SetPlayerTimerCommand : public ApeAnimScriptCommand {
public:
    SetPlayerTimerCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_time = -1.0f;        // +0x0C "Time"
    bool m_forceDisplay = false; // +0x10 "ForceDisplay"
    bool m_flag11 = false;       // +0x11 attribute 0x6D68B156
};

// @func 0x000899A8 SetPlayerTimerCommand::SetPlayerTimerCommand
SetPlayerTimerCommand::SetPlayerTimerCommand() = default;

// @func 0x00089A10 SetPlayerTimerCommand::Create
ApeAnimScriptCommand* SetPlayerTimerCommand::Create(XmlElement* element) const
{
    auto* command = new SetPlayerTimerCommand();
    element->GetFloat(kAttrTime, &command->m_time, -1.0f);
    element->GetBool(kAttrForceDisplay, &command->m_forceDisplay, command->m_forceDisplay);
    element->GetBool(kAttrTimer11, &command->m_flag11, command->m_flag11);
    return command;
}

// @func 0x00089B0C SetPlayerTimerCommand::Execute
void SetPlayerTimerCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    for (int i = 0; i < kPlayerCharacterSlots; ++i) {
        WorldObject* character = PlayerCharacterSlot(i);
        if (character && EntityIsCharacter(character))
            CharacterSetTimer(character, m_time, m_forceDisplay, m_flag11);
    }
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsO()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xC4C20B66u, new SetComponentScriptCommand());
    prototypes.Add(0x7C7D0917u, new SetComponentVisibleCommand());
    prototypes.Add(0xDF50133Eu, new ChainParamsCommand());
    prototypes.Add(AnimHash("SetControllerRumble"), new SetControllerRumbleCommand());
    prototypes.Add(0xBF6B7FBBu, new ShowVarTextCommand());
    prototypes.Add(0x824684A7u, new SetGameLimitCommand());
    prototypes.Add(0x270D8A8Du, new SetMarkerCommand());
    prototypes.Add(0xDB38C585u, new SetPlayerTimerCommand());
}

} // namespace levels
} // namespace sly
