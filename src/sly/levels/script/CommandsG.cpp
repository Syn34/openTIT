// openTIT - Game script commands, fourth alphabetical block (FxObjShake to GetJobState).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

using ape::xml::Color8;

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrObjectInst = AnimHash("ObjectInst");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrDurationVar = AnimHash("DurationVar");
constexpr uint32_t kAttrMagnitude = AnimHash("Magnitude");
constexpr uint32_t kAttrStartMagnitude = AnimHash("StartMagnitude");
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
constexpr uint32_t kAttrLineDimension = AnimHash("LineDimension");
constexpr uint32_t kAttrCullDist = AnimHash("CullDist");
// Shake attributes whose names were not recovered, in field order (+0x68..).
constexpr uint32_t kShakeFloatAttrs[8] = {
    0xA99476B1u, 0x8B265490u, 0x6CB8326Fu, 0, 0x9C906071u, 0x9E453910u, 0x2C56B9AAu, 0x863223EBu,
};
constexpr uint32_t kAttrShake88 = 0xE00D8E2Cu;
constexpr uint32_t kShakeFlagAttrs[4] = {0x7280543Du, 0x0626FA2Cu, 0xB960A308u, 0x3E58C7C1u};

constexpr uint32_t kAttrLength = AnimHash("Length");
constexpr uint32_t kAttrLifetime = AnimHash("lifetime");
constexpr uint32_t kAttrTexture = AnimHash("Texture");
constexpr uint32_t kAttrColor = AnimHash("color");
constexpr uint32_t kAttrBlendMode = AnimHash("blendmode");
constexpr uint32_t kAttrBone1 = AnimHash("Bone1");
constexpr uint32_t kAttrBone2 = AnimHash("Bone2");
constexpr uint32_t kAttrOffset1 = AnimHash("Offset1");
constexpr uint32_t kAttrOffset2 = AnimHash("Offset2");
constexpr uint32_t kAttrPivot1 = AnimHash("Pivot1");
constexpr uint32_t kAttrPivot2 = AnimHash("Pivot2");
constexpr uint32_t kAttrAlphaSpeed = AnimHash("AlphaSpeed");
constexpr uint32_t kAttrFadeTime = AnimHash("FadeTime");
constexpr uint32_t kAttrTrailB4 = 0xC1C61801u;
constexpr uint32_t kAttrObjSpace = AnimHash("ObjSpace");
constexpr uint32_t kAttrTrailC1 = 0xCD7B5B32u;
constexpr uint32_t kAttrTrailC2 = 0x50D7CE24u;
constexpr uint32_t kAttrWrap = AnimHash("Wrap");
constexpr uint32_t kAttrFrameRate = AnimHash("FrameRate");
constexpr uint32_t kAttrTrailC4 = 0x31781726u;
constexpr uint32_t kAttrHint1 = AnimHash("Hint1");
constexpr uint32_t kAttrHint2 = AnimHash("Hint2");

// "blendmode" values of FxTrail (table 0x00AC4B98).
constexpr uint32_t kTrailBlendModes[5] = {
    AnimHash("Opaque"), 0x092B001Cu, AnimHash("Blend"), AnimHash("Additive"), AnimHash("Subtractive"),
};

constexpr uint32_t kAttrGadgetState = AnimHash("GadgetState");
constexpr uint32_t kAttrGadgetFlag = 0xBD75D672u;
constexpr uint32_t kAttrTarget = AnimHash("target");
constexpr uint32_t kAttrAddTarget = AnimHash("AddTarget");
constexpr uint32_t kAttrTargetIndex = 0xDDCC7674u;
constexpr uint32_t kAttrAddTargetIndex = 0x1EB4C7D3u;
constexpr uint32_t kAttrLayer = AnimHash("Layer");
constexpr uint32_t kAttrSpeedVar = AnimHash("speedVar");
constexpr uint32_t kAttrStateVar = AnimHash("StateVar");
constexpr uint32_t kAttrJob = AnimHash("Job");
constexpr uint32_t kAttrJobVar = AnimHash("JobVar");

constexpr Color8 kWhite = {0xFF, 0xFF, 0xFF, 0xFF};  // NOTE: console global 0x01164CA0, assumed white
} // namespace

// ---------------------------------------------------------------------------
// "FxObjShake" (hash 0xF86D2B41; NOTE: name from a hash match): shakes an
// object or entity part, using the camera shake parameters.

