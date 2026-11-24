// openTIT - Goal ticker and episode completion tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningFloat g_goalTickerDisplayTime;
extern ape::TuningInt g_gameEpisodeMaxMasks;
extern ape::TuningInt g_gameEpisodeSafeState;
extern ape::TuningBool g_gameEpisodeShowAllTreasuresInHideout;
extern ape::TuningBool g_gameEpisodeShowAllSafeTreasuresInHideout;
extern ape::TuningHash g_gameEpisodeDefaultDisguise;
extern ape::TuningBool g_gameEpisodeInitCostumeDoorsOnReset;
extern ape::TuningInt g_gameEpisodeTreasureAwardNumTreasures;
extern ape::TuningInt g_gameEpisodePenguinAwardNumPenguins;
extern ape::TuningInt g_gameEpisodeArcadeAwardScoreIndex;
extern ape::TuningFloat g_gameEpisodeCompletionFormulaJobWeight;
extern ape::TuningFloat g_gameEpisodeCompletionFormulaMaskWeight;
extern ape::TuningFloat g_gameEpisodeCompletionFormulaBottleWeight;
extern ape::TuningFloat g_gameEpisodeCompletionFormulaTreasureWeight;
extern ape::TuningFloat g_gameEpisodeCompletionFormulaSafeWeight;

} // namespace sly
