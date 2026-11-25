// openTIT - Destructors and small overrides of the engine-side effect bases
// and the game's final effect classes (0x0079E428-0x007A3860).
#include "agl/AglPostProc.h"

namespace agl {

// --- engine bases --------------------------------------------------------------

// @func 0x007A1A90 AglFadingEffect::~AglFadingEffect
// @func 0x007A1AA0 AglFadingEffect::~AglFadingEffect (deleting)
AglFadingEffect::~AglFadingEffect() = default;
// @func 0x007A1AE4 AglFadingEffect::Apply
void AglFadingEffect::Apply() {}

// @func 0x007A1AE8 AglThermalVisionBase::~AglThermalVisionBase
// @func 0x007A1AF8 AglThermalVisionBase::~AglThermalVisionBase (deleting)
AglThermalVisionBase::~AglThermalVisionBase() = default;
// @func 0x007A1B3C AglThermalVisionBase::Apply
void AglThermalVisionBase::Apply() {}

// @func 0x007A1A3C AglGodRaysBase::~AglGodRaysBase
// @func 0x007A1A4C AglGodRaysBase::~AglGodRaysBase (deleting)
AglGodRaysBase::~AglGodRaysBase() = default;

// @func 0x0079E438 AglBlurBase::~AglBlurBase
// @func 0x007A1730 AglBlurBase::~AglBlurBase (deleting)
AglBlurBase::~AglBlurBase() = default;

// @func 0x0079E478 AglDofBase::~AglDofBase
// @func 0x007A17CC AglDofBase::~AglDofBase (deleting)
AglDofBase::~AglDofBase() = default;

// @func 0x0079E498 AglRadialBlurBase::~AglRadialBlurBase
// @func 0x007A1810 AglRadialBlurBase::~AglRadialBlurBase (deleting)
AglRadialBlurBase::~AglRadialBlurBase() = default;

// @func 0x0079E458 AglDesaturationBase::~AglDesaturationBase
// @func 0x007A1788 AglDesaturationBase::~AglDesaturationBase (deleting)
AglDesaturationBase::~AglDesaturationBase() = default;
// @func 0x007A1774 AglDesaturationBase::IsActive
bool AglDesaturationBase::IsActive() const { return enabled; }

// @func 0x007A16D8 AglBloomBase::~AglBloomBase
// @func 0x007A16E8 AglBloomBase::~AglBloomBase (deleting)
AglBloomBase::~AglBloomBase() = default;
// @func 0x007A172C AglBloomBase::Apply
void AglBloomBase::Apply() {}

// @func 0x00788264 AglBloomBase::CopyBloomSettings
void AglBloomBase::CopyBloomSettings(const AglBloomBase& other)
{
    enabled = other.enabled;
    for (int i = 0; i < 3; ++i)
        field_20[i] = other.field_20[i];
    field_30 = other.field_30;
    field_34 = other.field_34;
}

// @func 0x007A189C AglBloomNoiseBase::~AglBloomNoiseBase
// @func 0x007A18AC AglBloomNoiseBase::~AglBloomNoiseBase (deleting)
AglBloomNoiseBase::~AglBloomNoiseBase() = default;
// @func 0x007A18F0 AglBloomNoiseBase::Apply
void AglBloomNoiseBase::Apply() {}

// @func 0x007A18F4 AglBloomNoiseBase::CopyNoiseSettings
void AglBloomNoiseBase::CopyNoiseSettings(const AglBloomNoiseBase& other)
{
    enabled = other.enabled;
    for (int i = 0; i < 3; ++i)
        layers[i] = other.layers[i];
    for (int i = 0; i < 4; ++i)
        field_5C[i] = other.field_5C[i];
    for (int i = 0; i < 8; ++i)
        field_60[i] = other.field_60[i];
    field_80 = other.field_80;
    field_84 = other.field_84;
    field_90 = other.field_90;
    field_94 = other.field_94;
    field_98 = other.field_98;
    field_A0 = other.field_A0;
}

// --- the game's effects ----------------------------------------------------------

// @func 0x0079E428 AglBloomEffect::~AglBloomEffect
// @func 0x007A3524 AglBloomEffect::~AglBloomEffect (deleting)
AglBloomEffect::~AglBloomEffect() = default;

// @func 0x0079E468 AglDesaturationEffect::~AglDesaturationEffect
// @func 0x007A35AC AglDesaturationEffect::~AglDesaturationEffect (deleting)
AglDesaturationEffect::~AglDesaturationEffect() = default;

// @func 0x0079E4A8 AglRadialBlurEffect::~AglRadialBlurEffect
// @func 0x007A1858 AglRadialBlurEffect::~AglRadialBlurEffect (deleting)
AglRadialBlurEffect::~AglRadialBlurEffect() = default;
// @func 0x007A1854 AglRadialBlurEffect::Apply
void AglRadialBlurEffect::Apply() {}

// @func 0x0079E7A0 AglThermalVisionEffect::~AglThermalVisionEffect
// @func 0x007A3860 AglThermalVisionEffect::~AglThermalVisionEffect (deleting)
AglThermalVisionEffect::~AglThermalVisionEffect() = default;

// @func 0x0079E6DC AglScreenWarpEffect::~AglScreenWarpEffect
// @func 0x007A3700 AglScreenWarpEffect::~AglScreenWarpEffect (deleting)
AglScreenWarpEffect::~AglScreenWarpEffect()
{
    for (int i = 2; i >= 0; --i)
        MemBlockFree(&blocks[i]);
}

} // namespace agl
