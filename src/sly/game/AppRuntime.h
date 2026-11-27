// openTIT - Run-time state of the application object that SlyApp.h does not
// model yet: the per-frame scratch stack, the two local players' pads and
// cursors, and a few flags written by the game shell.
//
// On the console these live inside the SlyApp global (0x010B19E0) at the
// offsets given in the comments; they are kept separately here so SlyApp.h
// (owned by another module) does not have to change.
#pragma once

#include <cstdint>

#include "ape/math/ApeMath.h"

namespace ape { class ApeMemStack; class ApeGuiObject; struct GuiPadInput; }
namespace ape::input { class ApePadDevice; }

namespace sly {

struct SlyApp;

constexpr int kLocalPlayerCount = 2;

// Recent motion sensor samples of a pad (one per frame, 360 kept). Each
// sample is written twice, 360 entries apart, so the newest `n` samples can
// be read back without wrapping.
struct PadMotionHistory {
    static constexpr uint32_t kSamples = 360;

    void Record(const ape::Vec3& sample);
    // Copies the samples of the last `seconds` (at least one frame, at most
    // the whole history), newest first, into `out` and reports how many were
    // written. False when fewer samples than asked for exist yet.
    bool Collect(float seconds, ape::Vec3* out, uint32_t* count) const;

    ape::Vec3 samples[kSamples * 2] = {};     // +0x00 (app player +0x50)
    uint32_t oldest = 0;                      // +0x2D00
    uint32_t next = 0;                        // +0x2D04 slot of the next sample
};

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
    void SetField_D0(const float value[2]);
    // Analog value `index` of the pad (0 without a pad, for an index the
    // player may not use, or past the last analog).
    float GetAnalog(uint32_t index) const;
    bool IsPadConnected() const;
    // Per-frame input update: copies the pad's buttons, maps the left stick
    // to the d-pad in menus (with key repeat), records the motion sensors
    // and moves the GUI pointer (from the pad's pointer, else the stick).
    void UpdateInput(float dt);

    uint8_t index = 0;                        // +0xB4 (stride 0x2EC0)
    // +0xB5 bit 7 (bit 23 of the word at +0xB4): the pointer position
    // (+0xD0) is valid for the GUI. NOTE: meaning inferred from the GUI pad
    // filter.
    uint8_t field_B5 = 0;
    // +0xB5 bits 0-6: key repeat stage of the stick d-pad (0..6).
    uint8_t repeatStage = 0;
    // +0xB6: low half of the word at +0xB4. Bit 15: the pause menu holds
    // the pad (set when a menu opens); the left stick then works as the
    // d-pad. Bit 14 selects the second delay of each key repeat stage.
    uint16_t padFlags = 0;
    uint32_t inputFlags = 0;                  // +0xB8 held buttons
    uint32_t inputMask = 0;                   // +0xBC buttons changed this frame
    uint32_t released = 0;                    // +0xC0
    uint32_t analogMask = 0xFFFFFFFFu;        // +0xC4 the pad's valid analog values
    float repeatTimer = 0.0f;                 // +0xC8
    ape::input::ApePadDevice* pad = nullptr;  // +0xCC
    float field_D0[2] = {};                   // +0xD0 GUI pointer position (-1..1)
    PadMotionHistory motion;                  // +0x100
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

// GUI pad filter of the game: the held and changed buttons and the pointer
// of local player `pad` (false for other pads).
bool GuiPadFilterForPlayers(uint32_t pad, ape::GuiPadInput* input);

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
