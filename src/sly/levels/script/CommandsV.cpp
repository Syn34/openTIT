// openTIT - Game script commands, twentieth block (StopMusic to the hash 0x8A36758E command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrStopMusicFade = 0x83F172B0u;  // NOTE: name unknown, default true
constexpr uint32_t kAttrSoundVar = 0x3404783Cu;       // NOTE: name unknown (sound variable)
constexpr uint32_t kAttrStopSound10 = 0x55E57B9Cu;    // NOTE: name unknown; inverted when passed on
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrDef = AnimHash("def");
constexpr uint32_t kAttrBinocucomComp = AnimHash("BinocucomComp");
constexpr uint32_t kAttrInventoryComp = AnimHash("InventoryComp");
constexpr uint32_t kAttrMarkerComp = AnimHash("MarkerComp");
constexpr uint32_t kAttrReceptacleComp = AnimHash("ReceptacleComp");
constexpr uint32_t kAttrWeaponComp = 0x6EBFEF2Bu;     // NOTE: name unknown (weapon component def)
constexpr uint32_t kAttrSpeedOverride = AnimHash("SpeedOverride");
constexpr uint32_t kAttrRateBoth = 0x690E3A7Du;       // NOTE: name unknown; per-second rate for both slots
constexpr uint32_t kAttrRate1C = 0x6EC98295u;         // NOTE: name unknown
constexpr uint32_t kAttrRate20 = 0x6EC981A7u;         // NOTE: name unknown
constexpr uint32_t kAttrAngle24 = 0x82946CACu;        // NOTE: name unknown (degrees, stored in radians)
constexpr uint32_t kAttrForceReset = AnimHash("ForceReset");
constexpr uint32_t kAttrDef10 = 0x00D8E5D6u;          // NOTE: name unknown
constexpr uint32_t kAttrDef11 = 0xA18DAB9Fu;          // NOTE: name unknown
constexpr uint32_t kAttrDef12 = 0xB555B755u;          // NOTE: name unknown

constexpr float kDegToRad = 0.017453292f;
} // namespace

// ---------------------------------------------------------------------------
// "StopMusic" (NOTE: name from a hash match)

class StopMusicCommand : public ApeAnimScriptCommand {
public:
    StopMusicCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_flag = true;  // +0x0C attribute 0x83F172B0
};

// @func 0x00096858 StopMusicCommand::StopMusicCommand
StopMusicCommand::StopMusicCommand() = default;

// @func 0x000968A8 StopMusicCommand::Create
ApeAnimScriptCommand* StopMusicCommand::Create(XmlElement* element) const
{
    auto* command = new StopMusicCommand();
    element->GetBool(kAttrStopMusicFade, &command->m_flag, true);
    return command;
}

// @func 0x00096954 StopMusicCommand::Execute
void StopMusicCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    MusicStop(m_flag);
}

// ---------------------------------------------------------------------------
// "StopSound" (NOTE: name from a hash match): stops the voice held by a sound
// variable.

class StopSoundCommand : public ApeAnimScriptCommand {
public:
    StopSoundCommand();
    ~StopSoundCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_soundVar = 0;  // +0x0C attribute 0x3404783C
    bool m_flag10 = false;    // +0x10 attribute 0x55E57B9C
};

// @func 0x0009697C StopSoundCommand::StopSoundCommand
StopSoundCommand::StopSoundCommand() = default;

// @func 0x000969C4 StopSoundCommand::~StopSoundCommand
// @func 0x000969F4 StopSoundCommand::~StopSoundCommand (deleting)
StopSoundCommand::~StopSoundCommand() = default;

// @func 0x00096A4C StopSoundCommand::Create
ApeAnimScriptCommand* StopSoundCommand::Create(XmlElement* element) const
{
    auto* command = new StopSoundCommand();
    element->GetHash(kAttrSoundVar, &command->m_soundVar, 0);
    element->GetBool(kAttrStopSound10, &command->m_flag10, false);
    return command;
}

// @func 0x00096B14 StopSoundCommand::Execute
void StopSoundCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_soundVar == 0)
        return;
    ApeAnimScriptVar* var = instance->FindVar(m_soundVar);
    if (var && static_cast<int>(var->GetType()) == kAnimScriptVarType_Sound)
        SoundVarStop(var, !m_flag10);
}

// ---------------------------------------------------------------------------
// Swap commands: replace one component of an entity with another definition.
// They share the operand lookup: an object variable, else the entity name.

namespace {
WorldObject* SwapTarget(ApeAnimScriptInstance* instance, uint32_t refObject, uint32_t entityName)
{
    if (WorldObject* entity = EntityFromVar(instance, refObject))
        return entity;
    WorldObject* entity = FindEntity(entityName);
    if (!entity)
        ape::TempHexString(entityName);  // the release build's warning prints nothing
    return entity;
}
} // namespace

