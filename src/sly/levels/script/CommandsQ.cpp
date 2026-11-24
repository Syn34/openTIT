// openTIT - Game script commands, fifteenth block (SetHealth to the binocucom zoom power command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

#include <algorithm>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrDelta = AnimHash("delta");
constexpr uint32_t kAttrHealthValue = 0x95A6FD77u;   // NOTE: name unknown; absolute health, -1 = unused
constexpr uint32_t kAttrFullHealth = 0x1A07CE8Bu;    // NOTE: name unknown; restores full health
constexpr uint32_t kAttrClearStatus = 0x35D8B1D3u;   // NOTE: name unknown; clears status bits
constexpr uint32_t kAttrWorldValue = 0x2C9060E7u;    // NOTE: name unknown
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrEntityChild = AnimHash("EntityChild");
constexpr uint32_t kAttrEnabled = AnimHash("enabled");
constexpr uint32_t kAttrEnabled19 = 0xF5897C08u;     // NOTE: name unknown
constexpr uint32_t kAttrSpawnPt = AnimHash("SpawnPT");
constexpr uint32_t kAttrRadius = AnimHash("radius");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrTrackValue = 0x17A758DCu;    // NOTE: name unknown
constexpr uint32_t kAttrTrackValueVar = 0x9B5EF4ABu; // NOTE: name unknown
constexpr uint32_t kAttrTrack = 0xA3370170u;         // NOTE: name unknown
constexpr uint32_t kAttrForceReset = AnimHash("ForceReset");
constexpr uint32_t kElementSet = AnimHash("Set");
constexpr uint32_t kElementClear = AnimHash("Clear");
constexpr uint32_t kAttrLightGroup = AnimHash("lightGroup");
constexpr uint32_t kAttrPower = AnimHash("Power");

// Entity flag bit (+0xB8) that hides an entity.
constexpr uint64_t kEntityFlag_Hidden = 0x0000002000000000ull;
// Status bits cleared by SetHealth (mask of entity +0x178).
constexpr uint32_t kHealthStatusClearMask = 0x0FF04000u;
// Character interaction flag (+0x40000) used by the interaction command.
constexpr uint32_t kInteractionFlag = 0x40000u;
} // namespace

// ---------------------------------------------------------------------------
// "SetHealth"

class SetHealthCommand : public ApeAnimScriptCommand {
public:
    SetHealthCommand();
    explicit SetHealthCommand(XmlElement* element);
    ~SetHealthCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;       // +0x0C "Entity"
    uint32_t m_object = 0;       // +0x10 "object"
    float m_delta = 0.0f;        // +0x14 "delta"
    float m_value = -1.0f;       // +0x18 attribute 0x95A6FD77
    // +0x1C bit field.
    bool m_fullHealth = false;   // bit 31 attribute 0x1A07CE8B
    bool m_clearStatus = false;  // bit 30 attribute 0x35D8B1D3
};

// @func 0x0008A818 SetHealthCommand::SetHealthCommand
SetHealthCommand::SetHealthCommand() = default;

// @func 0x0073EA40 SetHealthCommand::~SetHealthCommand
// @func 0x0073EA70 SetHealthCommand::~SetHealthCommand (deleting)
SetHealthCommand::~SetHealthCommand() = default;

// @func 0x0008A880 SetHealthCommand::SetHealthCommand
SetHealthCommand::SetHealthCommand(XmlElement* element)
{
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrObject, &m_object, m_object);
    element->GetFloat(kAttrDelta, &m_delta, m_delta);
    element->GetFloat(kAttrHealthValue, &m_value, m_value);
    element->GetBool(kAttrFullHealth, &m_fullHealth, m_fullHealth);
    element->GetBool(kAttrClearStatus, &m_clearStatus, m_clearStatus);
}

// @func 0x0008AA00 SetHealthCommand::Create
ApeAnimScriptCommand* SetHealthCommand::Create(XmlElement* element) const
{
    return new SetHealthCommand(element);
}

