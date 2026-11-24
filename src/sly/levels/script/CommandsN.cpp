// openTIT - Game script commands, twelfth block (SetBinocucomLocked to the character controller command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrLocked = AnimHash("locked");
constexpr uint32_t kAttrPower = AnimHash("Power");
constexpr uint32_t kAttrRate = AnimHash("Rate");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrScriptName = AnimHash("ScriptName");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrAlpha = AnimHash("Alpha");
constexpr uint32_t kAttrAlphaVar = AnimHash("AlphaVar");
constexpr uint32_t kAttrBlend18 = 0xF5897C08u;       // NOTE: name unknown
constexpr uint32_t kAttrAbility = AnimHash("ability");
constexpr uint32_t kAttrBlock = AnimHash("block");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrController14 = 0x3A5ABCBEu;  // NOTE: name unknown; restores the default controller
constexpr uint32_t kAttrController15 = 0x706ED613u;  // NOTE: name unknown

// Binocucom view types, by index (console table 0x00F558BC).
constexpr uint32_t kBinocucomTypes[] = {AnimHash("Normal"), AnimHash("Thermal")};
} // namespace

// ---------------------------------------------------------------------------
// "SetBinocucomLocked"

class SetBinocucomLockedCommand : public ApeAnimScriptCommand {
public:
    SetBinocucomLockedCommand();
    ~SetBinocucomLockedCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_locked = true;  // +0x0C "locked"
};

// @func 0x00087034 SetBinocucomLockedCommand::SetBinocucomLockedCommand
SetBinocucomLockedCommand::SetBinocucomLockedCommand() = default;

// @func 0x0073E138 SetBinocucomLockedCommand::~SetBinocucomLockedCommand
// @func 0x0073E168 SetBinocucomLockedCommand::~SetBinocucomLockedCommand (deleting)
SetBinocucomLockedCommand::~SetBinocucomLockedCommand() = default;

// @func 0x00087084 SetBinocucomLockedCommand::Create
ApeAnimScriptCommand* SetBinocucomLockedCommand::Create(XmlElement* element) const
{
    auto* command = new SetBinocucomLockedCommand();
    element->GetBool(kAttrLocked, &command->m_locked, true);
    return command;
}

// @func 0x00087130 SetBinocucomLockedCommand::Execute
void SetBinocucomLockedCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* player = PlayerEntity();
    if (!player || !EntityIsCharacter(player))
        return;
    if (void* binocucom = PlayerBinocucom(player))
        BinocucomSetLocked(binocucom, m_locked);
}

// ---------------------------------------------------------------------------
// Hash 0x5E895638 (name unknown): sets the binocucom power.

class SetBinocucomPowerCommand : public ApeAnimScriptCommand {
public:
    SetBinocucomPowerCommand();
    ~SetBinocucomPowerCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_power = -1.0f;  // +0x0C "Power" (default when parsed: 0)
};

// @func 0x000871B0 SetBinocucomPowerCommand::SetBinocucomPowerCommand
SetBinocucomPowerCommand::SetBinocucomPowerCommand() = default;

// @func 0x0073E1C0 SetBinocucomPowerCommand::~SetBinocucomPowerCommand
// @func 0x0073E1F0 SetBinocucomPowerCommand::~SetBinocucomPowerCommand (deleting)
SetBinocucomPowerCommand::~SetBinocucomPowerCommand() = default;

// @func 0x00087210 SetBinocucomPowerCommand::Create
ApeAnimScriptCommand* SetBinocucomPowerCommand::Create(XmlElement* element) const
{
    auto* command = new SetBinocucomPowerCommand();
    element->GetFloat(kAttrPower, &command->m_power, 0.0f);
    return command;
}

// @func 0x000872C0 SetBinocucomPowerCommand::Execute
void SetBinocucomPowerCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    BinocucomSetPower(m_power);
}

// ---------------------------------------------------------------------------
// Hash 0xB5BFB042 (name unknown): sets a global rate (console global 0x010592F4).

class SetGlobalRateCommand : public ApeAnimScriptCommand {
public:
    SetGlobalRateCommand();
    ~SetGlobalRateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_rate = -1.0f;  // +0x0C "Rate" (default when parsed: 0)
};

// @func 0x000872E8 SetGlobalRateCommand::SetGlobalRateCommand
SetGlobalRateCommand::SetGlobalRateCommand() = default;

// @func 0x0073E248 SetGlobalRateCommand::~SetGlobalRateCommand
// @func 0x0073E278 SetGlobalRateCommand::~SetGlobalRateCommand (deleting)
SetGlobalRateCommand::~SetGlobalRateCommand() = default;

