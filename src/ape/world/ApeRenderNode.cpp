// openTIT - Render node table shared by the world objects and the renderer.
#include "ape/world/ApeRenderNode.h"

#include "ape/world/ApeObj.h"

#include <cstring>

namespace ape::world {

ApeRenderNodeTable* g_renderNodeTable = nullptr;

ApeRenderNode* GetRenderNodeOf(const ApeObj& obj)
{
    if (obj.GetRenderNodeIndex() == kNoRenderNode || !g_renderNodeTable)
        return nullptr;
    return &g_renderNodeTable->nodes[obj.GetRenderNodeIndex()];
}

// @func 0x0057BE98 GetRenderNodeBounds
ApeRenderNodeBounds* GetRenderNodeBounds(const ApeRenderNode* node)
{
    ApeRenderNodeTable& table = *g_renderNodeTable;
    return &table.bounds[node - table.nodes];
}

void ReleaseRenderNode(uint16_t index)
{
    ApeRenderNodeTable& table = *g_renderNodeTable;
    ApeRenderNode& node = table.nodes[index];
    node.owner->SetRenderNodeIndex(kNoRenderNode);

    if (node.field_80) {
        node.field_80 = nullptr;
        if (node.field_84 && table.freeBlock)
            table.freeBlock(node.field_84);
    }
    node.field_84 = nullptr;
    if (node.field_88 && table.freeBlock)
        table.freeBlock(node.field_88);

    const uint16_t last = uint16_t(table.count - 1);
    if (index == last) {
        std::memset(&node, 0, sizeof(node));
        std::memset(&table.bounds[index], 0, sizeof(table.bounds[index]));
    } else {
        const size_t moved = size_t(last - index);
        std::memmove(&table.nodes[index], &table.nodes[index + 1], moved * sizeof(ApeRenderNode));
        std::memmove(&table.bounds[index], &table.bounds[index + 1], moved * sizeof(ApeRenderNodeBounds));
        for (uint16_t i = index; i < last; ++i)
            table.nodes[i].owner->SetRenderNodeIndex(i);
    }
    --table.count;
}

// @func 0x0057CDB8 ClearRenderNodeRefs
void ClearRenderNodeRefs(ApeRenderNode* node)
{
    if (node->field_80)
        *node->field_80 = Vec3{0.0f, 0.0f, 0.0f, node->field_80->pad};
    if (Vec3* v = static_cast<Vec3*>(node->field_84))
        *v = Vec3{0.0f, 0.0f, 0.0f, v->pad};
    if (node->field_88)
        *reinterpret_cast<void**>(static_cast<uint8_t*>(node->field_88) + 0x40) = nullptr;
}

} // namespace ape::world