class FxObjShakeCommand : public ApeAnimScriptCommand {
public:
    FxObjShakeCommand();
    explicit FxObjShakeCommand(XmlElement* element);
    ~FxObjShakeCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;               // +0x0C "object"
    uint32_t m_entity = 0;               // +0x10 "Entity"
    uint32_t m_child = 0;                // +0x14 "Child"
    uint32_t m_objectInst = 0;           // +0x18 "ObjectInst"
    uint32_t m_durationVar = 0;          // +0x1C "DurationVar"
    float m_field20 = 318958.03f;        // +0x20 NOTE: runtime, initialised to 0x489EBD81
    float m_field24 = 1.0f;              // +0x24 NOTE: runtime
    float m_field28 = 0.0f;              // +0x28 NOTE: runtime
    uint32_t m_part = 0;                 // +0x2C "part"
    float m_duration = 1.0f;             // +0x30 "duration"
    float m_magnitude = 1.0f;            // +0x34 "Magnitude"
    float m_startMagnitude = 0.0f;       // +0x38 "StartMagnitude"
    uint32_t m_falloffMode = 0x85D98325u;// +0x3C "FalloffMode"
    float m_falloffStartTime = 0.0f;     // +0x40 "FalloffStartTime"
    float m_pitch = 1.0f;                // +0x44 "Pitch"
    float m_yaw = 0.0f;                  // +0x48 "Yaw"
    float m_roll = 1.0f;                 // +0x4C "Roll"
    float m_pitchFrequency = 4.0f;       // +0x50 "PitchFrequency"
    float m_yawFrequency = 6.5f;         // +0x54 "YawFrequency"
    float m_rollFrequency = 2.5f;        // +0x58 "RollFrequency"
    float m_xOffset = 0.0f;              // +0x5C "XOffset"
    float m_yOffset = 0.0f;              // +0x60 "YOffset"
    float m_zOffset = 0.0f;              // +0x64 "ZOffset"
    // +0x68..+0x88: further floats; +0x74 is "LineDimension".
    float m_floats[8] = {};
    float m_field88 = 0.0f;              // +0x88 attribute 0xE00D8E2C
    float m_cullDist = -1.0f;            // +0x8C "CullDist"
    bool m_flags[4] = {};                // +0x90..+0x93
};

// @func 0x00078E0C FxObjShakeCommand::FxObjShakeCommand
FxObjShakeCommand::FxObjShakeCommand() = default;

