// openTIT - Game script commands, twenty-first block (SwitchStatementEntity to the object flag command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

#include <algorithm>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kElementCase = AnimHash("Case");
constexpr uint32_t kElementDefault = AnimHash("Default");
constexpr uint32_t kAttrEntityDef = AnimHash("EntityDef");
constexpr uint32_t kAttrEntityInst = AnimHash("EntityInst");
constexpr uint32_t kAttrSpawn10 = 0x31B11071u;   // NOTE: names unknown for the attributes of 0x986C30AA
constexpr uint32_t kAttrSpawn20 = 0xFE747351u;
constexpr uint32_t kAttrSpawn24 = 0x37E35A18u;
constexpr uint32_t kAttrSpawnerEntity = AnimHash("SpawnerEntity");
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrSpawn30 = 0x09CFA52Du;
constexpr uint32_t kAttrSpawn34 = 0x4F479F4Eu;
constexpr uint32_t kAttrSpawn38 = 0xBBEFE7FCu;
constexpr uint32_t kAttrSpawn3C = 0xC8A764AAu;
constexpr uint32_t kAttrSpawn3D = 0x6DBD11CCu;
constexpr uint32_t kAttrSpawn3E = 0x771AFC9Fu;
constexpr uint32_t kAttrThrowDir = AnimHash("ThrowDir");
constexpr uint32_t kAttrThrowPower = AnimHash("ThrowPower");
constexpr uint32_t kAttrPlayerIndex = AnimHash("PlayerIndex");
constexpr uint32_t kAttrParentObject = AnimHash("ParentObject");
constexpr uint32_t kAttrThrow2C = 0xAA2C436Fu;   // NOTE: name unknown
constexpr uint32_t kAttrThrow2D = 0x63FE154Eu;   // NOTE: name unknown
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrObjectHash = AnimHash("ObjectHash");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrRecursive = 0xF5897C08u; // NOTE: name unknown; also walks the children

const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};             // NOTE: console global 0x01164B40
const ape::Vec3 kDefaultThrowDir = {0.0f, 0.0f, 1.0f, 0.0f};  // NOTE: console global 0x01164B70, assumed +Z

// World objects of kind 10 carry a flag byte (+0xD0 -> +0x70 -> +0x45).
constexpr uint8_t kObjectFlag45Enabled = 0x20;
} // namespace

// ---------------------------------------------------------------------------
// "SwitchStatementEntity": runs the commands of the first "Case" whose
// EntityDef or EntityInst matches the entity, else the "Default" commands.

class SwitchStatementEntityCommand : public ApeAnimScriptCommand {
public:
    static constexpr int kMaxCases = 4;

    SwitchStatementEntityCommand();
    explicit SwitchStatementEntityCommand(XmlElement* element);
    ~SwitchStatementEntityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    XmlElement* CreateDebugElement() const override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    struct Case {
        ape::AnimScriptCommandList commands;  // +0x00 (array of 4 initially)
        uint32_t entityDef = 0;               // +0x14 "EntityDef"
        uint32_t entityInst = 0;              // +0x18 "EntityInst"
    };

    // Creates the commands among the children of `element` into an empty
    // list; false when there are none (0x0009810C).
    static bool LoadCaseCommands(XmlElement* element, ape::AnimScriptCommandList& list);

    Case m_cases[kMaxCases];                  // +0x0C, 0x1C bytes each
    uint32_t m_caseCount = 0;                 // +0x7C
    uint32_t m_object = 0;                    // +0x80 "object"
    uint32_t m_entity = 0;                    // +0x84 "Entity"
    ape::AnimScriptCommandList m_default;     // +0x88 "Default" commands
};

// @func 0x00098044 SwitchStatementEntityCommand::SwitchStatementEntityCommand
SwitchStatementEntityCommand::SwitchStatementEntityCommand() = default;

// @func 0x0009810C SwitchStatementEntityCommand::LoadCaseCommands
bool SwitchStatementEntityCommand::LoadCaseCommands(XmlElement* element, ape::AnimScriptCommandList& list)
{
    if (!element || !list.empty())
        return false;
    size_t count = 0;
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (ape::IsCommandElement(child))
            ++count;
    }
    if (count == 0)
        return false;
    list.reserve(count);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!ape::IsCommandElement(child))
            continue;
        if (ApeAnimScriptCommand* command = ape::CreateCommand(child))
            list.push_back(command);
    }
    return true;
}

