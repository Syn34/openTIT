#include "video.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "log.h"
#include "runtime.h"

#ifdef OPENTIT_HAS_SDL2
#include <SDL.h>
#endif

namespace opentit::video {

namespace {
std::mutex g_m;
std::condition_variable g_cv;
std::vector<u8> g_frame;
u32 g_w = 0, g_h = 0;
bool g_new = false;
std::atomic<bool> g_active{false};
std::atomic<u64> g_presented{0};

#ifdef OPENTIT_HAS_SDL2
void window_thread(std::string title, int scale) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_WARN("video", "SDL_Init failed (%s): running without a window", SDL_GetError());
        return;
    }
    SDL_Window* win = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280 * scale / 2,
                                       720 * scale / 2, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!ren && win) ren = SDL_CreateRenderer(win, -1, 0);
    if (!win || !ren) {
        LOG_WARN("video", "could not create the window (%s)", SDL_GetError());
        return;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(ren, 1280, 720);
    SDL_Texture* tex = nullptr;
    u32 tw = 0, th = 0;
    g_active = true;
    LOG_INFO("video", "window open");
    std::vector<u8> local;
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) rtm::exit_process(0);
        }
        u32 w = 0, h = 0;
        {
            std::unique_lock<std::mutex> lk(g_m);
            g_cv.wait_for(lk, std::chrono::milliseconds(8), [] { return g_new; });
            if (g_new) {
                local.swap(g_frame);
                w = g_w;
                h = g_h;
                g_new = false;
            }
        }
        if (!w) continue;
        if (!tex || tw != w || th != h) {
            if (tex) SDL_DestroyTexture(tex);
            // BGRA8888 as a native 32-bit value is A,R,G,B in memory on little-endian hosts.
            tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, static_cast<int>(w),
                                    static_cast<int>(h));
            tw = w;
            th = h;
            SDL_RenderSetLogicalSize(ren, static_cast<int>(w), static_cast<int>(h));
        }
        SDL_UpdateTexture(tex, nullptr, local.data(), static_cast<int>(w * 4));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        ++g_presented;
    }
}
#endif
}  // namespace

bool start(const std::string& title, int scale) {
#ifdef OPENTIT_HAS_SDL2
    std::thread(window_thread, title, scale < 1 ? 2 : scale).detach();
    return true;
#else
    (void)title;
    (void)scale;
    LOG_INFO("video", "built without SDL2: no window");
    return false;
#endif
}

bool active() { return g_active; }

void present(const u8* argb, u32 pitch, u32 w, u32 h) {
    if (!w || !h || w > 4096 || h > 4096) return;
    std::lock_guard<std::mutex> lk(g_m);
    g_frame.resize(static_cast<std::size_t>(w) * h * 4);
    for (u32 y = 0; y < h; ++y) std::memcpy(&g_frame[static_cast<std::size_t>(y) * w * 4], argb + y * pitch, w * 4);
    g_w = w;
    g_h = h;
    g_new = true;
    g_cv.notify_one();
}

u64 frames_presented() { return g_presented; }

}  // namespace opentit::video
