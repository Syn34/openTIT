// openTIT - Binocucom goals tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/BinocucomGoalTuning.h"

#include <limits>

namespace sly {

// @func 0x003417D8 StaticInit_BinocucomGoalTuning
// @func 0x00341694 StaticDestroy_BinocucomGoalTuning
// Compiler-generated initialiser/destructor pair for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x0106B2E0).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningFloat g_binocucomPhotoDefaultAngle("Goal.BinocucomPhoto.DefaultAngle", 2.0f);
ape::TuningFloat g_binocucomPhotoDefaultDistMin("Goal.BinocucomPhoto.DefaultDistMin", 5.0f);
ape::TuningFloat g_binocucomPhotoDefaultDistMax("Goal.BinocucomPhoto.DefaultDistMax", 15.0f);
ape::TuningFloat g_binocucomPhotoDefaultDistMinClose("Goal.BinocucomPhoto.DefaultDistMinClose", 0.0f);
ape::TuningFloat g_binocucomPhotoDefaultDistMaxClose("Goal.BinocucomPhoto.DefaultDistMaxClose", 0.0f);
ape::TuningFloat g_binocucomPhotoDefaultDistCull("Goal.BinocucomPhoto.DefaultDistCull", 6e+01f);
ape::TuningFloat g_binocucomPhotoDefaultDisplayTime("Goal.BinocucomPhoto.DefaultDisplayTime", 2.0f);
ape::TuningFloat g_binocucomPhotoDefaultOcclusionRadius("Goal.BinocucomPhoto.DefaultOcclusionRadius", 1.0f);
ape::TuningFloat g_binocucomRecordDefaultAngle("Goal.BinocucomRecord.DefaultAngle", 3.6e+02f);
ape::TuningFloat g_binocucomRecordDefaultDistMin("Goal.BinocucomRecord.DefaultDistMin", 5.0f);
ape::TuningFloat g_binocucomRecordDefaultDistMax("Goal.BinocucomRecord.DefaultDistMax", 15.0f);
ape::TuningInt g_penguinMontageSubgameLossAwardNum("Goal.PenguinMontage.SubgameLossAwardNum", 27);
ape::TuningBool g_shootingGalleryDebugEnable("Goal.ShootingGallery.DebugEnable", false);

} // namespace sly
