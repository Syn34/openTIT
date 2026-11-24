// openTIT - Trigger tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/objects/TriggerTuning.h"

namespace sly {
namespace objects {

// @func 0x00226288 StaticInit_TriggerTuning
// @func 0x00226060 StaticDestroy_TriggerTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (The
// initialiser also clears a file-local zero vector and allocates the 8 KB deferred
// trigger-report buffer 0x01055B80; Trigger.cpp keeps that queue.)

ape::TuningBool g_debugDisplayTrigger_All("Debug.Display.Trigger. All", true);
ape::TuningBool g_debugDisplayTriggerLog("Debug.Display.Trigger.Log", false);
ape::TuningBool g_debugDisplayTriggerAlert("Debug.Display.Trigger.Alert", false);
ape::TuningBool g_audioOptimizationDisableVolumetricSounds("Audio.Optimization.DisableVolumetricSounds", false);
ape::TuningBool g_debugDisplayTriggerAmbient("Debug.Display.Trigger.Ambient", false);
ape::TuningBool g_debugDisplayTriggerCamera("Debug.Display.Trigger.Camera", false);
ape::TuningBool g_triggerCameraNoDeadExit("Entity.Trigger.Camera.NoDeadExit", false);
ape::TuningBool g_debugDisplayTriggerCameraBehavior("Debug.Display.Trigger.CameraBehavior", false);
ape::TuningBool g_debugDisplayTriggerCrawl("Debug.Display.Trigger.Crawl", false);
ape::TuningBool g_debugDisplayTriggerDeath("Debug.Display.Trigger.Death", false);
ape::TuningFloat g_triggerDeathSurfaceRayRange("Entity.Trigger.Death.SurfaceRayRange", 2e+01f);
ape::TuningBool g_debugDisplayTriggerFiltered("Debug.Display.Trigger.Filtered", false);
ape::TuningBool g_debugDisplayTriggerFX("Debug.Display.Trigger.FX", false);
ape::TuningBool g_debugDisplayTriggerJob("Debug.Display.Trigger.Job", false);
ape::TuningBool g_debugDisplayTriggerJumpDir("Debug.Display.Trigger.JumpDir", false);
ape::TuningFloat g_audioZoneXFadeTime("Audio.ZoneXFadeTime", 2.0f);
ape::TuningBool g_debugDisplayTriggerSoundZone("Debug.Display.Trigger.SoundZone", false);
ape::TuningBool g_debugDisplayTriggerTimeScale("Debug.Display.Trigger.TimeScale", false);
ape::TuningInt g_debugDisplayWeatherSystem("Debug.Display.WeatherSystem", 0);
ape::TuningBool g_debugDisplayTriggerWeather("Debug.Display.Trigger.Weather", false);
ape::TuningBool g_triggerWindDisplay("Entity.Trigger.Wind.Display", false);
ape::TuningBool g_debugDisplayTriggerUpdateEnable("Debug.Display.Trigger.UpdateEnable", false);
ape::TuningBool g_debugDisplayTriggerSmoke("Debug.Display.Trigger.Smoke", false);

} // namespace objects
} // namespace sly
