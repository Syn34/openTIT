// openTIT - Game script commands, twenty-fourth block (FadeDof to MoveCamera).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrFocusTarget = 0xED2B8734u;  // NOTE: names unknown for the focus-pull values
constexpr uint32_t kAttrFocus10 = 0x3C5308B5u;
constexpr uint32_t kAttrFocus14 = 0x59D5C9EEu;
constexpr uint32_t kAttrFocus18 = 0x7089C1B7u;
constexpr uint32_t kAttrFocus1C = 0x070ADB70u;
constexpr uint32_t kAttrFocus20 = 0xB1D3B545u;
constexpr uint32_t kAttrFocus24 = 0x3DB85A7Eu;
constexpr uint32_t kAttrNumIterations = AnimHash("numIterations");
constexpr uint32_t kAttrFocus30 = 0x26873C74u;
constexpr uint32_t kAttrFocus32 = 0x2E09C6B8u;
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;      // NOTE: name unknown
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrShooter = AnimHash("Shooter");
constexpr uint32_t kAttrTarget = AnimHash("Target");
constexpr uint32_t kAttrShoot14 = 0x1502BCC1u;      // NOTE: name unknown
constexpr uint32_t kAttrShootIndex = 0x21B3BD9Eu;   // NOTE: name unknown (1-based index)
constexpr uint32_t kAttrShoot1C = 0x9F451E77u;      // NOTE: name unknown
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrObjectInst = AnimHash("ObjectInst");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrMagnitude = AnimHash("Magnitude");
constexpr uint32_t kAttrStartMagnitude = 0x793E3FEEu;
constexpr uint32_t kAttrFalloffMode = AnimHash("FalloffMode");
constexpr uint32_t kAttrFalloffStartTime = AnimHash("FalloffStartTime");
constexpr uint32_t kAttrPitch = AnimHash("Pitch");
constexpr uint32_t kAttrYaw = AnimHash("Yaw");
constexpr uint32_t kAttrRoll = AnimHash("Roll");
constexpr uint32_t kAttrPitchFrequency = AnimHash("PitchFrequency");
constexpr uint32_t kAttrYawFrequency = AnimHash("YawFrequency");
constexpr uint32_t kAttrRollFrequency = AnimHash("RollFrequency");
constexpr uint32_t kAttrXOffset = AnimHash("XOffset");
constexpr uint32_t kAttrYOffset = AnimHash("YOffset");
constexpr uint32_t kAttrZOffset = AnimHash("ZOffset");
constexpr uint32_t kAttrXOffsetFrequency = 0xA99476B1u;  // NOTE: name unknown
constexpr uint32_t kAttrYOffsetFrequency = 0x8B265490u;  // NOTE: name unknown
constexpr uint32_t kAttrZOffsetFrequency = 0x6CB8326Fu;  // NOTE: name unknown
constexpr uint32_t kFalloffLinear = AnimHash("Linear");
constexpr uint32_t kAttrCameraWeight = 0x8F123DDDu;  // NOTE: name unknown (sequence variant of "CameraWeight")
constexpr uint32_t kAttrCurve = AnimHash("Curve");
constexpr uint32_t kAttrExponent = AnimHash("Exponent");
constexpr uint32_t kCurve_8105148C = 0x8105148Cu;    // NOTE: name unknown; exponent 0.5
constexpr uint32_t kCurve_Exp = AnimHash("Exp");
constexpr uint32_t kAttrPtName = 0x8D47DA8Fu;        // NOTE: name unknown (point instance name)
constexpr uint32_t kAttrAnim = AnimHash("anim");
constexpr uint32_t kAttrJump18 = 0xDFD4E345u;        // NOTE: name unknown
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrKill10 = 0x66D71193u;        // NOTE: name unknown (also used by "Kill")
constexpr uint32_t kAttrLook14 = 0xCF0502B9u;        // NOTE: names unknown for the look-at values
constexpr uint32_t kAttrLook18 = 0xF636DCCAu;
constexpr uint32_t kAttrLook20 = 0xBE36C5F4u;
constexpr uint32_t kAttrFov = AnimHash("FOV");
constexpr uint32_t kAttrSpline = 0xBC11E765u;        // NOTE: name unknown (spline name)
constexpr uint32_t kAttrStartOffset = AnimHash("StartOffset");
constexpr uint32_t kAttrSpeed = AnimHash("Speed");
constexpr uint32_t kAttrForward = AnimHash("forward");
constexpr uint32_t kAttrSprint = AnimHash("Sprint");
constexpr uint32_t kAttrMoveTo14 = 0xFCB43218u;      // NOTE: name unknown
constexpr uint32_t kAttrStartPoint = AnimHash("StartPoint");
constexpr uint32_t kAttrEndPoint = AnimHash("endpoint");
constexpr uint32_t kAttrCamera18 = 0x02A63CFAu;      // NOTE: names unknown for the next values
constexpr uint32_t kAttrCamera1C = 0x0FF6C00Cu;
constexpr uint32_t kAttrMoveTime = AnimHash("MoveTime");
constexpr uint32_t kAttrCamera24 = 0xC8FDDD9Eu;
constexpr uint32_t kAttrCamera28 = 0x0DC52BACu;
} // namespace

