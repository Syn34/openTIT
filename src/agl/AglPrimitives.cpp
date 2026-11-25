// openTIT - Agl built-in geometry: vertex declarations and procedurally built meshes.
#include "agl/AglPrimitives.h"

#include "agl/AglResources.h"

#include "ape/math/ApeMath.h"

#include <cstring>

namespace agl {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.28318531f;

// position, color, texture coordinate (0x00F670E4 and 0x00F67114).
const AglVertexStream kPrimitiveStreams[] = {
    { 0, static_cast<uint8_t>(gpu::VertexAttribType::Float), 3, 0, 0, false },
    { 3, static_cast<uint8_t>(gpu::VertexAttribType::UByteNorm), 4, 12, 0, false },
    { 8, static_cast<uint8_t>(gpu::VertexAttribType::Float), 2, 16, 0, false },
    { 0, 0, 0, 0, 0, false },
};
constexpr uint32_t kPrimitiveStride = 24;

struct PrimitiveVertex {
    float position[3];
    uint8_t color[4];
    float uv[2];
};
static_assert(sizeof(PrimitiveVertex) == kPrimitiveStride, "primitive vertex layout");

AglVertexDecl g_primitiveDecls[2];   // 0x010F0CC0, 0x010F0D40
bool g_primitiveDeclsReady = false;  // 0x00F67144

} // namespace

void VertexDeclInit(AglVertexDecl* decl, const AglVertexStream* streams, uint32_t stride)
{
    decl->stride = stride;
    decl->flags |= 0x8000;
    decl->attributeMask = 0;
    int i = 0;
    for (; streams[i].size != 0 && i < AglVertexDecl::kMaxStreams - 1; ++i) {
        decl->streams[i] = streams[i];
        decl->attributeMask = static_cast<uint16_t>(
            decl->attributeMask | (1u << (streams[i].attribute + streams[i].attributeOffset)));
    }
    decl->streams[i] = streams[i];   // terminator
    decl->streams[i].size = 0;
}

// @func 0x0051B5C0 GetPrimitiveVertexDecl
const AglVertexDecl* GetPrimitiveVertexDecl(bool alternate)
{
    if (!g_primitiveDeclsReady) {
        g_primitiveDeclsReady = true;
        VertexDeclInit(&g_primitiveDecls[0], kPrimitiveStreams, kPrimitiveStride);
        VertexDeclInit(&g_primitiveDecls[1], kPrimitiveStreams, kPrimitiveStride);
    }
    return &g_primitiveDecls[alternate ? 1 : 0];
}

gpu::PrimitiveType PrimitiveFromIndex(int index)
{
    static const gpu::PrimitiveType kTable[6] = {
        gpu::PrimitiveType::Points, gpu::PrimitiveType::LineStrip, gpu::PrimitiveType::Lines,
        gpu::PrimitiveType::TriangleStrip, gpu::PrimitiveType::TriangleFan, gpu::PrimitiveType::Triangles,
    };
    return kTable[index];
}

namespace {

void SetOwner(AglSubMesh* mesh, AglMaterial* material)
{
    AglResource* previous = mesh->owner;
    if (previous == material)
        return;
    mesh->owner = material;
    if (material)
        material->AddRef();
    if (previous)
        previous->Release();
}

} // namespace

// @func 0x0052374C CreateSubMesh
AglSubMesh* CreateSubMesh(AglMaterial* material, int primitiveIndex, uint32_t vertexCount,
                          uint32_t indexCount, const AglVertexDecl* decl)
{
    AglSubMesh* mesh = AglSubMesh::Create();
    if (!mesh)
        return nullptr;
    const gpu::PrimitiveType primitive = PrimitiveFromIndex(primitiveIndex);
    mesh->vertexDecl = decl;
    if (vertexCount != 0) {
        mesh->AddVertexStream();
        MemBlockAlloc(&mesh->vertices, vertexCount * decl->stride, 0x80, gpu::MemoryLocation::Main, 2);
    }
    mesh->vertexCount = vertexCount;
    if (indexCount != 0) {
        mesh->field_3C = 1;   // 16-bit indices
        MemBlockAlloc(&mesh->indices, indexCount * 2, 0x80, gpu::MemoryLocation::Main, 2);
    }
    mesh->indexCount = indexCount;
    mesh->primitive = static_cast<uint32_t>(primitive);
    SetOwner(mesh, material);
    return mesh;
}

// @func 0x00515750 GetDebugVertexDecl
const AglVertexDecl* GetDebugVertexDecl(bool alternate)
{
    static AglVertexDecl decls[2];
    static bool ready = false;
    if (!ready) {
        ready = true;
        VertexDeclInit(&decls[0], kPrimitiveStreams, kPrimitiveStride);
        VertexDeclInit(&decls[1], kPrimitiveStreams, kPrimitiveStride);
    }
    return &decls[alternate ? 1 : 0];
}

