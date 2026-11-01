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

// Write a PARAM.SFO. `max_len` gives a key's reserved size (strings default
// to their length + 1, rounded up to 4).
bool write_sfo(const std::string& path, const std::map<std::string, SfoValue>& values,
               const std::map<std::string, u32>& max_len = {});

}  // namespace opentit::loader
