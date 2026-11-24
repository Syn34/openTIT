// openTIT - Game script commands, seventeenth block (player ability to the skin script command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrAbility = AnimHash("ability");
constexpr uint32_t kAttrBlock = AnimHash("block");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrSetLocked = AnimHash("SetLocked");
constexpr uint32_t kAttrLock15 = 0x1E389259u;    // NOTE: name unknown
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrActiveDuration = AnimHash("ActiveDuration");
constexpr uint32_t kAttrRefreshDuration = AnimHash("RefreshDuration");
constexpr uint32_t kAttrNoGravity = AnimHash("NoGravity");
constexpr uint32_t kAttrGravity16 = 0x903B1E30u; // NOTE: name unknown
constexpr uint32_t kAttrGravity18 = 0xBE647980u; // NOTE: name unknown
constexpr uint32_t kAttrSpline = AnimHash("spline");
constexpr uint32_t kAttrReverse = AnimHash("Reverse");
constexpr uint32_t kAttrWorldObject = 0x6334AE6Fu;  // NOTE: name unknown; look the object up in the world
constexpr uint32_t kAttrSetting = AnimHash("Setting");
constexpr uint32_t kAttrLightDirection = AnimHash("LightDirection");
constexpr uint32_t kAttrScript = AnimHash("Script");
constexpr uint32_t kAttrPartIndex = AnimHash("PartIndex");

// Class flags (+0xC4) used here.
constexpr uint32_t kEntityClass3_Lockable = 0x20000u;  // NOTE: inferred from use
constexpr uint32_t kEntityClass3_Gravity = 0x1000u;    // NOTE: inferred from use

// NOTE: console global 0x01164B90; assumed to be a downward light.
const ape::Vec3 kDefaultLightDirection = {0.0f, -1.0f, 0.0f, 0.0f};
} // namespace

// ---------------------------------------------------------------------------
// Hash 0x3292B062 (name unknown): blocks or enables an ability for a whole
// player profile.

class PlayerAbilityCommand : public ApeAnimScriptCommand {
public:
    PlayerAbilityCommand();
    ~PlayerAbilityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerIdx = 0;   // +0x0C "PlayerIdx" (1-based in data)
    uint32_t m_ability = 0;    // +0x10 "ability"
    bool m_block = false;      // +0x14 "block"
    bool m_hasBlock = false;   // +0x15
    bool m_enable = false;     // +0x16 "enable"
    bool m_hasEnable = false;  // +0x17
};

// @func 0x0008DA24 PlayerAbilityCommand::PlayerAbilityCommand
PlayerAbilityCommand::PlayerAbilityCommand() = default;

// @func 0x0008DA84 PlayerAbilityCommand::~PlayerAbilityCommand
// @func 0x0008DAB4 PlayerAbilityCommand::~PlayerAbilityCommand (deleting)
PlayerAbilityCommand::~PlayerAbilityCommand() = default;

// @func 0x0008DB0C PlayerAbilityCommand::Create
ApeAnimScriptCommand* PlayerAbilityCommand::Create(XmlElement* element) const
{
    auto* command = new PlayerAbilityCommand();
    int32_t playerIdx = 0;
    if (element->GetInt(kAttrPlayerIdx, &playerIdx, 0) == ape::xml::kQuery_Ok)
        command->m_playerIdx = playerIdx - 1 < 0 ? 0 : playerIdx - 1;
    element->GetHash(kAttrAbility, &command->m_ability, 0);
    if (element->GetBool(kAttrBlock, &command->m_block, false) == ape::xml::kQuery_Ok)
        command->m_hasBlock = true;
    if (element->GetBool(kAttrEnable, &command->m_enable, false) == ape::xml::kQuery_Ok)
        command->m_hasEnable = true;
    return command;
}

// @func 0x0008DC70 PlayerAbilityCommand::Execute
void PlayerAbilityCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!m_hasBlock && !m_hasEnable)
        return;
    int ability = ProfileFindAbility(m_ability);
    if (ability == -1)
        return;
    if (m_hasBlock)
        ProfileSetAbilityBlocked(m_playerIdx, ability, m_block);
    else
        ProfileSetAbilityEnabled(m_playerIdx, ability, m_enable);
}