// "SwapBinocucom" (NOTE: name from a hash match)
class SwapBinocucomCommand : public ApeAnimScriptCommand {
public:
    SwapBinocucomCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_comp = 0;       // +0x14 "BinocucomComp"
};

// @func 0x00096BD4 SwapBinocucomCommand::SwapBinocucomCommand
SwapBinocucomCommand::SwapBinocucomCommand() = default;

// @func 0x00096C2C SwapBinocucomCommand::Create
ApeAnimScriptCommand* SwapBinocucomCommand::Create(XmlElement* element) const
{
    auto* command = new SwapBinocucomCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrBinocucomComp, &command->m_comp, command->m_comp);
    return command;
}

// @func 0x00096D18 SwapBinocucomCommand::Execute
void SwapBinocucomCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = EntityFromVar(instance, m_refObject);
    if (!entity)
        entity = FindEntity(m_entity);
    if (entity && EntityIsCharacter(entity))
        CharacterSwapBinocucom(entity, m_comp);
}

// Hash 0x52FDEC56 (name unknown): swaps an entity definition-level component.
class SwapDefCommand : public ApeAnimScriptCommand {
public:
    SwapDefCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    uint32_t m_def = 0;     // +0x14 "def"
};

// @func 0x00096E00 SwapDefCommand::SwapDefCommand
SwapDefCommand::SwapDefCommand() = default;

// @func 0x00096E50 SwapDefCommand::Create
ApeAnimScriptCommand* SwapDefCommand::Create(XmlElement* element) const
{
    auto* command = new SwapDefCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrDef, &command->m_def, command->m_def);
    return command;
}

// TODO(0x00096F40 SwapDefCommand::Execute) definition lookup (0x0073A1A4) and 0x0028728C.

// "SwapInventory" (NOTE: name from a hash match)
class SwapInventoryCommand : public ApeAnimScriptCommand {
public:
    SwapInventoryCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_comp = 0;       // +0x14 "InventoryComp"
};

// @func 0x00097188 SwapInventoryCommand::SwapInventoryCommand
SwapInventoryCommand::SwapInventoryCommand() = default;

// @func 0x000971E0 SwapInventoryCommand::Create
ApeAnimScriptCommand* SwapInventoryCommand::Create(XmlElement* element) const
{
    auto* command = new SwapInventoryCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrInventoryComp, &command->m_comp, command->m_comp);
    return command;
}

// @func 0x000972CC SwapInventoryCommand::Execute
void SwapInventoryCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (WorldObject* entity = SwapTarget(instance, m_refObject, m_entity))
        EntitySwapInventory(entity, m_comp);
}

// Hash 0x7B192C82 (name unknown): swaps the marker component.
class SwapMarkerCommand : public ApeAnimScriptCommand {
public:
    SwapMarkerCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_comp = 0;       // +0x14 "MarkerComp"
};

// @func 0x000973A8 SwapMarkerCommand::SwapMarkerCommand
SwapMarkerCommand::SwapMarkerCommand() = default;

// @func 0x00097400 SwapMarkerCommand::Create
ApeAnimScriptCommand* SwapMarkerCommand::Create(XmlElement* element) const
{
    auto* command = new SwapMarkerCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrMarkerComp, &command->m_comp, command->m_comp);
    return command;
}

// @func 0x000974EC SwapMarkerCommand::Execute
void SwapMarkerCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (WorldObject* entity = SwapTarget(instance, m_refObject, m_entity))
        EntitySwapMarker(entity, m_comp);
}

// "SwapReceptacle" (NOTE: name from a hash match)
class SwapReceptacleCommand : public ApeAnimScriptCommand {
public:
    SwapReceptacleCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_comp = 0;       // +0x14 "ReceptacleComp"
};

// @func 0x000975C8 SwapReceptacleCommand::SwapReceptacleCommand
SwapReceptacleCommand::SwapReceptacleCommand() = default;

// @func 0x00097618 SwapReceptacleCommand::Create
ApeAnimScriptCommand* SwapReceptacleCommand::Create(XmlElement* element) const
{
    auto* command = new SwapReceptacleCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrReceptacleComp, &command->m_comp, command->m_comp);
    return command;
}

// @func 0x00097708 SwapReceptacleCommand::Execute
void SwapReceptacleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (WorldObject* entity = SwapTarget(instance, m_refObject, m_entity))
        EntitySwapReceptacle(entity, m_comp);
}

// ---------------------------------------------------------------------------
// "SwapWeapon" (NOTE: name from a hash match)

