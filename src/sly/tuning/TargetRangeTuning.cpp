// openTIT - Target range and player turret tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/TargetRangeTuning.h"

#include <limits>

namespace sly {

// @func 0x001F78C4 StaticInit_TargetRangeTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x01052B80).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningVec3 g_playerTurretRotAccel("Entity.PlayerTurret.RotAccel", 9.5f, 9.5f, 0.0f);
ape::TuningVec3 g_playerTurretRotDamping("Entity.PlayerTurret.RotDamping", 0.15f, 0.15f, 1.0f);
ape::TuningVec3 g_playerTurretRotMax("Entity.PlayerTurret.RotMax", 2.0f, 2.0f, 0.0f);
ape::TuningFloat g_playerTurretCameraEaseInTime("Entity.PlayerTurret.Camera.EaseInTime", 1.5f);
ape::TuningFloat g_playerTurretCameraEaseOutTime("Entity.PlayerTurret.Camera.EaseOutTime", 2.0f);
ape::TuningFloat g_playerTurretCameraFOV("Entity.PlayerTurret.Camera.FOV", 5e+01f);
ape::TuningBool g_playerTurretShowDebug("Entity.PlayerTurret.ShowDebug", false);
ape::TuningBool g_playerTurretAdditiveHitReact("Entity.PlayerTurret.AdditiveHitReact", false);
ape::TuningBool g_playerTurretAlwaysUpdateReticleProbe("Entity.PlayerTurret.AlwaysUpdateReticleProbe", true);
ape::TuningFloat g_playerTurretMsgDisplayTime("Entity.PlayerTurret.MsgDisplayTime", 5.0f);
ape::TuningString g_playerTurretMsgFire("Entity.PlayerTurret.MsgFire", "$BUTTON_R1_ANIM $STR_SHOOT");
ape::TuningString g_playerTurretMsgMove("Entity.PlayerTurret.MsgMove", "$GENERICSTICK_CIRCLE_ANIM $STR_AIM");
ape::TuningFloat g_safeCountDownBeepStartTime("Entity.Safe.CountDownBeepStartTime", 1e+01f);
ape::TuningBool g_tgtRangeDebugRender("Entity.TgtRange.DebugRender", false);
ape::TuningBool g_tgtRangeInputR2vsR1("Entity.TgtRange.InputR2vsR1", false);
ape::TuningFloat g_tgtRangeChargeRumbleStrength("Entity.TgtRange.ChargeRumbleStrength", 0.2f);
ape::TuningFloat g_tgtRangeShotRumbleStrength("Entity.TgtRange.ShotRumbleStrength", 0.4f);
ape::TuningFloat g_tgtRangeShotRumbleTime("Entity.TgtRange.ShotRumbleTime", 0.5f);
ape::TuningFloat g_tgtRangeFailRumbleStrength("Entity.TgtRange.FailRumbleStrength", 0.4f);
ape::TuningFloat g_tgtRangeFailRumbleTime("Entity.TgtRange.FailRumbleTime", 0.5f);
ape::TuningFloat g_tgtRangeSuccessRumbleStrength("Entity.TgtRange.SuccessRumbleStrength", 0.4f);
ape::TuningFloat g_tgtRangeSuccessRumbleTime("Entity.TgtRange.SuccessRumbleTime", 0.5f);
ape::TuningFloat g_tgtRangeAttachSpeed("Entity.TgtRange.AttachSpeed", 1.0f);
ape::TuningVec3 g_tgtRangeCamOffset("Entity.TgtRange.CamOffset", -1.3f, 1.55f, -2.8f);
ape::TuningVec3 g_tgtRangeFireOffset("Entity.TgtRange.FireOffset", 0.0f, 1.64f, 2.0f);
ape::TuningFloat g_tgtRangeCamFOVDefault("Entity.TgtRange.CamFOVDefault", 6e+01f);
ape::TuningFloat g_tgtRangeCamFOVAim("Entity.TgtRange.CamFOVAim", 44.0f);
ape::TuningFloat g_tgtRangeCamPopTime("Entity.TgtRange.CamPopTime", 0.5f);
ape::TuningFloat g_tgtRangeUnchargeThreshold("Entity.TgtRange.UnchargeThreshold", 0.55f);
ape::TuningFloat g_tgtRangeChargePointOfNoReturn("Entity.TgtRange.ChargePointOfNoReturn", 0.9f);
ape::TuningFloat g_tgtRangeArrowPenetration("Entity.TgtRange.ArrowPenetration", 0.25f);
ape::TuningVec2 g_tgtRangeAimRotFriction("Entity.TgtRange.AimRot.Friction", 0.3f, 0.3f);
ape::TuningVec2 g_tgtRangeAimRotAccel("Entity.TgtRange.AimRot.Accel", 1.08e+03f, 9e+02f);
ape::TuningFloat g_tgtRangeAimRotAccelRampTime("Entity.TgtRange.AimRot.AccelRampTime", 1.0f);
ape::TuningVec2 g_tgtRangeAimRotSpeedMax("Entity.TgtRange.AimRot.SpeedMax", 1.08e+03f, 9e+02f);
ape::TuningVec2 g_tgtRangeAimRotAngleMin("Entity.TgtRange.AimRot.AngleMin", -5e+01f, -25.0f);
ape::TuningVec2 g_tgtRangeAimRotAngleMax("Entity.TgtRange.AimRot.AngleMax", 5e+01f, 25.0f);
ape::TuningFloat g_tgtRangeAimRotFOVScalar("Entity.TgtRange.AimRot.FOVScalar", 0.035f);
ape::TuningFloat g_tgtRangeRangeMax("Entity.TgtRange.RangeMax", 3e+02f);
ape::TuningFloat g_tgtRangeAimInterpCharge("Entity.TgtRange.AimInterpCharge", 0.5f);
ape::TuningFloat g_tgtRangeAimInterpUncharge("Entity.TgtRange.AimInterpUncharge", 1.5f);
ape::TuningFloat g_tgtRangeAimInterp("Entity.TgtRange.AimInterp", 0.1f);
ape::TuningFloat g_tgtRangeLockOnTargetRadius("Entity.TgtRange.LockOnTargetRadius", 0.5f);
ape::TuningFloat g_tgtRangeArrowCollision("Entity.TgtRange.ArrowCollision", 2.0f);
ape::TuningFloat g_tgtRangeReleaseDelay("Entity.TgtRange.ReleaseDelay", 0.13f);
ape::TuningFloat g_tgtRangeShowArrowDelay("Entity.TgtRange.ShowArrowDelay", 0.48f);
ape::TuningFloat g_tgtRangeChargeSpeed("Entity.TgtRange.ChargeSpeed", 1.25f);
ape::TuningFloat g_tgtRangeAimOffsetVisual("Entity.TgtRange.AimOffsetVisual", 2e+01f);
ape::TuningFloat g_tgtRangeReticleStartSpeed("Entity.TgtRange.ReticleStartSpeed", 0.672f);
ape::TuningFloat g_tgtRangeReticleEndSpeed("Entity.TgtRange.ReticleEndSpeed", 1.12f);
ape::TuningVec3 g_tgtRangeArrowStartOffset("Entity.TgtRange.ArrowStartOffset", 0.0f, 0.0f, 3.25f);
ape::TuningFloat g_tgtRangeCamPauseTime("Entity.TgtRange.CamPauseTime", 0.5f);
ape::TuningFloat g_tgtRangeCamPauseSpeed("Entity.TgtRange.CamPauseSpeed", 1.5f);
ape::TuningInt g_tgtRangeFlightInputMode("Entity.TgtRange.Flight.InputMode", 0);
ape::TuningInt g_tgtRangeFlightDanglingSegments("Entity.TgtRange.Flight.DanglingSegments", 9);
ape::TuningVec3 g_tgtRangeFlightCamOffset("Entity.TgtRange.Flight.CamOffset", 0.0f, 0.2f, -3.25f);
ape::TuningVec3 g_tgtRangeFlightCamRotation("Entity.TgtRange.Flight.CamRotation", 0.0f, 0.0f, 0.0f);
ape::TuningFloat g_tgtRangeFlightAimVelocity("Entity.TgtRange.Flight.AimVelocity", 0.2f);
ape::TuningFloat g_tgtRangeFlightAimAccel("Entity.TgtRange.Flight.AimAccel", 1.15f);
ape::TuningFloat g_tgtRangeFlightDuration("Entity.TgtRange.Flight.Duration", 5.0f);
ape::TuningFloat g_tgtRangeFlightAimFriction("Entity.TgtRange.Flight.AimFriction", 5.0f);
ape::TuningInt g_tgtRangeFlightRopeAlpha("Entity.TgtRange.Flight.RopeAlpha", 64);
ape::TuningFloat g_tgtRangeCutRopeColSizeSqr("Entity.TgtRange.CutRope.ColSizeSqr", 0.05f);
ape::TuningFloat g_tgtRangeCutRopeFadeout("Entity.TgtRange.CutRope.Fadeout", 1.5f);
ape::TuningVec2 g_tgtRangeFlightAimAngleMin("Entity.TgtRange.Flight.AimAngleMin", 1.0f, 1.0f);
ape::TuningVec2 g_tgtRangeFlightAimAccelScalarMotion("Entity.TgtRange.Flight.AimAccelScalarMotion", 35.0f, 35.0f);
ape::TuningVec2 g_tgtRangeFlightAimAccelScalarStick("Entity.TgtRange.Flight.AimAccelScalarStick", 2e+01f, 16.6f);
ape::TuningVec2 g_tgtRangeFlightInputSwitchAccel("Entity.TgtRange.Flight.InputSwitchAccel", 3.0f, 3.0f);
ape::TuningFloat g_tgtRangeMatOffset("Entity.TgtRange.MatOffset", 0.5f);
ape::TuningFloat g_tgtRangeBoneModAimOffset("Entity.TgtRange.BoneModAimOffset", 0.0f);
ape::TuningBool g_tgtRangeBoneModAimEnabled("Entity.TgtRange.BoneModAimEnabled", true);
ape::TuningBool g_timedStealDebugRender("Entity.TimedSteal.DebugRender", false);
ape::TuningFloat g_timedStealLookAtObjSpeed("Entity.TimedSteal.LookAtObjSpeed", 0.25f);

} // namespace sly
