// openTIT - ApeDebug primitive allocation (debug-draw queue entries).
#include "ape/memory/ApeDebugPrim.h"

#include <cstring>

namespace ape {
namespace ApeDebug {

PrimList& PrimPool::Get()
{
    if (!list) {
        list = new PrimList();
        list->SetName("ApeTListAllocator<T>::GetAllocator()");
        list->Init(initialCount);
    }
    return *list;
}

PrimPool& CurrentPool();

// @func 0x005536F4 ApeDebug::AllocPrim
Prim* AllocPrim(int32_t type, const uint8_t color[4], uint8_t flag)
{
    Prim* prim = CurrentPool().Get().AllocT();
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
    Prim* prim = AllocPrim(kPrim_Text, color, 1);
    if (!prim)
        return;
    std::strncpy(prim->text.text, text, sizeof(prim->text.text) - 1);
    prim->text.text[sizeof(prim->text.text) - 1] = '\0';
    prim->text.position[0] = position.x;
    prim->text.position[1] = position.y;
    prim->text.position[2] = position.z;
    prim->text.scale = scale;
}

} // namespace ApeDebug
} // namespace ape
