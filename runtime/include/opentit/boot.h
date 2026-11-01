#pragma once

namespace opentit {

// Boots the recompiled game: loads the user's EBOOT.elf into guest memory,
// binds HLE imports, creates the main PPU thread and calls the entry point.
int boot_main(int argc, char** argv);

}  // namespace opentit
