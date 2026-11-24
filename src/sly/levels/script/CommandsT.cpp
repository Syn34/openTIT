// openTIT - Game script commands, eighteenth block (SetSoundRegister to SmokeBomb).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
// Default position of the smoke cloud (shared vector 0x01164B40). NOTE: assumed zero.
const ape::Vec3 kSmokeOrigin = {0.0f, 0.0f, 0.0f, 0.0f};
constexpr uint32_t kAttrGlobalValue = 0xA016BE6Fu;   // NOTE: name unknown (float)
constexpr uint32_t kAttrScale = AnimHash("Scale");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrTimeScale12 = 0x8FE1FDC1u;  // NOTE: name unknown
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrMaterialHash = AnimHash("materialHash");
constexpr uint32_t kAttrUSpeed = AnimHash("USpeed");
constexpr uint32_t kAttrVSpeed = AnimHash("VSpeed");
constexpr uint32_t kAttrScript = AnimHash("Script");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrWeapon14 = 0x6EBFEF2Bu;     // NOTE: name unknown (parsed, unused)
constexpr uint32_t kAttrShoot = AnimHash("Shoot");
constexpr uint32_t kAttrPromptX = 0x36F05514u;      // NOTE: name unknown; show the X button line
constexpr uint32_t kAttrPromptCircle = 0x21BE4C63u; // NOTE: name unknown; show the circle button line
constexpr uint32_t kAttrPromptXText = 0x3CD0795Au;  // NOTE: name unknown
constexpr uint32_t kAttrPromptCircleText = 0xC2803EA7u;  // NOTE: name unknown
constexpr uint32_t kAttrForce = AnimHash("force");
constexpr uint32_t kAttrOwner = AnimHash("owner");
constexpr uint32_t kAttrRadius = AnimHash("radius");
constexpr uint32_t kAttrMode = AnimHash("mode");

// Time scale channels of the game world, by index (console table 0x00AC55F4).
// NOTE: the names behind the hashes are not known.
constexpr uint32_t kTimeScaleTypes[] = {0x000008D2u, 0x76D1F2F4u};
constexpr uint16_t kNoTimeScaleType = 0xFFFF;
} // namespace

// TODO(0x00091178 SetSoundRegisterCommand::SetSoundRegisterCommand) "SetSoundRegister" (#160):
// register number/value parser with range warnings; Execute 0x0009170C.

// ---------------------------------------------------------------------------
// Hash 0xB6CD6FF1 (name unknown): sets a global float (console 0x0108562C)
// when the attribute is present.

class SetGlobalValueCommand : public ApeAnimScriptCommand {
public:
    SetGlobalValueCommand();
    ~SetGlobalValueCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_mask = 0;   // +0x0C bit 0: value present
    float m_value = 0.0f;  // +0x10 attribute 0xA016BE6F
};

// @func 0x00091BBC SetGlobalValueCommand::SetGlobalValueCommand
SetGlobalValueCommand::SetGlobalValueCommand() = default;

// @func 0x0073F440 SetGlobalValueCommand::~SetGlobalValueCommand
// @func 0x0073F470 SetGlobalValueCommand::~SetGlobalValueCommand (deleting)
SetGlobalValueCommand::~SetGlobalValueCommand() = default;

// @func 0x00091C08 SetGlobalValueCommand::Create
ApeAnimScriptCommand* SetGlobalValueCommand::Create(XmlElement* element) const
{
    auto* command = new SetGlobalValueCommand();
    float value = 0.0f;
    if (element->GetFloat(kAttrGlobalValue, &value, 0.0f) == ape::xml::kQuery_Ok) {
        command->m_value = value;
        command->m_mask |= 1;
    }
    return command;
}

// @func 0x00091CD0 SetGlobalValueCommand::Execute
void SetGlobalValueCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_mask & 1)
        SetGlobalValue0108562C(m_value);
}

// ---------------------------------------------------------------------------
// "SetTimeScale" (NOTE: name from a hash match): scales the time of an
// entity, or of one of the world's time channels.

class SetTimeScaleCommand : public ApeAnimScriptCommand {
public:
    SetTimeScaleCommand();
    explicit SetTimeScaleCommand(XmlElement* element);
    ~SetTimeScaleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;                 // +0x0C "object" (object variable)
    uint16_t m_type = kNoTimeScaleType;    // +0x10 index of "Type"
    bool m_flag = false;                   // +0x12 bit 15, attribute 0x8FE1FDC1
    float m_scale = 1.0f;                  // +0x14 "Scale"
    float m_duration = -1.0f;              // +0x18 "duration"
};

