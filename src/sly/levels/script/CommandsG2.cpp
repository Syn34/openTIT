// openTIT - Game script commands, fifth block (game state queries Get...).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/Upgrades.h"
#include "sly/levels/GameServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrStateVar = AnimHash("StateVar");
constexpr uint32_t kAttrFxType = AnimHash("FxType");
constexpr uint32_t kAttrFxD419 = 0xD4196802u;  // NOTE: name unknown
constexpr uint32_t kAttrTextOnce = 0x7416F03Bu; // NOTE: name unknown
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrFlag = AnimHash("flag");
constexpr uint32_t kAttrPlayerIndex = AnimHash("PlayerIndex");
constexpr uint32_t kAttrMatrix = AnimHash("matrix");
constexpr uint32_t kAttrStateNVar[4] = {
    AnimHash("State0Var"), AnimHash("State1Var"), AnimHash("State2Var"), AnimHash("State3Var"),
};

// Stores `value` in the int variable `varHash` (other types are left alone).
void StoreInt(ApeAnimScriptInstance* instance, uint32_t varHash, int32_t value)
{
    if (varHash == 0)
        return;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (var && var->GetType() == AnimScriptVarType::Int)
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(value);
}
} // namespace

// Entity flag names accepted by the Get/SetEntityFlag commands (table
// 0x00AC4C28: name hash, flag bit). Unresolved names are kept as hashes.
const EntityFlagName kEntityFlagNames[kEntityFlagNameCount] = {
    {AnimHash("player"), 0x00000001u},       {AnimHash("Strafe"), 0x00000002u},
    {AnimHash("Combat"), 0x00000004u},       {AnimHash("Sneak"), 0x00000008u},
    {AnimHash("Crawl"), 0x00000010u},        {AnimHash("Disguise"), 0x00000020u},
    {0x8CFF6308u, 0x00000040u},              {0x7A7C6303u, 0x00000080u},
    {0x9EB99829u, 0x00000100u},              {AnimHash("SuperSpire"), 0x00000200u},
    {AnimHash("SuperGrind"), 0x00000400u},   {AnimHash("SuperSpring"), 0x00000800u},
    {AnimHash("SuperTrack"), 0x00001000u},   {0x76DAA94Du, 0x00004000u},
    {AnimHash("Carry"), 0x00008000u},        {0xA8C3D7DAu, 0x00010000u},
    {AnimHash("Invulnerable"), 0x00020000u}, {0xDA743E78u, 0x00040000u},
    {0x7313D07Du, 0x00080000u},              {AnimHash("Glide"), 0x00100000u},
    {AnimHash("Hover"), 0x00200000u},        {AnimHash("HealthMeter"), 0x00400000u},
    {0xCA4460B7u, 0x00800000u},              {AnimHash("PlayDead"), 0x01000000u},
    {0x29140264u, 0x02000000u},              {AnimHash("WallClimb"), 0x04000000u},
    {0x0C9EDD8Au, 0x08000000u},              {0x9A1B0741u, 0x10000000u},
    {0xA99C4202u, 0x20000000u},              {AnimHash("IncreaseAutoCollectDist"), 0x40000000u},
    {AnimHash("Smoke"), 0x80000000u},
};

