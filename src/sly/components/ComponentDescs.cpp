// openTIT - Component descriptions parsed from entity XML.
#include "sly/components/ComponentDescs.h"

#include "sly/components/ComponentHooks.h"

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

constexpr uint32_t kAttr_MaxDepth        = 0x4738D3DFu;
constexpr uint32_t kAttr_FloatSpeed      = 0xE165E36Bu;
constexpr uint32_t kAttr_SinkSpeed       = 0x543E5574u;
constexpr uint32_t kAttr_Buoyancy20      = 0xFFF7B927u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy24      = 0x7F154347u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy28      = 0x827EF485u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy3C      = 0x4801705Eu;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy40      = 0xCD0D2AF5u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy44      = 0x708A4E60u;   // NOTE: name unknown
constexpr uint32_t kAttr_TiltSpeed       = 0x8E11EB8Au;
constexpr uint32_t kAttr_Buoyancy4C      = 0xC4F41CF6u;   // NOTE: name unknown
constexpr uint32_t kAttr_MaxRotationX    = 0x735FF3D6u;
constexpr uint32_t kAttr_MaxRotationY    = 0x735FF3D7u;
constexpr uint32_t kAttr_MaxRotationZ    = 0x735FF3D8u;
constexpr uint32_t kAttr_ContactScript   = 0x143774CBu;
constexpr uint32_t kAttr_ContactInterval = 0x1E7884C5u;
constexpr uint32_t kAttr_GrabAngleH      = 0xB5292F41u;
constexpr uint32_t kAttr_MaxAngleV       = 0x9B3DBC47u;
constexpr uint32_t kAttr_MaxAngleH       = 0x9B3DBC39u;
constexpr uint32_t kAttr_GrabDistance    = 0x53A7F241u;
constexpr uint32_t kAttr_MaxDistance     = 0x06CD2139u;
constexpr uint32_t kAttr_StickThreshold  = 0x82E789DBu;
constexpr uint32_t kAttr_WallClimb50     = 0x977C8AB2u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb54     = 0xB081C3B4u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb58     = 0x01B279B3u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb60     = 0x009DAD78u;   // NOTE: name unknown

constexpr uint32_t kAttr_Size            = 0x0026D801u;
constexpr uint32_t kAttr_Offset          = 0x8ACA5893u;
constexpr uint32_t kAttr_Enabled         = 0xCADB1721u;
constexpr uint32_t kAttr_Physics91       = 0x8BB0F765u;   // NOTE: name unknown

constexpr uint32_t kAttr_Marker14        = 0x457D8076u;   // NOTE: name unknown
constexpr uint32_t kAttr_Marker18        = 0x4EA1C7E6u;   // NOTE: name unknown
constexpr uint32_t kAttr_MarkerRange     = 0x4852D5AFu;   // NOTE: name unknown
constexpr uint32_t kAttr_OnscreenColor   = 0x7087CB18u;
constexpr uint32_t kAttr_OffscreenColor  = 0x83D09DA8u;
constexpr uint32_t kAttr_Marker28        = 0xF6DB9816u;   // NOTE: name unknown

constexpr uint32_t kTag_ClosetSpawn      = 0x7480D3BAu;   // NOTE: element name unknown
constexpr uint32_t kAttr_Def             = 0x000107E5u;
constexpr uint32_t kAttr_SpawnPoint      = 0x6D4342F5u;
constexpr uint32_t kAttr_RespawnCooldown = 0x7307C873u;
constexpr uint32_t kAttr_RespawnEnabled  = 0x83CA0519u;
// Spawn kinds by "Type" (0x00AD538C): none, "Mine".
constexpr uint32_t kClosetSpawnTypes[2] = {0u, 0x00241C53u};

constexpr uint32_t kAttr_Name            = 0x0024728Bu;
constexpr uint32_t kTag_JointTwoSolver   = 0x75215015u;
constexpr uint32_t kTag_JointThreeSolver = 0x6AD11467u;
constexpr uint32_t kTag_FootSolver       = 0x895ACE01u;
constexpr uint32_t kTag_LookAtSolver     = 0x5BDDD9A5u;

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

