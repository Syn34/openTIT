// openTIT - RC bomb remote and item collect tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningFloat g_itemTargetingScoreBias;
extern ape::TuningFloat g_itemTargetingScoreBiasThrow;
extern ape::TuningBool g_itemCarryStatic;
extern ape::TuningVec2 g_itemCollectMonetaryEndHudPos;
extern ape::TuningVec2 g_itemCollectMaskEndHudPos;
extern ape::TuningVec2 g_itemCollectHealthEndHudPos;
extern ape::TuningFloat g_itemCollectMinApproachScale;
extern ape::TuningFloat g_itemCollectHudCollectLifetimeMax;
extern ape::TuningFloat g_itemCollectHudCollectMoveYOffset;
extern ape::TuningFloat g_itemCollectHudCollectStartScale;
extern ape::TuningFloat g_itemCollectHudCollectEndScale;
extern ape::TuningFloat g_itemCollectHudCollectSplinePt1Percent;
extern ape::TuningFloat g_itemCollectHudCollectSplinePt2Percent;
extern ape::TuningFloat g_itemCollectRBSettleRaycastDist;
extern ape::TuningFloat g_itemCollectAutoCollectInitialCollectDist;
extern ape::TuningBool g_itemCollectAutoCollectForceOn;
extern ape::TuningFloat g_itemCollectAutoCollectSafetyCollectDist;
extern ape::TuningFloat g_itemCollectTreasureCollectScale;
extern ape::TuningBool g_itemCollectTreasureAutoCollectEnabled;
extern ape::TuningVec3 g_itemCollectTreasureApproachEndScale;
extern ape::TuningFloat g_bRemoteDamageImpactDmg;
extern ape::TuningFloat g_bRemoteDamageImpactImp;
extern ape::TuningFloat g_bRemoteDamageImpactVelMin;
extern ape::TuningFloat g_bRemoteDamageImpactDmgMax;
extern ape::TuningFloat g_bRemoteDamageMeleeDmg;
extern ape::TuningFloat g_bRemoteDamageMeleeImp;
extern ape::TuningFloat g_bRemoteDamageBlastDmg;
extern ape::TuningFloat g_bRemoteDamageBlastImp;
extern ape::TuningFloat g_bRemoteDamageEnvDmg;
extern ape::TuningFloat g_bRemoteDamageEnvImp;
extern ape::TuningFloat g_bRemoteDamageProjDmg;
extern ape::TuningFloat g_bRemoteBombDamageExplosive;
extern ape::TuningFloat g_bRemoteBombDamageSleep;
extern ape::TuningFloat g_bRemoteBombDamageElectric;
extern ape::TuningFloat g_bRemoteBombDamageConfusion;
extern ape::TuningFloat g_bRemoteBombDamageShrink;
extern ape::TuningFloat g_bRemoteBombImpulse;
extern ape::TuningVec2 g_bRemoteBombRadius;
extern ape::TuningFloat g_bRemoteBombSuperScalar;
extern ape::TuningVec3 g_bRemoteCenterOfMass;
extern ape::TuningFloat g_bRemoteTriggerTimeMax;
extern ape::TuningFloat g_bRemoteImpactSFXSpeedMin;
extern ape::TuningFloat g_bRemoteStickySlopeMin;
extern ape::TuningFloat g_bRemoteKickSpeed;
extern ape::TuningFloat g_bRemoteKickAngle;
extern ape::TuningFloat g_bRemoteSeekRadiusMax;
extern ape::TuningVec2 g_bRemoteSeekAcceleration;
extern ape::TuningVec3 g_bRemotePodOffset;
extern ape::TuningVec3 g_bRemoteOscFreq;
extern ape::TuningVec3 g_bRemoteOscMag;
extern ape::TuningVec3 g_bRemotePodSpringPos;
extern ape::TuningVec3 g_bRemotePodSpringRot;
extern ape::TuningVec2 g_bRemotePodAttackOffset;
extern ape::TuningFloat g_bRemoteFireAngleMax;
extern ape::TuningFloat g_bRemoteReloadTime;
extern ape::TuningVec3 g_bRemoteRocketRate;
extern ape::TuningVec2 g_bRemoteRange;

} // namespace sly
