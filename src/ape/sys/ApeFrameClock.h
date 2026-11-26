// openTIT - Minimal view of the engine's global frame clock.
//
// NOTE: on the console this is a lazily constructed static object at
// 0x011DE094 (about 0x200 bytes, with a 400 byte table at +0x5C) owned by
// code outside this module. Only the two fields used by the input and
// controller recording code are reproduced: the frame delta (+0x04) and the
// fixed time step override (+0x08). The owner of the full object should
// replace this header when it is reconstructed.
#pragma once

#include <cstdint>

namespace ape {

struct FrameClock {
    uint32_t frame = 0;       // +0x00 frame number (NOTE: advanced by the clock owner)
    float deltaTime = 0.0f;   // +0x04 seconds elapsed in the current frame
    float fixedStep = 0.0f;   // +0x08 forced frame time (0 = real time)
    float field_0C = 0.0f;    // +0x0C set from the engine configuration (0x010C5A34) at start-up
    // +0x14 / +0x18: called while long operations (file loading, chunk
    // parsing) run, so that the loading screen keeps updating.
    void (*progressCallback)(void* arg) = nullptr;
    void* progressArg = nullptr;
};

FrameClock& GetFrameClock();
// Runs the progress callback, if one is installed.
void RunFrameClockProgress();

} // namespace ape
