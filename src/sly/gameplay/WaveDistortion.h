// openTIT - Screen-space wave distortions ("WaveDistort" script command).
//
// Up to 16 ripples can run at once (table 0x010AA394, 0x28 bytes each). A
// ripple is centred on an entity, eases in, holds for its duration and eases
// out; the water/ripple renderer (0x004286D8..0x0042AD9C, not ported) reads
// the table every frame.
#pragma once

#include "agl/AglTypes.h"

#include <cstdint>

namespace sly {

class WorldObject;

namespace gameplay {

struct WaveDistortionDesc {
    WorldObject* entity = nullptr;   // +0x00 centre
    float easeIn = 0.0f;             // +0x04 seconds
    float duration = 0.0f;           // +0x08 seconds at full strength
    float easeOut = 0.0f;            // +0x0C seconds
    float radius = 2.0f;             // +0x10
    float amplitude = -1.0f;         // +0x14
    float frequency = 0.0f;          // +0x18
    float waveOffset = 1.5707964f;   // +0x1C phase (pi/2)
};

struct WaveDistortion {
    WaveDistortionDesc desc;
    bool active = false;             // +0x20
    float time = 0.0f;               // +0x24 seconds since start
};

constexpr int kMaxWaveDistortions = 16;
WaveDistortion* WaveDistortions();

// Starts a ripple in the first free slot (ignored when all 16 are busy).
// Negative times are clamped to zero.
void AddWaveDistortion(const WaveDistortionDesc& desc);

// GPU buffers of the ripple renderer (0x010AA1B8, three plus one).
// NOTE: their exact use is in the unported renderer.
agl::AglMemBlock* RippleBuffers();       // [3]
agl::AglMemBlock& RippleExtraBuffer();

} // namespace gameplay
} // namespace sly