uint32_t LookupEntityFlag(uint32_t nameHash)
{
    for (const EntityFlagName& entry : kEntityFlagNames) {
        if (entry.nameHash == nameHash)
            return entry.flag;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Queries of the game world's option bits (world bytes +0x2C/+0x2D). The
// command names were not recovered.

// Hash 0x98474D43: StateVar = bit 4 of world byte +0x2D.
class GetWorldBit2D4Command : public ApeAnimScriptCommand {
public:
    GetWorldBit2D4Command();
    ~GetWorldBit2D4Command() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_stateVar = 0;  // +0x0C "StateVar"
};

// @func 0x0007B760 GetWorldBit2D4Command::GetWorldBit2D4Command
GetWorldBit2D4Command::GetWorldBit2D4Command() = default;

// @func 0x0007B7A8 GetWorldBit2D4Command::~GetWorldBit2D4Command
// @func 0x0007B7D8 GetWorldBit2D4Command::~GetWorldBit2D4Command (deleting)
GetWorldBit2D4Command::~GetWorldBit2D4Command() = default;

// @func 0x0007B830 GetWorldBit2D4Command::Create
ApeAnimScriptCommand* GetWorldBit2D4Command::Create(XmlElement* element) const
{
    auto* command = new GetWorldBit2D4Command();
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// @func 0x0007B8D4 GetWorldBit2D4Command::Execute
void GetWorldBit2D4Command::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    StoreInt(instance, m_stateVar, (GameWorldFlags2D() >> 4) & 1);
}

// Hash 0xE5B13789: StateVar = bit 0 of world byte +0x2C.
class GetWorldBit2C0Command : public ApeAnimScriptCommand {
public:
    GetWorldBit2C0Command();
    ~GetWorldBit2C0Command() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_stateVar = 0;  // +0x0C "StateVar"
};

// @func 0x0007B218 GetWorldBit2C0Command::GetWorldBit2C0Command
GetWorldBit2C0Command::GetWorldBit2C0Command() = default;

// @func 0x0007B260 GetWorldBit2C0Command::~GetWorldBit2C0Command
// @func 0x0007B290 GetWorldBit2C0Command::~GetWorldBit2C0Command (deleting)
GetWorldBit2C0Command::~GetWorldBit2C0Command() = default;

// @func 0x0007B2E8 GetWorldBit2C0Command::Create
ApeAnimScriptCommand* GetWorldBit2C0Command::Create(XmlElement* element) const
{
    auto* command = new GetWorldBit2C0Command();
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// @func 0x0007B38C GetWorldBit2C0Command::Execute
void GetWorldBit2C0Command::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    StoreInt(instance, m_stateVar, GameWorldFlags2C() & 1);
}

// Hash 0x9C86E926: StateVar = bit 1 of world byte +0x2C.
class GetWorldBit2C1Command : public ApeAnimScriptCommand {
public:
    GetWorldBit2C1Command();
    ~GetWorldBit2C1Command() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_stateVar = 0;  // +0x0C "StateVar"
};

// @func 0x0007C4FC GetWorldBit2C1Command::GetWorldBit2C1Command
GetWorldBit2C1Command::GetWorldBit2C1Command() = default;

// @func 0x0007C544 GetWorldBit2C1Command::~GetWorldBit2C1Command
// @func 0x0007C574 GetWorldBit2C1Command::~GetWorldBit2C1Command (deleting)
GetWorldBit2C1Command::~GetWorldBit2C1Command() = default;

// @func 0x0007C5CC GetWorldBit2C1Command::Create
ApeAnimScriptCommand* GetWorldBit2C1Command::Create(XmlElement* element) const
{
    auto* command = new GetWorldBit2C1Command();
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// @func 0x0007C670 GetWorldBit2C1Command::Execute
void GetWorldBit2C1Command::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    StoreInt(instance, m_stateVar, (GameWorldFlags2C() >> 1) & 1);
}

// "GetSafeState" (hash 0xCFC9EE8E; NOTE: name from a hash match).
class GetSafeStateCommand : public ApeAnimScriptCommand {
public:
    GetSafeStateCommand();
    ~GetSafeStateCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_stateVar = 0;  // +0x0C "StateVar"
};

// @func 0x0007C298 GetSafeStateCommand::GetSafeStateCommand
GetSafeStateCommand::GetSafeStateCommand() = default;

// @func 0x0007C2E0 GetSafeStateCommand::~GetSafeStateCommand
// @func 0x0007C310 GetSafeStateCommand::~GetSafeStateCommand (deleting)
GetSafeStateCommand::~GetSafeStateCommand() = default;

// @func 0x0007C368 GetSafeStateCommand::Create
ApeAnimScriptCommand* GetSafeStateCommand::Create(XmlElement* element) const
{
    auto* command = new GetSafeStateCommand();
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// TODO(0x0007C40C GetSafeStateCommand::Execute)

// ---------------------------------------------------------------------------
// Hash 0xC010ED68 (name unknown): an effect type plus one more name.

class FxTypeQueryCommand : public ApeAnimScriptCommand {
public:
    FxTypeQueryCommand();
    ~FxTypeQueryCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_fxType = 0;   // +0x0C "FxType"
    uint32_t m_field10 = 0;  // +0x10 attribute 0xD4196802
};

// @func 0x0007B974 FxTypeQueryCommand::FxTypeQueryCommand
FxTypeQueryCommand::FxTypeQueryCommand() = default;

// @func 0x0007B9C4 FxTypeQueryCommand::~FxTypeQueryCommand
// @func 0x0007B9F4 FxTypeQueryCommand::~FxTypeQueryCommand (deleting)
FxTypeQueryCommand::~FxTypeQueryCommand() = default;

// @func 0x0007BA4C FxTypeQueryCommand::Create
ApeAnimScriptCommand* FxTypeQueryCommand::Create(XmlElement* element) const
{
    auto* command = new FxTypeQueryCommand();
    element->GetHash(kAttrFxType, &command->m_fxType, 0);
    element->GetHash(kAttrFxD419, &command->m_field10, 0);
    return command;
}

// TODO(0x0007BB18 FxTypeQueryCommand::Execute)

// ---------------------------------------------------------------------------
// Hash 0xF77517AF (name unknown): shows a text message from the script.

class ShowTextCommand : public ApeAnimScriptCommand {
public:
    ShowTextCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    TextField m_text;        // +0x0C first child element the text parser accepts
    bool m_once = false;     // +0x58 attribute 0x7416F03B
};

// @func 0x0007AD08 ShowTextCommand::ShowTextCommand
ShowTextCommand::ShowTextCommand()
{
    m_text.Clear();
}

// @func 0x0007AD68 ShowTextCommand::Create
ApeAnimScriptCommand* ShowTextCommand::Create(XmlElement* element) const
{
    auto* command = new ShowTextCommand();
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (TextFieldParse(child, &command->m_text))
            break;
    }
    element->GetBool(kAttrTextOnce, &command->m_once, command->m_once);
    return command;
}

// @func 0x0007AE74 ShowTextCommand::Execute
void ShowTextCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (!m_text.flagA || !TextDisplayAvailable())
        return;
    TextDisplayShow(m_text);
    if (!m_once || m_text.flagB)
        TextFieldNotify(instance, m_text);
}

// ---------------------------------------------------------------------------
// GetEntityFlag (registered with hash 0xD8D02D85; its default constructor is
// outside this module's range).

class GetEntityFlagCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    uint32_t m_flag = 0;      // +0x14 "flag" (translated to the flag bit)
    uint32_t m_stateVar = 0;  // +0x18 "StateVar"
};