// @func 0x0008AA78 SetHealthCommand::Execute
void SetHealthCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntityByObjectName(instance, m_entity, m_object);
    if (!entity || !EntityHasClassFlag(entity, kEntityClass_Killable))
        return;
    if (m_fullHealth) {
        EntitySetHealth(entity, std::max(0.0f, EntityMaxHealth(entity)));
    } else if (m_delta > 0.0f) {
        EntityHeal(entity, m_delta);
    } else if (m_value >= 0.0f) {
        EntitySetHealth(entity, std::max(0.0f, std::min(m_value, EntityMaxHealth(entity))));
    }
    if (m_clearStatus)
        EntityClearStatus(entity, kHealthStatusClearMask);
}

// ---------------------------------------------------------------------------
// Hash 0x72794E29 (name unknown): sets (or resets, when the attribute is
// missing) a game world value.

class SetWorldValueCommand : public ApeAnimScriptCommand {
public:
    SetWorldValueCommand();
    ~SetWorldValueCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_value = 1.0f;     // +0x0C attribute 0x2C9060E7
    bool m_hasValue = false;  // +0x10
};

// @func 0x0008ABE8 SetWorldValueCommand::SetWorldValueCommand
SetWorldValueCommand::SetWorldValueCommand() = default;

// @func 0x0008AC44 SetWorldValueCommand::~SetWorldValueCommand
// @func 0x0008AC74 SetWorldValueCommand::~SetWorldValueCommand (deleting)
SetWorldValueCommand::~SetWorldValueCommand() = default;

// @func 0x0008ACD0 SetWorldValueCommand::Create
ApeAnimScriptCommand* SetWorldValueCommand::Create(XmlElement* element) const
{
    auto* command = new SetWorldValueCommand();
    command->m_hasValue =
        element->GetFloat(kAttrWorldValue, &command->m_value, 1.0f) == ape::xml::kQuery_Ok;
    return command;
}

// @func 0x0008AD90 SetWorldValueCommand::Execute
void SetWorldValueCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_hasValue)
        GameWorldSetValue48C82C(m_value);
    else
        GameWorldResetValue48C840();
}

// ---------------------------------------------------------------------------
// Hash 0xBE2D3C0A (name unknown): sets bit 20 of +0x3C0 of the object
// returned by 0x0035B908 (on the global 0x00F64D48).

class EnableHudFlagCommand : public ApeAnimScriptCommand {
public:
    EnableHudFlagCommand();
    ~EnableHudFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = false;  // +0x0C "enable"
};

// @func 0x0008ADE4 EnableHudFlagCommand::EnableHudFlagCommand
EnableHudFlagCommand::EnableHudFlagCommand() = default;

// @func 0x0073EAC8 EnableHudFlagCommand::~EnableHudFlagCommand
// @func 0x0073EAF8 EnableHudFlagCommand::~EnableHudFlagCommand (deleting)
EnableHudFlagCommand::~EnableHudFlagCommand() = default;

// @func 0x0008AE2C EnableHudFlagCommand::Create
ApeAnimScriptCommand* EnableHudFlagCommand::Create(XmlElement* element) const
{
    auto* command = new EnableHudFlagCommand();
    element->GetBool(kAttrEnable, &command->m_enable, false);
    return command;
}

// @func 0x0008AED0 EnableHudFlagCommand::Execute
void EnableHudFlagCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (void* object = FindObject35B908())
        Object35B908SetFlag(object, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0x90F19CF0 (name unknown): shows or
// hides an entity through its hidden flag.

class SetEntityShownCommand : public ApeAnimScriptCommand {
public:
    SetEntityShownCommand();
    ~SetEntityShownCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    bool m_visible = false; // +0x14 "visible"
};

// @func 0x0008AF24 SetEntityShownCommand::SetEntityShownCommand
SetEntityShownCommand::SetEntityShownCommand() = default;

// @func 0x0073EB50 SetEntityShownCommand::~SetEntityShownCommand
// @func 0x0073EB80 SetEntityShownCommand::~SetEntityShownCommand (deleting)
SetEntityShownCommand::~SetEntityShownCommand() = default;

// @func 0x0008AF74 SetEntityShownCommand::Create
ApeAnimScriptCommand* SetEntityShownCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntityShownCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    return command;
}

