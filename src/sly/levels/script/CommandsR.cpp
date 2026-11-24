// openTIT - Game script commands, sixteenth block (SetMusic to the player mount command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/PlayerNotifyFlags.h"
#include "sly/levels/ScriptServices.h"

#include <algorithm>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrMusic = AnimHash("Music");
constexpr uint32_t kAttrBank = AnimHash("Bank");
constexpr uint32_t kAttrMusic18 = 0x83F172B0u;  // NOTE: name unknown, default true
constexpr uint32_t kAttrMusic19 = 0x0138B87Fu;  // NOTE: name unknown
constexpr uint32_t kAttrRegister = AnimHash("Register");
constexpr uint32_t kAttrValue = AnimHash("value");
constexpr uint32_t kAttrRegisterTime = 0x21CA4030u;  // NOTE: name unknown (float)
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrEntityChild = AnimHash("EntityChild");
constexpr uint32_t kAttrEnabled = AnimHash("enabled");
constexpr uint32_t kAttrEnabled19 = 0xF5897C08u;  // NOTE: name unknown
constexpr uint32_t kAttrNotify = 0xD782B635u;     // NOTE: name unknown (flag name)
constexpr uint32_t kAttrPlayerIdx = AnimHash("PlayerIdx");
constexpr uint32_t kAttrSet = AnimHash("Set");
constexpr uint32_t kAttrColor = AnimHash("Color");
constexpr uint32_t kAttrThickness = AnimHash("thickness");
constexpr uint32_t kAttrPlayerIndex = AnimHash("PlayerIndex");
constexpr uint32_t kAttrEntityDef = AnimHash("EntityDef");
constexpr uint32_t kAttrPlayerEntity14 = 0x8C830DC3u;  // NOTE: name unknown
constexpr uint32_t kAttrSpawnPt = AnimHash("SpawnPT");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrMount18 = 0xD7718094u;  // NOTE: name unknown; picks the second call

constexpr ape::xml::Color8 kDefaultOutline = {0xFF, 0xFF, 0xFF, 0xFF};  // NOTE: console global 0x01164CA0

// Bit 28 of the killable entity status word +0x178 switched by command 0x8B4BC0F7.
constexpr uint32_t kStatusBit28 = 0x10000000u;
// Class flag (+0xC4) of entities that can be mounted by a player.
constexpr uint32_t kEntityClass3_Mountable = 0x40000000u;
} // namespace

// ---------------------------------------------------------------------------
// "SetMusic"

class SetMusicCommand : public ApeAnimScriptCommand {
public:
    SetMusicCommand();
    ~SetMusicCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    void RefreshSound();

    uint32_t m_music = 0;                 // +0x0C "Music"
    uint32_t m_bank = 0;                  // +0x10 "Bank"
    audio::SoundName* m_sound = nullptr;  // +0x14
    bool m_flag18 = true;                 // +0x18 attribute 0x83F172B0
    bool m_flag19 = false;                // +0x19 attribute 0x0138B87F
};

// @func 0x0008C428 SetMusicCommand::SetMusicCommand
SetMusicCommand::SetMusicCommand() = default;

void SetMusicCommand::RefreshSound()
{
    if (!audio::SoundNameHasDef(m_sound)) {
        audio::SoundNameRelease(m_sound);
        m_sound = FindSoundName(m_music, m_bank);
    }
}

// @func 0x0008C480 SetMusicCommand::Link
void SetMusicCommand::Link()
{
    RefreshSound();
}

// @func 0x0073EE80 SetMusicCommand::~SetMusicCommand
// @func 0x0073EED8 SetMusicCommand::~SetMusicCommand (deleting)
SetMusicCommand::~SetMusicCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x0008C4E4 SetMusicCommand::Create
ApeAnimScriptCommand* SetMusicCommand::Create(XmlElement* element) const
{
    auto* command = new SetMusicCommand();
    element->GetHash(kAttrMusic, &command->m_music, 0);
    element->GetHash(kAttrBank, &command->m_bank, 0);
    element->GetBool(kAttrMusic18, &command->m_flag18, command->m_flag18);
    element->GetBool(kAttrMusic19, &command->m_flag19, command->m_flag19);
    command->RefreshSound();
    return command;
}

// @func 0x0008C62C SetMusicCommand::Execute
void SetMusicCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!audio::SoundNameHasDef(m_sound)) {
        if (g_scriptWarnings)
            ape::TempFormat("SetMusic fails to find sound %s (in bank %s)", ape::TempHexString(m_music),
                            ape::TempHexString(m_bank));
        return;
    }
    MusicPlay(m_sound, m_flag18, m_flag19);
}

