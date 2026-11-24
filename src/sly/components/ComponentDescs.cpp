// openTIT - Component descriptions parsed from entity XML.
#include "sly/components/ComponentDescs.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace components {

namespace {

using ape::xml::XmlElement;

// Attribute name hashes (HashNameNoCase of the names). Names marked unknown
// had no match among the strings of the executable.
constexpr uint32_t kAttr_Set            = 0x00014042u;
constexpr uint32_t kAttr_Tree           = 0x00276B9Eu;
constexpr uint32_t kAttr_Behavior       = 0x88B4A252u;
constexpr uint32_t kAttr_Map            = 0x0001293Cu;
constexpr uint32_t kAttr_BlendMap       = 0xF690C76Bu;
constexpr uint32_t kAttr_Part           = 0x00255BF3u;
constexpr uint32_t kAttr_RotSpeed       = 0x2D3AC4D0u;
constexpr uint32_t kAttr_LockY          = 0x0453E88Eu;
constexpr uint32_t kAttr_FacePlayer1D   = 0x05AB7B55u;   // NOTE: name unknown
constexpr uint32_t kAttr_Type           = 0x0027873Au;
constexpr uint32_t kAttr_Radius         = 0x8FA16092u;
constexpr uint32_t kAttr_RefRadius      = 0x84323AC5u;
constexpr uint32_t kAttr_Height         = 0x7ECB8527u;
constexpr uint32_t kAttr_Mass           = 0x0023FEF4u;
constexpr uint32_t kAttr_StartEnabled   = 0xBEF25C9Fu;
constexpr uint32_t kAttr_MaterialObject = 0x4D0C3326u;
constexpr uint32_t kAttr_AnchorEntity   = 0x6EADF2D8u;
constexpr uint32_t kAttr_Script         = 0x9178C4CBu;
constexpr uint32_t kAttr_MinRange       = 0xDE1DA62Bu;
constexpr uint32_t kAttr_MaxRange       = 0x47FC4259u;
constexpr uint32_t kAttr_MinAngle       = 0xDD33E5C1u;
constexpr uint32_t kAttr_MaxAngle       = 0x471281EFu;
constexpr uint32_t kAttr_Strength       = 0x996B4041u;
constexpr uint32_t kAttr_Score          = 0x04B14672u;
constexpr uint32_t kAttr_EndPos         = 0x7A29AB99u;
constexpr uint32_t kAttr_StartVel       = 0x7D22FFFBu;
constexpr uint32_t kAttr_Damp           = 0x001FE6E0u;
constexpr uint32_t kAttr_Speed          = 0x04B70827u;
constexpr uint32_t kAttr_Cracker18      = 0xCC2ABC8Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_Damping        = 0x807548E2u;
constexpr uint32_t kAttr_OnDuration     = 0x476DF873u;
constexpr uint32_t kAttr_OffDuration    = 0x0148AFE3u;
constexpr uint32_t kAttr_TimeOffset     = 0x7B2F95A0u;
constexpr uint32_t kAttr_BoneName       = 0xA424A34Fu;
constexpr uint32_t kAttr_EntityName     = 0xC490A0EEu;
constexpr uint32_t kAttr_Grow           = 0x00218413u;
constexpr uint32_t kAttr_BallRoll28     = 0x5D1A6CBAu;   // NOTE: name unknown
constexpr uint32_t kAttr_BallRoll2C     = 0x2BCD00F1u;   // NOTE: name unknown
constexpr uint32_t kAttr_DamageSeq18    = 0xBF260CF3u;   // NOTE: name unknown
constexpr uint32_t kAttr_InitTime       = 0x3EB49FDDu;
constexpr uint32_t kAttr_EnabledTime    = 0x2B6FE96Eu;
constexpr uint32_t kAttr_DisabledTime   = 0x4FA1D749u;
constexpr uint32_t kAttr_DamageSeq29    = 0x173A6338u;   // NOTE: name unknown

constexpr float kDegToRad = 0.017453292f;

// Shared start of every parser: the description named by the element's
// "name" attribute, or null (after the compiled-out error) without one.
template <class Desc>
Desc* NewNamedDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    Desc* desc = new Desc(ape::HashNameNoCase(name));
    desc->scope = scope;
    return desc;
}

