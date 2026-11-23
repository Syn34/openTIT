// openTIT - Loading screen tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/LoadingScreenTuning.h"

namespace sly {

// @func 0x000B2114 StaticInit_LoadingScreenTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningString g_loadingScreenSoundBank("Game.LoadingScreen.SoundBank", "Audio\\Load\\MUS_Load.bnk");
ape::TuningInt g_loadingScreenSoundIndex("Game.LoadingScreen.SoundIndex", 0);
ape::TuningBool g_loadingScreenEnable("Game.LoadingScreen.Enable", true);
ape::TuningFloat g_loadingScreenZoomScale("Game.LoadingScreen.ZoomScale", 0.85f);
ape::TuningFloat g_loadingScreenZoomSpeed("Game.LoadingScreen.ZoomSpeed", 0.6f);

} // namespace sly
