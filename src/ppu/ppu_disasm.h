#pragma once

#include <string>

#include "common/types.h"

namespace opentit::ppu {

// Disassemble one instruction located at `pc` (used to resolve branch targets).
std::string disasm(u32 raw, u32 pc);

}  // namespace opentit::ppu
