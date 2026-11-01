// PS3 function NIDs: the first four bytes (little endian) of
// SHA-1(name || 0x6759659904250490566427499489741A).
#pragma once

#include <string>
#include <string_view>

#include "common/types.h"

namespace opentit::loader {

u32 compute_nid(std::string_view name);

// Returns the known export name for a NID, or nullptr when unidentified.
const char* nid_name(u32 nid);

// "name" if known, otherwise "library_XXXXXXXX".
std::string nid_display_name(std::string_view library, u32 nid);

}  // namespace opentit::loader
