// openTIT - Game script commands, eighth block (Impulse to MapEntityToVar).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrImpulse = AnimHash("Impulse");
constexpr uint32_t kAttrRandomDir = AnimHash("RandomDir");
constexpr uint32_t kAttrAngular = AnimHash("angular");
constexpr uint32_t kAttrImpulseBit29 = 0x6BAA379Fu;  // NOTE: name unknown
constexpr uint32_t kAttrImpulseBit28 = 0x099CBC7Cu;  // NOTE: name unknown
constexpr uint32_t kAttrInputPoints = AnimHash("InputPoints");
constexpr uint32_t kAttrInputMover1C = 0xB3106A84u;  // NOTE: name unknown
constexpr uint32_t kAttrDoReset = AnimHash("DoReset");
constexpr uint32_t kAttrCameraWeight = AnimHash("CameraWeight");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrCurve = AnimHash("Curve");
constexpr uint32_t kAttrExponent = AnimHash("Exponent");
constexpr uint32_t kCurve_Exp = AnimHash("Exp");
constexpr uint32_t kCurve_8105148C = 0x8105148Cu;     // NOTE: name unknown; exponent 0.5
constexpr uint32_t kAttrKill14 = 0x66D71193u;         // NOTE: name unknown
constexpr uint32_t kAttrKill15 = 0x739CF08Bu;         // NOTE: name unknown
constexpr uint32_t kAttrName = AnimHash("Name");
constexpr uint32_t kAttrLevel = AnimHash("level");
constexpr uint32_t kAttrObjectVar = AnimHash("ObjectVar");
constexpr uint32_t kAttrFromObjVar = AnimHash("fromObjVar");
constexpr uint32_t kAttrTrigger = AnimHash("trigger");
constexpr uint32_t kAttrSpawner = AnimHash("Spawner");
constexpr uint32_t kAttrSpawnerGroup = AnimHash("SpawnerGroup");
constexpr uint32_t kAttrLaser = AnimHash("laser");
constexpr uint32_t kAttrLaserField = AnimHash("LaserField");
constexpr uint32_t kAttrLaserGroup = AnimHash("LaserGroup");
constexpr uint32_t kAttrHudObj = AnimHash("HudObj");

const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};
} // namespace

// ---------------------------------------------------------------------------
// Impulse: pushes an entity's physics body.

class ImpulseCommand : public ApeAnimScriptCommand {
public:
    ImpulseCommand();
    explicit ImpulseCommand(XmlElement* element);
    ~ImpulseCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::Vec3 m_impulse = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x10 "Impulse"
    uint32_t m_object = 0;                           // +0x20 "object"
    uint32_t m_entity = 0;                           // +0x24 "Entity"
    // +0x28 bit field (bits 31..28).
    bool m_randomDir = false;  // bit 31 "RandomDir"
    bool m_angular = false;    // bit 30 "angular"
    bool m_bit29 = false;      // bit 29 attribute 0x6BAA379F
    bool m_bit28 = false;      // bit 28 attribute 0x099CBC7C
};

// @func 0x0007F5F0 ImpulseCommand::ImpulseCommand
ImpulseCommand::ImpulseCommand() = default;