// ---------------------------------------------------------------------------
// "FadeDof" (NOTE: name from a hash match; registered hash 0xE9BDD91F):
// focus pull in the sequence.

class SeqFocusPullCommand : public ApeAnimScriptCommand {
public:
    SeqFocusPullCommand();
    explicit SeqFocusPullCommand(XmlElement* element);
    ~SeqFocusPullCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_target = 0;     // +0x0C attribute 0xED2B8734
    float m_value10 = 0.0f;    // +0x10 attribute 0x3C5308B5
    float m_value14 = 1.0f;    // +0x14 attribute 0x59D5C9EE
    float m_value18 = 10.0f;   // +0x18 attribute 0x7089C1B7
    float m_value1C = 20.0f;   // +0x1C attribute 0x070ADB70
    float m_value20 = 10.0f;   // +0x20 attribute 0xB1D3B545
    float m_value24 = 40.0f;   // +0x24 attribute 0x3DB85A7E
    float m_duration = 1.0f;   // +0x28 "duration"
    int32_t m_iterations = 1;  // +0x2C "numIterations"
    bool m_flag30 = false;     // +0x30 attribute 0x26873C74
    bool m_wait = true;        // +0x31 attribute 0x8488565B
    bool m_flag32 = false;     // +0x32 attribute 0x2E09C6B8
};

// @func 0x0009D7C8 SeqFocusPullCommand::SeqFocusPullCommand
SeqFocusPullCommand::SeqFocusPullCommand() = default;

// @func 0x00740BE4 SeqFocusPullCommand::~SeqFocusPullCommand
// @func 0x00740C14 SeqFocusPullCommand::~SeqFocusPullCommand (deleting)
SeqFocusPullCommand::~SeqFocusPullCommand() = default;

// @func 0x0009D850 SeqFocusPullCommand::SeqFocusPullCommand
SeqFocusPullCommand::SeqFocusPullCommand(XmlElement* element)
{
    element->GetHash(kAttrFocusTarget, &m_target, 0);
    element->GetFloat(kAttrFocus10, &m_value10, m_value10);
    element->GetFloat(kAttrFocus14, &m_value14, m_value14);
    element->GetFloat(kAttrFocus18, &m_value18, m_value18);
    element->GetFloat(kAttrFocus1C, &m_value1C, m_value1C);
    element->GetFloat(kAttrFocus20, &m_value20, m_value20);
    element->GetFloat(kAttrFocus24, &m_value24, m_value24);
    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    element->GetInt(kAttrNumIterations, &m_iterations, m_iterations);
    element->GetBool(kAttrFocus30, &m_flag30, m_flag30);
    element->GetBool(kAttrSeqWait, &m_wait, m_wait);
    element->GetBool(kAttrFocus32, &m_flag32, m_flag32);
}

// @func 0x0009DA78 SeqFocusPullCommand::Create
ApeAnimScriptCommand* SeqFocusPullCommand::Create(XmlElement* element) const
{
    return new SeqFocusPullCommand(element);
}

// @func 0x0009DAE8 SeqFocusPullCommand::Execute
void SeqFocusPullCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    const float values[7] = {m_value10, m_value14, m_value18, m_value1C, m_value20, m_value24, m_duration};
    SequenceAddFocusPull(m_target, values, m_iterations, m_flag30, m_wait, m_flag32);
}

// ---------------------------------------------------------------------------
// Hash 0x2D84F263 (name unknown): a shooter fires at a target in the sequence.

