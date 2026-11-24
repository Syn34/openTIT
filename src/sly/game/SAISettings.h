// openTIT - Sly setting definitions and the game's setting manager (0x0043B450-0x0043EB48).
//
// The game registers fourteen definitions of its own (AI nav jump links, proxy points,
// spawners, lasers, sentry lights, teleporters, camera shakes, hot spots) plus four that
// live with the character code (EnemySetting, CritterSetting, PlayerSetting,
// ScriptedPathSetting). The parameters are static descriptors (sai_params below); a
// game system reads a setting's value through the descriptor's column.
//
// Setting elements are associated with level objects through the definition's
// association callback (tag settings). Those callbacks belong to the game systems
// (spawn manager, lasers, ...) and are reached through g_saiAssociationHooks.
#pragma once

#include "ape/setting/ApeSetting.h"

#include <vector>

namespace sly {

using ape::setting::Setting;
using ape::setting::SettingAssociationFn;
using ape::setting::SettingDef;
using ape::setting::SettingDefFactory;

// The static parameter descriptors (console globals 0x010AADB4-0x010AC8C8).
namespace sai_params {
using ape::setting::SettingColorParam;
using ape::setting::SettingParam;
using ape::setting::SettingVec3Param;

// AINavJumpLinkSetting
extern SettingParam LayerNum, LinkType, Heavy, Light, Boss, Player, Compass, MayUseDist, MaxSnapDist, Walk,
    PlatformJumpDistXZ, PlatformJumpHeight, PlatformFallHeight, Object1, Object2, Object3, Object4, Object5,
    Object6;
// Sly_AIProxyPoint
extern SettingParam ProxyAnim, ProxyScript, PathType, Filter, Reversible;
// Shared by both spawner definitions
extern SettingParam SpawnObjName, SpawnEffectName, SpawnDelayTime, RandomSpawnTime, SpawnProbability,
    NumOfSpawnBurst, LimitSpawnCount, RespawnDelayTime, MovementBoxName, SpawnMode, IsSpawnField,
    UseSpawnFieldCenterPlane, UseSpawnerGroupSpawnField, IsKeepRespawn, NoCull, SpawnRadius, NoSpawnRadius,
    ForceUseSpawnerNameAsObjName, TrackSpawnedObjects, SpawnSpeed, SpawnSpeedVariance, SpawnAngularVelocityX,
    SpawnAngularVelocityY, SpawnAngularVelocityZ, SpawnAngularVelocityVarianceX, SpawnAngularVelocityVarianceY,
    SpawnAngularVelocityVarianceZ, SpawnScatterRangeX, SpawnScatterRangeY, SpawnScatterRangeZ, LockToWorldY,
    LinkedObject, SpawnerGroup;
// Sly_EntitySpawner
extern SettingParam EntityDefName, SettingName, SettingNameAlt1, SettingNameAlt2, EntitySpawnerPad;
// Sly_AISpawner
extern SettingParam SpawnAISettingName, AISettingNameAlt1, AISettingNameAlt2, PatrolProxyPointName, ProjectorType;
// CameraShakeSetting
extern SettingParam ShakeDuration;
extern SettingVec3Param ShakeMinFreq, ShakeMaxFreq, ShakeMinDistance, ShakeMaxDistance;
// HotSpotSetting
extern SettingParam NumOfSide;
extern SettingColorParam Color, AlertColor;
// Sly_LaserGroup
extern SettingParam GroupActivationTrigger, GroupStartEnabled, GroupNavObstacle;

// The parameters every laser definition starts with.
struct LaserCommonParams {
    SettingParam geomDef, damageType, damageAmount, initType, alwaysShowEndPoints, sound;
};
// One spline movement block of the anchor and spline lasers.
struct LaserSplineMoveParams {
    SettingParam startOffset, startOffsetVariance, moveType, speedType, moveSpeed, moveSpeedVariance, dirScript,
        moveableSpline;
};
extern LaserCommonParams LaserFixedCommon, LaserAnchorCommon, LaserSplineCommon;
// Sly_LaserFixed
extern SettingParam Point1DirScript, Point2DirScript, FixedMoveableSpline, MoveableObj1, MoveableObj2,
    FixedStartEnabled;
// Sly_LaserAnchor
extern SettingParam PointAnchorDirScript, MoveableAnchorObj, AnchorStartEnabled;
extern LaserSplineMoveParams AnchorMove;
// Sly_LaserSpline
extern LaserSplineMoveParams SplineMove1, SplineMove2;
extern SettingParam UseSplineStartOffsetVariance1, UseSplineMoveSpeedVariance1, SplineStartEnabled;
// Sly_LaserField
extern SettingParam OutlineGeomDef, FieldDamageType, DamagePhantomLayer, FieldInitType, JointEntityDef,
    JointYOffset, LifetimeScript, TriggerFXDirScript, VertexColorR, VertexColorG, VertexColorB, VertexColorA,
    FieldActivationTrigger, DamageAmount, DamageAmountPerSec, FieldStartEnabled;
struct LaserFieldMaterialParams {
    LaserFieldMaterialParams();
    SettingParam name, uDist, vDist;
};
extern LaserFieldMaterialParams LaserFieldMaterials[3];
// Sly_SpawnerGroup
extern SettingParam GroupType, GroupLimitSpawnCount, MaxSpawns, MinHeavies, MinLights, MinDistFromPlayer,
    GroupSpawnMode;
// Sly_Teleporter
extern SettingParam TeleporterDef, DestControllable, DestObj, DestLevel, DestLevelSpawnPoint, SpawnOnCreate,
    NoTeleport, DestControllableCurrentPlayer, AlwaysHideContextual;
// Sly_SentryLight
extern SettingParam SentryActivationTrigger, SetInvisibleWhenDisabled, SentryLightEntityDef,
    SentryLightHalfAngle, SentryLightLength, UniversalForwardMoveSpeed, UniversalReverseMoveSpeed,
    UniversalForwardWaitTime, UniversalReverseWaitTime, MovementType, StartSeqPointIndex, StartSeqReversed;
// One sequence point of a sentry light (0x90 bytes); each instance takes the
// next point number (1-16) when constructed.
struct SentryLightPointParams {
    SentryLightPointParams();
    SettingParam splinePointIdx, forwardMoveSpeed, reverseMoveSpeed, forwardWaitTime, reverseWaitTime, isEnabled;
};
extern SentryLightPointParams SentryLightPoints[16];
} // namespace sai_params

// Association callbacks of the tag settings, provided by the game systems.
struct SAIAssociationHooks {
    SettingAssociationFn navJumpLink = nullptr;   // 0x00010B18
    SettingAssociationFn proxyPoint = nullptr;    // 0x00064F48 AIProxyPointTable::RegisterByName
    SettingAssociationFn entitySpawner = nullptr; // 0x00455468 LoadSpawnerSetting
    SettingAssociationFn aiSpawner = nullptr;     // 0x00455038
    SettingAssociationFn laserGroup = nullptr;    // 0x003A1828
    SettingAssociationFn laser = nullptr;         // 0x003A1BA8 (fixed, anchor and spline lasers)
    SettingAssociationFn laserField = nullptr;    // 0x003A20A4
    SettingAssociationFn spawnerGroup = nullptr;  // 0x00455728 LoadSpawnGroupSetting
    SettingAssociationFn teleporter = nullptr;    // 0x001EB9CC
    SettingAssociationFn sentryLight = nullptr;   // 0x001D2CFC
};
extern SAIAssociationHooks g_saiAssociationHooks;

// Factories of the definitions that live with the character code; null until
// those are ported (the manager skips null entries).
struct SAIExternalDefs {
    SettingDefFactory enemy = nullptr;        // 0x007833F0 EnemySetting
    SettingDefFactory critter = nullptr;      // 0x00783384 CritterSetting
    SettingDefFactory scriptedPath = nullptr; // 0x007834C8 ScriptedPathSetting
    SettingDefFactory player = nullptr;       // 0x0078345C PlayerSetting
};
extern SAIExternalDefs g_saiExternalDefs;

// The game's setting manager (vtable 0x00B217E8, singleton 0x00F658D4).
class SAISettingManager : public ape::setting::SettingManager {
public:
    explicit SAISettingManager(bool allocCache);
    ~SAISettingManager() override;

private:
    SAISettingManager(bool allocCache, const std::vector<SettingDefFactory>& defs);
};
extern SAISettingManager* g_saiSettingManager;

} // namespace sly