// @func 0x0009829C SwitchStatementEntityCommand::SwitchStatementEntityCommand
SwitchStatementEntityCommand::SwitchStatementEntityCommand(XmlElement* element)
{
    element->GetHash(kAttrObject, &m_object, 0);
    element->GetHash(kAttrEntity, &m_entity, 0);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t name = child->GetNameHash();
        if (name == kElementDefault) {
            if (m_default.empty())
                LoadCaseCommands(child, m_default);
        } else if (name == kElementCase && m_caseCount < kMaxCases) {
            Case& entry = m_cases[m_caseCount];
            child->GetHash(kAttrEntityDef, &entry.entityDef, 0);
            child->GetHash(kAttrEntityInst, &entry.entityInst, 0);
            if ((entry.entityDef != 0 || entry.entityInst != 0) && LoadCaseCommands(child, entry.commands))
                ++m_caseCount;
        }
    }
}

// @func 0x00098500 SwitchStatementEntityCommand::~SwitchStatementEntityCommand
// @func 0x000986C0 SwitchStatementEntityCommand::~SwitchStatementEntityCommand (deleting)
SwitchStatementEntityCommand::~SwitchStatementEntityCommand()
{
    for (uint32_t i = 0; i < m_caseCount; ++i) {
        for (ApeAnimScriptCommand* command : m_cases[i].commands) {
            if (command)
                command->Release();
        }
        m_cases[i].commands.clear();
    }
    for (ApeAnimScriptCommand* command : m_default) {
        if (command)
            command->Release();
    }
    m_default.clear();
}

// @func 0x0009870C SwitchStatementEntityCommand::Link
void SwitchStatementEntityCommand::Link()
{
    for (uint32_t i = 0; i < m_caseCount; ++i) {
        for (ApeAnimScriptCommand* command : m_cases[i].commands) {
            if (command)
                command->Link();
        }
    }
    for (ApeAnimScriptCommand* command : m_default) {
        if (command)
            command->Link();
    }
}

// @func 0x0009884C SwitchStatementEntityCommand::Create
ApeAnimScriptCommand* SwitchStatementEntityCommand::Create(XmlElement* element) const
{
    return new SwitchStatementEntityCommand(element);
}

// @func 0x000988C0 SwitchStatementEntityCommand::Execute
void SwitchStatementEntityCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (WorldObject* entity = ResolveEntity(instance, m_object, m_entity)) {
        const uint32_t defName = EntityDefName(entity);
        const uint32_t instName = EntityName(entity);
        for (uint32_t i = 0; i < m_caseCount; ++i) {
            const Case& entry = m_cases[i];
            if ((entry.entityDef != 0 && entry.entityDef == defName) ||
                (entry.entityInst != 0 && entry.entityInst == instName)) {
                for (ApeAnimScriptCommand* command : entry.commands) {
                    if (command)
                        command->Execute(instance, state);
                }
                return;
            }
        }
    }
    for (ApeAnimScriptCommand* command : m_default) {
        if (command)
            command->Execute(instance, state);
    }
}

// @func 0x00098AE8 SwitchStatementEntityCommand::CreateDebugElement
XmlElement* SwitchStatementEntityCommand::CreateDebugElement() const
{
    return NewDebugElement("SwitchStatementEntity");
}

// ---------------------------------------------------------------------------
// Hash 0x986C30AA (name unknown): spawner request with counts.

class SpawnRequestCommand : public ApeAnimScriptCommand {
public:
    SpawnRequestCommand();
    explicit SpawnRequestCommand(XmlElement* element);
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::Vec3 m_vector10 = kZero;   // +0x10 attribute 0x31B11071
    uint32_t m_hash20 = 0;          // +0x20 attribute 0xFE747351
    uint32_t m_hash24 = 0;          // +0x24 attribute 0x37E35A18
    uint32_t m_spawnerEntity = 0;   // +0x28 "SpawnerEntity"
    int32_t m_playerIdx = 0;        // +0x2C "PlayerIdx" (1-based in data)
    uint32_t m_hash30 = 0;          // +0x30 attribute 0x09CFA52D
    int32_t m_count34 = 0;          // +0x34 attribute 0x4F479F4E (at most +0x38)
    int32_t m_count38 = 0;          // +0x38 attribute 0xBBEFE7FC
    bool m_flag3C = true;           // +0x3C attribute 0xC8A764AA
    bool m_flag3D = false;          // +0x3D attribute 0x6DBD11CC
    bool m_flag3E = false;          // +0x3E attribute 0x771AFC9F
};

