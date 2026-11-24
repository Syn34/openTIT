// openTIT - Game script commands, twenty-second block: the first "Seq..." commands.
//
// Seq commands do not act directly: their Execute builds a cut-scene
// sequence command (sly/gameplay sequencer, 0x0042C000..) and appends it to
// the running sequence (0x0042C138). Those classes are reached through the
// SequenceAdd* services.
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrActivate = AnimHash("activate");
constexpr uint32_t kAttrMixState = AnimHash("MixState");
constexpr uint32_t kAttrCrossfadeDuration = AnimHash("CrossfadeDuration");
constexpr uint32_t kAttrParentEntity = AnimHash("ParentEntity");
constexpr uint32_t kAttrChildEntity = AnimHash("ChildEntity");
constexpr uint32_t kAttrLink14 = 0x20C00073u;  // NOTE: names unknown for the link attributes below
constexpr uint32_t kAttrLink18 = 0x277591B4u;
constexpr uint32_t kAttrLink20 = 0xEF4DDD13u;
constexpr uint32_t kAttrLink30 = 0x1612F114u;
constexpr uint32_t kAttrLink40 = 0xC7400937u;
constexpr uint32_t kAttrLink50 = 0xF4F1A270u;
constexpr uint32_t kAttrLink54 = 0xF91B96E2u;
constexpr uint32_t kAttrLink60 = 0x1F1D93C2u;
constexpr uint32_t kAttrLink70 = 0xA4F068E8u;
constexpr uint32_t kAttrLink80 = 0x36E7D729u;
constexpr uint32_t kAttrLink81 = 0x3E3F7E94u;
constexpr uint32_t kAttrLink82 = 0xD1A6825Eu;
constexpr uint32_t kAttrLink83 = 0x3E496BEBu;
constexpr uint32_t kAttrGroup = AnimHash("group");
constexpr uint32_t kAttrEvent = AnimHash("event");
constexpr uint32_t kAttrDataType = AnimHash("datatype");
constexpr uint32_t kAttrData = AnimHash("Data");
constexpr uint32_t kTypeFloat = AnimHash("Float");
constexpr uint32_t kTypeHash = AnimHash("hash");
constexpr uint32_t kTypeInt = AnimHash("int");
constexpr uint32_t kAttrCameraDef = AnimHash("CameraDef");
constexpr uint32_t kAttrPtName = 0x8D47DA8Fu;   // NOTE: name unknown (point instance name)
constexpr uint32_t kAttrFov = AnimHash("FOV");
constexpr uint32_t kAttrReleaseTime = 0x230A334Du;  // NOTE: name unknown
constexpr uint32_t kAttrSeqWait = 0x8488565Bu;      // NOTE: name unknown; shared by several Seq commands
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrRadius = AnimHash("radius");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrMagnitude = AnimHash("Magnitude");
constexpr uint32_t kAttrStartMagnitude = 0x793E3FEEu;  // NOTE: name unknown
constexpr uint32_t kAttrFalloffMode = AnimHash("FalloffMode");
constexpr uint32_t kAttrFalloffStartTime = AnimHash("FalloffStartTime");
constexpr uint32_t kAttrPitch = AnimHash("Pitch");
constexpr uint32_t kAttrYaw = AnimHash("Yaw");
constexpr uint32_t kAttrRoll = AnimHash("Roll");
constexpr uint32_t kAttrPitchFrequency = AnimHash("PitchFrequency");
constexpr uint32_t kAttrYawFrequency = AnimHash("YawFrequency");
constexpr uint32_t kAttrRollFrequency = AnimHash("RollFrequency");
constexpr uint32_t kFalloffLinear = AnimHash("Linear");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrDamage14 = 0x61CAD819u;  // NOTE: name unknown
constexpr uint32_t kAttrDamage18 = 0x806E6EC3u;  // NOTE: name unknown
constexpr uint32_t kAttrDamage = AnimHash("Damage");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrDamage20 = 0x1B75F0DAu;  // NOTE: name unknown

constexpr float kDegToRad = 0.017453292f;
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};  // NOTE: console global 0x01164B40

ape::Vec3 ToRadians(const ape::Vec3& v)
{
    return {v.x * kDegToRad, v.y * kDegToRad, v.z * kDegToRad, v.pad};
}
} // namespace

// ---------------------------------------------------------------------------
// "SeqActivateMixState" (NOTE: name from a hash match)

