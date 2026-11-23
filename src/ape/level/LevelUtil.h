// openTIT - small helpers shared by the level / setting / GUI loaders.
#pragma once

#include "ape/core/ApeString.h"

namespace ape {
namespace level_detail {

// Case-insensitive equality on top of the engine's stricmp wrapper; null never matches.
inline bool EqualsNoCase(const char* a, const char* b)
{
    return a && b && StrICmp(a, b) == 0;
}

} // namespace level_detail
} // namespace ape
