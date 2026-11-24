// openTIT - Game script commands, second alphabetical block (Detach... to Enqueue...).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrEaseIn = AnimHash("EaseIn");
constexpr uint32_t kAttrEaseOut = AnimHash("EaseOut");
constexpr uint32_t kAttrAmplitude = AnimHash("Amplitude");
constexpr uint32_t kAttrRadius = AnimHash("Radius");
constexpr uint32_t kAttrFreq = AnimHash("Freq");
constexpr uint32_t kAttrWaveOffset = AnimHash("WaveOffset");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrCollisionIndex = 0x82B8846Au;  // NOTE: name unknown, 1-based in data
constexpr uint32_t kAttrVar = AnimHash("var");
constexpr uint32_t kAttrEntityChild = AnimHash("EntityChild");
constexpr uint32_t kAttrComponent = AnimHash("component");
constexpr uint32_t kAttrDistance = AnimHash("distance");
constexpr uint32_t kAttrRamp = AnimHash("Ramp");
constexpr uint32_t kAttrMinValue = AnimHash("MinValue");
constexpr uint32_t kAttrLaserGroup = AnimHash("LaserGroup");
constexpr uint32_t kAttrLaserGroupVar = 0xABB2A7F3u;  // NOTE: name unknown
constexpr uint32_t kAttrLaserInst = AnimHash("LaserInst");
constexpr uint32_t kAttrLaser18 = 0x26AA71F6u;
constexpr uint32_t kAttrLaser1C = 0x6E6A93D5u;
constexpr uint32_t kAttrLaser20 = 0x448AAB12u;
constexpr uint32_t kAttrReset = AnimHash("Reset");
constexpr uint32_t kAttrLaser26 = 0x9C564761u;
constexpr uint32_t kAttrLocked = AnimHash("locked");
constexpr uint32_t kAttrSpawner = AnimHash("Spawner");
constexpr uint32_t kAttrSpawnerVar = AnimHash("SpawnerVar");
constexpr uint32_t kAttrSpawnerGroup = AnimHash("SpawnerGroup");
constexpr uint32_t kAttrSpawnerGroupVar = 0xE4508D90u;  // NOTE: name unknown
constexpr uint32_t kAttrSpawnDisable = 0x6557DBDDu;     // NOTE: name unknown, defaults to !enable
constexpr uint32_t kAttrSpawn16 = 0x03A6C100u;
constexpr uint32_t kAttrSpawn17 = 0xA92B6296u;
constexpr uint32_t kAttrSoundName = AnimHash("SoundName");
constexpr uint32_t kAttrSoundBank = AnimHash("SoundBank");
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrExclusivityGroupPriority = AnimHash("ExclusivityGroupPriority");
constexpr uint32_t kAttrSound20 = 0x6D18DE7Du;
constexpr const char* kAttrWorldRB = "worldRB";

constexpr float kHalfPi = 1.5707964f;
} // namespace

// ---------------------------------------------------------------------------
// Hash 0xE127428A (name unknown): detaches an entity from whatever carries it.

class DetachFromParentCommand : public ApeAnimScriptCommand {
public:
    DetachFromParentCommand();
    ~DetachFromParentCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;      // +0x0C "Entity"
    uint32_t m_refObject = 0;   // +0x10 "RefObject"
};

// @func 0x00071ED4 DetachFromParentCommand::DetachFromParentCommand
DetachFromParentCommand::DetachFromParentCommand() = default;

// @func 0x0073C4D8 DetachFromParentCommand::~DetachFromParentCommand
// @func 0x0073C508 DetachFromParentCommand::~DetachFromParentCommand (deleting)
DetachFromParentCommand::~DetachFromParentCommand() = default;

// @func 0x00071F1C DetachFromParentCommand::Create
ApeAnimScriptCommand* DetachFromParentCommand::Create(XmlElement* element) const
{
    auto* command = new DetachFromParentCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrRefObject, &command->m_refObject, command->m_refObject);
    return command;
}

// @func 0x00071FE4 DetachFromParentCommand::Execute
void DetachFromParentCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // Here the entity name is tried before the object variable.
    WorldObject* entity = m_entity ? FindEntity(m_entity) : nullptr;
    if (!entity && m_refObject)
        entity = EntityFromObject(instance->ResolveObject(m_refObject, false));
    if (!entity)
        return;
    if (WorldObject* parent = EntityAttachParent(entity))
        EntityDetach(parent, entity);
}

