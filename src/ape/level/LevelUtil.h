// openTIT - small helpers shared by the level / setting / GUI loaders.
#pragma once

#include <cctype>

namespace ape {
namespace level_detail {

// Case-insensitive ASCII string equality; null never matches.
// NOTE: stands in for the engine's stricmp wrapper (0x00555D70), which is not
// part of ApeString yet (see NOTES.md).
inline bool EqualsNoCase(const char* a, const char* b)
{
    if (!a || !b)
        return false;
    for (;; ++a, ++b) {
        const int ca = std::tolower((unsigned char)*a);
        const int cb = std::tolower((unsigned char)*b);
        if (ca != cb)
            return false;
        if (ca == 0)
            return true;
    }
}

} // namespace level_detail
} // namespace ape
