// openTIT - Screen-space wave distortions.
#include "sly/gameplay/WaveDistortion.h"

#include <algorithm>

namespace sly {
namespace gameplay {

namespace {
// @func 0x0042A37C StaticInit_WaveDistortion
// @func 0x0042A314 StaticDestroy_WaveDistortion
// Compiler-generated initialiser/finaliser of this file's globals: the
// ripple buffers (freed at exit) and the distortion table (defaults of
// WaveDistortionDesc, all inactive).
struct RippleState {
    agl::AglMemBlock buffers[3];
    agl::AglMemBlock extra;
    WaveDistortion waves[kMaxWaveDistortions];

    ~RippleState()
    {
        agl::MemBlockFree(&extra);
        for (int i = 2; i >= 0; --i)
            agl::MemBlockFree(&buffers[i]);
    }
};
RippleState g_ripples;
} // namespace

WaveDistortion* WaveDistortions() { return g_ripples.waves; }
agl::AglMemBlock* RippleBuffers() { return g_ripples.buffers; }
agl::AglMemBlock& RippleExtraBuffer() { return g_ripples.extra; }

// @func 0x0042A248 AddWaveDistortion
void AddWaveDistortion(const WaveDistortionDesc& desc)
{
    for (WaveDistortion& wave : g_ripples.waves) {
        if (wave.active)
            continue;
        wave.desc = desc;
        wave.desc.easeIn = std::max(desc.easeIn, 0.0f);
        wave.desc.duration = std::max(desc.duration, 0.0f);
        wave.desc.easeOut = std::max(desc.easeOut, 0.0f);
        wave.time = 0.0f;
        wave.active = true;
        return;
    }
}

} // namespace gameplay
} // namespace sly
