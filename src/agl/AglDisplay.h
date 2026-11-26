// openTIT - Agl display settings and frame command buffer housekeeping.
#pragma once

#include "agl/gpu/CommandList.h"

#include <cstddef>
#include <cstdint>

namespace ape { struct Matrix44; }

namespace agl {

// Output settings chosen at start-up (globals 0x010C5A24..0x010C5A4C and
// the default colors at 0x01164CA0).
struct AglDisplaySettings {
    uint16_t width = 1280;                  // 0x010C5A24
    uint16_t height = 720;                  // 0x010C5A26
    float aspect = 16.0f / 9.0f;            // 0x010C5A30
    bool widescreen = true;                 // 0x010C5A41: 16:9 output (else 4:3)
    bool stereoOutput = false;              // 0x010C5A42: frame packed 3D output active
    bool stereoAvailable = false;           // 0x010C5A43: the TV supports 3D
    uint16_t maxLights = 63;                // 0x010C5A4C
    bool postProcessEnabled = true;         // 0x00F66FC0
    uint8_t debugFlags = 0;                 // 0x010C5AB8 (bit 4 disables post-processing)
    uint8_t defaultWarpColor[4] = { 0x80, 0x80, 0x80, 0xFF };   // 0x01164CA0
    uint8_t defaultFogColor[4] = { 0x80, 0x80, 0x80, 0xFF };    // 0x01164CA4
};

AglDisplaySettings& GetDisplaySettings();
bool IsPostProcessEnabled();

// Scene light colors shared by every render target (0x010C5B00, scaled by 0x010C5B60).
struct AglLightingSettings {
    float colors[6][4] = {};
    float intensity = 1.0f;
};
AglLightingSettings& GetLightingSettings();

// --- stereoscopic 3D ---

// Which image a frame renders.
enum AglStereoMode : int32_t {
    kAglStereo_Mono  = 0,   // projection unchanged
    kAglStereo_Left  = 1,
    kAglStereo_Right = 2,
    kAglStereo_Off   = 3,   // stereo processing skipped entirely
};

AglStereoMode GetStereoMode();
void SetStereoMode(AglStereoMode mode);
// Requests stereo output (only honoured when the TV supports it).
void SetStereoEnabled(bool enable);
bool IsStereoEnabled();
void SetStereoSeparation(float separation);
void SetStereoConvergence(float convergence);
void SetStereoParams(float separation, float convergence);
float GetStereoSeparation();
float GetStereoConvergence();
// Offsets a projection matrix for one eye.
void StereoAdjustProjection(ape::Matrix44& out, const ape::Matrix44& projection, float separation,
                            float convergence, AglStereoMode mode);

// Sets the display gamma, clamped to the range the options menu allows.
void SetDisplayGamma(float gamma);
float GetDisplayGamma();

// Overflow handler of the frame command lists: reports the overflow and
// lets recording continue.
bool OnFrameCommandOverflow(gpu::CommandList& list, size_t commandsNeeded);
// Overflow handler of pre-recorded blocks (vertex program loads): ignored.
bool OnBlockCommandOverflow(gpu::CommandList& list, size_t commandsNeeded);

} // namespace agl
