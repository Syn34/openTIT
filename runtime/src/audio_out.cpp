#include "audio_out.h"

#include <atomic>
#include <cstring>

#include "log.h"
#include "runtime.h"

#ifdef OPENTIT_HAS_SDL2
#include <SDL.h>
#elif defined(OPENTIT_HAS_WIN32_UI)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mutex>
#include <vector>
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

#if !defined(OPENTIT_HAS_SDL2) && defined(OPENTIT_HAS_WIN32_UI)
// waveOut: a ring of buffers, each 1024 stereo float frames.
namespace {
constexpr int kBufs = 6;
constexpr u32 kFrames = 1024;
HWAVEOUT g_wo = nullptr;
WAVEHDR g_hdr[kBufs] = {};
std::vector<float> g_data[kBufs];
std::vector<float> g_pending;
std::mutex g_wm;
}  // namespace
#endif

bool start() {
#if !defined(OPENTIT_HAS_SDL2) && defined(OPENTIT_HAS_WIN32_UI)
    if (g_on) return true;
    if (rtm::config().headless) return false;
    WAVEFORMATEX f{};
    f.wFormatTag = 3;  // WAVE_FORMAT_IEEE_FLOAT
    f.nChannels = 2;
    f.nSamplesPerSec = 48000;
    f.wBitsPerSample = 32;
    f.nBlockAlign = 8;
    f.nAvgBytesPerSec = 48000 * 8;
    if (waveOutOpen(&g_wo, WAVE_MAPPER, &f, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        LOG_WARN("audio", "could not open an audio device");
        return false;
    }
    for (int k = 0; k < kBufs; ++k) {
        g_data[k].assign(kFrames * 2, 0.0f);
        g_hdr[k].lpData = reinterpret_cast<LPSTR>(g_data[k].data());
        g_hdr[k].dwBufferLength = kFrames * 8;
        g_hdr[k].dwFlags = WHDR_DONE;  // free
    }
    g_on = true;
    LOG_INFO("audio", "audio output at 48000 Hz (waveOut)");
    return true;
#elif defined(OPENTIT_HAS_SDL2)
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
#if !defined(OPENTIT_HAS_SDL2) && defined(OPENTIT_HAS_WIN32_UI)
    if (!g_on) return;
    std::lock_guard<std::mutex> lk(g_wm);
    g_pending.insert(g_pending.end(), stereo, stereo + 2 * frames);
    while (g_pending.size() >= kFrames * 2) {
        int free_buf = -1;
        for (int k = 0; k < kBufs; ++k)
            if (g_hdr[k].dwFlags & WHDR_DONE) { free_buf = k; break; }
        if (free_buf < 0) {  // device behind: drop to keep latency bounded
            g_pending.clear();
            return;
        }
        WAVEHDR& h = g_hdr[free_buf];
        if (h.dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(g_wo, &h, sizeof h);
        std::memcpy(g_data[free_buf].data(), g_pending.data(), kFrames * 8);
        g_pending.erase(g_pending.begin(), g_pending.begin() + kFrames * 2);
        h.dwFlags = 0;
        waveOutPrepareHeader(g_wo, &h, sizeof h);
        waveOutWrite(g_wo, &h, sizeof h);
    }
#elif defined(OPENTIT_HAS_SDL2)
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