// TODO(0x0008DE44 SetPosCommand::SetPosCommand) "SetPos" (#152): 0xF0-byte command with a
// transform built from Euler angles; see NOTES.md.

// ---------------------------------------------------------------------------
// Hash 0x80866784 (name unknown): locks an entity (or releases it).

class SetLockedCommand : public ApeAnimScriptCommand {
public:
    SetLockedCommand();
    ~SetLockedCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;     // +0x0C "Entity"
    uint32_t m_refObject = 0;  // +0x10 "RefObject"
    bool m_setLocked = false;  // +0x14 "SetLocked"
    bool m_flag15 = false;     // +0x15 attribute 0x1E389259
};

// @func 0x0008FB3C SetLockedCommand::SetLockedCommand
SetLockedCommand::SetLockedCommand() = default;

// @func 0x0073F198 SetLockedCommand::~SetLockedCommand
// @func 0x0073F1C8 SetLockedCommand::~SetLockedCommand (deleting)
SetLockedCommand::~SetLockedCommand() = default;

// @func 0x0008FB8C SetLockedCommand::Create
ApeAnimScriptCommand* SetLockedCommand::Create(XmlElement* element) const
{
    auto* command = new SetLockedCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrRefObject, &command->m_refObject, command->m_refObject);
    element->GetBool(kAttrSetLocked, &command->m_setLocked, command->m_setLocked);
    element->GetBool(kAttrLock15, &command->m_flag15, command->m_flag15);
    return command;
}

// @func 0x0008FCA0 SetLockedCommand::Execute
void SetLockedCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntityByObjectName(instance, m_entity, m_refObject);
    if (!entity || !EntityHasClassFlag3(entity, kEntityClass3_Lockable))
        return;
    if (m_setLocked)
        LockableSetLocked(entity, m_flag15, true);
    else
        EntityVirtualCommand(entity, 0, 0);
}

// ---------------------------------------------------------------------------
// Hash 0x02C1266A (name unknown): sets the player's targeting durations.

class SetTargetingDurationCommand : public ApeAnimScriptCommand {
public:
    SetTargetingDurationCommand();
    ~SetTargetingDurationCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_activeDuration = -1.0f;   // +0x0C "ActiveDuration" ("duration" sets both)
    float m_refreshDuration = -1.0f;  // +0x10 "RefreshDuration"
    // +0x14 bit field.
    bool m_hasActive = false;         // bit 31
    bool m_hasRefresh = false;        // bit 30
};

// @func 0x0008FE28 SetTargetingDurationCommand::SetTargetingDurationCommand
SetTargetingDurationCommand::SetTargetingDurationCommand() = default;

// @func 0x0008FE90 SetTargetingDurationCommand::~SetTargetingDurationCommand
// @func 0x0008FEC0 SetTargetingDurationCommand::~SetTargetingDurationCommand (deleting)
SetTargetingDurationCommand::~SetTargetingDurationCommand() = default;

// @func 0x0008FF20 SetTargetingDurationCommand::Create
ApeAnimScriptCommand* SetTargetingDurationCommand::Create(XmlElement* element) const
{
    auto* command = new SetTargetingDurationCommand();
    float duration = -1.0f;
    if (element->GetFloat(kAttrDuration, &duration, -1.0f) == ape::xml::kQuery_Ok) {
        command->m_hasActive = true;
        command->m_hasRefresh = true;
        command->m_activeDuration = duration;
        command->m_refreshDuration = duration;
    }
    if (element->GetFloat(kAttrActiveDuration, &command->m_activeDuration, -1.0f) == ape::xml::kQuery_Ok) {
        command->m_hasActive = true;
    } else if (element->GetFloat(kAttrRefreshDuration, &command->m_refreshDuration, -1.0f) ==
               ape::xml::kQuery_Ok) {
        // NOTE: the console overwrites the value read with 1.0 here and does
        // not set the "has refresh" bit (kept as is).
        command->m_refreshDuration = 1.0f;
    }
    return command;
}

// @func 0x0009008C SetTargetingDurationCommand::Execute
void SetTargetingDurationCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    void* gadget = EntityTargetingGadget(PlayerEntity());
    if (!gadget)
        return;
    if (m_hasActive)
        GadgetSetActiveDuration(gadget, m_activeDuration);
    if (m_hasRefresh)
        GadgetSetRefreshDuration(gadget, m_refreshDuration);
}