class SeqActivateMixStateCommand : public ApeAnimScriptCommand {
public:
    SeqActivateMixStateCommand();
    ~SeqActivateMixStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_activate = true;                       // +0x0C "activate"
    uint32_t m_mixStateName = 0;                  // +0x10 "MixState"
    const audio::MixState* m_mixState = nullptr;  // +0x14
    float m_crossfadeDuration = 0.0f;             // +0x18 "CrossfadeDuration"
};

// @func 0x0009A340 SeqActivateMixStateCommand::SeqActivateMixStateCommand
SeqActivateMixStateCommand::SeqActivateMixStateCommand() = default;

// @func 0x007400C0 SeqActivateMixStateCommand::~SeqActivateMixStateCommand
// @func 0x007400F0 SeqActivateMixStateCommand::~SeqActivateMixStateCommand (deleting)
SeqActivateMixStateCommand::~SeqActivateMixStateCommand() = default;

// @func 0x0009A398 SeqActivateMixStateCommand::Link
// (byte-identical to ActivateMixStateCommand::Link, 0x0006BB10)
void SeqActivateMixStateCommand::Link()
{
    if (!m_mixState && m_mixStateName != 0)
        m_mixState = FindMixState(m_mixStateName);
}

// @func 0x0009A3E4 SeqActivateMixStateCommand::Create
ApeAnimScriptCommand* SeqActivateMixStateCommand::Create(XmlElement* element) const
{
    auto* command = new SeqActivateMixStateCommand();
    element->GetBool(kAttrActivate, &command->m_activate, true);
    element->GetHash(kAttrMixState, &command->m_mixStateName, 0);
    element->GetFloat(kAttrCrossfadeDuration, &command->m_crossfadeDuration, command->m_crossfadeDuration);
    if (command->m_mixStateName != 0)
        command->m_mixState = FindMixState(command->m_mixStateName);
    return command;
}

// @func 0x0009A4F8 SeqActivateMixStateCommand::Execute
void SeqActivateMixStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!m_mixState) {
        ape::TempHexString(m_mixStateName);  // the release build's warning prints nothing
        return;
    }
    SequenceAddMixState(m_crossfadeDuration, m_activate, m_mixState);
}

// ---------------------------------------------------------------------------
// Hash 0xA1DF0C27 (name unknown): links a child entity to a parent in the
// sequence, with offsets and rotations.

class SeqLinkCommand : public ApeAnimScriptCommand {
public:
    SeqLinkCommand();
    explicit SeqLinkCommand(XmlElement* element);
    ~SeqLinkCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_parentEntity = 0;   // +0x0C "ParentEntity"
    uint32_t m_childEntity = 0;    // +0x10 "ChildEntity"
    uint32_t m_hash14 = 0;         // +0x14 attribute 0x20C00073
    uint32_t m_hash18 = 0;         // +0x18 attribute 0x277591B4
    ape::Vec3 m_vector20 = kZero;  // +0x20 attribute 0xEF4DDD13
    uint32_t m_hash30 = 0;         // +0x30 attribute 0x1612F114
    ape::Vec3 m_angles40 = kZero;  // +0x40 attribute 0xC7400937 (degrees -> radians)
    uint32_t m_hash50 = 0;         // +0x50 attribute 0xF4F1A270
    uint32_t m_hash54 = 0;         // +0x54 attribute 0xF91B96E2
    ape::Vec3 m_vector60 = kZero;  // +0x60 attribute 0x1F1D93C2
    ape::Vec3 m_angles70 = kZero;  // +0x70 attribute 0xA4F068E8 (degrees -> radians)
    bool m_flag80 = false;         // +0x80 attribute 0x36E7D729
    bool m_flag81 = false;         // +0x81 attribute 0x3E3F7E94
    bool m_flag82 = false;         // +0x82 attribute 0xD1A6825E
    bool m_flag83 = false;         // +0x83 attribute 0x3E496BEB
};

// @func 0x0009A594 SeqLinkCommand::SeqLinkCommand
SeqLinkCommand::SeqLinkCommand() = default;

// @func 0x00740148 SeqLinkCommand::~SeqLinkCommand
// @func 0x00740178 SeqLinkCommand::~SeqLinkCommand (deleting)
SeqLinkCommand::~SeqLinkCommand() = default;