// @func 0x0007F670 ImpulseCommand::ImpulseCommand
ImpulseCommand::ImpulseCommand(XmlElement* element)
{
    if (element->GetHash(kAttrEntity, &m_entity, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok) {
        return;
    }
    element->GetVec3(kAttrImpulse, &m_impulse.x, &kZero.x);
    element->GetBool(kAttrRandomDir, &m_randomDir, m_randomDir);
    element->GetBool(kAttrAngular, &m_angular, m_angular);
    element->GetBool(kAttrImpulseBit29, &m_bit29, m_bit29);
    element->GetBool(kAttrImpulseBit28, &m_bit28, m_bit28);
}

// @func 0x0007F874 ImpulseCommand::~ImpulseCommand
// @func 0x0007F8A4 ImpulseCommand::~ImpulseCommand (deleting)
ImpulseCommand::~ImpulseCommand() = default;

// @func 0x0007F8FC ImpulseCommand::Create
ApeAnimScriptCommand* ImpulseCommand::Create(XmlElement* element) const
{
    return new ImpulseCommand(element);
}

// TODO(0x0007F980 ImpulseCommand::Execute)

// ---------------------------------------------------------------------------
// InputMover

class InputMoverCommand : public ApeAnimScriptCommand {
public:
    InputMoverCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // +0x0C "object"
    uint32_t m_entity = 0;        // +0x10 "Entity"
    float m_inputPoints = -1.0f;  // +0x18 "InputPoints"
    bool m_flag1C = false;        // +0x1C attribute 0xB3106A84
    bool m_hasInputPoints = false;// +0x1D
    bool m_doReset = false;       // +0x1E "DoReset"
};

// @func 0x0007FFA8 InputMoverCommand::InputMoverCommand
InputMoverCommand::InputMoverCommand() = default;

// @func 0x00080010 InputMoverCommand::Create
ApeAnimScriptCommand* InputMoverCommand::Create(XmlElement* element) const
{
    auto* command = new InputMoverCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    if (element->GetFloat(kAttrInputPoints, &command->m_inputPoints, command->m_inputPoints) ==
        ape::xml::kQuery_Ok)
        command->m_hasInputPoints = true;
    element->GetBool(kAttrInputMover1C, &command->m_flag1C, command->m_flag1C);
    element->GetBool(kAttrDoReset, &command->m_doReset, command->m_doReset);
    return command;
}

// TODO(0x00080170 InputMoverCommand::Execute)

// ---------------------------------------------------------------------------
// Hash 0x96EE417F (name unknown): blends the camera weight with a curve.

class CameraWeightCommand : public ApeAnimScriptCommand {
public:
    CameraWeightCommand();
    explicit CameraWeightCommand(XmlElement* element);
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_cameraWeight = 1.0f;  // +0x0C "CameraWeight"
    float m_duration = 0.0f;      // +0x10 "duration"
    float m_exponent = 1.0f;      // +0x14 "Exponent" (default from "Curve")
};

// @func 0x000802E0 CameraWeightCommand::CameraWeightCommand
CameraWeightCommand::CameraWeightCommand() = default;

// @func 0x00080340 CameraWeightCommand::CameraWeightCommand
CameraWeightCommand::CameraWeightCommand(XmlElement* element)
{
    element->GetFloat(kAttrCameraWeight, &m_cameraWeight, 1.0f);
    element->GetFloat(kAttrDuration, &m_duration, 0.0f);
    uint32_t curve = 0;
    if (element->GetHash(kAttrCurve, &curve, 0) == ape::xml::kQuery_Ok) {
        // The curve name only picks the default exponent.
        if (curve == kCurve_8105148C)
            m_exponent = 0.5f;
        else if (curve == kCurve_Exp)
            m_exponent = 2.0f;
        else
            m_exponent = 1.0f;
    }
    element->GetFloat(kAttrExponent, &m_exponent, m_exponent);
    if (m_duration < 0.0f)
        m_duration = 0.0f;
}

// @func 0x000804DC CameraWeightCommand::Create
ApeAnimScriptCommand* CameraWeightCommand::Create(XmlElement* element) const
{
    return new CameraWeightCommand(element);
}

// @func 0x0008054C CameraWeightCommand::Execute
void CameraWeightCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    CameraBlendWeight(m_cameraWeight, m_duration, m_exponent);
}

// ---------------------------------------------------------------------------
// Hash 0x78811B3A (name unknown): acts on the boss "MsDecibel".

class MsDecibelCommand : public ApeAnimScriptCommand {
public:
    MsDecibelCommand();
    ~MsDecibelCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x0008058C MsDecibelCommand::MsDecibelCommand
MsDecibelCommand::MsDecibelCommand() = default;

// @func 0x000805CC MsDecibelCommand::~MsDecibelCommand
// @func 0x000805FC MsDecibelCommand::~MsDecibelCommand (deleting)
MsDecibelCommand::~MsDecibelCommand() = default;

// @func 0x00080654 MsDecibelCommand::Create
ApeAnimScriptCommand* MsDecibelCommand::Create(XmlElement*) const
{
    return new MsDecibelCommand();
}

// TODO(0x000806D4 MsDecibelCommand::Execute) calls 0x0005BB08 on the entity
// "MsDecibel" when its +0xF4 component exists.

// ---------------------------------------------------------------------------
// Kill

class KillCommand : public ApeAnimScriptCommand {
public:
    KillCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;   // +0x0C "object"
    uint32_t m_child = 0;    // +0x10 "Child"
    bool m_flag14 = false;   // +0x14 attribute 0x66D71193
    bool m_flag15 = false;   // +0x15 attribute 0x739CF08B
};

// @func 0x00080720 KillCommand::KillCommand
KillCommand::KillCommand() = default;

// @func 0x00080778 KillCommand::Create
ApeAnimScriptCommand* KillCommand::Create(XmlElement* element) const
{
    auto* command = new KillCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetBool(kAttrKill14, &command->m_flag14, command->m_flag14);
    element->GetBool(kAttrKill15, &command->m_flag15, command->m_flag15);
    return command;
}

// TODO(0x0008088C KillCommand::Execute)

// ---------------------------------------------------------------------------
// KillSequence

class KillSequenceCommand : public ApeAnimScriptCommand {
public:
    KillSequenceCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;  // +0x0C "Name"
};

// @func 0x00080A88 KillSequenceCommand::KillSequenceCommand
KillSequenceCommand::KillSequenceCommand() = default;

// @func 0x00080AD0 KillSequenceCommand::Create
ApeAnimScriptCommand* KillSequenceCommand::Create(XmlElement* element) const
{
    auto* command = new KillSequenceCommand();
    element->GetHash(kAttrName, &command->m_name, 0);
    return command;
}

// @func 0x00080B74 KillSequenceCommand::Execute
void KillSequenceCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    KillSequence(m_name);
}