// ---------------------------------------------------------------------------
// Hash 0xEFE548E4 (name unknown): wobbles an object with an eased sine wave.

class WaveDistortCommand : public ApeAnimScriptCommand {
public:
    WaveDistortCommand();
    ~WaveDistortCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // +0x0C "object"
    float m_duration = 0.0f;      // +0x10 "duration"
    float m_easeIn = 0.0f;        // +0x14 "EaseIn"
    float m_easeOut = 0.0f;       // +0x18 "EaseOut"
    float m_amplitude = -1.0f;    // +0x1C "Amplitude"
    float m_radius = 2.0f;        // +0x20 "Radius"
    float m_freq = 0.0f;          // +0x24 "Freq"
    float m_waveOffset = kHalfPi; // +0x28 "WaveOffset"
};

// @func 0x000720C0 WaveDistortCommand::WaveDistortCommand
WaveDistortCommand::WaveDistortCommand() = default;

// @func 0x0073C560 WaveDistortCommand::~WaveDistortCommand
// @func 0x0073C590 WaveDistortCommand::~WaveDistortCommand (deleting)
WaveDistortCommand::~WaveDistortCommand() = default;

// @func 0x00072138 WaveDistortCommand::Create
ApeAnimScriptCommand* WaveDistortCommand::Create(XmlElement* element) const
{
    auto* command = new WaveDistortCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetFloat(kAttrEaseIn, &command->m_easeIn, command->m_easeIn);
    element->GetFloat(kAttrEaseOut, &command->m_easeOut, command->m_easeOut);
    element->GetFloat(kAttrAmplitude, &command->m_amplitude, command->m_amplitude);
    element->GetFloat(kAttrRadius, &command->m_radius, command->m_radius);
    element->GetFloat(kAttrFreq, &command->m_freq, command->m_freq);
    element->GetFloat(kAttrWaveOffset, &command->m_waveOffset, command->m_waveOffset);
    return command;
}

// TODO(0x000722FC WaveDistortCommand::Execute) fills a wave description and
// hands it to 0x0042A248 (gameplay range).

// ---------------------------------------------------------------------------
// EnableCollision

class EnableCollisionCommand : public ApeAnimScriptCommand {
public:
    EnableCollisionCommand();
    ~EnableCollisionCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_rigidBody[0x40] = {};  // +0x0C "worldRB": rigid body name
    uint32_t m_entity = 0;        // +0x50 "Entity"
    uint32_t m_child = 0;         // +0x54 "Child"
    int32_t m_index = 0;          // +0x58 attribute 0x82B8846A (stored 0-based)
    bool m_enable = false;        // +0x5C "enable"
};

// @func 0x0007240C EnableCollisionCommand::EnableCollisionCommand
EnableCollisionCommand::EnableCollisionCommand() = default;

// @func 0x0073C5E8 EnableCollisionCommand::~EnableCollisionCommand
// @func 0x0073C618 EnableCollisionCommand::~EnableCollisionCommand (deleting)
EnableCollisionCommand::~EnableCollisionCommand() = default;

