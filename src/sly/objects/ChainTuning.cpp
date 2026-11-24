// openTIT - Chain tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/objects/ChainTuning.h"

namespace sly {
namespace objects {

// @func 0x001D50C4 StaticInit_ChainTuning
// @func 0x001D4C28 StaticDestroy_ChainTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (The
// initialiser also clears a file-local zero vector, the shared "unset" NaN sentinel
// and the rope solver's job-busy word 0x01042500; see Rope.cpp.)

ape::TuningFloat g_chainSplineHandleLen("Entity.Chain.SplineHandleLen", 0.25f);
ape::TuningFloat g_chainGravityScalar("Entity.Chain.GravityScalar", 2.25f);
ape::TuningFloat g_chainLinkMass("Entity.Chain.LinkMass", 75.0f);
ape::TuningFloat g_chainDamping("Entity.Chain.Damping", 0.35f);
ape::TuningInt g_chainSolverItersOuter("Entity.Chain.SolverItersOuter", 1);
ape::TuningInt g_chainSolverItersInner("Entity.Chain.SolverItersInner", 4);
ape::TuningBool g_chainDisableBreak("Entity.Chain.DisableBreak", false);
ape::TuningVec2 g_chainWobbleFreq("Entity.Chain.WobbleFreq", 3.5f, 5.5f);
ape::TuningFloat g_chainWobbleMag("Entity.Chain.WobbleMag", 0.1f);
ape::TuningFloat g_pushBlockRaycastForwardYOffset("Entity.PushBlock.RaycastForwardYOffset", 0.2f);
ape::TuningFloat g_pushBlockRaycastForwardDist("Entity.PushBlock.RaycastForwardDist", 0.15f);
ape::TuningFloat g_pushBlockRaycastDownwardDist("Entity.PushBlock.RaycastDownwardDist", 0.25f);
ape::TuningFloat g_pushBlockRaycastFallDist("Entity.PushBlock.RaycastFallDist", 0.5f);
ape::TuningFloat g_pushBlockImpactAngleThreshold("Entity.PushBlock.ImpactAngleThreshold", 1.7e+02f);
ape::TuningFloat g_pushBlockCamShakeDuration("Entity.PushBlock.CamShakeDuration", 1.1f);
ape::TuningFloat g_pushBlockCamShakeMagnitude("Entity.PushBlock.CamShakeMagnitude", 0.4f);
ape::TuningFloat g_pushBlockCamShakeRampUpStartMagnitude("Entity.PushBlock.CamShakeRampUpStartMagnitude", 0.0f);
ape::TuningHash g_pushBlockCamShakeFOModeHash("Entity.PushBlock.CamShakeFOModeHash", 0x85D98325u);
ape::TuningFloat g_pushBlockCamShakeFOStartTime("Entity.PushBlock.CamShakeFOStartTime", 0.0f);
ape::TuningFloat g_pushBlockCamShakePitch("Entity.PushBlock.CamShakePitch", 1.0f);
ape::TuningFloat g_pushBlockCamShakeYaw("Entity.PushBlock.CamShakeYaw", 0.0f);
ape::TuningFloat g_pushBlockCamShakeRoll("Entity.PushBlock.CamShakeRoll", 1.0f);
ape::TuningFloat g_pushBlockCamShakePitchFrequency("Entity.PushBlock.CamShakePitchFrequency", 4.0f);
ape::TuningFloat g_pushBlockCamShakeYawFrequency("Entity.PushBlock.CamShakeYawFrequency", 6.5f);
ape::TuningFloat g_pushBlockCamShakeRollFrequency("Entity.PushBlock.CamShakeRollFrequency", 2.5f);
ape::TuningBool g_ropeDebugRender("Entity.Rope.Debug.Render", false);
ape::TuningColor g_ropeDebugColorSplinePt("Entity.Rope.Debug.ColorSplinePt", { 208, 192, 216, 208 });
ape::TuningColor g_ropeDebugColorSplineCtrl("Entity.Rope.Debug.ColorSplineCtrl", { 188, 48, 0, 96 });
ape::TuningInt g_ropeUpdateMode("Entity.Rope.UpdateMode", 3);
ape::TuningInt g_ropeNSegsDefault("Entity.Rope.NSegsDefault", 4);
ape::TuningFloat g_ropeSplineHandleLen("Entity.Rope.SplineHandleLen", 0.35f);
ape::TuningFloat g_ropeGravityScalar("Entity.Rope.GravityScalar", 2.25f);
ape::TuningFloat g_ropeTension("Entity.Rope.Tension", 2.5f);
ape::TuningFloat g_ropeSegmentMass("Entity.Rope.SegmentMass", 7.5f);
ape::TuningFloat g_ropeDampingWorld("Entity.Rope.DampingWorld", 0.015f);
ape::TuningFloat g_ropeDampingLocal("Entity.Rope.DampingLocal", 0.085f);
ape::TuningFloat g_ropeSpringConstRetract("Entity.Rope.SpringConstRetract", 4e+02f);
ape::TuningFloat g_ropeSpringConstExtend("Entity.Rope.SpringConstExtend", 0.75f);
ape::TuningInt g_ropeSolverFreqMin("Entity.Rope.SolverFreqMin", 180);
ape::TuningFloat g_ropeWindHeading("Entity.Rope.Wind.Heading", 33.0f);
ape::TuningVec3 g_ropeWindFreq("Entity.Rope.Wind.Freq", 0.15f, 0.231f, 0.934f);
ape::TuningVec3 g_ropeWindMagnitude("Entity.Rope.Wind.Magnitude", 15.8f, 24.4f, 18.7f);
ape::TuningVec2 g_ropeFootstepScalar("Entity.Rope.FootstepScalar", 0.25f, 2.0f);
ape::TuningInt g_springboardDebugLevel("Entity.Springboard.DebugLevel", 0);
ape::TuningFloat g_springboardDebugRange("Entity.Springboard.DebugRange", 1.5f);
ape::TuningInt g_springboardDebugNSteps("Entity.Springboard.DebugNSteps", 15);
ape::TuningColor g_springboardDebugColor0("Entity.Springboard.DebugColor0", { 192, 96, 208, 192 });
ape::TuningColor g_springboardDebugColor1("Entity.Springboard.DebugColor1", { 160, 192, 192, 192 });
ape::TuningFloat g_sentryLightAlertFollowDistMax("Entity.SentryLight.AlertFollowDistMax", 1e+01f);
ape::TuningFloat g_sentryLightAlertOnTgtSpeed("Entity.SentryLight.AlertOnTgtSpeed", 5.0f);
ape::TuningFloat g_sentryLightAlertCooldownTgtSpeed("Entity.SentryLight.AlertCooldownTgtSpeed", 5.0f);

} // namespace objects
} // namespace sly