// @func 0x00078F04 FxObjShakeCommand::FxObjShakeCommand
FxObjShakeCommand::FxObjShakeCommand(XmlElement* element)
{
    // The target is the first of Entity, ObjectInst and object given.
    if (element->GetHash(kAttrEntity, &m_entity, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrObjectInst, &m_objectInst, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok) {
        return;
    }
    element->GetHash(kAttrChild, &m_child, m_child);
    element->GetHash(kAttrPart, &m_part, m_part);
    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    element->GetHash(kAttrDurationVar, &m_durationVar, m_durationVar);
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
    element->GetFloat(kAttrXOffset, &m_xOffset, m_xOffset);
    element->GetFloat(kAttrYOffset, &m_yOffset, m_yOffset);
    element->GetFloat(kAttrZOffset, &m_zOffset, m_zOffset);
    for (int i = 0; i < 8; ++i) {
        const uint32_t attr = (i == 3) ? kAttrLineDimension : kShakeFloatAttrs[i];
        element->GetFloat(attr, &m_floats[i], m_floats[i]);
    }
    element->GetFloat(kAttrShake88, &m_field88, m_field88);
    element->GetFloat(kAttrCullDist, &m_cullDist, m_cullDist);
    for (int i = 0; i < 4; ++i)
        element->GetBool(kShakeFlagAttrs[i], &m_flags[i], m_flags[i]);
}

// @func 0x00079494 FxObjShakeCommand::~FxObjShakeCommand
// @func 0x000794C4 FxObjShakeCommand::~FxObjShakeCommand (deleting)
FxObjShakeCommand::~FxObjShakeCommand() = default;

// @func 0x0007951C FxObjShakeCommand::Create
ApeAnimScriptCommand* FxObjShakeCommand::Create(XmlElement* element) const
{
    return new FxObjShakeCommand(element);
}

// TODO(0x0007958C FxObjShakeCommand::Execute)

// ---------------------------------------------------------------------------
// FxTrail: a ribbon between two bones of an object.

class FxTrailCommand : public ApeAnimScriptCommand {
public:
    FxTrailCommand();
    explicit FxTrailCommand(XmlElement* element);
    ~FxTrailCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;                              // +0x0C "object"
    uint32_t m_part = 0;                                // +0x10 "part"
    uint32_t m_field20 = 0xEB2F2138u;                   // +0x20 NOTE: hash, never read from data
    float m_field24 = 1.0f;                             // +0x24 NOTE: never read from data
    float m_field28 = 0.0f;                             // +0x28 NOTE: never read from data
    float m_length = 0.5f;                              // +0x2C "Length"
    float m_lifetime = 1.0f;                            // +0x30 "lifetime"
    uint32_t m_texture = 0;                             // +0x34 "Texture"
    float m_wrap = 1.0f;                                // +0x38 "Wrap"
    Color8 m_color = kWhite;                            // +0x3C "color"
    uint32_t m_blendMode = 2;                           // +0x40 "blendmode" (index into kTrailBlendModes)
    uint32_t m_bone1 = 0;                               // +0x44 "Bone1"
    uint32_t m_bone2 = 0;                               // +0x48 "Bone2"
    ape::Vec3 m_offset1 = {0.0f, 0.0f, 0.0f, 0.0f};     // +0x50 "Offset1"
    ape::Vec3 m_offset2 = {0.0f, 0.0f, 0.0f, 0.0f};     // +0x60 "Offset2"
    ape::Vec3 m_pivot1 = {0.0f, 0.0f, 0.0f, 0.0f};      // +0x70 "Pivot1"
    ape::Vec3 m_pivot2 = {0.0f, 0.0f, 0.0f, 0.0f};      // +0x80 "Pivot2"
    ape::Vec3 m_hint1 = {0.0f, 0.0f, 0.0f, 0.0f};       // +0x90 "Hint1"
    ape::Vec3 m_hint2 = {0.0f, 0.0f, 0.0f, 0.0f};       // +0xA0 "Hint2"
    float m_alphaSpeed = 75.0f;                         // +0xB0 "AlphaSpeed"
    float m_fieldB4 = -1.0f;                            // +0xB4 attribute 0xC1C61801
    float m_fadeTime = 0.05f;                           // +0xB8 "FadeTime"
    float m_frameRate = -1.0f;                          // +0xBC "FrameRate"
    bool m_objSpace = false;                            // +0xC0 "ObjSpace"
    bool m_flagC1 = false;                              // +0xC1 attribute 0xCD7B5B32
    bool m_flagC2 = false;                              // +0xC2 attribute 0x50D7CE24
    int32_t m_fieldC4 = 1;                              // +0xC4 attribute 0x31781726
};

// @func 0x000797A4 FxTrailCommand::FxTrailCommand
FxTrailCommand::FxTrailCommand() = default;

// @func 0x00079938 FxTrailCommand::FxTrailCommand
FxTrailCommand::FxTrailCommand(XmlElement* element)
{
    if (element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok)
        return;
    element->GetHash(kAttrPart, &m_part, m_part);
    element->GetFloat(kAttrLength, &m_length, m_length);
    element->GetFloat(kAttrLifetime, &m_lifetime, m_lifetime);
    element->GetHash(kAttrTexture, &m_texture, m_texture);
    element->GetColor(kAttrColor, &m_color, &m_color);
    element->GetEnum(kAttrBlendMode, &m_blendMode, kTrailBlendModes, 5, m_blendMode);
    element->GetHash(kAttrBone1, &m_bone1, m_bone1);
    element->GetHash(kAttrBone2, &m_bone2, m_bone2);
    element->GetVec3(kAttrOffset1, &m_offset1.x, &m_offset1.x);
    element->GetVec3(kAttrOffset2, &m_offset2.x, &m_offset2.x);
    element->GetVec3(kAttrPivot1, &m_pivot1.x, &m_pivot1.x);
    element->GetVec3(kAttrPivot2, &m_pivot2.x, &m_pivot2.x);
    element->GetFloat(kAttrAlphaSpeed, &m_alphaSpeed, m_alphaSpeed);
    element->GetFloat(kAttrFadeTime, &m_fadeTime, m_fadeTime);
    element->GetFloat(kAttrTrailB4, &m_fieldB4, m_fieldB4);
    element->GetBool(kAttrObjSpace, &m_objSpace, m_objSpace);
    element->GetBool(kAttrTrailC1, &m_flagC1, m_flagC1);
    element->GetBool(kAttrTrailC2, &m_flagC2, m_flagC2);
    element->GetFloat(kAttrWrap, &m_wrap, m_wrap);
    element->GetFloat(kAttrFrameRate, &m_frameRate, m_frameRate);
    element->GetInt(kAttrTrailC4, &m_fieldC4, m_fieldC4);
    element->GetVec3(kAttrHint1, &m_hint1.x, &m_hint1.x);
    element->GetVec3(kAttrHint2, &m_hint2.x, &m_hint2.x);

    if (m_flagC1 && m_lifetime < 0.0f) {
        // NOTE: the compiled data carries no line numbers; the console prints
        // the element row + 1, which is 0 for compiled documents.
        ape::TempFormat("%s(%ld): FX trail with infinite life and no auto-detach on object "
                        "invisibility - probable memory leak.",
                        element->GetDocument() ? element->GetDocument()->GetDocumentPath() : "", 0L);
    }
}

// @func 0x00079E84 FxTrailCommand::~FxTrailCommand
// @func 0x00079EB4 FxTrailCommand::~FxTrailCommand (deleting)
FxTrailCommand::~FxTrailCommand() = default;

// @func 0x00079F0C FxTrailCommand::Create
ApeAnimScriptCommand* FxTrailCommand::Create(XmlElement* element) const
{
    return new FxTrailCommand(element);
}

// TODO(0x00079F7C FxTrailCommand::Execute)

// ---------------------------------------------------------------------------
// Gadget: sets the gadget state of an entity.

class GadgetCommand : public ApeAnimScriptCommand {
public:
    GadgetCommand();
    ~GadgetCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;       // +0x0C "object"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_gadgetState = 0;  // +0x14 "GadgetState"
    // +0x18 bit 31 / bit 30: attribute 0xBD75D672 given / its value.
    bool m_hasFlag = false;
    bool m_flag = false;
};

// @func 0x0007A084 GadgetCommand::GadgetCommand
GadgetCommand::GadgetCommand() = default;

// @func 0x0073CEF0 GadgetCommand::~GadgetCommand
// @func 0x0073CF20 GadgetCommand::~GadgetCommand (deleting)
GadgetCommand::~GadgetCommand() = default;

// @func 0x0007A0E4 GadgetCommand::Create
ApeAnimScriptCommand* GadgetCommand::Create(XmlElement* element) const
{
    auto* command = new GadgetCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrGadgetState, &command->m_gadgetState, command->m_gadgetState);
    bool value = false;
    command->m_hasFlag = element->GetBool(kAttrGadgetFlag, &value, command->m_flag) == ape::xml::kQuery_Ok;
    command->m_flag = value;
    return command;
}

