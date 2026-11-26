// openTIT - Pooled, reference counted Agl resource objects (meshes, materials, targets).
#include "agl/AglResources.h"

#include "agl/AglTexture.h"
#include "agl/AglPrimitives.h"
#include "agl/AglShader.h"
#include "agl/AglShaderClass.h"
#include "agl/AglState.h"

#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeTFreeList.h"

#include <mutex>

namespace agl {

// @func 0x004FCD78 AglSubMesh::LimitBoneInfluences
void AglSubMesh::LimitBoneInfluences(int maxInfluences)
{
    if (maxInfluences == 4 || !skinWeights)
        return;
    for (uint32_t v = 0; v < vertexCount; ++v) {
        uint8_t* bones = skinWeights + v * 8;
        uint8_t* weights = bones + 4;
        // Influences past the limit lose both their bone and their weight.
        for (int i = maxInfluences < 1 ? 1 : maxInfluences; i < 4; ++i) {
            bones[i] = 0;
            weights[i] = 0;
        }
        // Tiny secondary weights are dropped; the first weight always stays.
        uint32_t w[4] = { weights[0], weights[1], weights[2], weights[3] };
        for (int i = 1; i < 4; ++i)
            if (w[i] < 4)
                w[i] = 0;
        const uint32_t total = w[0] + w[1] + w[2] + w[3];
        uint8_t sum = 0;
        for (int i = 1; i < 4; ++i) {
            weights[i] = static_cast<uint8_t>(total ? (w[i] * 255) / total : 0);
            sum = static_cast<uint8_t>(sum + weights[i]);
        }
        // The first weight takes whatever is left so that the four add to 255.
        weights[0] = static_cast<uint8_t>(255 - sum);
    }
}

namespace {

std::mutex g_resourceLock;   // 0x010DDAF4 (shared with textures)
std::mutex g_topologyLock;   // 0x010DDC80

template <class T>
T* PoolNew()
{
    return ape::ApeTListAllocator<T>::GetAllocator().New();
}

template <class T>
void PoolDelete(void* object, std::mutex& lock)
{
    std::lock_guard<std::mutex> guard(lock);
    ape::ApeTListAllocator<T>::GetAllocator().Delete(static_cast<T*>(object));
}

} // namespace

// --- AglSubMesh ---

namespace {
uint32_t g_nextSubMeshId = 0;       // shared resource id counter on the console (0x01164CD0)
uint32_t g_nextSubMeshSerial = 0;   // 0x010DD580
}

// @func 0x005386DC AglSubMesh::AglSubMesh (platform class; platformData starts empty)
// @func 0x004FCE88 AglSubMesh::AglSubMesh
// @func 0x004FCFE8 AglSubMesh::AglSubMesh (base object copy)
AglSubMesh::AglSubMesh()
{
    refCount = 1;
    resourceFlags = 0x0800;
    id = ++g_nextSubMeshId;
    serial = g_nextSubMeshSerial++;
}

// @func 0x004FD008 AglSubMesh::~AglSubMesh
// @func 0x004FD198 AglSubMesh::~AglSubMesh (base object copy)
// @func 0x004FD150 AglSubMesh::~AglSubMesh (deleting)
AglSubMesh::~AglSubMesh()
{
    MemBlockFree(&vertices);
    vertices = AglMemBlock();
    MemBlockFree(&indices);
    if (skinWeights) {
        ape::MemFree(skinWeights);
        skinWeights = nullptr;
    }
    if (skinMatrices) {
        ape::MemFree(skinMatrices);
        skinMatrices = nullptr;
    }
    if (owner)
        owner->Release();
}

namespace {

// First vertex constant register of the bone matrices.
constexpr uint32_t kBoneRegister = 0x40;
// Bones a mesh can use (the console's stack buffer holds 64).
constexpr uint32_t kMaxMeshBones = 64;

// Set when meshes with GPU side buffers draw through their platform path
// (0x00F674FC). NOTE: the console sets it outside this module.
bool g_usePlatformMeshPath = false;

// TODO(0x005382C0 AglSubMesh::DrawPlatform) draw of meshes whose vertices
// are skinned into the GPU side buffers (platformData).
void DrawPlatformMesh(const AglSubMesh& /*mesh*/, const AglRenderItem* /*item*/)
{
}

} // namespace

// @func 0x00538B44 AglSubMesh::Draw
void AglSubMesh::Draw(const AglRenderItem* item)
{
    gpu::CommandList& cmd = Cmd();

    // Skinned meshes reference bones of the item's palette through their
    // bone table (a count followed by the palette index of every bone).
    if (const auto* boneTable = static_cast<const uint8_t*>(skinMatrices)) {
        const uint32_t count = boneTable[0] < kMaxMeshBones ? boneTable[0] : kMaxMeshBones;
        float bones[kMaxMeshBones * 12];
        for (uint32_t i = 0; i < count; ++i) {
            const float* bone = item->bonePalette + size_t(boneTable[i + 1]) * 12;
            for (int k = 0; k < 12; ++k)
                bones[i * 12 + k] = bone[k];
        }
        cmd.SetVertexProgramConstants(kBoneRegister, count * 12, bones);
    }

    if (platformData && g_usePlatformMeshPath) {
        DrawPlatformMesh(*this, item);
        return;
    }

    BindVertexArrays(vertexDecl, vertices.Ptr());

    CommitFragmentProgram();
    const auto prim = static_cast<gpu::PrimitiveType>(primitive);
    if (indexCount == 0)
        cmd.DrawArrays(prim, 0, vertexCount);
    else
        cmd.DrawIndexArray(prim, indexCount, static_cast<gpu::IndexType>(field_3C), indices.Ptr());
}

AglSubMesh* AglSubMesh::Create()
{
    return PoolNew<AglSubMesh>();
}

// @func 0x00525CD8 AglSubMesh::Destroy
void AglSubMesh::Destroy(void* object)
{
    PoolDelete<AglSubMesh>(object, g_resourceLock);
}

// @func 0x00508484 AglSubMesh::Release
void AglSubMesh::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglSubMesh::Destroy);
}

