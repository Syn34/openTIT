// openTIT - Binocucom UI tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/BinocucomUiTuning.h"

#include <limits>

namespace sly {

// @func 0x00328644 StaticInit_BinocucomUiTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

// File-local "unset" sentinel (quiet NaN) from a shared header (0x0106B260).
[[maybe_unused]] static const float s_unsetFloat = std::numeric_limits<float>::quiet_NaN();

ape::TuningFloat g_binocucomHelpTextFadeTime("UI.Binocucom.HelpTextFadeTime", 1.0f);

} // namespace sly
