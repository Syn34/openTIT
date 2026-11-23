// openTIT - Mega knight and dragon boss tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/MegaKnightDragonTuning.h"

#include <limits>

namespace sly {

// @func 0x0031C128 StaticInit_MegaKnightDragonTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x01068B80).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningBool g_dragonBossDebugEnable("Entity.DragonBoss.DebugEnable", false);
ape::TuningBool g_dragonBossHead1Alive("Entity.DragonBoss.Head1Alive", true);
ape::TuningBool g_dragonBossHead2Alive("Entity.DragonBoss.Head2Alive", true);
ape::TuningBool g_dragonBossHead3Alive("Entity.DragonBoss.Head3Alive", true);
ape::TuningBool g_dragonBossPhaseOver("Entity.DragonBoss.PhaseOver", false);
ape::TuningFloat g_dragonBossDefaultAnimBlend("Entity.DragonBoss.DefaultAnimBlend", 0.5f);
ape::TuningVec3 g_dragonBossPhase1Speed("Entity.DragonBoss.Phase1.Speed", 1.0f, 1.0f, 1.0f);
ape::TuningVec3 g_dragonBossPhase2Speed("Entity.DragonBoss.Phase2.Speed", 1.0f, 1.0f, 1.0f);
ape::TuningBool g_dragonBossContextualHelpEnabled("Entity.DragonBoss.ContextualHelpEnabled", true);
ape::TuningInt g_dragonBossPhase1HeadshotMashes("Entity.DragonBoss.Phase1.HeadshotMashes", 5);
ape::TuningInt g_dragonBossPhase2HeadshotMashes("Entity.DragonBoss.Phase2.HeadshotMashes", 5);
ape::TuningFloat g_dragonBossPhase1HeadshotDuration("Entity.DragonBoss.Phase1.HeadshotDuration", 5.0f);
ape::TuningFloat g_dragonBossPhase2HeadshotDuration("Entity.DragonBoss.Phase2.HeadshotDuration", 5.0f);
ape::TuningFloat g_dragonBossHeadshotCooldown("Entity.DragonBoss.HeadshotCooldown", 0.25f);
ape::TuningFloat g_dragonBossFinishMoveRadius("Entity.DragonBoss.FinishMoveRadius", 5.0f);
ape::TuningInt g_dragonBossPhase1HealthPercent("Entity.DragonBoss.Phase1.HealthPercent", 100);
ape::TuningInt g_dragonBossPhase2HealthPercent("Entity.DragonBoss.Phase2.HealthPercent", 100);
ape::TuningFloat g_dragonBossWindStr("Entity.DragonBoss.WindStr", 1e+04f);
ape::TuningFloat g_dragonBossWindStrVita("Entity.DragonBoss.WindStrVita", 1e+04f);
ape::TuningFloat g_dragonBossRampUp("Entity.DragonBoss.RampUp", 2.5f);
ape::TuningFloat g_dragonBossRampDown("Entity.DragonBoss.RampDown", 2.5f);
ape::TuningFloat g_dragonBossAirDampeningPercent("Entity.DragonBoss.AirDampeningPercent", 0.1f);
ape::TuningFloat g_dragonBossAirDampeningTime("Entity.DragonBoss.AirDampeningTime", 0.45f);
ape::TuningVec3 g_dragonBossFireballDepthOffset("Entity.DragonBoss.FireballDepthOffset", 0.0f, 0.0f, 2.0f);
ape::TuningVec3 g_dragonBossHomingDepthOffset("Entity.DragonBoss.HomingDepthOffset", 0.0f, 0.5f, 0.55f);
ape::TuningVec3 g_dragonBossHomingUpDir("Entity.DragonBoss.HomingUpDir", 0.5f, 0.5f, 0.0f);
ape::TuningVec3 g_dragonBossStunMarkerOffset("Entity.DragonBoss.StunMarkerOffset", 0.0f, 0.0f, 0.0f);
ape::TuningFloat g_dragonBossFireballBTargetOffset("Entity.DragonBoss.FireballBTargetOffset", 2.0f);
ape::TuningFloat g_dragonBossFireballSrcOffset("Entity.DragonBoss.FireballSrcOffset", 3.0f);
ape::TuningFloat g_dragonBossMIRVSpeedMultiplier("Entity.DragonBoss.MIRVSpeedMultiplier", 1.0f);
ape::TuningFloat g_dragonBossMIRVStartTime("Entity.DragonBoss.MIRVStartTime", 0.1f);
ape::TuningInt g_dragonBossPhase1HeadHP("Entity.DragonBoss.Phase1.HeadHP", 3);
ape::TuningFloat g_dragonBossPhase1FireballASpeed("Entity.DragonBoss.Phase1.FireballASpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase1FireballAGravity("Entity.DragonBoss.Phase1.FireballAGravity", 1.5f);
ape::TuningFloat g_dragonBossPhase1FireballBSpeed("Entity.DragonBoss.Phase1.FireballBSpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase1FireballBGravity("Entity.DragonBoss.Phase1.FireballBGravity", 1.5f);
ape::TuningFloat g_dragonBossPhase1FireballCSpeed("Entity.DragonBoss.Phase1.FireballCSpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase1FireballCGravity("Entity.DragonBoss.Phase1.FireballCGravity", 1.5f);
ape::TuningFloat g_dragonBossPhase2FireballTime("Entity.DragonBoss.Phase2.FireballTime", 2e+01f);
ape::TuningInt g_dragonBossPhase2HeadHP("Entity.DragonBoss.Phase2.HeadHP", 3);
ape::TuningFloat g_dragonBossPhase2FireballASpeed("Entity.DragonBoss.Phase2.FireballASpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase2FireballAGravity("Entity.DragonBoss.Phase2.FireballAGravity", 1.5f);
ape::TuningFloat g_dragonBossPhase2FireballBSpeed("Entity.DragonBoss.Phase2.FireballBSpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase2FireballBGravity("Entity.DragonBoss.Phase2.FireballBGravity", 1.5f);
ape::TuningFloat g_dragonBossPhase2FireballCSpeed("Entity.DragonBoss.Phase2.FireballCSpeed", 2e+01f);
ape::TuningFloat g_dragonBossPhase2FireballCGravity("Entity.DragonBoss.Phase2.FireballCGravity", 1.5f);
ape::TuningString g_megaKnightMKState("Entity.MegaKnight.MKState", "[**null**]");
ape::TuningBool g_megaKnightStateLocked("Entity.MegaKnight.StateLocked", false);
ape::TuningBool g_megaKnightDamageHeart("Entity.MegaKnight.DamageHeart", false);
ape::TuningBool g_megaKnightForceWinPhase("Entity.MegaKnight.ForceWinPhase", false);
ape::TuningBool g_megaKnightDamageShoudlerLeft("Entity.MegaKnight.DamageShoudlerLeft", false);
ape::TuningBool g_megaKnightDamageShoudlerRight("Entity.MegaKnight.DamageShoudlerRight", false);
ape::TuningBool g_megaKnightIKEnabled("Entity.MegaKnight.IKEnabled", true);
ape::TuningFloat g_megaKnightIKBlend("Entity.MegaKnight.IKBlend", 24.0f);
ape::TuningBool g_megaKnightAimLayerLeftArmEnabled("Entity.MegaKnight.AimLayerLeftArmEnabled", true);
ape::TuningBool g_megaKnightAimLayerRightArmEnabled("Entity.MegaKnight.AimLayerRightArmEnabled", true);
ape::TuningBool g_megaKnightDebugEnable("Entity.MegaKnight.DebugEnable", true);
ape::TuningFloat g_megaKnightAnimBlend("Entity.MegaKnight.AnimBlend", 0.25f);
ape::TuningFloat g_megaKnightAnimSpeedMultiplier("Entity.MegaKnight.AnimSpeedMultiplier", 1.0f);
ape::TuningFloat g_megaKnightFinishingMoveRadius("Entity.MegaKnight.FinishingMoveRadius", 7.5f);
ape::TuningFloat g_megaKnightArrowBucketRadius("Entity.MegaKnight.ArrowBucketRadius", 7.5f);
ape::TuningVec3 g_megaKnightMarkerOffsetTrampoline("Entity.MegaKnight.MarkerOffsetTrampoline", 0.0f, 5.0f, 0.0f);
ape::TuningVec3 g_megaKnightMarkerOffsetShoulder("Entity.MegaKnight.MarkerOffsetShoulder", 0.0f, 5.0f, 0.0f);
ape::TuningVec3 g_megaKnightMarkerOffsetHeart("Entity.MegaKnight.MarkerOffsetHeart", 0.0f, 1.5f, 7.0f);
ape::TuningFloat g_megaKnightStartIdle("Entity.MegaKnight.StartIdle", 1.0f);
ape::TuningFloat g_megaKnightIKHeightMod("Entity.MegaKnight.IKHeightMod", -0.25f);
ape::TuningInt g_megaKnightPhase1HealthPercent("Entity.MegaKnight.Phase1HealthPercent", 100);
ape::TuningInt g_megaKnightPhase2HealthPercent("Entity.MegaKnight.Phase2HealthPercent", 100);
ape::TuningFloat g_megaKnightTimeOffPlatformVODelay("Entity.MegaKnight.TimeOffPlatformVODelay", 3e+01f);
ape::TuningFloat g_megaKnightTooClosetoBossVODistSqr("Entity.MegaKnight.TooClosetoBossVODistSqr", 5.0f);
ape::TuningFloat g_megaKnightStateValuesGroundHeightLimit("Entity.MegaKnight.StateValues.GroundHeightLimit", 12.0f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase1Arm2("Entity.MegaKnight.StateValues.MonitorPlatform.Phase1Arm2", 7.5f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase1Arm1("Entity.MegaKnight.StateValues.MonitorPlatform.Phase1Arm1", 7.5f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase1Arm0("Entity.MegaKnight.StateValues.MonitorPlatform.Phase1Arm0", 7.5f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase2Arm2("Entity.MegaKnight.StateValues.MonitorPlatform.Phase2Arm2", 7.5f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase2Arm1("Entity.MegaKnight.StateValues.MonitorPlatform.Phase2Arm1", 7.5f);
ape::TuningFloat g_megaKnightStateValuesMonitorPlatformPhase2Arm0("Entity.MegaKnight.StateValues.MonitorPlatform.Phase2Arm0", 7.5f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCamDistFromPlayer("Entity.MegaKnight.StateValues.LaserCutter.CamDistFromPlayer", 2e+01f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCamHeightAbovePlayer("Entity.MegaKnight.StateValues.LaserCutter.CamHeightAbovePlayer", 2.5f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCamHeightAboveBoss("Entity.MegaKnight.StateValues.LaserCutter.CamHeightAboveBoss", 3.4f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCamDistInFrontOfBoss("Entity.MegaKnight.StateValues.LaserCutter.CamDistInFrontOfBoss", 0.0f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCamEaseIn("Entity.MegaKnight.StateValues.LaserCutter.CamEaseIn", 0.1f);
ape::TuningFloat g_megaKnightStateValuesShoulderHitIdleDuration("Entity.MegaKnight.StateValues.ShoulderHit.IdleDuration", 1.5f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase1Arm2("Entity.MegaKnight.StateValues.LaserCutter.Phase1Arm2", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase1Arm1("Entity.MegaKnight.StateValues.LaserCutter.Phase1Arm1", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase1Arm0("Entity.MegaKnight.StateValues.LaserCutter.Phase1Arm0", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase2Arm2("Entity.MegaKnight.StateValues.LaserCutter.Phase2Arm2", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase2Arm1("Entity.MegaKnight.StateValues.LaserCutter.Phase2Arm1", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterPhase2Arm0("Entity.MegaKnight.StateValues.LaserCutter.Phase2Arm0", 0.55f);
ape::TuningFloat g_megaKnightStateValuesLaserCutterCutCancelDelay("Entity.MegaKnight.StateValues.LaserCutter.CutCancelDelay", 0.75f);
ape::TuningFloat g_megaKnightStateValuesShootPlatformStartupSpeed("Entity.MegaKnight.StateValues.ShootPlatform.StartupSpeed", 3.5f);
ape::TuningFloat g_megaKnightStateValuesShootPlatformImpulseStr("Entity.MegaKnight.StateValues.ShootPlatform.ImpulseStr", 25.0f);
ape::TuningVec3 g_megaKnightLaserEmitOffset("Entity.MegaKnight.Laser.EmitOffset", 3.75f, -1.25f, 0.0f);
ape::TuningFloat g_megaKnightLaserDefaultDuration("Entity.MegaKnight.Laser.DefaultDuration", 1e+01f);
ape::TuningFloat g_megaKnightLaserTrackingSpeedFar("Entity.MegaKnight.Laser.TrackingSpeedFar", 15.75f);
ape::TuningFloat g_megaKnightLaserTrackingSpeedInterpDist("Entity.MegaKnight.Laser.TrackingSpeedInterpDist", 1e+01f);
ape::TuningFloat g_megaKnightLaserTrackingSpeedInterp("Entity.MegaKnight.Laser.TrackingSpeedInterp", 0.5f);
ape::TuningFloat g_megaKnightLaserTrackingSpeedClose("Entity.MegaKnight.Laser.TrackingSpeedClose", 8.75f);
ape::TuningFloat g_megaKnightLaserEmitDuration("Entity.MegaKnight.Laser.EmitDuration", 0.2f);
ape::TuningFloat g_megaKnightLaserDamageNormal("Entity.MegaKnight.Laser.DamageNormal", 2e+01f);
ape::TuningFloat g_megaKnightLaserDamageQuick("Entity.MegaKnight.Laser.DamageQuick", 2e+01f);
ape::TuningFloat g_megaKnightLaserDamagePlatform("Entity.MegaKnight.Laser.DamagePlatform", 2e+01f);
ape::TuningBool g_megaKnightLaserDamageEnabled("Entity.MegaKnight.Laser.DamageEnabled", false);
ape::TuningFloat g_megaKnightLaserStartingDistMax("Entity.MegaKnight.Laser.StartingDistMax", 5.0f);
ape::TuningFloat g_megaKnightLaserOvershoot("Entity.MegaKnight.Laser.Overshoot", 1.0f);
ape::TuningFloat g_megaKnightMissileLeadDist("Entity.MegaKnight.Missile.LeadDist", 5.0f);
ape::TuningFloat g_megaKnightMissileReticleHeight("Entity.MegaKnight.Missile.ReticleHeight", 0.1f);
ape::TuningFloat g_megaKnightMissileReticleScaleTime("Entity.MegaKnight.Missile.ReticleScaleTime", 0.15f);
ape::TuningFloat g_megaKnightMissileReticleScaleMin("Entity.MegaKnight.Missile.ReticleScaleMin", 0.15f);
ape::TuningFloat g_megaKnightMissileReticleScaleMax("Entity.MegaKnight.Missile.ReticleScaleMax", 2.0f);
ape::TuningBool g_megaKnightMissileReticleRotRandom("Entity.MegaKnight.Missile.ReticleRotRandom", true);
ape::TuningFloat g_megaKnightMissileReticleRotSpeed("Entity.MegaKnight.Missile.ReticleRotSpeed", 0.075f);
ape::TuningFloat g_megaKnightCameraHitFractionThreshold("Entity.MegaKnight.Camera.HitFractionThreshold", 0.5f);
ape::TuningFloat g_megaKnightCameraHitFractionSpeed("Entity.MegaKnight.Camera.HitFractionSpeed", 1.0f);
ape::TuningFloat g_megaKnightCameraColHeightOffsetMax("Entity.MegaKnight.Camera.ColHeightOffsetMax", 11.0f);
ape::TuningFloat g_megaKnightCameraColHeightOffsetRotMax("Entity.MegaKnight.Camera.ColHeightOffsetRotMax", 57.0f);
ape::TuningFloat g_megaKnightCameraMinHeight("Entity.MegaKnight.Camera.MinHeight", 1.0f);
ape::TuningFloat g_megaKnightFloorRippleDamage("Entity.MegaKnight.FloorRipple.Damage", 1e+01f);
ape::TuningBool g_megaKnightFloorRippleDamageEnabled("Entity.MegaKnight.FloorRipple.DamageEnabled", true);
ape::TuningFloat g_megaKnightFloorRippleMaxHeight("Entity.MegaKnight.FloorRipple.MaxHeight", 4.0f);
ape::TuningFloat g_megaKnightFloorRippleImpulseStr("Entity.MegaKnight.FloorRipple.ImpulseStr", 1e+01f);
ape::TuningFloat g_megaKnightFloorRippleHitCooldown("Entity.MegaKnight.FloorRipple.HitCooldown", 0.5f);

} // namespace sly
