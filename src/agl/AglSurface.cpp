// openTIT - Agl GPU memory blocks, image formats, render surfaces and texture images.
#include "agl/AglTypes.h"

#include "ape/memory/ApeMem.h"

#include <cstring>

namespace agl {

namespace {

// Memory classes of the system allocator that back the two GPU memory
// locations: class 4 is video ("local") memory, class 5 IO-mapped main memory.
constexpr int kMemClassLocal = ape::kMemClass_HeapA;
constexpr int kMemClassMain  = ape::kMemClass_HeapB;

int MemClassFor(gpu::MemoryLocation location)
{
    return location == gpu::MemoryLocation::Main ? kMemClassMain : kMemClassLocal;
}

// 0x00F67258
const AglFormatInfo kFormats[kAglFmt_Count] = {
    { 0x9E,  32, "X8R8G8B8",     0xAAE4, 0, 1 },
    { 0xBE,  32, "LIN_X8R8G8B8", 0xAAE4, 5, 2 },
    { 0x85,  32, "A8R8G8B8",     0xAAE4, 0, 3 },
    { 0xA5,  32, "LIN_A8R8G8B8", 0xAAE4, 8, 4 },
    { 0xB2,  16, "SHADOW_Z16",   0xAAE4, 1, 5 },
    { 0xB0,  32, "SHADOW_Z24S8", 0xAAE4, 2, 6 },
    { 0xAB,  16, "Z16",          0xAAD8, 1, 7 },
    { 0xA5,  32, "D24S8",        0xAA93, 2, 8 },
    { 0xA6,   4, "LIN_DXT1",     0xAAE4, 0, 9 },
    { 0xA7,   8, "LIN_DXT3",     0xAAE4, 0, 10 },
    { 0xA8,   8, "LIN_DXT5",     0xAAE4, 0, 11 },
    { 0x86,   4, "DXT1",         0xAAE4, 0, 12 },
    { 0x87,   8, "DXT3",         0xAAE4, 0, 13 },
    { 0x88,   8, "DXT5",         0xAAE4, 0, 14 },
    { 0x8B,  16, "G8B8",         0xAAE4, 0, 15 },
    { 0xAB,  16, "LIN_G8B8",     0xAAE4, 0, 16 },
    { 0x8B,  16, "R8G8",         0xAAD8, 0, 17 },
    { 0xAB,  16, "LIN_R8G8",     0xAAD8, 0, 18 },
    { 0xA1,   8, "INDEXED_8",    0xAAE4, 0, 0 },
};

// 0x00F6741C: no AA (color), no AA (depth/16-bit), 2x, 4x.
const AglAntialiasInfo kAANone      = { kAglAA_None, 1, 1, 1, 0, 0, 0xA };
const AglAntialiasInfo kAANoneDepth = { kAglAA_None, 1, 1, 1, 0, 0, 0 };
const AglAntialiasInfo kAA2x        = { kAglAA_2x,   2, 2, 1, 3, 7, 0xB };
const AglAntialiasInfo kAA4x        = { kAglAA_4x,   4, 2, 2, 4, 8, 0xA };

// Console memory tile and z-cull regions. On PC they only exist as
// bookkeeping so surfaces keep the same identities.
constexpr int kMaxTiles = 14;
constexpr uint32_t kZcullMemory = 0x300000;   // pixels covered by all z-cull regions

uint16_t g_tilesUsed = 0;          // 0x00F67244
int32_t g_nextCompressionTag = 0;  // 0x00F67248
int32_t g_nextZcullIndex = 0;      // 0x00F6724C
uint32_t g_zcullUsed = 0;          // 0x00F67250

struct TileInfo {
    gpu::MemoryLocation location;
    uint32_t offset;
    uint32_t size;
    uint32_t pitch;
    uint32_t compression;
    uint32_t compressionTag;
    int32_t bank;
};
TileInfo g_tileInfo[kMaxTiles];    // 0x010F1190

int AllocTile()
{
    for (int i = 0; i < kMaxTiles; ++i) {
        if (!(g_tilesUsed & (1u << i))) {
            g_tilesUsed = static_cast<uint16_t>(g_tilesUsed | (1u << i));
            return i;
        }
    }
    return -1;
}

AglSurface* NewSurface()
{
    AglSurface* surface = new AglSurface();
    SurfaceInit(surface);
    return surface;
}

} // namespace

// --- GPU memory --------------------------------------------------------------

// @func 0x0052F198 AglMemBlock::Alloc
void MemBlockAlloc(AglMemBlock* block, uint32_t size, uint32_t align, gpu::MemoryLocation location,
                   uint8_t category)
{
    block->size = size;
    block->category = category;
    block->borrowed = false;
    block->location = location;
    block->data = ape::SysAlloc(align, size, MemClassFor(location), category);
}

// @func 0x0052EFE0 AglMemBlock::Free
void MemBlockFree(AglMemBlock* block)
{
    if (!block->data || block->borrowed)
        return;
    ape::SysFree(block->data, MemClassFor(block->location), block->category);
    block->data = nullptr;
}

void MemBlockMakeView(AglMemBlock* block, const AglMemBlock& source)
{
    *block = source;
    block->borrowed = true;
}

// --- formats -------------------------------------------------------------------

const AglFormatInfo& GetFormatInfo(uint32_t format)
{
    return kFormats[format < kAglFmt_Count ? format : kAglFmt_A8R8G8B8];
}

bool IsDepthFormat(uint32_t format)
{
    return format == kAglFmt_ShadowZ16 || format == kAglFmt_ShadowZ24S8 || format == kAglFmt_Z16 ||
           format == kAglFmt_D24S8;
}

const AglAntialiasInfo* GetAntialiasInfo(uint32_t antialias, uint32_t format)
{
    switch (antialias) {
    case kAglAA_None:
        // 16-bit depth surfaces use the uncompressed variant.
        return (format == kAglFmt_Z16 || format == kAglFmt_ShadowZ16) ? &kAANoneDepth : &kAANone;
    case kAglAA_2x:
        return &kAA2x;
    case kAglAA_4x:
        return &kAA4x;
    default:
        return nullptr;
    }
}

// --- render surfaces -------------------------------------------------------------

// @func 0x0053044C AglSurface::Init
void SurfaceInit(AglSurface* surface)
{
    *surface = AglSurface();
    surface->mem.location = gpu::MemoryLocation::Main;
}

// @func 0x00530318 AglSurface::ComputeSize
uint32_t SurfaceComputeSize(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias,
                            bool tiled, uint32_t* outPitch)
{
    const AglAntialiasInfo* aa = GetAntialiasInfo(antialias, format);
    const uint32_t xScale = aa ? aa->xScale : 1;
    const uint32_t yScale = aa ? aa->yScale : 1;
    const uint32_t bytesPerPixel = (GetFormatInfo(format).bitsPerPixel + 7) / 8;

    // Rows are padded to a multiple of 256 bytes...
    const uint32_t pixelsPer256 = 256 / bytesPerPixel;
    uint32_t pitch = ((width + pixelsPer256 - 1) & ~(pixelsPer256 - 1)) * bytesPerPixel * xScale;
    // ...and tiled surfaces to a pitch the tiling hardware supports.
    if (tiled) {
        static const uint32_t kTiledPitches[] = {
            0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0xA00, 0xC00, 0xD00, 0xE00, 0x1000,
            0x1400, 0x1800, 0x1A00, 0x1C00, 0x2000, 0x2800, 0x3000, 0x3400, 0x3800, 0x4000,
            0x5000, 0x6000, 0x6800, 0x7000, 0x8000, 0xA000, 0xC000, 0xD000, 0xE000, 0x10000,
        };
        for (uint32_t p : kTiledPitches) {
            if (p >= pitch) {
                pitch = p;
                break;
            }
        }
    }
    *outPitch = pitch;
    // Height is padded to 32 rows; the size is rounded up to 64 KB.
    const uint64_t bytes = uint64_t(pitch) * ((height + 31) & ~31u) * yScale;
    return static_cast<uint32_t>((bytes + 0xFFFF) & ~uint64_t(0xFFFF));
}

// @func 0x0052F32C AglSurface::BindTile
void SurfaceBindTile(AglSurface* surface, bool depth)
{
    if (surface->tileIndex < 0)
        return;
    const AglAntialiasInfo* aa = GetAntialiasInfo(surface->antialias, surface->format);
    TileInfo& tile = g_tileInfo[surface->tileIndex];
    tile.location = surface->mem.location;
    tile.offset = surface->pitchOffset;
    tile.size = surface->mem.size;
    tile.pitch = surface->pitch;
    tile.compression = aa ? (depth ? aa->depthCompression : aa->compression) : 0;
    tile.compressionTag = surface->compressionTag >= 0 ? uint32_t(surface->compressionTag) : 0;
    tile.bank = surface->tileBank;
}

// @func 0x00530998 AglSurface::BindZcull
void SurfaceBindZcull(AglSurface* surface)
{
    // Z-cull is a console hardware feature; the region index and start
    // offset are kept for parity, there is nothing to program on PC.
    if (surface->zcullIndex < 0)
        return;
}

// @func 0x00530DB4 AglSurface::Create
AglSurface* SurfaceCreate(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias,
                                gpu::MemoryLocation location)
{
    AglSurface* surface = NewSurface();
    const AglAntialiasInfo* aa = GetAntialiasInfo(antialias, format);
    uint32_t pitch = 0;
    const uint32_t size = SurfaceComputeSize(width, height, format, antialias, true, &pitch);

    MemBlockAlloc(&surface->mem, size, 0x10000, location, kAglMem_RenderTarget);
    surface->type = kAglSurface_Pitch;
    surface->antialias = aa ? aa->mode : kAglAA_None;
    surface->format = format;
    surface->surfaceFormat = GetFormatInfo(format).surfaceFormat;
    surface->width = static_cast<uint16_t>(width);
    surface->height = static_cast<uint16_t>(height);
    // NOTE: the console stored 0 here and computed the pitch on demand from
    // the tile; keeping the padded pitch is equivalent and simpler.
    surface->pitch = pitch;
    surface->bitsPerPixel = GetFormatInfo(format).bitsPerPixel;
    surface->tileIndex = static_cast<int8_t>(AllocTile());
    surface->compressionTag = -1;
    if (aa && aa->compression != 0) {
        surface->compressionTag = g_nextCompressionTag;
        g_nextCompressionTag += static_cast<int32_t>((size + 0xFFFF) >> 16);
    }
    surface->tileBank = 0;
    SurfaceBindTile(surface, false);
    return surface;
}

// @func 0x00530A9C AglSurface::CreateDepth
AglSurface* SurfaceCreateDepth(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias)
{
    AglSurface* surface = NewSurface();
    const AglAntialiasInfo* aa = GetAntialiasInfo(antialias, format);
    uint32_t pitch = 0;
    const uint32_t size = SurfaceComputeSize(width, height, format, antialias, true, &pitch);

    MemBlockAlloc(&surface->mem, size, 0x10000, gpu::MemoryLocation::Local, kAglMem_RenderTarget);
    surface->type = kAglSurface_Pitch;
    surface->antialias = aa ? aa->mode : kAglAA_None;
    surface->format = format;
    surface->surfaceFormat = GetFormatInfo(format).surfaceFormat;
    surface->width = static_cast<uint16_t>(width);
    surface->height = static_cast<uint16_t>(height);
    surface->pitch = pitch;
    surface->bitsPerPixel = GetFormatInfo(format).bitsPerPixel;
    surface->tileIndex = static_cast<int8_t>(AllocTile());
    surface->compressionTag = -1;
    if (aa && aa->depthCompression != 0) {
        surface->compressionTag = g_nextCompressionTag;
        g_nextCompressionTag += static_cast<int32_t>((size + 0xFFFF) >> 16);
    }
    surface->tileBank = 1;
    SurfaceBindTile(surface, true);

    // Claim a z-cull region while the shared z-cull memory lasts.
    const uint32_t xScale = aa ? aa->xScale : 1;
    const uint32_t yScale = aa ? aa->yScale : 1;
    const uint32_t region = ((height * yScale + 63) & ~63u) * ((width * xScale + 63) & ~63u);
    surface->zcullStart = g_zcullUsed;
    if (g_zcullUsed + region < kZcullMemory) {
        g_zcullUsed += region;
        surface->zcullIndex = static_cast<int8_t>(g_nextZcullIndex++);
    } else {
        surface->zcullIndex = -1;
    }
    SurfaceBindZcull(surface);
    return surface;
}

// @func 0x00532E70 AglSurface::CreateView
AglSurface* SurfaceCreateView(const AglSurface* source, uint16_t width, uint16_t height,
                                    uint32_t yOffset)
{
    AglSurface* view = NewSurface();
    *view = *source;
    MemBlockMakeView(&view->mem, source->mem);
    view->isView = true;
    view->width = width;
    view->height = height;
    // The view starts on a row that is a multiple of 8.
    if (yOffset != 0) {
        const uint32_t rows = ((yOffset - 1) & ~7u) + 8;
        view->pitchOffset += rows * source->pitch;
    }
    return view;
}

void SurfaceDestroy(AglSurface* surface)
{
    if (!surface)
        return;
    if (!surface->isView && surface->tileIndex >= 0)
        g_tilesUsed = static_cast<uint16_t>(g_tilesUsed & ~(1u << surface->tileIndex));
    MemBlockFree(&surface->mem);
    delete surface;
}

// @func 0x00537684 AglSurface::Release
void SurfaceRelease(AglSurface** slot)
{
    AglSurface* surface = *slot;
    if (surface && !surface->isView) {
        // Tile and z-cull regions are bookkeeping only on PC.
        if (surface->tileIndex >= 0)
            g_tilesUsed = static_cast<uint16_t>(g_tilesUsed & ~(1u << surface->tileIndex));
        MemBlockFree(&surface->mem);
        delete surface;
    }
    *slot = nullptr;
}

// @func 0x0052F06C AglSurface::FreeMemory
// Releases a surface's memory and, unless it is a view, its tile region.
void SurfaceFreeMemory(AglSurface* surface)
{
    if (!surface->isView && surface->tileIndex >= 0)
        g_tilesUsed = static_cast<uint16_t>(g_tilesUsed & ~(1u << surface->tileIndex));
    MemBlockFree(&surface->mem);
}

// --- textures ----------------------------------------------------------------------

// @func 0x00532D5C AglTextureImage::FromSurface
void TextureFromSurface(AglTextureImage* texture, AglSurface* surface)
{
    MemBlockMakeView(&texture->mem, surface->mem);
    texture->surface = surface;
    texture->format = surface->format;
    texture->isView = true;

    const AglFormatInfo& info = GetFormatInfo(surface->format);
    gpu::TextureDesc& d = texture->desc;
    d.format = info.textureFormat;
    d.mipmaps = 1;
    d.dimension = gpu::TextureDimension::Tex2D;
    d.cubemap = false;
    d.remap = info.remap;
    d.width = surface->width;
    d.height = surface->height;
    d.depth = 1;
    d.pitch = surface->pitch;
    // Render target views start at the surface's first visible row.
    d.memory = surface->mem.Ptr(surface->pitchOffset + size_t(surface->pitch) * surface->y);
}

// @func 0x00536E3C AglTextureImage::Create
void TextureCreate(AglTextureImage* texture, uint16_t width, uint16_t height, uint8_t mipCount, uint32_t format,
                   bool fromSurface, AglSurface** outSurface, bool mainMemory)
{
    if (fromSurface) {
        AglSurface* surface = SurfaceCreate(width, height, format, kAglAA_None, gpu::MemoryLocation::Local);
        TextureFromSurface(texture, surface);
        if (outSurface)
            *outSurface = surface;
        return;
    }

    const AglFormatInfo& info = GetFormatInfo(format);
    texture->format = format;
    gpu::TextureDesc& d = texture->desc;
    d.format = info.textureFormat;
    d.mipmaps = mipCount;
    d.dimension = gpu::TextureDimension::Tex2D;
    d.cubemap = false;
    d.remap = info.remap;
    d.width = width;
    d.height = height;
    d.depth = 1;
    const uint32_t rowBytes = (uint32_t(width) * info.bitsPerPixel) >> 3;
    d.pitch = (info.textureFormat & gpu::kTexFmt_Linear) ? rowBytes : 0;

    // NOTE: every level is sized with the full row length, which
    // over-allocates small mips of swizzled textures (as on the console).
    uint32_t size = 0;
    for (uint8_t level = 0; level < mipCount; ++level)
        size += (uint32_t(height >> level) * rowBytes);

    MemBlockAlloc(&texture->mem, size, 0x80,
                  mainMemory ? gpu::MemoryLocation::Main : gpu::MemoryLocation::Local, kAglMem_Texture);
    d.memory = texture->mem.Ptr();
    texture->isView = false;
}

} // namespace agl
