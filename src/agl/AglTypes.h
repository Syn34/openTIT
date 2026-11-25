// openTIT - Core Agl graphics layer types: GPU memory blocks, formats, surfaces, textures.
//
// Agl is the engine's graphics layer. These are its low-level resource
// objects; everything GPU specific goes through agl::gpu (CommandList and
// Device), so on PC the same objects work with any backend.
#pragma once

#include "agl/gpu/GpuTypes.h"

#include <cstddef>
#include <cstdint>

namespace agl {

// --- GPU memory -----------------------------------------------------------

// Memory categories passed to the system allocator with GPU allocations
// (used for memory reports only).
enum AglMemCategory : uint8_t {
    kAglMem_General      = 0,
    kAglMem_Texture      = 1,
    kAglMem_RenderTarget = 4,
    kAglMem_Shader       = 5,
    kAglMem_CommandBuffer = 7,
};

// A block of GPU-visible memory. Blocks can also be views into another
// block's memory (render target aliases, texture views); views never free.
struct AglMemBlock {
    void* data = nullptr;
    gpu::MemoryLocation location = gpu::MemoryLocation::Local;
    uint32_t size = 0;
    bool borrowed = false;     // memory owned by another block
    uint8_t category = 0;      // AglMemCategory

    gpu::GpuPtr Ptr(size_t offset = 0) const
    {
        return gpu::GpuPtr{ static_cast<const uint8_t*>(data) + offset, location };
    }
};

// Allocates 'size' bytes aligned to 'align' in local or main GPU memory.
void MemBlockAlloc(AglMemBlock* block, uint32_t size, uint32_t align, gpu::MemoryLocation location,
                   uint8_t category);
// Releases the memory (no-op for views and empty blocks).
void MemBlockFree(AglMemBlock* block);
// Turns 'block' into a view of 'source' (same memory, not owned).
void MemBlockMakeView(AglMemBlock* block, const AglMemBlock& source);

// --- formats --------------------------------------------------------------

// Agl image formats (index into the format table).
enum AglFormat : uint32_t {
    kAglFmt_X8R8G8B8 = 0,
    kAglFmt_LinX8R8G8B8,
    kAglFmt_A8R8G8B8,
    kAglFmt_LinA8R8G8B8,
    kAglFmt_ShadowZ16,
    kAglFmt_ShadowZ24S8,
    kAglFmt_Z16,
    kAglFmt_D24S8,
    kAglFmt_LinDXT1,
    kAglFmt_LinDXT3,
    kAglFmt_LinDXT5,
    kAglFmt_DXT1,
    kAglFmt_DXT3,
    kAglFmt_DXT5,
    kAglFmt_G8B8,
    kAglFmt_LinG8B8,
    kAglFmt_R8G8,
    kAglFmt_LinR8G8,
    kAglFmt_Indexed8,
    kAglFmt_Count
};

struct AglFormatInfo {
    uint8_t textureFormat;    // gpu::TextureFormat | flags
    uint8_t bitsPerPixel;
    const char* name;
    uint32_t remap;           // texture component swizzle
    uint8_t surfaceFormat;    // SurfaceColorFormat / SurfaceDepthFormat when renderable, else 0
    uint8_t id;
};

const AglFormatInfo& GetFormatInfo(uint32_t format);
// First format whose texture format byte is 'textureFormat' (a GTF image's
// format); 0 when none matches.
uint32_t FindFormatByTextureFormat(uint8_t textureFormat);
bool IsDepthFormat(uint32_t format);

// Antialiasing modes of render surfaces (console surface AA values).
enum AglAntialias : uint32_t {
    kAglAA_None = 0,
    kAglAA_2x   = 3,
    kAglAA_4x   = 4,
};

struct AglAntialiasInfo {
    uint32_t mode;            // AglAntialias
    uint32_t samples;
    uint32_t xScale;          // storage multiplier in x
    uint32_t yScale;          // storage multiplier in y
    uint32_t zcullFormat;
    uint32_t compression;     // tile compression for color surfaces
    uint32_t depthCompression;// tile compression for depth surfaces
};

// Null for unsupported modes.
const AglAntialiasInfo* GetAntialiasInfo(uint32_t antialias, uint32_t format);

// --- render surfaces ------------------------------------------------------

// Surface layout.
enum AglSurfaceType : uint8_t {
    kAglSurface_Pitch   = 1,
    kAglSurface_Swizzle = 2,
};

// A color or depth render target image.
struct AglSurface {
    AglMemBlock mem;
    uint32_t pitchOffset = 0;   // byte offset of the image inside 'mem' (views)
    uint8_t type = kAglSurface_Pitch;
    uint32_t antialias = kAglAA_None;
    uint32_t format = kAglFmt_LinX8R8G8B8;   // AglFormat
    uint32_t surfaceFormat = 5;              // gpu SurfaceColorFormat / SurfaceDepthFormat value
    uint32_t pitch = 0x40;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t bitsPerPixel = 0;
    int8_t tileIndex = -1;      // console memory tile region (bookkeeping only on PC)
    int8_t tileBank = -1;
    int8_t zcullIndex = -1;     // console z-cull region (bookkeeping only on PC)
    uint32_t zcullStart = 0;
    int32_t compressionTag = -1;
    bool isView = false;        // shares another surface's memory and tile

    gpu::GpuPtr Ptr() const { return mem.Ptr(pitchOffset); }
};

void SurfaceInit(AglSurface* surface);
// Pitch (written to *outPitch) and total byte size of a surface.
uint32_t SurfaceComputeSize(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias,
                            bool tiled, uint32_t* outPitch);
// Creates a tiled render target (color or depth) with its own memory.
AglSurface* SurfaceCreate(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias,
                                gpu::MemoryLocation location);
// Creates a depth surface with a z-cull region.
AglSurface* SurfaceCreateDepth(uint32_t width, uint32_t height, uint32_t format, uint32_t antialias);
// Creates a view of 'source' with a new size, starting 'yOffset' rows in.
AglSurface* SurfaceCreateView(const AglSurface* source, uint16_t width, uint16_t height,
                                    uint32_t yOffset);
// Releases the surface memory and tile region (the object stays valid).
void SurfaceFreeMemory(AglSurface* surface);
void SurfaceDestroy(AglSurface* surface);
// Releases the surface held in '*slot' (views are only dropped) and clears the slot.
void SurfaceRelease(AglSurface** slot);
void SurfaceBindTile(AglSurface* surface, bool depth);
void SurfaceBindZcull(AglSurface* surface);

// --- textures -------------------------------------------------------------

struct AglTextureImage {
    AglMemBlock mem;
    gpu::TextureDesc desc;
    AglSurface* surface = nullptr;   // render surface backing the image, if any
    uint32_t format = 0;                   // AglFormat
    bool isView = false;                   // 'surface' belongs to someone else

    uint8_t MipCount() const { return desc.mipmaps; }
};

// Allocates image memory for a width x height texture with 'mipCount'
// levels. When 'fromSurface' is set a render surface is created instead and
// the texture becomes a view of it (*outSurface receives the surface).
void TextureCreate(AglTextureImage* texture, uint16_t width, uint16_t height, uint8_t mipCount, uint32_t format,
                   bool fromSurface, AglSurface** outSurface, bool mainMemory);
// Makes 'texture' a sampling view of a render surface.
void TextureFromSurface(AglTextureImage* texture, AglSurface* surface);

} // namespace agl
