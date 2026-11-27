// openTIT - Application run-time state helpers.
#include "sly/game/AppRuntime.h"

#include "ape/core/ApeSystem.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/sys/ApeFrameClock.h"
#include "ape/level/ApeGui.h"
#include "agl/AglDisplay.h"
#include "crt/libc/Libc.h"
#include "sly/SlyApp.h"
#include "sly/tuning/GameTuning.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace crt {
[[noreturn]] void ExitImmediate(int code);
}

namespace sly {

namespace {
AppRuntime g_appRuntime;
bool g_shuttingDown = false;      // 0x010C5A45
bool g_skipCheckEnabled = false;  // 0x00F65C84
} // namespace

// @func 0x00457100 AppPlayer::~AppPlayer
// @func 0x004643F8 AppPlayer::~AppPlayer (deleting)
AppPlayer::~AppPlayer()
{
    if (pad) {
        pad->StopRumble(0);
        pad = nullptr;
    }
}

// @func 0x00464488 AppPlayer::Rumble
void AppPlayer::Rumble(float intensity, float duration)
{
    if (pad && pad->IsRumbleEnabled())
        pad->StartRumble(intensity, duration, 0);
}

// @func 0x004644E0 AppPlayer::StopRumble
void AppPlayer::StopRumble()
{
    if (pad)
        pad->StopRumble(0);
}

// @func 0x0046452C AppPlayer::SetRumblePaused
void AppPlayer::SetRumblePaused(bool paused)
{
    if (pad)
        pad->SetRumblePaused(paused);
}

// @func 0x00464578 AppPlayer::SetField_D0
void AppPlayer::SetField_D0(const float value[2])
{
    field_D0[0] = value[0];
    field_D0[1] = value[1];
}

// --- input ------------------------------------------------------------------

namespace {

// Directions the left stick maps to in menus: Left, Right, Up, Down
// (0x010BD6D0, filled on first use, and the d-pad bits 0x00AE8234).
struct StickDirection {
    float x, y;
    uint32_t button;
};
constexpr StickDirection kStickDirections[4] = {
    {-1.0f, 0.0f, ape::input::kButton_Left},
    {1.0f, 0.0f, ape::input::kButton_Right},
    {0.0f, -1.0f, ape::input::kButton_Up},
    {0.0f, 1.0f, ape::input::kButton_Down},
};
constexpr uint32_t kDPadButtons = 0xF;
constexpr uint32_t kMaxRepeatStage = 6;
constexpr float kStickDirectionHalfAngle = 0.7853981852531433f;   // 45 degrees

// Analog valid bits of the pad's pointer (analog values 16 and 17).
constexpr uint32_t kPointerAnalogBits = 0x30000;
constexpr int kAnalogPointerX = 16;
constexpr int kAnalogPointerY = 17;

// Delay before the next repeat of a held d-pad direction, by repeat stage.
float KeyRepeatDelay(uint32_t stage, bool secondColumn)
{
    const ape::TuningVec2* modes[kMaxRepeatStage + 1] = {
        &g_uIKeyRepeatMode0, &g_uIKeyRepeatMode1, &g_uIKeyRepeatMode2, &g_uIKeyRepeatMode3,
        &g_uIKeyRepeatMode4, &g_uIKeyRepeatMode5, &g_uIKeyRepeatMode6,
    };
    return modes[std::min(stage, kMaxRepeatStage)]->Get()[secondColumn ? 1 : 0];
}

} // namespace

// @func 0x0045B1E8 AppPlayer::UpdateInput
void AppPlayer::UpdateInput(float dt)
{
    using namespace ape::input;
    if (pad == nullptr)
        return;

    inputFlags = pad->GetButtons();
    inputMask = pad->GetChangedButtons();
    released = pad->GetReleasedButtons();
    analogMask = pad->GetAnalogValidMask();

    // In menus the left stick also drives the d-pad.
    if (padFlags & 0x8000) {
        const float x = (analogMask & 1) ? pad->GetAnalog(kAnalog_LeftX) : 0.0f;
        const float y = (analogMask & 2) ? pad->GetAnalog(kAnalog_LeftY) : 0.0f;
        const float lengthSq = x * x + y * y;
        const float deadzone = g_uIMapLStickToDPadDeadzone.Get();
        if (lengthSq >= deadzone * deadzone) {
            const float inv = 1.0f / std::sqrt(lengthSq);
            for (const StickDirection& dir : kStickDirections) {
                const float cosine = x * inv * dir.x + y * inv * dir.y;
                if (cosine > 0.0f && crt::ACosF(std::clamp(cosine, -1.0f, 1.0f)) <= kStickDirectionHalfAngle) {
                    inputFlags |= dir.button;
                    if (repeatTimer == 0.0f && repeatStage == 0)
                        inputMask |= dir.button;
                    break;
                }
            }
        }
    }

    // Key repeat of held d-pad directions, faster with every repeat.
    const uint32_t directions = inputFlags & kDPadButtons;
    if (directions == 0) {
        repeatTimer = 0.0f;
        repeatStage = 0;
    } else {
        repeatTimer += dt;
        if (KeyRepeatDelay(repeatStage, (padFlags & 0x4000) != 0) <= repeatTimer) {
            inputMask |= directions;
            repeatStage = static_cast<uint8_t>(std::min<uint32_t>(repeatStage + 1u, kMaxRepeatStage));
            repeatTimer = 0.0f;
        }
    }

    if (pad->IsPad()) {
        const ape::Vec3 sample{pad->GetAnalog(kAnalog_SensorX), pad->GetAnalog(kAnalog_SensorY),
                               pad->GetAnalog(kAnalog_SensorZ), 0.0f};
        motion.Record(sample);
    }

    // GUI pointer: the pad's own pointer when it has one, else the stick.
    const float smoothing = g_uIPointerSmoothing.Get();
    if ((analogMask & kPointerAnalogBits) == kPointerAnalogBits) {
        float px = pad->GetAnalog(kAnalogPointerX);
        float py = pad->GetAnalog(kAnalogPointerY);
        if (field_B5 & 0x80) {
            px = (px - field_D0[0]) * smoothing + field_D0[0];
            py = (py - field_D0[1]) * smoothing + field_D0[1];
        }
        field_D0[0] = px;
        field_D0[1] = py;
        field_B5 |= 0x80;
    } else {
        field_B5 &= 0x7F;
        if (pad->IsConnected() && pad->GetDeviceClass() != 1) {
            float x = pad->GetAnalog(kAnalog_LeftX);
            float y = pad->GetAnalog(kAnalog_LeftY);
            const float threshold = g_uIPointerThreshold.Get();
            if (std::fabs(x) < threshold)
                x = 0.0f;
            if (std::fabs(y) < threshold)
                y = 0.0f;
            if (g_uIPointerSquareInput.Get()) {
                x *= std::fabs(x);
                y *= std::fabs(y);
            }
            const float speed = g_uIPointerSpeed.Get();
            x *= speed;
            y *= agl::GetDisplaySettings().aspect * speed;
            field_D0[0] = std::clamp(field_D0[0] + x * dt * smoothing, -1.0f, 1.0f);
            field_D0[1] = std::clamp(field_D0[1] + y * dt * smoothing, -1.0f, 1.0f);
            field_B5 |= 0x80;
        }
    }
}

void PadMotionHistory::Record(const ape::Vec3& sample)
{
    samples[next] = sample;
    samples[next + kSamples] = sample;
    if (++next >= kSamples)
        next = 0;
    // A full ring drops its oldest sample.
    if (next <= oldest && ++oldest >= kSamples)
        oldest = 0;
}

// @func 0x0046426C PadMotionHistory::Collect
bool PadMotionHistory::Collect(float seconds, ape::Vec3* out, uint32_t* count) const
{
    // Samples are taken once per frame of the engine clock.
    const float frameTime = ape::GetFrameClock().deltaTime;
    const uint32_t wanted = std::min(std::max(static_cast<uint32_t>(seconds / frameTime), 1u), kSamples);
    const uint32_t available = oldest < next ? next - oldest : kSamples;
    const uint32_t n = std::min(wanted, available);
    uint32_t slot = next;
    for (uint32_t i = 0; i < n; ++i) {
        slot = slot == 0 ? kSamples - 1 : slot - 1;
        out[i] = samples[slot + kSamples];
    }
    *count = n;
    return n == wanted;
}

AppRuntime& GetAppRuntime()
{
    return g_appRuntime;
}

bool AppIsShuttingDown()
{
    return g_shuttingDown;
}

void SetAppShuttingDown(bool value)
{
    g_shuttingDown = value;
}

// @func 0x00458B1C AppExit
void AppExit()
{
    AppShutdown(g_app);
    ape::PostGameShutdownHook();
    crt::ExitImmediate(0);
}

// @func 0x004645A0 AppSetPauseRequest
void AppSetPauseRequest(bool requested, uint32_t value)
{
    g_appRuntime.pauseRequested = requested;
    g_appRuntime.pauseValue = value;
}

void SetSkipCheckEnabled(bool enabled)
{
    g_skipCheckEnabled = enabled;
}

// @func 0x0048BEC8 AnyPlayerRequestsSkip
bool AnyPlayerRequestsSkip()
{
    if (!g_skipCheckEnabled)
        return false;
    for (const AppPlayer& player : g_appRuntime.players) {
        if (player.inputFlags & player.inputMask & 0x40u)
            return true;
    }
    return false;
}

// @func 0x00464794 GuiPadFilterForPlayers
bool GuiPadFilterForPlayers(uint32_t pad, ape::GuiPadInput* input)
{
    if (pad >= uint32_t(kLocalPlayerCount))
        return false;
    const AppPlayer& player = GetAppRuntime().players[pad];
    input->buttons = player.inputFlags;
    input->edges = player.inputMask;
    // NOTE: the word at +0xC4 is handed over as the input flags; the port
    // calls it the analog mask.
    input->flags = player.analogMask;
    input->cursor[0] = player.field_D0[0];
    input->cursor[1] = player.field_D0[1];
    if (player.field_B5 & 0x80)
        input->flags |= ape::kGuiPadInput_Cursor;
    return true;
}

} // namespace sly
