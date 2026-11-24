// openTIT - Pooled, reference counted Agl resource objects (meshes, materials, targets).
#include "agl/AglResources.h"

#include "agl/AglTexture.h"

#include "ape/memory/ApeTFreeList.h"

#include <mutex>

namespace agl {

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

// --- AglTopology ---

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

AglMaterial* AglMaterial::Create()
{
    return PoolNew<AglMaterial>();
}

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

// --- AglRenderTarget ---

AglRenderTarget* AglRenderTarget::Create()
{
    return PoolNew<AglRenderTarget>();
}

// @func 0x00522E70 AglRenderTarget::Destroy
void AglRenderTarget::Destroy(void* object)
{
    PoolDelete<AglRenderTarget>(object, g_resourceLock);
}

// @func 0x005216FC AglRenderTarget::Release
void AglRenderTarget::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglRenderTarget::Destroy);
}

} // namespace agl
