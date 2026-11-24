// openTIT - Agl post-process chain: the screen effects applied after a render target's scene.
#include "agl/AglPostProc.h"

#include "agl/AglDisplay.h"
#include "agl/AglPostProcShaders.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"

#include "ape/core/ApeString.h"

#include <algorithm>

namespace agl {

// --- effect base ---------------------------------------------------------

AglPostProcEffect::AglPostProcEffect(const char* effectName)
    : name(effectName), nameHash(effectName ? ape::HashNameNoCase(effectName) : 0)
{
}

AglPostProcEffect::~AglPostProcEffect() = default;

// @func 0x0051F09C AglFadingEffect::SetEnabled
void AglFadingEffect::SetEnabled(bool on)
{
    if (on) {
        enabled = true;
        fadeTimer = 0.0f;
        fadingOut = false;
        fadingIn = true;
        return;
    }
    // Disabling starts the fade-out (once).
    if (!fadingOut && enabled) {
        fadingOut = true;
        fadeTimer = fadeTime;
    }
}

// @func 0x0051F160 AglFadingEffect::UpdateFade
void AglFadingEffect::UpdateFade(float dt)
{
    if (!fadingOut)
        return;
    fadeTimer -= dt;
    if (fadeTimer <= 0.0f) {
        enabled = false;
        fadingOut = false;
    }
}

// @func 0x0051F3C0 AglOwningEffect::~AglOwningEffect
// @func 0x0051F4B0 AglOwningEffect::~AglOwningEffect (copy 2)
// @func 0x0051F42C AglOwningEffect::~AglOwningEffect (deleting)
AglOwningEffect::~AglOwningEffect()
{
    if (resource)
        resource->Release();
}

// --- thermal vision ------------------------------------------------------

// @func 0x0051F708 AglThermalVisionEffect::AglThermalVisionEffect
AglThermalVisionEffect::AglThermalVisionEffect()
    : AglPostProcEffect(nullptr)
{
}

void AglThermalVisionEffect::Apply()
{
    // TODO(0x0054664C AglThermalVisionEffect::Apply)
}

// --- screen warp -----------------------------------------------------------

// @func 0x0051F1C0 AglScreenWarpEffect::AglScreenWarpEffect
AglScreenWarpEffect::AglScreenWarpEffect()
    : AglOwningEffect("ScreenWarp")
{
    const AglDisplaySettings& display = GetDisplaySettings();
    std::copy(display.defaultWarpColor, display.defaultWarpColor + 4, color);
    field_166C = 1;
    field_1670 = 0;
    field_1674 = 0;
    const float width = display.width;
    const float height = display.height;
    rect[0] = width;
    rect[1] = 0.0f;
    rect[2] = width;
    rect[3] = height;
    rect[4] = 0.0f;
    rect[5] = height;
    SetGridSize(8, 8);
    field_1650 = 0;
    field_1658 = 0;
    field_165C = 0;
    strength[0] = 1.0f;
    strength[1] = 1.0f;
}

// @func 0x0051F520 AglScreenWarpEffect::SetGridSize
void AglScreenWarpEffect::SetGridSize(uint8_t newColumns, uint8_t newRows)
{
    columns = std::min<uint8_t>(newColumns, kMaxGrid);
    rows = std::min<uint8_t>(newRows, kMaxGrid);
    // One triangle strip over the whole grid: each row zig-zags between
    // the vertices of the row above (top) and its own (bottom); rows are
    // joined with two degenerate indices.
    indexCount = 0;
    uint16_t top = static_cast<uint16_t>(columns + 1);
    uint16_t bottom = 0;
    for (uint8_t r = 0; r < rows; ++r) {
        uint16_t lastBottom = bottom;
        for (uint8_t c = 0; c <= columns; ++c) {
            lastBottom = bottom;
            indices[indexCount++] = top++;
            indices[indexCount++] = bottom++;
        }
        indices[indexCount++] = lastBottom;
        indices[indexCount++] = top;
    }
    // The last row needs no join.
    indexCount = static_cast<uint16_t>(indexCount - 2);
}

void AglScreenWarpEffect::Apply()
{
    // TODO(0x00545A38 AglScreenWarpEffect::Apply)
}

// --- motion trail ------------------------------------------------------------

// @func 0x0051EF28 AglMotionTrailEffect::AglMotionTrailEffect
AglMotionTrailEffect::AglMotionTrailEffect()
    : AglFadingEffect(nullptr)
{
    constexpr float kFrameTime = 0.016f;
    constexpr float kDecay = 0.75f;          // trail intensity kept per frame
    constexpr float kVisible = 0.05f;        // intensity below which the trail is gone
    const float step = std::max(std::min(0.1f, kFrameTime), kFrameTime);
    timeStep = step;
    // The fade-out lasts as many frames as the trail takes to decay.
    uint16_t frames = 0;
    float intensity = 1.0f;
    for (; frames < 60; ++frames) {
        intensity *= kDecay;
        if (intensity < kVisible)
            break;
    }
    fadeTime = static_cast<float>(frames) * timeStep;
    fadeTimer = 0.0f;
    fadingOut = false;
    fadingIn = false;
}

void AglMotionTrailEffect::Apply()
{
    // TODO(0x00544870 AglMotionTrailEffect::Apply)
}

// @func 0x00543BAC AglMotionTrailEffect::Init
void AglMotionTrailEffect::Init()
{
    static bool initialised = false;
    if (!initialised) {
        initialised = true;
        LoadTuning();
    }
}

// @func 0x00543C04 AglMotionTrailEffect::LoadTuning
void AglMotionTrailEffect::LoadTuning()
{
    const float* t = GetPostProcTuning().values;
    tuning[3] = t[8];    // 0x98 <- 0x01164060
    tuning[1] = t[9];    // 0x90 <- 0x01164064
    tuning[4] = t[12];   // 0x9C <- 0x01164070
    tuning[2] = t[13];   // 0x94 <- 0x01164074
    tuning[0] = t[15];   // 0x8C <- 0x0116407C
}

// --- god rays, blur, depth of field ---------------------------------------

AglPostProcTuning& GetPostProcTuning()
{
    static AglPostProcTuning tuning;
    return tuning;
}

// @func 0x005428B0 AglGodRaysEffect::Init
void AglGodRaysEffect::Init()
{
    static bool initialised = false;
    if (!initialised) {
        initialised = true;
        LoadTuning();
    }
}

// @func 0x00542908 AglGodRaysEffect::LoadTuning
void AglGodRaysEffect::LoadTuning()
{
    const float* t = GetPostProcTuning().values;
    tuning[0] = t[14];   // 0x84 <- 0x01164078
    tuning[1] = t[1];    // 0x88 <- 0x01164044
    tuning[4] = t[0];    // 0x94 <- 0x01164040
    tuning[2] = t[5];    // 0x8C <- 0x01164054
    tuning[5] = t[4];    // 0x98 <- 0x01164050
    tuning[3] = t[7];    // 0x90 <- 0x0116405C
    tuning[6] = t[6];    // 0x9C <- 0x01164058
}

namespace {
bool g_blurInitialised = false;         // 0x00F679FC
bool g_dofInitialised = false;          // 0x00F679FE
bool g_bloomNoiseTuningLoaded = false;  // 0x00F679FF
}

// @func 0x0053E164 AglBlurEffect::Init
void AglBlurEffect::Init()
{
    g_blurInitialised = true;
}

// @func 0x0053F874 AglDofEffect::Init
void AglDofEffect::Init()
{
    g_dofInitialised = true;
}

// @func 0x00540DA4 AglBloomNoiseEffect::LoadTuning
void AglBloomNoiseEffect::LoadTuning()
{
    // Nothing to read: only marks the defaults as loaded.
    g_bloomNoiseTuningLoaded = true;
}

// --- bloom noise -------------------------------------------------------------

namespace {
uint32_t g_noiseSeed = 0x12345678;   // 0x01164890
uint32_t g_noiseValue = 0;           // 0x01164888
}

// @func 0x0051F660 AglBloomNoiseEffect::RandomizeOffset
void AglBloomNoiseEffect::RandomizeOffset()
{
    // Two rounds of a rotate/add/xor mix; the previous and the new 16-bit
    // value give the texture offset in [0, 1).
    auto rotl = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
    const uint32_t a = (rotl(g_noiseSeed, 4) + rotl(g_noiseSeed, 16)) ^ 0x87654321u;
    const uint32_t b = (rotl(a, 20) + a) ^ 0x87654321u;
    const float previous = static_cast<float>(g_noiseValue);
    g_noiseSeed = rotl(b, 16);
    g_noiseValue = b & 0xFFFF;
    constexpr float kScale = 1.0f / 65535.0f;
    noiseOffset[0] = previous * kScale;
    noiseOffset[1] = static_cast<float>(a & 0xFFFF) * kScale;
}

// --- chain -------------------------------------------------------------------

// @func 0x0051F850 AglPostProcChain::AglPostProcChain
AglPostProcChain::AglPostProcChain()
{
    effects[kThermalVision] = &m_thermalVision;
    effects[kBloom] = &m_bloom;
    effects[kDoF] = &m_dof;
    effects[kBlur] = &m_blur;
    effects[kRadialBlur] = &m_radialBlur;
    effects[kDesaturation] = &m_desaturation;
    effects[kScreenWarp] = &m_screenWarp;
    effects[kGodRays] = &m_godRays;
    effects[kMotionTrail] = &m_motionTrail;
    effects[kBloomNoise] = &m_bloomNoise;
    for (AglPostProcEffect* effect : effects)
        effect->SetEnabled(false);
}

AglPostProcChain::~AglPostProcChain() = default;

// --- start-up ------------------------------------------------------------------

// @func 0x0051FF1C InitPostProcShaders
void InitPostProcShaders()
{
    LoadThermalVisionShaders();
    LoadBloomShaders();
    LoadDofShaders();
    LoadMonochromeShaders();
    LoadWarpShaders();
    InitGodRaysShaders();
    InitMotionTrailShaders();
    LoadBloomNoiseShaders();
    if (!AreSlyShadersDisabled())
        LoadDarkScreenShaders();
}

} // namespace agl
