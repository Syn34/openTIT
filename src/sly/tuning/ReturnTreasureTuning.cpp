// openTIT - Return-treasure microgame tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/ReturnTreasureTuning.h"

namespace sly {

// @func 0x00426BB8 StaticInit_ReturnTreasureTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningFloat g_returnTreasurePosInterpTime("Microgame.ReturnTreasure.PosInterpTime", 1.5f);
ape::TuningFloat g_returnTreasureScaleInterpTime("Microgame.ReturnTreasure.ScaleInterpTime", 1.5f);
ape::TuningFloat g_returnTreasureTickerInfoTime("Microgame.ReturnTreasure.TickerInfoTime", 1e+01f);
ape::TuningFloat g_returnTreasurePosMoveYOffset("Microgame.ReturnTreasure.PosMoveYOffset", 2.0f);
ape::TuningFloat g_returnTreasureCountDownStartTime("Microgame.ReturnTreasure.CountDownStartTime", 1e+01f);

} // namespace sly
