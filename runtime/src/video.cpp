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
#elif defined(OPENTIT_HAS_WIN32_UI)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

#if !defined(OPENTIT_HAS_SDL2) && defined(OPENTIT_HAS_WIN32_UI)
// ---------------------------------------------------------------- native Win32 window + XInput
struct XPad { WORD buttons; BYTE lt, rt; SHORT lx, ly, rx, ry; };
struct XState { DWORD packet; XPad pad; };
using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XState*);
XInputGetStateFn g_xinput = nullptr;
HWND g_hwnd = nullptr;
std::vector<u8> g_shown;
u32 g_sw = 0, g_sh = 0;

LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CLOSE: rtm::exit_process(0);
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc;
        GetClientRect(h, &rc);
        if (g_sw) {
            BITMAPINFO bi{};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = static_cast<LONG>(g_sw);
            bi.bmiHeader.biHeight = -static_cast<LONG>(g_sh);  // top-down
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            SetStretchBltMode(dc, HALFTONE);
            StretchDIBits(dc, 0, 0, rc.right, rc.bottom, 0, 0, static_cast<int>(g_sw), static_cast<int>(g_sh), g_shown.data(), &bi,
                          DIB_RGB_COLORS, SRCCOPY);
        } else {
            FillRect(dc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    default: return DefWindowProcA(h, m, w, l);
    }
}

u8 axis_u8(SHORT v, bool invert) {
    int a = (static_cast<int>(v) + 32768) >> 8;
    if (invert) a = 255 - a;
    return static_cast<u8>(a > 112 && a < 144 ? 128 : a);
}

void poll_input() {
    input::Pad p;
    const bool focused = GetForegroundWindow() == g_hwnd;
    auto key = [&](int vk) { return focused && (GetAsyncKeyState(vk) & 0x8000) != 0; };
    u16 d1 = 0, d2 = 0;
    if (key('I')) d1 |= input::BTN_UP;
    if (key('K')) d1 |= input::BTN_DOWN;
    if (key('J')) d1 |= input::BTN_LEFT;
    if (key('L')) d1 |= input::BTN_RIGHT;
    if (key(VK_RETURN)) d1 |= input::BTN_START;
    if (key(VK_BACK)) d1 |= input::BTN_SELECT;
    if (key('Z')) d1 |= input::BTN_L3;
    if (key('X')) d1 |= input::BTN_R3;
    if (key(VK_SPACE)) d2 |= input::BTN_CROSS;
    if (key('F')) d2 |= input::BTN_SQUARE;
    if (key('C')) d2 |= input::BTN_CIRCLE;
    if (key('R')) d2 |= input::BTN_TRIANGLE;
    if (key('Q')) d2 |= input::BTN_L1;
    if (key('E')) d2 |= input::BTN_R1;
    if (key(VK_LSHIFT)) d2 |= input::BTN_L2;
    if (key(VK_LCONTROL)) d2 |= input::BTN_R2;
    int lx = 128, ly = 128, rx = 128, ry = 128;
    if (key('A')) lx = 0;
    if (key('D')) lx = 255;
    if (key('W')) ly = 0;
    if (key('S')) ly = 255;
    if (key(VK_LEFT)) rx = 0;
    if (key(VK_RIGHT)) rx = 255;
    if (key(VK_UP)) ry = 0;
    if (key(VK_DOWN)) ry = 255;
    u8 l2 = (d2 & input::BTN_L2) ? 255 : 0, r2 = (d2 & input::BTN_R2) ? 255 : 0;
    XState xs{};
    if (g_xinput && g_xinput(0, &xs) == 0) {
        const WORD b = xs.pad.buttons;
        if (b & 0x0001) d1 |= input::BTN_UP;
        if (b & 0x0002) d1 |= input::BTN_DOWN;
        if (b & 0x0004) d1 |= input::BTN_LEFT;
        if (b & 0x0008) d1 |= input::BTN_RIGHT;
        if (b & 0x0010) d1 |= input::BTN_START;
        if (b & 0x0020) d1 |= input::BTN_SELECT;
        if (b & 0x0040) d1 |= input::BTN_L3;
        if (b & 0x0080) d1 |= input::BTN_R3;
        if (b & 0x0100) d2 |= input::BTN_L1;
        if (b & 0x0200) d2 |= input::BTN_R1;
        if (b & 0x1000) d2 |= input::BTN_CROSS;
        if (b & 0x2000) d2 |= input::BTN_CIRCLE;
        if (b & 0x4000) d2 |= input::BTN_SQUARE;
        if (b & 0x8000) d2 |= input::BTN_TRIANGLE;
        if (xs.pad.lt > l2) l2 = xs.pad.lt;
        if (xs.pad.rt > r2) r2 = xs.pad.rt;
        if (xs.pad.lt > 64) d2 |= input::BTN_L2;
        if (xs.pad.rt > 64) d2 |= input::BTN_R2;
        if (lx == 128) lx = axis_u8(xs.pad.lx, false);
        if (ly == 128) ly = axis_u8(xs.pad.ly, true);  // XInput: up is positive
        if (rx == 128) rx = axis_u8(xs.pad.rx, false);
        if (ry == 128) ry = axis_u8(xs.pad.ry, true);
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
    for (const char* dll : {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"}) {
        if (HMODULE x = LoadLibraryA(dll)) {
            g_xinput = reinterpret_cast<XInputGetStateFn>(reinterpret_cast<void*>(GetProcAddress(x, "XInputGetState")));
            if (g_xinput) break;
        }
    }
    WNDCLASSA wc{};
    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "openTIT";
    RegisterClassA(&wc);
    RECT r{0, 0, 1280 * scale / 2, 720 * scale / 2};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowA("openTIT", title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) {
        LOG_WARN("video", "could not create the window");
        return;
    }
    g_active = true;
    LOG_INFO("video", "window open%s", g_xinput ? " (XInput controllers enabled)" : "");
    std::vector<u8> local;
    for (;;) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        poll_input();
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
        // Guest A,R,G,B bytes -> DIB B,G,R,x: a byte swap per pixel.
        g_shown.resize(local.size());
        for (std::size_t k = 0; k + 4 <= local.size(); k += 4) {
            g_shown[k] = local[k + 3];
            g_shown[k + 1] = local[k + 2];
            g_shown[k + 2] = local[k + 1];
            g_shown[k + 3] = 0;
        }
        g_sw = w;
        g_sh = h;
        InvalidateRect(g_hwnd, nullptr, FALSE);
        UpdateWindow(g_hwnd);
        ++g_presented;
    }
}
#endif
}  // namespace

bool start(const std::string& title, int scale) {
#if defined(OPENTIT_HAS_SDL2) || defined(OPENTIT_HAS_WIN32_UI)
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
