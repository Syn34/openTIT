// openTIT - Render node table shared by the world objects and the renderer.
//
// Every renderable world object (geometry, light, emitter and camera
// instances) owns one entry of a dense table; the object stores the entry
// index (ApeObj::m_renderNode) and the entry points back at its owner. A
// second array, parallel to the nodes, holds each node's world bounding
// sphere for culling. The renderer fills the rest of a node; only the parts
// the world code touches are named here.
#pragma once

#include "ape/math/ApeBounds.h"

#include <cstdint>

namespace ape::world {

class ApeObj;

struct alignas(16) ApeRenderNode {
    uint8_t renderData[0x80];   // NOTE: renderer owned
    Vec3* field_80;             // NOTE: cleared by ClearRenderNodeRefs; non-null means field_84 is owned
    void* field_84;             // NOTE: freed through the table's free hook
    void* field_88;             // NOTE: freed through the table's free hook; +0x40 is a pointer
    uint32_t field_8C;
    struct ApeVisRecord* visRecord; // 0x90 draw record of geometry/emitter instances (ApeVisTable.h)
    ApeObj* owner;              // 0x94
    uint32_t field_98;
    uint32_t field_9C;
};

struct alignas(16) ApeRenderNodeBounds {
    Sphere sphere;              // world bounding sphere of the owner
    Vec4 field_10;              // NOTE: renderer owned
};

// The table (pointer at 0x01194598 on the console).
struct ApeRenderNodeTable {
    ApeRenderNode* nodes = nullptr;
    ApeRenderNodeBounds* bounds = nullptr;
    uint16_t count = 0;
    uint16_t capacity = 0;
    // Frees the per-node blocks (console: virtual slot 4 of the table's
    // allocator). Null leaves them alone.
    void (*freeBlock)(void* block) = nullptr;
};

// The active table; null until the renderer creates one.
extern ApeRenderNodeTable* g_renderNodeTable;

// Node owned by `obj`, or null.
ApeRenderNode* GetRenderNodeOf(const ApeObj& obj);
// Bounds entry that belongs to a node.
ApeRenderNodeBounds* GetRenderNodeBounds(const ApeRenderNode* node);
// Removes a node from the table, compacting the table and renumbering the
// owners of the nodes that moved.
void ReleaseRenderNode(uint16_t index);
// Clears the vectors / pointers a node borrows from its owner.
void ClearRenderNodeRefs(ApeRenderNode* node);

} // namespace ape::world
