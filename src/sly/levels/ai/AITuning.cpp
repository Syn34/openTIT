// openTIT - "AI.*" tuning variables (see AITuning.h).
#include "sly/levels/ai/AITuning.h"

namespace sly {
namespace levels {
namespace ai {

// NOTE: defaults read from the register values 0x00067738 passes; the two
// range values share one register (15.0) assumed unchanged at that point.
ape::TuningFloat g_aiChaseVOFrequency("AI.ChaseVOFrequency", 5.0f);
ape::TuningFloat g_aiAttackPlayerRecoveryTime("AI.AttackPlayerRecoveryTime", 3.0f);
ape::TuningFloat g_aiAudioAlertRangeNormal("AI.AudioAlertRangeNormal", 15.0f);
ape::TuningFloat g_aiAudioAlertRangeFootsteps("AI.AudioAlertRangeFootsteps", 7.5f);
ape::TuningFloat g_aiAudioAlertRangeBottle("AI.AudioAlertRangeBottle", 15.0f);

} // namespace ai
} // namespace levels
} // namespace sly
