// openTIT - Game script commands, fourteenth block (spline follower commands to the MsDecibel range command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrSpline = AnimHash("spline");
constexpr uint32_t kAttrSpeed = AnimHash("Speed");
constexpr uint32_t kAttrDoTarget = AnimHash("DoTarget");
constexpr uint32_t kAttrAxis = AnimHash("Axis");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrFlag = AnimHash("flag");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrEnable11 = 0xEEA6E96Fu;  // NOTE: name unknown; picks the other entity call
constexpr uint32_t kAttrDecibel0 = 0x8CFF50C1u;  // NOTE: name unknown (ends in "1")
constexpr uint32_t kAttrDecibel1 = 0x8CFF50C2u;  // NOTE: name unknown (ends in "2")
constexpr uint32_t kEntityMsDecibel = AnimHash("MsDecibel");

constexpr int kComponentKind_SplineFollower = 0x22;  // NOTE: kind 0x22 components follow splines

// Flag names of SetEntityFlag and their 64-bit masks on entity +0xB8
// (console table 0x00AC51A0).
struct EntityFlagMask {
    uint32_t nameHash;
    uint64_t mask;
};
constexpr EntityFlagMask kEntityFlagMasks[] = {
    {0x9EB99829u, 0x0000000000000100ull},
    {0xA8C3D7DAu, 0x0000000000010000ull},
    {0x29140264u, 0x0000000002000000ull},
    {0x0C9EDD8Au, 0x0000000008000000ull},
    {0x4F187A1Fu, 0x0000000040000000ull},
    {AnimHash("PhotoTarget"), 0x0000000400000000ull},
    {0x76E96977u, 0x0000000800000000ull},
};

// Spline follower component of the entity held by an object of the instance
// (looked up by name among the instance's objects only).
void* SplineFollowerOf(ApeAnimScriptInstance* instance, uint32_t objectName)
{
    WorldObject* entity = EntityFromObject(instance->ResolveObject(objectName, false));
    return entity ? EntityGetComponent(entity, kComponentKind_SplineFollower) : nullptr;
}
} // namespace

// ---------------------------------------------------------------------------
// Hash 0xC5EC47B7 (name unknown): puts a spline follower on a spline.

class SplineFollowSetSplineCommand : public ApeAnimScriptCommand {
public:
    SplineFollowSetSplineCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_spline = 0;  // +0x10 "spline"
};

// @func 0x00089D98 SplineFollowSetSplineCommand::SplineFollowSetSplineCommand
SplineFollowSetSplineCommand::SplineFollowSetSplineCommand() = default;

// @func 0x00089DE0 SplineFollowSetSplineCommand::Create
ApeAnimScriptCommand* SplineFollowSetSplineCommand::Create(XmlElement* element) const
{
    auto* command = new SplineFollowSetSplineCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrSpline, &command->m_spline, command->m_spline);
    return command;
}

// @func 0x00089EB0 SplineFollowSetSplineCommand::Execute
void SplineFollowSetSplineCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (void* follower = SplineFollowerOf(instance, m_object))
        SplineFollowerSetSpline(follower, m_spline);
}

// ---------------------------------------------------------------------------
// Hash 0x72A5FAD0 (name unknown): sets a spline follower's speed.

class SplineFollowSetSpeedCommand : public ApeAnimScriptCommand {
public:
    SplineFollowSetSpeedCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    float m_speed = 0.0f;   // +0x10 "Speed"
};

// @func 0x00089F3C SplineFollowSetSpeedCommand::SplineFollowSetSpeedCommand
SplineFollowSetSpeedCommand::SplineFollowSetSpeedCommand() = default;

// @func 0x00089F8C SplineFollowSetSpeedCommand::Create
ApeAnimScriptCommand* SplineFollowSetSpeedCommand::Create(XmlElement* element) const
{
    auto* command = new SplineFollowSetSpeedCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetFloat(kAttrSpeed, &command->m_speed, command->m_speed);
    return command;
}

// @func 0x0008A058 SplineFollowSetSpeedCommand::Execute
void SplineFollowSetSpeedCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (void* follower = SplineFollowerOf(instance, m_object))
        SplineFollowerSetSpeed(follower, m_speed);
}

// ---------------------------------------------------------------------------
// Hash 0xC6D07177 (name unknown): sets the facing axis/part of a spline follower.

