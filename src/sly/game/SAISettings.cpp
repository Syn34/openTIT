// openTIT - Sly setting definitions and the game's setting manager (0x0043B450-0x0043EB48).
#include "sly/game/SAISettings.h"

#include "ape/core/ApeString.h"

#include <algorithm>
#include <vector>

namespace sly {

using ape::DTDValue;
using ape::kDTD_Bool;
using ape::kDTD_Float;
using ape::kDTD_Hash;
using ape::kDTD_Int;
using ape::kDTD_Int1;
using ape::kDTD_String;
using ape::setting::kParam_Optional;
using ape::setting::SettingBool;
using ape::setting::SettingFloat;
using ape::setting::SettingHash;
using ape::setting::SettingInt;
using ape::setting::SettingString;

SAIAssociationHooks g_saiAssociationHooks;
SAIExternalDefs g_saiExternalDefs;
SAISettingManager* g_saiSettingManager = nullptr;

namespace {

// Compile-time HashNameNoCase().
constexpr uint32_t NameHash(const char* name)
{
    uint32_t hash = 0;
    for (; *name; ++name) {
        int c = (signed char)*name;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}

// Value names of the enumerated parameters, with their hashes
// (0x00F6579C-0x00F657FC, 0x00F65924).
const char* const kSplineMoveTypeNames[3] = { "Fixed", "Loop", "PingPong" };
const char* const kSplineSpeedTypeNames[3] = { "PercentPerSec", "MeterPerSec", "TimeFromAtoB" };
const char* const kLaserDamageTypeNames[2] = { "Fire", "Electric" };
const char* const kDamagePhantomLayerNames[2] = { "OnlyPlayer", "AllCharacters" };
const char* const kLaserInitTypeNames[2] = { "None", "Flicker" };
const char* const kSentryMovementTypeNames[2] = { "pingpong", "linear" };
const char* const kSpawnModeNames[3] = { "SpawnAtBegining", "SpawnBaseOnTrigger", "SpawnBaseOnScript" };

// Shorthands for the descriptor table below. Every SAI parameter is optional.
DTDValue Int(int32_t v) { return SettingInt(v); }
DTDValue Hash(const char* name) { return SettingHash(NameHash(name)); }
DTDValue Float(float v) { return SettingFloat(v); }
DTDValue Bool(bool v) { return SettingBool(v); }
DTDValue Str(const char* v) { return SettingString(v); }

void AddEnumNames(ape::setting::SettingParam& param, const char* const* names, uint32_t count)
{
    param.attribute->AllocEnumNames(count);
    for (uint32_t i = 0; i < count; ++i)
        param.attribute->AddEnumName(names[i]);
}

template <size_t N>
void AddEnumNames(ape::setting::SettingParam& param, const char* const (&names)[N])
{
    AddEnumNames(param, names, (uint32_t)N);
}

} // namespace

// ---------------------------------------------------------------------------
// Parameter descriptors
// ---------------------------------------------------------------------------

// The descriptors are constructed by the module's static initialiser and their
// vtables reset by its static destructor; both are the C++ globals below.
// @func 0x0043EB48 SAISettingParams static initialiser
// @func 0x0043E514 SAISettingParams static destructor
namespace sai_params {

#define SAI_PARAM(var, name, type, def) SettingParam var(name, type, kParam_Optional, def)

SAI_PARAM(LayerNum, "LayerNum", kDTD_Int1, Int(0));
SAI_PARAM(LinkType, "LinkType", kDTD_Hash, Hash("PointLink"));
SAI_PARAM(Heavy, "Heavy", kDTD_Bool, Bool(false));
SAI_PARAM(Light, "Light", kDTD_Bool, Bool(true));
SAI_PARAM(Boss, "Boss", kDTD_Bool, Bool(false));
SAI_PARAM(Player, "Player", kDTD_Bool, Bool(true));
SAI_PARAM(Compass, "Compass", kDTD_Bool, Bool(false));
SAI_PARAM(MayUseDist, "MayUseDist", kDTD_Float, Float(1.0f));
SAI_PARAM(MaxSnapDist, "MaxSnapDist", kDTD_Float, Float(10.0f));
SAI_PARAM(Walk, "Walk", kDTD_Bool, Bool(false));
SAI_PARAM(PlatformJumpDistXZ, "PlatformJumpDistXZ", kDTD_Float, Float(0.0f));
SAI_PARAM(PlatformJumpHeight, "PlatformJumpHeight", kDTD_Float, Float(0.0f));
SAI_PARAM(PlatformFallHeight, "PlatformFallHeight", kDTD_Float, Float(0.0f));
SAI_PARAM(Object1, "Object1", kDTD_String, Str(nullptr));
SAI_PARAM(Object2, "Object2", kDTD_String, Str(nullptr));
SAI_PARAM(Object3, "Object3", kDTD_String, Str(nullptr));
SAI_PARAM(Object4, "Object4", kDTD_String, Str(nullptr));
SAI_PARAM(Object5, "Object5", kDTD_String, Str(nullptr));
SAI_PARAM(Object6, "Object6", kDTD_String, Str(nullptr));

SAI_PARAM(ProxyAnim, "Anim", kDTD_String, Str(nullptr));
SAI_PARAM(ProxyScript, "Script", kDTD_String, Str(nullptr));
SAI_PARAM(PathType, "PathType", kDTD_Hash, Hash("Loop"));
SAI_PARAM(Filter, "Filter", kDTD_Hash, Hash("Any"));
SAI_PARAM(Reversible, "Reversible", kDTD_Bool, Bool(true));

SAI_PARAM(SpawnObjName, "SpawnObjName", kDTD_String, Str(nullptr));
SAI_PARAM(SpawnEffectName, "SpawnEffectName", kDTD_String, Str(nullptr));
SAI_PARAM(SpawnDelayTime, "SpawnDelayTime", kDTD_Float, Float(0.0f));
SAI_PARAM(RandomSpawnTime, "RandomSpawnTime", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnProbability, "SpawnProbability", kDTD_Float, Float(100.0f));
SAI_PARAM(NumOfSpawnBurst, "NumOfSpawnBurst", kDTD_Int1, Int(1));
SAI_PARAM(LimitSpawnCount, "LimitSpawnCount", kDTD_Int1, Int(1));
SAI_PARAM(RespawnDelayTime, "RespawnDelayTime", kDTD_Float, Float(0.5f));
SAI_PARAM(MovementBoxName, "MovementBoxName", kDTD_String, Str(nullptr));
SAI_PARAM(SpawnMode, "SpawnMode", kDTD_Hash, Hash("SpawnAtBegining"));
SAI_PARAM(IsSpawnField, "IsSpawnField", kDTD_Bool, Bool(false));
SAI_PARAM(UseSpawnFieldCenterPlane, "UseSpawnFieldCenterPlane", kDTD_Bool, Bool(false));
SAI_PARAM(UseSpawnerGroupSpawnField, "UseSpawnerGroupSpawnField", kDTD_Bool, Bool(false));
SAI_PARAM(IsKeepRespawn, "IsKeepRespawn", kDTD_Bool, Bool(false));
SAI_PARAM(NoCull, "NoCull", kDTD_Bool, Bool(false));
SAI_PARAM(SpawnRadius, "SpawnRadius", kDTD_Float, Float(0.0f));
SAI_PARAM(NoSpawnRadius, "NoSpawnRadius", kDTD_Float, Float(0.0f));
SAI_PARAM(ForceUseSpawnerNameAsObjName, "ForceUseSpawnerNameAsObjName", kDTD_Bool, Bool(false));
SAI_PARAM(TrackSpawnedObjects, "TrackSpawnedObjects", kDTD_Bool, Bool(true));
SAI_PARAM(SpawnSpeed, "SpawnSpeed", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnSpeedVariance, "SpawnSpeedVariance", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityX, "SpawnAngularVelocityX", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityY, "SpawnAngularVelocityY", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityZ, "SpawnAngularVelocityZ", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityVarianceX, "SpawnAngularVelocityVarianceX", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityVarianceY, "SpawnAngularVelocityVarianceY", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnAngularVelocityVarianceZ, "SpawnAngularVelocityVarianceZ", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnScatterRangeX, "SpawnScatterRangeX", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnScatterRangeY, "SpawnScatterRangeY", kDTD_Float, Float(0.0f));
SAI_PARAM(SpawnScatterRangeZ, "SpawnScatterRangeZ", kDTD_Float, Float(0.0f));
SAI_PARAM(LockToWorldY, "LockToWorldY", kDTD_Bool, Bool(false));
SAI_PARAM(LinkedObject, "LinkedObject", kDTD_String, Str(""));
SAI_PARAM(SpawnerGroup, "SpawnerGroup", kDTD_String, Str(""));

SAI_PARAM(EntityDefName, "EntityDefName", kDTD_String, Str(nullptr));
SAI_PARAM(SettingName, "SettingName", kDTD_String, Str(nullptr));
SAI_PARAM(SettingNameAlt1, "SettingNameAlt1", kDTD_String, Str(nullptr));
SAI_PARAM(SettingNameAlt2, "SettingNameAlt2", kDTD_String, Str(nullptr));
SAI_PARAM(EntitySpawnerPad, "pad", kDTD_Float, Float(0.0f));

SAI_PARAM(SpawnAISettingName, "SpawnAISettingName", kDTD_String, Str(nullptr));
SAI_PARAM(AISettingNameAlt1, "AISettingNameAlt1", kDTD_String, Str(nullptr));
SAI_PARAM(AISettingNameAlt2, "AISettingNameAlt2", kDTD_String, Str(nullptr));
// (sic) "Partrol" is the attribute's spelling in the data.
SAI_PARAM(PatrolProxyPointName, "PartrolProxyPointName", kDTD_String, Str(nullptr));
SAI_PARAM(ProjectorType, "ProjectorType", kDTD_Hash, SettingHash(0));

SAI_PARAM(ShakeDuration, "ShakeDuration", kDTD_Float, Float(2.0f));
SettingVec3Param ShakeMinFreq("ShakeMinFreq", kParam_Optional, 1.0f, 1.0f, 1.0f);
SettingVec3Param ShakeMaxFreq("ShakeMaxFreq", kParam_Optional, 2.0f, 2.0f, 2.0f);
SettingVec3Param ShakeMinDistance("ShakeMinDistance", kParam_Optional, 0.08f, 0.04f, 0.15f);
SettingVec3Param ShakeMaxDistance("ShakeMaxDistance", kParam_Optional, 0.015f, 0.09f, 0.35f);

SAI_PARAM(NumOfSide, "NumOfSide", kDTD_Int1, Int(12));
// NOTE: the defaults are copied from the engine colour globals 0x01164CA0 and
// 0x01164CAC, which are filled at start-up elsewhere; their values are not
// known here (0x01164CA0 is assumed to be the default object colour).
SettingColorParam Color("Color", kParam_Optional, 0x80, 0x80, 0x80, 0x80);
SettingColorParam AlertColor("AlertColor", kParam_Optional, 0, 0, 0, 0);

SAI_PARAM(GroupActivationTrigger, "ActivationTrigger", kDTD_String, Str(nullptr));
SAI_PARAM(GroupStartEnabled, "StartEnabled", kDTD_Bool, Bool(true));
SAI_PARAM(GroupNavObstacle, "NavObstacle", kDTD_Bool, Bool(true));

#define SAI_LASER_COMMON                                                                                       \
    {                                                                                                          \
        { "LaserGeomDef", kDTD_String, kParam_Optional, Str(nullptr) },                                        \
            { "LaserDamageType", kDTD_Hash, kParam_Optional, Hash("Fire") },                                   \
            { "LaserDamageAmount", kDTD_Float, kParam_Optional, Float(10.0f) },                                \
            { "InitType", kDTD_Hash, kParam_Optional, Hash("None") },                                          \
            { "AlwaysShowEndPoints", kDTD_Bool, kParam_Optional, Bool(false) },                                \
            { "LaserSound", kDTD_String, kParam_Optional, Str(nullptr) },                                      \
    }

LaserCommonParams LaserFixedCommon = SAI_LASER_COMMON;
SAI_PARAM(Point1DirScript, "Point1DirScript", kDTD_String, Str(nullptr));
SAI_PARAM(Point2DirScript, "Point2DirScript", kDTD_String, Str(nullptr));
SAI_PARAM(FixedMoveableSpline, "MoveableSpline", kDTD_String, Str(nullptr));
SAI_PARAM(MoveableObj1, "MoveableObj1", kDTD_String, Str(nullptr));
SAI_PARAM(MoveableObj2, "MoveableObj2", kDTD_String, Str(nullptr));
SAI_PARAM(FixedStartEnabled, "StartEnabled", kDTD_Bool, Bool(true));

LaserCommonParams LaserAnchorCommon = SAI_LASER_COMMON;
SAI_PARAM(PointAnchorDirScript, "PointAnchorDirScript", kDTD_String, Str(nullptr));
SAI_PARAM(MoveableAnchorObj, "MoveableAnchorObj", kDTD_String, Str(nullptr));
LaserSplineMoveParams AnchorMove = {
    { "SplineStartOffset", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineStartOffsetVariance", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineMoveType", kDTD_Hash, kParam_Optional, Hash("Fixed") },
    { "SplineSpeedType", kDTD_Hash, kParam_Optional, Hash("PercentPerSec") },
    { "SplineMoveSpeed", kDTD_Float, kParam_Optional, Float(1.0f) },
    { "SplineMoveSpeedVariance", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "PointSplineDirScript", kDTD_String, kParam_Optional, Str(nullptr) },
    { "MoveableSpline", kDTD_String, kParam_Optional, Str(nullptr) },
};
SAI_PARAM(AnchorStartEnabled, "StartEnabled", kDTD_Bool, Bool(true));

LaserCommonParams LaserSplineCommon = SAI_LASER_COMMON;
LaserSplineMoveParams SplineMove1 = {
    { "SplineStartOffset1", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineStartOffsetVariance1", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineMoveType1", kDTD_Hash, kParam_Optional, Hash("Fixed") },
    { "SplineSpeedType1", kDTD_Hash, kParam_Optional, Hash("PercentPerSec") },
    { "SplineMoveSpeed1", kDTD_Float, kParam_Optional, Float(1.0f) },
    { "SplineMoveSpeedVariance1", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "PointSpline1DirScript", kDTD_String, kParam_Optional, Str(nullptr) },
    { "MoveableSpline1", kDTD_String, kParam_Optional, Str(nullptr) },
};
LaserSplineMoveParams SplineMove2 = {
    { "SplineStartOffset2", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineStartOffsetVariance2", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "SplineMoveType2", kDTD_Hash, kParam_Optional, Hash("Fixed") },
    { "SplineSpeedType2", kDTD_Hash, kParam_Optional, Hash("PercentPerSec") },
    { "SplineMoveSpeed2", kDTD_Float, kParam_Optional, Float(1.0f) },
    { "SplineMoveSpeedVariance2", kDTD_Float, kParam_Optional, Float(0.0f) },
    { "PointSpline2DirScript", kDTD_String, kParam_Optional, Str(nullptr) },
    { "MoveableSpline2", kDTD_String, kParam_Optional, Str(nullptr) },
};
// NOTE: only registered with the second movement block, despite the "1".
SAI_PARAM(UseSplineStartOffsetVariance1, "UseSplineStartOffsetVariance1", kDTD_Bool, Bool(false));
SAI_PARAM(UseSplineMoveSpeedVariance1, "UseSplineMoveSpeedVariance1", kDTD_Bool, Bool(false));
SAI_PARAM(SplineStartEnabled, "StartEnabled", kDTD_Bool, Bool(true));

SAI_PARAM(OutlineGeomDef, "OutlineGeomDef", kDTD_String, Str(nullptr));
SAI_PARAM(FieldDamageType, "LaserDamageType", kDTD_Hash, Hash("Fire"));
SAI_PARAM(DamagePhantomLayer, "DamagePhantomLayer", kDTD_Hash, Hash("OnlyPlayer"));
SAI_PARAM(FieldInitType, "InitType", kDTD_Hash, Hash("None"));
SAI_PARAM(JointEntityDef, "JointEntityDef", kDTD_String, Str(nullptr));
SAI_PARAM(JointYOffset, "JointYOffset", kDTD_Float, Float(0.0f));
SAI_PARAM(LifetimeScript, "LifetimeScript", kDTD_String, Str(nullptr));
SAI_PARAM(TriggerFXDirScript, "TriggerFXDirScript", kDTD_String, Str(nullptr));
SAI_PARAM(VertexColorR, "VertexColorR", kDTD_Int1, Int(0));
SAI_PARAM(VertexColorG, "VertexColorG", kDTD_Int1, Int(0));
SAI_PARAM(VertexColorB, "VertexColorB", kDTD_Int1, Int(0));
SAI_PARAM(VertexColorA, "VertexColorA", kDTD_Int1, Int(0));
SAI_PARAM(FieldActivationTrigger, "ActivationTrigger", kDTD_String, Str(nullptr));
SAI_PARAM(DamageAmount, "DamageAmount", kDTD_Float, Float(10.0f));
SAI_PARAM(DamageAmountPerSec, "DamageAmountPerSec", kDTD_Float, Float(0.0f));
SAI_PARAM(FieldStartEnabled, "StartEnabled", kDTD_Bool, Bool(true));

namespace {
const char* const kMaterialNameNames[3] = { "MaterialName1", "MaterialName2", "MaterialName3" };
const char* const kMaterialUDistNames[3] = { "MaterialUDist1", "MaterialUDist2", "MaterialUDist3" };
const char* const kMaterialVDistNames[3] = { "MaterialVDist1", "MaterialVDist2", "MaterialVDist3" };
const float kMaterialDistDefaults[3] = { 0.0f, 1.0f, 0.0f };
int g_materialParamCount = 0;   // 0x00F65920
} // namespace

LaserFieldMaterialParams::LaserFieldMaterialParams()
    : name(kMaterialNameNames[g_materialParamCount], kDTD_String, kParam_Optional, Str(nullptr)),
      uDist(kMaterialUDistNames[g_materialParamCount], kDTD_Float, kParam_Optional,
            Float(kMaterialDistDefaults[g_materialParamCount])),
      vDist(kMaterialVDistNames[g_materialParamCount], kDTD_Float, kParam_Optional,
            Float(kMaterialDistDefaults[g_materialParamCount]))
{
    ++g_materialParamCount;
}
LaserFieldMaterialParams LaserFieldMaterials[3];

SAI_PARAM(GroupType, "GroupType", kDTD_Hash, SettingHash(0));
SAI_PARAM(GroupLimitSpawnCount, "LimitSpawnCount", kDTD_Int, Int(0));
SAI_PARAM(MaxSpawns, "MaxSpawns", kDTD_Int, Int(0));
SAI_PARAM(MinHeavies, "MinHeavies", kDTD_Int, Int(0));
SAI_PARAM(MinLights, "MinLights", kDTD_Int, Int(0));
SAI_PARAM(MinDistFromPlayer, "MinDistFromPlayer", kDTD_Float, Float(0.0f));
SAI_PARAM(GroupSpawnMode, "SpawnMode", kDTD_Hash, Hash("SpawnAtBegining"));

SAI_PARAM(TeleporterDef, "TeleporterDef", kDTD_String, Str(nullptr));
SAI_PARAM(DestControllable, "DestControllable", kDTD_String, Str(nullptr));
SAI_PARAM(DestObj, "DestObj", kDTD_String, Str(nullptr));
SAI_PARAM(DestLevel, "DestLevel", kDTD_String, Str(nullptr));
SAI_PARAM(DestLevelSpawnPoint, "DestLevelSpawnPoint", kDTD_String, Str(nullptr));
SAI_PARAM(SpawnOnCreate, "SpawnOnCreate", kDTD_Bool, Bool(true));
SAI_PARAM(NoTeleport, "NoTeleport", kDTD_Bool, Bool(false));
SAI_PARAM(DestControllableCurrentPlayer, "DestControllableCurrentPlayer", kDTD_Bool, Bool(false));
SAI_PARAM(AlwaysHideContextual, "AlwaysHideContextual", kDTD_Bool, Bool(false));

SAI_PARAM(SentryActivationTrigger, "ActivationTrigger", kDTD_String, Str(nullptr));
SAI_PARAM(SetInvisibleWhenDisabled, "SetInvisibleWhenDisabled", kDTD_Bool, Bool(false));
SAI_PARAM(SentryLightEntityDef, "SentryLightEntityDef", kDTD_String, Str("SpotLightGunA"));
SAI_PARAM(SentryLightHalfAngle, "SentryLightHalfAngle", kDTD_Float, Float(20.0f));
SAI_PARAM(SentryLightLength, "SentryLightLength", kDTD_Float, Float(13.0f));
SAI_PARAM(UniversalForwardMoveSpeed, "UniversalForwardMoveSpeed", kDTD_Float, Float(0.0f));
SAI_PARAM(UniversalReverseMoveSpeed, "UniversalReverseMoveSpeed", kDTD_Float, Float(0.0f));
SAI_PARAM(UniversalForwardWaitTime, "UniversalForwardWaitTime", kDTD_Float, Float(0.0f));
SAI_PARAM(UniversalReverseWaitTime, "UniversalReverseWaitTime", kDTD_Float, Float(0.0f));
SAI_PARAM(MovementType, "MovementType", kDTD_Hash, Hash("pingpong"));
SAI_PARAM(StartSeqPointIndex, "StartSeqPointIndex", kDTD_Int1, Int(1));
SAI_PARAM(StartSeqReversed, "StartSeqReversed", kDTD_Bool, Bool(false));

#undef SAI_PARAM
#undef SAI_LASER_COMMON

namespace {
#define SAI_SIXTEEN(prefix)                                                                                    \
    {                                                                                                          \
        prefix "1", prefix "2", prefix "3", prefix "4", prefix "5", prefix "6", prefix "7", prefix "8",        \
            prefix "9", prefix "10", prefix "11", prefix "12", prefix "13", prefix "14", prefix "15",          \
            prefix "16"                                                                                        \
    }
// 0x00F6592C-0x00F65AA8
const char* const kSplinePointIdxNames[16] = SAI_SIXTEEN("splinePointIdx");
const char* const kForwardMoveSpeedNames[16] = SAI_SIXTEEN("forwardMoveSpeed");
const char* const kReverseMoveSpeedNames[16] = SAI_SIXTEEN("reverseMoveSpeed");
const char* const kForwardWaitTimeNames[16] = SAI_SIXTEEN("forwardWaitTime");
const char* const kReverseWaitTimeNames[16] = SAI_SIXTEEN("reverseWaitTime");
const char* const kIsEnabledNames[16] = SAI_SIXTEEN("isEnabled");
#undef SAI_SIXTEEN
int g_sentryPointCount = 0;   // 0x00F65AAC
} // namespace

// @func 0x0043E040 SentryLightPointParams::SentryLightPointParams
SentryLightPointParams::SentryLightPointParams()
    : splinePointIdx(kSplinePointIdxNames[g_sentryPointCount], kDTD_Int1, kParam_Optional,
                     Int(g_sentryPointCount + 1)),
      forwardMoveSpeed(kForwardMoveSpeedNames[g_sentryPointCount], kDTD_Float, kParam_Optional, Float(0.5f)),
      reverseMoveSpeed(kReverseMoveSpeedNames[g_sentryPointCount], kDTD_Float, kParam_Optional, Float(0.5f)),
      forwardWaitTime(kForwardWaitTimeNames[g_sentryPointCount], kDTD_Float, kParam_Optional, Float(0.0f)),
      reverseWaitTime(kReverseWaitTimeNames[g_sentryPointCount], kDTD_Float, kParam_Optional, Float(0.0f)),
      // Only the first point starts enabled.
      isEnabled(kIsEnabledNames[g_sentryPointCount], kDTD_Bool, kParam_Optional, Bool(g_sentryPointCount == 0))
{
    ++g_sentryPointCount;
}
SentryLightPointParams SentryLightPoints[16];

} // namespace sai_params

// ---------------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------------

namespace {

using namespace sai_params;

// Association trampolines: the game systems install their callbacks in
// g_saiAssociationHooks at any time.
#define SAI_ASSOCIATION(fn, hook)                                                                              \
    void fn(Setting* setting, const char* value, bool reload)                                                  \
    {                                                                                                          \
        if (g_saiAssociationHooks.hook)                                                                        \
            g_saiAssociationHooks.hook(setting, value, reload);                                                \
    }
SAI_ASSOCIATION(AssociateNavJumpLink, navJumpLink)
SAI_ASSOCIATION(AssociateProxyPoint, proxyPoint)
SAI_ASSOCIATION(AssociateEntitySpawner, entitySpawner)
SAI_ASSOCIATION(AssociateAISpawner, aiSpawner)
SAI_ASSOCIATION(AssociateLaserGroup, laserGroup)
SAI_ASSOCIATION(AssociateLaser, laser)
SAI_ASSOCIATION(AssociateLaserField, laserField)
SAI_ASSOCIATION(AssociateSpawnerGroup, spawnerGroup)
SAI_ASSOCIATION(AssociateTeleporter, teleporter)
SAI_ASSOCIATION(AssociateSentryLight, sentryLight)
#undef SAI_ASSOCIATION

// Element name prefix and association callback, set at the end of InitParams.
void FinishDef(SettingDef& def, const char* prefix, SettingAssociationFn association)
{
    def.dtd->SetNamePrefix(prefix);
    def.association = association;
}

void AddLaserCommon(SettingDef& def, LaserCommonParams& p)
{
    def.AddParam(p.geomDef);
    def.AddParam(p.damageType);
    AddEnumNames(p.damageType, kLaserDamageTypeNames);
    def.AddParam(p.damageAmount);
    def.AddParam(p.initType);
    AddEnumNames(p.initType, kLaserInitTypeNames);
    def.AddParam(p.alwaysShowEndPoints);
    def.AddParam(p.sound);
}

// NOTE: on the console each definition creates a setting subclass with its own
// vtable (0x00B214A0, 0x00B214FC, ...); they add nothing to Setting, so the
// definitions here create plain Settings (the hot spot one is 4 bytes larger).

class AINavJumpLinkSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "AINavJumpLinkSetting"; }
    uint32_t GetTypeHash() const override { return 0xD3124ACD; }

