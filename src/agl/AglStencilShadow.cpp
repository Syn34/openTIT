// openTIT - Agl stencil shadows: shared geometry and settings.
#include "agl/AglStencilShadow.h"

#include <cmath>

namespace agl {

namespace {

AglStencilShadowSettings g_settings;
int32_t g_renderSettingInt = 0;        // 0x00F66FC8
float g_renderSettingFloat = 0.0f;     // 0x00F66FCC
AglRenderHook* g_renderHook = nullptr; // 0x00F66FD0
bool g_renderHookEnabled = false;      // 0x00F66FD4

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.28318531f;

// Wraps an angle into [-pi, pi).
float WrapAngle(float a)
{
    const float bias = a < 0.0f ? -kPi : kPi;
    const float shifted = bias + a;
    return shifted - static_cast<float>(static_cast<int64_t>(shifted / kTwoPi)) * kTwoPi - bias;
}

// Parabolic sine approximation with one refinement step (valid on [-pi, pi]).
float ParabolicSin(float a)
{
    const float y = a * 1.2732395f - a * 0.40528473f * std::fabs(a);
    return (y * std::fabs(y) - y) * 0.225f + y;
}

} // namespace

AglStencilShadowSettings& GetStencilShadowSettings()
{
    return g_settings;
}

// @func 0x005275C0 SetStencilShadowConeAngle
void SetStencilShadowConeAngle(float degrees)
{
    g_settings.coneCos = ape::FastCos(degrees * 0.017453292f);
}

// @func 0x00527608 SetStencilShadowIntensity
void SetStencilShadowIntensity(float intensity)
{
    g_settings.intensity = intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity);
}

// @func 0x005271F8 BuildStencilShadowCircle
void BuildStencilShadowCircle()
{
    for (int i = 0; i < 8; ++i) {
        const float a = WrapAngle(static_cast<float>(i) * 45.0f * 0.017453292f);
        float b = a + 1.5707964f;
        if (b > kPi)
            b -= kTwoPi;
        g_settings.circle[i] = { -ParabolicSin(a) * 0.5f, 0.0f, ParabolicSin(b) * 0.5f, 0.0f };
    }
}

// @func 0x0052730C InitStencilShadowQuad
void InitStencilShadowQuad()
{
    static const AglVertexStream kStreams[] = {
        { 0, static_cast<uint8_t>(gpu::VertexAttribType::Float), 3, 0, 0, false },
        { 0, 0, 0, 0, 0, false },
    };
    VertexDeclInit(&g_settings.quadDecl, kStreams, 12);
    MemBlockAlloc(&g_settings.quad, g_settings.quadDecl.stride * 4, 0x80, gpu::MemoryLocation::Local, 2);
    if (auto* v = static_cast<float*>(g_settings.quad.data)) {
        // Triangle strip over clip space: top left, bottom left, top right, bottom right.
        const float quad[12] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
        for (int i = 0; i < 12; ++i)
            v[i] = quad[i];
    }
}

std::vector<AglStencilShadowGroup*>& GetStencilShadowGroups()
{
    static std::vector<AglStencilShadowGroup*> groups;   // 0x010DFA24
    return groups;
}

// @func 0x00527484 InitStencilShadowSystem
void InitStencilShadowSystem()
{
    InitStencilShadowQuad();
    BuildStencilShadowCircle();
    auto* group = new AglStencilShadowGroup();
    GetStencilShadowGroups().push_back(group);
    group->active = true;
    group->edges = new AglStencilShadowEdges();
}

// @func 0x00527650 SetRenderSettingInt
void SetRenderSettingInt(int32_t value)
{
    g_renderSettingInt = value;
}

// @func 0x0052765C SetRenderSettingFloat
void SetRenderSettingFloat(float value)
{
    g_renderSettingFloat = value;
}

// @func 0x00527668 SetRenderHook
void SetRenderHook(AglRenderHook* hook)
{
    g_renderHook = hook;
    hook->OnInstall();
}

void SetRenderHookEnabled(bool enabled)
{
    g_renderHookEnabled = enabled;
}

// @func 0x005176F8 CallRenderHook
void CallRenderHook()
{
    if (g_renderHookEnabled && g_renderHook)
        g_renderHook->OnFrame();
}

} // namespace agl