// @func 0x00539874 AglSubMesh::GetMemoryUnits
uint32_t AglSubMesh::GetMemoryUnits() const
{
    // NOTE: the console sums one block per vertex stream; this model keeps
    // a single vertex block.
    const uint32_t vertexUnits = vertexStreamCount ? vertices.size >> 4 : 0u;
    return vertexUnits + (indices.size >> 4);
}

// --- AglTopology ---

// @func 0x0050A14C AglTopology::SetName
void AglTopology::SetName(const char* topologyName)
{
    name = topologyName;
    for (uint16_t i = 0; i < meshCount; ++i)
        meshes[i].mesh->name = name;
}

// @func 0x0050A19C AglTopology::QueryMemoryUnits
void AglTopology::QueryMemoryUnits() const
{
    for (uint16_t i = 0; i < meshCount; ++i)
        (void)meshes[i].mesh->GetMemoryUnits();
}

// @func 0x0050A214 AglTopology::SetFlags
void AglTopology::SetFlags(bool flag80, bool flag40, bool flag20)
{
    flags = static_cast<uint8_t>((flags & 0x1F) | (flag80 ? 0x80 : 0) | (flag40 ? 0x40 : 0) | (flag20 ? 0x20 : 0));
}

// @func 0x0050A268 AglTopology::GetLodScale
float AglTopology::GetLodScale() const
{
    return lod ? lod[4] : 0.0f;
}

AglTopology* AglTopology::Create()
{
    return PoolNew<AglTopology>();
}

// @func 0x005261A0 AglTopology::Destroy
void AglTopology::Destroy(void* object)
{
    PoolDelete<AglTopology>(object, g_topologyLock);
}