// ---------------------------------------------------------------------------
// Hash 0xD1BB9067 (name unknown): gravity switches of an entity.

class SetGravityCommand : public ApeAnimScriptCommand {
public:
    SetGravityCommand();
    ~SetGravityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;        // +0x0C "Entity"
    uint32_t m_refObject = 0;     // +0x10 "RefObject"
    bool m_noGravity = false;     // +0x14 "NoGravity"
    bool m_hasNoGravity = false;  // +0x15
    bool m_flag16 = false;        // +0x16 attribute 0x903B1E30
    bool m_hasFlag16 = false;     // +0x17
    bool m_flag18 = false;        // +0x18 attribute 0xBE647980
};

// @func 0x00090124 SetGravityCommand::SetGravityCommand
SetGravityCommand::SetGravityCommand() = default;

// @func 0x0073F220 SetGravityCommand::~SetGravityCommand
// @func 0x0073F250 SetGravityCommand::~SetGravityCommand (deleting)
SetGravityCommand::~SetGravityCommand() = default;

// @func 0x0009018C SetGravityCommand::Create
ApeAnimScriptCommand* SetGravityCommand::Create(XmlElement* element) const
{
    auto* command = new SetGravityCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrRefObject, &command->m_refObject, command->m_refObject);
    // Anything but "missing" counts as present (a wrong type too).
    if (element->GetBool(kAttrNoGravity, &command->m_noGravity, command->m_noGravity) != ape::xml::kQuery_Missing)
        command->m_hasNoGravity = true;
    if (element->GetBool(kAttrGravity16, &command->m_flag16, command->m_flag16) != ape::xml::kQuery_Missing)
        command->m_hasFlag16 = true;
    element->GetBool(kAttrGravity18, &command->m_flag18, command->m_flag18);
    return command;
}

// @func 0x000902F8 SetGravityCommand::Execute
void SetGravityCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntityByObjectName(instance, m_entity, m_refObject);
    if (!entity || !EntityHasClassFlag3(entity, kEntityClass3_Gravity))
        return;
    if (m_hasNoGravity)
        EntitySetNoGravity(entity, m_noGravity);
    if (m_flag18)
        PhysicsEntityReset1BD540(entity);
    if (m_hasFlag16)
        PhysicsEntitySet1BD450(entity, m_flag16);
}

// ---------------------------------------------------------------------------
// Hash 0xACE05706 (name unknown): puts an entity on a spline.

class SetEntitySplineCommand : public ApeAnimScriptCommand {
public:
    SetEntitySplineCommand();
    ~SetEntitySplineCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;      // +0x0C "object"
    uint32_t m_entity = 0;      // +0x10 "Entity"
    uint32_t m_spline = 0;      // +0x14 "spline"
    bool m_hasReverse = false;  // +0x18
    bool m_reverse = false;     // +0x19 "Reverse"
};

// @func 0x000903F8 SetEntitySplineCommand::SetEntitySplineCommand
SetEntitySplineCommand::SetEntitySplineCommand() = default;

// @func 0x0073F2A8 SetEntitySplineCommand::~SetEntitySplineCommand
// @func 0x0073F2D8 SetEntitySplineCommand::~SetEntitySplineCommand (deleting)
SetEntitySplineCommand::~SetEntitySplineCommand() = default;

// @func 0x00090450 SetEntitySplineCommand::Create
ApeAnimScriptCommand* SetEntitySplineCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntitySplineCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrSpline, &command->m_spline, command->m_spline);
    if (element->GetBool(kAttrReverse, &command->m_reverse, command->m_reverse) != ape::xml::kQuery_Missing)
        command->m_hasReverse = true;
    return command;
}

// @func 0x0009057C SetEntitySplineCommand::Execute
// Puts a sentry light on another patrol spline and/or reverses it.
void SetEntitySplineCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity || !EntityIsSentryLight(entity))
        return;
    if (m_spline != 0) {
        if (void* spline = FindSplineDefByName(m_spline))
            SentryLightSetSpline(entity, spline);
    }
    if (m_hasReverse)
        SentryLightReverse(entity);
}