class SeqShootCommand : public ApeAnimScriptCommand {
public:
    SeqShootCommand();
    ~SeqShootCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_shooter = 0;  // +0x0C "Shooter"
    uint32_t m_target = 0;   // +0x10 "Target"
    uint32_t m_hash14 = 0;   // +0x14 attribute 0x1502BCC1
    int32_t m_index = 0;     // +0x18 attribute 0x21B3BD9E (1-based in data)
    bool m_flag1C = false;   // +0x1C attribute 0x9F451E77
};

// @func 0x0009DBA0 SeqShootCommand::SeqShootCommand
SeqShootCommand::SeqShootCommand() = default;

// @func 0x00740C6C SeqShootCommand::~SeqShootCommand
// @func 0x00740C9C SeqShootCommand::~SeqShootCommand (deleting)
SeqShootCommand::~SeqShootCommand() = default;

// @func 0x0009DBF8 SeqShootCommand::Create
ApeAnimScriptCommand* SeqShootCommand::Create(XmlElement* element) const
{
    auto* command = new SeqShootCommand();
    element->GetHash(kAttrShooter, &command->m_shooter, 0);
    element->GetHash(kAttrTarget, &command->m_target, command->m_target);
    element->GetHash(kAttrShoot14, &command->m_hash14, command->m_hash14);
    int32_t index = 0;
    if (element->GetInt(kAttrShootIndex, &index, 0) == ape::xml::kQuery_Ok)
        command->m_index = index > 0 ? index - 1 : 0;
    element->GetBool(kAttrShoot1C, &command->m_flag1C, command->m_flag1C);
    return command;
}

// @func 0x0009DD54 SeqShootCommand::Execute
void SeqShootCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_shooter != 0 && m_target != 0)
        SequenceAddShoot(m_shooter, m_target, m_hash14, m_index, m_flag1C);
}

// ---------------------------------------------------------------------------
// Hash 0x7C2668C0 (name unknown): shakes an object in the sequence.

class SeqObjectShakeCommand : public ApeAnimScriptCommand {
public:
    SeqObjectShakeCommand();
    explicit SeqObjectShakeCommand(XmlElement* element);
    ~SeqObjectShakeCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;              // +0x0C "object"
    uint32_t m_entity = 0;              // +0x10 "Entity"
    uint32_t m_objectInst = 0;          // +0x14 "ObjectInst"
    uint32_t m_field18 = 0x489EBD81u;   // +0x18 NOTE: constant set by the constructor, meaning unknown
    float m_field1C = 1.0f;             // +0x1C NOTE: never parsed
    uint32_t m_part = 0;                // +0x24 "part"
    float m_duration = 1.0f;            // +0x28 "duration"
    float m_magnitude = 1.0f;           // +0x2C "Magnitude"
    float m_startMagnitude = 0.0f;      // +0x30 attribute 0x793E3FEE
    uint32_t m_falloffMode = kFalloffLinear;  // +0x34 "FalloffMode"
    float m_falloffStartTime = 0.0f;    // +0x38 "FalloffStartTime"
    float m_pitch = 1.0f;               // +0x3C "Pitch"
    float m_yaw = 0.0f;                 // +0x40 "Yaw"
    float m_roll = 1.0f;                // +0x44 "Roll"
    float m_pitchFrequency = 4.0f;      // +0x48 "PitchFrequency"
    float m_yawFrequency = 6.5f;        // +0x4C "YawFrequency"
    float m_rollFrequency = 2.5f;       // +0x50 "RollFrequency"
    float m_offset[3] = {};             // +0x54 "XOffset", "YOffset", "ZOffset"
    float m_offsetFrequency[3] = {};    // +0x60 offset frequencies
    float m_field84 = -1.0f;            // +0x84 NOTE: never parsed
    bool m_wait = false;                // +0x8C attribute 0x8488565B
};

// @func 0x0009DDF4 SeqObjectShakeCommand::SeqObjectShakeCommand
SeqObjectShakeCommand::SeqObjectShakeCommand() = default;

// @func 0x00740CF4 SeqObjectShakeCommand::~SeqObjectShakeCommand
// @func 0x00740D24 SeqObjectShakeCommand::~SeqObjectShakeCommand (deleting)
SeqObjectShakeCommand::~SeqObjectShakeCommand() = default;