// @func 0x00508B54 AglTopology::Release
void AglTopology::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglTopology::Destroy);
}

// --- AglMaterial ---

// @func 0x0051D058 AglMaterial::Destroy
void AglMaterial::Destroy(void* object)
{
    PoolDelete<AglMaterial>(object, g_resourceLock);
}

// @func 0x0050CAA8 AglMaterial::Release
void AglMaterial::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglMaterial::Destroy);
}

// --- AglRenderSurface ---

namespace {
uint32_t g_nextResourceId = 0;   // shared with textures on the console (0x01164CD0)
}

// @func 0x0051AB74 AglRenderSurface::AglRenderSurface
AglRenderSurface::AglRenderSurface()
{
    refCount = 1;
    resourceFlags = 0x1800;
    id = ++g_nextResourceId;
}

// @func 0x0051AD90 AglRenderSurface::~AglRenderSurface
// @func 0x0051AC50 AglRenderSurface::~AglRenderSurface (copy 2)
// @func 0x0051ACE4 AglRenderSurface::~AglRenderSurface (deleting)
// @func 0x0053C870 AglRenderSurface::~AglRenderSurface (platform class)
// @func 0x0053C8A0 AglRenderSurface::~AglRenderSurface (platform class, deleting)
AglRenderSurface::~AglRenderSurface()
{
    // NOTE: the console releases the surfaces of texture wrappers too. Here
    // those surfaces belong to the texture (or to the caller, for shared
    // depth buffers), so only render surfaces that own their surfaces free them.
    if (flags & kFlag_Texture)
        return;
    if (color)
        SurfaceRelease(&color);
    if (color2)
        SurfaceRelease(&color2);
    if (depth)
        SurfaceRelease(&depth);
}

// @func 0x0051AE24 AglRenderSurface::GetDefaultRects
void AglRenderSurface::GetDefaultRects(const uint16_t** viewport, const uint16_t** scissor) const
{
    static uint16_t fullViewport[4];   // 0x010F0CA8
    static uint16_t fullScissor[4];    // 0x010F0CB8
    const uint16_t* vp = *viewport;
    if (!vp) {
        fullViewport[0] = 0;
        fullViewport[1] = 0;
        fullViewport[2] = static_cast<uint16_t>(width);
        fullViewport[3] = static_cast<uint16_t>(height);
        vp = fullViewport;
        *viewport = vp;
    }
    if (!*scissor) {
        for (int i = 0; i < 4; ++i)
            fullScissor[i] = vp[i];
        *scissor = fullScissor;
    }
}

// @func 0x0051AF28 AglRenderSurface::Create
AglRenderSurface* AglRenderSurface::Create()
{
    return PoolNew<AglRenderSurface>();
}

// @func 0x0051B4C4 AglRenderSurface::CreateForTexture
AglRenderSurface* AglRenderSurface::CreateForTexture(AglSurface* color, AglSurface* depth, AglTexture* texture)
{
    AglRenderSurface* rs = Create();
    rs->texture = texture;
    rs->flags = (rs->flags & 0x01FFFFFF) | kFlag_Texture | (color ? kFlag_HasColor : 0u) | (depth ? kFlag_HasDepth : 0u);
    rs->width = texture->width;
    rs->height = texture->height;
    if (depth)
        rs->depth = depth;
    if (color)
        rs->color = color;
    // Clear color only, or color, depth and stencil.
    rs->clearMask = depth ? 0xF3 : 0xF0;
    return rs;
}

// @func 0x0051B0E8 AglRenderSurface::Destroy
void AglRenderSurface::Destroy(void* object)
{
    PoolDelete<AglRenderSurface>(object, g_resourceLock);
}

// @func 0x0051AEE8 AglRenderSurface::Release
void AglRenderSurface::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglRenderSurface::Destroy);
}

} // namespace agl
