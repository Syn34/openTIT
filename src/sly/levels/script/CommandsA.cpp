// openTIT - Game script commands, first alphabetical block (Activate... to Attach...).
#include "sly/levels/script/CommandsA.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/game/AudioSystem.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ScriptServices.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrObject = AnimHash("Object");
constexpr uint32_t kAttrActivate = AnimHash("activate");
constexpr uint32_t kAttrAllowCancel = AnimHash("AllowCancel");
constexpr uint32_t kAttrForcePlayer = AnimHash("ForcePlayer");
constexpr uint32_t kAttrMixState = AnimHash("MixState");
constexpr uint32_t kAttrCrossfadeDuration = AnimHash("CrossfadeDuration");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrMaterial = AnimHash("Material");
constexpr uint32_t kAttrDurationVar = AnimHash("DurationVar");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrFrom = AnimHash("From");
constexpr uint32_t kAttrTo = AnimHash("To");
constexpr uint32_t kAttrAnimLayer = AnimHash("AnimLayer");
constexpr uint32_t kAttrAnimTimeVar = AnimHash("AnimTimeVar");
constexpr uint32_t kAttrAnimTime = AnimHash("AnimTime");
constexpr uint32_t kAttrScaleByLength = 0x2FCB2115u;  // NOTE: name unknown
constexpr uint32_t kAttrAttachOffset = AnimHash("AttachOffset");
constexpr uint32_t kAttrAttachRot = AnimHash("AttachRot");
constexpr uint32_t kAttrAttachPart = AnimHash("AttachPart");
constexpr uint32_t kAttrAttacheeVar = AnimHash("AttacheeVar");
constexpr uint32_t kAttrAttacheeEntity = AnimHash("AttacheeEntity");
constexpr uint32_t kAttrTargetVar = 0x5538A9B9u;     // NOTE: name unknown
constexpr uint32_t kAttrTargetEntity = 0x30A5A3F1u;  // NOTE: name unknown
constexpr uint32_t kAttrIgnoreRot = AnimHash("IgnoreRot");
constexpr uint32_t kAttrParent = AnimHash("Parent");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrParentPart = 0x20C00073u;
constexpr uint32_t kAttrParentOffset = 0xEF4DDD13u;
constexpr uint32_t kAttrParentRotation = 0xC7400937u;
constexpr uint32_t kAttrChildPart = 0xF91B96E2u;
constexpr uint32_t kAttrChildOffset = 0x1F1D93C2u;
constexpr uint32_t kAttrChildRotation = 0xA4F068E8u;
constexpr uint32_t kAttrGravity = AnimHash("Gravity");
constexpr uint32_t kAttrSyncAnimation = AnimHash("SyncAnimation");
constexpr uint32_t kAttrAttachFlag75 = 0x3E3F7E94u;
constexpr uint32_t kAttrAttachFlag76 = 0xD1A6825Eu;
constexpr uint32_t kAttrAttachFlag77 = 0xDC623C3Eu;
constexpr uint32_t kAttrAttachFlag79 = 0x3B757A97u;
constexpr uint32_t kAttrOffsetPos = AnimHash("OffsetPos");
constexpr uint32_t kAttrBlastDirection = 0x8C3BE0FFu;
constexpr uint32_t kAttrDamage = AnimHash("damage");
constexpr uint32_t kAttrImpulse = AnimHash("Impulse");
constexpr uint32_t kAttrRadiusMin = AnimHash("RadiusMin");
constexpr uint32_t kAttrRadiusMax = AnimHash("RadiusMax");
constexpr uint32_t kAttrIgnoreHeight = AnimHash("IgnoreHeight");
constexpr uint32_t kAttrReactType = AnimHash("ReactType");
constexpr uint32_t kAttrBlastField4C = 0xC831C860u;
constexpr uint32_t kElemDamageTypes = AnimHash("DamageTypes");
constexpr uint32_t kAttrAnim = AnimHash("Anim");
constexpr uint32_t kAttrSpeed = AnimHash("speed");
constexpr uint32_t kAttrLoop = AnimHash("Loop");
constexpr uint32_t kAttrCameraAnimFlag = 0x855E65C3u;
constexpr uint32_t kAttrRadius = AnimHash("Radius");
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
constexpr uint32_t kAttrRemoveAnimation = AnimHash("RemoveAnimation");
constexpr uint32_t kAttrGoal = AnimHash("Goal");
constexpr uint32_t kAttrGoalVar = AnimHash("GoalVar");
constexpr uint32_t kAttrCommand = AnimHash("Command");
constexpr uint32_t kAttrDir = AnimHash("dir");
constexpr uint32_t kAttrDirection = AnimHash("direction");
constexpr uint32_t kAttrAngle = AnimHash("Angle");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrSkipAnim = AnimHash("SkipAnim");
constexpr uint32_t kAttrCrossfadeFlag23B = 0x16C2F884u;
constexpr uint32_t kAttrCrossfadeFlag23C = 0xF9D05ED8u;
constexpr uint32_t kAttrCrossfadeFlag23D = 0xC1DF2672u;
constexpr uint32_t kAttrDefaultMat = AnimHash("DefaultMat");
constexpr uint32_t kAttrCrossfade230 = 0x11CCCBF1u;
constexpr uint32_t kAttrFromAlpha = AnimHash("FromAlpha");
constexpr uint32_t kAttrToAlpha = AnimHash("ToAlpha");
constexpr uint32_t kElemEntityChild = AnimHash("EntityChild");
constexpr uint32_t kAttrName = AnimHash("Name");
constexpr uint32_t kAttrType = AnimHash("Type");
constexpr uint32_t kAttrKnockbackImpulse = AnimHash("KnockbackImpulse");