class SwapWeaponCommand : public ApeAnimScriptCommand {
public:
    SwapWeaponCommand();
    explicit SwapWeaponCommand(XmlElement* element);
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;     // +0x0C "RefObject"
    uint32_t m_entity = 0;        // +0x10 "Entity"
    uint32_t m_weaponComp = 0;    // +0x14 attribute 0x6EBFEF2B
    float m_speedOverride = 0.0f; // +0x18 "SpeedOverride"
    float m_interval1C = 0.0f;    // +0x1C 1 / rate (attributes 0x690E3A7D, 0x6EC98295)
    float m_interval20 = 0.0f;    // +0x20 1 / rate (attributes 0x690E3A7D, 0x6EC981A7)
    float m_angle = 0.0f;         // +0x24 attribute 0x82946CAC (radians)
    bool m_forceReset = false;    // +0x28 "ForceReset"
};

// @func 0x000977E4 SwapWeaponCommand::SwapWeaponCommand
SwapWeaponCommand::SwapWeaponCommand() = default;

// @func 0x00097850 SwapWeaponCommand::SwapWeaponCommand
SwapWeaponCommand::SwapWeaponCommand(XmlElement* element)
{
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetHash(kAttrEntity, &m_entity, m_entity);
    element->GetHash(kAttrWeaponComp, &m_weaponComp, m_weaponComp);
    element->GetFloat(kAttrSpeedOverride, &m_speedOverride, m_speedOverride);

    // Rates are given per second and stored as intervals; only positive
    // values count.
    float rate = 0.0f;
    if (element->GetFloat(kAttrRateBoth, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        m_interval1C = m_interval20 = 1.0f / rate;
    if (element->GetFloat(kAttrRate1C, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        m_interval1C = 1.0f / rate;
    if (element->GetFloat(kAttrRate20, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        m_interval20 = 1.0f / rate;
    if (element->GetFloat(kAttrAngle24, &m_angle, 0.0f) == ape::xml::kQuery_Ok)
        m_angle *= kDegToRad;
    element->GetBool(kAttrForceReset, &m_forceReset, false);
}

// @func 0x00097AB4 SwapWeaponCommand::Create
ApeAnimScriptCommand* SwapWeaponCommand::Create(XmlElement* element) const
{
    return new SwapWeaponCommand(element);
}

// TODO(0x00097B24 SwapWeaponCommand::Execute) entity lookup then 0x001CAA88 with the
// weapon settings.

// ---------------------------------------------------------------------------
// Hash 0x8A36758E (name unknown): definition switch with three flags.

class DefFlagsCommand : public ApeAnimScriptCommand {
public:
    DefFlagsCommand();
    ~DefFlagsCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_def = 0;     // +0x0C "def"
    bool m_flag10 = false;  // +0x10 attribute 0x00D8E5D6
    bool m_flag11 = false;  // +0x11 attribute 0xA18DAB9F
    bool m_flag12 = false;  // +0x12 attribute 0xB555B755
};

// @func 0x00097C58 DefFlagsCommand::DefFlagsCommand
DefFlagsCommand::DefFlagsCommand() = default;

// @func 0x00097CB0 DefFlagsCommand::~DefFlagsCommand
// @func 0x00097CE0 DefFlagsCommand::~DefFlagsCommand (deleting)
DefFlagsCommand::~DefFlagsCommand() = default;

// @func 0x00097D38 DefFlagsCommand::Create
ApeAnimScriptCommand* DefFlagsCommand::Create(XmlElement* element) const
{
    auto* command = new DefFlagsCommand();
    element->GetHash(kAttrDef, &command->m_def, 0);
    element->GetBool(kAttrDef10, &command->m_flag10, command->m_flag10);
    element->GetBool(kAttrDef11, &command->m_flag11, command->m_flag11);
    element->GetBool(kAttrDef12, &command->m_flag12, command->m_flag12);
    return command;
}

// TODO(0x00097E50 DefFlagsCommand::Execute) walks the entities of the definition
// (0x000C9FC8, 0x00124E58, 0x00238270, 0x0024B1A8, 0x00261328).

// ---------------------------------------------------------------------------

void RegisterScriptCommandsV()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("StopMusic"), new StopMusicCommand());
    prototypes.Add(AnimHash("StopSound"), new StopSoundCommand());
    prototypes.Add(AnimHash("SwapBinocucom"), new SwapBinocucomCommand());
    prototypes.Add(0x52FDEC56u, new SwapDefCommand());
    prototypes.Add(AnimHash("SwapInventory"), new SwapInventoryCommand());
    prototypes.Add(0x7B192C82u, new SwapMarkerCommand());
    prototypes.Add(AnimHash("SwapReceptacle"), new SwapReceptacleCommand());
    prototypes.Add(AnimHash("SwapWeapon"), new SwapWeaponCommand());
    prototypes.Add(0x8A36758Eu, new DefFlagsCommand());
}

} // namespace levels
} // namespace sly
