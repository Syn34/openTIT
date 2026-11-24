// openTIT - Trigger tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace objects {

extern ape::TuningBool g_debugDisplayTrigger_All;   // 0x01056500
extern ape::TuningBool g_debugDisplayTriggerLog;   // 0x010569C0
extern ape::TuningBool g_debugDisplayTriggerAlert;   // 0x01056598
extern ape::TuningBool g_audioOptimizationDisableVolumetricSounds;   // 0x01055B00
extern ape::TuningBool g_debugDisplayTriggerAmbient;   // 0x0105654C
extern ape::TuningBool g_debugDisplayTriggerCamera;   // 0x01056A10
extern ape::TuningBool g_triggerCameraNoDeadExit;   // 0x01056A60
extern ape::TuningBool g_debugDisplayTriggerCameraBehavior;   // 0x01056630
extern ape::TuningBool g_debugDisplayTriggerCrawl;   // 0x0105667C
extern ape::TuningBool g_debugDisplayTriggerDeath;   // 0x010566C8
extern ape::TuningFloat g_triggerDeathSurfaceRayRange;   // 0x01056AB0
extern ape::TuningBool g_debugDisplayTriggerFiltered;   // 0x01056714
extern ape::TuningBool g_debugDisplayTriggerFX;   // 0x01056760
extern ape::TuningBool g_debugDisplayTriggerJob;   // 0x010567AC
extern ape::TuningBool g_debugDisplayTriggerJumpDir;   // 0x010567F8
extern ape::TuningFloat g_audioZoneXFadeTime;   // 0x01056B00
extern ape::TuningBool g_debugDisplayTriggerSoundZone;   // 0x010565E4
extern ape::TuningBool g_debugDisplayTriggerTimeScale;   // 0x01056844
extern ape::TuningInt g_debugDisplayWeatherSystem;   // 0x01056890
extern ape::TuningBool g_debugDisplayTriggerWeather;   // 0x010568DC
extern ape::TuningBool g_triggerWindDisplay;   // 0x01056B50
extern ape::TuningBool g_debugDisplayTriggerUpdateEnable;   // 0x01056928
extern ape::TuningBool g_debugDisplayTriggerSmoke;   // 0x01056974

} // namespace objects
} // namespace sly