// @func 0x00091CF0 SetTimeScaleCommand::SetTimeScaleCommand
SetTimeScaleCommand::SetTimeScaleCommand() = default;

// @func 0x00091D70 SetTimeScaleCommand::SetTimeScaleCommand
SetTimeScaleCommand::SetTimeScaleCommand(XmlElement* element)
{
    element->GetFloat(kAttrScale, &m_scale, 0.0f);
    if (element->GetHash(kAttrObject, &m_object, 0) == ape::xml::kQuery_Ok)
        return;
    uint32_t type = 0;
    if (element->GetHash(kAttrType, &type, 0) != ape::xml::kQuery_Ok)
        return;
    for (uint16_t i = 0; i < sizeof(kTimeScaleTypes) / sizeof(kTimeScaleTypes[0]); ++i) {
        if (kTimeScaleTypes[i] == type) {
            m_type = i;
            break;
        }
    }
    element->GetBool(kAttrTimeScale12, &m_flag, m_flag);
    element->GetFloat(kAttrDuration, &m_duration, 0.0f);
}

// @func 0x00091F1C SetTimeScaleCommand::~SetTimeScaleCommand
// @func 0x00091F4C SetTimeScaleCommand::~SetTimeScaleCommand (deleting)
SetTimeScaleCommand::~SetTimeScaleCommand() = default;

// @func 0x00091FA4 SetTimeScaleCommand::Create
ApeAnimScriptCommand* SetTimeScaleCommand::Create(XmlElement* element) const
{
    return new SetTimeScaleCommand(element);
}

// @func 0x00092014 SetTimeScaleCommand::Execute
void SetTimeScaleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_type == kNoTimeScaleType) {
        if (WorldObject* entity = EntityFromVar(instance, m_object))
            EntitySetTimeScale(entity, m_scale);
        return;
    }
    GameWorldSetTimeScale(m_type, m_scale, m_duration, m_flag);
}

// ---------------------------------------------------------------------------
// "SetUVSpeed" (NOTE: name from a hash match): scroll speed of a material's
// texture animation.

class SetUVSpeedCommand : public ApeAnimScriptCommand {
public:
    SetUVSpeedCommand();
    ~SetUVSpeedCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object" (object variable)
    uint32_t m_material = 0;  // +0x10 "materialHash"
    float m_uSpeed = 0.0f;    // +0x14 "USpeed"
    float m_vSpeed = 0.0f;    // +0x18 "VSpeed"
    bool m_hasU = false;      // +0x1C
    bool m_hasV = false;      // +0x1D
};

// @func 0x00092168 SetUVSpeedCommand::SetUVSpeedCommand
SetUVSpeedCommand::SetUVSpeedCommand() = default;

// @func 0x0073F4C8 SetUVSpeedCommand::~SetUVSpeedCommand
// @func 0x0073F4F8 SetUVSpeedCommand::~SetUVSpeedCommand (deleting)
SetUVSpeedCommand::~SetUVSpeedCommand() = default;

// @func 0x000921C8 SetUVSpeedCommand::Create
ApeAnimScriptCommand* SetUVSpeedCommand::Create(XmlElement* element) const
{
    auto* command = new SetUVSpeedCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrMaterialHash, &command->m_material, command->m_material);
    if (element->GetFloat(kAttrUSpeed, &command->m_uSpeed, command->m_uSpeed) != ape::xml::kQuery_Missing)
        command->m_hasU = true;
    if (element->GetFloat(kAttrVSpeed, &command->m_vSpeed, command->m_vSpeed) != ape::xml::kQuery_Missing)
        command->m_hasV = true;
    return command;
}

// @func 0x0009230C SetUVSpeedCommand::Execute
void SetUVSpeedCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeRefCounted* object = ObjectFromVar(instance, m_object);
    if (!object || !ObjectIsRenderInstance(object))
        return;
    void* material = RenderInstanceFindMaterial(object, m_material);
    if (!material) {
        ape::TempFormat("Material '%u' could not be found.", m_material);
        return;
    }
    void* anim = MaterialAnimData(material);
    if (!anim) {
        ape::TempFormat("Material '%u' has no anim data.", m_material);
        return;
    }
    if (m_hasU)
        MaterialAnimSetUSpeed(anim, m_uSpeed);
    if (m_hasV)
        MaterialAnimSetVSpeed(anim, m_vSpeed);
}

