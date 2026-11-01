#include "audio_out.h"

#include <atomic>
#include <cstring>

#include "log.h"
#include "runtime.h"

#ifdef OPENTIT_HAS_SDL2
#include <SDL.h>
#endif

namespace opentit::audio_out {

namespace {
#ifdef OPENTIT_HAS_SDL2
SDL_AudioDeviceID g_dev = 0;
#endif
std::atomic<bool> g_on{false};

float be_float(const u8* p) {
    const u32 v = load_be<u32>(p);
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}
}  // namespace

bool start() {
#ifdef OPENTIT_HAS_SDL2
    if (g_on) return true;
    if (rtm::config().headless) return false;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        LOG_WARN("audio", "no audio (%s)", SDL_GetError());
        return false;
    }
    SDL_AudioSpec want{}, have{};
    want.freq = 48000;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 512;
    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!g_dev) {
        LOG_WARN("audio", "could not open an audio device (%s)", SDL_GetError());
        return false;
    }
    SDL_PauseAudioDevice(g_dev, 0);
    g_on = true;
    LOG_INFO("audio", "audio output at %d Hz", have.freq);
    return true;
#else
    return false;
#endif
}

void push(const float* stereo, u32 frames) {
#ifdef OPENTIT_HAS_SDL2
    if (!g_on) return;
    // Keep latency bounded: drop audio if the device has fallen far behind.
    if (SDL_GetQueuedAudioSize(g_dev) > 48000 * 2 * 4 / 8) return;
    SDL_QueueAudio(g_dev, stereo, frames * 2 * 4);
#else
    (void)stereo;
    (void)frames;
#endif
}

void mix_block(const u8* block, u32 channels, float level, float* out) {
    for (u32 f = 0; f < 256; ++f) {
        const u8* s = block + static_cast<std::size_t>(f) * channels * 4;
        float l, r;
        if (channels >= 8) {
            // L R C LFE Ls Rs Lb Rb
            const float c = be_float(s + 8) * 0.7071f;
            l = be_float(s) + c + 0.7071f * (be_float(s + 16) + be_float(s + 24));
            r = be_float(s + 4) + c + 0.7071f * (be_float(s + 20) + be_float(s + 28));
        } else if (channels >= 2) {
            l = be_float(s);
            r = be_float(s + 4);
        } else {
            l = r = be_float(s);
        }
        out[2 * f] += l * level;
        out[2 * f + 1] += r * level;
    }
}

}  // namespace opentit::audio_out