// @func 0x0007A240 GadgetCommand::Execute
void GadgetCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // NOTE: unlike most commands, a given "Entity" overrides the object variable.
    WorldObject* entity = m_object != 0 ? EntityFromVar(instance, m_object) : nullptr;
    if (m_entity != 0)
        entity = FindEntity(m_entity);
    if (!entity || !EntityIsGadgetObject(entity))
        return;
    if (m_gadgetState != 0)
        GadgetObjectSetState(entity, m_gadgetState);
    if (m_hasFlag)
        GadgetObjectSetFlag(entity, m_flag);
}

// ---------------------------------------------------------------------------
// GadgetTurret: sets or adds a turret target.

class GadgetTurretCommand : public ApeAnimScriptCommand {
public:
    GadgetTurretCommand();
    ~GadgetTurretCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;      // +0x0C "Entity"
    uint32_t m_target = 0;      // +0x10 "target" or "AddTarget"
    int32_t m_targetIndex = 9;  // +0x14 attribute 0xDDCC7674 or 0x1EB4C7D3
    bool m_add = false;         // +0x18 true for the "Add..." forms
};

// @func 0x0007A38C GadgetTurretCommand::GadgetTurretCommand
GadgetTurretCommand::GadgetTurretCommand() = default;

// @func 0x0073CF78 GadgetTurretCommand::~GadgetTurretCommand
// @func 0x0073CFA8 GadgetTurretCommand::~GadgetTurretCommand (deleting)
GadgetTurretCommand::~GadgetTurretCommand() = default;