// ---------------------------------------------------------------------------
// "SetMusicRegister" (NOTE: name from a hash match): sets an interactive
// music register.

class SetMusicRegisterCommand : public ApeAnimScriptCommand {
public:
    SetMusicRegisterCommand();
    explicit SetMusicRegisterCommand(XmlElement* element);
    ~SetMusicRegisterCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_register = -1;  // +0x0C "Register" (1-based in data)
    int32_t m_value = 0;      // +0x10 "value" (-128..127)
    float m_time = 0.0f;      // +0x14 attribute 0x21CA4030
};

// @func 0x0008C6DC SetMusicRegisterCommand::SetMusicRegisterCommand
SetMusicRegisterCommand::SetMusicRegisterCommand() = default;

// @func 0x0073B800 SetMusicRegisterCommand::~SetMusicRegisterCommand
// @func 0x0073EF4C SetMusicRegisterCommand::~SetMusicRegisterCommand (deleting)
SetMusicRegisterCommand::~SetMusicRegisterCommand() = default;

// @func 0x0008C72C SetMusicRegisterCommand::SetMusicRegisterCommand
SetMusicRegisterCommand::SetMusicRegisterCommand(XmlElement* element)
{
    element->GetInt(kAttrRegister, &m_register, -1);
    element->GetInt(kAttrValue, &m_value, m_value);
    element->GetFloat(kAttrRegisterTime, &m_time, m_time);
    --m_register;
    m_value = std::max(-128, std::min(127, m_value));
}

// @func 0x0008C834 SetMusicRegisterCommand::Create
ApeAnimScriptCommand* SetMusicRegisterCommand::Create(XmlElement* element) const
{
    return new SetMusicRegisterCommand(element);
}

// @func 0x0008C8A4 SetMusicRegisterCommand::Execute
void SetMusicRegisterCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    MusicSetRegister(m_register, static_cast<float>(m_value), m_time);
}

// ---------------------------------------------------------------------------
// Hash 0x185B0922 (name unknown): a placeholder whose Create builds nothing,
// so the element is dropped from the script.

class NullCommand : public ApeAnimScriptCommand {
public:
    NullCommand();
    ~NullCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x0008C8F0 NullCommand::NullCommand
NullCommand::NullCommand() = default;

// @func 0x0073B830 NullCommand::~NullCommand
// @func 0x0073EFA8 NullCommand::~NullCommand (deleting)
NullCommand::~NullCommand() = default;

// @func 0x0008C938 NullCommand::Create
ApeAnimScriptCommand* NullCommand::Create(XmlElement*) const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// Hash 0x8B4BC0F7 (name unknown): switches bit 28 of a killable entity's
// status word.

class EntityStatusBitCommand : public ApeAnimScriptCommand {
public:
    EntityStatusBitCommand();
    ~EntityStatusBitCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;       // +0x0C "object"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_entityChild = 0;  // +0x14 "EntityChild"
    bool m_enabled = false;      // +0x18 "enabled"
    bool m_flag19 = false;       // +0x19 attribute 0xF5897C08 (parsed, unused)
};

// @func 0x0008C94C EntityStatusBitCommand::EntityStatusBitCommand
EntityStatusBitCommand::EntityStatusBitCommand() = default;

// @func 0x0073F000 EntityStatusBitCommand::~EntityStatusBitCommand
// @func 0x0073F030 EntityStatusBitCommand::~EntityStatusBitCommand (deleting)
EntityStatusBitCommand::~EntityStatusBitCommand() = default;

// @func 0x0008C9A4 EntityStatusBitCommand::Create
ApeAnimScriptCommand* EntityStatusBitCommand::Create(XmlElement* element) const
{
    auto* command = new EntityStatusBitCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrEntityChild, &command->m_entityChild, command->m_entityChild);
    element->GetBool(kAttrEnabled, &command->m_enabled, command->m_enabled);
    element->GetBool(kAttrEnabled19, &command->m_flag19, command->m_flag19);
    return command;
}

// @func 0x0008CAE8 EntityStatusBitCommand::Execute
void EntityStatusBitCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = EntityFromVar(instance, m_object);
    if (!entity && m_entity != 0)
        entity = ResolveEventEntity(instance, m_entity);
    if (entity && m_entityChild != 0)
        entity = EntityFindChild(entity, m_entityChild);
    if (entity && EntityHasClassFlag(entity, kEntityClass_Killable))
        EntitySetStatusBits(entity, kStatusBit28, m_enabled);
}

// ---------------------------------------------------------------------------
// "SetNotifyFlag" (NOTE: name from a hash match)