void ReadPulseField(XmlElement* elem, PulseFieldComponentDesc& desc)
{
    elem->GetFloat(kAttr_Radius, &desc.radius, desc.radius);
    elem->GetFloat(kAttr_Strength, &desc.strength, desc.strength);
    elem->GetFloat(kAttr_Damping, &desc.damping, desc.damping);
    elem->GetFloat(kAttr_OnDuration, &desc.onDuration, desc.onDuration);
    elem->GetFloat(kAttr_OffDuration, &desc.offDuration, desc.offDuration);
    elem->GetFloat(kAttr_TimeOffset, &desc.timeOffset, desc.timeOffset);
}

} // namespace

// @func 0x002975F4 ParseAnimComponentDesc
ComponentDesc* ParseAnimComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    AnimComponentDesc* desc = NewNamedDesc<AnimComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Set, &desc->animSet, desc->animSet);
        elem->GetHash(kAttr_Tree, &desc->animTree, desc->animTree);
    }
    return desc;
}

// @func 0x0027CA10 ParseControlAIComponentDesc
ComponentDesc* ParseControlAIComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ControlAIComponentDesc* desc = NewNamedDesc<ControlAIComponentDesc>(root, elem, scope);
    if (desc)
        elem->GetHash(kAttr_Behavior, &desc->behavior, desc->behavior);
    return desc;
}

// @func 0x002879AC ParseAnimMapComponentDesc
ComponentDesc* ParseAnimMapComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    AnimMapComponentDesc* desc = NewNamedDesc<AnimMapComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Map, &desc->map, desc->map);
        elem->GetHash(kAttr_Tree, &desc->tree, desc->tree);
        elem->GetHash(kAttr_BlendMap, &desc->blendMap, desc->blendMap);
        elem->GetHash(kAttr_Set, &desc->animSet, desc->animSet);
    }
    return desc;
}

// @func 0x0028F080 ParseFacePlayerComponentDesc
ComponentDesc* ParseFacePlayerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    FacePlayerComponentDesc* desc = NewNamedDesc<FacePlayerComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Part, &desc->part, desc->part);
        if (elem->GetFloat(kAttr_RotSpeed, &desc->rotSpeed, desc->rotSpeed) == ape::xml::kQuery_Ok)
            desc->rotSpeed *= kDegToRad;
        elem->GetBool(kAttr_LockY, &desc->lockY, desc->lockY);
        elem->GetBool(kAttr_FacePlayer1D, &desc->flag1D, desc->flag1D);
    }
    return desc;
}

// @func 0x0029FAA0 ParseRadarBlipComponentDesc
ComponentDesc* ParseRadarBlipComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name) {
        // NOTE: the console still reads "Type" into the null description.
        return nullptr;
    }
    RadarBlipComponentDesc* desc = new RadarBlipComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Type, &desc->blipType, desc->blipType);
    return desc;
}

// @func 0x002BB9FC ParseSimplePhysicsComponentDesc
ComponentDesc* ParseSimplePhysicsComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SimplePhysicsComponentDesc* desc = NewNamedDesc<SimplePhysicsComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_Radius, &desc->radius, desc->radius);
        elem->GetFloat(kAttr_Height, &desc->height, desc->height);
        elem->GetFloat(kAttr_Mass, &desc->mass, desc->mass);
        elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    }
    return desc;
}

// @func 0x002AF0D4 ParseJumpRopeComponentDesc
ComponentDesc* ParseJumpRopeComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    JumpRopeComponentDesc* desc = NewNamedDesc<JumpRopeComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_MaterialObject, &desc->materialObject, desc->materialObject);
        elem->GetHash(kAttr_AnchorEntity, &desc->anchorEntity, desc->anchorEntity);
    }
    return desc;
}

// @func 0x00280614 ParseAeSplineComponentDesc
ComponentDesc* ParseAeSplineComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    return NewNamedDesc<AeSplineComponentDesc>(root, elem, scope);
}