    // @func 0x0043B450 AINavJumpLinkSettingDef::InitParams
    void InitParams() override
    {
        static const char* const kLinkTypeNames[3] = { "PointLink", "StripLink", "PlatformLink" };
        AddParam(LayerNum);
        AddParam(LinkType);
        AddEnumNames(LinkType, kLinkTypeNames);
        for (SettingParam* p : { &Heavy, &Light, &Boss, &Player, &Compass, &MayUseDist, &MaxSnapDist, &Walk,
                                 &PlatformJumpDistXZ, &PlatformJumpHeight, &PlatformFallHeight, &Object1, &Object2,
                                 &Object3, &Object4, &Object5, &Object6 })
            AddParam(*p);
        FinishDef(*this, "SAINavJumpLink_", &AssociateNavJumpLink);
    }

    // @func 0x0043B778 AINavJumpLinkSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("AINavJumpLinkSetting") == 0xD3124ACD, "type hash");

class AIProxyPointSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_AIProxyPoint"; }
    uint32_t GetTypeHash() const override { return 0xF066BEC9; }

    // @func 0x0043B800 AIProxyPointSettingDef::InitParams
    void InitParams() override
    {
        static const char* const kPathTypeNames[3] = { "Loop", "PingPong", "Random" };
        static const char* const kFilterNames[4] = { "Any", "Heavy", "Light", "Special" };
        AddParam(ProxyAnim);
        AddParam(ProxyScript);
        AddParam(PathType);
        AddEnumNames(PathType, kPathTypeNames);
        AddParam(Filter);
        AddEnumNames(Filter, kFilterNames);
        AddParam(Reversible);
        FinishDef(*this, "SAIProxyPt_", &AssociateProxyPoint);
    }