// @func 0x00523AA8 GetDebugMaterial
AglMaterial* GetDebugMaterial(int textured)
{
    return GetBuiltinMaterial(textured == 1 ? 4 : 3);
}

// @func 0x00523AE0 CreateDebugMesh
AglDebugMesh* CreateDebugMesh(AglMaterial* material, int primitiveIndex, uint32_t vertexCount,
                              uint32_t indexCount)
{
    if (!material)
        material = GetBuiltinMaterial(3);
    AglSubMesh* mesh = CreateSubMesh(material, primitiveIndex, vertexCount, indexCount, GetDebugVertexDecl(false));
    AglDebugMesh* handle = new AglDebugMesh();
    handle->mesh = mesh;
    return handle;
}

// @func 0x005239E8 DestroyDebugMesh
void DestroyDebugMesh(AglDebugMesh* handle)
{
    if (handle->mesh) {
        handle->mesh->Release();
        handle->mesh = nullptr;
    }
    delete handle;
}

// @func 0x00523A5C LockDebugMeshVertices
void* LockDebugMeshVertices(AglDebugMesh* handle)
{
    // NOTE: the console also flags the block as CPU-written so it is
    // flushed before the GPU reads it.
    return handle->mesh->vertices.data;
}

// @func 0x00523A98 UnlockDebugMeshVertices
void UnlockDebugMeshVertices(AglDebugMesh*)
{
}

// @func 0x0051B788 CreateSphereMesh
AglSubMesh* CreateSphereMesh(AglMaterial* material, int rings, int segments)
{
    // Every ring is a strip of (segments + 1) vertex pairs.
    const uint32_t vertexCount = static_cast<uint32_t>((rings * segments + rings) * 2);
    AglSubMesh* mesh = AglSubMesh::Create();
    if (!mesh)
        return nullptr;
    mesh->name = "Sphere";
    const AglVertexDecl* decl = GetPrimitiveVertexDecl(false);
    mesh->vertexDecl = decl;
    if (vertexCount != 0) {
        mesh->AddVertexStream();
        MemBlockAlloc(&mesh->vertices, vertexCount * decl->stride, 0x80, gpu::MemoryLocation::Local, 2);
    }
    mesh->vertexCount = vertexCount;
    mesh->primitive = static_cast<uint32_t>(gpu::PrimitiveType::TriangleStrip);
    SetOwner(mesh, material);

    auto* out = static_cast<PrimitiveVertex*>(mesh->vertices.data);
    if (!out || rings <= 0)
        return mesh;
    const float ringStep = kPi / static_cast<float>(rings);
    const float segmentStep = kTwoPi / static_cast<float>(segments);
    for (int ring = 0; ring < rings; ++ring) {
        const float a0 = static_cast<float>(ring) * ringStep;
        const float a1 = (static_cast<float>(ring) + 1.0f) * ringStep;
        const float sin0 = ape::FastSin(a0), cos0 = ape::FastCos(a0);
        const float sin1 = ape::FastSin(a1), cos1 = ape::FastCos(a1);
        for (int seg = 0; seg <= segments; ++seg) {
            const float b = static_cast<float>(seg) * segmentStep;
            const float sb = ape::FastSin(b), cb = ape::FastCos(b);
            // NOTE: the texture coordinates are the x and y of the position.
            PrimitiveVertex& upper = *out++;
            upper = { { sin0 * sb, cos0, sin0 * cb }, { 0xFF, 0xFF, 0xFF, 0xFF }, { sin0 * sb, cos0 } };
            PrimitiveVertex& lower = *out++;
            lower = { { sin1 * sb, cos1, sin1 * cb }, { 0xFF, 0xFF, 0xFF, 0xFF }, { sin1 * sb, cos1 } };
        }
    }
    return mesh;
}

