// openTIT - Parameter descriptors of the AI setting classes.
//
// The critter, buddy, enemy and sentry setting classes (AISettingClasses.h)
// register these descriptors; they are globals built by the static
// initialiser 0x00067738 (tagged in AITuning.cpp). `AIPropParam` maps the
// AIProp names used by the AI code to them.
#pragma once

#include "ape/setting/ApeSetting.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {
namespace ai {

// Descriptor of an AI setting property, or null for properties registered by
// other modules (proxy points, jump links).
const ape::setting::SettingParam* AIPropParam(AIProp prop);

} // namespace ai
} // namespace levels
} // namespace sly