// @func 0x0009A670 SeqLinkCommand::SeqLinkCommand
SeqLinkCommand::SeqLinkCommand(XmlElement* element)
{
    element->GetHash(kAttrParentEntity, &m_parentEntity, 0);
    element->GetHash(kAttrChildEntity, &m_childEntity, m_childEntity);
    element->GetHash(kAttrLink14, &m_hash14, m_hash14);
    element->GetHash(kAttrLink18, &m_hash18, m_hash18);
    element->GetVec3(kAttrLink20, &m_vector20.x, &m_vector20.x);
    element->GetHash(kAttrLink30, &m_hash30, m_hash30);
    element->GetVec3(kAttrLink40, &m_angles40.x, &m_angles40.x);
    element->GetHash(kAttrLink50, &m_hash50, m_hash50);
    m_angles40 = ToRadians(m_angles40);
    element->GetHash(kAttrLink54, &m_hash54, m_hash54);
    element->GetVec3(kAttrLink60, &m_vector60.x, &m_vector60.x);
    element->GetVec3(kAttrLink70, &m_angles70.x, &m_angles70.x);
    m_angles70 = ToRadians(m_angles70);
    element->GetBool(kAttrLink80, &m_flag80, m_flag80);
    element->GetBool(kAttrLink81, &m_flag81, m_flag81);
    element->GetBool(kAttrLink82, &m_flag82, m_flag82);
    element->GetBool(kAttrLink83, &m_flag83, m_flag83);
}

// @func 0x0009A9B0 SeqLinkCommand::Create
ApeAnimScriptCommand* SeqLinkCommand::Create(XmlElement* element) const
{
    return new SeqLinkCommand(element);
}

// TODO(0x0009AA20 SeqLinkCommand::Execute) builds the sequence link command (0x00431254 ...).

// ---------------------------------------------------------------------------
// "SeqPostMessage" style command (hash 0xFC622478, name unknown): posts a
// game event to a group as part of the sequence.

class SeqPostEventCommand : public ApeAnimScriptCommand {
public:
    SeqPostEventCommand();
    ~SeqPostEventCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_group = 0;  // +0x0C "group"
    uint32_t m_event = 0;  // +0x10 "event"
    uint32_t m_data = 0;   // +0x14 "Data" (float, hash or int by "datatype")
};

// @func 0x0009AF14 SeqPostEventCommand::SeqPostEventCommand
SeqPostEventCommand::SeqPostEventCommand() = default;

// @func 0x00740258 SeqPostEventCommand::~SeqPostEventCommand
// @func 0x00740288 SeqPostEventCommand::~SeqPostEventCommand (deleting)
SeqPostEventCommand::~SeqPostEventCommand() = default;

// @func 0x0009AF60 SeqPostEventCommand::Create
ApeAnimScriptCommand* SeqPostEventCommand::Create(XmlElement* element) const
{
    auto* command = new SeqPostEventCommand();
    element->GetHash(kAttrGroup, &command->m_group, 0);
    element->GetHash(kAttrEvent, &command->m_event, command->m_event);
    uint32_t type = 0;
    element->GetHash(kAttrDataType, &type, 0);
    if (type == kTypeFloat) {
        float value = 0.0f;
        element->GetFloat(kAttrData, &value, 0.0f);
        std::memcpy(&command->m_data, &value, sizeof(value));
    } else if (type == kTypeHash) {
        element->GetHash(kAttrData, &command->m_data, 0);
    } else if (type == kTypeInt) {
        int32_t value = 0;
        element->GetInt(kAttrData, &value, 0);
        command->m_data = static_cast<uint32_t>(value);
    }
    return command;
}

// @func 0x0009B0F0 SeqPostEventCommand::Execute
void SeqPostEventCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddPostEvent(m_group, m_event, m_data);
}

// ---------------------------------------------------------------------------
// "SeqCameraCut" style command (hash 0x91B5C0DD, name unknown): cuts the
// sequence camera to a point.

class SeqCameraCutCommand : public ApeAnimScriptCommand {
public:
    SeqCameraCutCommand();
    ~SeqCameraCutCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_cameraDef = 0;  // +0x0C "CameraDef"
    uint32_t m_point = 0;      // +0x10 attribute 0x8D47DA8F
    float m_fov = 0.0f;        // +0x14 "FOV"
};

// @func 0x0009AC9C SeqCameraCutCommand::SeqCameraCutCommand
SeqCameraCutCommand::SeqCameraCutCommand() = default;

// @func 0x007401D0 SeqCameraCutCommand::~SeqCameraCutCommand
// @func 0x00740200 SeqCameraCutCommand::~SeqCameraCutCommand (deleting)
SeqCameraCutCommand::~SeqCameraCutCommand() = default;

// @func 0x0009ACEC SeqCameraCutCommand::Create
ApeAnimScriptCommand* SeqCameraCutCommand::Create(XmlElement* element) const
{
    auto* command = new SeqCameraCutCommand();
    element->GetHash(kAttrCameraDef, &command->m_cameraDef, 0);
    element->GetHash(kAttrPtName, &command->m_point, command->m_point);
    element->GetFloat(kAttrFov, &command->m_fov, command->m_fov);
    return command;
}

