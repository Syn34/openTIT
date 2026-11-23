// openTIT - Target range and player turret tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningVec3 g_playerTurretRotAccel;
extern ape::TuningVec3 g_playerTurretRotDamping;
extern ape::TuningVec3 g_playerTurretRotMax;
extern ape::TuningFloat g_playerTurretCameraEaseInTime;
extern ape::TuningFloat g_playerTurretCameraEaseOutTime;
extern ape::TuningFloat g_playerTurretCameraFOV;
extern ape::TuningBool g_playerTurretShowDebug;
extern ape::TuningBool g_playerTurretAdditiveHitReact;
extern ape::TuningBool g_playerTurretAlwaysUpdateReticleProbe;
extern ape::TuningFloat g_playerTurretMsgDisplayTime;
extern ape::TuningString g_playerTurretMsgFire;
extern ape::TuningString g_playerTurretMsgMove;
extern ape::TuningFloat g_safeCountDownBeepStartTime;
extern ape::TuningBool g_tgtRangeDebugRender;
extern ape::TuningBool g_tgtRangeInputR2vsR1;
extern ape::TuningFloat g_tgtRangeChargeRumbleStrength;
extern ape::TuningFloat g_tgtRangeShotRumbleStrength;
extern ape::TuningFloat g_tgtRangeShotRumbleTime;
extern ape::TuningFloat g_tgtRangeFailRumbleStrength;
extern ape::TuningFloat g_tgtRangeFailRumbleTime;
extern ape::TuningFloat g_tgtRangeSuccessRumbleStrength;
extern ape::TuningFloat g_tgtRangeSuccessRumbleTime;
extern ape::TuningFloat g_tgtRangeAttachSpeed;
extern ape::TuningVec3 g_tgtRangeCamOffset;
extern ape::TuningVec3 g_tgtRangeFireOffset;
extern ape::TuningFloat g_tgtRangeCamFOVDefault;
extern ape::TuningFloat g_tgtRangeCamFOVAim;
extern ape::TuningFloat g_tgtRangeCamPopTime;
extern ape::TuningFloat g_tgtRangeUnchargeThreshold;
extern ape::TuningFloat g_tgtRangeChargePointOfNoReturn;
extern ape::TuningFloat g_tgtRangeArrowPenetration;
extern ape::TuningVec2 g_tgtRangeAimRotFriction;
extern ape::TuningVec2 g_tgtRangeAimRotAccel;
extern ape::TuningFloat g_tgtRangeAimRotAccelRampTime;
extern ape::TuningVec2 g_tgtRangeAimRotSpeedMax;
extern ape::TuningVec2 g_tgtRangeAimRotAngleMin;
extern ape::TuningVec2 g_tgtRangeAimRotAngleMax;
extern ape::TuningFloat g_tgtRangeAimRotFOVScalar;
extern ape::TuningFloat g_tgtRangeRangeMax;
extern ape::TuningFloat g_tgtRangeAimInterpCharge;
extern ape::TuningFloat g_tgtRangeAimInterpUncharge;
extern ape::TuningFloat g_tgtRangeAimInterp;
extern ape::TuningFloat g_tgtRangeLockOnTargetRadius;
extern ape::TuningFloat g_tgtRangeArrowCollision;
extern ape::TuningFloat g_tgtRangeReleaseDelay;
extern ape::TuningFloat g_tgtRangeShowArrowDelay;
extern ape::TuningFloat g_tgtRangeChargeSpeed;
extern ape::TuningFloat g_tgtRangeAimOffsetVisual;
extern ape::TuningFloat g_tgtRangeReticleStartSpeed;
extern ape::TuningFloat g_tgtRangeReticleEndSpeed;
extern ape::TuningVec3 g_tgtRangeArrowStartOffset;
extern ape::TuningFloat g_tgtRangeCamPauseTime;
extern ape::TuningFloat g_tgtRangeCamPauseSpeed;
extern ape::TuningInt g_tgtRangeFlightInputMode;
extern ape::TuningInt g_tgtRangeFlightDanglingSegments;
extern ape::TuningVec3 g_tgtRangeFlightCamOffset;
extern ape::TuningVec3 g_tgtRangeFlightCamRotation;
extern ape::TuningFloat g_tgtRangeFlightAimVelocity;
extern ape::TuningFloat g_tgtRangeFlightAimAccel;
extern ape::TuningFloat g_tgtRangeFlightDuration;
extern ape::TuningFloat g_tgtRangeFlightAimFriction;
extern ape::TuningInt g_tgtRangeFlightRopeAlpha;
extern ape::TuningFloat g_tgtRangeCutRopeColSizeSqr;
extern ape::TuningFloat g_tgtRangeCutRopeFadeout;
extern ape::TuningVec2 g_tgtRangeFlightAimAngleMin;
extern ape::TuningVec2 g_tgtRangeFlightAimAccelScalarMotion;
extern ape::TuningVec2 g_tgtRangeFlightAimAccelScalarStick;
extern ape::TuningVec2 g_tgtRangeFlightInputSwitchAccel;
extern ape::TuningFloat g_tgtRangeMatOffset;
extern ape::TuningFloat g_tgtRangeBoneModAimOffset;
extern ape::TuningBool g_tgtRangeBoneModAimEnabled;
extern ape::TuningBool g_timedStealDebugRender;
extern ape::TuningFloat g_timedStealLookAtObjSpeed;

} // namespace sly
