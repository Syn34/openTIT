// openTIT - Tuning variables of the entity component module (0x002D6A40).
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace components {

extern ape::TuningInt g_debugDisplayCharAnim;

extern ape::TuningBool g_animationNoBlend;

extern ape::TuningInt g_characterMoveSpecialBallMode;
extern ape::TuningFloat g_characterMoveSpecialBallAccel;
extern ape::TuningFloat g_characterMoveSpecialBallDampingA;
extern ape::TuningFloat g_characterMoveSpecialBallFriction;
extern ape::TuningFloat g_characterMoveSpecialBallGravity;
extern ape::TuningFloat g_characterMoveSpecialBallJoyMin;
extern ape::TuningFloat g_characterMoveSpecialBallMass;
extern ape::TuningFloat g_characterMoveSpecialBallRestitution;
extern ape::TuningFloat g_characterMoveSpecialBallSpeedMax;
extern ape::TuningVec3 g_characterMoveSpecialBallTurnParams;
extern ape::TuningFloat g_characterMoveSpecialBallWalkSpeedMin;
extern ape::TuningFloat g_characterMoveSpecialBallWalkAnimSpeed;
extern ape::TuningFloat g_characterMoveSpecialBallRunSpeedMin;
extern ape::TuningFloat g_characterMoveSpecialBallRunAnimSpeed;
extern ape::TuningFloat g_characterMoveSpecialBallAnimTimeMin;
extern ape::TuningVec3 g_characterMoveSpecialBallCharOffset;
extern ape::TuningFloat g_characterMoveSpecialBallImpactDamage;
extern ape::TuningFloat g_characterMoveSpecialBallImpactImpulse;
extern ape::TuningFloat g_characterMoveSpecialBallSFXImpactSpeedMin;
extern ape::TuningFloat g_characterMoveSpecialBallSFXRollSpeedMin;
extern ape::TuningFloat g_characterMoveSpecialBallSFXRollPOffset;
extern ape::TuningFloat g_characterMoveSpecialBallSFXRollPFactor;
extern ape::TuningFloat g_characterMoveSpecialBallColOffsetMax;

extern ape::TuningHash g_binocucomDefaultShowSFX;
extern ape::TuningHash g_binocucomDefaultHideSFX;
extern ape::TuningHash g_binocucomDefaultAmbientSFX;
extern ape::TuningHash g_binocucomDefaultZoomInSFX;
extern ape::TuningHash g_binocucomDefaultZoomOutSFX;
extern ape::TuningHash g_binocucomDefaultPhotoSFX;
extern ape::TuningHash g_binocucomDefault3DPhotoAnimGeo;
extern ape::TuningHash g_binocucomDefault3DPhotoAnim;
extern ape::TuningString g_binocucomDefaultUIParentGroup;
extern ape::TuningString g_binocucomDefaultNGReticul;
extern ape::TuningString g_binocucomDefaultPartialReticle;
extern ape::TuningString g_binocucomDefaultFullReticle;
extern ape::TuningString g_binocucomDefaultReticleTitle;
extern ape::TuningString g_binocucomDefaultPhotoParentGroup;
extern ape::TuningString g_binocucomDefaultPhotoImage;
extern ape::TuningString g_binocucomDefaultPhotoTitle;
extern ape::TuningString g_binocucomDefaultPhotoDate;
extern ape::TuningString g_binocucomDefaultTarget;
extern ape::TuningString g_binocucomDefaultCheckMark;
extern ape::TuningString g_binocucomDefaultCheckBox;
extern ape::TuningHash g_binocucomDefault2DPhotoStartAnim;
extern ape::TuningHash g_binocucomDefault2DPhotoEndAnim;
extern ape::TuningFloat g_binocucomDefaultPhotoDisplayTime;
extern ape::TuningString g_binocucomDefaultHelpText;
extern ape::TuningString g_binocucomDefaultHelpTextSecondary;
extern ape::TuningHash g_binocucomDefaultInGameShowScript;
extern ape::TuningHash g_binocucomDefaultCinematicShowScript;
extern ape::TuningHash g_binocucomDefaultTutorialShowScript;
extern ape::TuningHash g_binocucomDefaultInGameHideScript;
extern ape::TuningHash g_binocucomDefaultCinematicHideScript;
extern ape::TuningHash g_binocucomDefaultTutorialHideScript;
extern ape::TuningHash g_binocucomDefaultShowLCharScript;
extern ape::TuningHash g_binocucomDefaultShowRCharScript;
extern ape::TuningHash g_binocucomDefaultShowStaticScript;
extern ape::TuningHash g_binocucomDefaultStaticScript;
extern ape::TuningHash g_binocucomDefaultAlertScript;
extern ape::TuningHash g_binocucomDefaultFullReticleScript;
extern ape::TuningHash g_binocucomDefaultPartialReticleScript;
extern ape::TuningHash g_binocucomDefaultCenteredNGReticleScript;
extern ape::TuningColor g_binocucomVisionNormalColorTint;
extern ape::TuningFloat g_binocucomVisionNormalVisionOffset;
extern ape::TuningFloat g_binocucomVisionNormalScanLineFreq;
extern ape::TuningBool g_binocucomVisionNormalMono;
extern ape::TuningBool g_binocucomVisionNormalBloom;
extern ape::TuningColor g_binocucomVisionThermalColorTint;
extern ape::TuningFloat g_binocucomVisionThermalVisionOffset;
extern ape::TuningFloat g_binocucomVisionThermalScanLineFreq;
extern ape::TuningBool g_binocucomVisionThermalMono;
extern ape::TuningBool g_binocucomVisionThermalBloom;

extern ape::TuningFloat g_uiBinocucomZoomArrowMaxU;
extern ape::TuningFloat g_uiBinocucomZoomArrowMinU;
extern ape::TuningFloat g_uiBinocucomZoomBarMaxU;
extern ape::TuningFloat g_uiBinocucomZoomBarMinU;
extern ape::TuningInt g_uiBinocucomMaxLines;
extern ape::TuningFloat g_uiBinocucomMax2DReticleDistance;
extern ape::TuningColor g_uiBinocucomNearReticleColor;
extern ape::TuningColor g_uiBinocucomFarReticleColor;
extern ape::TuningFloat g_uiBinocucomTargetListSpacing;
extern ape::TuningFloat g_uiBinocucomTargetListYAdjust;
extern ape::TuningVec3 g_uiBinocucomScreenCenterOffset;
extern ape::TuningFloat g_uiBinocucomTargetAngleWeight;
extern ape::TuningFloat g_uiBinocucomTargetDistanceWeight;
extern ape::TuningFloat g_uiBinocucomDisplayRateFudgeFactor;
extern ape::TuningString g_uiBinocucomCtrlHintTakePhotoStr;
extern ape::TuningString g_uiBinocucomCtrlHintZoomInStr;
extern ape::TuningString g_uiBinocucomCtrlHintZoomOutStr;
extern ape::TuningString g_uiBinocucomCtrlHintMoveStr;
extern ape::TuningString g_uiBinocucomPhotoErrorZoomInStr;
extern ape::TuningString g_uiBinocucomPhotoErrorZoomOutStr;
extern ape::TuningString g_uiBinocucomPhotoErrorBadFramingStr;
extern ape::TuningString g_uiBinocucomPhotoErrorNoItemsInViewStr;
extern ape::TuningString g_uiBinocucomCtrlHintShootStr;
extern ape::TuningFloat g_uiBinocucomCtrlHintAlphaSpeed;

extern ape::TuningFloat g_binocucomPhotoPowerLossFromDamage;
extern ape::TuningHash g_binocucomPhotoPowerDrainedScript;
extern ape::TuningHash g_binocucomActivationNoPowerScript;

extern ape::TuningBool g_uiBinocucomUsePhotoError;

extern ape::TuningFloat g_binocucomDefaultHelpTextSecondaryYOffset;

extern ape::TuningBool g_uiBinocucomRadarEnabled;
extern ape::TuningVec3 g_uiBinocucomRadarOrigin;
extern ape::TuningVec3 g_uiBinocucomRadarBasisX;
extern ape::TuningVec3 g_uiBinocucomRadarBasisY;
extern ape::TuningFloat g_uiBinocucomRadarWorldRadius;
extern ape::TuningColor g_uiBinocucomRadarEnemyColor;
extern ape::TuningColor g_uiBinocucomRadarGoalColor;
extern ape::TuningBool g_uiBinocucomRadarDebugOutlineRegion;
extern ape::TuningBool g_uiBinocucomRadarDebugDrawAxes;
extern ape::TuningBool g_uiBinocucomRadarPulseGoalBlipsByDistance;
extern ape::TuningFloat g_uiBinocucomRadarMinGoalPulseSeconds;
extern ape::TuningFloat g_uiBinocucomRadarMaxGoalPulseSeconds;
extern ape::TuningFloat g_uiBinocucomRadarGoalPulseSecondPerMeter;
extern ape::TuningBool g_uiBinocucomRadarFadeRadarEnemies;
extern ape::TuningFloat g_uiBinocucomRadarStartEnemyFadeDistance;
extern ape::TuningFloat g_uiBinocucomRadarBlipSize;
extern ape::TuningFloat g_uiBinocucomRadarSweepFXAlphaDecay;
extern ape::TuningBool g_uiBinocucomRadarDebugDrawSweep;
extern ape::TuningFloat g_uiBinocucomRadarSweepAnimAngleOffset;
extern ape::TuningHash g_uiBinocucomRadarSweepAnimGeo;
extern ape::TuningBool g_uiBinocucomRadarEnableEnemySweepFX;
extern ape::TuningBool g_uiBinocucomRadarEnableOnScreenGoalSweepFX;
extern ape::TuningBool g_uiBinocucomRadarEnableOffScreenGoalSweepFX;

extern ape::TuningVec3 g_binocucomReticleOffset;
extern ape::TuningHash g_binocucomPowerMeterHash;
extern ape::TuningHash g_binocucomPowerMeterIdleAnimHash;
extern ape::TuningHash g_binocucomPowerMeterPulseAnimHash;

extern ape::TuningHash g_uiBinocucomRadarBlipPlayer;
extern ape::TuningHash g_uiBinocucomRadarBlipEnemy;
extern ape::TuningHash g_uiBinocucomRadarBlipGoal;
extern ape::TuningHash g_uiBinocucomRadarBlipGoalEdge;
extern ape::TuningHash g_uiBinocucomRadarBlipHideout;
extern ape::TuningHash g_uiBinocucomRadarBlipVision;
extern ape::TuningHash g_uiBinocucomRadarBlipBottle;
extern ape::TuningHash g_uiBinocucomRadarBlipSafe;
extern ape::TuningFloat g_uiBinocucomRadarAlphaWorldYMin;
extern ape::TuningFloat g_uiBinocucomRadarAlphaWorldYMax;
extern ape::TuningFloat g_uiBinocucomRadarAlphaMin;
extern ape::TuningFloat g_uiBinocucomRadarAlphaMax;
extern ape::TuningColor g_uiBinocucomRadarEnemyColor2;
extern ape::TuningColor g_uiBinocucomRadarBottleColor;
extern ape::TuningColor g_uiBinocucomRadarSafeColor;

extern ape::TuningInt g_debugDisplayBuoyancy;
extern ape::TuningInt g_debugDisplayCombat;

extern ape::TuningBool g_combatAttackOnHold;
extern ape::TuningInt g_combatMode;
extern ape::TuningFloat g_combatFreezeScalar;
extern ape::TuningFloat g_combatFreezeDuration;
extern ape::TuningFloat g_combatFreezeTimeMin;
extern ape::TuningFloat g_combatWallAngleMin;
extern ape::TuningInt g_combatRangeBBoxCache;
extern ape::TuningColor g_combatDebugColor0;
extern ape::TuningColor g_combatDebugColor1;
extern ape::TuningColor g_combatDebugColor2;

extern ape::TuningFloat g_contextualMarkerScaleMin;
extern ape::TuningFloat g_contextualMarkerScaleMax;
extern ape::TuningFloat g_contextualMarkerScaleDistMin;
extern ape::TuningFloat g_contextualMarkerScaleDistMax;
extern ape::TuningFloat g_contextualMarkerScaleFactorFOV;

extern ape::TuningFloat g_controlExtJumpVel;

extern ape::TuningBool g_fxDeferSync;
extern ape::TuningFloat g_fxSlowTimeFadeTime;
extern ape::TuningFloat g_fxSlowTimeTrailLen;
extern ape::TuningColor g_fxSlowTimeTrailColor;
extern ape::TuningFloat g_fxSlowTimeTrailAlphaSpeed;

extern ape::TuningBool g_animationIKLookAtDisable;
extern ape::TuningFloat g_animationIKLookAtAngleMax;

extern ape::TuningBool g_debugDisplayInteractComp;

extern ape::TuningFloat g_inventoryStartScale;
extern ape::TuningFloat g_inventoryEndScale;
extern ape::TuningFloat g_inventoryScaleInterpRate;
extern ape::TuningFloat g_inventoryCoinIncreasePickpocketPercent;
extern ape::TuningFloat g_inventoryCoinIncreaseDeathPercent;
extern ape::TuningInt g_inventoryPickpocketAward1Num;
extern ape::TuningInt g_inventoryPickpocketAward1NumDefs;
extern ape::TuningInt g_inventoryPickpocketAward2Num;
extern ape::TuningInt g_inventoryPickpocketAward2NumDefs;
extern ape::TuningFloat g_inventoryPickpocketNoCollectTime;

extern ape::TuningInt g_debugDisplayJumpRope;

extern ape::TuningBool g_monsterClosetKillMonsters;

extern ape::TuningInt g_debugDisplayCharPhysics;

extern ape::TuningFloat g_physicsGravityScalar;
extern ape::TuningBool g_physicsRaycastEnable;
extern ape::TuningFloat g_physicsInputGroundGravityEffect;
extern ape::TuningFloat g_physicsInputGroundFrictionStatic;
extern ape::TuningFloat g_physicsInputGroundFrictionDyn;
extern ape::TuningFloat g_physicsInputAirFrictionXZ;
extern ape::TuningFloat g_physicsInputAirFrictionY;
extern ape::TuningFloat g_physicsInputAirRayRange;
extern ape::TuningFloat g_physicsInputGroundSlopeMax;
extern ape::TuningFloat g_physicsInputWallSlopeMin;
extern ape::TuningFloat g_physicsInputImpactDamageScalarObj;
extern ape::TuningFloat g_physicsInputImpactDamageScalarChar;
extern ape::TuningFloat g_physicsInputImpactDamageSpeedMin;
extern ape::TuningFloat g_physicsInputImpactDamageVelRatioMin;
extern ape::TuningFloat g_physicsKeepDistance;
extern ape::TuningFloat g_physicsBottomRadius;
extern ape::TuningInt g_physicsGroundPhantomMode;
extern ape::TuningVec3 g_physicsGroundPhantomSize;

extern ape::TuningVec3 g_characterMoveGroundCrawlColSize;
extern ape::TuningVec3 g_characterMoveGroundCrawlColOffset;
extern ape::TuningVec3 g_characterMoveGroundPlayDeadColSize;

extern ape::TuningFloat g_physicsPenetrationDepth;
extern ape::TuningFloat g_physicsFriction;
extern ape::TuningFloat g_physicsRestitution;
extern ape::TuningFloat g_physicsWalkRayRange;
extern ape::TuningFloat g_physicsMaxLinearVelocity;
extern ape::TuningFloat g_physicsMaxLinearAccel;
extern ape::TuningFloat g_physicsSupportSlopeMax;

extern ape::TuningFloat g_characterEdgeGrabNormalYCosMax;
extern ape::TuningInt g_characterEdgeGrabTagMode;
extern ape::TuningBool g_characterEdgeGrabAutoCatch;
extern ape::TuningFloat g_characterEdgeGrabHeightMin;
extern ape::TuningBool g_characterMoveGroundNormalRayEnable;
extern ape::TuningFloat g_characterMoveGroundNormalRayRange;

extern ape::TuningFloat g_physicsJumpTimeMin;
extern ape::TuningFloat g_physicsInputCornerPushVel;

extern ape::TuningInt g_procAnimProcMode;
extern ape::TuningFloat g_procAnimCutSpeed;
extern ape::TuningInt g_procAnimDebugLevel;
extern ape::TuningColor g_procAnimDebugColor0;
extern ape::TuningColor g_procAnimDebugColor1;
extern ape::TuningColor g_procAnimDebugColor2;
extern ape::TuningColor g_procAnimDebugColor3;
extern ape::TuningColor g_procAnimDebugColor4;

extern ape::TuningBool g_eyesDebugDraw;
extern ape::TuningBool g_eyesDebugPrintFloatTrack;

extern ape::TuningFloat g_radarSplineMoveSpeed;
extern ape::TuningFloat g_radarSplineMoveTimeout;
extern ape::TuningFloat g_radarSplineMoveInputWaitTime;
extern ape::TuningFloat g_radarGuidanceSplnPosOffset;
extern ape::TuningFloat g_radarMaxCompassArrowRotSpeed;
extern ape::TuningFloat g_radarDefaultActiveDuration;
extern ape::TuningFloat g_radarDefaultRefreshDuration;
extern ape::TuningBool g_radarUsePathAlways;
extern ape::TuningBool g_radarUsePathNever;
extern ape::TuningBool g_radarDrawForwardPaths;
extern ape::TuningBool g_radarDrawBackwardPaths;
extern ape::TuningInt g_radarAwardNum;
extern ape::TuningInt g_radarAwardNumActivate;
extern ape::TuningFloat g_radarRadarDeactivateTime;
extern ape::TuningFloat g_radarSafeMarkerYOffset;

extern ape::TuningBool g_receptacleDebugEnable;

extern ape::TuningColor g_projectorAlertColor;
extern ape::TuningColor g_projectorNormColor;

extern ape::TuningInt g_debugDisplayShooting;

extern ape::TuningFloat g_shootingCrackShotMarkerScaleMin;
extern ape::TuningFloat g_shootingCrackShotMarkerScaleMax;
extern ape::TuningFloat g_shootingCrackShotMarkerScaleDistMin;
extern ape::TuningFloat g_shootingCrackShotMarkerScaleDistMax;
extern ape::TuningInt g_shootingCrackShotAwardAwardNum;
extern ape::TuningInt g_shootingCrackShotAwardNumKills;
extern ape::TuningFloat g_shootingCrackShotAwardTime;

extern ape::TuningInt g_debugDisplayTargeting;

extern ape::TuningInt g_targetingMode;

extern ape::TuningFloat g_characterWallClimbStickThresholdLow;
extern ape::TuningFloat g_characterWallClimbStickThresholdHigh;
extern ape::TuningFloat g_characterWallClimbSlideRumbleStrength;
extern ape::TuningFloat g_characterWallClimbSlideRumbleDuration;

extern ape::TuningInt g_debugDisplayWheelchair;

extern ape::TuningBool g_wheelchairDisable;
extern ape::TuningVec2 g_wheelchairWheelTrackWidth;
extern ape::TuningVec2 g_wheelchairWheelRadius;
extern ape::TuningVec2 g_wheelchairWheelCamber;
extern ape::TuningFloat g_wheelchairWheelDeformSpeedMin;
extern ape::TuningFloat g_wheelchairWheelDeformSpeedMax;
extern ape::TuningFloat g_wheelchairWheelDeformFactor;
extern ape::TuningFloat g_wheelchairWheelDeformAngle;
extern ape::TuningFloat g_wheelchairWheelDeformRange;
extern ape::TuningFloat g_wheelchairWheelSpinnerFriction;
extern ape::TuningBool g_wheelchairTrailEnable;
extern ape::TuningFloat g_wheelchairTrailLength;
extern ape::TuningFloat g_wheelchairTrailFPS;
extern ape::TuningColor g_wheelchairTrailColor;
extern ape::TuningFloat g_wheelchairTrailWidth;
extern ape::TuningFloat g_wheelchairTrailYOffset;
extern ape::TuningFloat g_wheelchairTrailAlphaSpeed;
extern ape::TuningFloat g_wheelchairArmGainInterp;
extern ape::TuningFloat g_wheelchairArmBoneLengthMax;
extern ape::TuningFloat g_wheelchairArmSolverFPS;
extern ape::TuningFloat g_wheelchairArmSpringL;
extern ape::TuningFloat g_wheelchairArmSpringA;
extern ape::TuningFloat g_wheelchairArmDampingA;
extern ape::TuningFloat g_wheelchairArmDampingW;
extern ape::TuningFloat g_wheelchairArmBoundRadius;
extern ape::TuningInt g_wheelchairRemoteMode;
extern ape::TuningFloat g_wheelchairRemoteTrigTimeMin;
extern ape::TuningVec3 g_wheelchairRemoteOffset;
extern ape::TuningVec2 g_wheelchairRemoteVel;
extern ape::TuningVec2 g_wheelchairRemoteAVel;
extern ape::TuningVec2 g_wheelchairRemoteAngleX;
extern ape::TuningVec2 g_wheelchairRemoteAngleY;
extern ape::TuningFloat g_wheelchairRemoteStickDistMax;
extern ape::TuningFloat g_wheelchairRemoteUIDisplayTime;
extern ape::TuningFloat g_wheelchairShieldRadius;
extern ape::TuningFloat g_wheelchairShieldObjSize;
extern ape::TuningVec3 g_wheelchairShieldOffset;
extern ape::TuningVec3 g_wheelchairShieldPowerCost;
extern ape::TuningInt g_wheelchairRemoteBombPocketAwardNum;
extern ape::TuningInt g_wheelchairRemoteBombPocketAwardNumBombs;

} // namespace components
} // namespace sly