// @func 0x00072464 EnableCollisionCommand::Create
ApeAnimScriptCommand* EnableCollisionCommand::Create(XmlElement* element) const
{
    auto* command = new EnableCollisionCommand();
    const char* rigidBody = element->GetString(kAttrWorldRB);
    if (rigidBody && rigidBody[0]) {
        std::strncpy(command->m_rigidBody, rigidBody, sizeof(command->m_rigidBody) - 1);
        command->m_rigidBody[sizeof(command->m_rigidBody) - 1] = '\0';
    }
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    if (element->GetInt(kAttrCollisionIndex, &command->m_index, command->m_index) == ape::xml::kQuery_Ok)
        command->m_index -= 1;
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// TODO(0x000725E0 EnableCollisionCommand::Execute) finds the Havok rigid body.

// ---------------------------------------------------------------------------
// EnableComponent

class EnableComponentCommand : public ApeAnimScriptCommand {
public:
    EnableComponentCommand();
    ~EnableComponentCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_var = 0;          // +0x0C "var"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_entityChild = 0;  // +0x14 "EntityChild"
    uint32_t m_component = 0;    // +0x18 "component"
    bool m_enable = false;       // +0x1C "enable"
};

// @func 0x000727DC EnableComponentCommand::EnableComponentCommand
EnableComponentCommand::EnableComponentCommand() = default;

// @func 0x0073C670 EnableComponentCommand::~EnableComponentCommand
// @func 0x0073C6A0 EnableComponentCommand::~EnableComponentCommand (deleting)
EnableComponentCommand::~EnableComponentCommand() = default;

// @func 0x0007283C EnableComponentCommand::Create
ApeAnimScriptCommand* EnableComponentCommand::Create(XmlElement* element) const
{
    auto* command = new EnableComponentCommand();
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrEntityChild, &command->m_entityChild, command->m_entityChild);
    element->GetHash(kAttrComponent, &command->m_component, command->m_component);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// TODO(0x00072974 EnableComponentCommand::Execute) "AnimScriptCommandEnableComponent".

// ---------------------------------------------------------------------------
// Hash 0xC99BEAB1 (name unknown, between EnableComponent and EnableDarkScreen):
// sets the game world flag +0x12828D.

class EnableWorldFlagCommand : public ApeAnimScriptCommand {
public:
    EnableWorldFlagCommand();
    ~EnableWorldFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = false;  // +0x0C "enable"
};

// @func 0x00072B64 EnableWorldFlagCommand::EnableWorldFlagCommand
EnableWorldFlagCommand::EnableWorldFlagCommand() = default;

// @func 0x0073C6F8 EnableWorldFlagCommand::~EnableWorldFlagCommand
// @func 0x0073C728 EnableWorldFlagCommand::~EnableWorldFlagCommand (deleting)
EnableWorldFlagCommand::~EnableWorldFlagCommand() = default;

// @func 0x00072BAC EnableWorldFlagCommand::Create
ApeAnimScriptCommand* EnableWorldFlagCommand::Create(XmlElement* element) const
{
    auto* command = new EnableWorldFlagCommand();
    element->GetBool(kAttrEnable, &command->m_enable, false);
    return command;
}

// @func 0x00072C50 EnableWorldFlagCommand::Execute
void EnableWorldFlagCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SetWorldFlag12828D(m_enable);
}

// ---------------------------------------------------------------------------
// "EnableDarkScreen" (hash 0x4E92FD25; NOTE: name from a hash match).

class EnableDarkScreenCommand : public ApeAnimScriptCommand {
public:
    EnableDarkScreenCommand();
    ~EnableDarkScreenCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = false;      // +0x0C "enable"
    float m_distance = 0.0f;    // +0x10 "distance"
    float m_ramp = 1.0f;        // +0x14 "Ramp"
    float m_minValue = 0.0f;    // +0x18 "MinValue"
};

// @func 0x00072C6C EnableDarkScreenCommand::EnableDarkScreenCommand
EnableDarkScreenCommand::EnableDarkScreenCommand() = default;

// @func 0x0073C780 EnableDarkScreenCommand::~EnableDarkScreenCommand
// @func 0x0073C7B0 EnableDarkScreenCommand::~EnableDarkScreenCommand (deleting)
EnableDarkScreenCommand::~EnableDarkScreenCommand() = default;

// @func 0x00072CC8 EnableDarkScreenCommand::Create
ApeAnimScriptCommand* EnableDarkScreenCommand::Create(XmlElement* element) const
{
    auto* command = new EnableDarkScreenCommand();
    // NOTE: the console passes the same default (0) to every getter, so a
    // missing attribute stores 0 even for "Ramp".
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetFloat(kAttrDistance, &command->m_distance, 0.0f);
    element->GetFloat(kAttrRamp, &command->m_ramp, 0.0f);
    element->GetFloat(kAttrMinValue, &command->m_minValue, 0.0f);
    return command;
}

// @func 0x00072DF8 EnableDarkScreenCommand::Execute
void EnableDarkScreenCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SetDarkScreen(m_enable, m_distance, m_ramp, m_minValue);
}

// ---------------------------------------------------------------------------
// EnableLaser