// @func 0x0008B070 SetEntityShownCommand::Execute
void SetEntityShownCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = EntityFromVar(instance, m_object);
    if (!entity && m_entity != 0)
        entity = ResolveEventEntity(instance, m_entity);
    if (entity)
        EntitySetFlags64(entity, kEntityFlag_Hidden, !m_visible);
}

// ---------------------------------------------------------------------------
// Hash 0x5E2BDAD5 (name unknown): enables or disables an entity's
// interaction component (or the killable entity's interaction flag).

class EnableInteractionCommand : public ApeAnimScriptCommand {
public:
    EnableInteractionCommand();
    ~EnableInteractionCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;       // +0x0C "object"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_entityChild = 0;  // +0x14 "EntityChild"
    bool m_enabled = false;      // +0x18 "enabled"
    bool m_flag19 = false;       // +0x19 attribute 0xF5897C08
};

// @func 0x0008B1C4 EnableInteractionCommand::EnableInteractionCommand
EnableInteractionCommand::EnableInteractionCommand() = default;

// @func 0x0073EBD8 EnableInteractionCommand::~EnableInteractionCommand
// @func 0x0073EC08 EnableInteractionCommand::~EnableInteractionCommand (deleting)
EnableInteractionCommand::~EnableInteractionCommand() = default;

// @func 0x0008B21C EnableInteractionCommand::Create
ApeAnimScriptCommand* EnableInteractionCommand::Create(XmlElement* element) const
{
    auto* command = new EnableInteractionCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrEntityChild, &command->m_entityChild, command->m_entityChild);
    element->GetBool(kAttrEnabled, &command->m_enabled, command->m_enabled);
    element->GetBool(kAttrEnabled19, &command->m_flag19, command->m_flag19);
    return command;
}

// @func 0x0008B360 EnableInteractionCommand::Execute
void EnableInteractionCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = EntityFromVar(instance, m_object);
    if (!entity && m_entity != 0)
        entity = ResolveEventEntity(instance, m_entity);
    if (entity && m_entityChild != 0)
        entity = EntityFindChild(entity, m_entityChild);
    if (!entity)
        return;
    if (void* interaction = EntityInteraction(entity))
        InteractionSetEnabled(interaction, m_enabled);
    else if (EntityHasClassFlag(entity, kEntityClass_Killable))
        EntitySetStateFlag(entity, kInteractionFlag, m_enabled, m_flag19);
}

// ---------------------------------------------------------------------------
// "SetLeash" (NOTE: name from a hash match): keeps the player near a point.

class SetLeashCommand : public ApeAnimScriptCommand {
public:
    SetLeashCommand();
    ~SetLeashCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enabled = false;   // +0x0C "enabled"
    uint32_t m_spawnPt = 0;   // +0x10 "SpawnPT" (object variable or point instance name)
    float m_radius = 0.0f;    // +0x14 "radius"
    float m_duration = 0.0f;  // +0x18 "duration"
};

// @func 0x0008B968 SetLeashCommand::SetLeashCommand
SetLeashCommand::SetLeashCommand() = default;

// @func 0x0073ECE8 SetLeashCommand::~SetLeashCommand
// @func 0x0073ED18 SetLeashCommand::~SetLeashCommand (deleting)
SetLeashCommand::~SetLeashCommand() = default;

