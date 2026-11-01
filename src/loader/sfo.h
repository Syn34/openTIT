// PARAM.SFO reader (little-endian key/value table on the disc).
#pragma once

#include <map>
#include <optional>
#include <string>
#include <variant>

#include "common/types.h"

namespace opentit::loader {

using SfoValue = std::variant<std::string, u32>;

std::optional<std::map<std::string, SfoValue>> read_sfo(const std::string& path, std::string* error);

}  // namespace opentit::loader