// ---------------------------------------------------------------------------
// LoadLevel

class LoadLevelCommand : public ApeAnimScriptCommand {
public:
    LoadLevelCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_level = 0;  // +0x0C "level"
};

// @func 0x00080B9C LoadLevelCommand::LoadLevelCommand
LoadLevelCommand::LoadLevelCommand() = default;

// @func 0x00080BE4 LoadLevelCommand::Create
ApeAnimScriptCommand* LoadLevelCommand::Create(XmlElement* element) const
{
    auto* command = new LoadLevelCommand();
    element->GetHash(kAttrLevel, &command->m_level, 0);
    return command;
}

// @func 0x00080C88 LoadLevelCommand::Execute
void LoadLevelCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_level == 0)
        return;
    // Asking for the current level restarts it.
    if (CurrentLevelName() == m_level)
        RestartLevel();
    else
        ChangeLevel(m_level);
}

// ---------------------------------------------------------------------------
// MapEntityToVar: binds an object variable to a game object of some kind.

class MapEntityToVarCommand : public ApeAnimScriptCommand {
public:
    MapEntityToVarCommand();
    explicit MapEntityToVarCommand(XmlElement* element);
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_objectVar = 0;     // +0x0C "ObjectVar": variable to set
    uint32_t m_fromObjVar = 0;    // +0x10 "fromObjVar"
    uint32_t m_entity = 0;        // +0x14 "Entity"
    uint32_t m_trigger = 0;       // +0x18 "trigger"
    uint32_t m_spawner = 0;       // +0x1C "Spawner"
    uint32_t m_spawnerGroup = 0;  // +0x20 "SpawnerGroup"
    uint32_t m_laser = 0;         // +0x24 "laser"
    uint32_t m_laserField = 0;    // +0x28 "LaserField"
    uint32_t m_laserGroup = 0;    // +0x2C "LaserGroup"
    uint32_t m_hudObj = 0;        // +0x30 "HudObj"
    uint32_t m_child = 0;         // +0x34 "Child"
};

// @func 0x00080CF8 MapEntityToVarCommand::MapEntityToVarCommand
MapEntityToVarCommand::MapEntityToVarCommand() = default;

// @func 0x00080D68 MapEntityToVarCommand::MapEntityToVarCommand
MapEntityToVarCommand::MapEntityToVarCommand(XmlElement* element)
{
    element->GetHash(kAttrObjectVar, &m_objectVar, 0);
    element->GetHash(kAttrFromObjVar, &m_fromObjVar, 0);
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrTrigger, &m_trigger, 0);
    element->GetHash(kAttrSpawner, &m_spawner, 0);
    element->GetHash(kAttrSpawnerGroup, &m_spawnerGroup, 0);
    element->GetHash(kAttrLaser, &m_laser, 0);
    element->GetHash(kAttrLaserField, &m_laserField, 0);
    element->GetHash(kAttrLaserGroup, &m_laserGroup, 0);
    element->GetHash(kAttrHudObj, &m_hudObj, 0);
    element->GetHash(kAttrChild, &m_child, 0);
}

// @func 0x00080F54 MapEntityToVarCommand::Create
ApeAnimScriptCommand* MapEntityToVarCommand::Create(XmlElement* element) const
{
    return new MapEntityToVarCommand(element);
}

// TODO(0x00080FC4 MapEntityToVarCommand::Execute) "MapEntityToVar cannot find entity (inst) %s".

// ---------------------------------------------------------------------------

void RegisterScriptCommandsJ()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("Impulse"), new ImpulseCommand());
    prototypes.Add(AnimHash("InputMover"), new InputMoverCommand());
    prototypes.Add(0x96EE417Fu, new CameraWeightCommand());
    prototypes.Add(0x78811B3Au, new MsDecibelCommand());
    prototypes.Add(AnimHash("Kill"), new KillCommand());
    prototypes.Add(AnimHash("KillSequence"), new KillSequenceCommand());
    prototypes.Add(AnimHash("LoadLevel"), new LoadLevelCommand());
    prototypes.Add(AnimHash("MapEntityToVar"), new MapEntityToVarCommand());
}

} // namespace levels
} // namespace sly
