// openTIT - Setting classes of the AI settings level objects.
#include "sly/levels/ai/AISettingClasses.h"

namespace sly {
namespace levels {
namespace ai {

// @func 0x0001CE40 CritterSettingClass::RegisterProperties
void CritterSettingClass::RegisterProperties()
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

// @func 0x00023F3C BuddySettingClass::RegisterProperties
void BuddySettingClass::RegisterProperties()
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

// @func 0x0002E4FC EnemySettingClass::RegisterProperties
void EnemySettingClass::RegisterProperties()
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

// @func 0x0003BE60 SentrySettingClass::RegisterProperties
void SentrySettingClass::RegisterProperties()
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

// @func 0x0001D10C CritterSettingClass::CreateInstance
AISettingInstance* CritterSettingClass::CreateInstance(const void* source, const void* owner)
{
    (void)owner;
    // NOTE: the console instance is 0x28 bytes; its own fields are not used here.
    return new AISettingInstance(source, this);
}

// @func 0x00023FB0 BuddySettingClass::CreateInstance
AISettingInstance* BuddySettingClass::CreateInstance(const void* source, const void* owner)
{
    (void)owner;
    // NOTE: the console instance is 0x24 bytes; its own fields are not used here.
    return new AISettingInstance(source, this);
}

// @func 0x0002EB74 EnemySettingClass::CreateInstance
AISettingInstance* EnemySettingClass::CreateInstance(const void* source, const void* owner)
{
    (void)owner;
    // NOTE: the console instance is 0x28 bytes; its own fields are not used here.
    return new AISettingInstance(source, this);
}

// @func 0x0003C000 SentrySettingClass::CreateInstance
AISettingInstance* SentrySettingClass::CreateInstance(const void* source, const void* owner)
{
    (void)owner;
    // NOTE: the console instance is 0x24 bytes; its own fields are not used here.
    return new AISettingInstance(source, this);
}

} // namespace ai
} // namespace levels
} // namespace sly