// "Name" of up to `count` children with tag `tag`, in order.
void ReadChildNames(XmlElement* elem, uint32_t tag, uint32_t* out, uint32_t count)
{
    XmlElement* child = elem->FirstChildElement(tag);
    for (uint32_t i = 0; child && i < count; ++i) {
        uint32_t value = 0;
        child->GetHash(kAttr_Name, &value, 0);
        out[i] = value;
        child = child->NextSiblingElement(tag);
    }
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

// @func 0x0026EFD4 ParsePhysicsComponentDesc
ComponentDesc* ParsePhysicsComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    PhysicsComponentDesc* desc = NewNamedDesc<PhysicsComponentDesc>(root, elem, scope);
    if (desc) {
        desc->world = LevelPhysicsWorld();
        elem->GetFloat(kAttr_Mass, &desc->mass, desc->mass);
        elem->GetVec3(kAttr_Size, &desc->size.x, &desc->size.x);
        elem->GetVec3(kAttr_Offset, &desc->offset.x, &desc->offset.x);
        elem->GetBool(kAttr_Enabled, &desc->enabled, desc->enabled);
        elem->GetBool(kAttr_Physics91, &desc->flag91, desc->flag91);
    }
    return desc;
}

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

// @func 0x00291AD0 ParseIKComponentDesc
ComponentDesc* ParseIKComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    IKComponentDesc* desc = NewNamedDesc<IKComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    ReadChildNames(elem, kTag_JointTwoSolver, desc->jointTwoSolvers, 8);
    ReadChildNames(elem, kTag_JointThreeSolver, desc->jointThreeSolvers, 8);
    ReadChildNames(elem, kTag_FootSolver, desc->footSolvers, 2);
    if (XmlElement* lookAt = elem->FirstChildElement(kTag_LookAtSolver)) {
        uint32_t value = 0;
        lookAt->GetHash(kAttr_Name, &value, 0);
        desc->lookAtSolver = value;
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

// @func 0x002BA548 ParseOffscreenMarkerComponentDesc
ComponentDesc* ParseOffscreenMarkerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    OffscreenMarkerComponentDesc* desc = new OffscreenMarkerComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, true);
    elem->GetHash(kAttr_Marker14, &desc->hash14, desc->hash14);
    elem->GetHash(kAttr_Marker18, &desc->hash18, desc->hash18);
    elem->GetFloat(kAttr_MarkerRange, &desc->rangeSq, desc->rangeSq);
    desc->hasOnscreenColor =
        elem->GetColor(kAttr_OnscreenColor, &desc->onscreenColor, &desc->onscreenColor) == ape::xml::kQuery_Ok;
    desc->hasOffscreenColor =
        elem->GetColor(kAttr_OffscreenColor, &desc->offscreenColor, &desc->offscreenColor) == ape::xml::kQuery_Ok;
    elem->GetBool(kAttr_Marker28, &desc->flag28, false);
    if (desc->rangeSq > 0.0f)
        desc->rangeSq *= desc->rangeSq;
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

