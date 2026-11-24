// openTIT - Agl display settings and frame command buffer housekeeping.
#include "agl/AglDisplay.h"

#include "agl/gpu/Device.h"

#include "ape/math/ApeMatrixOps.h"

#include <cstdio>

namespace agl {

namespace {

constexpr float kMinGamma = 0.8f;
constexpr float kMaxGamma = 1.2f;

float g_gamma = 1.0f;   // 0x010F0E58

AglDisplaySettings g_display;
AglLightingSettings g_lighting;

AglStereoMode g_stereoMode = kAglStereo_Mono;   // 0x010F0ED8
bool g_stereoEnabled = false;                   // 0x00F671DC
float g_stereoSeparation = 1.0f;                // 0x010F0EC8
float g_stereoConvergence = 0.05f;              // 0x010F0ECC

constexpr float kMinSeparation = 0.1f;
constexpr float kMaxSeparation = 150.0f;
constexpr float kMinConvergence = 0.01f;
constexpr float kMaxConvergence = 0.1f;

float ClampF(float value, float lo, float hi)
{
    if (value < lo)
        return lo;
    return value <= hi ? value : hi;
}

} // namespace

AglDisplaySettings& GetDisplaySettings()
{
    return g_display;
}

bool IsPostProcessEnabled()
{
    return g_display.postProcessEnabled && !(g_display.debugFlags & 0x10);
}

AglLightingSettings& GetLightingSettings()
{
    return g_lighting;
}

AglStereoMode GetStereoMode()
{
    return g_stereoMode;
}

void SetStereoMode(AglStereoMode mode)
{
    g_stereoMode = mode;
}

// @func 0x00536958 AglStereo::SetEnabled
void SetStereoEnabled(bool enable)
{
    g_stereoEnabled = enable && g_display.stereoAvailable;
}

bool IsStereoEnabled()
{
    return g_stereoEnabled;
}

// @func 0x00536998 AglStereo::SetSeparation
void SetStereoSeparation(float separation)
{
    g_stereoSeparation = ClampF(separation, kMinSeparation, kMaxSeparation);
}

// @func 0x005369E8 AglStereo::SetConvergence
void SetStereoConvergence(float convergence)
{
    g_stereoConvergence = ClampF(convergence, kMinConvergence, kMaxConvergence);
}

// @func 0x00536A38 AglStereo::SetParams
void SetStereoParams(float separation, float convergence)
{
    g_stereoSeparation = ClampF(separation, kMinSeparation, kMaxSeparation);
    g_stereoConvergence = ClampF(convergence, kMinConvergence, kMaxConvergence);
}

float GetStereoSeparation()
{
    return g_stereoSeparation;
}

float GetStereoConvergence()
{
    return g_stereoConvergence;
}

// TODO(0x00536290 StereoAdjustProjection)
// The console uses a stereo utility routine (with its own parameter checks)
// that shifts the frustum for each eye. This is the standard parallel-axis
// asymmetric frustum offset.
void StereoAdjustProjection(ape::Matrix44& out, const ape::Matrix44& projection, float separation,
                            float convergence, AglStereoMode mode)
{
    out = projection;
    if (mode != kAglStereo_Left && mode != kAglStereo_Right)
        return;
    const float eye = mode == kAglStereo_Left ? -0.5f : 0.5f;
    const float shift = eye * separation * convergence;
    // Horizontal shift of clip x proportional to w (offset frustum) and an
    // eye translation scaled by the projection's x scale.
    out.rows[2].x += shift;
    out.rows[3].x -= shift * projection.rows[0].x;
}

// @func 0x0052F8A0 AglDisplay::SetGamma
void SetDisplayGamma(float gamma)
{
    float clamped = kMinGamma;
    if (gamma >= kMinGamma)
        clamped = gamma <= kMaxGamma ? gamma : kMaxGamma;
    if (g_gamma == clamped)
        return;
    g_gamma = clamped;
    if (gpu::Device* device = gpu::GetDevice())
        device->SetGamma(clamped);
}

float GetDisplayGamma()
{
    return g_gamma;
}

// @func 0x00537ED0 AglPs3CmndBuf::OnOverflow
bool OnFrameCommandOverflow(gpu::CommandList& list, size_t commandsNeeded)
{
    std::printf("AglPs3CmndBuf: Overflow: [%s], curr:%zu, overflow: %zu commands\n", list.GetName(),
                list.GetCommandCount(), commandsNeeded);
    return true;
}

// @func 0x00537818 AglVertexProgram::OnOverflow
bool OnBlockCommandOverflow(gpu::CommandList& list, size_t commandsNeeded)
{
    return true;
}

} // namespace agl