// @func 0x00087348 SetGlobalRateCommand::Create
ApeAnimScriptCommand* SetGlobalRateCommand::Create(XmlElement* element) const
{
    auto* command = new SetGlobalRateCommand();
    element->GetFloat(kAttrRate, &command->m_rate, 0.0f);
    return command;
}

// @func 0x000873F8 SetGlobalRateCommand::Execute
void SetGlobalRateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SetGlobalRate010592F4(m_rate);
}

// ---------------------------------------------------------------------------
// Hash 0x060CB347 (name unknown): picks the binocucom view type.

class SetBinocucomTypeCommand : public ApeAnimScriptCommand {
public:
    SetBinocucomTypeCommand();
    ~SetBinocucomTypeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_type = 0;  // +0x0C "Type": "Normal" or "Thermal"
};

// @func 0x00087414 SetBinocucomTypeCommand::SetBinocucomTypeCommand
SetBinocucomTypeCommand::SetBinocucomTypeCommand() = default;

// @func 0x0008745C SetBinocucomTypeCommand::~SetBinocucomTypeCommand
// @func 0x0008748C SetBinocucomTypeCommand::~SetBinocucomTypeCommand (deleting)
SetBinocucomTypeCommand::~SetBinocucomTypeCommand() = default;

// @func 0x000874E4 SetBinocucomTypeCommand::Create
ApeAnimScriptCommand* SetBinocucomTypeCommand::Create(XmlElement* element) const
{
    auto* command = new SetBinocucomTypeCommand();
    element->GetHash(kAttrType, &command->m_type, 0);
    return command;
}

// @func 0x00087588 SetBinocucomTypeCommand::Execute
void SetBinocucomTypeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* player = PlayerEntity();
    void* binocucom = player ? PlayerBinocucom(player) : nullptr;
    if (!binocucom)
        return;
    for (int i = 0; i < static_cast<int>(sizeof(kBinocucomTypes) / sizeof(kBinocucomTypes[0])); ++i) {
        if (kBinocucomTypes[i] == m_type) {
            BinocucomSetViewType(binocucom, i);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Hash 0xA58714CA (name unknown): names the script the binocucom runs.

class SetBinocucomScriptCommand : public ApeAnimScriptCommand {
public:
    SetBinocucomScriptCommand();
    ~SetBinocucomScriptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_scriptName = 0;  // +0x0C "ScriptName"
};

// @func 0x0008762C SetBinocucomScriptCommand::SetBinocucomScriptCommand
SetBinocucomScriptCommand::SetBinocucomScriptCommand() = default;

// @func 0x0008767C SetBinocucomScriptCommand::~SetBinocucomScriptCommand
// @func 0x000876AC SetBinocucomScriptCommand::~SetBinocucomScriptCommand (deleting)
SetBinocucomScriptCommand::~SetBinocucomScriptCommand() = default;

// @func 0x00087704 SetBinocucomScriptCommand::Create
ApeAnimScriptCommand* SetBinocucomScriptCommand::Create(XmlElement* element) const
{
    auto* command = new SetBinocucomScriptCommand();
    element->GetHash(kAttrScriptName, &command->m_scriptName, 0);
    return command;
}

// @func 0x000877AC SetBinocucomScriptCommand::Execute
void SetBinocucomScriptCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (PlayerEntity())
        BinocucomSetScript(m_scriptName);
}

// ---------------------------------------------------------------------------
// "SetBlendAlpha": sets the alpha of a render object.

class SetBlendAlphaCommand : public ApeAnimScriptCommand {
public:
    SetBlendAlphaCommand();
    ~SetBlendAlphaCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object" (object variable)
    uint32_t m_alphaVar = 0;  // +0x10 "AlphaVar"
    float m_alpha = 1.0f;     // +0x14 "Alpha"
    bool m_flag18 = false;    // +0x18 attribute 0xF5897C08
};

// @func 0x00086A34 SetBlendAlphaCommand::SetBlendAlphaCommand
SetBlendAlphaCommand::SetBlendAlphaCommand() = default;

// @func 0x00086A8C SetBlendAlphaCommand::~SetBlendAlphaCommand
// @func 0x00086ABC SetBlendAlphaCommand::~SetBlendAlphaCommand (deleting)
SetBlendAlphaCommand::~SetBlendAlphaCommand() = default;

// @func 0x00086B18 SetBlendAlphaCommand::Create
ApeAnimScriptCommand* SetBlendAlphaCommand::Create(XmlElement* element) const
{
    auto* command = new SetBlendAlphaCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetFloat(kAttrAlpha, &command->m_alpha, 1.0f);
        element->GetHash(kAttrAlphaVar, &command->m_alphaVar, 0);
        element->GetBool(kAttrBlend18, &command->m_flag18, false);
    }
    return command;
}

