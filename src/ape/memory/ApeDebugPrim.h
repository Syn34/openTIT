// openTIT - ApeDebug primitive allocation (debug-draw queue entries).
//
// Debug primitives are pooled in ApeTListAllocator<ApeDebug::Prim>; the
// debug renderer walks the pool's live list to draw them.
//
// NOTE: only the allocation helpers live in this address range; the renderer
// and the other primitive kinds are elsewhere (see NOTES.md).
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTFreeList.h"

#include <cstdint>

namespace ape {
namespace ApeDebug {

enum PrimType : int32_t {
    kPrim_Text3D = 3,   // NOTE: other values are not identified yet
};

struct Prim {
    int32_t type;
    uint8_t color[4];   // r, g, b, a
    uint8_t flag;       // NOTE: set to 1 for 3D text; meaning unknown (depth test?)
    float position[3];
    float scale;
    char text[0x60];    // NUL-terminated, at most 0x5F characters
};
static_assert(sizeof(Prim) == 0x7C, "ApeDebug::Prim layout");

// Takes a primitive from the shared pool and fills in the common header.
Prim* AllocPrim(int32_t type, const uint8_t color[4], uint8_t flag);
// Queues a text label at a world position.
void AddText3D(float scale, const Vec3& position, const char* text, const uint8_t color[4]);

using PrimAllocator = ApeTListAllocator<Prim>;

} // namespace ApeDebug
} // namespace ape
