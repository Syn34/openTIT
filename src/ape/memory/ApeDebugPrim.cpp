// openTIT - ApeDebug primitive allocation (debug-draw queue entries).
#include "ape/memory/ApeDebugPrim.h"

#include <cstring>

namespace ape {
namespace ApeDebug {

// @func 0x005536F4 ApeDebug::AllocPrim
Prim* AllocPrim(int32_t type, const uint8_t color[4], uint8_t flag)
{
    Prim* prim = PrimAllocator::GetAllocator().AllocT();
    if (!prim)
        return nullptr;
    std::memcpy(prim->color, color, sizeof(prim->color));
    prim->type = type;
    prim->flag = flag;
    return prim;
}

// @func 0x00553868 ApeDebug::AddText3D
void AddText3D(float scale, const Vec3& position, const char* text, const uint8_t color[4])
{
    Prim* prim = AllocPrim(kPrim_Text3D, color, 1);
    if (!prim)
        return;
    std::strncpy(prim->text, text, sizeof(prim->text) - 1);
    prim->text[sizeof(prim->text) - 1] = '\0';
    prim->position[0] = position.x;
    prim->position[1] = position.y;
    prim->position[2] = position.z;
    prim->scale = scale;
}

} // namespace ApeDebug
} // namespace ape
