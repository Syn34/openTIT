// openTIT - Agl display settings and frame command buffer housekeeping.
#include "agl/AglDisplay.h"

#include "agl/gpu/Device.h"

#include <cstdio>

namespace agl {

namespace {

constexpr float kMinGamma = 0.8f;
constexpr float kMaxGamma = 1.2f;

float g_gamma = 1.0f;   // 0x010F0E58

} // namespace

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