    // @func 0x0043BBD0 AIProxyPointSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_AIProxyPoint") == 0xF066BEC9, "type hash");

// @func 0x0043BC58 AddSpawnerParams
// The parameters both spawner definitions end with.
void AddSpawnerParams(SettingDef& def)
{
    for (SettingParam* p : { &SpawnObjName, &SpawnEffectName, &SpawnDelayTime, &RandomSpawnTime, &SpawnProbability,
                             &NumOfSpawnBurst, &LimitSpawnCount, &RespawnDelayTime, &MovementBoxName, &SpawnMode })
        def.AddParam(*p);
    AddEnumNames(SpawnMode, kSpawnModeNames);
    for (SettingParam* p :
         { &IsSpawnField, &UseSpawnFieldCenterPlane, &UseSpawnerGroupSpawnField, &IsKeepRespawn, &NoCull,
           &SpawnRadius, &NoSpawnRadius, &ForceUseSpawnerNameAsObjName, &TrackSpawnedObjects, &SpawnSpeed,
           &SpawnSpeedVariance, &SpawnAngularVelocityX, &SpawnAngularVelocityY, &SpawnAngularVelocityZ,
           &SpawnAngularVelocityVarianceX, &SpawnAngularVelocityVarianceY, &SpawnAngularVelocityVarianceZ,
           &SpawnScatterRangeX, &SpawnScatterRangeY, &SpawnScatterRangeZ, &LockToWorldY, &LinkedObject,
           &SpawnerGroup })
        def.AddParam(*p);
}

class EntitySpawnerSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_EntitySpawner"; }
    uint32_t GetTypeHash() const override { return 0xC9064726; }

