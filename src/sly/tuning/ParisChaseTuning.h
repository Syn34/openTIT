// openTIT - Paris chase goal tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningBool g_qTEDebugEnable;
extern ape::TuningBool g_parisChaseDebugEnable;
extern ape::TuningFloat g_parisChaseMaxVanDeviation;
extern ape::TuningFloat g_parisChaseDeviationSpeed;
extern ape::TuningFloat g_parisChaseVanColsize;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeDuration;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeMagnitude;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeRampUpStartMagnitude;
extern ape::TuningHash g_parisChaseCameraShakeCamShakeFOModeHash;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeFOStartTime;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakePitch;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeYaw;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeRoll;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakePitchFrequency;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeYawFrequency;
extern ape::TuningFloat g_parisChaseCameraShakeCamShakeRollFrequency;
extern ape::TuningFloat g_parisChaseCameraRollMaxRollRate;
extern ape::TuningFloat g_parisChaseCameraRollDeviationScale;
extern ape::TuningFloat g_parisChaseCameraRollDeviationRateScale;
extern ape::TuningFloat g_parisChaseCameraRollInputScale;

} // namespace sly
