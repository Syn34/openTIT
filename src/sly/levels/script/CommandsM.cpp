// openTIT - Game script commands, eleventh block (RewindHub to the hash 0x881CF70D command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrLayer = AnimHash("layer");
constexpr uint32_t kAttrLayerFloatVar = 0xA4871DCBu;  // NOTE: name unknown
constexpr uint32_t kAttrReset10 = 0x25F79944u;        // NOTE: name unknown
constexpr uint32_t kAttrReset11 = 0x399AA9F7u;        // NOTE: name unknown
constexpr uint32_t kAttrEntityNameVar = AnimHash("EntityNameVar");
constexpr uint32_t kAttrScore18 = 0xECCE69B4u;        // NOTE: name unknown
constexpr uint32_t kAttrScore1C = 0xDC6BAA58u;        // NOTE: name unknown
constexpr uint32_t kAttrScore20 = 0x33D74B44u;        // NOTE: name unknown
constexpr uint32_t kAttrData = AnimHash("Data");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrState = AnimHash("State");
constexpr uint32_t kAttrClip = AnimHash("clip");
constexpr uint32_t kAttrTarget = AnimHash("Target");
constexpr uint32_t kAttrTargetVar = AnimHash("TargetVar");
constexpr uint32_t kAttrValue = AnimHash("value");
constexpr uint32_t kAttrState28 = 0x9C3FA861u;        // NOTE: name unknown
constexpr uint32_t kAttrAllow = AnimHash("Allow");
constexpr uint32_t kAttrBinocucomMode = 0x19AD5C28u;  // NOTE: name unknown

// Score event types, by index (console table 0x00F65BF8; entry 0 is "none").
constexpr int kScoreEventTypeCount = 33;
constexpr uint32_t kScoreEventTypes[kScoreEventTypeCount] = {
    0x00000000u, 0x879F2BDAu, 0x6362ED0Au, 0x03B7913Cu, 0xA0D76C20u, 0x57C506B6u, 0x04C20F25u,
    0xBE194E0Bu, 0x084A5778u, 0x1EE967E4u, 0xBE52171Eu, 0x2990B813u, 0x0886499Bu, 0x6A46FCAFu,
    0xAC70D594u, 0xCC517C5Cu, 0x69988172u, 0xFE2D0293u, 0x65631C62u, 0xD8F4C1C9u, 0xDFEA6076u,
    0x7113F25Du, 0xB1BC9747u, 0x71445310u, 0x62C083CBu, 0x78B40788u, 0x3F7B9656u, 0xCDCED4B4u,
    0x77297F71u, 0x2B1FBAB8u, 0xFB0005EAu, 0x5E503067u, 0x1DD1C81Bu,
};
} // namespace

// ---------------------------------------------------------------------------
// "RewindHub" (NOTE: name from a hash match): no parameters, does nothing in
// the release build.

class RewindHubCommand : public ApeAnimScriptCommand {
public:
    RewindHubCommand();
    ~RewindHubCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x0008582C RewindHubCommand::RewindHubCommand
RewindHubCommand::RewindHubCommand() = default;

// @func 0x0073DF18 RewindHubCommand::~RewindHubCommand
// @func 0x0073DF48 RewindHubCommand::~RewindHubCommand (deleting)
RewindHubCommand::~RewindHubCommand() = default;

// @func 0x0008586C RewindHubCommand::Create
ApeAnimScriptCommand* RewindHubCommand::Create(XmlElement*) const
{
    return new RewindHubCommand();
}

// @func 0x000858EC RewindHubCommand::Execute
void RewindHubCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
}

// ---------------------------------------------------------------------------
// Hash 0x299E87A1 (name unknown): acts on an animation layer of an entity.

class AnimLayerCommand : public ApeAnimScriptCommand {
public:
    AnimLayerCommand();
    ~AnimLayerCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    uint32_t m_layer = 0;     // +0x14 "layer"
    uint32_t m_floatVar = 0;  // +0x18 attribute 0xA4871DCB
};

// @func 0x000858F8 AnimLayerCommand::AnimLayerCommand
AnimLayerCommand::AnimLayerCommand() = default;

// @func 0x00085950 AnimLayerCommand::~AnimLayerCommand
// @func 0x00085980 AnimLayerCommand::~AnimLayerCommand (deleting)
AnimLayerCommand::~AnimLayerCommand() = default;

// @func 0x000859D8 AnimLayerCommand::Create
ApeAnimScriptCommand* AnimLayerCommand::Create(XmlElement* element) const
{
    auto* command = new AnimLayerCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrEntity, &command->m_entity, 0) != ape::xml::kQuery_Ok)
        return command;
    element->GetHash(kAttrLayer, &command->m_layer, 0);
    element->GetHash(kAttrLayerFloatVar, &command->m_floatVar, 0);
    return command;
}