// TODO(0x0009ADE0 SeqCameraCutCommand::Execute) point lookup and the sequence camera command
// (0x0043280C).

// ---------------------------------------------------------------------------
// Hash 0x99F875FE (name unknown): releases the sequence camera.

class SeqCameraReleaseCommand : public ApeAnimScriptCommand {
public:
    SeqCameraReleaseCommand();
    ~SeqCameraReleaseCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    float m_time = 0.0f;  // +0x0C attribute 0x230A334D
    bool m_wait = false;  // +0x10 attribute 0x8488565B
};

// @func 0x0009B170 SeqCameraReleaseCommand::SeqCameraReleaseCommand
SeqCameraReleaseCommand::SeqCameraReleaseCommand() = default;

// @func 0x007402E0 SeqCameraReleaseCommand::~SeqCameraReleaseCommand
// @func 0x00740310 SeqCameraReleaseCommand::~SeqCameraReleaseCommand (deleting)
SeqCameraReleaseCommand::~SeqCameraReleaseCommand() = default;

// @func 0x0009B1D0 SeqCameraReleaseCommand::Create
ApeAnimScriptCommand* SeqCameraReleaseCommand::Create(XmlElement* element) const
{
    auto* command = new SeqCameraReleaseCommand();
    element->GetFloat(kAttrReleaseTime, &command->m_time, 0.0f);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x0009B2A4 SeqCameraReleaseCommand::Execute
void SeqCameraReleaseCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    SequenceAddCameraRelease(m_time, instance, m_wait);
}

// ---------------------------------------------------------------------------
// "SeqCameraShake" style command (hash 0x053879BE, name unknown)

class SeqCameraShakeCommand : public ApeAnimScriptCommand {
public:
    SeqCameraShakeCommand();
    explicit SeqCameraShakeCommand(XmlElement* element);
    ~SeqCameraShakeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;         // +0x0C "Entity"
    SequenceShakeParams m_params;  // +0x10..+0x6F
    bool m_wait = false;           // +0x70 attribute 0x8488565B
};

// @func 0x0009B31C SeqCameraShakeCommand::SeqCameraShakeCommand
SeqCameraShakeCommand::SeqCameraShakeCommand() = default;

// @func 0x00740368 SeqCameraShakeCommand::~SeqCameraShakeCommand
// @func 0x00740398 SeqCameraShakeCommand::~SeqCameraShakeCommand (deleting)
SeqCameraShakeCommand::~SeqCameraShakeCommand() = default;

// @func 0x0009B3DC SeqCameraShakeCommand::SeqCameraShakeCommand
SeqCameraShakeCommand::SeqCameraShakeCommand(XmlElement* element)
{
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetFloat(kAttrRadius, &m_params.radius, m_params.radius);
    element->GetFloat(kAttrDuration, &m_params.duration, m_params.duration);
    element->GetFloat(kAttrMagnitude, &m_params.magnitude, m_params.magnitude);
    element->GetFloat(kAttrStartMagnitude, &m_params.startMagnitude, m_params.startMagnitude);
    element->GetHash(kAttrFalloffMode, &m_params.falloffMode, m_params.falloffMode);
    element->GetFloat(kAttrFalloffStartTime, &m_params.falloffStartTime, m_params.falloffStartTime);
    element->GetFloat(kAttrPitch, &m_params.pitch, m_params.pitch);
    element->GetFloat(kAttrYaw, &m_params.yaw, m_params.yaw);
    element->GetFloat(kAttrRoll, &m_params.roll, m_params.roll);
    element->GetFloat(kAttrPitchFrequency, &m_params.pitchFrequency, m_params.pitchFrequency);
    element->GetFloat(kAttrYawFrequency, &m_params.yawFrequency, m_params.yawFrequency);
    // The console reads RollFrequency twice.
    element->GetFloat(kAttrRollFrequency, &m_params.rollFrequency, m_params.rollFrequency);
    element->GetBool(kAttrSeqWait, &m_wait, m_wait);
}

// @func 0x0009B698 SeqCameraShakeCommand::Create
ApeAnimScriptCommand* SeqCameraShakeCommand::Create(XmlElement* element) const
{
    return new SeqCameraShakeCommand(element);
}

// @func 0x0009B708 SeqCameraShakeCommand::Execute
void SeqCameraShakeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    SequenceAddCameraShake(m_entity, m_params, m_wait);
}

// ---------------------------------------------------------------------------
// Hash 0x03D3CE95 (name unknown): moves a character to a point in the sequence.