// @func 0x0008B9C0 SetLeashCommand::Create
ApeAnimScriptCommand* SetLeashCommand::Create(XmlElement* element) const
{
    auto* command = new SetLeashCommand();
    element->GetBool(kAttrEnabled, &command->m_enabled, false);
    element->GetHash(kAttrSpawnPt, &command->m_spawnPt, command->m_spawnPt);
    element->GetFloat(kAttrRadius, &command->m_radius, command->m_radius);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    return command;
}

// @func 0x0008BAE0 SetLeashCommand::Execute
void SetLeashCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* point = PointInstanceFromObject(ObjectFromVar(instance, m_spawnPt));
    if (!point)
        point = FindPointInstance(m_spawnPt);
    WorldObject* player = PlayerEntity();
    if (!player || !EntityHasClassFlag2(player, kEntityClass2_Leashable))
        return;
    if (!point) {
        if (m_spawnPt != 0)
            PlayerSetLeashEnabled(player, false);
        return;
    }
    PlayerSetLeash(player, m_radius * m_radius, m_duration, point);
    PlayerSetLeashEnabled(player, m_enabled);
}

// ---------------------------------------------------------------------------
// Hash 0x3FCA7434 (name unknown): selects a track/value of the world object
// +0x1281F8.

class WorldTrackCommand : public ApeAnimScriptCommand {
public:
    WorldTrackCommand();
    ~WorldTrackCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_value = 0.0f;        // +0x0C attribute 0x17A758DC
    uint32_t m_valueVar = 0;     // +0x10 attribute 0x9B5EF4AB
    uint32_t m_track = 0;        // +0x14 attribute 0xA3370170
    bool m_forceReset = false;   // +0x18 "ForceReset"
    bool m_hasValue = false;     // +0x19
    bool m_hasTrack = false;     // +0x1A
};

// @func 0x0008BC34 WorldTrackCommand::WorldTrackCommand
WorldTrackCommand::WorldTrackCommand() = default;

// @func 0x0073ED70 WorldTrackCommand::~WorldTrackCommand
// @func 0x0073EDA0 WorldTrackCommand::~WorldTrackCommand (deleting)
WorldTrackCommand::~WorldTrackCommand() = default;

// @func 0x0008BCA8 WorldTrackCommand::Create
ApeAnimScriptCommand* WorldTrackCommand::Create(XmlElement* element) const
{
    auto* command = new WorldTrackCommand();
    if (element->GetFloat(kAttrTrackValue, &command->m_value, 0.0f) == ape::xml::kQuery_Ok)
        command->m_hasValue = true;
    if (element->GetHash(kAttrTrackValueVar, &command->m_valueVar, command->m_valueVar) == ape::xml::kQuery_Ok)
        command->m_hasValue = true;
    if (element->GetHash(kAttrTrack, &command->m_track, command->m_track) == ape::xml::kQuery_Ok)
        command->m_hasTrack = true;
    element->GetBool(kAttrForceReset, &command->m_forceReset, command->m_forceReset);
    return command;
}

// @func 0x0008BE18 WorldTrackCommand::Execute
// Controls the level's music/ambience track player (game world +0x1281F8):
// switch track (optionally forcing a reset) and set its value.
void WorldTrackCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    void* player = WorldTrackPlayer();
    if (!player)
        return;
    if (m_forceReset)
        TrackPlayerReset(player, m_hasTrack ? m_track : 0);
    else if (m_hasTrack)
        TrackPlayerSelect(player, m_track);
    if (m_hasValue)
        TrackPlayerSetValue(player, ResolveFloat(instance, m_valueVar, m_value));
}

// ---------------------------------------------------------------------------
// "InstanceLightGroup" (NOTE: name from a hash match): sets and clears light
// group bits of a render object.

class InstanceLightGroupCommand : public ApeAnimScriptCommand {
public:
    InstanceLightGroupCommand();
    ~InstanceLightGroupCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object" (object variable)
    uint32_t m_setMask = 0;    // +0x10 bits of the "Set" children
    uint32_t m_clearMask = 0;  // +0x14 bits of the "Clear" children
};