// @func 0x0007AF38 GetEntityFlagCommand::Create
ApeAnimScriptCommand* GetEntityFlagCommand::Create(XmlElement* element) const
{
    auto* command = new GetEntityFlagCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    uint32_t flagName = 0;
    if (element->GetHash(kAttrFlag, &flagName, 0) == ape::xml::kQuery_Ok) {
        if (uint32_t flag = LookupEntityFlag(flagName))
            command->m_flag = flag;
        if (command->m_flag == 0)
            ScriptWarning("AnimScriptCommandGetEntityFlag: Unrecognized flag specifier '%s'.", flagName);
    }
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// TODO(0x0007B0CC GetEntityFlagCommand::Execute)

// ---------------------------------------------------------------------------
// "GetRespawnInfo" (hash 0x8D10E0C0; NOTE: name from a hash match)

class GetRespawnInfoCommand : public ApeAnimScriptCommand {
public:
    GetRespawnInfoCommand();
    ~GetRespawnInfoCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;          // +0x0C "object"
    uint32_t m_entity = 0;          // +0x10 "Entity"
    int32_t m_playerIndex = -1;     // +0x14 "PlayerIndex"
    uint32_t m_matrix = 0;          // +0x18 "matrix": matrix variable receiving the respawn point
    uint32_t m_stateVars[4] = {};   // +0x1C "State0Var".."State3Var"
};

// @func 0x0007BC80 GetRespawnInfoCommand::GetRespawnInfoCommand
GetRespawnInfoCommand::GetRespawnInfoCommand() = default;

// @func 0x0007BCE8 GetRespawnInfoCommand::~GetRespawnInfoCommand
// @func 0x0007BD18 GetRespawnInfoCommand::~GetRespawnInfoCommand (deleting)
GetRespawnInfoCommand::~GetRespawnInfoCommand() = default;

// @func 0x0007BD70 GetRespawnInfoCommand::Create
ApeAnimScriptCommand* GetRespawnInfoCommand::Create(XmlElement* element) const
{
    auto* command = new GetRespawnInfoCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrEntity, &command->m_entity, 0) == ape::xml::kQuery_Ok ||
        element->GetInt(kAttrPlayerIndex, &command->m_playerIndex, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrMatrix, &command->m_matrix, 0);
        for (int i = 0; i < 4; ++i)
            element->GetHash(kAttrStateNVar[i], &command->m_stateVars[i], 0);
    }
    return command;
}

// TODO(0x0007BF3C GetRespawnInfoCommand::Execute)

// ---------------------------------------------------------------------------

void RegisterScriptCommandsG2()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0x98474D43u, new GetWorldBit2D4Command());
    prototypes.Add(0xC010ED68u, new FxTypeQueryCommand());
    prototypes.Add(0xF77517AFu, new ShowTextCommand());
    prototypes.Add(0xD8D02D85u, new GetEntityFlagCommand());
    prototypes.Add(0xE5B13789u, new GetWorldBit2C0Command());
    prototypes.Add(AnimHash("GetRespawnInfo"), new GetRespawnInfoCommand());
    prototypes.Add(AnimHash("GetSafeState"), new GetSafeStateCommand());
    prototypes.Add(0x9C86E926u, new GetWorldBit2C1Command());
}

} // namespace levels
} // namespace sly