class SetNotifyFlagCommand : public ApeAnimScriptCommand {
public:
    SetNotifyFlagCommand();
    ~SetNotifyFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_flag = 0;     // +0x0C attribute 0xD782B635
    int32_t m_playerIdx = 0; // +0x10 "PlayerIdx" (1-based in data)
    bool m_set = false;      // +0x14 "Set"
};

// @func 0x0008CC60 SetNotifyFlagCommand::SetNotifyFlagCommand
SetNotifyFlagCommand::SetNotifyFlagCommand() = default;

// @func 0x0073F088 SetNotifyFlagCommand::~SetNotifyFlagCommand
// @func 0x0073F0B8 SetNotifyFlagCommand::~SetNotifyFlagCommand (deleting)
SetNotifyFlagCommand::~SetNotifyFlagCommand() = default;

// @func 0x0008CCB8 SetNotifyFlagCommand::Create
ApeAnimScriptCommand* SetNotifyFlagCommand::Create(XmlElement* element) const
{
    auto* command = new SetNotifyFlagCommand();
    element->GetHash(kAttrNotify, &command->m_flag, 0);
    if (element->GetInt(kAttrPlayerIdx, &command->m_playerIdx, command->m_playerIdx) == ape::xml::kQuery_Ok &&
        command->m_playerIdx > 0)
        --command->m_playerIdx;
    element->GetBool(kAttrSet, &command->m_set, command->m_set);
    return command;
}

// @func 0x0008CDCC SetNotifyFlagCommand::Execute
void SetNotifyFlagCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_flag == 0)
        return;
    if (PlayerNotifyFlags* flags = PlayerNotifyFlagsOf(m_playerIdx))
        flags->Set(m_flag, m_set);
}

// ---------------------------------------------------------------------------
// "SetOutlineColor" (NOTE: name from a hash match)

class SetOutlineColorCommand : public ApeAnimScriptCommand {
public:
    SetOutlineColorCommand();
    ~SetOutlineColorCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;                        // +0x0C "object" (object variable)
    ape::xml::Color8 m_color = kDefaultOutline;   // +0x10 "Color"
    float m_thickness = 7.25f;                    // +0x14 "thickness"
};

// @func 0x0008CE40 SetOutlineColorCommand::SetOutlineColorCommand
SetOutlineColorCommand::SetOutlineColorCommand() = default;

// @func 0x0008CEC8 SetOutlineColorCommand::~SetOutlineColorCommand
// @func 0x0008CEF8 SetOutlineColorCommand::~SetOutlineColorCommand (deleting)
SetOutlineColorCommand::~SetOutlineColorCommand() = default;

// @func 0x0008CF50 SetOutlineColorCommand::Create
ApeAnimScriptCommand* SetOutlineColorCommand::Create(XmlElement* element) const
{
    auto* command = new SetOutlineColorCommand();
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    element->GetColor(kAttrColor, &command->m_color, &command->m_color);
    element->GetFloat(kAttrThickness, &command->m_thickness, command->m_thickness);
    return command;
}

// @func 0x0008D070 SetOutlineColorCommand::Execute
void SetOutlineColorCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeRefCounted* object = ObjectFromVar(instance, m_object);
    if (!object)
        return;
    if (WorldObject* entity = EntityFromObject(object))
        EntitySetOutline(entity, m_color, m_thickness);
    else
        ObjectSetOutline(object, m_color, m_thickness);
}

// ---------------------------------------------------------------------------
// "SetPlayerEntity" (NOTE: name from a hash match): switches a player to
// another character definition.

class SetPlayerEntityCommand : public ApeAnimScriptCommand {
public:
    SetPlayerEntityCommand();
    ~SetPlayerEntityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerIndex = -1;  // +0x0C "PlayerIndex"
    uint32_t m_entityDef = 0;    // +0x10 "EntityDef"
    bool m_flag14 = false;       // +0x14 attribute 0x8C830DC3
};

// @func 0x0008D16C SetPlayerEntityCommand::SetPlayerEntityCommand
SetPlayerEntityCommand::SetPlayerEntityCommand() = default;

// @func 0x0008D1C4 SetPlayerEntityCommand::~SetPlayerEntityCommand
// @func 0x0008D1F4 SetPlayerEntityCommand::~SetPlayerEntityCommand (deleting)
SetPlayerEntityCommand::~SetPlayerEntityCommand() = default;

// @func 0x0008D24C SetPlayerEntityCommand::Create
ApeAnimScriptCommand* SetPlayerEntityCommand::Create(XmlElement* element) const
{
    auto* command = new SetPlayerEntityCommand();
    element->GetInt(kAttrPlayerIndex, &command->m_playerIndex, -1);
    element->GetHash(kAttrEntityDef, &command->m_entityDef, 0);
    element->GetBool(kAttrPlayerEntity14, &command->m_flag14, false);
    return command;
}

