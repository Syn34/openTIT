// openTIT - Score / marker entity tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/ScoreMarkerTuning.h"

namespace sly {

// @func 0x0020BD08 StaticInit_ScoreMarkerTuning
// @func 0x0020BC08 StaticDestroy_ScoreMarkerTuning
// Compiler-generated initialiser/destructor pair for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningVec3 g_markerOffset("Entity.Marker.Offset", 0.0f, 0.5f, 0.0f);
ape::TuningVec3 g_markerTriggerJobOffset("Entity.Marker.TriggerJobOffset", 0.0f, 2.0f, 0.0f);
ape::TuningFloat g_scoreVisTimeDefault("Entity.Score.VisTimeDefault", 0.75f);
ape::TuningFloat g_scoreFadeTimeDefault("Entity.Score.FadeTimeDefault", 0.5f);
ape::TuningString g_scoreInstNameValue("Entity.Score.InstNameValue", "Value");
ape::TuningString g_scoreInstNameRoot("Entity.Score.InstNameRoot", ".InGame.Elements.Score");
ape::TuningString g_scoreInstNameParent("Entity.Score.InstNameParent", ".InGame.HUD.Scores.Popup");
ape::TuningVec3 g_scoreOffset("Entity.Score.Offset", -75.0f, -5e+01f, 0.0f);
ape::TuningString g_scoreInstNameHilite("Entity.Score.InstNameHilite", "HL");

} // namespace sly