// ---------------------------------------------------------------------------
// Hash 0x908A1BE5 (name unknown): shows or hides an entity's (or a world
// object's) render instance.

class SetRenderVisibleCommand : public ApeAnimScriptCommand {
public:
    SetRenderVisibleCommand();
    ~SetRenderVisibleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;      // +0x0C "object"
    uint32_t m_entity = 0;      // +0x10 "Entity"
    bool m_enable = false;      // +0x14 "enable"
    bool m_fromWorld = false;   // +0x15 attribute 0x6334AE6F
};

// @func 0x000906E8 SetRenderVisibleCommand::SetRenderVisibleCommand
SetRenderVisibleCommand::SetRenderVisibleCommand() = default;

// @func 0x0073F330 SetRenderVisibleCommand::~SetRenderVisibleCommand
// @func 0x0073F360 SetRenderVisibleCommand::~SetRenderVisibleCommand (deleting)
SetRenderVisibleCommand::~SetRenderVisibleCommand() = default;

// @func 0x00090738 SetRenderVisibleCommand::Create
ApeAnimScriptCommand* SetRenderVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new SetRenderVisibleCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    element->GetBool(kAttrWorldObject, &command->m_fromWorld, command->m_fromWorld);
    return command;
}

// @func 0x0009084C SetRenderVisibleCommand::Execute
void SetRenderVisibleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = nullptr;
    if (m_object != 0 && !m_fromWorld)
        entity = EntityFromVar(instance, m_object);
    if (!entity && m_entity != 0)
        entity = FindEntity(m_entity);

    ape::ApeRefCounted* object = nullptr;
    if (entity)
        object = EntityWorldObject(entity);
    else if (m_object != 0 && m_fromWorld)
        object = instance->ResolveObject(m_object, true);
    if (!object || !ObjectIsRenderInstance(object))
        return;
    if (RenderInstanceShown(object) != m_enable) {
        RenderInstanceSetShown(object, m_enable);
        RenderInstanceRefresh(object);
    }
}

// ---------------------------------------------------------------------------
// Hash 0x6F37AF4C (name unknown): picks a lighting setting and/or the light
// direction of the level lighting object (console pointer 0x00F66FD0).

class SetLightSettingCommand : public ApeAnimScriptCommand {
public:
    SetLightSettingCommand();
    ~SetLightSettingCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_setting = 0;                          // +0x0C "Setting"
    ape::Vec3 m_direction = kDefaultLightDirection;  // +0x10 "LightDirection"
    bool m_hasSetting = false;                       // +0x20
    bool m_hasDirection = false;                     // +0x21

    // Presets of the two shadow modes.
    static void ApplyPresetMode0(SunLightSettings& light, uint32_t setting);
    static void ApplyPresetMode1(SunLightSettings& light, uint32_t setting);
};

// @func 0x000909E4 SetLightSettingCommand::SetLightSettingCommand
SetLightSettingCommand::SetLightSettingCommand() = default;

// @func 0x00090A5C SetLightSettingCommand::~SetLightSettingCommand
// @func 0x00090A8C SetLightSettingCommand::~SetLightSettingCommand (deleting)
SetLightSettingCommand::~SetLightSettingCommand() = default;

// @func 0x00090AE4 SetLightSettingCommand::Create
ApeAnimScriptCommand* SetLightSettingCommand::Create(XmlElement* element) const
{
    auto* command = new SetLightSettingCommand();
    if (element->GetHash(kAttrSetting, &command->m_setting, 0) == ape::xml::kQuery_Ok)
        command->m_hasSetting = true;
    if (element->GetVec3(kAttrLightDirection, &command->m_direction.x, &command->m_direction.x) ==
        ape::xml::kQuery_Ok)
        command->m_hasDirection = true;
    return command;
}

namespace {
// Light presets. NOTE: only "Default" is a known name.
constexpr uint32_t kLightPreset_67430483 = 0x67430483u;
constexpr uint32_t kLightPreset_466CDEF5 = 0x466CDEF5u;
constexpr uint32_t kLightPreset_9535758E = 0x9535758Eu;
constexpr uint32_t kLightPresetDefault = 0x86DF6221u;   // "Default"
} // namespace