// @func 0x00098B4C SpawnRequestCommand::SpawnRequestCommand
SpawnRequestCommand::SpawnRequestCommand() = default;

// @func 0x00098BE4 SpawnRequestCommand::SpawnRequestCommand
SpawnRequestCommand::SpawnRequestCommand(XmlElement* element)
{
    element->GetVec3(kAttrSpawn10, &m_vector10.x, &m_vector10.x);
    element->GetHash(kAttrSpawn20, &m_hash20, m_hash20);
    element->GetHash(kAttrSpawn24, &m_hash24, m_hash24);
    element->GetHash(kAttrSpawnerEntity, &m_spawnerEntity, m_spawnerEntity);
    if (element->GetInt(kAttrPlayerIdx, &m_playerIdx, m_playerIdx) == ape::xml::kQuery_Ok && m_playerIdx != 0)
        --m_playerIdx;
    element->GetHash(kAttrSpawn30, &m_hash30, m_hash30);
    element->GetInt(kAttrSpawn34, &m_count34, m_count34);
    element->GetInt(kAttrSpawn38, &m_count38, m_count38);
    element->GetBool(kAttrSpawn3C, &m_flag3C, m_flag3C);
    element->GetBool(kAttrSpawn3D, &m_flag3D, m_flag3D);
    element->GetBool(kAttrSpawn3E, &m_flag3E, m_flag3E);
    m_count34 = std::min(m_count34, m_count38);
}

// @func 0x00098E34 SpawnRequestCommand::Create
ApeAnimScriptCommand* SpawnRequestCommand::Create(XmlElement* element) const
{
    return new SpawnRequestCommand(element);
}

// TODO(0x00098EA4 SpawnRequestCommand::Execute) spawner lookup and spawn request
// (0x0048E360 / 0x0048ED50).

// ---------------------------------------------------------------------------
// "ThrowPlayer" (NOTE: name from a hash match)

class ThrowPlayerCommand : public ApeAnimScriptCommand {
public:
    ThrowPlayerCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::Vec3 m_throwDir = kDefaultThrowDir;  // +0x10 "ThrowDir"
    float m_throwPower = 1.0f;                // +0x20 "ThrowPower"
    int32_t m_playerIndex = -1;               // +0x24 "PlayerIndex"
    uint32_t m_parentObject = 0;              // +0x28 "ParentObject"
    bool m_flag2C = false;                    // +0x2C attribute 0xAA2C436F
    bool m_flag2D = false;                    // +0x2D attribute 0x63FE154E
};

// @func 0x00099074 ThrowPlayerCommand::ThrowPlayerCommand
ThrowPlayerCommand::ThrowPlayerCommand() = default;

// @func 0x00099100 ThrowPlayerCommand::Create
ApeAnimScriptCommand* ThrowPlayerCommand::Create(XmlElement* element) const
{
    auto* command = new ThrowPlayerCommand();
    element->GetVec3(kAttrThrowDir, &command->m_throwDir.x, &command->m_throwDir.x);
    element->GetFloat(kAttrThrowPower, &command->m_throwPower, 1.0f);
    element->GetInt(kAttrPlayerIndex, &command->m_playerIndex, -1);
    element->GetHash(kAttrParentObject, &command->m_parentObject, 0);
    element->GetBool(kAttrThrow2C, &command->m_flag2C, false);
    element->GetBool(kAttrThrow2D, &command->m_flag2D, false);
    return command;
}

// TODO(0x000992D8 ThrowPlayerCommand::Execute) throws the player (0x00488004 / 0x00488078).

// ---------------------------------------------------------------------------
// "UnlockDisguise" (NOTE: name from a hash match)

