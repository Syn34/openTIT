// Minimal SHA-1, used only to compute PS3 function NIDs from export names.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace opentit {

std::array<std::uint8_t, 20> sha1(const void* data, std::size_t size);

}  // namespace opentit
