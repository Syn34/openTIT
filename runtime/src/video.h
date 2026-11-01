// Game window: shows the display buffer the game flips to.
#pragma once

#include <string>

#include "common/types.h"

namespace opentit::video {

// Open the window (no-op if built without SDL2 or running headless).
bool start(const std::string& title, int scale);
bool active();
// Copy a frame (guest A8R8G8B8 / X8R8G8B8 rows) for the window to show.
void present(const u8* argb, u32 pitch, u32 width, u32 height);
u64 frames_presented();

}  // namespace opentit::video
