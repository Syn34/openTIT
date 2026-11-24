// openTIT - Game script commands that send game events (SendEvent, BroadcastEvent).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kSelf = AnimHash("this");
constexpr uint32_t kAttrSender = AnimHash("Sender");
constexpr uint32_t kAttrReceiver = 0xD6F981F9u;       // NOTE: name unknown
constexpr uint32_t kAttrReceiverChild = 0x6A87CEA3u;  // NOTE: name unknown
constexpr uint32_t kAttrEvent = AnimHash("Event");
constexpr uint32_t kAttrDataType = AnimHash("dataType");
constexpr uint32_t kAttrIgnoreWarnings = AnimHash("IgnoreWarnings");
constexpr uint32_t kAttrData = AnimHash("data");
constexpr uint32_t kAttrDataVar = AnimHash("DataVar");
constexpr uint32_t kAttrGroup = AnimHash("Group");
constexpr uint32_t kAttrFilterDef = AnimHash("FilterDef");
constexpr uint32_t kAttrRange = AnimHash("range");

constexpr uint32_t kDataType_Entity = AnimHash("Entity");
constexpr uint32_t kDataType_Float = AnimHash("float");
constexpr uint32_t kDataType_Hash = AnimHash("Hash");
constexpr uint32_t kDataType_Int = AnimHash("int");

// Variable consulted when a name does not resolve to an object (fixed name
// hash, type 15). NOTE: name unknown; holds an entity.
constexpr uint32_t kEntityVarName = 0x70D68D3Au;
constexpr AnimScriptVarType kVarType_Entity = static_cast<AnimScriptVarType>(15);
} // namespace

// @func 0x0009A190 ResolveEventEntity
WorldObject* ResolveEventEntity(ApeAnimScriptInstance* instance, uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    WorldObject* entity = EntityFromObject(instance->ResolveObject(nameHash, false));
    if (!entity) {
        ApeAnimScriptVar* var = instance->FindVar(kEntityVarName);
        if (var && var->GetType() == kVarType_Entity)
            entity = EntityVarValue(var);
    }
    if (!entity)
        entity = FindEntity(nameHash);
    return entity;
}

// ---------------------------------------------------------------------------
// SendEvent: sends a game event from one entity to another.

class SendEventCommand : public ApeAnimScriptCommand {
public:
    SendEventCommand();
    explicit SendEventCommand(XmlElement* element);
    ~SendEventCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

protected:
    // Event payload: an entity (name, replaced by the entity on use), a
    // float, a hash or an int depending on "dataType".
    union EventData {
        uint32_t hash;
        int32_t i;
        float f;
    };

    // Payload handed to the event system: the resolved entity for entity
    // data, else the raw 32-bit value.
    uintptr_t Payload(ApeAnimScriptInstance* instance) const
    {
        if (m_dataType == kDataType_Entity)
            return reinterpret_cast<uintptr_t>(ResolveEventEntity(instance, m_data.hash));
        return m_data.hash;
    }

    uint32_t m_sender = kSelf;        // +0x0C "Sender"
    uint32_t m_receiver = kSelf;      // +0x10 attribute 0xD6F981F9
    uint32_t m_receiverChild = 0;     // +0x14 attribute 0x6A87CEA3
    uint32_t m_event = 0;             // +0x18 "Event"
    uint32_t m_dataType = 0;          // +0x1C "dataType"
    uint32_t m_dataVar = 0;           // +0x20 "DataVar"
    bool m_ignoreWarnings = false;    // +0x24 "IgnoreWarnings"
    EventData m_data = {};            // +0x28 "data"
};

// @func 0x0006E5CC SendEventCommand::SendEventCommand
SendEventCommand::SendEventCommand() = default;

// @func 0x0073BF00 SendEventCommand::~SendEventCommand
// @func 0x0073BF30 SendEventCommand::~SendEventCommand (deleting)
SendEventCommand::~SendEventCommand() = default;