    // @func 0x0043C070 EntitySpawnerSettingDef::InitParams
    void InitParams() override
    {
        for (SettingParam* p : { &EntityDefName, &SettingName, &SettingNameAlt1, &SettingNameAlt2, &EntitySpawnerPad })
            AddParam(*p);
        AddSpawnerParams(*this);
        FinishDef(*this, "SEntitySpawner_", &AssociateEntitySpawner);
    }

    // @func 0x0043C128 EntitySpawnerSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_EntitySpawner") == 0xC9064726, "type hash");

class AISpawnerSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_AISpawner"; }
    uint32_t GetTypeHash() const override { return 0xFF76E7C1; }

    // @func 0x0043C1B0 AISpawnerSettingDef::InitParams
    void InitParams() override
    {
        static const char* const kProjectorTypeNames[2] = { "Simple", "HighQuality" };
        for (SettingParam* p :
             { &SpawnAISettingName, &AISettingNameAlt1, &AISettingNameAlt2, &PatrolProxyPointName, &ProjectorType })
            AddParam(*p);
        AddEnumNames(ProjectorType, kProjectorTypeNames);
        AddSpawnerParams(*this);
        FinishDef(*this, "SAISpawner_", &AssociateAISpawner);
    }

    // @func 0x0043C35C AISpawnerSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_AISpawner") == 0xFF76E7C1, "type hash");

class CameraShakeSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "CameraShakeSetting"; }
    uint32_t GetTypeHash() const override { return 0x5D1AFC2F; }

