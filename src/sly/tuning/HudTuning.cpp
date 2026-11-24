// openTIT - Interaction prompts, coin counter, HUD wheel, radar and map tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/HudTuning.h"

namespace sly {

// @func 0x0038A254 StaticInit_HudTuning
// @func 0x00389D00 StaticDestroy_HudTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (It also constructs the HUD module's arrays, which belong to the HUD port.)

ape::TuningString g_uIInteractUIInteractMsg("UI.Interact.UIInteractMsg", "$BUTTON_FIRE3 $STR_INTERACT_ENTER");
ape::TuningString g_uIInteractUIInteractSafehouseMsg("UI.Interact.UIInteractSafehouseMsg", "$BUTTON_FIRE3 $STR_INTERACT_SAFEHOUSE_ENTER");
ape::TuningString g_uIInteractUICancelMsg("UI.Interact.UICancelMsg", "$BUTTON_FIRE3 $STR_INTERACT_EXIT");
ape::TuningFloat g_gameMonetaryCollectCoinCounterMovementTimeMax("Game.MonetaryCollect.CoinCounter.MovementTimeMax", 0.7f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterCountUpTimeMax("Game.MonetaryCollect.CoinCounter.CountUpTimeMax", 1.13f);
ape::TuningVec3 g_gameMonetaryCollectCoinCounterEndHUDOffset("Game.MonetaryCollect.CoinCounter.EndHUDOffset", 5.3f, 2.78f, 1.0f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterMoveHUDYOffset("Game.MonetaryCollect.CoinCounter.MoveHUDYOffset", -1.5f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterMinScale("Game.MonetaryCollect.CoinCounter.MinScale", 0.1f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterMinAlpha("Game.MonetaryCollect.CoinCounter.MinAlpha", 0.1f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterDelayTimeMax("Game.MonetaryCollect.CoinCounter.DelayTimeMax", 0.33f);
ape::TuningFloat g_gameMonetaryCollectCoinCounterShowTimeMax("Game.MonetaryCollect.CoinCounter.ShowTimeMax", 2.5f);
ape::TuningVec3 g_gameMonetaryCollectCoinCounterCountUpTextScale("Game.MonetaryCollect.CoinCounter.CountUpTextScale", 2.0f, 2.0f, 1.0f);
ape::TuningColor g_gameMonetaryCollectCoinCounterCountUpTextColor("Game.MonetaryCollect.CoinCounter.CountUpTextColor", ape::xml::Color8{0xEB, 0xC1, 0x13, 0xFF});
ape::TuningVec3 g_uI3DHealthBaseObj480Correct("UI.3D.HealthBaseObj480Correct", 0.0f, 0.0f, -1.0f);
ape::TuningVec3 g_uI3DHealthMeter480Correct("UI.3D.HealthMeter480Correct", 0.0f, 0.0f, -1.0f);
ape::TuningFloat g_uIMainMenuUVScaling("UI.MainMenu.UV Scaling", 2.0f);
ape::TuningFloat g_hudWheelSlotRotSpeed("Hud.Wheel.SlotRotSpeed", 720.0f);
ape::TuningFloat g_hudWheelAnalogLMin("Hud.Wheel.AnalogLMin", 0.85f);
ape::TuningFloat g_hudWheelAnalogAngleDelta("Hud.Wheel.AnalogAngleDelta", 20.0f);
ape::TuningFloat g_radarAlphaWorldYMin("Radar.Alpha.WorldYMin", 2.85f);
ape::TuningFloat g_radarAlphaWorldYMax("Radar.Alpha.WorldYMax", 14.0f);
ape::TuningFloat g_radarAlphaMin("Radar.Alpha.Min", 0.05f);
ape::TuningFloat g_radarAlphaMax("Radar.Alpha.Max", 1.0f);
ape::TuningFloat g_hudMapZoomMin("Hud.Map.ZoomMin", 1.0f);
ape::TuningFloat g_hudMapZoomMax("Hud.Map.ZoomMax", 5.0f);
ape::TuningFloat g_hudMapZoomStep("Hud.Map.ZoomStep", 1.25f);
ape::TuningFloat g_hudMapZoomDefault("Hud.Map.ZoomDefault", 3.0f);
ape::TuningFloat g_hudMapPanStep("Hud.Map.PanStep", 0.4f);
ape::TuningFloat g_hudMapBlipScaleMin("Hud.Map.BlipScaleMin", 0.3f);
ape::TuningBool g_hudMapBlipScaleEnable("Hud.Map.BlipScaleEnable", false);
ape::TuningFloat g_hudMapUVBuffer("Hud.Map.UVBuffer", 1.0f);
ape::TuningColor g_hudMapIconDisabledColor("Hud.Map.IconDisabledColor", ape::xml::Color8{0x9B, 0x9B, 0x9B, 0xFF});

} // namespace sly
