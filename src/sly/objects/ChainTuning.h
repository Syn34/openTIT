// openTIT - Chain tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace objects {

extern ape::TuningFloat g_chainSplineHandleLen;   // 0x010421B8
extern ape::TuningFloat g_chainGravityScalar;   // 0x01042208
extern ape::TuningFloat g_chainLinkMass;   // 0x01042258
extern ape::TuningFloat g_chainDamping;   // 0x010422A8
extern ape::TuningInt g_chainSolverItersOuter;   // 0x010422F8
extern ape::TuningInt g_chainSolverItersInner;   // 0x01042348
extern ape::TuningBool g_chainDisableBreak;   // 0x01042398
extern ape::TuningVec2 g_chainWobbleFreq;   // 0x010423E8
extern ape::TuningFloat g_chainWobbleMag;   // 0x01042438
extern ape::TuningFloat g_pushBlockRaycastForwardYOffset;   // 0x01041C10
extern ape::TuningFloat g_pushBlockRaycastForwardDist;   // 0x01041C5C
extern ape::TuningFloat g_pushBlockRaycastDownwardDist;   // 0x01041CA8
extern ape::TuningFloat g_pushBlockRaycastFallDist;   // 0x01041CF4
extern ape::TuningFloat g_pushBlockImpactAngleThreshold;   // 0x01041D40
extern ape::TuningFloat g_pushBlockCamShakeDuration;   // 0x01041D8C
extern ape::TuningFloat g_pushBlockCamShakeMagnitude;   // 0x01041DD8
extern ape::TuningFloat g_pushBlockCamShakeRampUpStartMagnitude;   // 0x01041E24
extern ape::TuningHash g_pushBlockCamShakeFOModeHash;   // 0x01041E70
extern ape::TuningFloat g_pushBlockCamShakeFOStartTime;   // 0x01041EBC
extern ape::TuningFloat g_pushBlockCamShakePitch;   // 0x01041F08
extern ape::TuningFloat g_pushBlockCamShakeYaw;   // 0x01041F54
extern ape::TuningFloat g_pushBlockCamShakeRoll;   // 0x01041FA0
extern ape::TuningFloat g_pushBlockCamShakePitchFrequency;   // 0x01041FEC
extern ape::TuningFloat g_pushBlockCamShakeYawFrequency;   // 0x01042038
extern ape::TuningFloat g_pushBlockCamShakeRollFrequency;   // 0x01042084
extern ape::TuningBool g_ropeDebugRender;   // 0x01042580
extern ape::TuningColor g_ropeDebugColorSplinePt;   // 0x010425D0
extern ape::TuningColor g_ropeDebugColorSplineCtrl;   // 0x0104261C
extern ape::TuningInt g_ropeUpdateMode;   // 0x01042668
extern ape::TuningInt g_ropeNSegsDefault;   // 0x010426B8
extern ape::TuningFloat g_ropeSplineHandleLen;   // 0x01042708
extern ape::TuningFloat g_ropeGravityScalar;   // 0x01042758
extern ape::TuningFloat g_ropeTension;   // 0x010427A8
extern ape::TuningFloat g_ropeSegmentMass;   // 0x010427F8
extern ape::TuningFloat g_ropeDampingWorld;   // 0x01042848
extern ape::TuningFloat g_ropeDampingLocal;   // 0x01042898
extern ape::TuningFloat g_ropeSpringConstRetract;   // 0x010428E8
extern ape::TuningFloat g_ropeSpringConstExtend;   // 0x01042938
extern ape::TuningInt g_ropeSolverFreqMin;   // 0x01042988
extern ape::TuningFloat g_ropeWindHeading;   // 0x010429D8
extern ape::TuningVec3 g_ropeWindFreq;   // 0x01042A30
extern ape::TuningVec3 g_ropeWindMagnitude;   // 0x01042A90
extern ape::TuningVec2 g_ropeFootstepScalar;   // 0x01042AF0
extern ape::TuningInt g_springboardDebugLevel;   // 0x01041A88
extern ape::TuningFloat g_springboardDebugRange;   // 0x01041AD8
extern ape::TuningInt g_springboardDebugNSteps;   // 0x01041B28
extern ape::TuningColor g_springboardDebugColor0;   // 0x01041B78
extern ape::TuningColor g_springboardDebugColor1;   // 0x01041BC4
extern ape::TuningFloat g_sentryLightAlertFollowDistMax;   // 0x010420D0
extern ape::TuningFloat g_sentryLightAlertOnTgtSpeed;   // 0x0104211C
extern ape::TuningFloat g_sentryLightAlertCooldownTgtSpeed;   // 0x01042168

} // namespace objects
} // namespace sly