// @func 0x0006E63C SendEventCommand::SendEventCommand
SendEventCommand::SendEventCommand(XmlElement* element)
{
    element->GetHash(kAttrSender, &m_sender, kSelf);
    element->GetHash(kAttrReceiver, &m_receiver, m_receiver);
    element->GetHash(kAttrReceiverChild, &m_receiverChild, m_receiverChild);
    element->GetHash(kAttrEvent, &m_event, m_event);
    element->GetHash(kAttrDataType, &m_dataType, m_dataType);
    element->GetBool(kAttrIgnoreWarnings, &m_ignoreWarnings, m_ignoreWarnings);
    switch (m_dataType) {
    case kDataType_Entity:
    case kDataType_Hash:
        element->GetHash(kAttrData, &m_data.hash, m_data.hash);
        break;
    case kDataType_Float:
        element->GetFloat(kAttrData, &m_data.f, m_data.f);
        break;
    case kDataType_Int:
        element->GetInt(kAttrData, &m_data.i, m_data.i);
        break;
    default:
        break;
    }
    element->GetHash(kAttrDataVar, &m_dataVar, m_dataVar);
}

// @func 0x0006E870 SendEventCommand::Create
ApeAnimScriptCommand* SendEventCommand::Create(XmlElement* element) const
{
    return new SendEventCommand(element);
}

// @func 0x0006E8E0 SendEventCommand::Execute
void SendEventCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* sender = ResolveEventEntity(instance, m_sender);
    WorldObject* receiver = ResolveEventEntity(instance, m_receiver);
    if (receiver && m_receiverChild)
        receiver = EntityFindChild(receiver, m_receiverChild);
    if (!receiver) {
        if (!m_ignoreWarnings)
            ScriptWarning("%s", m_receiver);
        return;
    }
    // NOTE: the console stores the resolved entity over the entity name, so a
    // second execution would resolve the pointer value as a name. The name is
    // kept here.
    SendGameEvent(sender, receiver, m_event, Payload(instance), true);
}

// ---------------------------------------------------------------------------
// BroadcastEvent: sends a game event to a group of entities.

class BroadcastEventCommand : public SendEventCommand {
public:
    BroadcastEventCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    explicit BroadcastEventCommand(XmlElement* element);
    ~BroadcastEventCommand() override;

    uint32_t m_group = 0;      // +0x2C "Group"
    uint32_t m_filterDef = 0;  // +0x30 "FilterDef"
    float m_range = 0.0f;      // +0x34 "range": 0 broadcasts to the whole group
};

// @func 0x0006E9E4 BroadcastEventCommand::BroadcastEventCommand
BroadcastEventCommand::BroadcastEventCommand() = default;

// @func 0x0073BF88 BroadcastEventCommand::~BroadcastEventCommand
// @func 0x0073BFB8 BroadcastEventCommand::~BroadcastEventCommand (deleting)
BroadcastEventCommand::~BroadcastEventCommand() = default;

BroadcastEventCommand::BroadcastEventCommand(XmlElement* element) : SendEventCommand(element)
{
    element->GetHash(kAttrGroup, &m_group, 0);
    element->GetHash(kAttrFilterDef, &m_filterDef, m_filterDef);
    element->GetFloat(kAttrRange, &m_range, m_range);
}

// @func 0x0006EA64 BroadcastEventCommand::Create
ApeAnimScriptCommand* BroadcastEventCommand::Create(XmlElement* element) const
{
    return new BroadcastEventCommand(element);
}

// @func 0x0006EB50 BroadcastEventCommand::Execute
void BroadcastEventCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* sender = ResolveEventEntity(instance, m_sender);
    if (m_dataType == kDataType_Hash && m_dataVar) {
        // A hash payload can come from an int variable (stored back like the console does).
        ApeAnimScriptVar* var = instance->FindVar(m_dataVar);
        m_data.i = (var && var->GetType() == AnimScriptVarType::Int) ? var->GetInt() : 0;
    }
    const uintptr_t payload = Payload(instance);
    if (m_range > 0.0f)
        BroadcastGameEventInRange(sender, m_group, m_event, payload, m_range);
    else
        BroadcastGameEvent(sender, m_group, m_event, payload);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsEvent()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("BroadcastEvent"), new BroadcastEventCommand());
    prototypes.Add(AnimHash("SendEvent"), new SendEventCommand());
}

} // namespace levels
} // namespace sly
