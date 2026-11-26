// openTIT - Tuning variables of the economy / collectible module and its neighbours
// (subtitles, memory debug display, loading screen hints) that share one translation
// unit with the wind code (0x004A8xxx-0x004AFD54).
#pragma once

#include "ape/tuning/ApeTuning.h"

#include <cstdint>

namespace sly {

extern ape::TuningFloat g_economyLowChanceHealthSpawns;     // Game.Economy.LowThreshold.ChanceHealthSpawns
extern ape::TuningFloat g_economyHighChanceHealthSpawns;    // Game.Economy.HighThreshold.ChanceHealthSpawns
extern ape::TuningFloat g_economyHealthThreshold;
extern ape::TuningFloat g_economyHealthSpawnDelayTimeMax;
// Game.Economy.ThrowCollectible.*
extern ape::TuningVec3 g_throwFromSpawnerOffset;           // value at 0x010C1F00
extern ape::TuningFloat g_throwMaxNoSpawnDist;
extern ape::TuningFloat g_throwSpawnRangeDistInit;
extern ape::TuningFloat g_throwSpawnRangeDistMin;
extern ape::TuningFloat g_throwSpawnRangeDistMax;
extern ape::TuningFloat g_throwDeltaRangeDist;
extern ape::TuningFloat g_throwSpawnDelayTimeMin;
extern ape::TuningFloat g_throwSpawnDelayTimeMax;
extern ape::TuningFloat g_throwOverShootOffset;
extern ape::TuningFloat g_throwSpawnNoCollectTime;
extern ape::TuningFloat g_throwDeltaRangeAngle;
extern ape::TuningInt g_throwMaxSpawnAttempts;
extern ape::TuningVec3 g_throwSpawnRandAngVel;
extern ape::TuningVec3 g_throwSpawnRandAngInstVel;
extern ape::TuningFloat g_throwPlayerPosAdjustFactor;
extern ape::TuningFloat g_throwGlobalGravityScalar;
extern ape::TuningFloat g_throwCoinIncreasePercent;
extern ape::TuningFloat g_throwBoltIncreasePercent;        // value at 0x010C2478

extern ape::TuningBool g_debugDisplayMemory;
extern ape::TuningBool g_debugDisplayMemoryRsxMain;
extern ape::TuningBool g_debugMemoryDumpToFile;
extern ape::TuningFloat g_subtitleDisplayRate;
extern ape::TuningFloat g_subtitleDisplayRateFudgeFactor;
extern ape::TuningInt g_subtitleMaxLines;
extern ape::TuningFloat g_subtitleExtraTime;
extern ape::TuningVec3 g_debugDisplayTrackScrollVel;
extern ape::TuningBool g_debugDisplayLineOfSite;
extern ape::TuningInt g_debugDisplayCalcArc;
extern ape::TuningInt g_loadingScreenNumText;
extern ape::TuningString g_loadingScreenTextPrefix;

// Other globals of the unit; NOTE: purpose unknown.
extern uint32_t g_economyUnknown26B8[3];
extern float g_economyUnknown1AA0;
extern uint32_t g_economyUnknown1B84;

} // namespace sly
