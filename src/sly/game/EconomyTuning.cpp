// openTIT - Tuning variables of the economy / collectible translation unit.
#include "sly/game/EconomyTuning.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>
#include <limits>

namespace sly {

// @func 0x004B0124 StaticInit_EconomyTuning
// @func 0x004AFD54 StaticDestroy_EconomyTuning
// Compiler-generated initialiser/destructor pair for the globals of this
// translation unit (it also holds the wind code, see Wind.cpp).

// Plain globals the initialiser sets. NOTE: their users are not identified.
uint32_t g_economyUnknown26B8[3] = {};                                    // 0x010C26B8
float g_economyUnknown1AA0 = std::numeric_limits<float>::quiet_NaN();     // 0x010C1AA0

ape::TuningFloat g_economyLowChanceHealthSpawns("Game.Economy.LowThreshold.ChanceHealthSpawns", 0.9f);
ape::TuningFloat g_economyHighChanceHealthSpawns("Game.Economy.HighThreshold.ChanceHealthSpawns", 0.4f);
ape::TuningFloat g_economyHealthThreshold("Game.Economy.HealthThreshold", 0.2f);
ape::TuningFloat g_economyHealthSpawnDelayTimeMax("Game.Economy.HealthSpawnDelayTimeMax", 2.0f);
ape::TuningVec3 g_throwFromSpawnerOffset("Game.Economy.ThrowCollectible.FromSpawnerOffset", 0.0f, 0.75f, 0.0f);
ape::TuningFloat g_throwMaxNoSpawnDist("Game.Economy.ThrowCollectible.MaxNoSpawnDist", 75.0f);
ape::TuningFloat g_throwSpawnRangeDistInit("Game.Economy.ThrowCollectible.SpawnRangeDistInit", 2.1f);
ape::TuningFloat g_throwSpawnRangeDistMin("Game.Economy.ThrowCollectible.SpawnRangeDistMin", 2.1f);
ape::TuningFloat g_throwSpawnRangeDistMax("Game.Economy.ThrowCollectible.SpawnRangeDistMax", 3.1f);
ape::TuningFloat g_throwDeltaRangeDist("Game.Economy.ThrowCollectible.DeltaRangeDist", 0.3f);
ape::TuningFloat g_throwSpawnDelayTimeMin("Game.Economy.ThrowCollectible.SpawnDelayTimeMin", 0.0f);
ape::TuningFloat g_throwSpawnDelayTimeMax("Game.Economy.ThrowCollectible.SpawnDelayTimeMax", 0.3f);
ape::TuningFloat g_throwOverShootOffset("Game.Economy.ThrowCollectible.OverShootOffset", 3.0f);
ape::TuningFloat g_throwSpawnNoCollectTime("Game.Economy.ThrowCollectible.SpawnNoCollectTime", 0.6f);
ape::TuningFloat g_throwDeltaRangeAngle("Game.Economy.ThrowCollectible.DeltaRangeAngle", 34.0f);
ape::TuningInt g_throwMaxSpawnAttempts("Game.Economy.ThrowCollectible.MaxSpawnAttempts", 10);
ape::TuningVec3 g_throwSpawnRandAngVel("Game.Economy.ThrowCollectible.SpawnRandAngVel", 0.0f, 1440.0f, 0.0f);
ape::TuningVec3 g_throwSpawnRandAngInstVel("Game.Economy.ThrowCollectible.SpawnRandAngInstVel", 0.0f, 20.0f, 0.0f);
ape::TuningFloat g_throwPlayerPosAdjustFactor("Game.Economy.ThrowCollectible.PlayerPosAdjustFactor", 0.5f);
ape::TuningFloat g_throwGlobalGravityScalar("Game.Economy.ThrowCollectible.GlobalGravityScalar", 1.5f);
ape::TuningFloat g_throwCoinIncreasePercent("Game.Economy.ThrowCollectible.CoinIncreasePercent", 1.5f);
ape::TuningFloat g_throwBoltIncreasePercent("Game.Economy.ThrowCollectible.BoltIncreasePercent", 2.0f);

namespace {
// 0x80-byte records (16 reserved) used by 0x004AE498/0x004AED50 and cleared by
// ResetWindGlobals. NOTE: contents not identified.
struct UnitRecord80 {
    uint8_t bytes[0x80];
};
ape::ApeTArray<UnitRecord80> g_unitRecords(16);                      // 0x010C2480
} // namespace

ape::TuningBool g_debugDisplayMemory("Debug.Display.Memory", false);
ape::TuningBool g_debugDisplayMemoryRsxMain("Debug.Display.Memory (RSX Main)", false);
ape::TuningBool g_debugMemoryDumpToFile("Debug.Memory.DumpToFile", false);

uint32_t g_economyUnknown1B84[2] = {};    // 0x010C1B84 (0x010C1B88 is used by 0x004A8CD0..0x004A9084)
uint32_t g_economyUnknown1C08[5] = {};    // 0x010C1C08

ape::TuningFloat g_subtitleDisplayRate("UI.Subtitle.DisplayRate", -1.0f);
ape::TuningFloat g_subtitleDisplayRateFudgeFactor("UI.Subtitle.DisplayRateFudgeFactor", 0.5f);
ape::TuningInt g_subtitleMaxLines("UI.Subtitle.MaxLines", 2);
ape::TuningFloat g_subtitleExtraTime("UI.Subtitle.ExtraTime", 1.0f);
ape::TuningVec3 g_debugDisplayTrackScrollVel("Debug.Display.TrackScrollVel", 0.0f, 2.0f, 0.0f);
ape::TuningBool g_debugDisplayLineOfSite("Debug.Display.LineOfSite", false);
ape::TuningInt g_debugDisplayCalcArc("Debug.Display.CalcArc", 0);
ape::TuningInt g_loadingScreenNumText("Game.LoadingScreen.NumText", 0);
ape::TuningString g_loadingScreenTextPrefix("Game.LoadingScreen.TextPrefix", "HINT_GENERAL_");

namespace {
// Sorted map (32 entries) used by 0x004ACD68/0x004AD154. NOTE: contents not identified.
ape::ApeTSortedMap<void*> g_unitMap(32);                             // 0x010C1D50
} // namespace

} // namespace sly