    // @func 0x0043C3E4 CameraShakeSettingDef::InitParams
    void InitParams() override
    {
        AddParam(ShakeDuration);
        AddParam(ShakeMinFreq);
        AddParam(ShakeMaxFreq);
        AddParam(ShakeMinDistance);
        AddParam(ShakeMaxDistance);
    }

    // @func 0x0043C46C CameraShakeSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("CameraShakeSetting") == 0x5D1AFC2F, "type hash");

// Hot spot setting: 0x28 bytes on the console.
class HotSpotSetting final : public Setting {
public:
    using Setting::Setting;
    uint32_t field_24 = 0;   // NOTE: never initialised or read by the console code seen
};

class HotSpotSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "HotSpotSetting"; }
    uint32_t GetTypeHash() const override { return 0xBC5EDCA1; }
    uint32_t GetGroupHash() const override { return 0xC9D7446A; }

    // @func 0x0043C4F4 HotSpotSettingDef::InitParams
    void InitParams() override
    {
        AddParam(NumOfSide);
        AddParam(Color);
        AddParam(AlertColor);
    }

    // @func 0x0043C554 HotSpotSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new HotSpotSetting(this, name); }
};
static_assert(NameHash("HotSpotSetting") == 0xBC5EDCA1, "type hash");

class LaserGroupSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_LaserGroup"; }
    uint32_t GetTypeHash() const override { return 0xA2A8CB13; }

    // @func 0x0043C5DC LaserGroupSettingDef::InitParams
    void InitParams() override
    {
        AddParam(GroupActivationTrigger);
        AddParam(GroupStartEnabled);
        AddParam(GroupNavObstacle);
        FinishDef(*this, "SLaserGroup_", &AssociateLaserGroup);
    }

    // @func 0x0043C664 LaserGroupSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_LaserGroup") == 0xA2A8CB13, "type hash");

class LaserFixedSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_LaserFixed"; }
    uint32_t GetTypeHash() const override { return 0xA296BC08; }

    // @func 0x0043C6EC LaserFixedSettingDef::InitParams
    void InitParams() override
    {
        AddLaserCommon(*this, LaserFixedCommon);
        for (SettingParam* p : { &Point1DirScript, &Point2DirScript, &FixedMoveableSpline, &MoveableObj1,
                                 &MoveableObj2, &FixedStartEnabled })
            AddParam(*p);
        FinishDef(*this, "SLaserFixed_", &AssociateLaser);
    }

    // @func 0x0043C9C0 LaserFixedSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_LaserFixed") == 0xA296BC08, "type hash");

class LaserAnchorSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_LaserAnchor"; }
    uint32_t GetTypeHash() const override { return 0xA7F58081; }

    // @func 0x0043CA48 LaserAnchorSettingDef::InitParams
    void InitParams() override
    {
        AddLaserCommon(*this, LaserAnchorCommon);
        AddParam(PointAnchorDirScript);
        AddParam(MoveableAnchorObj);
        AddParam(AnchorMove.startOffset);
        AddParam(AnchorMove.startOffsetVariance);
        AddParam(AnchorMove.speedType);
        AddEnumNames(AnchorMove.speedType, kSplineSpeedTypeNames);
        AddParam(AnchorMove.moveSpeed);
        AddParam(AnchorMove.moveSpeedVariance);
        AddParam(AnchorMove.dirScript);
        AddParam(AnchorMove.moveableSpline);
        AddParam(AnchorMove.moveType);
        AddEnumNames(AnchorMove.moveType, kSplineMoveTypeNames);
        AddParam(AnchorStartEnabled);
        FinishDef(*this, "SLaserAnchor_", &AssociateLaser);
    }

    // @func 0x0043CEC0 LaserAnchorSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_LaserAnchor") == 0xA7F58081, "type hash");

class LaserSplineSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_LaserSpline"; }
    uint32_t GetTypeHash() const override { return 0xC6CD059D; }

    // @func 0x0043CF48 LaserSplineSettingDef::InitParams
    void InitParams() override
    {
        AddLaserCommon(*this, LaserSplineCommon);
        LaserSplineMoveParams* blocks[2] = { &SplineMove1, &SplineMove2 };
        for (int i = 0; i < 2; ++i) {
            LaserSplineMoveParams& move = *blocks[i];
            AddParam(move.startOffset);
            AddParam(move.startOffsetVariance);
            if (i == 1)
                AddParam(UseSplineStartOffsetVariance1);
            AddParam(move.speedType);
            AddEnumNames(move.speedType, kSplineSpeedTypeNames);
            AddParam(move.moveSpeed);
            AddParam(move.moveSpeedVariance);
            if (i == 1)
                AddParam(UseSplineMoveSpeedVariance1);
            AddParam(move.dirScript);
            AddParam(move.moveableSpline);
            AddParam(move.moveType);
            AddEnumNames(move.moveType, kSplineMoveTypeNames);
        }
        AddParam(SplineStartEnabled);
        FinishDef(*this, "SLaserSpline_", &AssociateLaser);
    }

    // @func 0x0043D480 LaserSplineSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_LaserSpline") == 0xC6CD059D, "type hash");

class LaserFieldSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_LaserField"; }
    uint32_t GetTypeHash() const override { return 0xA296758E; }

    // @func 0x0043D508 LaserFieldSettingDef::InitParams
    void InitParams() override
    {
        AddParam(OutlineGeomDef);
        AddParam(FieldDamageType);
        AddEnumNames(FieldDamageType, kLaserDamageTypeNames);
        AddParam(DamagePhantomLayer);
        AddEnumNames(DamagePhantomLayer, kDamagePhantomLayerNames);
        AddParam(FieldInitType);
        AddEnumNames(FieldInitType, kLaserInitTypeNames);
        for (SettingParam* p : { &JointEntityDef, &JointYOffset, &LifetimeScript, &TriggerFXDirScript, &VertexColorR,
                                 &VertexColorG, &VertexColorB, &VertexColorA, &FieldActivationTrigger, &DamageAmount,
                                 &DamageAmountPerSec, &FieldStartEnabled })
            AddParam(*p);
        for (LaserFieldMaterialParams& material : LaserFieldMaterials) {
            AddParam(material.name);
            AddParam(material.uDist);
            AddParam(material.vDist);
        }
        FinishDef(*this, "SLaserField_", &AssociateLaserField);
    }

    // @func 0x0043D91C LaserFieldSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_LaserField") == 0xA296758E, "type hash");

class SpawnerGroupSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_SpawnerGroup"; }
    uint32_t GetTypeHash() const override { return 0xC0C6EED6; }

    // @func 0x0043DAAC SpawnerGroupSettingDef::InitParams
    void InitParams() override
    {
        static const char* const kGroupTypeNames[3] = { "Random", "Sequential", "All" };
        AddParam(GroupType);
        AddEnumNames(GroupType, kGroupTypeNames);
        for (SettingParam* p : { &GroupLimitSpawnCount, &MaxSpawns, &MinHeavies, &MinLights, &MinDistFromPlayer,
                                 &GroupSpawnMode })
            AddParam(*p);
        AddEnumNames(GroupSpawnMode, kSpawnModeNames);
        FinishDef(*this, "SSpawnerGroup_", &AssociateSpawnerGroup);
    }

    // @func 0x0043DE30 SpawnerGroupSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_SpawnerGroup") == 0xC0C6EED6, "type hash");

class TeleporterSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_Teleporter"; }
    uint32_t GetTypeHash() const override { return 0xBBE4F197; }

    // @func 0x0043DEB8 TeleporterSettingDef::InitParams
    void InitParams() override
    {
        for (SettingParam* p : { &TeleporterDef, &DestControllable, &DestObj, &DestLevel, &DestLevelSpawnPoint,
                                 &SpawnOnCreate, &NoTeleport, &DestControllableCurrentPlayer, &AlwaysHideContextual })
            AddParam(*p);
        FinishDef(*this, "STeleporter_", &AssociateTeleporter);
    }

    // @func 0x0043DFB8 TeleporterSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_Teleporter") == 0xBBE4F197, "type hash");

class SentryLightSettingDef final : public SettingDef {
public:
    const char* GetName() const override { return "Sly_SentryLight"; }
    uint32_t GetTypeHash() const override { return 0xE51273D8; }

    // @func 0x0043E1D8 SentryLightSettingDef::InitParams
    void InitParams() override
    {
        for (SettingParam* p : { &SentryActivationTrigger, &SetInvisibleWhenDisabled, &SentryLightEntityDef,
                                 &SentryLightHalfAngle, &SentryLightLength, &UniversalForwardMoveSpeed,
                                 &UniversalReverseMoveSpeed, &UniversalForwardWaitTime, &UniversalReverseWaitTime,
                                 &StartSeqPointIndex, &StartSeqReversed, &MovementType })
            AddParam(*p);
        AddEnumNames(MovementType, kSentryMovementTypeNames);
        for (SentryLightPointParams& point : SentryLightPoints) {
            AddParam(point.splinePointIdx);
            AddParam(point.forwardMoveSpeed);
            AddParam(point.forwardWaitTime);
            AddParam(point.reverseMoveSpeed);
            AddParam(point.reverseWaitTime);
            AddParam(point.isEnabled);
        }
        FinishDef(*this, "SSentryLight_", &AssociateSentryLight);
    }

    // @func 0x0043E48C SentryLightSettingDef::CreateSetting
    Setting* CreateSetting(const char* name) override { return new Setting(this, name); }
};
static_assert(NameHash("Sly_SentryLight") == 0xE51273D8, "type hash");

template <class T>
SettingDef* CreateDef()
{
    return new T;
}

} // namespace

// ---------------------------------------------------------------------------
// SAISettingManager
// ---------------------------------------------------------------------------

namespace {
// The definition table (0x00F6588C, 18 entries; the character definitions
// come from g_saiExternalDefs).
std::vector<SettingDefFactory> BuildDefTable()
{
    std::vector<SettingDefFactory> table = {
        g_saiExternalDefs.enemy,
        &CreateDef<AIProxyPointSettingDef>,
        &CreateDef<AISpawnerSettingDef>,
        &CreateDef<AINavJumpLinkSettingDef>,
        &CreateDef<SpawnerGroupSettingDef>,
        &CreateDef<EntitySpawnerSettingDef>,
        &CreateDef<HotSpotSettingDef>,
        &CreateDef<TeleporterSettingDef>,
        &CreateDef<CameraShakeSettingDef>,
        &CreateDef<LaserGroupSettingDef>,
        &CreateDef<LaserFixedSettingDef>,
        &CreateDef<LaserAnchorSettingDef>,
        &CreateDef<LaserSplineSettingDef>,
        &CreateDef<LaserFieldSettingDef>,
        &CreateDef<SentryLightSettingDef>,
        g_saiExternalDefs.critter,
        g_saiExternalDefs.scriptedPath,
        g_saiExternalDefs.player,
    };
    table.erase(std::remove(table.begin(), table.end(), nullptr), table.end());
    return table;
}
} // namespace

// @func 0x0043D9A4 SAISettingManager::SAISettingManager
SAISettingManager::SAISettingManager(bool allocCache)
    : SAISettingManager(allocCache, BuildDefTable())
{
}

SAISettingManager::SAISettingManager(bool allocCache, const std::vector<SettingDefFactory>& defs)
    : SettingManager(defs.data(), (uint32_t)defs.size(), nullptr, 0, 0x3C00, allocCache, true)
{
    g_saiSettingManager = this;
}

// @func 0x0043DA08 SAISettingManager::~SAISettingManager
// @func 0x0043DA44 SAISettingManager::~SAISettingManager (deleting)
SAISettingManager::~SAISettingManager()
{
    g_saiSettingManager = nullptr;
}

} // namespace sly