// TODO(0x00085B10 AnimLayerCommand::Execute) animation player layer calls (0x0034C834..).

// ---------------------------------------------------------------------------
// Hash 0x8D0B7E84 (name unknown): resets parts of a character's state.

class ResetCharacterCommand : public ApeAnimScriptCommand {
public:
    ResetCharacterCommand();
    ~ResetCharacterCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    bool m_reset10 = true;  // +0x10 attribute 0x25F79944
    bool m_reset11 = true;  // +0x11 attribute 0x399AA9F7
};

// @func 0x00085608 ResetCharacterCommand::ResetCharacterCommand
ResetCharacterCommand::ResetCharacterCommand() = default;

// @func 0x0073DE90 ResetCharacterCommand::~ResetCharacterCommand
// @func 0x0073DEC0 ResetCharacterCommand::~ResetCharacterCommand (deleting)
ResetCharacterCommand::~ResetCharacterCommand() = default;

// @func 0x00085660 ResetCharacterCommand::Create
ApeAnimScriptCommand* ResetCharacterCommand::Create(XmlElement* element) const
{
    auto* command = new ResetCharacterCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetBool(kAttrReset10, &command->m_reset10, command->m_reset10);
    element->GetBool(kAttrReset11, &command->m_reset11, command->m_reset11);
    return command;
}

// @func 0x00085750 ResetCharacterCommand::Execute
void ResetCharacterCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // The object lookup here checks the "Entity" user tag, not "pEntity".
    WorldObject* entity = nullptr;
    if (ape::ApeRefCounted* object = instance->ResolveObject(m_entity, false)) {
        if (WorldObjectUserTag(object) == kUserTag_EntityAlt)
            entity = static_cast<WorldObject*>(WorldObjectUserData(object));
    }
    if (!entity)
        entity = FindEntity(m_entity);
    if (!entity || !EntityHasClassFlag2(entity, kEntityClass2_Character))
        return;
    if (m_reset10)
        CharacterReset1F4460(entity);
    if (m_reset11)
        CharacterReset1ED9D4(entity);
}

// ---------------------------------------------------------------------------
// "ScoreEvent" (NOTE: name from a hash match): reports a game statistics event.

class ScoreEventCommand : public ApeAnimScriptCommand {
public:
    ScoreEventCommand();
    ~ScoreEventCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;         // +0x0C "object"
    uint32_t m_entity = 0;         // +0x10 "Entity"
    uint32_t m_entityNameVar = 0;  // +0x14 "EntityNameVar"
    uint32_t m_hash18 = 0;         // +0x18 attribute 0xECCE69B4
    uint32_t m_hash1C = 0;         // +0x1C attribute 0xDC6BAA58
    uint32_t m_hash20 = 0;         // +0x20 attribute 0x33D74B44
    uint32_t m_data = 0;           // +0x24 "Data"
    int32_t m_type = 0;            // +0x28 index of "Type" in kScoreEventTypes
};

// @func 0x00085DB0 ScoreEventCommand::ScoreEventCommand
ScoreEventCommand::ScoreEventCommand() = default;

// @func 0x0073DFA0 ScoreEventCommand::~ScoreEventCommand
// @func 0x0073DFD0 ScoreEventCommand::~ScoreEventCommand (deleting)
ScoreEventCommand::~ScoreEventCommand() = default;

// @func 0x00085E10 ScoreEventCommand::Create
ApeAnimScriptCommand* ScoreEventCommand::Create(XmlElement* element) const
{
    auto* command = new ScoreEventCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrEntityNameVar, &command->m_entityNameVar, command->m_entityNameVar);
    element->GetHash(kAttrScore18, &command->m_hash18, command->m_hash18);
    element->GetHash(kAttrScore1C, &command->m_hash1C, command->m_hash1C);
    element->GetHash(kAttrScore20, &command->m_hash20, command->m_hash20);
    element->GetHash(kAttrData, &command->m_data, command->m_data);
    uint32_t type = 0;
    if (element->GetHash(kAttrType, &type, 0) == ape::xml::kQuery_Ok) {
        for (int i = 0; i < kScoreEventTypeCount; ++i) {
            if (kScoreEventTypes[i] == type) {
                command->m_type = i;
                break;
            }
        }
    }
    return command;
}

// TODO(0x00086000 ScoreEventCommand::Execute) builds one of several statistics event
// records (0x00464EA0, 0x00465494, ...) from the entities and posts it (0x00482D14).

// ---------------------------------------------------------------------------
// Hash 0x2FE3EEC7 (name unknown): state/clip query on an entity with a target.