// ---------------------------------------------------------------------------
// Hash 0x83F543E7 (name unknown): runs a script on a live killable entity.

class SetEntityScriptCommand : public ApeAnimScriptCommand {
public:
    SetEntityScriptCommand();
    ~SetEntityScriptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object" (object variable, else entity name)
    uint32_t m_script = 0;  // +0x10 "Script"
};

// @func 0x0009241C SetEntityScriptCommand::SetEntityScriptCommand
SetEntityScriptCommand::SetEntityScriptCommand() = default;

// @func 0x0073F550 SetEntityScriptCommand::~SetEntityScriptCommand
// @func 0x0073F580 SetEntityScriptCommand::~SetEntityScriptCommand (deleting)
SetEntityScriptCommand::~SetEntityScriptCommand() = default;

// @func 0x00092464 SetEntityScriptCommand::Create
ApeAnimScriptCommand* SetEntityScriptCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntityScriptCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrScript, &command->m_script, command->m_script);
    return command;
}

// @func 0x00092534 SetEntityScriptCommand::Execute
void SetEntityScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_object);
    if (entity && EntityIsAlive(entity) && EntityHasClassFlag(entity, kEntityClass_Killable))
        EntityRunScript(entity, m_script);
}

// ---------------------------------------------------------------------------
// "ShootWeapon" (NOTE: name from a hash match)

class ShootWeaponCommand : public ApeAnimScriptCommand {
public:
    ShootWeaponCommand();
    ~ShootWeaponCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_hash14 = 0;     // +0x14 attribute 0x6EBFEF2B
    bool m_shoot = false;      // +0x18 "Shoot"
};

// @func 0x00092628 ShootWeaponCommand::ShootWeaponCommand
ShootWeaponCommand::ShootWeaponCommand() = default;

// @func 0x0073F5D8 ShootWeaponCommand::~ShootWeaponCommand
// @func 0x0073F608 ShootWeaponCommand::~ShootWeaponCommand (deleting)
ShootWeaponCommand::~ShootWeaponCommand() = default;

// @func 0x00092680 ShootWeaponCommand::Create
ApeAnimScriptCommand* ShootWeaponCommand::Create(XmlElement* element) const
{
    auto* command = new ShootWeaponCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrWeapon14, &command->m_hash14, command->m_hash14);
    element->GetBool(kAttrShoot, &command->m_shoot, command->m_shoot);
    return command;
}

// @func 0x00092790 ShootWeaponCommand::Execute
void ShootWeaponCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_refObject, m_entity);
    if (!entity)
        return;
    if (void* weapon = EntitySkin(entity))
        WeaponSetShooting(weapon, m_shoot);
}

// ---------------------------------------------------------------------------
// "ShowHudPrompt" (NOTE: name from a hash match): a message with optional
// X/circle button lines.

class ShowHudPromptCommand : public ApeAnimScriptCommand {
public:
    ShowHudPromptCommand();
    ~ShowHudPromptCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_message[0x81] = {};    // +0x0C "msg"
    char m_buttons[0x81] = {};    // +0x8D button line built from the options
    bool m_showX = true;          // +0x10E attribute 0x36F05514
    bool m_showCircle = true;     // +0x10F attribute 0x21BE4C63
};

// @func 0x00092870 ShowHudPromptCommand::ShowHudPromptCommand
ShowHudPromptCommand::ShowHudPromptCommand() = default;

// @func 0x0073F660 ShowHudPromptCommand::~ShowHudPromptCommand
// @func 0x0073F690 ShowHudPromptCommand::~ShowHudPromptCommand (deleting)
ShowHudPromptCommand::~ShowHudPromptCommand() = default;

// @func 0x000928D0 ShowHudPromptCommand::Create
ApeAnimScriptCommand* ShowHudPromptCommand::Create(XmlElement* element) const
{
    auto* command = new ShowHudPromptCommand();
    const char* message = element->GetString("msg");
    if (message && *message)
        std::strncpy(command->m_message, message, 0x80);
    element->GetBool(kAttrPromptX, &command->m_showX, command->m_showX);
    element->GetBool(kAttrPromptCircle, &command->m_showCircle, command->m_showCircle);

    const char* xText = element->GetString(kAttrPromptXText);
    if (!xText)
        xText = "$STR_YES";
    const char* circleText = element->GetString(kAttrPromptCircleText);
    if (!circleText)
        circleText = "$STR_NO";

    const char* buttons = nullptr;
    if (command->m_showX && command->m_showCircle)
        buttons = ape::TempFormat("$BUTTON_X %s $BUTTON_CIRCLE %s", xText, circleText);
    else if (command->m_showX)
        buttons = ape::TempFormat("$BUTTON_X %s", xText);
    else if (command->m_showCircle)
        buttons = ape::TempFormat("$BUTTON_CIRCLE %s", circleText);
    if (buttons)
        std::strncpy(command->m_buttons, buttons, 0x80);
    return command;
}

