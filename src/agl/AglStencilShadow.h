// openTIT - Agl stencil shadows: shared geometry and settings.
#pragma once

#include "agl/AglPrimitives.h"
#include "agl/AglTypes.h"

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace agl {

struct AglStencilShadowSettings {
    float coneCos = 1.0f;          // 0x010DFA00: cosine of the shadow cone angle
    float intensity = 1.0f;        // 0x010DF9E8: 0..1
    ape::Vec4 circle[8] = {};      // 0x010DFA40: unit circle (radius 0.5) in the xz plane
    AglMemBlock quad;              // 0x010DF8BC: full-screen quad (triangle strip)
    AglVertexDecl quadDecl;        // 0x010DF960
};

AglStencilShadowSettings& GetStencilShadowSettings();

void SetStencilShadowConeAngle(float degrees);
void SetStencilShadowIntensity(float intensity);
// Fills the eight directions of the shadow circle (every 45 degrees).
void BuildStencilShadowCircle();
// Creates the full-screen quad used to darken the stencilled pixels.
void InitStencilShadowQuad();

// Renderer settings set together with the shadow settings (0x00F66FC8, 0x00F66FCC).
void SetRenderSettingInt(int32_t value);
void SetRenderSettingFloat(float value);

// An optional object notified by the renderer (0x00F66FD0, enabled by 0x00F66FD4).
class AglRenderHook {
public:
    virtual ~AglRenderHook() = default;
    virtual void OnInstall() {}
    virtual void OnFrame() {}
};
void SetRenderHook(AglRenderHook* hook);
void SetRenderHookEnabled(bool enabled);
void CallRenderHook();

} // namespace agl
