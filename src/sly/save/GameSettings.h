// openTIT - The options block kept in the settings save (0x24 bytes).
//
// The game world embeds it at +0x1030; SaveSettings/LoadSettings copy it
// verbatim. The options menu restores one page at a time (audio, controls,
// display), the boot code builds a fresh block with Reset().
#pragma once

#include <cstdint>

namespace sly {

struct GameSettings {
    // flags bits. NOTE: named from the menu pages that restore them; which
    // option each bit stands for is not decoded.
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
    uint32_t field_08 = 0;             // +0x08
    float volumes[4] = {};             // +0x0C NOTE: +0x0C and +0x10 feed the music / effect volume globals
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