// @func 0x0009DEE8 SeqObjectShakeCommand::SeqObjectShakeCommand
SeqObjectShakeCommand::SeqObjectShakeCommand(XmlElement* element)
{
    if (element->GetHash(kAttrEntity, &m_entity, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrObjectInst, &m_objectInst, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok)
        return;
    element->GetHash(kAttrPart, &m_part, m_part);
    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    element->GetFloat(kAttrMagnitude, &m_magnitude, m_magnitude);
    element->GetFloat(kAttrStartMagnitude, &m_startMagnitude, m_startMagnitude);
    element->GetHash(kAttrFalloffMode, &m_falloffMode, m_falloffMode);
    element->GetFloat(kAttrFalloffStartTime, &m_falloffStartTime, m_falloffStartTime);
    element->GetFloat(kAttrPitch, &m_pitch, m_pitch);
    element->GetFloat(kAttrYaw, &m_yaw, m_yaw);
    element->GetFloat(kAttrRoll, &m_roll, m_roll);
    element->GetFloat(kAttrPitchFrequency, &m_pitchFrequency, m_pitchFrequency);
    element->GetFloat(kAttrYawFrequency, &m_yawFrequency, m_yawFrequency);
    element->GetFloat(kAttrRollFrequency, &m_rollFrequency, m_rollFrequency);
    element->GetFloat(kAttrXOffset, &m_offset[0], m_offset[0]);
    element->GetFloat(kAttrYOffset, &m_offset[1], m_offset[1]);
    element->GetFloat(kAttrZOffset, &m_offset[2], m_offset[2]);
    element->GetFloat(kAttrXOffsetFrequency, &m_offsetFrequency[0], m_offsetFrequency[0]);
    element->GetFloat(kAttrYOffsetFrequency, &m_offsetFrequency[1], m_offsetFrequency[1]);
    element->GetFloat(kAttrZOffsetFrequency, &m_offsetFrequency[2], m_offsetFrequency[2]);
    element->GetBool(kAttrSeqWait, &m_wait, m_wait);
}

// @func 0x0009E2E0 SeqObjectShakeCommand::Create
ApeAnimScriptCommand* SeqObjectShakeCommand::Create(XmlElement* element) const
{
    return new SeqObjectShakeCommand(element);
}

// TODO(0x0009E350 SeqObjectShakeCommand::Execute) resolves the object/part and builds the
// sequence shake command (0x00435854).

// ---------------------------------------------------------------------------
// Hash 0xD8B4387E (name unknown): sequence variant of the camera weight blend.

class SeqCameraWeightCommand : public ApeAnimScriptCommand {
public:
    SeqCameraWeightCommand();
    explicit SeqCameraWeightCommand(XmlElement* element);
    ~SeqCameraWeightCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_cameraWeight = 0.0f;  // +0x0C attribute 0x8F123DDD
    float m_duration = 0.0f;      // +0x10 "duration"
    float m_exponent = 1.0f;      // +0x14 "Exponent" (default from "Curve")
};

// @func 0x0009E4CC SeqCameraWeightCommand::SeqCameraWeightCommand
SeqCameraWeightCommand::SeqCameraWeightCommand() = default;

// @func 0x00740D7C SeqCameraWeightCommand::~SeqCameraWeightCommand
// @func 0x00740DAC SeqCameraWeightCommand::~SeqCameraWeightCommand (deleting)
SeqCameraWeightCommand::~SeqCameraWeightCommand() = default;

// @func 0x0009E528 SeqCameraWeightCommand::SeqCameraWeightCommand
SeqCameraWeightCommand::SeqCameraWeightCommand(XmlElement* element)
{
    element->GetFloat(kAttrCameraWeight, &m_cameraWeight, 1.0f);
    element->GetFloat(kAttrDuration, &m_duration, 0.0f);
    uint32_t curve = 0;
    if (element->GetHash(kAttrCurve, &curve, 0) == ape::xml::kQuery_Ok) {
        if (curve == kCurve_8105148C)
            m_exponent = 0.5f;
        else if (curve == kCurve_Exp)
            m_exponent = 2.0f;
        else
            m_exponent = 1.0f;
    }
    element->GetFloat(kAttrExponent, &m_exponent, 0.0f);
    if (m_duration < 0.0f)
        m_duration = 0.0f;
}

// @func 0x0009E69C SeqCameraWeightCommand::Create
ApeAnimScriptCommand* SeqCameraWeightCommand::Create(XmlElement* element) const
{
    return new SeqCameraWeightCommand(element);
}

// @func 0x0009E70C SeqCameraWeightCommand::Execute
void SeqCameraWeightCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddCameraWeight(m_cameraWeight, m_duration, m_exponent);
}

