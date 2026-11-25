// openTIT - Parameter descriptors of the AI setting classes (see AISettingParams.h).
#include "sly/levels/ai/AISettingParams.h"

namespace sly {
namespace levels {
namespace ai {

using ape::kDTD_Bool;
using ape::kDTD_Float;
using ape::kDTD_Hash;
using ape::kDTD_String;
using ape::setting::SettingBool;
using ape::setting::SettingFloat;
using ape::setting::SettingHash;
using ape::setting::SettingParam;
using ape::setting::SettingString;
using ape::setting::SettingVec3Param;

namespace {

// In console construction order (sentry first). Usage 1 = optional.
// Hash defaults: 0x002237D4 "Idle", 0x0021C5E0 "Head", 0x00013E6B "Run"; the other
// anim names are not in the executable.
SettingParam Sentry_SpawnAnimClip("SpawnAnimClip", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F952AC
SettingParam Sentry_IdleAnimClip("IdleAnimClip", kDTD_Hash, 1, SettingHash(0x002237D4u));  // 00F952C4
SettingVec3Param Sentry_FormationOffset("FormationOffset", 1, 0.0f, 0.0f, -5.0f);  // 00F952E0
SettingParam Sentry_Awareness("Awareness", kDTD_Bool, 1, SettingBool(true));  // 00F95320
SettingParam Sentry_AudioAlertRadius("AudioAlertRadius", kDTD_Float, 1, SettingFloat(11.0f));  // 00F95338
SettingParam Sentry_AudioAlertWhileALertFail("AudioAlertWhileALertFail", kDTD_Bool, 1, SettingBool(false));  // 00F95350
SettingParam Sentry_AudioAlertHeightMin("AudioAlertHeightMin", kDTD_Float, 1, SettingFloat(-8.0f));  // 00F95368
SettingParam Sentry_AudioAlertHeightMax("AudioAlertHeightMax", kDTD_Float, 1, SettingFloat(8.0f));  // 00F95380
SettingParam Sentry_VisionDistance("VisionDistance", kDTD_Float, 1, SettingFloat(7.0f));  // 00F95398
SettingParam Sentry_VisionFOVAngle("VisionFOVAngle", kDTD_Float, 1, SettingFloat(25.0f));  // 00F953B0
SettingParam Sentry_VisionHeightMin("VisionHeightMin", kDTD_Float, 1, SettingFloat(-3.5f));  // 00F953C8
SettingParam Sentry_VisionHeightMax("VisionHeightMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F953E0
SettingParam Sentry_VisionAttachBone("VisionAttachBone", kDTD_Hash, 1, SettingHash(0x0021C5E0u));  // 00F953F8
SettingParam Sentry_SneakAllowed("SneakAllowed", kDTD_Bool, 1, SettingBool(true));  // 00F95410
SettingParam Sentry_DetectionCylinderRadius("DetectionCylinderRadius", kDTD_Float, 1, SettingFloat(2.0f));  // 00F95428
SettingParam Sentry_DetectionCylinderHeight("DetectionCylinderHeight", kDTD_Float, 1, SettingFloat(4.2f));  // 00F95440
SettingParam Sentry_DetectionCylinderYOffset("DetectionCylinderYOffset", kDTD_Float, 1, SettingFloat(-1.0f));  // 00F95458
SettingParam Sentry_AlwaysApplyGravity("AlwaysApplyGravity", kDTD_Bool, 1, SettingBool(false));  // 00F95470
SettingParam Sentry_OrientToTerrain("OrientToTerrain", kDTD_Bool, 1, SettingBool(false));  // 00F95488
SettingParam Critter_BoundingRadius("BoundingRadius", kDTD_Float, 1, SettingFloat(10.0f));  // 00F94784
SettingParam Critter_ScaleMin("ScaleMin", kDTD_Float, 1, SettingFloat(1.0f));  // 00F9479C
SettingParam Critter_ScaleMax("ScaleMax", kDTD_Float, 1, SettingFloat(1.0f));  // 00F947B4
SettingParam Critter_AvoidanceRadius("AvoidanceRadius", kDTD_Float, 1, SettingFloat(0.5f));  // 00F947E4
SettingParam Critter_IdleDelayMin("IdleDelayMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F948BC
SettingParam Critter_IdleDelayMax("IdleDelayMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F948D4
SettingParam Critter_POIDelayMin("POIDelayMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F948EC
SettingParam Critter_POIDelayMax("POIDelayMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94904
SettingParam Critter_QuickTurnDelayMin("QuickTurnDelayMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F9491C
SettingParam Critter_QuickTurnDelayMax("QuickTurnDelayMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94934
SettingParam Critter_QuickTurnAngleMin("QuickTurnAngleMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F9494C
SettingParam Critter_QuickTurnAngleMax("QuickTurnAngleMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94964
SettingParam Critter_POITargetDist("POITargetDist", kDTD_Float, 1, SettingFloat(1.0f));  // 00F9497C
SettingParam Critter_FleePlayerDist("FleePlayerDist", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94994
SettingParam Critter_StopFleeingPlayerDist("StopFleeingPlayerDist", kDTD_Float, 1, SettingFloat(0.0f));  // 00F949AC
SettingParam Critter_ChargePlayerDist("ChargePlayerDist", kDTD_Float, 1, SettingFloat(0.0f));  // 00F949C4
SettingParam Critter_AttackPlayerDist("AttackPlayerDist", kDTD_Float, 1, SettingFloat(0.0f));  // 00F949DC
SettingParam Critter_NoticePlayerDist("NoticePlayerDist", kDTD_Float, 1, SettingFloat(0.0f));  // 00F949F4
SettingParam Critter_SpeedScale("SpeedScale", kDTD_Float, 1, SettingFloat(1.0f));  // 00F94A0C
SettingParam Critter_AttackBone("AttackBone", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94A24
SettingParam Critter_IdleAnimClip("IdleAnimClip", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94A3C
SettingParam Critter_SpawnAnimClip("SpawnAnimClip", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94A54
SettingParam Critter_FleeAnimClip("FleeAnimClip", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94A6C
SettingParam Critter_SpawnPosIsDst("SpawnPosIsDst", kDTD_Bool, 1, SettingBool(false));  // 00F94A84
SettingParam Critter_OneHitKill("OneHitKill", kDTD_Bool, 1, SettingBool(true));  // 00F94A9C
SettingParam Critter_AlwaysUseBoundingRadius("AlwaysUseBoundingRadius", kDTD_Bool, 1, SettingBool(false));  // 00F947CC
SettingParam Critter_IsFlyer("IsFlyer", kDTD_Bool, 1, SettingBool(false));  // 00F947FC
SettingParam Critter_UseFlightAnims("UseFlightAnims", kDTD_Bool, 1, SettingBool(false));  // 00F94814
SettingParam Critter_OrientYUp("OrientYUp", kDTD_Bool, 1, SettingBool(true));  // 00F9482C
SettingParam Critter_UseNavigation("UseNavigation", kDTD_Bool, 1, SettingBool(false));  // 00F94844
SettingParam Critter_UseAvoidance("UseAvoidance", kDTD_Bool, 1, SettingBool(false));  // 00F9485C
SettingParam Critter_UsePhysics("UsePhysics", kDTD_Bool, 1, SettingBool(false));  // 00F94874
SettingParam Critter_SnapToGround("SnapToGround", kDTD_Bool, 1, SettingBool(false));  // 00F9488C
SettingParam Critter_PerchOnRails("PerchOnRails", kDTD_Bool, 1, SettingBool(false));  // 00F948A4
SettingParam Buddy_FollowDist("FollowDist", kDTD_Float, 1, SettingFloat(1.0f));  // 00F94AB4
SettingParam Buddy_AttackDist("AttackDist", kDTD_Float, 1, SettingFloat(3.0f));  // 00F94ACC
SettingParam Buddy_Murray("Murray", kDTD_Bool, 1, SettingBool(false));  // 00F94AE4
SettingParam Buddy_Bentley("Bentley", kDTD_Bool, 1, SettingBool(false));  // 00F94AFC
SettingParam Enemy_EntityDefName("EntityDefName", kDTD_String, 1, SettingString(""));  // 00F94B14
SettingParam Enemy_Health("Health", kDTD_Float, 1, SettingFloat(100.0f));  // 00F94B5C
SettingParam Enemy_Heavy("Heavy", kDTD_Bool, 1, SettingBool(false));  // 00F94B74
SettingParam Enemy_RatTrooper("RatTrooper", kDTD_Bool, 1, SettingBool(false));  // 00F94B8C
SettingParam Enemy_Turret("Turret", kDTD_Bool, 1, SettingBool(false));  // 00F94BA4
SettingParam Enemy_TurretAttackMinTime("TurretAttackMinTime", kDTD_Float, 1, SettingFloat(-1.0f));  // 00F94BBC
SettingParam Enemy_TurretAttackMaxTime("TurretAttackMaxTime", kDTD_Float, 1, SettingFloat(-1.0f));  // 00F94BD4
SettingParam Enemy_Cover("Cover", kDTD_Bool, 1, SettingBool(false));  // 00F94BEC
SettingParam Enemy_Patrol("Patrol", kDTD_Bool, 1, SettingBool(false));  // 00F94C04
SettingParam Enemy_PatrolIdleMinTime("PatrolIdleMinTime", kDTD_Float, 1, SettingFloat(5.0f));  // 00F94C1C
SettingParam Enemy_PatrolIdleMaxTime("PatrolIdleMaxTime", kDTD_Float, 1, SettingFloat(12.0f));  // 00F94C34
SettingParam Enemy_PatrolInteractionMinTime("PatrolInteractionMinTime", kDTD_Float, 1, SettingFloat(30.0f));  // 00F94C4C
SettingParam Enemy_PatrolInteractionMinDist("PatrolInteractionMinDist", kDTD_Float, 1, SettingFloat(5.0f));  // 00F94C64
SettingParam Enemy_PatrolInteractionMaxDist("PatrolInteractionMaxDist", kDTD_Float, 1, SettingFloat(7.0f));  // 00F94C7C
SettingParam Enemy_Scale("Scale", kDTD_Float, 1, SettingFloat(1.0f));  // 00F94D6C
SettingParam Enemy_IdleAnimClip("IdleAnimClip", kDTD_Hash, 1, SettingHash(0x5340E61Cu));  // 00F94C94
SettingParam Enemy_SpawnAnimClip("SpawnAnimClip", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94CAC
SettingParam Enemy_MovementAnim("MovementAnim", kDTD_Hash, 1, SettingHash(0x00000000u));  // 00F94CC4
SettingParam Enemy_CombatAttackCloseAnim("CombatAttackCloseAnim", kDTD_Hash, 1, SettingHash(0x69109D1Cu));  // 00F94CDC
SettingParam Enemy_CombatAttackMidAnim("CombatAttackMidAnim", kDTD_Hash, 1, SettingHash(0x70CAE26Cu));  // 00F94CF4
SettingParam Enemy_CombatAttackFarAnim("CombatAttackFarAnim", kDTD_Hash, 1, SettingHash(0x70CAC73Bu));  // 00F94D0C
SettingParam Enemy_CombatAttackRangedAnim("CombatAttackRangedAnim", kDTD_Hash, 1, SettingHash(0xD2001403u));  // 00F94D24
SettingParam Enemy_CombatOtherAnim("CombatOtherAnim", kDTD_Hash, 1, SettingHash(0xBD744C7Cu));  // 00F94D3C
SettingParam Enemy_CombatMovementAnim("CombatMovementAnim", kDTD_Hash, 1, SettingHash(0x00013E6Bu));  // 00F94D54
SettingParam Enemy_OptimalTargetRangeMin("OptimalTargetRangeMin", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94D84
SettingParam Enemy_OptimalTargetRangeMax("OptimalTargetRangeMax", kDTD_Float, 1, SettingFloat(8.0f));  // 00F94D9C
SettingParam Enemy_CloseAttackRange("CloseAttackRange", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94DB4
SettingParam Enemy_CloseAttackAngle("CloseAttackAngle", kDTD_Float, 1, SettingFloat(30.0f));  // 00F94DCC
SettingParam Enemy_CloseAttackHeightMin("CloseAttackHeightMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94DE4
SettingParam Enemy_CloseAttackHeightMax("CloseAttackHeightMax", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94DFC
SettingParam Enemy_MidAttackRange("MidAttackRange", kDTD_Float, 1, SettingFloat(8.0f));  // 00F94E14
SettingParam Enemy_MidAttackAngle("MidAttackAngle", kDTD_Float, 1, SettingFloat(30.0f));  // 00F94E2C
SettingParam Enemy_MidAttackHeightMin("MidAttackHeightMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94E44
SettingParam Enemy_MidAttackHeightMax("MidAttackHeightMax", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94E5C
SettingParam Enemy_FarAttackRange("FarAttackRange", kDTD_Float, 1, SettingFloat(15.0f));  // 00F94E74
SettingParam Enemy_FarAttackAngle("FarAttackAngle", kDTD_Float, 1, SettingFloat(10.0f));  // 00F94E8C
SettingParam Enemy_FarAttackHeightMin("FarAttackHeightMin", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94EA4
SettingParam Enemy_FarAttackHeightMax("FarAttackHeightMax", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94EBC
SettingParam Enemy_RangedAttackRange("RangedAttackRange", kDTD_Float, 1, SettingFloat(30.0f));  // 00F94ED4
SettingParam Enemy_RangedAttackAngle("RangedAttackAngle", kDTD_Float, 1, SettingFloat(10.0f));  // 00F94EEC
SettingParam Enemy_RangedAttackHeightMin("RangedAttackHeightMin", kDTD_Float, 1, SettingFloat(-20.0f));  // 00F94F04
SettingParam Enemy_RangedAttackHeightMax("RangedAttackHeightMax", kDTD_Float, 1, SettingFloat(20.0f));  // 00F94F1C
SettingParam Enemy_RangedAttackChaseFrequency("RangedAttackChaseFrequency", kDTD_Float, 1, SettingFloat(2.0f));  // 00F94F34
SettingParam Enemy_CounterAttackRange("CounterAttackRange", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94F4C
SettingParam Enemy_SearchRange("SearchRange", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94F64
SettingParam Enemy_DodgeRate("DodgeRate", kDTD_Float, 1, SettingFloat(0.0f));  // 00F94F7C
SettingParam Enemy_TauntRate("TauntRate", kDTD_Float, 1, SettingFloat(1.0f));  // 00F94F94
SettingParam Enemy_StunTime("StunTime", kDTD_Float, 1, SettingFloat(4.0f));  // 00F94FAC
SettingParam Enemy_SleepTime("SleepTime", kDTD_Float, 1, SettingFloat(10.0f));  // 00F94FC4
SettingParam Enemy_ThrowDamage("ThrowDamage", kDTD_Float, 1, SettingFloat(50.0f));  // 00F94FDC
SettingParam Enemy_KnockbackDamage("KnockbackDamage", kDTD_Float, 1, SettingFloat(20.0f));  // 00F94FF4
SettingParam Enemy_TorsoIKAngleOffset("TorsoIKAngleOffset", kDTD_Float, 1, SettingFloat(0.0f));  // 00F9500C
SettingParam Enemy_AttackRequiresLOS("AttackRequiresLOS", kDTD_Bool, 1, SettingBool(true));  // 00F9503C
SettingParam Enemy_AttackControllerUseTimer("AttackControllerUseTimer", kDTD_Bool, 1, SettingBool(true));  // 00F95024
SettingParam Enemy_PlayerDamageOnly("PlayerDamageOnly", kDTD_Bool, 1, SettingBool(false));  // 00F95054
SettingParam Enemy_StealthKillAttachTest("StealthKillAttachTest", kDTD_Bool, 1, SettingBool(false));  // 00F9506C
SettingParam Enemy_InitialAttackTime("InitialAttackTime", kDTD_Float, 1, SettingFloat(3.0f));  // 00F95084
SettingParam Enemy_Awareness("Awareness", kDTD_Bool, 1, SettingBool(true));  // 00F9509C
SettingParam Enemy_RespondToDistressCall("RespondToDistressCall", kDTD_Bool, 1, SettingBool(true));  // 00F950B4
SettingParam Enemy_NoTarget("NoTarget", kDTD_Bool, 1, SettingBool(false));  // 00F94B2C
SettingParam Enemy_CombatAlways("CombatAlways", kDTD_Bool, 1, SettingBool(false));  // 00F94B44
SettingParam Enemy_LoseTargetTime("LoseTargetTime", kDTD_Float, 1, SettingFloat(10.0f));  // 00F950CC
SettingParam Enemy_LoseTargetDist("LoseTargetDist", kDTD_Float, 1, SettingFloat(30.0f));  // 00F950E4
SettingParam Enemy_TooCloseCylinderRadius("TooCloseCylinderRadius", kDTD_Float, 1, SettingFloat(2.0f));  // 00F950FC
SettingParam Enemy_TooCloseCylinderHeight("TooCloseCylinderHeight", kDTD_Float, 1, SettingFloat(3.8f));  // 00F95114
SettingParam Enemy_LightCylinderRadius("LightCylinderRadius", kDTD_Float, 1, SettingFloat(4.5f));  // 00F9512C
SettingParam Enemy_LightCylinderHeight("LightCylinderHeight", kDTD_Float, 1, SettingFloat(2.5f));  // 00F95144
SettingParam Enemy_AudioAlertRadius("AudioAlertRadius", kDTD_Float, 1, SettingFloat(11.0f));  // 00F9515C
SettingParam Enemy_AudioAlertHeightMin("AudioAlertHeightMin", kDTD_Float, 1, SettingFloat(-8.0f));  // 00F95174
SettingParam Enemy_AudioAlertHeightMax("AudioAlertHeightMax", kDTD_Float, 1, SettingFloat(8.0f));  // 00F9518C
SettingParam Enemy_AudioAlertInvestigate("AudioAlertInvestigate", kDTD_Bool, 1, SettingBool(true));  // 00F951A4
SettingParam Enemy_CallForHelpDist("CallForHelpDist", kDTD_Float, 1, SettingFloat(15.0f));  // 00F951BC
SettingParam Enemy_CallForHelpHeightMin("CallForHelpHeightMin", kDTD_Float, 1, SettingFloat(-5.0f));  // 00F951D4
SettingParam Enemy_CallForHelpHeightMax("CallForHelpHeightMax", kDTD_Float, 1, SettingFloat(5.0f));  // 00F951EC
SettingParam Enemy_VisionDistance("VisionDistance", kDTD_Float, 1, SettingFloat(7.0f));  // 00F95204
SettingParam Enemy_VisionDistanceCombat("VisionDistanceCombat", kDTD_Float, 1, SettingFloat(20.0f));  // 00F9521C
SettingParam Enemy_VisionFOVAngle("VisionFOVAngle", kDTD_Float, 1, SettingFloat(25.0f));  // 00F95234
SettingParam Enemy_VisionHeightMin("VisionHeightMin", kDTD_Float, 1, SettingFloat(-3.5f));  // 00F9524C
SettingParam Enemy_VisionHeightMax("VisionHeightMax", kDTD_Float, 1, SettingFloat(0.0f));  // 00F95264
SettingParam Enemy_VisionHeightsRelativeToBase("VisionHeightsRelativeToBase", kDTD_Bool, 1, SettingBool(false));  // 00F9527C
SettingParam Enemy_VisionAttachBone("VisionAttachBone", kDTD_Hash, 1, SettingHash(0x0021C5E0u));  // 00F95294

} // namespace

const SettingParam* AIPropParam(AIProp prop)
{
    switch (prop) {
    case AIProp::Critter_BoundingRadius: return &Critter_BoundingRadius;
    case AIProp::Critter_ScaleMin: return &Critter_ScaleMin;
    case AIProp::Critter_ScaleMax: return &Critter_ScaleMax;
    case AIProp::Critter_AlwaysUseBoundingRadius: return &Critter_AlwaysUseBoundingRadius;
    case AIProp::Critter_AvoidanceRadius: return &Critter_AvoidanceRadius;
    case AIProp::Critter_IsFlyer: return &Critter_IsFlyer;
    case AIProp::Critter_UseFlightAnims: return &Critter_UseFlightAnims;
    case AIProp::Critter_OrientYUp: return &Critter_OrientYUp;
    case AIProp::Critter_UseNavigation: return &Critter_UseNavigation;
    case AIProp::Critter_UseAvoidance: return &Critter_UseAvoidance;
    case AIProp::Critter_UsePhysics: return &Critter_UsePhysics;
    case AIProp::Critter_SnapToGround: return &Critter_SnapToGround;
    case AIProp::Critter_PerchOnRails: return &Critter_PerchOnRails;
    case AIProp::Critter_IdleDelayMin: return &Critter_IdleDelayMin;
    case AIProp::Critter_IdleDelayMax: return &Critter_IdleDelayMax;
    case AIProp::Critter_POIDelayMin: return &Critter_POIDelayMin;
    case AIProp::Critter_POIDelayMax: return &Critter_POIDelayMax;
    case AIProp::Critter_QuickTurnDelayMin: return &Critter_QuickTurnDelayMin;
    case AIProp::Critter_QuickTurnDelayMax: return &Critter_QuickTurnDelayMax;
    case AIProp::Critter_QuickTurnAngleMin: return &Critter_QuickTurnAngleMin;
    case AIProp::Critter_QuickTurnAngleMax: return &Critter_QuickTurnAngleMax;
    case AIProp::Critter_POITargetDist: return &Critter_POITargetDist;
    case AIProp::Critter_FleePlayerDist: return &Critter_FleePlayerDist;
    case AIProp::Critter_StopFleeingPlayerDist: return &Critter_StopFleeingPlayerDist;
    case AIProp::Critter_ChargePlayerDist: return &Critter_ChargePlayerDist;
    case AIProp::Critter_AttackPlayerDist: return &Critter_AttackPlayerDist;
    case AIProp::Critter_NoticePlayerDist: return &Critter_NoticePlayerDist;
    case AIProp::Critter_SpeedScale: return &Critter_SpeedScale;
    case AIProp::Critter_AttackBone: return &Critter_AttackBone;
    case AIProp::Critter_IdleAnimClip: return &Critter_IdleAnimClip;
    case AIProp::Critter_SpawnAnimClip: return &Critter_SpawnAnimClip;
    case AIProp::Critter_FleeAnimClip: return &Critter_FleeAnimClip;
    case AIProp::Critter_SpawnPosIsDst: return &Critter_SpawnPosIsDst;
    case AIProp::Critter_OneHitKill: return &Critter_OneHitKill;
    case AIProp::Buddy_FollowDist: return &Buddy_FollowDist;
    case AIProp::Buddy_AttackDist: return &Buddy_AttackDist;
    case AIProp::Buddy_Murray: return &Buddy_Murray;
    case AIProp::Buddy_Bentley: return &Buddy_Bentley;
    case AIProp::Enemy_EntityDefName: return &Enemy_EntityDefName;
    case AIProp::Enemy_Awareness: return &Enemy_Awareness;
    case AIProp::Enemy_RespondToDistressCall: return &Enemy_RespondToDistressCall;
    case AIProp::Enemy_NoTarget: return &Enemy_NoTarget;
    case AIProp::Enemy_CombatAlways: return &Enemy_CombatAlways;
    case AIProp::Enemy_Health: return &Enemy_Health;
    case AIProp::Enemy_Heavy: return &Enemy_Heavy;
    case AIProp::Enemy_RatTrooper: return &Enemy_RatTrooper;
    case AIProp::Enemy_Turret: return &Enemy_Turret;
    case AIProp::Enemy_TurretAttackMinTime: return &Enemy_TurretAttackMinTime;
    case AIProp::Enemy_TurretAttackMaxTime: return &Enemy_TurretAttackMaxTime;
    case AIProp::Enemy_Cover: return &Enemy_Cover;
    case AIProp::Enemy_Patrol: return &Enemy_Patrol;
    case AIProp::Enemy_PatrolIdleMinTime: return &Enemy_PatrolIdleMinTime;
    case AIProp::Enemy_PatrolIdleMaxTime: return &Enemy_PatrolIdleMaxTime;
    case AIProp::Enemy_PatrolInteractionMinTime: return &Enemy_PatrolInteractionMinTime;
    case AIProp::Enemy_PatrolInteractionMinDist: return &Enemy_PatrolInteractionMinDist;
    case AIProp::Enemy_PatrolInteractionMaxDist: return &Enemy_PatrolInteractionMaxDist;
    case AIProp::Enemy_IdleAnimClip: return &Enemy_IdleAnimClip;
    case AIProp::Enemy_SpawnAnimClip: return &Enemy_SpawnAnimClip;
    case AIProp::Enemy_MovementAnim: return &Enemy_MovementAnim;
    case AIProp::Enemy_CombatAttackCloseAnim: return &Enemy_CombatAttackCloseAnim;
    case AIProp::Enemy_CombatAttackMidAnim: return &Enemy_CombatAttackMidAnim;
    case AIProp::Enemy_CombatAttackFarAnim: return &Enemy_CombatAttackFarAnim;
    case AIProp::Enemy_CombatAttackRangedAnim: return &Enemy_CombatAttackRangedAnim;
    case AIProp::Enemy_CombatOtherAnim: return &Enemy_CombatOtherAnim;
    case AIProp::Enemy_CombatMovementAnim: return &Enemy_CombatMovementAnim;
    case AIProp::Enemy_Scale: return &Enemy_Scale;
    case AIProp::Enemy_OptimalTargetRangeMin: return &Enemy_OptimalTargetRangeMin;
    case AIProp::Enemy_OptimalTargetRangeMax: return &Enemy_OptimalTargetRangeMax;
    case AIProp::Enemy_CloseAttackRange: return &Enemy_CloseAttackRange;
    case AIProp::Enemy_CloseAttackAngle: return &Enemy_CloseAttackAngle;
    case AIProp::Enemy_CloseAttackHeightMin: return &Enemy_CloseAttackHeightMin;
    case AIProp::Enemy_CloseAttackHeightMax: return &Enemy_CloseAttackHeightMax;
    case AIProp::Enemy_MidAttackRange: return &Enemy_MidAttackRange;
    case AIProp::Enemy_MidAttackAngle: return &Enemy_MidAttackAngle;
    case AIProp::Enemy_MidAttackHeightMin: return &Enemy_MidAttackHeightMin;
    case AIProp::Enemy_MidAttackHeightMax: return &Enemy_MidAttackHeightMax;
    case AIProp::Enemy_FarAttackRange: return &Enemy_FarAttackRange;
    case AIProp::Enemy_FarAttackAngle: return &Enemy_FarAttackAngle;
    case AIProp::Enemy_FarAttackHeightMin: return &Enemy_FarAttackHeightMin;
    case AIProp::Enemy_FarAttackHeightMax: return &Enemy_FarAttackHeightMax;
    case AIProp::Enemy_RangedAttackRange: return &Enemy_RangedAttackRange;
    case AIProp::Enemy_RangedAttackAngle: return &Enemy_RangedAttackAngle;
    case AIProp::Enemy_RangedAttackHeightMin: return &Enemy_RangedAttackHeightMin;
    case AIProp::Enemy_RangedAttackHeightMax: return &Enemy_RangedAttackHeightMax;
    case AIProp::Enemy_RangedAttackChaseFrequency: return &Enemy_RangedAttackChaseFrequency;
    case AIProp::Enemy_CounterAttackRange: return &Enemy_CounterAttackRange;
    case AIProp::Enemy_SearchRange: return &Enemy_SearchRange;
    case AIProp::Enemy_DodgeRate: return &Enemy_DodgeRate;
    case AIProp::Enemy_TauntRate: return &Enemy_TauntRate;
    case AIProp::Enemy_StunTime: return &Enemy_StunTime;
    case AIProp::Enemy_SleepTime: return &Enemy_SleepTime;
    case AIProp::Enemy_ThrowDamage: return &Enemy_ThrowDamage;
    case AIProp::Enemy_KnockbackDamage: return &Enemy_KnockbackDamage;
    case AIProp::Enemy_TorsoIKAngleOffset: return &Enemy_TorsoIKAngleOffset;
    case AIProp::Enemy_AttackControllerUseTimer: return &Enemy_AttackControllerUseTimer;
    case AIProp::Enemy_AttackRequiresLOS: return &Enemy_AttackRequiresLOS;
    case AIProp::Enemy_PlayerDamageOnly: return &Enemy_PlayerDamageOnly;
    case AIProp::Enemy_StealthKillAttachTest: return &Enemy_StealthKillAttachTest;
    case AIProp::Enemy_InitialAttackTime: return &Enemy_InitialAttackTime;
    case AIProp::Enemy_LoseTargetTime: return &Enemy_LoseTargetTime;
    case AIProp::Enemy_LoseTargetDist: return &Enemy_LoseTargetDist;
    case AIProp::Enemy_TooCloseCylinderRadius: return &Enemy_TooCloseCylinderRadius;
    case AIProp::Enemy_TooCloseCylinderHeight: return &Enemy_TooCloseCylinderHeight;
    case AIProp::Enemy_LightCylinderRadius: return &Enemy_LightCylinderRadius;
    case AIProp::Enemy_LightCylinderHeight: return &Enemy_LightCylinderHeight;
    case AIProp::Enemy_AudioAlertRadius: return &Enemy_AudioAlertRadius;
    case AIProp::Enemy_AudioAlertHeightMin: return &Enemy_AudioAlertHeightMin;
    case AIProp::Enemy_AudioAlertHeightMax: return &Enemy_AudioAlertHeightMax;
    case AIProp::Enemy_AudioAlertInvestigate: return &Enemy_AudioAlertInvestigate;
    case AIProp::Enemy_CallForHelpDist: return &Enemy_CallForHelpDist;
    case AIProp::Enemy_CallForHelpHeightMin: return &Enemy_CallForHelpHeightMin;
    case AIProp::Enemy_CallForHelpHeightMax: return &Enemy_CallForHelpHeightMax;
    case AIProp::Enemy_VisionDistance: return &Enemy_VisionDistance;
    case AIProp::Enemy_VisionDistanceCombat: return &Enemy_VisionDistanceCombat;
    case AIProp::Enemy_VisionFOVAngle: return &Enemy_VisionFOVAngle;
    case AIProp::Enemy_VisionHeightMin: return &Enemy_VisionHeightMin;
    case AIProp::Enemy_VisionHeightMax: return &Enemy_VisionHeightMax;
    case AIProp::Enemy_VisionHeightsRelativeToBase: return &Enemy_VisionHeightsRelativeToBase;
    case AIProp::Enemy_VisionAttachBone: return &Enemy_VisionAttachBone;
    case AIProp::Sentry_SpawnAnimClip: return &Sentry_SpawnAnimClip;
    case AIProp::Sentry_IdleAnimClip: return &Sentry_IdleAnimClip;
    case AIProp::Sentry_FormationOffset: return &Sentry_FormationOffset;
    case AIProp::Sentry_Awareness: return &Sentry_Awareness;
    case AIProp::Sentry_AudioAlertRadius: return &Sentry_AudioAlertRadius;
    case AIProp::Sentry_AudioAlertWhileALertFail: return &Sentry_AudioAlertWhileALertFail;
    case AIProp::Sentry_AudioAlertHeightMin: return &Sentry_AudioAlertHeightMin;
    case AIProp::Sentry_AudioAlertHeightMax: return &Sentry_AudioAlertHeightMax;
    case AIProp::Sentry_VisionDistance: return &Sentry_VisionDistance;
    case AIProp::Sentry_VisionFOVAngle: return &Sentry_VisionFOVAngle;
    case AIProp::Sentry_VisionHeightMin: return &Sentry_VisionHeightMin;
    case AIProp::Sentry_VisionHeightMax: return &Sentry_VisionHeightMax;
    case AIProp::Sentry_VisionAttachBone: return &Sentry_VisionAttachBone;
    case AIProp::Sentry_SneakAllowed: return &Sentry_SneakAllowed;
    case AIProp::Sentry_DetectionCylinderRadius: return &Sentry_DetectionCylinderRadius;
    case AIProp::Sentry_DetectionCylinderHeight: return &Sentry_DetectionCylinderHeight;
    case AIProp::Sentry_DetectionCylinderYOffset: return &Sentry_DetectionCylinderYOffset;
    case AIProp::Sentry_AlwaysApplyGravity: return &Sentry_AlwaysApplyGravity;
    case AIProp::Sentry_OrientToTerrain: return &Sentry_OrientToTerrain;
    default: return nullptr;
    }
}

// -- per-parameter accessors (0x00734114..0x00735340) --------------------------

using ape::setting::Setting;

namespace critter_setting {

// @func 0x00734114 CritterSetting::SpawnAnimClip
uint32_t SpawnAnimClip(const Setting& setting) { return setting.Value(Critter_SpawnAnimClip).u; }
// @func 0x00734198 CritterSetting::SpawnPosIsDst
bool SpawnPosIsDst(const Setting& setting) { return setting.Value(Critter_SpawnPosIsDst).b; }
// @func 0x00734270 CritterSetting::UseFlightAnims
bool UseFlightAnims(const Setting& setting) { return setting.Value(Critter_UseFlightAnims).b; }
// @func 0x00734290 CritterSetting::POIDelayMin
float POIDelayMin(const Setting& setting) { return setting.Value(Critter_POIDelayMin).f; }
// @func 0x007342B0 CritterSetting::POIDelayMax
float POIDelayMax(const Setting& setting) { return setting.Value(Critter_POIDelayMax).f; }

} // namespace critter_setting

namespace enemy_setting {

// @func 0x007349C0 EnemySetting::OptimalTargetRangeMax
float OptimalTargetRangeMax(const Setting& setting) { return setting.Value(Enemy_OptimalTargetRangeMax).f; }
// @func 0x007349E0 EnemySetting::Cover
bool Cover(const Setting& setting) { return setting.Value(Enemy_Cover).b; }
// @func 0x00734A00 EnemySetting::Heavy
bool Heavy(const Setting& setting) { return setting.Value(Enemy_Heavy).b; }
// @func 0x00734A20 EnemySetting::LoseTargetTime
float LoseTargetTime(const Setting& setting) { return setting.Value(Enemy_LoseTargetTime).f; }
// @func 0x00734A40 EnemySetting::FarAttackRange
float FarAttackRange(const Setting& setting) { return setting.Value(Enemy_FarAttackRange).f; }
// @func 0x00734A60 EnemySetting::RangedAttackChaseFrequency
float RangedAttackChaseFrequency(const Setting& setting) { return setting.Value(Enemy_RangedAttackChaseFrequency).f; }
// @func 0x00734BD8 EnemySetting::AttackRequiresLOS
bool AttackRequiresLOS(const Setting& setting) { return setting.Value(Enemy_AttackRequiresLOS).b; }
// @func 0x00734BF8 EnemySetting::CounterAttackRange
float CounterAttackRange(const Setting& setting) { return setting.Value(Enemy_CounterAttackRange).f; }
// @func 0x00734C18 EnemySetting::OptimalTargetRangeMin
float OptimalTargetRangeMin(const Setting& setting) { return setting.Value(Enemy_OptimalTargetRangeMin).f; }
// @func 0x00734C38 EnemySetting::CombatAttackFarAnim
uint32_t CombatAttackFarAnim(const Setting& setting) { return setting.Value(Enemy_CombatAttackFarAnim).u; }
// @func 0x00734C58 EnemySetting::CombatAttackRangedAnim
uint32_t CombatAttackRangedAnim(const Setting& setting) { return setting.Value(Enemy_CombatAttackRangedAnim).u; }
// @func 0x00734C78 EnemySetting::Patrol
bool Patrol(const Setting& setting) { return setting.Value(Enemy_Patrol).b; }
// @func 0x00734C98 EnemySetting::AudioAlertInvestigate
bool AudioAlertInvestigate(const Setting& setting) { return setting.Value(Enemy_AudioAlertInvestigate).b; }
// @func 0x00734DD8 EnemySetting::Scale
float Scale(const Setting& setting) { return setting.Value(Enemy_Scale).f; }
// @func 0x00734F80 EnemySetting::PatrolIdleMinTime
float PatrolIdleMinTime(const Setting& setting) { return setting.Value(Enemy_PatrolIdleMinTime).f; }
// @func 0x00734FA0 EnemySetting::PatrolIdleMaxTime
float PatrolIdleMaxTime(const Setting& setting) { return setting.Value(Enemy_PatrolIdleMaxTime).f; }
// @func 0x00734FE0 EnemySetting::CombatOtherAnim
uint32_t CombatOtherAnim(const Setting& setting) { return setting.Value(Enemy_CombatOtherAnim).u; }
// @func 0x00735030 EnemySetting::CombatAttackCloseAnim
uint32_t CombatAttackCloseAnim(const Setting& setting) { return setting.Value(Enemy_CombatAttackCloseAnim).u; }
// @func 0x00735050 EnemySetting::CombatAttackMidAnim
uint32_t CombatAttackMidAnim(const Setting& setting) { return setting.Value(Enemy_CombatAttackMidAnim).u; }
// @func 0x00735070 EnemySetting::CloseAttackRange
float CloseAttackRange(const Setting& setting) { return setting.Value(Enemy_CloseAttackRange).f; }
// @func 0x00735090 EnemySetting::CloseAttackAngle
float CloseAttackAngle(const Setting& setting) { return setting.Value(Enemy_CloseAttackAngle).f; }
// @func 0x007350B0 EnemySetting::CloseAttackHeightMin
float CloseAttackHeightMin(const Setting& setting) { return setting.Value(Enemy_CloseAttackHeightMin).f; }
// @func 0x007350D0 EnemySetting::CloseAttackHeightMax
float CloseAttackHeightMax(const Setting& setting) { return setting.Value(Enemy_CloseAttackHeightMax).f; }
// @func 0x007350F0 EnemySetting::MidAttackRange
float MidAttackRange(const Setting& setting) { return setting.Value(Enemy_MidAttackRange).f; }
// @func 0x00735110 EnemySetting::MidAttackAngle
float MidAttackAngle(const Setting& setting) { return setting.Value(Enemy_MidAttackAngle).f; }
// @func 0x00735130 EnemySetting::MidAttackHeightMin
float MidAttackHeightMin(const Setting& setting) { return setting.Value(Enemy_MidAttackHeightMin).f; }
// @func 0x00735150 EnemySetting::MidAttackHeightMax
float MidAttackHeightMax(const Setting& setting) { return setting.Value(Enemy_MidAttackHeightMax).f; }
// @func 0x00735170 EnemySetting::FarAttackAngle
float FarAttackAngle(const Setting& setting) { return setting.Value(Enemy_FarAttackAngle).f; }
// @func 0x00735190 EnemySetting::FarAttackHeightMin
float FarAttackHeightMin(const Setting& setting) { return setting.Value(Enemy_FarAttackHeightMin).f; }
// @func 0x007351B0 EnemySetting::FarAttackHeightMax
float FarAttackHeightMax(const Setting& setting) { return setting.Value(Enemy_FarAttackHeightMax).f; }
// @func 0x007351D0 EnemySetting::RangedAttackRange
float RangedAttackRange(const Setting& setting) { return setting.Value(Enemy_RangedAttackRange).f; }
// @func 0x007351F0 EnemySetting::RangedAttackAngle
float RangedAttackAngle(const Setting& setting) { return setting.Value(Enemy_RangedAttackAngle).f; }
// @func 0x00735210 EnemySetting::RangedAttackHeightMin
float RangedAttackHeightMin(const Setting& setting) { return setting.Value(Enemy_RangedAttackHeightMin).f; }
// @func 0x00735230 EnemySetting::RangedAttackHeightMax
float RangedAttackHeightMax(const Setting& setting) { return setting.Value(Enemy_RangedAttackHeightMax).f; }
// @func 0x00735250 EnemySetting::TurretAttackMaxTime
float TurretAttackMaxTime(const Setting& setting) { return setting.Value(Enemy_TurretAttackMaxTime).f; }
// @func 0x00735270 EnemySetting::AttackControllerUseTimer
bool AttackControllerUseTimer(const Setting& setting) { return setting.Value(Enemy_AttackControllerUseTimer).b; }
// @func 0x00735290 EnemySetting::CombatMovementAnim
uint32_t CombatMovementAnim(const Setting& setting) { return setting.Value(Enemy_CombatMovementAnim).u; }
// @func 0x007352B0 EnemySetting::TurretAttackMinTime
float TurretAttackMinTime(const Setting& setting) { return setting.Value(Enemy_TurretAttackMinTime).f; }
// @func 0x00735340 EnemySetting::IdleAnimClip
uint32_t IdleAnimClip(const Setting& setting) { return setting.Value(Enemy_IdleAnimClip).u; }

} // namespace enemy_setting

} // namespace ai
} // namespace levels
} // namespace sly
