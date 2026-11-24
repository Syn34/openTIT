// openTIT - Interaction prompts, coin counter, HUD wheel, radar and map tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {

extern ape::TuningString g_uIInteractUIInteractMsg;
extern ape::TuningString g_uIInteractUIInteractSafehouseMsg;
extern ape::TuningString g_uIInteractUICancelMsg;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterMovementTimeMax;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterCountUpTimeMax;
extern ape::TuningVec3 g_gameMonetaryCollectCoinCounterEndHUDOffset;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterMoveHUDYOffset;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterMinScale;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterMinAlpha;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterDelayTimeMax;
extern ape::TuningFloat g_gameMonetaryCollectCoinCounterShowTimeMax;
extern ape::TuningVec3 g_gameMonetaryCollectCoinCounterCountUpTextScale;
extern ape::TuningColor g_gameMonetaryCollectCoinCounterCountUpTextColor;
extern ape::TuningVec3 g_uI3DHealthBaseObj480Correct;
extern ape::TuningVec3 g_uI3DHealthMeter480Correct;
extern ape::TuningFloat g_uIMainMenuUVScaling;
extern ape::TuningFloat g_hudWheelSlotRotSpeed;
extern ape::TuningFloat g_hudWheelAnalogLMin;
extern ape::TuningFloat g_hudWheelAnalogAngleDelta;
extern ape::TuningFloat g_radarAlphaWorldYMin;
extern ape::TuningFloat g_radarAlphaWorldYMax;
extern ape::TuningFloat g_radarAlphaMin;
extern ape::TuningFloat g_radarAlphaMax;
extern ape::TuningFloat g_hudMapZoomMin;
extern ape::TuningFloat g_hudMapZoomMax;
extern ape::TuningFloat g_hudMapZoomStep;
extern ape::TuningFloat g_hudMapZoomDefault;
extern ape::TuningFloat g_hudMapPanStep;
extern ape::TuningFloat g_hudMapBlipScaleMin;
extern ape::TuningBool g_hudMapBlipScaleEnable;
extern ape::TuningFloat g_hudMapUVBuffer;
extern ape::TuningColor g_hudMapIconDisabledColor;

} // namespace sly
