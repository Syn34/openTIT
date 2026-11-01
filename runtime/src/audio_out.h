// Host audio output (SDL2): 48 kHz stereo float.
#pragma once

#include "common/types.h"

namespace opentit::audio_out {

bool start();  // false when unavailable (no SDL2, headless, no device)
void push(const float* stereo, u32 frames);

// Mix one cellAudio block (256 frames of big-endian floats with `channels`
// interleaved) into stereo `out` (512 floats), adding to what is there.
void mix_block(const u8* block, u32 channels, float level, float* out);

}  // namespace opentit::audio_out