// @func 0x0007A3EC GadgetTurretCommand::Create
ApeAnimScriptCommand* GadgetTurretCommand::Create(XmlElement* element) const
{
    auto* command = new GadgetTurretCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    if (element->GetHash(kAttrTarget, &command->m_target, command->m_target) == ape::xml::kQuery_Ok)
        command->m_add = false;
    else if (element->GetHash(kAttrAddTarget, &command->m_target, command->m_target) == ape::xml::kQuery_Ok)
        command->m_add = true;

    int32_t index = 0;
    if (element->GetInt(kAttrTargetIndex, &index, command->m_targetIndex) == ape::xml::kQuery_Ok) {
        command->m_add = false;
        command->m_targetIndex = index;
    } else if (element->GetInt(kAttrAddTargetIndex, &index, command->m_targetIndex) == ape::xml::kQuery_Ok) {
        command->m_add = true;
        command->m_targetIndex = index;
    }
    return command;
}

// @func 0x0007A58C GadgetTurretCommand::Execute
// Adds or removes a target of a gadget turret: a named entity, or the
// player with the given index (9 = none).
void GadgetTurretCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* turret = nullptr;
    if (m_entity != 0) {
        WorldObject* entity = FindEntity(m_entity);
        if (entity && EntityIsGadgetTurret(entity))
            turret = entity;
    }

    WorldObject* target = nullptr;
    WorldObject* candidate = nullptr;
    if (m_target != 0)
        candidate = FindEntity(m_target);
    else if (m_targetIndex != kNoPlayerIndex)
        candidate = PlayerEntityByIndex(m_targetIndex);
    if (candidate && EntityHasClassFlag(candidate, kEntityClass_Targetable))
        target = candidate;

    if (!turret)
        return;
    if (m_add)
        TurretAddTarget(turret, target);
    else
        TurretRemoveTarget(turret, target);
}

// ---------------------------------------------------------------------------
// "GetAnimSpeed" (hash 0x55C11A40; NOTE: name from a hash match)

class GetAnimSpeedCommand : public ApeAnimScriptCommand {
public:
    GetAnimSpeedCommand();
    ~GetAnimSpeedCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    uint32_t m_layer = 0;     // +0x14 "Layer"
    uint32_t m_speedVar = 0;  // +0x18 "speedVar": float variable receiving the speed
};

// @func 0x0007A6E8 GetAnimSpeedCommand::GetAnimSpeedCommand
GetAnimSpeedCommand::GetAnimSpeedCommand() = default;

// @func 0x0007A740 GetAnimSpeedCommand::~GetAnimSpeedCommand
// @func 0x0007A770 GetAnimSpeedCommand::~GetAnimSpeedCommand (deleting)
GetAnimSpeedCommand::~GetAnimSpeedCommand() = default;

// @func 0x0007A7C8 GetAnimSpeedCommand::Create
ApeAnimScriptCommand* GetAnimSpeedCommand::Create(XmlElement* element) const
{
    auto* command = new GetAnimSpeedCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrEntity, &command->m_entity, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrLayer, &command->m_layer, 0);
        element->GetHash(kAttrSpeedVar, &command->m_speedVar, 0);
    }
    return command;
}

// @func 0x0007A8F4 GetAnimSpeedCommand::Execute
// Reads the playback speed of an animation layer into a float variable. The
// controller is the character's when the object belongs to one, else the one
// recorded for the object variable.
void GetAnimSpeedCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_speedVar == 0)
        return;

    WorldObject* entity = nullptr;
    if (m_object != 0) {
        ape::ApeRefCounted* object = instance->ResolveObject(m_object, false);
        if (!object)
            object = instance->ResolveObject(m_object, true);
        if (!object)
            return;
        entity = EntityFromObject(object);
    } else if (m_entity != 0) {
        entity = FindEntity(m_entity);
    }

    void* controller = nullptr;
    if (entity) {
        if (void* character = EntityCharacter(entity))
            controller = CharacterAnimController(character);
    }
    if (!controller)
        controller = ScriptObjectAnimController(instance, m_object);
    if (!controller)
        return;

    void* anim = AnimControllerFindLayer(controller, static_cast<int32_t>(m_layer));
    if (!anim)
        return;
    ApeAnimScriptVar* var = instance->FindVar(m_speedVar);
    if (var && var->GetType() == AnimScriptVarType::Float)
        static_cast<ape::ApeAnimScriptFloatVar*>(var)->SetFloat(AnimSpeed(anim));
}

// ---------------------------------------------------------------------------
// "GetBinocucomVisible" (hash 0x599B5141; NOTE: name from a hash match)

class GetBinocucomVisibleCommand : public ApeAnimScriptCommand {
public:
    GetBinocucomVisibleCommand();
    ~GetBinocucomVisibleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_stateVar = 0;  // +0x0C "StateVar"
};

