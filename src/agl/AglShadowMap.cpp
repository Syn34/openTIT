// openTIT - Agl shadow maps: cascade settings and the shadow map object.
#include "agl/AglShadowMap.h"

#include <algorithm>

namespace agl {

namespace {
AglShadowSettings* g_shadowSettings = nullptr;
}

// @func 0x0052DA10 AglShadowSettings::AglShadowSettings
AglShadowSettings::AglShadowSettings() = default;

AglShadowSettings& GetShadowSettings()
{
    if (!g_shadowSettings)
        InitShadowSettings();
    return *g_shadowSettings;
}

// @func 0x0052DACC InitShadowSettings
void InitShadowSettings()
{
    static AglShadowSettings settings;
    g_shadowSettings = &settings;
}

// @func 0x0052D018 AglShadowSettings::SetSplits
void AglShadowSettings::SetSplits(const float* values, uint8_t count)
{
    splitCount = count;
    for (uint8_t i = 0; i < count && i < kMaxSplits; ++i)
        splits[i] = values[i];
}

// @func 0x0052DAEC AglShadowSettings::GetDeferredVariant
int AglShadowSettings::GetDeferredVariant(uint32_t variantFlags, int cascade) const
{
    const bool soft = (variantFlags & kVariant_Soft) != 0;
    const int taps = soft ? 2 : 4;
    // Later cascades use fewer filter taps.
    // (The console's unsigned divide yields 0 for a zero split count.)
    const int step = splitCount ? static_cast<int>((taps + 1u) / splitCount) : 0;
    const int reduced = taps - cascade * step;
    int variant = std::max(reduced, soft ? 1 : 0);
    if (variantFlags & kVariant_NoFilter)
        variant = 10;
    else if (soft)
        variant += 5;
    return variant;
}

// @func 0x00548D14 AglShadowMapBase::AglShadowMapBase
AglShadowMapBase::AglShadowMapBase()
{
    for (uint16_t& v : field_1C)
        v = 0;
    field_284 = 0;
    field_288 = 0;
    debugView = false;
}

// @func 0x005493F4 AglShadowMapBase::ToggleDebugView
void AglShadowMapBase::ToggleDebugView()
{
    debugView = !debugView;
}

namespace {
// @func 0x0052AA48 AglShadowMapBase::LatchFrame
void LatchFrame(AglShadowMapBase* map)
{
    map->field_200 = map->field_1A0;
}
}

// @func 0x005493B0 AglShadowMapBase::Reset
void AglShadowMapBase::Reset()
{
    LatchFrame(this);
    for (uint32_t& count : casterCounts)
        count = 0;
}

// @func 0x0052D08C AglShadowMap::AglShadowMap
AglShadowMap::AglShadowMap() = default;

} // namespace agl