// Default direction of CharacterSpecialCommand (console global 0x01164B70).
// NOTE: a world constant filled at start-up; assumed to be the unit Z axis.
const ape::Vec3 kDefaultDirection = {0.0f, 0.0f, 1.0f, 0.0f};

// Damage type names of the "DamageTypes" child of Blast, in bit order
// (table 0x00AC66CC). Unresolved names are kept as hashes.
constexpr uint32_t kBlastDamageTypes[13] = {
    AnimHash("SuperSpring"), 0xCB378DD0u, 0xB4AC34A8u, AnimHash("PushBlock"), 0xF66BC199u,
    AnimHash("CrackShot"), AnimHash("Bomb"), AnimHash("laser"), 0x001FE77Fu,
    AnimHash("ReflectProj"), AnimHash("RatchetWrench"), AnimHash("ColeAmp"), AnimHash("ThrowImpact"),
};

void ScaleVec3(ape::Vec3& v, float s)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
}

constexpr float kDegToRad = 0.017453292f;
constexpr float kMinDuration = 1.0e-5f;
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};
} // namespace

// ---------------------------------------------------------------------------
// #1 (0x36DF0092): switch an entity's interaction on or off

// @func 0x0006B6E0 ActivateInteractionCommand::ActivateInteractionCommand
ActivateInteractionCommand::ActivateInteractionCommand() = default;

ActivateInteractionCommand::ActivateInteractionCommand(XmlElement* element)
{
    // "activate" is only read when the command names its entity.
    if (element->GetHash(kAttrEntity, &m_entity, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrRefObject, &m_refObject, 0) == ape::xml::kQuery_Ok) {
        element->GetBool(kAttrActivate, &m_activate, true);
    }
    element->GetBool(kAttrAllowCancel, &m_allowCancel, m_allowCancel);
    element->GetBool(kAttrForcePlayer, &m_forcePlayer, m_forcePlayer);
}

// @func 0x0006B738 ActivateInteractionCommand::~ActivateInteractionCommand
// @func 0x0006B768 ActivateInteractionCommand::~ActivateInteractionCommand (deleting)
ActivateInteractionCommand::~ActivateInteractionCommand() = default;

// @func 0x0006B7C0 ActivateInteractionCommand::Create
ApeAnimScriptCommand* ActivateInteractionCommand::Create(XmlElement* element) const
{
    return new ActivateInteractionCommand(element);
}

// @func 0x0006B918 ActivateInteractionCommand::Execute
void ActivateInteractionCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_refObject, m_entity);
    if (!entity)
        return;
    void* interaction = EntityInteractionComponent(entity);
    if (!interaction)
        return;

    WorldObject* player = PlayerEntity();
    // "ForcePlayer" makes the player character take the interaction at once.
    if (m_forcePlayer && m_activate && player && EntityIsCharacter(player))
        CharacterBeginInteraction(player, InteractionComponentId(interaction));
    if (m_activate)
        InteractionActivate(interaction, player);
    else
        InteractionDeactivate(interaction);
    InteractionSetCancelAllowed(interaction, m_allowCancel);
}

// ---------------------------------------------------------------------------
// ActivateMixState

// @func 0x0006BAB0 ActivateMixStateCommand::ActivateMixStateCommand
ActivateMixStateCommand::ActivateMixStateCommand() = default;

ActivateMixStateCommand::ActivateMixStateCommand(XmlElement* element)
{
    element->GetBool(kAttrActivate, &m_activate, true);
    element->GetHash(kAttrMixState, &m_mixStateName, 0);
    element->GetFloat(kAttrCrossfadeDuration, &m_crossfadeDuration, m_crossfadeDuration);
    if (m_mixStateName)
        m_mixState = FindMixState(m_mixStateName);
}

// @func 0x0006BB10 ActivateMixStateCommand::Link
void ActivateMixStateCommand::Link()
{
    // Mix states may be loaded after the script: retry the lookup.
    if (!m_mixState && m_mixStateName)
        m_mixState = FindMixState(m_mixStateName);
}

// @func 0x0006BB5C ActivateMixStateCommand::Create
ApeAnimScriptCommand* ActivateMixStateCommand::Create(XmlElement* element) const
{
    return new ActivateMixStateCommand(element);
}

// @func 0x0006BC70 ActivateMixStateCommand::Execute
void ActivateMixStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (!m_mixState) {
        ScriptWarning("ActivateMixState fails to find mix state %s", m_mixStateName);
        return;
    }
    if (m_activate) {
        if (!audio::IsMixStateActive(m_mixState))
            audio::PushMixState(m_mixState, m_crossfadeDuration);
    } else if (audio::IsMixStateActive(m_mixState)) {
        audio::RemoveMixState(m_mixState, m_crossfadeDuration);
    }
}

// ---------------------------------------------------------------------------
// #3 (0x37A78BFD): hand a part/material of an object to the player

// @func 0x0006BD3C ObjectMaterialCommand::ObjectMaterialCommand
ObjectMaterialCommand::ObjectMaterialCommand() = default;