class UnlockDisguiseCommand : public ApeAnimScriptCommand {
public:
    UnlockDisguiseCommand();
    ~UnlockDisguiseCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x00099510 UnlockDisguiseCommand::UnlockDisguiseCommand
UnlockDisguiseCommand::UnlockDisguiseCommand() = default;

// @func 0x00099550 UnlockDisguiseCommand::~UnlockDisguiseCommand
// @func 0x00099580 UnlockDisguiseCommand::~UnlockDisguiseCommand (deleting)
UnlockDisguiseCommand::~UnlockDisguiseCommand() = default;

// @func 0x000995D8 UnlockDisguiseCommand::Create
ApeAnimScriptCommand* UnlockDisguiseCommand::Create(XmlElement*) const
{
    return new UnlockDisguiseCommand();
}

// @func 0x00099658 UnlockDisguiseCommand::Execute
void UnlockDisguiseCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameStateUnlockDisguise(true);
}

// ---------------------------------------------------------------------------
// "UnlockMovie" (NOTE: name from a hash match)

class UnlockMovieCommand : public ApeAnimScriptCommand {
public:
    UnlockMovieCommand();
    ~UnlockMovieCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;  // +0x0C "name"
};

// @func 0x00099694 UnlockMovieCommand::UnlockMovieCommand
UnlockMovieCommand::UnlockMovieCommand() = default;

// @func 0x000996DC UnlockMovieCommand::~UnlockMovieCommand
// @func 0x0009970C UnlockMovieCommand::~UnlockMovieCommand (deleting)
UnlockMovieCommand::~UnlockMovieCommand() = default;

// @func 0x00099764 UnlockMovieCommand::Create
ApeAnimScriptCommand* UnlockMovieCommand::Create(XmlElement* element) const
{
    auto* command = new UnlockMovieCommand();
    element->GetHash(kAttrName, &command->m_name, 0);
    return command;
}

// @func 0x00099808 UnlockMovieCommand::Execute
void UnlockMovieCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_name != 0)
        GameStateUnlockMovie(m_name);
}

// ---------------------------------------------------------------------------
// Hash 0xE41C7FAC (name unknown): sets a flag of a world object (and,
// optionally, of its direct children).

class ObjectFlagCommand : public ApeAnimScriptCommand {
public:
    ObjectFlagCommand();
    ~ObjectFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object" (or "ObjectHash" as a number)
    bool m_enable = false;     // +0x10 "enable"
    bool m_recursive = false;  // +0x11 attribute 0xF5897C08
};

// @func 0x00099844 ObjectFlagCommand::ObjectFlagCommand
ObjectFlagCommand::ObjectFlagCommand() = default;

// @func 0x0009989C ObjectFlagCommand::~ObjectFlagCommand
// @func 0x000998CC ObjectFlagCommand::~ObjectFlagCommand (deleting)
ObjectFlagCommand::~ObjectFlagCommand() = default;

// @func 0x00099924 ObjectFlagCommand::Create
ApeAnimScriptCommand* ObjectFlagCommand::Create(XmlElement* element) const
{
    auto* command = new ObjectFlagCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) != ape::xml::kQuery_Ok) {
        int32_t hash = 0;
        element->GetInt(kAttrObjectHash, &hash, 0);
        command->m_object = static_cast<uint32_t>(hash);
    }
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetBool(kAttrRecursive, &command->m_recursive, false);
    return command;
}

// @func 0x00099A44 ObjectFlagCommand::Execute
void ObjectFlagCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    const uint8_t value = m_enable ? kObjectFlag45Enabled : 0;
    ape::ApeRefCounted* object = instance->ResolveObject(m_object, false);
    bool first = true;
    while (object) {
        ObjectSetFlag45(object, value);
        if (!m_recursive)
            break;
        // The object itself, then its first child and that child's siblings.
        object = first ? ObjectFirstChild(object) : ObjectNextSibling(object);
        first = false;
    }
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsW()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SwitchStatementEntity"), new SwitchStatementEntityCommand());
    prototypes.Add(0x986C30AAu, new SpawnRequestCommand());
    prototypes.Add(AnimHash("ThrowPlayer"), new ThrowPlayerCommand());
    prototypes.Add(AnimHash("UnlockDisguise"), new UnlockDisguiseCommand());
    prototypes.Add(AnimHash("UnlockMovie"), new UnlockMovieCommand());
    prototypes.Add(0xE41C7FACu, new ObjectFlagCommand());
}

} // namespace levels
} // namespace sly
