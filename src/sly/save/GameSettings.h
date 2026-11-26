// openTIT - The options block kept in the settings save (0x24 bytes).
//
// The game world embeds it at +0x1030; SaveSettings/LoadSettings copy it
// verbatim. The options menu restores one page at a time (audio, controls,
// display), the boot code builds a fresh block with Reset().
#pragma once

#include <cstdint>

namespace sly {

struct GameSettings {
    // flags bits. NOTE: the display bits are not decoded.
    static constexpr uint32_t kFlag_InvertCameraX = 0x40000000u;   // bit 30
    static constexpr uint32_t kFlag_InvertCameraY = 0x20000000u;   // bit 29
    static constexpr uint32_t kFlag_Vibration = 0x10000000u;       // bit 28
    static constexpr uint32_t kControlFlagsMask = 0x70000000u;   // bits 28-30
    static constexpr uint32_t kControlDefaults = 0x10000000u;    // bit 28 on, 29/30 off
    static constexpr uint32_t kDisplayFlagsMask = 0x0F000000u;   // bits 24-27
    static constexpr uint32_t kDisplayDefaults = 0x0A000000u;    // bits 25 and 27 on

    // Audio page: every volume back to full (0x000ADC28).
    void ResetAudio();
    // Controls page (0x000ADC40).
    void ResetControls();
    // Display page: brightness to the middle, display flags and screen scale
    // to their defaults; drops the game world's preview object (0x000ADC5C).
    void ResetDisplay();
    // Fresh block: every page at its default and the system language
    // (0x000ADCF8).
    void Reset();

    uint32_t flags = 0;                // +0x00
    float brightness = 0.5f;           // +0x04
    // +0x08: the audio page's choices (indices into their option texts).
    uint8_t audioFormat = 0;           // +0x08 default, SS, SH, DPRII, 5.1, 7.1
    uint8_t centerSpeaker = 0;         // +0x09 default, small, large
    uint8_t dynamicRange = 0;          // +0x0A default, max, wide, normal, narrow, midnight
    uint8_t field_0B = 0;              // +0x0B
    // +0x0C: sound effects, music, voice, movie volumes (0..1).
    float volumes[4] = {};
    float screenScale = 1.0f;          // +0x1C NOTE: name guessed (reset with the display page)
    uint8_t language = 0;              // +0x20 game world +0x1050
};

// Save system start-up (0x000AE4A4): loads the save icon and marks saving
// available. The console also loads and mounts its system modules here.
void SaveSystemInit();
// Save system shut-down (0x000AE660): frees the icon.
void SaveSystemShutdown();
bool SaveSystemAvailable();
// Development path that wrote the progress block to a loose file
// ("save.dat"); compiled out, so it always reports kSave_Failed (0x000AE784).
int32_t SaveToLooseFile(const char* path, const void* data, uint32_t size);

} // namespace sly