// @func 0x0006BD8C ObjectMaterialCommand::~ObjectMaterialCommand
// @func 0x0006BDBC ObjectMaterialCommand::~ObjectMaterialCommand (deleting)
ObjectMaterialCommand::~ObjectMaterialCommand() = default;

// @func 0x0006BE14 ObjectMaterialCommand::Create
ApeAnimScriptCommand* ObjectMaterialCommand::Create(XmlElement* element) const
{
    auto* command = new ObjectMaterialCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrPart, &command->m_part, 0);
    element->GetHash(kAttrMaterial, &command->m_material, 0);
    return command;
}

// @func 0x0006BF28 ObjectMaterialCommand::Execute
void ObjectMaterialCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeRefCounted* object = nullptr;
    if (m_object != 0) {
        object = instance->ResolveObject(m_object, false);
        if (!object)
            object = instance->ResolveObject(m_object, true);
    }
    if (!object && m_entity != 0) {
        if (WorldObject* entity = FindEntity(m_entity))
            object = EntityWorldObject(entity);
        else
            ScriptWarning("ObjectMaterial: no entity %s", m_entity);
    }
    if (!object)
        return;

    // "part" narrows the object down to a named descendant.
    if (m_part != 0) {
        object = ObjectFindDescendant(object, m_part);
        if (!object)
            ScriptWarning("ObjectMaterial: no part %s", m_part);
    }
    if (!object || !ObjectIsGeometryInstance(object))
        return;
    WorldObject* player = PlayerEntity();
    if (!player)
        return;
    if (void* binocucom = PlayerBinocucom(player))
        BinocucomSetObjectMaterial(binocucom, object, m_material);
}

// ---------------------------------------------------------------------------
// #4 (0xB09EA328): blend an animation layer of an entity between two values

// @func 0x0006C0C8 AnimLayerBlendCommand::AnimLayerBlendCommand
AnimLayerBlendCommand::AnimLayerBlendCommand() = default;

// @func 0x0006C130 AnimLayerBlendCommand::~AnimLayerBlendCommand
// @func 0x0006C160 AnimLayerBlendCommand::~AnimLayerBlendCommand (deleting)
AnimLayerBlendCommand::~AnimLayerBlendCommand() = default;

// @func 0x0006C1B8 AnimLayerBlendCommand::Create
ApeAnimScriptCommand* AnimLayerBlendCommand::Create(XmlElement* element) const
{
    auto* command = new AnimLayerBlendCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    if (command->m_object || command->m_entity) {
        element->GetHash(kAttrDurationVar, &command->m_durationVar, command->m_durationVar);
        element->GetFloat(kAttrDuration, &command->m_duration, command->m_duration);
        element->GetFloat(kAttrFrom, &command->m_from, command->m_from);
        element->GetFloat(kAttrTo, &command->m_to, command->m_to);
        element->GetInt(kAttrAnimLayer, &command->m_animLayer, command->m_animLayer);
    }
    return command;
}

// @func 0x0006C370 AnimLayerBlendCommand::Execute
void AnimLayerBlendCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    WorldObject* entity = EntityFromVar(instance, m_object);
    if (!entity) {
        if (m_entity == 0) {
            if (m_object)
                ScriptWarning("%s", m_object);
            return;
        }
        entity = FindEntity(m_entity);
        if (!entity) {
            ScriptWarning("%s", m_entity);
            return;
        }
    }

    // Runs as a timed command: the weight follows the state clock.
    float duration = ResolveFloat(instance, m_durationVar, m_duration);
    float weight = m_to;
    if (duration >= kMinDuration && state->GetTime() < duration)
        weight = m_from + (m_to - m_from) * (state->GetTime() / duration);

    if (void* character = EntityCharacter(entity))
        CharacterSetAnimLayerWeight(character, weight, m_animLayer);
    else
        EntitySetAnimLayerWeight(entity, weight, m_animLayer);
}

// ---------------------------------------------------------------------------
// #5 (0x794FCBAC): set the clock of an animation layer

// @func 0x0006C5B0 AnimLayerTimeCommand::AnimLayerTimeCommand
AnimLayerTimeCommand::AnimLayerTimeCommand() = default;

// @func 0x0006C610 AnimLayerTimeCommand::~AnimLayerTimeCommand
// @func 0x0006C640 AnimLayerTimeCommand::~AnimLayerTimeCommand (deleting)
AnimLayerTimeCommand::~AnimLayerTimeCommand() = default;

// @func 0x0006C698 AnimLayerTimeCommand::Create
ApeAnimScriptCommand* AnimLayerTimeCommand::Create(XmlElement* element) const
{
    auto* command = new AnimLayerTimeCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrAnimTimeVar, &command->m_animTimeVar, command->m_animTimeVar);
        element->GetFloat(kAttrAnimTime, &command->m_animTime, command->m_animTime);
        element->GetInt(kAttrAnimLayer, &command->m_animLayer, command->m_animLayer);
        element->GetBool(kAttrScaleByLength, &command->m_scaleByLength, command->m_scaleByLength);
    }
    return command;
}

