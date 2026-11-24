// openTIT - Collision reports, edge grab, Havok defaults and animation IK tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/PhysicsDebugTuning.h"

namespace sly {

// @func 0x00355FF8 StaticInit_PhysicsDebugTuning
// @func 0x00355A90 StaticDestroy_PhysicsDebugTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (It also constructs the module's compact arrays and pointer arrays, which belong to the physics debug port.)

ape::TuningBool g_debugReportsDumpCollisionAll("Debug.Reports.DumpCollision(All)", false);
ape::TuningBool g_debugReportsDumpCollisionOrigin("Debug.Reports.DumpCollision(Origin)", false);
ape::TuningBool g_debugReportsDumpHavokMemory("Debug.Reports.Dump Havok Memory", false);
ape::TuningBool g_entityCharacterEdgeGrabProcessAll("Entity.Character.EdgeGrab.ProcessAll", false);
ape::TuningFloat g_entityCharacterEdgeGrabYSlopeMax("Entity.Character.EdgeGrab.YSlopeMax", 40.0f);
ape::TuningFloat g_entityCharacterEdgeGrabFaceAngleMin("Entity.Character.EdgeGrab.FaceAngleMin", 60.0f);
ape::TuningFloat g_entityCharacterEdgeGrabMidptDistSqMax("Entity.Character.EdgeGrab.MidptDistSqMax", 0.001f);
ape::TuningBool g_entityCharacterEdgeGrabProcessInGame("Entity.Character.EdgeGrab.Process In Game", false);
ape::TuningBool g_entityCharacterEdgeGrabValidateLoaded("Entity.Character.EdgeGrab.Validate Loaded", false);
ape::TuningBool g_havokDefaultEnableCharacterPush("Havok.Default.EnableCharacterPush", false);
ape::TuningFloat g_havokDefaultGravity("Havok.Default.Gravity", -9.8f);
ape::TuningFloat g_havokDefaultWorldSize("Havok.Default.WorldSize", 200.0f);
ape::TuningVec3 g_havokDefaultFloorSize("Havok.Default.FloorSize", 50.0f, 0.5f, 50.0f);
ape::TuningVec3 g_havokDefaultFloorPos("Havok.Default.FloorPos", 0.0f, -6.5f, 0.0f);
ape::TuningFloat g_havokDefaultFloorRestitution("Havok.Default.FloorRestitution", 0.9f);
ape::TuningInt g_animationLoadBalanceFrames("Animation.LoadBalanceFrames", 2);
ape::TuningFloat g_animationLoadBalanceDistDefault("Animation.LoadBalanceDistDefault", 45.0f);
ape::TuningBool g_animationLoadBalanceEnable("Animation.LoadBalanceEnable", true);
ape::TuningInt g_debugDisplayIK("Debug.Display.IK", 0);
ape::TuningFloat g_animationIKLookAtInterpInc("Animation.IK.LookAt.InterpInc", 0.2f);
ape::TuningFloat g_animationIKLookAtInterpDec("Animation.IK.LookAt.InterpDec", 0.15f);
ape::TuningFloat g_animationIKLookAtAngVelocityMax("Animation.IK.LookAt.AngVelocityMax", 360.0f);
ape::TuningFloat g_animationIKTwoJointSolverInterp("Animation.IK.TwoJointSolver.Interp", 0.2f);
ape::TuningFloat g_animationIKFootSolverMaxSlope("Animation.IK.FootSolver.MaxSlope", 50.0f);
ape::TuningInt g_animationIKFootSolverMode("Animation.IK.FootSolver.Mode", 1);

} // namespace sly