// @func 0x0007AAA8 GetBinocucomVisibleCommand::GetBinocucomVisibleCommand
GetBinocucomVisibleCommand::GetBinocucomVisibleCommand() = default;

// @func 0x0007AAF0 GetBinocucomVisibleCommand::~GetBinocucomVisibleCommand
// @func 0x0007AB20 GetBinocucomVisibleCommand::~GetBinocucomVisibleCommand (deleting)
GetBinocucomVisibleCommand::~GetBinocucomVisibleCommand() = default;

// @func 0x0007AB80 GetBinocucomVisibleCommand::Create
ApeAnimScriptCommand* GetBinocucomVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new GetBinocucomVisibleCommand();
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    return command;
}

// @func 0x0007AC24 GetBinocucomVisibleCommand::Execute
void GetBinocucomVisibleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_stateVar == 0)
        return;
    bool visible = false;
    if (WorldObject* player = PlayerEntity()) {
        if (void* binocucom = PlayerBinocucom(player))
            visible = BinocucomIsVisible(binocucom);
    }
    ApeAnimScriptVar* var = instance->FindVar(m_stateVar);
    if (var && var->GetType() == AnimScriptVarType::Int)
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(visible ? 1 : 0);
}

// ---------------------------------------------------------------------------
// "GetJobState" (hash 0x9929642A; NOTE: name from a hash match)

class GetJobStateCommand : public ApeAnimScriptCommand {
public:
    GetJobStateCommand();
    ~GetJobStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_job = 0;       // +0x0C "Job"
    uint32_t m_stateVar = 0;  // +0x10 "StateVar"
    uint32_t m_jobVar = 0;    // +0x14 "JobVar"
};

// @func 0x0007B420 GetJobStateCommand::GetJobStateCommand
GetJobStateCommand::GetJobStateCommand() = default;

// @func 0x0007B470 GetJobStateCommand::~GetJobStateCommand
// @func 0x0007B4A0 GetJobStateCommand::~GetJobStateCommand (deleting)
GetJobStateCommand::~GetJobStateCommand() = default;

// @func 0x0007B4F8 GetJobStateCommand::Create
ApeAnimScriptCommand* GetJobStateCommand::Create(XmlElement* element) const
{
    auto* command = new GetJobStateCommand();
    element->GetHash(kAttrJob, &command->m_job, 0);
    element->GetHash(kAttrStateVar, &command->m_stateVar, 0);
    element->GetHash(kAttrJobVar, &command->m_jobVar, 0);
    return command;
}

// Job states as hashes (table 0x00AC4D74, indexed by 0x002D98A4).
// NOTE: case of the names unknown.
static constexpr uint32_t kJobStateNames[3] = {
    AnimHash("unavailable"), AnimHash("available"), AnimHash("complete"),
};

// @func 0x0007B5E8 GetJobStateCommand::Execute
// "JobVar" receives the current job's name, "StateVar" the state of "Job".
void GetJobStateCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_stateVar == 0 && m_jobVar == 0)
        return;
    if (m_jobVar != 0 && HasCurrentJob()) {
        ApeAnimScriptVar* var = instance->FindVar(m_jobVar);
        if (var && var->GetType() == AnimScriptVarType::Hash)
            static_cast<ape::ApeAnimScriptHashVar*>(var)->SetHash(CurrentJobName());
    }
    if (m_job != 0 && m_stateVar != 0) {
        int state = 0;
        if (void* job = GameStateFindJob(m_job))
            state = JobState(job);
        ApeAnimScriptVar* var = instance->FindVar(m_stateVar);
        if (var && var->GetType() == AnimScriptVarType::Hash)
            static_cast<ape::ApeAnimScriptHashVar*>(var)->SetHash(kJobStateNames[state]);
    }
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsG()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("FxObjShake"), new FxObjShakeCommand());
    prototypes.Add(AnimHash("FxTrail"), new FxTrailCommand());
    prototypes.Add(AnimHash("Gadget"), new GadgetCommand());
    prototypes.Add(AnimHash("GadgetTurret"), new GadgetTurretCommand());
    prototypes.Add(AnimHash("GetAnimSpeed"), new GetAnimSpeedCommand());
    prototypes.Add(AnimHash("GetBinocucomVisible"), new GetBinocucomVisibleCommand());
    prototypes.Add(AnimHash("GetJobState"), new GetJobStateCommand());
}

} // namespace levels
} // namespace sly