class EnableLaserCommand : public ApeAnimScriptCommand {
public:
    EnableLaserCommand();
    ~EnableLaserCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_laserGroup = 0;     // +0x0C "LaserGroup"
    uint32_t m_laserGroupVar = 0;  // +0x10 attribute 0xABB2A7F3
    uint32_t m_laserInst = 0;      // +0x14 "LaserInst"
    uint32_t m_field18 = 0;        // +0x18 attribute 0x26AA71F6
    uint32_t m_field1C = 0;        // +0x1C attribute 0x6E6A93D5
    uint32_t m_field20 = 0;        // +0x20 attribute 0x448AAB12
    bool m_enable = false;         // +0x24 "enable"
    bool m_reset = false;          // +0x25 "Reset"
    bool m_flag26 = false;         // +0x26 attribute 0x9C564761
};

// @func 0x00072E38 EnableLaserCommand::EnableLaserCommand
EnableLaserCommand::EnableLaserCommand() = default;

// @func 0x0073C808 EnableLaserCommand::~EnableLaserCommand
// @func 0x0073C838 EnableLaserCommand::~EnableLaserCommand (deleting)
EnableLaserCommand::~EnableLaserCommand() = default;

// @func 0x00072EA8 EnableLaserCommand::Create
ApeAnimScriptCommand* EnableLaserCommand::Create(XmlElement* element) const
{
    auto* command = new EnableLaserCommand();
    element->GetHash(kAttrLaserGroup, &command->m_laserGroup, 0);
    element->GetHash(kAttrLaserGroupVar, &command->m_laserGroupVar, 0);
    element->GetHash(kAttrLaserInst, &command->m_laserInst, 0);
    element->GetHash(kAttrLaser18, &command->m_field18, 0);
    element->GetHash(kAttrLaser1C, &command->m_field1C, 0);
    element->GetHash(kAttrLaser20, &command->m_field20, 0);
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetBool(kAttrReset, &command->m_reset, false);
    element->GetBool(kAttrLaser26, &command->m_flag26, false);
    return command;
}

// TODO(0x00073078 EnableLaserCommand::Execute) laser field groups.

// ---------------------------------------------------------------------------
// Hash 0xC0F6B7A8 (name unknown, between EnableLaser and EnableSentryLight):
// sets the global switch at 0x010AF1BC.

class EnableGlobalSwitchCommand : public ApeAnimScriptCommand {
public:
    EnableGlobalSwitchCommand();
    ~EnableGlobalSwitchCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_enable = true;  // +0x0C "enable"
};

// @func 0x00073498 EnableGlobalSwitchCommand::EnableGlobalSwitchCommand
EnableGlobalSwitchCommand::EnableGlobalSwitchCommand() = default;

// @func 0x0073B5D8 EnableGlobalSwitchCommand::~EnableGlobalSwitchCommand
// @func 0x0073C890 EnableGlobalSwitchCommand::~EnableGlobalSwitchCommand (deleting)
EnableGlobalSwitchCommand::~EnableGlobalSwitchCommand() = default;

// @func 0x000734E0 EnableGlobalSwitchCommand::Create
ApeAnimScriptCommand* EnableGlobalSwitchCommand::Create(XmlElement* element) const
{
    auto* command = new EnableGlobalSwitchCommand();
    element->GetBool(kAttrEnable, &command->m_enable, true);
    return command;
}

// @func 0x00073588 EnableGlobalSwitchCommand::Execute
void EnableGlobalSwitchCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SetGlobalSwitch010AF1BC(m_enable);
}

// ---------------------------------------------------------------------------
// "EnableSentryLight" (hash 0x54836B14; NOTE: name from a hash match).

class EnableSentryLightCommand : public ApeAnimScriptCommand {
public:
    EnableSentryLightCommand();
    ~EnableSentryLightCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    bool m_enable = true;      // +0x14 "enable"
    bool m_hasEnable = false;  // +0x15
    bool m_locked = false;     // +0x16 "locked"
    bool m_hasLocked = false;  // +0x17
};

// @func 0x00073598 EnableSentryLightCommand::EnableSentryLightCommand
EnableSentryLightCommand::EnableSentryLightCommand() = default;

// @func 0x0073C8E8 EnableSentryLightCommand::~EnableSentryLightCommand
// @func 0x0073C918 EnableSentryLightCommand::~EnableSentryLightCommand (deleting)
EnableSentryLightCommand::~EnableSentryLightCommand() = default;