// @func 0x0006C800 AnimLayerTimeCommand::Execute
// Sets the playback time of an animation layer of an object: through the
// character or entity that owns it, else directly on the object's animation
// controller.
void AnimLayerTimeCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    ape::ApeAnimScriptObjectVar* var = FindObjectVar(instance, m_object);
    if (!var || !var->GetObject())
        return;

    float time = ResolveFloat(instance, m_animTimeVar, m_animTime);
    WorldObject* entity = EntityFromWorldObject(var->GetObject(), false, false);
    if (!entity) {
        void* controller = ObjectVarAnimController(var);
        if (!controller)
            return;
        void* anim = AnimControllerFindLayer(controller, m_animLayer);
        if (!anim)
            return;
        if (m_scaleByLength) {
            const float length = AnimLength(anim);
            time = length > kMinDuration ? length * time : 1.0f;
        }
        AnimSetTime(anim, time);
        return;
    }

    if (void* character = EntityCharacter(entity)) {
        CharacterBeginAnimEdit(character);
        CharacterSetAnimTime(character, time, m_scaleByLength, m_animLayer);
        CharacterEndAnimEdit(character);
    } else {
        EntitySetAnimElapsed(entity, time, m_scaleByLength, m_animLayer);
    }
}

// ---------------------------------------------------------------------------
// #6 (0xC495712B): no data

// @func 0x0006CA38 GlobalToggleCommand::GlobalToggleCommand
GlobalToggleCommand::GlobalToggleCommand() = default;

// @func 0x0006CA78 GlobalToggleCommand::~GlobalToggleCommand
// @func 0x0006CAA8 GlobalToggleCommand::~GlobalToggleCommand (deleting)
GlobalToggleCommand::~GlobalToggleCommand() = default;

// @func 0x0006CB00 GlobalToggleCommand::Create
ApeAnimScriptCommand* GlobalToggleCommand::Create(XmlElement*) const
{
    return new GlobalToggleCommand();
}

// @func 0x0006CB80 GlobalToggleCommand::Execute
void GlobalToggleCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GlobalToggleOn();
}

// ---------------------------------------------------------------------------
// AttachCarryItem (0x81E249E9)

// @func 0x0006CBB0 AttachCarryItemCommand::AttachCarryItemCommand
AttachCarryItemCommand::AttachCarryItemCommand() = default;

// @func 0x0006CC68 AttachCarryItemCommand::AttachCarryItemCommand
AttachCarryItemCommand::AttachCarryItemCommand(XmlElement* element)
{
    if (element->GetVec3(kAttrAttachOffset, &m_offset.x, &kZero.x) == ape::xml::kQuery_Ok)
        m_hasOffset = true;

    float degrees[3];
    if (element->GetVec3(kAttrAttachRot, degrees, &kZero.x) == ape::xml::kQuery_Ok) {
        // Euler angles in degrees -> quaternion (half angles).
        const float hx = degrees[0] * kDegToRad * 0.5f;
        const float hy = degrees[1] * kDegToRad * 0.5f;
        const float hz = degrees[2] * kDegToRad * 0.5f;
        const float cy = ape::FastCos(hy), cz = ape::FastCos(hz), cx = ape::FastCos(hx);
        const float sy = ape::FastSin(hy), sz = ape::FastSin(hz), sx = ape::FastSin(hx);
        m_hasRotation = true;
        m_rotation.w = cy * cz * cx - sy * sz * sx;
        m_rotation.x = sy * sz * cx + cy * cz * sx;
        m_rotation.z = cy * sz * cx - cz * sy * sx;
        m_rotation.y = cz * sy * cx + cy * sz * sx;
    }

    if (element->GetHash(kAttrAttachPart, &m_attachPart, 0) == ape::xml::kQuery_Ok)
        m_hasPart = true;
    element->GetHash(kAttrAttacheeVar, &m_attacheeVar, 0);
    element->GetHash(kAttrAttacheeEntity, &m_attacheeEntity, 0);
    element->GetHash(kAttrTargetVar, &m_targetVar, 0);
    element->GetHash(kAttrTargetEntity, &m_targetEntity, 0);
    element->GetBool(kAttrIgnoreRot, &m_ignoreRot, m_ignoreRot);
}

// @func 0x0006CF7C AttachCarryItemCommand::~AttachCarryItemCommand
// @func 0x0006CFAC AttachCarryItemCommand::~AttachCarryItemCommand (deleting)
AttachCarryItemCommand::~AttachCarryItemCommand() = default;

// @func 0x0006D004 AttachCarryItemCommand::Create
ApeAnimScriptCommand* AttachCarryItemCommand::Create(XmlElement* element) const
{
    return new AttachCarryItemCommand(element);
}

// @func 0x0006D078 AttachCarryItemCommand::Execute
void AttachCarryItemCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* attachee = ResolveEntity(instance, m_attacheeVar, m_attacheeEntity);
    WorldObject* target = ResolveEntity(instance, m_targetVar, m_targetEntity);

    WorldObject* character = (attachee && EntityIsCharacter(attachee)) ? attachee : nullptr;
    if (!character) {
        if (m_attacheeVar)
            ScriptWarning("%s", m_attacheeVar);
        if (m_attacheeEntity)
            ScriptWarning("%s", m_attacheeEntity);
    }
    if (!target) {
        if (m_targetVar)
            ScriptWarning("%s", m_targetVar);
        if (m_targetEntity)
            ScriptWarning("%s", m_targetEntity);
    }
    if (!character || !target)
        return;

    if (!CharacterCanAttach(character)) {
        ScriptWarning("%s", EntityName(character));
        ScriptWarning("%s", EntityName(target));
        return;
    }
    CharacterAttachTo(character, target, m_hasPart ? &m_attachPart : nullptr,
                      m_hasOffset ? &m_offset : nullptr, m_hasRotation ? &m_rotation : nullptr,
                      m_ignoreRot);
    if (EntityHasClassFlag(target, kEntityClass_20))
        EntitySetAttachMode(target, 1);
    CharacterBlendMotion(character, 0.0f, true);
}