// @func 0x00092B24 ShowHudPromptCommand::Execute
void ShowHudPromptCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameWorldShowPrompt(PlayerEntity(), m_message, m_buttons);
}

// ---------------------------------------------------------------------------
// Hash 0xC4162D5B (name unknown): game world call 0x00482B20 with a "force" flag.

class WorldForceCommand : public ApeAnimScriptCommand {
public:
    WorldForceCommand();
    ~WorldForceCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_force = false;  // +0x0C "force"
};

// @func 0x00092B8C WorldForceCommand::WorldForceCommand
WorldForceCommand::WorldForceCommand() = default;

// @func 0x0073F6E8 WorldForceCommand::~WorldForceCommand
// @func 0x0073F718 WorldForceCommand::~WorldForceCommand (deleting)
WorldForceCommand::~WorldForceCommand() = default;

// @func 0x00092BD4 WorldForceCommand::Create
ApeAnimScriptCommand* WorldForceCommand::Create(XmlElement* element) const
{
    auto* command = new WorldForceCommand();
    element->GetBool(kAttrForce, &command->m_force, false);
    return command;
}

// @func 0x00092C78 WorldForceCommand::Execute
void WorldForceCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameWorldCall482B20(m_force);
}

// ---------------------------------------------------------------------------
// "SmokeBomb": drops a smoke cloud that blinds the AI.

class SmokeBombCommand : public ApeAnimScriptCommand {
public:
    SmokeBombCommand();
    ~SmokeBombCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object" (object variable: position)
    uint32_t m_owner = 0;     // +0x10 "owner" (object variable)
    float m_radius = 0.0f;    // +0x14 "radius"
    float m_duration = 0.0f;  // +0x18 "duration"
    int32_t m_mode = 0;       // +0x1C "mode"
};

// @func 0x00092CAC SmokeBombCommand::SmokeBombCommand
SmokeBombCommand::SmokeBombCommand() = default;

// @func 0x00092D0C SmokeBombCommand::~SmokeBombCommand
// @func 0x00092D3C SmokeBombCommand::~SmokeBombCommand (deleting)
SmokeBombCommand::~SmokeBombCommand() = default;

// @func 0x00092D94 SmokeBombCommand::Create
ApeAnimScriptCommand* SmokeBombCommand::Create(XmlElement* element) const
{
    auto* command = new SmokeBombCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrOwner, &command->m_owner, command->m_owner);
        element->GetFloat(kAttrRadius, &command->m_radius, command->m_radius);
        element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
        element->GetInt(kAttrMode, &command->m_mode, command->m_mode);
    }
    return command;
}

// @func 0x00092EE4 SmokeBombCommand::Execute
// Adds a smoke cloud to the level AI at the object's position (the origin
// when there is none), owned by the entity of "owner".
void SmokeBombCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::Vec3 position = kSmokeOrigin;
    if (ape::ApeRefCounted* object = ObjectFromVar(instance, m_object))
        position = ObjectWorldPosition(object);
    WorldObject* owner = EntityFromVar(instance, m_owner);
    LevelAIAddSmoke(owner, m_radius, m_duration, m_mode, position);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsT()
{
    auto& prototypes = ape::CommandPrototypes();
    // #160 "SetSoundRegister" (0x71814B90) is not ported yet.
    prototypes.Add(0xB6CD6FF1u, new SetGlobalValueCommand());
    prototypes.Add(AnimHash("SetTimeScale"), new SetTimeScaleCommand());
    prototypes.Add(AnimHash("SetUVSpeed"), new SetUVSpeedCommand());
    prototypes.Add(0x83F543E7u, new SetEntityScriptCommand());
    prototypes.Add(AnimHash("ShootWeapon"), new ShootWeaponCommand());
    prototypes.Add(AnimHash("ShowHudPrompt"), new ShowHudPromptCommand());
    prototypes.Add(0xC4162D5Bu, new WorldForceCommand());
    prototypes.Add(AnimHash("SmokeBomb"), new SmokeBombCommand());
}

} // namespace levels
} // namespace sly