// @func 0x0008BFB8 InstanceLightGroupCommand::InstanceLightGroupCommand
InstanceLightGroupCommand::InstanceLightGroupCommand() = default;

// @func 0x0008C008 InstanceLightGroupCommand::~InstanceLightGroupCommand
// @func 0x0008C038 InstanceLightGroupCommand::~InstanceLightGroupCommand (deleting)
InstanceLightGroupCommand::~InstanceLightGroupCommand() = default;

// @func 0x0008C090 InstanceLightGroupCommand::Create
ApeAnimScriptCommand* InstanceLightGroupCommand::Create(XmlElement* element) const
{
    auto* command = new InstanceLightGroupCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) != ape::xml::kQuery_Ok)
        return command;
    for (XmlElement* child = element->FirstChildElement(kElementSet); child;
         child = child->NextSiblingElement(kElementSet)) {
        int32_t group = 0;
        child->GetInt(kAttrLightGroup, &group, 0);
        command->m_setMask |= 1u << (group & 31);
    }
    for (XmlElement* child = element->FirstChildElement(kElementClear); child;
         child = child->NextSiblingElement(kElementClear)) {
        int32_t group = 0;
        child->GetInt(kAttrLightGroup, &group, 0);
        command->m_clearMask |= 1u << (group & 31);
    }
    return command;
}

// @func 0x0008C230 InstanceLightGroupCommand::Execute
void InstanceLightGroupCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeRefCounted* object = ObjectFromVar(instance, m_object);
    if (object && ObjectIsRenderInstance(object))
        RenderInstanceSetLightGroupMask(object, (RenderInstanceLightGroupMask(object) | m_setMask) & ~m_clearMask);
}

// ---------------------------------------------------------------------------
// Hash 0xC3E53CB8 (name unknown): sets a second binocucom power value.

class SetBinocucomZoomCommand : public ApeAnimScriptCommand {
public:
    SetBinocucomZoomCommand();
    ~SetBinocucomZoomCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_power = -1.0f;  // +0x0C "Power" (default when parsed: 0)
};

// @func 0x0008C2F0 SetBinocucomZoomCommand::SetBinocucomZoomCommand
SetBinocucomZoomCommand::SetBinocucomZoomCommand() = default;

// @func 0x0073EDF8 SetBinocucomZoomCommand::~SetBinocucomZoomCommand
// @func 0x0073EE28 SetBinocucomZoomCommand::~SetBinocucomZoomCommand (deleting)
SetBinocucomZoomCommand::~SetBinocucomZoomCommand() = default;

// @func 0x0008C350 SetBinocucomZoomCommand::Create
ApeAnimScriptCommand* SetBinocucomZoomCommand::Create(XmlElement* element) const
{
    auto* command = new SetBinocucomZoomCommand();
    element->GetFloat(kAttrPower, &command->m_power, 0.0f);
    return command;
}

// @func 0x0008C400 SetBinocucomZoomCommand::Execute
void SetBinocucomZoomCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    BinocucomSetPower2(m_power);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsQ()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SetHealth"), new SetHealthCommand());
    prototypes.Add(0x72794E29u, new SetWorldValueCommand());
    prototypes.Add(0xBE2D3C0Au, new EnableHudFlagCommand());
    prototypes.Add(0x90F19CF0u, new SetEntityShownCommand());
    prototypes.Add(0x5E2BDAD5u, new EnableInteractionCommand());
    prototypes.Add(AnimHash("SetLeash"), new SetLeashCommand());
    prototypes.Add(0x3FCA7434u, new WorldTrackCommand());
    prototypes.Add(AnimHash("InstanceLightGroup"), new InstanceLightGroupCommand());
    prototypes.Add(0xC3E53CB8u, new SetBinocucomZoomCommand());
}

} // namespace levels
} // namespace sly