// @func 0x002CBB40 ParseTrackerComponentDesc
ComponentDesc* ParseTrackerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    TrackerComponentDesc* desc = new TrackerComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Script, &desc->script, desc->script);
    elem->GetFloat(kAttr_MinRange, &desc->minRange, desc->minRange);
    elem->GetFloat(kAttr_MaxRange, &desc->maxRange, desc->maxRange);
    elem->GetFloat(kAttr_MinAngle, &desc->minAngle, desc->minAngle);
    elem->GetFloat(kAttr_MaxAngle, &desc->maxAngle, desc->maxAngle);
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    return desc;
}

// @func 0x002B06D4 ParseVortexComponentDesc
ComponentDesc* ParseVortexComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    VortexComponentDesc* desc = NewNamedDesc<VortexComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_Radius, &desc->radius, desc->radius);
        elem->GetFloat(kAttr_Strength, &desc->strength, desc->strength);
    }
    return desc;
}

// @func 0x002B1D18 ParseScoringComponentDesc
ComponentDesc* ParseScoringComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ScoringComponentDesc* desc = NewNamedDesc<ScoringComponentDesc>(root, elem, scope);
    if (desc) {
        const float zero[3] = {0.0f, 0.0f, 0.0f};
        elem->GetFloat(kAttr_Score, &desc->score, desc->score);
        elem->GetVec3(kAttr_EndPos, &desc->endPos.x, zero);
        elem->GetFloat(kAttr_StartVel, &desc->startVel, 0.0f);
        elem->GetFloat(kAttr_Damp, &desc->damp, 5.0f);
        elem->GetFloat(kAttr_Speed, &desc->speed, 20.0f);
    }
    return desc;
}

// @func 0x002B1628 ParseSystemCrackerComponentDesc
ComponentDesc* ParseSystemCrackerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SystemCrackerComponentDesc* desc = NewNamedDesc<SystemCrackerComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Script, &desc->script, desc->script);
        elem->GetBool(kAttr_Cracker18, &desc->flag18, false);
    }
    return desc;
}

// @func 0x002B2814 ParseMagNetComponentDesc
ComponentDesc* ParseMagNetComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    MagNetComponentDesc* desc = NewNamedDesc<MagNetComponentDesc>(root, elem, scope);
    if (desc)
        ReadPulseField(elem, *desc);
    return desc;
}

// @func 0x002B4E78 ParseSpeedBoostComponentDesc
ComponentDesc* ParseSpeedBoostComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SpeedBoostComponentDesc* desc = NewNamedDesc<SpeedBoostComponentDesc>(root, elem, scope);
    if (desc)
        ReadPulseField(elem, *desc);
    return desc;
}

// @func 0x002D08C0 ParseBallRollComponentDesc
ComponentDesc* ParseBallRollComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    BallRollComponentDesc* desc = NewNamedDesc<BallRollComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_BoneName, &desc->boneName, 0);
        elem->GetHash(kAttr_EntityName, &desc->entityName, 0);
        // NOTE: both reads target the same field with a default of 0, so
        // "Radius" only survives when "RefRadius" is present too (console bug).
        elem->GetFloat(kAttr_Radius, &desc->radius, 0.0f);
        elem->GetFloat(kAttr_RefRadius, &desc->radius, 0.0f);
        elem->GetBool(kAttr_Grow, &desc->grow, false);
        elem->GetHash(kAttr_BallRoll28, &desc->hash28, 0);
        elem->GetHash(kAttr_BallRoll2C, &desc->hash2C, 0);
    }
    return desc;
}

// @func 0x002D2F3C ParseDamageSequenceComponentDesc
ComponentDesc* ParseDamageSequenceComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    DamageSequenceComponentDesc* desc = new DamageSequenceComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Script, &desc->script, desc->script);
    elem->GetInt(kAttr_DamageSeq18, &desc->int18, desc->int18);
    elem->GetFloat(kAttr_InitTime, &desc->initTime, desc->initTime);
    elem->GetFloat(kAttr_EnabledTime, &desc->enabledTime, desc->enabledTime);
    elem->GetFloat(kAttr_DisabledTime, &desc->disabledTime, desc->disabledTime);
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    elem->GetBool(kAttr_DamageSeq29, &desc->flag29, desc->flag29);
    return desc;
}

} // namespace components
} // namespace sly
