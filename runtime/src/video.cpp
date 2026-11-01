#include "video.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "input.h"
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
u8 axis_u8(Sint16 v) { return static_cast<u8>((static_cast<int>(v) + 32768) >> 8); }

// Read the keyboard and the first game controller into pad 0.
void poll_input(SDL_GameController* gc) {
    input::Pad p;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    auto key = [&](SDL_Scancode s) { return k[s] != 0; };
    u16 d1 = 0, d2 = 0;
    if (key(SDL_SCANCODE_I)) d1 |= input::BTN_UP;
    if (key(SDL_SCANCODE_K)) d1 |= input::BTN_DOWN;
    if (key(SDL_SCANCODE_J)) d1 |= input::BTN_LEFT;
    if (key(SDL_SCANCODE_L)) d1 |= input::BTN_RIGHT;
    if (key(SDL_SCANCODE_RETURN)) d1 |= input::BTN_START;
    if (key(SDL_SCANCODE_BACKSPACE)) d1 |= input::BTN_SELECT;
    if (key(SDL_SCANCODE_Z)) d1 |= input::BTN_L3;
    if (key(SDL_SCANCODE_X)) d1 |= input::BTN_R3;
    if (key(SDL_SCANCODE_SPACE)) d2 |= input::BTN_CROSS;
    if (key(SDL_SCANCODE_F)) d2 |= input::BTN_SQUARE;
    if (key(SDL_SCANCODE_C)) d2 |= input::BTN_CIRCLE;
    if (key(SDL_SCANCODE_R)) d2 |= input::BTN_TRIANGLE;
    if (key(SDL_SCANCODE_Q)) d2 |= input::BTN_L1;
    if (key(SDL_SCANCODE_E)) d2 |= input::BTN_R1;
    if (key(SDL_SCANCODE_LSHIFT)) d2 |= input::BTN_L2;
    if (key(SDL_SCANCODE_LCTRL)) d2 |= input::BTN_R2;
    int lx = 128, ly = 128, rx = 128, ry = 128;
    if (key(SDL_SCANCODE_A)) lx = 0;
    if (key(SDL_SCANCODE_D)) lx = 255;
    if (key(SDL_SCANCODE_W)) ly = 0;
    if (key(SDL_SCANCODE_S)) ly = 255;
    if (key(SDL_SCANCODE_LEFT)) rx = 0;
    if (key(SDL_SCANCODE_RIGHT)) rx = 255;
    if (key(SDL_SCANCODE_UP)) ry = 0;
    if (key(SDL_SCANCODE_DOWN)) ry = 255;
    u8 l2 = (d2 & input::BTN_L2) ? 255 : 0, r2 = (d2 & input::BTN_R2) ? 255 : 0;
    if (gc) {
        auto btn = [&](SDL_GameControllerButton b) { return SDL_GameControllerGetButton(gc, b) != 0; };
        if (btn(SDL_CONTROLLER_BUTTON_DPAD_UP)) d1 |= input::BTN_UP;
        if (btn(SDL_CONTROLLER_BUTTON_DPAD_DOWN)) d1 |= input::BTN_DOWN;
        if (btn(SDL_CONTROLLER_BUTTON_DPAD_LEFT)) d1 |= input::BTN_LEFT;
        if (btn(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) d1 |= input::BTN_RIGHT;
        if (btn(SDL_CONTROLLER_BUTTON_START)) d1 |= input::BTN_START;
        if (btn(SDL_CONTROLLER_BUTTON_BACK)) d1 |= input::BTN_SELECT;
        if (btn(SDL_CONTROLLER_BUTTON_LEFTSTICK)) d1 |= input::BTN_L3;
        if (btn(SDL_CONTROLLER_BUTTON_RIGHTSTICK)) d1 |= input::BTN_R3;
        if (btn(SDL_CONTROLLER_BUTTON_A)) d2 |= input::BTN_CROSS;
        if (btn(SDL_CONTROLLER_BUTTON_B)) d2 |= input::BTN_CIRCLE;
        if (btn(SDL_CONTROLLER_BUTTON_X)) d2 |= input::BTN_SQUARE;
        if (btn(SDL_CONTROLLER_BUTTON_Y)) d2 |= input::BTN_TRIANGLE;
        if (btn(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) d2 |= input::BTN_L1;
        if (btn(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) d2 |= input::BTN_R1;
        const int tl = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7;
        const int tr = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7;
        if (tl > l2) l2 = static_cast<u8>(tl);
        if (tr > r2) r2 = static_cast<u8>(tr);
        if (tl > 64) d2 |= input::BTN_L2;
        if (tr > 64) d2 |= input::BTN_R2;
        auto stick = [](int key_value, Sint16 axis) {
            const int a = axis_u8(axis);
            return key_value != 128 ? key_value : (a > 112 && a < 144 ? 128 : a);  // small dead zone
        };
        lx = stick(lx, SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX));
        ly = stick(ly, SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY));
        rx = stick(rx, SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX));
        ry = stick(ry, SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY));
    }
    p.digital1 = d1;
    p.digital2 = d2;
    p.lx = static_cast<u8>(lx);
    p.ly = static_cast<u8>(ly);
    p.rx = static_cast<u8>(rx);
    p.ry = static_cast<u8>(ry);
    auto pr = [](bool on) { return static_cast<u8>(on ? 255 : 0); };
    p.pressure[0] = pr(d1 & input::BTN_RIGHT);
    p.pressure[1] = pr(d1 & input::BTN_LEFT);
    p.pressure[2] = pr(d1 & input::BTN_UP);
    p.pressure[3] = pr(d1 & input::BTN_DOWN);
    p.pressure[4] = pr(d2 & input::BTN_TRIANGLE);
    p.pressure[5] = pr(d2 & input::BTN_CIRCLE);
    p.pressure[6] = pr(d2 & input::BTN_CROSS);
    p.pressure[7] = pr(d2 & input::BTN_SQUARE);
    p.pressure[8] = pr(d2 & input::BTN_L1);
    p.pressure[9] = pr(d2 & input::BTN_R1);
    p.pressure[10] = l2;
    p.pressure[11] = r2;
    input::set_pad(0, p);
}

void window_thread(std::string title, int scale) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
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
    SDL_GameController* gc = nullptr;
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) rtm::exit_process(0);
            if (e.type == SDL_CONTROLLERDEVICEADDED && !gc) {
                gc = SDL_GameControllerOpen(e.cdevice.which);
                if (gc) LOG_INFO("input", "controller: %s", SDL_GameControllerName(gc));
            }
            if (e.type == SDL_CONTROLLERDEVICEREMOVED && gc &&
                e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc))) {
                SDL_GameControllerClose(gc);
                gc = nullptr;
                LOG_INFO("input", "controller disconnected");
            }
        }
        poll_input(gc);
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