class SplineFollowSetAxisCommand : public ApeAnimScriptCommand {
public:
    SplineFollowSetAxisCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    uint32_t m_axis = 0x5A;    // +0x10 "Axis" (default 'Z')
    uint32_t m_part = 0;       // +0x14 "part"
    bool m_doTarget = false;   // +0x18 "DoTarget"
};

// @func 0x0008A0E0 SplineFollowSetAxisCommand::SplineFollowSetAxisCommand
SplineFollowSetAxisCommand::SplineFollowSetAxisCommand() = default;

// @func 0x0008A140 SplineFollowSetAxisCommand::Create
ApeAnimScriptCommand* SplineFollowSetAxisCommand::Create(XmlElement* element) const
{
    auto* command = new SplineFollowSetAxisCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    bool doTarget = false;
    element->GetBool(kAttrDoTarget, &doTarget, false);
    element->GetHash(kAttrAxis, &command->m_axis, command->m_axis);
    element->GetHash(kAttrPart, &command->m_part, command->m_part);
    command->m_doTarget = doTarget;
    return command;
}

// @func 0x0008A260 SplineFollowSetAxisCommand::Execute
void SplineFollowSetAxisCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (void* follower = SplineFollowerOf(instance, m_object))
        SplineFollowerSetAxis(follower, m_axis, m_part, m_doTarget);
}

// ---------------------------------------------------------------------------
// Hash 0xAFAF3B97 (name unknown): detaches a spline follower from its spline
// and gives it a value.

class SplineFollowDetachCommand : public ApeAnimScriptCommand {
public:
    SplineFollowDetachCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    float m_value = 1.0f;   // +0x10 NOTE: the console reads it from the "object" attribute too
};

// @func 0x00089BCC SplineFollowDetachCommand::SplineFollowDetachCommand
SplineFollowDetachCommand::SplineFollowDetachCommand() = default;

// @func 0x00089C1C SplineFollowDetachCommand::Create
ApeAnimScriptCommand* SplineFollowDetachCommand::Create(XmlElement* element) const
{
    auto* command = new SplineFollowDetachCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    // NOTE: same attribute name as above in the original data layout.
    element->GetFloat(kAttrObject, &command->m_value, command->m_value);
    return command;
}

// @func 0x00089CEC SplineFollowDetachCommand::Execute
void SplineFollowDetachCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (void* follower = SplineFollowerOf(instance, m_object)) {
        SplineFollowerSetSpline(follower, 0);
        SplineFollowerSetFreeMotion(follower, m_value, 100.0f);
    }
}

// ---------------------------------------------------------------------------
// "SetEntityFlag" (class name from the warning text; registered hash
// 0x06F47B91): sets or clears a 64-bit entity flag by name.

class SetEntityFlagCommand : public ApeAnimScriptCommand {
public:
    SetEntityFlagCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    uint64_t m_mask = 0;    // +0x18 from "flag"
    bool m_enable = false;  // +0x20 "enable"
};

// @func 0x0008A30C SetEntityFlagCommand::SetEntityFlagCommand
SetEntityFlagCommand::SetEntityFlagCommand() = default;