// ---------------------------------------------------------------------------
// AttachEntity

// @func 0x0006D3A0 AttachEntityCommand::AttachEntityCommand
AttachEntityCommand::AttachEntityCommand() = default;

// @func 0x0006D488 AttachEntityCommand::AttachEntityCommand
AttachEntityCommand::AttachEntityCommand(XmlElement* element)
{
    element->GetHash(kAttrParent, &m_parent, 0);
    element->GetHash(kAttrChild, &m_child, m_child);
    element->GetHash(kAttrParentPart, &m_parentPart, m_parentPart);
    element->GetVec3(kAttrParentOffset, &m_parentOffset.x, &m_parentOffset.x);
    element->GetVec3(kAttrParentRotation, &m_parentRotation.x, &m_parentRotation.x);
    ScaleVec3(m_parentRotation, kDegToRad);
    element->GetHash(kAttrChildPart, &m_childPart, m_childPart);
    element->GetVec3(kAttrChildOffset, &m_childOffset.x, &m_childOffset.x);
    element->GetVec3(kAttrChildRotation, &m_childRotation.x, &m_childRotation.x);
    ScaleVec3(m_childRotation, kDegToRad);
    if (element->GetFloat(kAttrGravity, &m_gravity, m_gravity) == ape::xml::kQuery_Ok)
        m_hasGravity = true;
    element->GetBool(kAttrSyncAnimation, &m_syncAnimation, m_syncAnimation);
    element->GetBool(kAttrAttachFlag75, &m_flag75, m_flag75);
    element->GetBool(kAttrAttachFlag76, &m_flag76, m_flag76);
    element->GetBool(kAttrAttachFlag77, &m_flag77, m_flag77);
    element->GetBool(kAttrAttachFlag79, &m_flag79, m_flag79);
}

// @func 0x0006D7C8 AttachEntityCommand::Create
ApeAnimScriptCommand* AttachEntityCommand::Create(XmlElement* element) const
{
    return new AttachEntityCommand(element);
}

// TODO(0x0006D840 AttachEntityCommand::Execute) attaches through the entity's
// carry component (0x000D9544, 0x002B91EC/0x002B9454) or the world transform.

// ---------------------------------------------------------------------------
// Blast

// @func 0x0006DD14 BlastCommand::BlastCommand
BlastCommand::BlastCommand() = default;

// @func 0x0006DDB8 BlastCommand::BlastCommand
BlastCommand::BlastCommand(XmlElement* element)
{
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetVec3(kAttrOffsetPos, &m_offsetPos.x, &kZero.x);
    element->GetVec3(kAttrBlastDirection, &m_direction.x, &kZero.x);
    element->GetFloat(kAttrDamage, &m_damage, 0.0f);
    element->GetFloat(kAttrImpulse, &m_impulse, 0.0f);
    element->GetFloat(kAttrRadiusMin, &m_radiusMin, 0.0f);
    element->GetFloat(kAttrRadiusMax, &m_radiusMax, 0.0f);
    element->GetBool(kAttrIgnoreHeight, &m_ignoreHeight, false);
    element->GetHash(kAttrReactType, &m_reactType, 0);

    int32_t value = 0;
    if (element->GetInt(kAttrBlastField4C, &value, 0) == ape::xml::kQuery_Ok)
        m_field4C = value > 0 ? value - 1 : 0;

    if (XmlElement* types = element->FirstChildElement(kElemDamageTypes)) {
        for (int i = 0; i < kDamageTypeCount; ++i) {
            bool enabled = false;
            if (types->GetBool(kBlastDamageTypes[i], &enabled, false) != ape::xml::kQuery_Ok)
                continue;
            if (enabled)
                m_damageTypeMask |= 1u << i;
            else
                m_damageTypeMask &= ~(1u << i);
        }
    }
    if (m_radiusMin > m_radiusMax)
        m_radiusMin = 0.0f;
}

// @func 0x0006E0C4 BlastCommand::~BlastCommand
// @func 0x0006E0F4 BlastCommand::~BlastCommand (deleting)
BlastCommand::~BlastCommand() = default;

// @func 0x0006E14C BlastCommand::Create
ApeAnimScriptCommand* BlastCommand::Create(XmlElement* element) const
{
    return new BlastCommand(element);
}

// TODO(0x0006E1BC BlastCommand::Execute) Ghidra cannot decode it; it starts by
// looking up the "this" object variable.

// ---------------------------------------------------------------------------
// Camera commands. The game camera is the entity at world +0x1281E0.

// @func 0x0006ECB8 CameraClearEaseCommand::CameraClearEaseCommand
CameraClearEaseCommand::CameraClearEaseCommand() = default;

// @func 0x0006ECF8 CameraClearEaseCommand::Create
ApeAnimScriptCommand* CameraClearEaseCommand::Create(XmlElement*) const
{
    return new CameraClearEaseCommand();
}