class EntityStateTargetCommand : public ApeAnimScriptCommand {
public:
    EntityStateTargetCommand();
    ~EntityStateTargetCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;     // +0x0C "Entity"
    uint32_t m_object = 0;     // +0x10 "object"
    uint32_t m_state = 0;      // +0x14 "State"
    uint32_t m_clip = 0;       // +0x18 "clip"
    uint32_t m_target = 0;     // +0x1C "Target"
    uint32_t m_targetVar = 0;  // +0x20 "TargetVar"
    float m_value = 0.0f;      // +0x24 "value"
    bool m_flag28 = false;     // +0x28 attribute 0x9C3FA861
};

// @func 0x000864C8 EntityStateTargetCommand::EntityStateTargetCommand
EntityStateTargetCommand::EntityStateTargetCommand() = default;

// @func 0x0073E028 EntityStateTargetCommand::~EntityStateTargetCommand
// @func 0x0073E058 EntityStateTargetCommand::~EntityStateTargetCommand (deleting)
EntityStateTargetCommand::~EntityStateTargetCommand() = default;

// @func 0x00086528 EntityStateTargetCommand::Create
ApeAnimScriptCommand* EntityStateTargetCommand::Create(XmlElement* element) const
{
    auto* command = new EntityStateTargetCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    element->GetHash(kAttrClip, &command->m_clip, command->m_clip);
    element->GetHash(kAttrTarget, &command->m_target, command->m_target);
    element->GetHash(kAttrTargetVar, &command->m_targetVar, command->m_targetVar);
    element->GetFloat(kAttrValue, &command->m_value, command->m_value);
    element->GetBool(kAttrState28, &command->m_flag28, command->m_flag28);
    return command;
}

// TODO(0x000866D4 EntityStateTargetCommand::Execute) resolves the entity and target
// (through 0x0009A190) and applies the state/clip value.

// ---------------------------------------------------------------------------
// Hash 0xCB0BC14E (name unknown): sets bit 3 of the game world option byte +0x2D.

class AllowWorldOptionCommand : public ApeAnimScriptCommand {
public:
    AllowWorldOptionCommand();
    ~AllowWorldOptionCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_allow = true;  // +0x0C "Allow"
};

// @func 0x000868A0 AllowWorldOptionCommand::AllowWorldOptionCommand
AllowWorldOptionCommand::AllowWorldOptionCommand() = default;

// @func 0x000868E8 AllowWorldOptionCommand::~AllowWorldOptionCommand
// @func 0x00086918 AllowWorldOptionCommand::~AllowWorldOptionCommand (deleting)
AllowWorldOptionCommand::~AllowWorldOptionCommand() = default;

// @func 0x00086970 AllowWorldOptionCommand::Create
ApeAnimScriptCommand* AllowWorldOptionCommand::Create(XmlElement* element) const
{
    auto* command = new AllowWorldOptionCommand();
    element->GetBool(kAttrAllow, &command->m_allow, true);
    return command;
}

// @func 0x00086A18 AllowWorldOptionCommand::Execute
void AllowWorldOptionCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SetGameWorldFlag2D(0x08, m_allow);
}

// ---------------------------------------------------------------------------
// Hash 0x881CF70D (name unknown): sets a mode of an entity's binocucom.

class BinocucomModeCommand : public ApeAnimScriptCommand {
public:
    BinocucomModeCommand();
    ~BinocucomModeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    uint32_t m_mode = 0;    // +0x14 attribute 0x19AD5C28
};

// @func 0x00086DD8 BinocucomModeCommand::BinocucomModeCommand
BinocucomModeCommand::BinocucomModeCommand() = default;

// @func 0x0073E0B0 BinocucomModeCommand::~BinocucomModeCommand
// @func 0x0073E0E0 BinocucomModeCommand::~BinocucomModeCommand (deleting)
BinocucomModeCommand::~BinocucomModeCommand() = default;

// @func 0x00086E28 BinocucomModeCommand::Create
ApeAnimScriptCommand* BinocucomModeCommand::Create(XmlElement* element) const
{
    auto* command = new BinocucomModeCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrBinocucomMode, &command->m_mode, 0);
    return command;
}

// @func 0x00086F18 BinocucomModeCommand::Execute
void BinocucomModeCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_mode == 0)
        return;
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity)
        return;
    if (void* binocucom = PlayerBinocucom(entity))
        BinocucomSetMode(binocucom, m_mode);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsM()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("RewindHub"), new RewindHubCommand());
    prototypes.Add(0x299E87A1u, new AnimLayerCommand());
    prototypes.Add(0x8D0B7E84u, new ResetCharacterCommand());
    prototypes.Add(AnimHash("ScoreEvent"), new ScoreEventCommand());
    // #105 (hash 0xA80527F2, "SendEvent") is registered by RegisterScriptCommandsEvent().
    prototypes.Add(0x2FE3EEC7u, new EntityStateTargetCommand());
    prototypes.Add(0xCB0BC14Eu, new AllowWorldOptionCommand());
    prototypes.Add(0x881CF70Du, new BinocucomModeCommand());
}

} // namespace levels
} // namespace sly
