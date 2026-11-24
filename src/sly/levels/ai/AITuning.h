// openTIT - "AI.*" tuning variables used by the level AI.
//
// On the console these are constructed by the large static initialiser
// 0x00067738 (AI settings and boss tuning, outside this range); only the
// ones this directory reads are defined here, with the defaults that
// initialiser passes. Whoever ports 0x00067738 should take them over.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace levels {
namespace ai {

extern ape::TuningFloat g_aiChaseVOFrequency;           // 0x00F9CB78
extern ape::TuningFloat g_aiAttackPlayerRecoveryTime;   // 0x00F9CBC4
extern ape::TuningFloat g_aiAudioAlertRangeNormal;      // 0x00F9CC10
extern ape::TuningFloat g_aiAudioAlertRangeFootsteps;   // 0x00F9CC5C
extern ape::TuningFloat g_aiAudioAlertRangeBottle;      // 0x00F9CCA8

} // namespace ai
} // namespace levels
} // namespace sly
