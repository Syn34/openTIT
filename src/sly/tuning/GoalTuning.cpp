// openTIT - Goal ticker and episode completion tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/GoalTuning.h"

namespace sly {

// @func 0x002E7A6C StaticInit_GoalTuning
// @func 0x002E76F4 StaticDestroy_GoalTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (It also constructs the goal module's five 12-byte-record compact arrays and one pointer array; those belong to the goal manager port.)

ape::TuningFloat g_goalTickerDisplayTime("Goal.Ticker.DisplayTime", 4.0f);
ape::TuningInt g_gameEpisodeMaxMasks("Game.Episode.MaxMasks", 50);
ape::TuningInt g_gameEpisodeSafeState("Game.Episode.SafeState", 0);
ape::TuningBool g_gameEpisodeShowAllTreasuresInHideout("Game.Episode.ShowAllTreasuresInHideout", true);
ape::TuningBool g_gameEpisodeShowAllSafeTreasuresInHideout("Game.Episode.ShowAllSafeTreasuresInHideout", true);
ape::TuningHash g_gameEpisodeDefaultDisguise("Game.Episode.DefaultDisguise", 0x0E84789Au);
ape::TuningBool g_gameEpisodeInitCostumeDoorsOnReset("Game.Episode.InitCostumeDoorsOnReset", false);
ape::TuningInt g_gameEpisodeTreasureAwardNumTreasures("Game.Episode.TreasureAwardNumTreasures", 20);
ape::TuningInt g_gameEpisodePenguinAwardNumPenguins("Game.Episode.PenguinAwardNumPenguins", 40);
ape::TuningInt g_gameEpisodeArcadeAwardScoreIndex("Game.Episode.ArcadeAwardScoreIndex", 1);
ape::TuningFloat g_gameEpisodeCompletionFormulaJobWeight("Game.Episode.CompletionFormula.JobWeight", 85.0f);
ape::TuningFloat g_gameEpisodeCompletionFormulaMaskWeight("Game.Episode.CompletionFormula.MaskWeight", 3.0f);
ape::TuningFloat g_gameEpisodeCompletionFormulaBottleWeight("Game.Episode.CompletionFormula.BottleWeight", 8.0f);
ape::TuningFloat g_gameEpisodeCompletionFormulaTreasureWeight("Game.Episode.CompletionFormula.TreasureWeight", 3.0f);
ape::TuningFloat g_gameEpisodeCompletionFormulaSafeWeight("Game.Episode.CompletionFormula.SafeWeight", 1.0f);

} // namespace sly