// @func 0x00086C58 SetBlendAlphaCommand::Execute
void SetBlendAlphaCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeRefCounted* object = ObjectFromVar(instance, m_object);
    float alpha = ResolveFloat(instance, m_alphaVar, m_alpha);
    if (object && ObjectIsRenderInstance(object))
        RenderInstanceSetAlpha(object, static_cast<uint8_t>(static_cast<int>(alpha * 255.0f)), m_flag18);
}

// ---------------------------------------------------------------------------
// Hash 0x4023B405 (name unknown): blocks or enables a character ability.

class SetAbilityCommand : public ApeAnimScriptCommand {
public:
    SetAbilityCommand();
    ~SetAbilityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_ability = 0;    // +0x14 "ability"
    bool m_block = false;      // +0x18 "block"
    bool m_hasBlock = false;   // +0x19
    bool m_enable = false;     // +0x1A "enable"
    bool m_hasEnable = false;  // +0x1B
};

// @func 0x000877F0 SetAbilityCommand::SetAbilityCommand
SetAbilityCommand::SetAbilityCommand() = default;

// @func 0x00087850 SetAbilityCommand::~SetAbilityCommand
// @func 0x00087880 SetAbilityCommand::~SetAbilityCommand (deleting)
SetAbilityCommand::~SetAbilityCommand() = default;

// @func 0x000878D8 SetAbilityCommand::Create
ApeAnimScriptCommand* SetAbilityCommand::Create(XmlElement* element) const
{
    auto* command = new SetAbilityCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrAbility, &command->m_ability, 0);
    if (element->GetBool(kAttrBlock, &command->m_block, false) == ape::xml::kQuery_Ok)
        command->m_hasBlock = true;
    if (element->GetBool(kAttrEnable, &command->m_enable, false) == ape::xml::kQuery_Ok)
        command->m_hasEnable = true;
    return command;
}

// @func 0x00087A40 SetAbilityCommand::Execute
void SetAbilityCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (!m_hasBlock && !m_hasEnable)
        return;
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity || !EntityIsCharacter(entity))
        return;
    int ability = CharacterFindAbility(m_ability);
    if (ability < 0)
        return;
    if (m_hasBlock)
        CharacterSetAbilityBlocked(entity, ability, m_block);
    else
        CharacterSetAbilityEnabled(entity, ability, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0xE39BD656 (name unknown): resets and enables a character's controller.

class SetControllerCommand : public ApeAnimScriptCommand {
public:
    SetControllerCommand();
    ~SetControllerCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;           // +0x0C "object"
    uint32_t m_entity = 0;           // +0x10 "Entity"
    bool m_restoreDefault = false;   // +0x14 attribute 0x3A5ABCBE
    bool m_flag15 = false;           // +0x15 attribute 0x706ED613
    bool m_enable = false;           // +0x16 "enable"
};

// @func 0x00087C14 SetControllerCommand::SetControllerCommand
SetControllerCommand::SetControllerCommand() = default;

// @func 0x00087C74 SetControllerCommand::~SetControllerCommand
// @func 0x00087CA4 SetControllerCommand::~SetControllerCommand (deleting)
SetControllerCommand::~SetControllerCommand() = default;

// @func 0x00087CFC SetControllerCommand::Create
ApeAnimScriptCommand* SetControllerCommand::Create(XmlElement* element) const
{
    auto* command = new SetControllerCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetBool(kAttrController14, &command->m_restoreDefault, false);
    element->GetBool(kAttrController15, &command->m_flag15, false);
    element->GetBool(kAttrEnable, &command->m_enable, false);
    return command;
}

// @func 0x00087E34 SetControllerCommand::Execute
void SetControllerCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (!m_restoreDefault && !m_flag15)
        return;
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity || !EntityIsCharacter(entity))
        return;
    if (m_restoreDefault)
        CharacterRestoreDefaultController(entity);
    else
        CharacterResetController(entity);
    CharacterEnableController(entity, m_enable);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsN()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SetBinocucomLocked"), new SetBinocucomLockedCommand());
    prototypes.Add(0x5E895638u, new SetBinocucomPowerCommand());
    prototypes.Add(0xB5BFB042u, new SetGlobalRateCommand());
    prototypes.Add(0x060CB347u, new SetBinocucomTypeCommand());
    prototypes.Add(0xA58714CAu, new SetBinocucomScriptCommand());
    prototypes.Add(AnimHash("SetBlendAlpha"), new SetBlendAlphaCommand());
    prototypes.Add(0x4023B405u, new SetAbilityCommand());
    prototypes.Add(0xE39BD656u, new SetControllerCommand());
}

} // namespace levels
} // namespace sly
