// openTIT - Collision reports, edge grab, Havok defaults and animation IK tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningBool g_debugReportsDumpCollisionAll;
extern ape::TuningBool g_debugReportsDumpCollisionOrigin;
extern ape::TuningBool g_debugReportsDumpHavokMemory;
extern ape::TuningBool g_entityCharacterEdgeGrabProcessAll;
extern ape::TuningFloat g_entityCharacterEdgeGrabYSlopeMax;
extern ape::TuningFloat g_entityCharacterEdgeGrabFaceAngleMin;
extern ape::TuningFloat g_entityCharacterEdgeGrabMidptDistSqMax;
extern ape::TuningBool g_entityCharacterEdgeGrabProcessInGame;
extern ape::TuningBool g_entityCharacterEdgeGrabValidateLoaded;
extern ape::TuningBool g_havokDefaultEnableCharacterPush;
extern ape::TuningFloat g_havokDefaultGravity;
extern ape::TuningFloat g_havokDefaultWorldSize;
extern ape::TuningVec3 g_havokDefaultFloorSize;
extern ape::TuningVec3 g_havokDefaultFloorPos;
extern ape::TuningFloat g_havokDefaultFloorRestitution;
extern ape::TuningInt g_animationLoadBalanceFrames;
extern ape::TuningFloat g_animationLoadBalanceDistDefault;
extern ape::TuningBool g_animationLoadBalanceEnable;
extern ape::TuningInt g_debugDisplayIK;
extern ape::TuningFloat g_animationIKLookAtInterpInc;
extern ape::TuningFloat g_animationIKLookAtInterpDec;
extern ape::TuningFloat g_animationIKLookAtAngVelocityMax;
extern ape::TuningFloat g_animationIKTwoJointSolverInterp;
extern ape::TuningFloat g_animationIKFootSolverMaxSlope;
extern ape::TuningInt g_animationIKFootSolverMode;

} // namespace sly
