// openTIT - Level start-up helpers (camera settings, director scripts).
#pragma once

namespace agl { class AglRenderTarget; }

namespace ape {

class ApeLevelDef;

void ApplyLevelCamera(const ApeLevelDef& level, agl::AglRenderTarget& target, bool force);
void LoadLevelDirectorFiles(const ApeLevelDef& level);

} // namespace ape
