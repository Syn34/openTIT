// openTIT - Agl display settings and frame command buffer housekeeping.
#pragma once

#include "agl/gpu/CommandList.h"

#include <cstddef>

namespace agl {

// Sets the display gamma, clamped to the range the options menu allows.
void SetDisplayGamma(float gamma);
float GetDisplayGamma();

// Overflow handler of the frame command lists: reports the overflow and
// lets recording continue.
bool OnFrameCommandOverflow(gpu::CommandList& list, size_t commandsNeeded);
// Overflow handler of pre-recorded blocks (vertex program loads): ignored.
bool OnBlockCommandOverflow(gpu::CommandList& list, size_t commandsNeeded);

} // namespace agl