// @func 0x002ACBF8 ParseBuoyancyComponentDesc
ComponentDesc* ParseBuoyancyComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    BuoyancyComponentDesc* desc = NewNamedDesc<BuoyancyComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_MaxDepth, &desc->maxDepth, desc->maxDepth);
        elem->GetFloat(kAttr_FloatSpeed, &desc->floatSpeed, desc->floatSpeed);
        elem->GetFloat(kAttr_SinkSpeed, &desc->sinkSpeed, desc->sinkSpeed);
        elem->GetFloat(kAttr_Buoyancy20, &desc->value20, desc->value20);
        elem->GetFloat(kAttr_Buoyancy40, &desc->value40, desc->value40);
        elem->GetFloat(kAttr_Buoyancy3C, &desc->value3C, desc->value3C);
        elem->GetFloat(kAttr_Buoyancy24, &desc->value24, desc->value24);
        elem->GetFloat(kAttr_Buoyancy44, &desc->value44, desc->value44);
        elem->GetFloat(kAttr_Buoyancy28, &desc->value28, desc->value28);
        elem->GetFloat(kAttr_MaxRotationX, &desc->maxRotationX, desc->maxRotationX);
        elem->GetFloat(kAttr_MaxRotationY, &desc->maxRotationY, desc->maxRotationY);
        elem->GetFloat(kAttr_MaxRotationZ, &desc->maxRotationZ, desc->maxRotationZ);
        elem->GetFloat(kAttr_TiltSpeed, &desc->tiltSpeed, desc->tiltSpeed);
        elem->GetFloat(kAttr_Buoyancy4C, &desc->value4C, desc->value4C);
        elem->GetHash(kAttr_ContactScript, &desc->contactScript, desc->contactScript);
        elem->GetFloat(kAttr_ContactInterval, &desc->contactInterval, desc->contactInterval);
    }
    return desc;
}

// @func 0x002CC470 ParseWallClimbComponentDesc
ComponentDesc* ParseWallClimbComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    WallClimbComponentDesc* desc = NewNamedDesc<WallClimbComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_GrabAngleH, &desc->grabAngleH, desc->grabAngleH);
        elem->GetFloat(kAttr_MaxAngleV, &desc->maxAngleV, desc->maxAngleV);
        elem->GetFloat(kAttr_MaxAngleH, &desc->maxAngleH, desc->maxAngleH);
        desc->grabAngleH *= kDegToRad;
        desc->maxAngleV *= kDegToRad;
        desc->maxAngleH *= kDegToRad;
        elem->GetFloat(kAttr_GrabDistance, &desc->grabDistance, desc->grabDistance);
        elem->GetFloat(kAttr_MaxDistance, &desc->maxDistance, desc->maxDistance);
        elem->GetFloat(kAttr_StickThreshold, &desc->stickThreshold, desc->stickThreshold);
        elem->GetFloat(kAttr_WallClimb50, &desc->value50, desc->value50);
        elem->GetFloat(kAttr_WallClimb54, &desc->value54, desc->value54);
        elem->GetFloat(kAttr_WallClimb58, &desc->value58, desc->value58);
        elem->GetFloat(kAttr_WallClimb60, &desc->value60, desc->value60);
    }
    return desc;
}

// @func 0x002B44AC ParseMonsterClosetComponentDesc
ComponentDesc* ParseMonsterClosetComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    MonsterClosetComponentDesc* desc = NewNamedDesc<MonsterClosetComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    for (XmlElement* child = elem->FirstChildElement(kTag_ClosetSpawn); child;
         child = child->NextSiblingElement(kTag_ClosetSpawn)) {
        MonsterClosetSpawn spawn;
        uint32_t typeName = 0;
        child->GetHash(kAttr_Def, &spawn.def, 0);
        child->GetHash(kAttr_SpawnPoint, &spawn.spawnPoint, spawn.spawnPoint);
        child->GetFloat(kAttr_RespawnCooldown, &spawn.respawnCooldown, spawn.respawnCooldown);
        child->GetBool(kAttr_RespawnEnabled, &spawn.respawnEnabled, spawn.respawnEnabled);
        child->GetHash(kAttr_Type, &typeName, 0);
        // No early exit: the last matching kind wins; unknown kinds become 0.
        uint32_t type = 0;
        for (uint32_t i = 0; i < 2; ++i)
            if (typeName == kClosetSpawnTypes[i])
                type = i;
        spawn.type = type;
        if (spawn.def != 0)
            desc->spawns.PushBack(spawn);
    }
    elem->GetFloat(kAttr_RespawnCooldown, &desc->respawnCooldown, desc->respawnCooldown);
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
