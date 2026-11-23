// openTIT - AI spawn tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/AiSpawnTuning.h"

#include <limits>

namespace sly {

// @func 0x00456710 StaticInit_AiSpawnTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x010B16C0).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningFloat g_defaultNoSpawnDist("AI.DefaultNoSpawnDist", 2e+01f);

} // namespace sly