// @func 0x0006ED80 CameraClearEaseCommand::Execute
void CameraClearEaseCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (WorldObject* camera = GameCamera()) {
        CameraSetEase(camera, -1.0f);
        CameraSetEaseRate(camera, 0.1f);
    }
}

// @func 0x0006EDE4 CameraPlayAnimCommand::CameraPlayAnimCommand
CameraPlayAnimCommand::CameraPlayAnimCommand() = default;

// @func 0x0006EE44 CameraPlayAnimCommand::Create
ApeAnimScriptCommand* CameraPlayAnimCommand::Create(XmlElement* element) const
{
    auto* command = new CameraPlayAnimCommand();
    element->GetHash(kAttrAnim, &command->m_anim, 0);
    element->GetFloat(kAttrSpeed, &command->m_speed, command->m_speed);
    element->GetBool(kAttrLoop, &command->m_loop, command->m_loop);
    element->GetBool(kAttrCameraAnimFlag, &command->m_flag15, command->m_flag15);
    return command;
}

// @func 0x0006EF5C CameraPlayAnimCommand::Execute
void CameraPlayAnimCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* camera = GameCamera();
    if (camera && EntityIsAlive(camera) && m_anim)
        CameraPlayAnim(camera, m_anim, m_speed, m_loop, this, m_flag15);
}

// @func 0x0006EFD4 CameraShakeCommand::CameraShakeCommand
CameraShakeCommand::CameraShakeCommand() = default;

// @func 0x0006F094 CameraShakeCommand::CameraShakeCommand
CameraShakeCommand::CameraShakeCommand(XmlElement* element)
{
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetFloat(kAttrRadius, &m_radius, m_radius);
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
}

// @func 0x0006F300 CameraShakeCommand::~CameraShakeCommand
// @func 0x0006F330 CameraShakeCommand::~CameraShakeCommand (deleting)
CameraShakeCommand::~CameraShakeCommand() = default;

// @func 0x0006F388 CameraShakeCommand::Create
ApeAnimScriptCommand* CameraShakeCommand::Create(XmlElement* element) const
{
    return new CameraShakeCommand(element);
}

// @func 0x0006F3F8 CameraShakeCommand::Execute
// Starts a camera shake; with "RefObject" the shake originates at that object
// (the camera manager attenuates it by distance within "Radius").
void CameraShakeCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    if (ape::ApeAnimScriptObjectVar* var = FindObjectVar(instance, m_refObject)) {
        if (var->GetObject())
            m_origin = ObjectWorldPosition(var->GetObject());
    }
    SequenceShakeParams params;
    params.duration = m_duration;
    params.magnitude = m_magnitude;
    params.startMagnitude = m_startMagnitude;
    params.falloffMode = m_falloffMode;
    params.falloffStartTime = m_falloffStartTime;
    params.pitch = m_pitch;
    params.yaw = m_yaw;
    params.roll = m_roll;
    params.pitchFrequency = m_pitchFrequency;
    params.yawFrequency = m_yawFrequency;
    params.rollFrequency = m_rollFrequency;
    params.radius = m_radius;
    params.vector40 = m_origin;
    params.value50 = m_field60;
    CameraAddShake(params);
}

// @func 0x0006F614 CameraStopAnimCommand::CameraStopAnimCommand
CameraStopAnimCommand::CameraStopAnimCommand() = default;

// @func 0x0006F664 CameraStopAnimCommand::Create
ApeAnimScriptCommand* CameraStopAnimCommand::Create(XmlElement* element) const
{
    auto* command = new CameraStopAnimCommand();
    element->GetBool(kAttrRemoveAnimation, &command->m_removeAnimation, false);
    return command;
}

// @func 0x0006F70C CameraStopAnimCommand::Execute
void CameraStopAnimCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* camera = GameCamera();
    if (camera && EntityIsAlive(camera))
        CameraStopAnim(camera, m_removeAnimation);
}

// ---------------------------------------------------------------------------
// CloseBinocucom

// @func 0x0006FCE0 CloseBinocucomCommand::CloseBinocucomCommand
CloseBinocucomCommand::CloseBinocucomCommand() = default;

// @func 0x0006FD20 CloseBinocucomCommand::Create
ApeAnimScriptCommand* CloseBinocucomCommand::Create(XmlElement*) const
{
    return new CloseBinocucomCommand();
}

// @func 0x0006FDA0 CloseBinocucomCommand::Execute
void CloseBinocucomCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* player = PlayerEntity();
    if (!player || !EntityIsCharacter(player))
        return;
    if (void* binocucom = PlayerBinocucom(player))
        BinocucomClose(binocucom);
}

// ---------------------------------------------------------------------------
// CompleteToGoal

// @func 0x0006FE0C CompleteGoalCommand::CompleteGoalCommand
CompleteGoalCommand::CompleteGoalCommand() = default;

// @func 0x0006FE54 CompleteGoalCommand::Create
ApeAnimScriptCommand* CompleteGoalCommand::Create(XmlElement* element) const
{
    auto* command = new CompleteGoalCommand();
    element->GetHash(kAttrGoal, &command->m_goal, 0);
    element->GetHash(kAttrGoalVar, &command->m_goalVar, command->m_goalVar);
    return command;
}

// @func 0x0006FF1C CompleteGoalCommand::Execute
void CompleteGoalCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    uint32_t goal = static_cast<uint32_t>(ResolveInt(instance, m_goalVar, static_cast<int32_t>(m_goal)));
    if (goal)
        GoalSetComplete(goal, true, true);
}

