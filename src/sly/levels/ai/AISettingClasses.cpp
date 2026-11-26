// openTIT - Setting classes of the AI settings level objects.
#include "sly/levels/ai/AISettingClasses.h"

#include "sly/levels/ai/AISettingParams.h"

namespace sly {
namespace levels {
namespace ai {

namespace {
// Group hash of the character setting classes (also used by the hot spot
// definitions; see ape::setting::SettingDef::GetGroupHash).
constexpr uint32_t kAISettingGroup = 0xC9D7446Au;
} // namespace

void AISettingClass::AddProperty(AIProp prop)
{
    // Properties of other modules (proxy points, jump links) have no
    // descriptor here.
    if (const ape::setting::SettingParam* param = AIPropParam(prop))
        AddParam(const_cast<ape::setting::SettingParam&>(*param));
}

// @func 0x0001CE40 CritterSettingClass::InitParams
void CritterSettingClass::InitParams()
{
    static const AIProp kProps[] = {
        AIProp::Critter_BoundingRadius,
        AIProp::Critter_ScaleMin,
        AIProp::Critter_ScaleMax,
        AIProp::Critter_AlwaysUseBoundingRadius,
        AIProp::Critter_AvoidanceRadius,
        AIProp::Critter_IsFlyer,
        AIProp::Critter_UseFlightAnims,
        AIProp::Critter_OrientYUp,
        AIProp::Critter_UseNavigation,
        AIProp::Critter_UseAvoidance,
        AIProp::Critter_UsePhysics,
        AIProp::Critter_SnapToGround,
        AIProp::Critter_PerchOnRails,
        AIProp::Critter_IdleDelayMin,
        AIProp::Critter_IdleDelayMax,
        AIProp::Critter_POIDelayMin,
        AIProp::Critter_POIDelayMax,
        AIProp::Critter_QuickTurnDelayMin,
        AIProp::Critter_QuickTurnDelayMax,
        AIProp::Critter_QuickTurnAngleMin,
        AIProp::Critter_QuickTurnAngleMax,
        AIProp::Critter_POITargetDist,
        AIProp::Critter_FleePlayerDist,
        AIProp::Critter_StopFleeingPlayerDist,
        AIProp::Critter_ChargePlayerDist,
        AIProp::Critter_AttackPlayerDist,
        AIProp::Critter_NoticePlayerDist,
        AIProp::Critter_SpeedScale,
        AIProp::Critter_AttackBone,
        AIProp::Critter_IdleAnimClip,
        AIProp::Critter_SpawnAnimClip,
        AIProp::Critter_FleeAnimClip,
        AIProp::Critter_SpawnPosIsDst,
        AIProp::Critter_OneHitKill,
    };
    for (AIProp prop : kProps)
        AddProperty(prop);
}

// @func 0x00023F3C BuddySettingClass::InitParams
void BuddySettingClass::InitParams()
{
    static const AIProp kProps[] = {
        AIProp::Buddy_FollowDist,
        AIProp::Buddy_AttackDist,
        AIProp::Buddy_Murray,
        AIProp::Buddy_Bentley,
    };
    for (AIProp prop : kProps)
        AddProperty(prop);
}

// @func 0x0002E4FC EnemySettingClass::InitParams
void EnemySettingClass::InitParams()
{
    static const AIProp kProps[] = {
        AIProp::Enemy_EntityDefName,
        AIProp::Enemy_Awareness,
        AIProp::Enemy_RespondToDistressCall,
        AIProp::Enemy_NoTarget,
        AIProp::Enemy_CombatAlways,
        AIProp::Enemy_Health,
        AIProp::Enemy_Heavy,
        AIProp::Enemy_RatTrooper,
        AIProp::Enemy_Turret,
        AIProp::Enemy_TurretAttackMinTime,
        AIProp::Enemy_TurretAttackMaxTime,
        AIProp::Enemy_Cover,
        AIProp::Enemy_Patrol,
        AIProp::Enemy_PatrolIdleMinTime,
        AIProp::Enemy_PatrolIdleMaxTime,
        AIProp::Enemy_PatrolInteractionMinTime,
        AIProp::Enemy_PatrolInteractionMinDist,
        AIProp::Enemy_PatrolInteractionMaxDist,
        AIProp::Enemy_IdleAnimClip,
        AIProp::Enemy_SpawnAnimClip,
        AIProp::Enemy_MovementAnim,
        AIProp::Enemy_CombatAttackCloseAnim,
        AIProp::Enemy_CombatAttackMidAnim,
        AIProp::Enemy_CombatAttackFarAnim,
        AIProp::Enemy_CombatAttackRangedAnim,
        AIProp::Enemy_CombatOtherAnim,
        AIProp::Enemy_CombatMovementAnim,
        AIProp::Enemy_Scale,
        AIProp::Enemy_OptimalTargetRangeMin,
        AIProp::Enemy_OptimalTargetRangeMax,
        AIProp::Enemy_CloseAttackRange,
        AIProp::Enemy_CloseAttackAngle,
        AIProp::Enemy_CloseAttackHeightMin,
        AIProp::Enemy_CloseAttackHeightMax,
        AIProp::Enemy_MidAttackRange,
        AIProp::Enemy_MidAttackAngle,
        AIProp::Enemy_MidAttackHeightMin,
        AIProp::Enemy_MidAttackHeightMax,
        AIProp::Enemy_FarAttackRange,
        AIProp::Enemy_FarAttackAngle,
        AIProp::Enemy_FarAttackHeightMin,
        AIProp::Enemy_FarAttackHeightMax,
        AIProp::Enemy_RangedAttackRange,
        AIProp::Enemy_RangedAttackAngle,
        AIProp::Enemy_RangedAttackHeightMin,
        AIProp::Enemy_RangedAttackHeightMax,
        AIProp::Enemy_RangedAttackChaseFrequency,
        AIProp::Enemy_CounterAttackRange,
        AIProp::Enemy_SearchRange,
        AIProp::Enemy_DodgeRate,
        AIProp::Enemy_TauntRate,
        AIProp::Enemy_StunTime,
        AIProp::Enemy_SleepTime,
        AIProp::Enemy_ThrowDamage,
        AIProp::Enemy_KnockbackDamage,
        AIProp::Enemy_TorsoIKAngleOffset,
        AIProp::Enemy_AttackControllerUseTimer,
        AIProp::Enemy_AttackRequiresLOS,
        AIProp::Enemy_PlayerDamageOnly,
        AIProp::Enemy_StealthKillAttachTest,
        AIProp::Enemy_InitialAttackTime,
        AIProp::Enemy_LoseTargetTime,
        AIProp::Enemy_LoseTargetDist,
        AIProp::Enemy_TooCloseCylinderRadius,
        AIProp::Enemy_TooCloseCylinderHeight,
        AIProp::Enemy_LightCylinderRadius,
        AIProp::Enemy_LightCylinderHeight,
        AIProp::Enemy_AudioAlertRadius,
        AIProp::Enemy_AudioAlertHeightMin,
        AIProp::Enemy_AudioAlertHeightMax,
        AIProp::Enemy_AudioAlertInvestigate,
        AIProp::Enemy_CallForHelpDist,
        AIProp::Enemy_CallForHelpHeightMin,
        AIProp::Enemy_CallForHelpHeightMax,
        AIProp::Enemy_VisionDistance,
        AIProp::Enemy_VisionDistanceCombat,
        AIProp::Enemy_VisionFOVAngle,
        AIProp::Enemy_VisionHeightMin,
        AIProp::Enemy_VisionHeightMax,
        AIProp::Enemy_VisionHeightsRelativeToBase,
        AIProp::Enemy_VisionAttachBone,
    };
    for (AIProp prop : kProps)
        AddProperty(prop);
}

// @func 0x0003BE60 SentrySettingClass::InitParams
void SentrySettingClass::InitParams()
{
    static const AIProp kProps[] = {
        AIProp::Sentry_SpawnAnimClip,
        AIProp::Sentry_IdleAnimClip,
        AIProp::Sentry_FormationOffset,
        AIProp::Sentry_Awareness,
        AIProp::Sentry_AudioAlertRadius,
        AIProp::Sentry_AudioAlertWhileALertFail,
        AIProp::Sentry_AudioAlertHeightMin,
        AIProp::Sentry_AudioAlertHeightMax,
        AIProp::Sentry_VisionDistance,
        AIProp::Sentry_VisionFOVAngle,
        AIProp::Sentry_VisionHeightMin,
        AIProp::Sentry_VisionHeightMax,
        AIProp::Sentry_VisionAttachBone,
        AIProp::Sentry_SneakAllowed,
        AIProp::Sentry_DetectionCylinderRadius,
        AIProp::Sentry_DetectionCylinderHeight,
        AIProp::Sentry_DetectionCylinderYOffset,
        AIProp::Sentry_AlwaysApplyGravity,
        AIProp::Sentry_OrientToTerrain,
    };
    for (AIProp prop : kProps)
        AddProperty(prop);
}

// @func 0x0001D10C CritterSettingClass::CreateSetting
ape::setting::Setting* CritterSettingClass::CreateSetting(const char* name)
{
    // NOTE: the console instance is 0x28 bytes; its own fields are not used here.
    return new CritterSettingInstance(this, name);
}

// @func 0x00023FB0 BuddySettingClass::CreateSetting
ape::setting::Setting* BuddySettingClass::CreateSetting(const char* name)
{
    // NOTE: the console instance is 0x24 bytes; its own fields are not used here.
    return new BuddySettingInstance(this, name);
}

// @func 0x0002EB74 EnemySettingClass::CreateSetting
ape::setting::Setting* EnemySettingClass::CreateSetting(const char* name)
{
    // NOTE: the console instance is 0x28 bytes; its own fields are not used here.
    return new EnemySettingInstance(this, name);
}

// @func 0x0003C000 SentrySettingClass::CreateSetting
ape::setting::Setting* SentrySettingClass::CreateSetting(const char* name)
{
    // NOTE: the console instance is 0x24 bytes; its own fields are not used here.
    return new SentrySettingInstance(this, name);
}

// ---------------------------------------------------------------------------
// Identification of the setting classes, destructors of the classes and of
// their instances.

// @func 0x00733F2C CritterSettingClass::~CritterSettingClass
// @func 0x007388E8 CritterSettingClass::~CritterSettingClass (deleting)
CritterSettingClass::~CritterSettingClass() = default;
// @func 0x007388BC CritterSettingClass::GetName
const char* CritterSettingClass::GetName() const { return "CritterSetting"; }
// @func 0x007388C8 CritterSettingClass::GetTypeHash
uint32_t CritterSettingClass::GetTypeHash() const { return 0x958CAC69u; }   // name hash
// @func 0x007388D8 CritterSettingClass::GetGroupHash
uint32_t CritterSettingClass::GetGroupHash() const { return kAISettingGroup; }

// @func 0x00734340 BuddySettingClass::~BuddySettingClass
// @func 0x00738B5C BuddySettingClass::~BuddySettingClass (deleting)
BuddySettingClass::~BuddySettingClass() = default;
// @func 0x00738B44 BuddySettingClass::GetName
const char* BuddySettingClass::GetName() const { return "PlayerSetting"; }
// @func 0x00738B50 BuddySettingClass::GetTypeHash
uint32_t BuddySettingClass::GetTypeHash() const { return 0x6B05BC2Fu; }   // name hash

// @func 0x007346F0 EnemySettingClass::~EnemySettingClass
// @func 0x00738E40 EnemySettingClass::~EnemySettingClass (deleting)
EnemySettingClass::~EnemySettingClass() = default;
// @func 0x00738E14 EnemySettingClass::GetName
const char* EnemySettingClass::GetName() const { return "EnemySetting"; }
// @func 0x00738E20 EnemySettingClass::GetTypeHash
uint32_t EnemySettingClass::GetTypeHash() const { return 0x9F641DC8u; }   // name hash
// @func 0x00738E30 EnemySettingClass::GetGroupHash
uint32_t EnemySettingClass::GetGroupHash() const { return kAISettingGroup; }

// @func 0x00735360 SentrySettingClass::~SentrySettingClass
// @func 0x00739164 SentrySettingClass::~SentrySettingClass (deleting)
SentrySettingClass::~SentrySettingClass() = default;
// @func 0x00739138 SentrySettingClass::GetName
const char* SentrySettingClass::GetName() const { return "ScriptedPathSetting"; }
// @func 0x00739144 SentrySettingClass::GetTypeHash
uint32_t SentrySettingClass::GetTypeHash() const { return 0xDC568581u; }   // name hash
// @func 0x00739154 SentrySettingClass::GetGroupHash
uint32_t SentrySettingClass::GetGroupHash() const { return kAISettingGroup; }

// @func 0x00733F5C CritterSettingInstance::~CritterSettingInstance
// @func 0x00738940 CritterSettingInstance::~CritterSettingInstance (deleting)
CritterSettingInstance::~CritterSettingInstance() = default;
// @func 0x00734370 BuddySettingInstance::~BuddySettingInstance
// @func 0x00738BB4 BuddySettingInstance::~BuddySettingInstance (deleting)
BuddySettingInstance::~BuddySettingInstance() = default;
// @func 0x00734720 EnemySettingInstance::~EnemySettingInstance
// @func 0x00738E98 EnemySettingInstance::~EnemySettingInstance (deleting)
EnemySettingInstance::~EnemySettingInstance() = default;
// @func 0x00735390 SentrySettingInstance::~SentrySettingInstance
// @func 0x007391BC SentrySettingInstance::~SentrySettingInstance (deleting)
SentrySettingInstance::~SentrySettingInstance() = default;

} // namespace ai
} // namespace levels
} // namespace sly