// @func 0x0051BC20 CreateIndexedSphereMesh
AglSubMesh* CreateIndexedSphereMesh(AglMaterial* material, int rings, int segments)
{
    // One vertex per cell centre; each band between two rings is a strip
    // of (segments + 1) index pairs, bands separated by a restart index.
    const uint32_t vertexCount = static_cast<uint32_t>(rings * segments);
    const uint32_t bands = rings > 0 ? static_cast<uint32_t>(rings - 1) : 0;
    const uint32_t indexCount = static_cast<uint32_t>(rings * segments * 2 - segments * 2 + (rings - 1) * 2 + rings - 2);
    AglSubMesh* mesh = AglSubMesh::Create();
    if (!mesh)
        return nullptr;
    mesh->name = "SphereIdx";
    const AglVertexDecl* decl = GetPrimitiveVertexDecl(true);
    mesh->vertexDecl = decl;
    if (vertexCount != 0) {
        mesh->AddVertexStream();
        MemBlockAlloc(&mesh->vertices, vertexCount * decl->stride, 0x80, gpu::MemoryLocation::Local, 2);
    }
    mesh->vertexCount = vertexCount;
    if (indexCount != 0) {
        mesh->field_3C = 1;   // 16-bit indices
        MemBlockAlloc(&mesh->indices, indexCount * 2, 0x80, gpu::MemoryLocation::Local, 2);
    }
    mesh->indexCount = indexCount;
    mesh->primitive = static_cast<uint32_t>(gpu::PrimitiveType::TriangleStrip);
    SetOwner(mesh, material);

    auto* out = static_cast<PrimitiveVertex*>(mesh->vertices.data);
    if (!out)
        return mesh;
    const float ringStep = kPi / static_cast<float>(rings);
    const float segmentStep = kTwoPi / static_cast<float>(segments);
    for (int ring = 0; ring < rings; ++ring) {
        const float a = static_cast<float>(ring) * ringStep + ringStep * 0.5f;
        const float sinA = ape::FastSin(a), cosA = ape::FastCos(a);
        for (int seg = 0; seg < segments; ++seg) {
            const float b = static_cast<float>(seg) * segmentStep + segmentStep * 0.5f;
            const float x = sinA * ape::FastSin(b);
            const float z = sinA * ape::FastCos(b);
            // NOTE: the texture coordinates are the x and y of the position.
            *out++ = { { x, cosA, z }, { 0xFF, 0xFF, 0xFF, 0xFF }, { x, cosA } };
        }
    }

    auto* index = static_cast<uint16_t*>(mesh->indices.data);
    for (uint32_t band = 0; band < bands; ++band) {
        if (band > 0)
            *index++ = 0xFFFF;   // strip restart
        const int first = static_cast<int>(band) * segments;
        for (int k = 0; k <= segments; ++k) {
            const int column = k % segments;
            *index++ = static_cast<uint16_t>(first + column);
            *index++ = static_cast<uint16_t>(first + column + segments);
        }
    }
    return mesh;
}

// @func 0x0051C1A8 CreateBoxMesh
AglSubMesh* CreateBoxMesh(AglMaterial* material, bool solid)
{
    // Corner i of the unit cube has x, y, z = -0.5 where bit 0, 1, 2 of i is set.
    static const uint8_t kEdges[24] = {   // 0x00F67145: twelve edges
        0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7,
    };
    static const uint8_t kFaces[36] = {   // 0x00F6715D: twelve triangles
        0, 1, 2, 1, 3, 2, 1, 0, 4, 1, 4, 5, 2, 6, 4, 2, 4, 0,
        3, 1, 5, 3, 5, 7, 7, 5, 4, 7, 4, 6, 3, 7, 6, 3, 6, 2,
    };
    const uint8_t* table = solid ? kFaces : kEdges;
    const uint32_t indexCount = solid ? 36 : 24;

    AglSubMesh* mesh = AglSubMesh::Create();
    if (!mesh)
        return nullptr;
    mesh->name = "Cube";
    const AglVertexDecl* decl = GetPrimitiveVertexDecl(true);
    mesh->vertexDecl = decl;
    mesh->AddVertexStream();
    MemBlockAlloc(&mesh->vertices, 8 * decl->stride, 0x80, gpu::MemoryLocation::Local, 2);
    mesh->vertexCount = 8;
    mesh->field_3C = 1;   // 16-bit indices
    MemBlockAlloc(&mesh->indices, indexCount * 2, 0x80, gpu::MemoryLocation::Local, 2);
    mesh->indexCount = indexCount;
    mesh->primitive = static_cast<uint32_t>(solid ? gpu::PrimitiveType::Triangles : gpu::PrimitiveType::Lines);
    SetOwner(mesh, material);

    auto* out = static_cast<PrimitiveVertex*>(mesh->vertices.data);
    if (!out)
        return mesh;
    for (int corner = 0; corner < 8; ++corner) {
        out[corner] = { { (corner & 1) ? -0.5f : 0.5f, (corner & 2) ? -0.5f : 0.5f, (corner & 4) ? -0.5f : 0.5f },
                        { 0xFF, 0xFF, 0xFF, 0xFF }, { 0.0f, 0.0f } };
    }
    auto* index = static_cast<uint16_t*>(mesh->indices.data);
    for (uint32_t i = 0; i < indexCount; ++i)
        index[i] = table[i];
    return mesh;
}

} // namespace agl
