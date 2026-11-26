// openTIT - Run-time state of the application object that SlyApp.h does not
// model yet: the per-frame scratch stack, the two local players' pads and
// cursors, and a few flags written by the game shell.
//
// On the console these live inside the SlyApp global (0x010B19E0) at the
// offsets given in the comments; they are kept separately here so SlyApp.h
// (owned by another module) does not have to change.
#pragma once

#include <cstdint>

namespace ape { class ApeMemStack; class ApeGuiObject; }
namespace ape::input { class ApePadDevice; }

namespace sly {

struct SlyApp;

constexpr int kLocalPlayerCount = 2;

// A local player's input block (vtable 0x00B219C0, app +0xB0, stride
// 0x2EC0). Offsets below are relative to the app object.
struct AppPlayer {
    AppPlayer() = default;
    virtual ~AppPlayer();
    AppPlayer(const AppPlayer&) = delete;
    AppPlayer& operator=(const AppPlayer&) = delete;

    // Rumble on the player's pad (motor 0), if the pad allows rumble.
    void Rumble(float intensity, float duration);
    void StopRumble();
    void SetRumblePaused(bool paused);
    // NOTE: stores two words at +0xD0/+0xD4 (meaning unknown; the app
    // constructor initialises them from a global pair).
    void SetField_D0(const uint32_t value[2]);
    // Analog value `index` of the pad (0 without a pad, for an index the
    // player may not use, or past the last analog).
    float GetAnalog(uint32_t index) const;
    bool IsPadConnected() const;

    uint8_t index = 0;                        // +0xB4 (stride 0x2EC0)
    uint32_t inputFlags = 0;                  // +0xB8
    uint32_t inputMask = 0;                   // +0xBC
    uint32_t analogMask = 0xFFFFFFFFu;        // +0xC4 analog values the player may use
    ape::input::ApePadDevice* pad = nullptr;  // +0xCC
    uint32_t field_D0[2] = {};                // +0xD0
};

struct AppRuntime {
    ape::ApeMemStack* frameStack = nullptr;   // +0xA0
    float frameTime = 1.0f / 60.0f;           // +0xA4
    AppPlayer players[kLocalPlayerCount];
    ape::ApeGuiObject* cursors[kLocalPlayerCount] = {}; // +0x5E30
    uint32_t pauseValue = 0;                  // +0x5E38
    bool pauseRequested = false;              // +0x5E3C
};

AppRuntime& GetAppRuntime();

// Set when shutdown starts (0x010C5A45).
bool AppIsShuttingDown();
void SetAppShuttingDown(bool value);

// Shuts the application down and terminates the process.
[[noreturn]] void AppExit();

// Records a pause request from the system layer.
void AppSetPauseRequest(bool requested, uint32_t value);

// True if any local player holds the "skip" input (bit 6 of both the input
// flags and mask). Only checked while the gate flag (0x00F65C84) is set.
bool AnyPlayerRequestsSkip();
void SetSkipCheckEnabled(bool enabled);

} // namespace sly