// @func 0x00090D00 SetLightSettingCommand::ApplyPresetMode0
void SetLightSettingCommand::ApplyPresetMode0(SunLightSettings& light, uint32_t setting)
{
    if (setting != kLightPreset_9535758E)
        return;
    light.value100 = 60.0f;
    SunLightSetRange(light, 75.0f);
    light.value108 = 0.035f;
    light.value10C = 0.4f;
    if (light.cascade[0]) {
        light.cascade[0]->depthMin = -60.0f;
        light.cascade[0]->depthMax = 140.0f;
    }
}

// @func 0x00090C00 SetLightSettingCommand::ApplyPresetMode1
void SetLightSettingCommand::ApplyPresetMode1(SunLightSettings& light, uint32_t setting)
{
    if (setting == kLightPreset_67430483 || setting == kLightPreset_466CDEF5)
        return;
    if (setting == kLightPreset_9535758E) {
        SunLightCascade* nearCascade = light.cascade[0];
        SunLightCascade* farCascade = light.cascade[1];
        SunLightSetRange(light, 60.0f);
        nearCascade->splitStart = 0.0f;
        nearCascade->splitEnd = 1.0f;
        farCascade->splitStart = 1.0f;
        farCascade->splitEnd = 0.0f;
        nearCascade->depthMin = -60.0f;
        nearCascade->depthMax = 140.0f;
        farCascade->depthMin = -60.0f;
        farCascade->depthMax = 140.0f;
    } else if (setting == kLightPresetDefault) {
        SunLightRestoreDefaults();
    }
}

// @func 0x00090DB0 SetLightSettingCommand::Execute
void SetLightSettingCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SunLightSettings* light = GetSunLightSettings();
    if (!light)
        return;
    if (m_hasSetting && m_setting != 0) {
        if (light->mode == 0)
            ApplyPresetMode0(*light, m_setting);
        else if (light->mode == 1)
            ApplyPresetMode1(*light, m_setting);
    }
    if (m_hasDirection)
        light->direction = m_direction;
}


// ---------------------------------------------------------------------------
// Hash 0x92C1D3B7 (name unknown): runs a script on a part of an entity's skin.

class SetSkinScriptCommand : public ApeAnimScriptCommand {
public:
    SetSkinScriptCommand();
    ~SetSkinScriptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    uint32_t m_script = 0;    // +0x14 "Script"
    int32_t m_partIndex = 0;  // +0x18 "PartIndex"
};

// @func 0x00090E74 SetSkinScriptCommand::SetSkinScriptCommand
SetSkinScriptCommand::SetSkinScriptCommand() = default;

// @func 0x0073F3B8 SetSkinScriptCommand::~SetSkinScriptCommand
// @func 0x0073F3E8 SetSkinScriptCommand::~SetSkinScriptCommand (deleting)
SetSkinScriptCommand::~SetSkinScriptCommand() = default;

// @func 0x00090EC4 SetSkinScriptCommand::Create
ApeAnimScriptCommand* SetSkinScriptCommand::Create(XmlElement* element) const
{
    auto* command = new SetSkinScriptCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrScript, &command->m_script, command->m_script);
    element->GetInt(kAttrPartIndex, &command->m_partIndex, command->m_partIndex);
    return command;
}

// @func 0x00090FE0 SetSkinScriptCommand::Execute
void SetSkinScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity)
        return;
    if (void* skin = EntitySkin(entity))
        SkinSetScript(skin, m_script, m_partIndex);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsS()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0x3292B062u, new PlayerAbilityCommand());
    // #152 "SetPos" (0x9195F6B2) is not ported yet.
    prototypes.Add(0x80866784u, new SetLockedCommand());
    prototypes.Add(0x02C1266Au, new SetTargetingDurationCommand());
    prototypes.Add(0xD1BB9067u, new SetGravityCommand());
    prototypes.Add(0xACE05706u, new SetEntitySplineCommand());
    prototypes.Add(0x908A1BE5u, new SetRenderVisibleCommand());
    prototypes.Add(0x6F37AF4Cu, new SetLightSettingCommand());
    prototypes.Add(0x92C1D3B7u, new SetSkinScriptCommand());
}

} // namespace levels
} // namespace sly
