// openTIT - Effects (object shake, trails) tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningBool g_fXDisableAll;
extern ape::TuningFloat g_fXShakeSpeedInterp;
extern ape::TuningFloat g_fXObjShakeDefaultCullDist;
extern ape::TuningBool g_fXObjShakeDisableIfNotRendered;
extern ape::TuningFloat g_fXTrailFrameRate;
extern ape::TuningInt g_fXTrailNoTexture;
extern ape::TuningBool g_fXTrailOutlineEnable;
extern ape::TuningColor g_fXTrailOutlineColor;
extern ape::TuningFloat g_fXTrailPointAccelMax;
extern ape::TuningFloat g_fXTrailHintAngleMin;

} // namespace sly