// ---------------------------------------------------------------------------
// "JumpTo" (NOTE: name from a hash match): a character jumps to a point.

class SeqJumpToCommand : public ApeAnimScriptCommand {
public:
    SeqJumpToCommand();
    ~SeqJumpToCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_point = 0;   // +0x10 attribute 0x8D47DA8F
    uint32_t m_anim = 0;    // +0x14 "anim"
    bool m_flag18 = false;  // +0x18 attribute 0xDFD4E345
    bool m_wait = true;     // +0x19 attribute 0x8488565B
};

// @func 0x0009E78C SeqJumpToCommand::SeqJumpToCommand
SeqJumpToCommand::SeqJumpToCommand() = default;

// @func 0x00740E04 SeqJumpToCommand::~SeqJumpToCommand
// @func 0x00740E34 SeqJumpToCommand::~SeqJumpToCommand (deleting)
SeqJumpToCommand::~SeqJumpToCommand() = default;

// @func 0x0009E7E4 SeqJumpToCommand::Create
ApeAnimScriptCommand* SeqJumpToCommand::Create(XmlElement* element) const
{
    auto* command = new SeqJumpToCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrPtName, &command->m_point, command->m_point);
    element->GetHash(kAttrAnim, &command->m_anim, command->m_anim);
    element->GetBool(kAttrJump18, &command->m_flag18, command->m_flag18);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x0009E928 SeqJumpToCommand::Execute
void SeqJumpToCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_point == 0 || m_entity == 0)
        return;
    WorldObject* point = FindPointInstance(m_point);
    if (!point) {
        ape::TempHexString(m_point);
        return;
    }
    SequenceAddJumpTo(m_entity, point, m_anim, m_flag18, m_wait);
}

// ---------------------------------------------------------------------------
// "KillEntity" (NOTE: name from a hash match): removes an entity in the sequence.

class SeqKillEntityCommand : public ApeAnimScriptCommand {
public:
    SeqKillEntityCommand();
    ~SeqKillEntityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_name = 0;    // +0x0C "name"
    bool m_flag10 = false;  // +0x10 attribute 0x66D71193
};

// @func 0x0009EA0C SeqKillEntityCommand::SeqKillEntityCommand
SeqKillEntityCommand::SeqKillEntityCommand() = default;

// @func 0x00740E8C SeqKillEntityCommand::~SeqKillEntityCommand
// @func 0x00740EBC SeqKillEntityCommand::~SeqKillEntityCommand (deleting)
SeqKillEntityCommand::~SeqKillEntityCommand() = default;

// @func 0x0009EA54 SeqKillEntityCommand::Create
ApeAnimScriptCommand* SeqKillEntityCommand::Create(XmlElement* element) const
{
    auto* command = new SeqKillEntityCommand();
    element->GetHash(kAttrName, &command->m_name, 0);
    element->GetBool(kAttrKill10, &command->m_flag10, command->m_flag10);
    return command;
}

// @func 0x0009EB1C SeqKillEntityCommand::Execute
void SeqKillEntityCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_name != 0)
        SequenceAddRemoveEntity(m_name, m_flag10);
}

// ---------------------------------------------------------------------------
// "LookAt" (NOTE: name from a hash match)

class SeqLookAtCommand : public ApeAnimScriptCommand {
public:
    SeqLookAtCommand();
    ~SeqLookAtCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;   // +0x0C "Entity"
    uint32_t m_target = 0;   // +0x10 "Target"
    uint32_t m_hash14 = 0;   // +0x14 attribute 0xCF0502B9 (alternative to Entity)
    uint32_t m_hash18 = 0;   // +0x18 attribute 0xF636DCCA (alternative to Target)
    float m_duration = 0.0f; // +0x1C "duration"
    float m_value20 = 0.0f;  // +0x20 attribute 0xBE36C5F4
    float m_fov = 0.0f;      // +0x24 "FOV"
};

