// openTIT - Spawn rope entity tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/SpawnRopeTuning.h"

#include <limits>

namespace sly {

// @func 0x0009A108 StaticInit_SpawnRopeTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x00F9CD30).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningFloat g_spawnRopeRangeMax("Entity.SpawnRope.RangeMax", 2.5e+02f);

} // namespace sly