class SeqMoveToPointCommand : public ApeAnimScriptCommand {
public:
    SeqMoveToPointCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;  // +0x0C "Entity"
    uint32_t m_point = 0;   // +0x10 attribute 0x8D47DA8F
    bool m_wait = true;     // +0x14 attribute 0x8488565B
};

// @func 0x0009B78C SeqMoveToPointCommand::SeqMoveToPointCommand
SeqMoveToPointCommand::SeqMoveToPointCommand() = default;

// @func 0x0009B7DC SeqMoveToPointCommand::Create
ApeAnimScriptCommand* SeqMoveToPointCommand::Create(XmlElement* element) const
{
    auto* command = new SeqMoveToPointCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrPtName, &command->m_point, command->m_point);
    element->GetBool(kAttrSeqWait, &command->m_wait, command->m_wait);
    return command;
}

// @func 0x0009B8D4 SeqMoveToPointCommand::Execute
void SeqMoveToPointCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_point == 0)
        return;
    WorldObject* point = FindPointInstance(m_point);
    if (!point) {
        ape::TempHexString(m_point);
        return;
    }
    SequenceAddMoveToPoint(m_entity, point, m_wait);
}

// ---------------------------------------------------------------------------
// "SeqDamage" (NOTE: name from a hash match)

class SeqDamageCommand : public ApeAnimScriptCommand {
public:
    SeqDamageCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;   // +0x0C "object"
    uint32_t m_child = 0;    // +0x10 "Child"
    uint32_t m_hash14 = 0;   // +0x14 attribute 0x61CAD819
    uint32_t m_hash18 = 0;   // +0x18 attribute 0x806E6EC3
    float m_damage = 0.0f;   // +0x1C "Damage"
    float m_value20 = 0.0f;  // +0x20 attribute 0x1B75F0DA
    uint32_t m_type = 0;     // +0x24 "Type"
};

// @func 0x0009B9A4 SeqDamageCommand::SeqDamageCommand
SeqDamageCommand::SeqDamageCommand() = default;

// @func 0x0009B9FC SeqDamageCommand::Create
ApeAnimScriptCommand* SeqDamageCommand::Create(XmlElement* element) const
{
    auto* command = new SeqDamageCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetHash(kAttrDamage14, &command->m_hash14, command->m_hash14);
    element->GetHash(kAttrDamage18, &command->m_hash18, command->m_hash18);
    element->GetFloat(kAttrDamage, &command->m_damage, command->m_damage);
    element->GetHash(kAttrType, &command->m_type, command->m_type);
    element->GetFloat(kAttrDamage20, &command->m_value20, command->m_value20);
    return command;
}

// TODO(0x0009BB80 SeqDamageCommand::Execute) builds the sequence damage command (0x004338B4).

// ---------------------------------------------------------------------------
// "SeqAttach" style command (hash 0xFF8D82F5, name unknown)

class SeqAttachCommand : public ApeAnimScriptCommand {
public:
    SeqAttachCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_parentEntity = 0;  // +0x0C "ParentEntity"
    uint32_t m_childEntity = 0;   // +0x10 "ChildEntity"
};

// @func 0x0009BCA4 SeqAttachCommand::SeqAttachCommand
SeqAttachCommand::SeqAttachCommand() = default;

// @func 0x0009BCEC SeqAttachCommand::Create
ApeAnimScriptCommand* SeqAttachCommand::Create(XmlElement* element) const
{
    auto* command = new SeqAttachCommand();
    element->GetHash(kAttrParentEntity, &command->m_parentEntity, 0);
    element->GetHash(kAttrChildEntity, &command->m_childEntity, command->m_childEntity);
    return command;
}

// @func 0x0009BDB8 SeqAttachCommand::Execute
void SeqAttachCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_parentEntity != 0 && m_childEntity != 0)
        SequenceAddAttach(m_parentEntity, m_childEntity);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsX()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SeqActivateMixState"), new SeqActivateMixStateCommand());
    prototypes.Add(0xA1DF0C27u, new SeqLinkCommand());
    prototypes.Add(0xFC622478u, new SeqPostEventCommand());
    prototypes.Add(0x91B5C0DDu, new SeqCameraCutCommand());
    prototypes.Add(0x99F875FEu, new SeqCameraReleaseCommand());
    prototypes.Add(0x053879BEu, new SeqCameraShakeCommand());
    prototypes.Add(0x03D3CE95u, new SeqMoveToPointCommand());
    prototypes.Add(AnimHash("SeqDamage"), new SeqDamageCommand());
    prototypes.Add(0xFF8D82F5u, new SeqAttachCommand());
}

} // namespace levels
} // namespace sly
