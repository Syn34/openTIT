// openTIT - Paris chase goal tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/ParisChaseTuning.h"

namespace sly {

// @func 0x0043B0C0 StaticInit_ParisChaseTuning
// @func 0x0043AE5C StaticDestroy_ParisChaseTuning
// Compiler-generated initialiser/destructor pair for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningBool g_qTEDebugEnable("Goal.QTE.DebugEnable", false);
ape::TuningBool g_parisChaseDebugEnable("Goal.ParisChase.DebugEnable", false);
ape::TuningFloat g_parisChaseMaxVanDeviation("Goal.ParisChase.MaxVanDeviation", 2.0f);
ape::TuningFloat g_parisChaseDeviationSpeed("Goal.ParisChase.DeviationSpeed", 8.0f);
ape::TuningFloat g_parisChaseVanColsize("Goal.ParisChase.VanColsize", 3.5f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeDuration("Goal.ParisChase.CameraShake.CamShakeDuration", 1.1f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeMagnitude("Goal.ParisChase.CameraShake.CamShakeMagnitude", 4.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeRampUpStartMagnitude("Goal.ParisChase.CameraShake.CamShakeRampUpStartMagnitude", 0.0f);
ape::TuningHash g_parisChaseCameraShakeCamShakeFOModeHash("Goal.ParisChase.CameraShake.CamShakeFOModeHash", 0x85D98325u);
ape::TuningFloat g_parisChaseCameraShakeCamShakeFOStartTime("Goal.ParisChase.CameraShake.CamShakeFOStartTime", 0.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakePitch("Goal.ParisChase.CameraShake.CamShakePitch", 1.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeYaw("Goal.ParisChase.CameraShake.CamShakeYaw", 0.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeRoll("Goal.ParisChase.CameraShake.CamShakeRoll", 1.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakePitchFrequency("Goal.ParisChase.CameraShake.CamShakePitchFrequency", 4.0f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeYawFrequency("Goal.ParisChase.CameraShake.CamShakeYawFrequency", 6.5f);
ape::TuningFloat g_parisChaseCameraShakeCamShakeRollFrequency("Goal.ParisChase.CameraShake.CamShakeRollFrequency", 2.5f);
ape::TuningFloat g_parisChaseCameraRollMaxRollRate("Goal.ParisChase.CameraRoll.MaxRollRate", 6e+01f);
ape::TuningFloat g_parisChaseCameraRollDeviationScale("Goal.ParisChase.CameraRoll.DeviationScale", 3.0f);
ape::TuningFloat g_parisChaseCameraRollDeviationRateScale("Goal.ParisChase.CameraRoll.DeviationRateScale", 6e+01f);
ape::TuningFloat g_parisChaseCameraRollInputScale("Goal.ParisChase.CameraRoll.InputScale", 5.0f);

} // namespace sly