// @func 0x0009EBA0 SeqLookAtCommand::SeqLookAtCommand
SeqLookAtCommand::SeqLookAtCommand() = default;

// @func 0x00740F14 SeqLookAtCommand::~SeqLookAtCommand
// @func 0x00740F44 SeqLookAtCommand::~SeqLookAtCommand (deleting)
SeqLookAtCommand::~SeqLookAtCommand() = default;

// @func 0x0009EC00 SeqLookAtCommand::Create
ApeAnimScriptCommand* SeqLookAtCommand::Create(XmlElement* element) const
{
    auto* command = new SeqLookAtCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrTarget, &command->m_target, command->m_target);
    element->GetHash(kAttrLook14, &command->m_hash14, command->m_hash14);
    element->GetHash(kAttrLook18, &command->m_hash18, command->m_hash18);
    element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
    element->GetFloat(kAttrLook20, &command->m_value20, command->m_value20);
    element->GetFloat(kAttrFov, &command->m_fov, command->m_fov);
    return command;
}

// @func 0x0009ED90 SeqLookAtCommand::Execute
void SeqLookAtCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if ((m_entity != 0 || m_hash14 != 0) && (m_target != 0 || m_hash18 != 0))
        SequenceAddLookAt(m_duration, m_value20, m_fov, m_entity, m_target, m_hash14, m_hash18);
}

// ---------------------------------------------------------------------------
// "MoveAlong" (NOTE: name from a hash match): a character follows a spline.

class SeqMoveAlongCommand : public ApeAnimScriptCommand {
public:
    SeqMoveAlongCommand();
    ~SeqMoveAlongCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;       // +0x0C "Entity"
    uint32_t m_spline = 0;       // +0x10 attribute 0xBC11E765
    uint32_t m_anim = 0;         // +0x14 "anim"
    float m_startOffset = 0.0f;  // +0x18 "StartOffset"
    float m_speed = 1.0f;        // +0x1C "Speed"
    bool m_forward = true;       // +0x20 "forward"
    bool m_sprint = false;       // +0x21 "Sprint"
    bool m_wait = true;          // +0x22 attribute 0x8488565B
};

// @func 0x0009EE54 SeqMoveAlongCommand::SeqMoveAlongCommand
SeqMoveAlongCommand::SeqMoveAlongCommand() = default;

// @func 0x00740F9C SeqMoveAlongCommand::~SeqMoveAlongCommand
// @func 0x00740FCC SeqMoveAlongCommand::~SeqMoveAlongCommand (deleting)
SeqMoveAlongCommand::~SeqMoveAlongCommand() = default;

// @func 0x0009EEBC SeqMoveAlongCommand::Create
ApeAnimScriptCommand* SeqMoveAlongCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMoveAlongCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrSpline, &command->m_spline, command->m_spline);
    element->GetHash(kAttrAnim, &command->m_anim, command->m_anim);
    element->GetFloat(kAttrStartOffset, &command->m_startOffset, command->m_startOffset);
    element->GetFloat(kAttrSpeed, &command->m_speed, command->m_speed);
    element->GetBool(kAttrForward, &command->m_forward, command->m_forward);
    element->GetBool(kAttrSprint, &command->m_sprint, command->m_sprint);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x0009F070 SeqMoveAlongCommand::Execute
void SeqMoveAlongCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_spline == 0 || m_entity == 0)
        return;
    if (void* spline = FindLevelSpline(m_spline))
        SequenceAddMoveAlong(m_entity, spline, m_anim, m_startOffset, m_speed, m_forward, m_sprint, m_wait);
}

// ---------------------------------------------------------------------------
// "MoveTo" (NOTE: name from a hash match)

class SeqMoveToCommand : public ApeAnimScriptCommand {
public:
    SeqMoveToCommand();
    ~SeqMoveToCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_point = 0;   // +0x10 attribute 0x8D47DA8F
    uint32_t m_hash14 = 0;  // +0x14 attribute 0xFCB43218
    uint32_t m_anim = 0;    // +0x18 "anim"
    float m_speed = 1.0f;   // +0x1C "Speed"
    bool m_sprint = false;  // +0x20 "Sprint"
    bool m_wait = true;     // +0x21 attribute 0x8488565B
};

// @func 0x0009F3FC SeqMoveToCommand::SeqMoveToCommand
SeqMoveToCommand::SeqMoveToCommand() = default;