// @func 0x00073600 EnableSentryLightCommand::Create
ApeAnimScriptCommand* EnableSentryLightCommand::Create(XmlElement* element) const
{
    auto* command = new EnableSentryLightCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    if (element->GetBool(kAttrEnable, &command->m_enable, command->m_enable) == ape::xml::kQuery_Ok)
        command->m_hasEnable = true;
    if (element->GetBool(kAttrLocked, &command->m_locked, command->m_locked) == ape::xml::kQuery_Ok)
        command->m_hasLocked = true;
    return command;
}

// TODO(0x00073748 EnableSentryLightCommand::Execute)

// ---------------------------------------------------------------------------
// EnableSpawner / "EnableSpawnerGroup" (hash 0x3F74DF1A; name from a hash match)

class EnableSpawnerCommand : public ApeAnimScriptCommand {
public:
    EnableSpawnerCommand();
    ~EnableSpawnerCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_spawner = 0;     // +0x0C "Spawner"
    uint32_t m_spawnerVar = 0;  // +0x10 "SpawnerVar"
    bool m_enable = true;       // +0x14 "enable"
    bool m_disable = true;      // +0x15 attribute 0x6557DBDD (defaults to !enable)
    bool m_flag16 = false;      // +0x16 attribute 0x03A6C100
    bool m_flag17 = false;      // +0x17 attribute 0xA92B6296
    bool m_reset = false;       // +0x18 "Reset"
};

// @func 0x000738B0 EnableSpawnerCommand::EnableSpawnerCommand
EnableSpawnerCommand::EnableSpawnerCommand() = default;

// @func 0x0073C970 EnableSpawnerCommand::~EnableSpawnerCommand
// @func 0x0073C9A0 EnableSpawnerCommand::~EnableSpawnerCommand (deleting)
EnableSpawnerCommand::~EnableSpawnerCommand() = default;

// @func 0x00073910 EnableSpawnerCommand::Create
ApeAnimScriptCommand* EnableSpawnerCommand::Create(XmlElement* element) const
{
    auto* command = new EnableSpawnerCommand();
    element->GetHash(kAttrSpawner, &command->m_spawner, 0);
    element->GetHash(kAttrSpawnerVar, &command->m_spawnerVar, command->m_spawnerVar);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    command->m_disable = !command->m_enable;
    element->GetBool(kAttrSpawnDisable, &command->m_disable, command->m_disable);
    element->GetBool(kAttrSpawn16, &command->m_flag16, command->m_flag16);
    element->GetBool(kAttrSpawn17, &command->m_flag17, command->m_flag17);
    element->GetBool(kAttrReset, &command->m_reset, command->m_reset);
    return command;
}

// TODO(0x00073AA8 EnableSpawnerCommand::Execute)

class EnableSpawnerGroupCommand : public ApeAnimScriptCommand {
public:
    EnableSpawnerGroupCommand();
    ~EnableSpawnerGroupCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_group = 0;      // +0x0C "SpawnerGroup"
    uint32_t m_groupVar = 0;   // +0x10 attribute 0xE4508D90
    bool m_enable = true;      // +0x14 "enable"
    bool m_disable = true;     // +0x15 attribute 0x6557DBDD (defaults to !enable)
    bool m_flag16 = false;     // +0x16 attribute 0x03A6C100
    bool m_reset = false;      // +0x17 "Reset"
    bool m_flag18 = false;     // +0x18 attribute 0xA92B6296
};

// @func 0x00073CB8 EnableSpawnerGroupCommand::EnableSpawnerGroupCommand
EnableSpawnerGroupCommand::EnableSpawnerGroupCommand() = default;

// @func 0x0073CA28 EnableSpawnerGroupCommand::~EnableSpawnerGroupCommand
// @func 0x0073CA58 EnableSpawnerGroupCommand::~EnableSpawnerGroupCommand (deleting)
EnableSpawnerGroupCommand::~EnableSpawnerGroupCommand() = default;