// @func 0x0008D344 SetPlayerEntityCommand::Execute
void SetPlayerEntityCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_playerIndex >= 0 && m_entityDef != 0)
        GameWorldSetPlayerEntity(m_playerIndex, m_entityDef, m_flag14);
}

// ---------------------------------------------------------------------------
// Hash 0x80E15258 (name unknown): moves a player to a spawn point.

class SetPlayerSpawnCommand : public ApeAnimScriptCommand {
public:
    SetPlayerSpawnCommand();
    ~SetPlayerSpawnCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerIndex = -1;  // +0x0C "PlayerIndex"
    uint32_t m_spawnPt = 0;      // +0x10 "SpawnPT"
};

// @func 0x0008D3B0 SetPlayerSpawnCommand::SetPlayerSpawnCommand
SetPlayerSpawnCommand::SetPlayerSpawnCommand() = default;

// @func 0x0008D408 SetPlayerSpawnCommand::~SetPlayerSpawnCommand
// @func 0x0008D438 SetPlayerSpawnCommand::~SetPlayerSpawnCommand (deleting)
SetPlayerSpawnCommand::~SetPlayerSpawnCommand() = default;

// @func 0x0008D490 SetPlayerSpawnCommand::Create
ApeAnimScriptCommand* SetPlayerSpawnCommand::Create(XmlElement* element) const
{
    auto* command = new SetPlayerSpawnCommand();
    element->GetInt(kAttrPlayerIndex, &command->m_playerIndex, -1);
    element->GetHash(kAttrSpawnPt, &command->m_spawnPt, 0);
    return command;
}

// TODO(0x0008D564 SetPlayerSpawnCommand::Execute) copies the point instance's
// transform (point list 0x00F8F154) and calls 0x00488298 on the world; the
// transform layout passed is not decoded.

// ---------------------------------------------------------------------------
// Hash 0xA1BA0E32 (name unknown): mounts or dismounts a player on an entity.

class PlayerMountCommand : public ApeAnimScriptCommand {
public:
    PlayerMountCommand();
    ~PlayerMountCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    int32_t m_playerIdx = 0;   // +0x14 "PlayerIdx"
    bool m_flag18 = false;     // +0x18 attribute 0xD7718094
};

// @func 0x0008D6C0 PlayerMountCommand::PlayerMountCommand
PlayerMountCommand::PlayerMountCommand() = default;

// @func 0x0008D710 PlayerMountCommand::~PlayerMountCommand
// @func 0x0008D740 PlayerMountCommand::~PlayerMountCommand (deleting)
PlayerMountCommand::~PlayerMountCommand() = default;

// @func 0x0008D798 PlayerMountCommand::Create
ApeAnimScriptCommand* PlayerMountCommand::Create(XmlElement* element) const
{
    auto* command = new PlayerMountCommand();
    if (element->GetHash(kAttrEntity, &command->m_entity, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrRefObject, &command->m_refObject, 0) != ape::xml::kQuery_Ok)
        return command;
    element->GetInt(kAttrPlayerIdx, &command->m_playerIdx, command->m_playerIdx);
    element->GetBool(kAttrMount18, &command->m_flag18, command->m_flag18);
    return command;
}

// @func 0x0008D8C4 PlayerMountCommand::Execute
void PlayerMountCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_refObject, m_entity);
    if (!entity || !EntityHasClassFlag3(entity, kEntityClass3_Mountable))
        return;
    WorldObject* player = PlayerEntityByIndex(m_playerIdx);
    if (m_flag18)
        MountableAttachPlayer(entity, player);
    else
        MountableDetachPlayer(entity, player);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsR()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SetMusic"), new SetMusicCommand());
    prototypes.Add(AnimHash("SetMusicRegister"), new SetMusicRegisterCommand());
    prototypes.Add(0x185B0922u, new NullCommand());
    prototypes.Add(0x8B4BC0F7u, new EntityStatusBitCommand());
    prototypes.Add(AnimHash("SetNotifyFlag"), new SetNotifyFlagCommand());
    prototypes.Add(AnimHash("SetOutlineColor"), new SetOutlineColorCommand());
    prototypes.Add(AnimHash("SetPlayerEntity"), new SetPlayerEntityCommand());
    prototypes.Add(0x80E15258u, new SetPlayerSpawnCommand());
    prototypes.Add(0xA1BA0E32u, new PlayerMountCommand());
}

} // namespace levels
} // namespace sly
