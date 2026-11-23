// openTIT - West boss tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/WestBossTuning.h"

namespace sly {

// @func 0x000C24E8 StaticInit_WestBossTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningFloat g_westBossStuckCameraInterp("Entity.WestBoss.StuckCamera.Interp", 0.1f);
ape::TuningFloat g_westBossStuckCameraGimbalRadius("Entity.WestBoss.StuckCamera.GimbalRadius", 4.66f);
ape::TuningFloat g_westBossStuckCameraAngle("Entity.WestBoss.StuckCamera.Angle", 21.0f);
ape::TuningFloat g_westBossStuckCameraTargetYOffset("Entity.WestBoss.StuckCamera.TargetYOffset", 1.01f);
ape::TuningFloat g_westBossStuckCameraTargetZOffset("Entity.WestBoss.StuckCamera.TargetZOffset", 0.8f);
ape::TuningFloat g_westBossStuckCameraEaseInTime("Entity.WestBoss.StuckCamera.EaseInTime", 1.0f);
ape::TuningFloat g_westBossStuckCameraEaseOutTime("Entity.WestBoss.StuckCamera.EaseOutTime", 1.0f);
ape::TuningBool g_westBossAllowAnyMeleeAttackForKnockback("Entity.WestBoss.AllowAnyMeleeAttackForKnockback", false);
ape::TuningBool g_westBossDebugDrawThrownGadgetSpln("Entity.WestBoss.DebugDrawThrownGadgetSpln", false);

} // namespace sly