// ---------------------------------------------------------------------------
// CharacterSpecialCommand (0xB8B01DB7)

// @func 0x0006F768 CharacterSpecialCommand::CharacterSpecialCommand
CharacterSpecialCommand::CharacterSpecialCommand() = default;

// @func 0x0006F7C8 CharacterSpecialCommand::CharacterSpecialCommand
CharacterSpecialCommand::CharacterSpecialCommand(XmlElement* element)
{
    element->GetHash(kAttrObject, &m_object, 0);
    element->GetHash(kAttrEntity, &m_entity, 0);
    element->GetHash(kAttrCommand, &m_command, m_command);
    switch (m_command) {
    case kCommand_Direction: {
        float dir[3];
        int result = element->GetVec3(kAttrDir, dir, &kDefaultDirection.x);
        if (result != ape::xml::kQuery_Ok)
            result = element->GetVec3(kAttrDirection, dir, &kDefaultDirection.x);
        // NOTE: the console stores the default vector, not the value read.
        if (result == ape::xml::kQuery_Ok)
            m_direction = kDefaultDirection;
        break;
    }
    case kCommand_Angle:
        element->GetFloat(kAttrAngle, &m_angle, -1.0f);
        break;
    case kCommand_Enable:
        element->GetBool(kAttrEnable, &m_flag, m_flag);
        break;
    case kCommand_SkipAnim:
        element->GetBool(kAttrSkipAnim, &m_flag, false);
        break;
    default:
        break;
    }
}

// @func 0x0006F9D0 CharacterSpecialCommand::Create
ApeAnimScriptCommand* CharacterSpecialCommand::Create(XmlElement* element) const
{
    return new CharacterSpecialCommand(element);
}

// @func 0x0006FA40 CharacterSpecialCommand::Execute
void CharacterSpecialCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity) {
        ScriptWarning("CharacterSpecial: no entity %s", m_entity);
        return;
    }
    if (!EntityIsCharacter(entity)) {
        ScriptWarning("CharacterSpecial: %s is not a character", m_entity);
        return;
    }
    switch (m_command) {
    case kCommand_Direction:
        if (void* special = CharacterSpecialComponent(entity))
            SpecialSetDirection(special, m_direction);
        break;
    case kCommand_Angle:
        if (void* special = CharacterSpecialComponent(entity))
            SpecialSetAngle(special, m_angle);
        break;
    case kCommand_Enable:
        if (void* special = CharacterSpecialComponent(entity))
            SpecialSetEnabled(special, m_flag);
        break;
    case kCommand_SkipAnim:
        CharacterSetSkipAnim(entity, m_flag);
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// EntityGameStateCommand (0xCB291F57)

// @func 0x0006FFF0 EntityGameStateCommand::EntityGameStateCommand
EntityGameStateCommand::EntityGameStateCommand() = default;

// @func 0x00070040 EntityGameStateCommand::Create
ApeAnimScriptCommand* EntityGameStateCommand::Create(XmlElement* element) const
{
    auto* command = new EntityGameStateCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    return command;
}

// @func 0x0007010C EntityGameStateCommand::Execute
void EntityGameStateCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    WorldObject* entity = ResolveEntity(instance, m_object, m_entity);
    if (!entity) {
        ScriptWarning("%s", m_entity);
        return;
    }
    GameStateNotifyEntity(EntityName(entity));
}

// ---------------------------------------------------------------------------
// ClearCounterCommand (0xA1D6F663)

// @func 0x0006F4B8 ClearCounterCommand::ClearCounterCommand
ClearCounterCommand::ClearCounterCommand() = default;

// @func 0x0006F4F8 ClearCounterCommand::~ClearCounterCommand
// @func 0x0006F528 ClearCounterCommand::~ClearCounterCommand (deleting)
ClearCounterCommand::~ClearCounterCommand() = default;

// @func 0x0006F580 ClearCounterCommand::Create
ApeAnimScriptCommand* ClearCounterCommand::Create(XmlElement*) const
{
    return new ClearCounterCommand();
}

// @func 0x0006F600 ClearCounterCommand::Execute
void ClearCounterCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    ClearGameCounter();
}

// ---------------------------------------------------------------------------
// Crossfade

namespace {
// Alpha values are given as ints and clamped to a byte.
uint8_t ClampAlpha(int32_t value)
{
    float alpha = static_cast<float>(value);
    if (alpha > 255.0f)
        alpha = 255.0f;
    if (alpha < 0.0f)
        alpha = 0.0f;
    return static_cast<uint8_t>(alpha);
}

void ReadMaterialMap(XmlElement* parent, ape::AVLMap<uint32_t, uint32_t>& map)
{
    for (XmlElement* entry = parent->FirstChildElement(kAttrMaterial); entry;
         entry = entry->NextSiblingElement(kAttrMaterial)) {
        uint32_t from = 0, to = 0;
        entry->GetHash(kAttrFrom, &from, 0);
        entry->GetHash(kAttrTo, &to, 0);
        map.Add(from, to);
    }
}
} // namespace

// @func 0x0007020C CrossfadeCommand::CrossfadeCommand
CrossfadeCommand::CrossfadeCommand() = default;