// @func 0x007410AC SeqMoveToCommand::~SeqMoveToCommand
// @func 0x007410DC SeqMoveToCommand::~SeqMoveToCommand (deleting)
SeqMoveToCommand::~SeqMoveToCommand() = default;

// @func 0x0009F464 SeqMoveToCommand::Create
ApeAnimScriptCommand* SeqMoveToCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMoveToCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrPtName, &command->m_point, command->m_point);
    element->GetHash(kAttrMoveTo14, &command->m_hash14, command->m_hash14);
    element->GetHash(kAttrAnim, &command->m_anim, command->m_anim);
    element->GetFloat(kAttrSpeed, &command->m_speed, command->m_speed);
    element->GetBool(kAttrSprint, &command->m_sprint, command->m_sprint);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// TODO(0x0009F5F8 SeqMoveToCommand::Execute) point or entity target lookup and the sequence
// move command (0x00431848).

// ---------------------------------------------------------------------------
// "MoveCamera" (NOTE: name from a hash match)

class SeqMoveCameraCommand : public ApeAnimScriptCommand {
public:
    SeqMoveCameraCommand();
    ~SeqMoveCameraCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_startPoint = 0;  // +0x0C "StartPoint"
    uint32_t m_endPoint = 0;    // +0x10 "endpoint"
    float m_fov = 0.0f;         // +0x14 "FOV"
    float m_value18 = 0.0f;     // +0x18 attribute 0x02A63CFA
    float m_value1C = 0.0f;     // +0x1C attribute 0x0FF6C00C
    float m_moveTime = 0.0f;    // +0x20 "MoveTime"
    uint32_t m_hash24 = 0;      // +0x24 attribute 0xC8FDDD9E
    float m_value28 = 0.0f;     // +0x28 attribute 0x0DC52BAC
};

// @func 0x0009F154 SeqMoveCameraCommand::SeqMoveCameraCommand
SeqMoveCameraCommand::SeqMoveCameraCommand() = default;

// @func 0x00741024 SeqMoveCameraCommand::~SeqMoveCameraCommand
// @func 0x00741054 SeqMoveCameraCommand::~SeqMoveCameraCommand (deleting)
SeqMoveCameraCommand::~SeqMoveCameraCommand() = default;

// @func 0x0009F1B4 SeqMoveCameraCommand::Create
ApeAnimScriptCommand* SeqMoveCameraCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMoveCameraCommand();
    element->GetHash(kAttrStartPoint, &command->m_startPoint, 0);
    element->GetHash(kAttrEndPoint, &command->m_endPoint, command->m_endPoint);
    element->GetFloat(kAttrFov, &command->m_fov, command->m_fov);
    element->GetFloat(kAttrCamera18, &command->m_value18, command->m_value18);
    element->GetFloat(kAttrCamera1C, &command->m_value1C, command->m_value1C);
    element->GetFloat(kAttrMoveTime, &command->m_moveTime, command->m_moveTime);
    element->GetHash(kAttrCamera24, &command->m_hash24, command->m_hash24);
    element->GetFloat(kAttrCamera28, &command->m_value28, command->m_value28);
    return command;
}

// @func 0x0009F360 SeqMoveCameraCommand::Execute
void SeqMoveCameraCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    // NOTE: the hash +0x24 is not passed on by the console.
    SequenceAddMoveCamera(m_fov, m_value18, m_value1C, m_moveTime, m_value28, m_startPoint, m_endPoint);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsZ()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("FadeDof"), new SeqFocusPullCommand());
    prototypes.Add(0x2D84F263u, new SeqShootCommand());
    prototypes.Add(0x7C2668C0u, new SeqObjectShakeCommand());
    prototypes.Add(0xD8B4387Eu, new SeqCameraWeightCommand());
    prototypes.Add(AnimHash("JumpTo"), new SeqJumpToCommand());
    prototypes.Add(AnimHash("KillEntity"), new SeqKillEntityCommand());
    prototypes.Add(AnimHash("LookAt"), new SeqLookAtCommand());
    prototypes.Add(AnimHash("MoveAlong"), new SeqMoveAlongCommand());
    prototypes.Add(AnimHash("MoveTo"), new SeqMoveToCommand());
    prototypes.Add(AnimHash("MoveCamera"), new SeqMoveCameraCommand());
}

} // namespace levels
} // namespace sly