// @func 0x00073D18 EnableSpawnerGroupCommand::Create
ApeAnimScriptCommand* EnableSpawnerGroupCommand::Create(XmlElement* element) const
{
    auto* command = new EnableSpawnerGroupCommand();
    element->GetHash(kAttrSpawnerGroup, &command->m_group, 0);
    element->GetHash(kAttrSpawnerGroupVar, &command->m_groupVar, command->m_groupVar);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    command->m_disable = !command->m_enable;
    element->GetBool(kAttrSpawnDisable, &command->m_disable, command->m_disable);
    element->GetBool(kAttrSpawn16, &command->m_flag16, command->m_flag16);
    element->GetBool(kAttrReset, &command->m_reset, command->m_reset);
    element->GetBool(kAttrSpawn17, &command->m_flag18, command->m_flag18);
    return command;
}

// TODO(0x00073EAC EnableSpawnerGroupCommand::Execute)

// ---------------------------------------------------------------------------
// EnqueueSound

class EnqueueSoundCommand : public ApeAnimScriptCommand {
public:
    EnqueueSoundCommand();
    ~EnqueueSoundCommand() override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    // Looks the sound up again unless the held reference is still valid.
    void RefreshSound();

    uint32_t m_soundName = 0;               // +0x0C "SoundName"
    uint32_t m_soundBank = 0;               // +0x10 "SoundBank"
    audio::SoundName* m_sound = nullptr;    // +0x14
    uint32_t m_exclusivityGroup = 0;        // +0x18 "ExclusivityGroup"
    int32_t m_exclusivityPriority = 0;      // +0x1C "ExclusivityGroupPriority"
    bool m_flag20 = false;                  // +0x20 attribute 0x6D18DE7D
};

// @func 0x00074058 EnqueueSoundCommand::EnqueueSoundCommand
EnqueueSoundCommand::EnqueueSoundCommand() = default;

void EnqueueSoundCommand::RefreshSound()
{
    if (m_soundName && !audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_soundName, m_soundBank);
    }
}

// @func 0x000740B0 EnqueueSoundCommand::Link
void EnqueueSoundCommand::Link()
{
    RefreshSound();
}

// @func 0x00074118 EnqueueSoundCommand::~EnqueueSoundCommand
// @func 0x00074164 EnqueueSoundCommand::~EnqueueSoundCommand (deleting)
EnqueueSoundCommand::~EnqueueSoundCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x000741D8 EnqueueSoundCommand::Create
ApeAnimScriptCommand* EnqueueSoundCommand::Create(XmlElement* element) const
{
    auto* command = new EnqueueSoundCommand();
    element->GetHash(kAttrSoundName, &command->m_soundName, 0);
    element->GetHash(kAttrSoundBank, &command->m_soundBank, command->m_soundBank);
    element->GetHash(kAttrExclusivityGroup, &command->m_exclusivityGroup, command->m_exclusivityGroup);
    element->GetInt(kAttrExclusivityGroupPriority, &command->m_exclusivityPriority,
                    command->m_exclusivityPriority);
    element->GetBool(kAttrSound20, &command->m_flag20, command->m_flag20);
    command->RefreshSound();
    return command;
}

// TODO(0x00074354 EnqueueSoundCommand::Execute) "EnqueueSound fails to find sound %s (in bank %s)".

// ---------------------------------------------------------------------------

void RegisterScriptCommandsE()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xE127428Au, new DetachFromParentCommand());
    prototypes.Add(0xEFE548E4u, new WaveDistortCommand());
    prototypes.Add(AnimHash("EnableCollision"), new EnableCollisionCommand());
    prototypes.Add(AnimHash("EnableComponent"), new EnableComponentCommand());
    prototypes.Add(0xC99BEAB1u, new EnableWorldFlagCommand());
    prototypes.Add(AnimHash("EnableDarkScreen"), new EnableDarkScreenCommand());
    prototypes.Add(AnimHash("EnableLaser"), new EnableLaserCommand());
    prototypes.Add(0xC0F6B7A8u, new EnableGlobalSwitchCommand());
    prototypes.Add(AnimHash("EnableSentryLight"), new EnableSentryLightCommand());
    prototypes.Add(AnimHash("EnableSpawner"), new EnableSpawnerCommand());
    prototypes.Add(AnimHash("EnableSpawnerGroup"), new EnableSpawnerGroupCommand());
    prototypes.Add(AnimHash("EnqueueSound"), new EnqueueSoundCommand());
}

} // namespace levels
} // namespace sly