// @func 0x00070390 CrossfadeCommand::CrossfadeCommand
CrossfadeCommand::CrossfadeCommand(XmlElement* element)
{
    if (element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok)
        return;

    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    element->GetBool(kAttrCrossfadeFlag23B, &m_flag23B, m_flag23B);
    element->GetBool(kAttrCrossfadeFlag23C, &m_flag23C, m_flag23C);
    element->GetBool(kAttrCrossfadeFlag23D, &m_flag23D, m_flag23D);
    element->GetHash(kAttrDefaultMat, &m_defaultMat, m_defaultMat);
    element->GetHash(kAttrCrossfade230, &m_field230, m_field230);

    int32_t fromAlpha = 0, toAlpha = 0;
    element->GetInt(kAttrFromAlpha, &fromAlpha, 0xFF);
    element->GetInt(kAttrToAlpha, &toAlpha, 0xFF);
    m_fromAlpha = ClampAlpha(fromAlpha);
    m_toAlpha = ClampAlpha(toAlpha);

    ReadMaterialMap(element, m_materials);

    int count = 0;
    for (XmlElement* child = element->FirstChildElement(kElemEntityChild); child && count < kMaxChildren;
         child = child->NextSiblingElement(kElemEntityChild), ++count) {
        child->GetHash(kAttrName, &m_children[count].name, 0);
        ReadMaterialMap(child, m_children[count].materials);
    }
    m_childCount = static_cast<uint8_t>(count);
}

// @func 0x00070894 CrossfadeCommand::~CrossfadeCommand
// @func 0x00070B00 CrossfadeCommand::~CrossfadeCommand (deleting)
CrossfadeCommand::~CrossfadeCommand() = default;

// @func 0x00070B48 CrossfadeCommand::Create
ApeAnimScriptCommand* CrossfadeCommand::Create(XmlElement* element) const
{
    return new CrossfadeCommand(element);
}

// TODO(0x00070BB8 CrossfadeCommand::Execute) builds the cross-fade materials.

// ---------------------------------------------------------------------------
// Damage

// @func 0x000717B8 DamageCommand::DamageCommand
DamageCommand::DamageCommand() = default;

// @func 0x00071808 DamageCommand::Create
ApeAnimScriptCommand* DamageCommand::Create(XmlElement* element) const
{
    auto* command = new DamageCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetFloat(kAttrDamage, &command->m_damage, command->m_damage);
    element->GetHash(kAttrType, &command->m_type, command->m_type);
    element->GetFloat(kAttrKnockbackImpulse, &command->m_knockbackImpulse, command->m_knockbackImpulse);
    return command;
}

// TODO(0x00071928 DamageCommand::Execute) builds a damage record and calls the
// entity's damage virtuals (slots 86/88).

// ---------------------------------------------------------------------------
// DetachEntity

// @func 0x00071C2C DetachEntityCommand::DetachEntityCommand
DetachEntityCommand::DetachEntityCommand() = default;

// @func 0x00071C7C DetachEntityCommand::Create
ApeAnimScriptCommand* DetachEntityCommand::Create(XmlElement* element) const
{
    auto* command = new DetachEntityCommand();
    element->GetHash(kAttrParent, &command->m_parent, 0);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    return command;
}

// @func 0x00071D48 DetachEntityCommand::Execute
void DetachEntityCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState*)
{
    // Each operand is an object variable or, failing that, an entity name.
    WorldObject* parent = ResolveEntity(instance, m_parent, m_parent);
    if (!parent)
        return;
    WorldObject* child = ResolveEntity(instance, m_child, m_child);
    if (!child)
        return;
    EntityDetach(parent, child);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsA()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(0x36DF0092u, new ActivateInteractionCommand());
    prototypes.Add(AnimHash("ActivateMixState"), new ActivateMixStateCommand());
    prototypes.Add(0x37A78BFDu, new ObjectMaterialCommand());
    prototypes.Add(0xB09EA328u, new AnimLayerBlendCommand());
    prototypes.Add(0x794FCBACu, new AnimLayerTimeCommand());
    prototypes.Add(0xC495712Bu, new GlobalToggleCommand());
    prototypes.Add(AnimHash("AttachCarryItem"), new AttachCarryItemCommand());
    prototypes.Add(AnimHash("AttachEntity"), new AttachEntityCommand());
    prototypes.Add(AnimHash("Blast"), new BlastCommand());
    prototypes.Add(AnimHash("CameraClearEase"), new CameraClearEaseCommand());
    prototypes.Add(0xD1F7C236u, new CameraPlayAnimCommand());
    prototypes.Add(AnimHash("CameraShake"), new CameraShakeCommand());
    prototypes.Add(0x3D0E7A04u, new CameraStopAnimCommand());
    prototypes.Add(AnimHash("CloseBinoccucom"), new CloseBinocucomCommand());  // sic
    prototypes.Add(AnimHash("CompleteToGoal"), new CompleteGoalCommand());
    prototypes.Add(0xB8B01DB7u, new CharacterSpecialCommand());
    prototypes.Add(0xCB291F57u, new EntityGameStateCommand());
    prototypes.Add(0xA1D6F663u, new ClearCounterCommand());
    prototypes.Add(AnimHash("Crossfade"), new CrossfadeCommand());
    prototypes.Add(AnimHash("Damage"), new DamageCommand());
    prototypes.Add(AnimHash("DetachEntity"), new DetachEntityCommand());
}

} // namespace levels
} // namespace sly