// @func 0x0008A36C SetEntityFlagCommand::Create
ApeAnimScriptCommand* SetEntityFlagCommand::Create(XmlElement* element) const
{
    auto* command = new SetEntityFlagCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    uint32_t flag = 0;
    if (element->GetHash(kAttrFlag, &flag, 0) == ape::xml::kQuery_Ok) {
        for (const EntityFlagMask& entry : kEntityFlagMasks) {
            if (entry.nameHash == flag) {
                command->m_mask = entry.mask;
                break;
            }
        }
        if (command->m_mask == 0)
            ape::TempFormat("AnimScriptCommandSetEntityFlag: Unrecognized flag specifier '%s'.",
                            ape::TempHexString(flag));
    }
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// @func 0x0008A500 SetEntityFlagCommand::Execute
void SetEntityFlagCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (m_mask == 0)
        return;
    WorldObject* entity = nullptr;
    if (m_object != 0)
        entity = EntityFromObject(instance->ResolveObject(m_object, false));
    if (!entity && m_entity != 0)
        entity = FindEntity(m_entity);
    if (entity)
        EntitySetFlags64(entity, m_mask, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0xAAECF6DB (name unknown): switches an entity on or off through one of
// two entity calls.

class EntitySwitchCommand : public ApeAnimScriptCommand {
public:
    EntitySwitchCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object" (entity name or object)
    bool m_enable = false;     // +0x10 "enable"
    bool m_altCall = false;    // +0x11 attribute 0xEEA6E96F
};

// @func 0x0008A5D0 EntitySwitchCommand::EntitySwitchCommand
EntitySwitchCommand::EntitySwitchCommand() = default;

// @func 0x0008A628 EntitySwitchCommand::Create
ApeAnimScriptCommand* EntitySwitchCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySwitchCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    element->GetBool(kAttrEnable11, &command->m_altCall, command->m_altCall);
    return command;
}

// @func 0x0008A71C EntitySwitchCommand::Execute
void EntitySwitchCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // The live entity of that name first, then an object of the instance.
    WorldObject* entity = FindEntity(m_object);
    if (!entity)
        entity = EntityFromObject(instance->ResolveObject(m_object, false));
    if (!entity)
        return;
    if (m_altCall)
        EntityVirtualSwitch44(entity, m_enable);
    else
        EntityVirtualSwitch54(entity, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0x7A1B655B (name unknown): resolves an entity and does nothing else in
// the release build (the remaining code only formats warnings).

class EntityCheckCommand : public ApeAnimScriptCommand {
public:
    EntityCheckCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
};

// @func 0x0008B508 EntityCheckCommand::EntityCheckCommand
EntityCheckCommand::EntityCheckCommand() = default;

// @func 0x0008B558 EntityCheckCommand::Create
ApeAnimScriptCommand* EntityCheckCommand::Create(XmlElement* element) const
{
    auto* command = new EntityCheckCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    return command;
}

// @func 0x0008B624 EntityCheckCommand::Execute
void EntityCheckCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    // An alive entity of class 0x40 is fine; anything else would only warn.
    (void)entity;
}

// ---------------------------------------------------------------------------
// Hash 0xA560F585 (name unknown): gives the "MsDecibel" boss two named points.

class MsDecibelRangeCommand : public ApeAnimScriptCommand {
public:
    MsDecibelRangeCommand();
    ~MsDecibelRangeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_point0 = 0;  // +0x0C attribute 0x8CFF50C1
    uint32_t m_point1 = 0;  // +0x10 attribute 0x8CFF50C2
};

// @func 0x0008B768 MsDecibelRangeCommand::MsDecibelRangeCommand
MsDecibelRangeCommand::MsDecibelRangeCommand() = default;

// @func 0x0008B7B0 MsDecibelRangeCommand::~MsDecibelRangeCommand
// @func 0x0008B7E0 MsDecibelRangeCommand::~MsDecibelRangeCommand (deleting)
MsDecibelRangeCommand::~MsDecibelRangeCommand() = default;

// @func 0x0008B838 MsDecibelRangeCommand::Create
ApeAnimScriptCommand* MsDecibelRangeCommand::Create(XmlElement* element) const
{
    auto* command = new MsDecibelRangeCommand();
    element->GetHash(kAttrDecibel0, &command->m_point0, 0);
    element->GetHash(kAttrDecibel1, &command->m_point1, 0);
    return command;
}

// @func 0x0008B908 MsDecibelRangeCommand::Execute
void MsDecibelRangeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* boss = FindEntity(kEntityMsDecibel);
    if (!boss)
        return;
    if (void* brain = EntityBrain(boss))
        MsDecibelSetRange(brain, m_point0, m_point1);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsP()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0xC5EC47B7u, new SplineFollowSetSplineCommand());
    prototypes.Add(0x72A5FAD0u, new SplineFollowSetSpeedCommand());
    prototypes.Add(0xC6D07177u, new SplineFollowSetAxisCommand());
    prototypes.Add(0xAFAF3B97u, new SplineFollowDetachCommand());
    prototypes.Add(AnimHash("SetEntityFlag"), new SetEntityFlagCommand());
    prototypes.Add(0xAAECF6DBu, new EntitySwitchCommand());
    prototypes.Add(0x7A1B655Bu, new EntityCheckCommand());
    prototypes.Add(0xA560F585u, new MsDecibelRangeCommand());
}

} // namespace levels
} // namespace sly
