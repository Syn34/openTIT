// openTIT - Agl texture resources: creation, loading (BMP, TGA, GTF) and default textures.
#include "agl/AglTexture.h"

#include "agl/AglResources.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/memory/ApeTFreeList.h"

#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace agl {

namespace {

uint32_t g_nextTextureId = 0;         // 0x01164CD0
uint32_t g_nextTextureSerial = 0;     // 0x010DD594
std::mutex g_textureLock;             // 0x010DDAF4
AglTexture* g_defaultTextures[kAglTex_DefaultCount] = {};   // 0x010DDADC
const uint8_t* g_packBuffer = nullptr;                      // 0x01164DEC

using TextureAllocator = ape::ApeTListAllocator<AglTexture>;

uint16_t ReadLE16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t ReadLE32(const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
uint16_t ReadBE16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
uint32_t ReadBE32(const uint8_t* p) { return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]; }

// Magenta (255, 0, 255) is the transparency key of BMP and TGA images.
bool IsColorKey(const uint8_t* bgr)
{
    return bgr[0] == 0xFF && bgr[1] == 0x00 && bgr[2] == 0xFF;
}

// Converts one row of 24/32-bit BGR(A) pixels to A,R,G,B bytes.
void ConvertRow(const uint8_t* src, uint8_t* dst, uint32_t width, bool hasAlpha)
{
    const uint32_t srcStride = hasAlpha ? 4 : 3;
    for (uint32_t x = 0; x < width; ++x, src += srcStride, dst += 4) {
        if (IsColorKey(src)) {
            dst[0] = dst[1] = dst[2] = dst[3] = 0;
        } else {
            dst[0] = hasAlpha ? src[3] : 0xFF;
            dst[1] = src[2];
            dst[2] = src[1];
            dst[3] = src[0];
        }
    }
}

uint8_t* ScratchAlloc(size_t size)
{
    return static_cast<uint8_t*>(ape::GetMemStack()->Alloc(size));
}

// Builds a small uncompressed 24-bit BMP in memory.
std::vector<uint8_t> MakeBmp(uint32_t width, uint32_t height, const uint8_t* rowBgr)
{
    const uint32_t pixelBytes = width * height * 3;
    std::vector<uint8_t> bmp(54 + pixelBytes, 0);
    auto put16 = [&](size_t at, uint32_t v) { bmp[at] = uint8_t(v); bmp[at + 1] = uint8_t(v >> 8); };
    auto put32 = [&](size_t at, uint32_t v) { put16(at, v & 0xFFFF); put16(at + 2, v >> 16); };
    bmp[0] = 'B';
    bmp[1] = 'M';
    put32(2, uint32_t(bmp.size()));
    put32(10, 54);
    put32(14, 40);
    put32(18, width);
    put32(22, height);
    put16(26, 1);
    put16(28, 24);
    put32(34, pixelBytes);
    for (uint32_t y = 0; y < height; ++y)
        std::memcpy(&bmp[54 + y * width * 3], rowBgr, width * 3);
    return bmp;
}

} // namespace

// --- construction ------------------------------------------------------------------

// @func 0x0050A2F0 AglTextureBase::AglTextureBase
// @func 0x0050A3E4 AglTextureBase::AglTextureBase (copy 2)
AglTextureBase::AglTextureBase()
{
    refCount = 1;
    resourceFlags = 0x0400;
    id = ++g_nextTextureId;
    image.mem.location = gpu::MemoryLocation::Main;
    serial = g_nextTextureSerial++;
    sampler[0] = 1;
    sampler[1] = 1;
    sampler[2] = 1;
    sampler[3] = 3;
    sampler[4] = 3;
    sampler[5] = 3;
    sampler[6] = 7;
}

// @func 0x0050A5E4 AglTextureBase::~AglTextureBase
// @func 0x0050A404 AglTextureBase::~AglTextureBase (copy 2)
// @func 0x0050A4E8 AglTextureBase::~AglTextureBase (deleting)
AglTextureBase::~AglTextureBase()
{
    if (attachment) {
        attachment->Release();
        attachment = nullptr;
    }
    MemBlockFree(&image.mem);
    if (!image.isView && image.surface) {
        SurfaceDestroy(image.surface);
        image.surface = nullptr;
    }
}

// @func 0x0053A284 AglTexture::AglTexture
AglTexture::AglTexture() = default;

// @func 0x0053A2C4 AglTexture::~AglTexture
// @func 0x0053A2F4 AglTexture::~AglTexture (deleting)
AglTexture::~AglTexture() = default;

// @func 0x0051D368 AglTexture::Create
AglTexture* AglTexture::Create()
{
    return TextureAllocator::GetAllocator().New();
}

// @func 0x0051E1D8 AglTexture::Destroy
void AglTexture::Destroy(void* object)
{
    std::lock_guard<std::mutex> lock(g_textureLock);
    TextureAllocator::GetAllocator().Delete(static_cast<AglTexture*>(object));
}

// @func 0x0050AF9C AglTexture::Release
void AglTexture::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglTexture::Destroy);
}

void AglTexture::SetName(const char* textureName)
{
    ape::StrNCopy(name, textureName, sizeof(name));
    name[sizeof(name) - 1] = '\0';
    nameHash = ape::HashNameNoCase(name);
}

// @func 0x005398CC AglTexture::UseDefaultImage
bool AglTexture::UseDefaultImage()
{
    const AglTexture* fallback = GetDefaultTexture(kAglTex_White);
    if (!fallback)
        return false;
    image.desc = fallback->image.desc;
    MemBlockMakeView(&image.mem, fallback->image.mem);
    image.surface = nullptr;
    return true;
}

// Wrap modes live in sampler[3..5]; bit 1 of sampler[6] marks them as set.
// @func 0x007A0E98 AglTextureBase::SetWrap
void AglTextureBase::SetWrap(uint16_t wrapS, uint16_t wrapT, uint16_t wrapR)
{
    sampler[3] = wrapS;
    sampler[4] = wrapT;
    sampler[5] = wrapR;
    sampler[6] |= 2;
}

namespace {
// Name hash -> texture (0x010DDAF4).
std::map<uint32_t, AglTexture*>& TextureNames()
{
    static std::map<uint32_t, AglTexture*> names;
    return names;
}
} // namespace

void RegisterTextureName(AglTexture* texture)
{
    TextureNames()[texture->nameHash] = texture;
}

void UnregisterTextureName(AglTexture* texture)
{
    auto it = TextureNames().find(texture->nameHash);
    if (it != TextureNames().end() && it->second == texture)
        TextureNames().erase(it);
}

// @func 0x0050C398 FindTexture
AglTexture* FindTexture(uint32_t nameHash)
{
    auto it = TextureNames().find(nameHash);
    return it != TextureNames().end() ? it->second : nullptr;
}

// @func 0x0050A28C AglTextureBase::SetHeader
void AglTextureBase::SetHeader(const AglTextureHeader& header)
{
    nameHash = header.nameHash;
    width = header.width;
    height = header.height;
    ape::StrNCopy(name, header.name, sizeof(name));
    name[sizeof(name) - 1] = '\0';
}

// @func 0x0053A94C AglTexture::ApplyHeader
void AglTexture::ApplyHeader(const AglTextureHeader& header)
{
    SetHeader(header);
    if (g_packBuffer)
        LoadGtf(g_packBuffer + header.packOffset, header.packSize, true);
}

// --- loading ----------------------------------------------------------------------

// @func 0x0053A34C AglTexture::CreateFromPixels
bool AglTexture::CreateFromPixels(const AglDecodedImage& decoded)
{
    if ((decoded.width & 7) != 0 || (decoded.height & 7) != 0)
        return false;
    width = decoded.width;
    height = decoded.height;
    TextureCreate(&image, decoded.width, decoded.height, 1, kAglFmt_LinA8R8G8B8, false, nullptr, false);
    sampler[0] = 1;
    sampler[1] = 1;
    sampler[6] |= 1;
    sampler[2] = 0;
    if (!image.mem.data)
        return false;
    std::memcpy(image.mem.data, decoded.pixels, size_t(width) * height * 4);
    return true;
}

// @func 0x0053A448 AglTexture::LoadGtf
bool AglTexture::LoadGtf(const void* data, uint32_t size, bool inPlace)
{
    if (!inPlace) {
        MemBlockAlloc(&image.mem, size, 0x80, gpu::MemoryLocation::Local, kAglMem_Texture);
        if (image.mem.data)
            std::memcpy(image.mem.data, data, size);
    } else {
        image.mem.data = const_cast<void*>(data);
        image.mem.size = size;
        image.mem.category = kAglMem_Texture;
        image.mem.location = gpu::MemoryLocation::Local;
        image.mem.borrowed = true;
    }

    ParseGtfImage(size);
    return true;
}

void AglTexture::ParseGtfImage(uint32_t size)
{
    // GTF: header { version, size, numTextures } followed by texture
    // attributes { id, offsetToTex, textureSize, texture descriptor }.
    const auto* gtf = static_cast<const uint8_t*>(image.mem.data);
    if (gtf && size >= 12 && ReadBE32(gtf + 4) <= size) {
        const uint8_t* attr = gtf + 12;
        const uint32_t offset = ReadBE32(attr + 4);
        const uint32_t texSize = ReadBE32(attr + 8);
        if (offset + texSize <= size) {
            const uint8_t* t = attr + 12;
            gpu::TextureDesc& d = image.desc;
            d.format = t[0];
            d.mipmaps = t[1];
            d.dimension = static_cast<gpu::TextureDimension>(t[2]);
            d.cubemap = t[3] != 0;
            d.remap = ReadBE32(t + 4);
            d.width = ReadBE16(t + 8);
            d.height = ReadBE16(t + 10);
            d.depth = ReadBE16(t + 12);
            d.pitch = ReadBE32(t + 16);
            d.memory = image.mem.Ptr(offset);
        }
    }
    width = image.desc.width;
    height = image.desc.height;
}

// @func 0x0053A640 AglTexture::Load
bool AglTexture::Load(const void* data, uint32_t size, const char* extension)
{
    AglDecodedImage decoded;
    bool ok;
    if (ape::StrICmp(extension, "BMP") == 0)
        ok = DecodeBmp(static_cast<const uint8_t*>(data), size, &decoded);
    else if (ape::StrICmp(extension, "TGA") == 0)
        ok = DecodeTga(static_cast<const uint8_t*>(data), size, &decoded);
    else if (ape::StrICmp(extension, "GTF") == 0)
        return LoadGtf(data, size, false);
    else
        return false;
    if (!ok)
        return false;
    const bool created = CreateFromPixels(decoded);
    if (decoded.pixels)
        ape::GetMemStack()->Free(decoded.pixels);
    return created;
}

// @func 0x0050A6C8 DecodeBmp
bool DecodeBmp(const uint8_t* data, uint32_t size, AglDecodedImage* out)
{
    if (size < 54 || data[0] != 'B' || data[1] != 'M')
        return false;
    // Only BITMAPINFOHEADER files with one plane are supported.
    if (ReadLE16(data + 14) != 40)
        return false;
    out->width = ReadLE16(data + 18);
    out->height = ReadLE16(data + 22);
    if (ReadLE16(data + 26) != 1)
        return false;
    const uint16_t bpp = ReadLE16(data + 28);
    uint32_t colorsUsed = ReadLE32(data + 46);
    const uint32_t pixelOffset = ReadLE32(data + 10);
    if (((uint32_t(out->width) * bpp) >> 3) == 0)
        return false;
    out->bitsPerPixel = bpp;
    const uint32_t width = out->width;
    const uint32_t height = out->height;

    if (bpp == 24 || bpp == 32) {
        out->pixels = ScratchAlloc(size_t(width) * height * 4);
        if (!out->pixels)
            return false;
        out->bitsPerPixel = 32;
        // Rows are stored bottom-up.
        const uint8_t* src = data + pixelOffset;
        for (uint32_t row = 0; row < height; ++row) {
            ConvertRow(src, out->pixels + size_t(height - 1 - row) * width * 4, width, bpp == 32);
            src += width * (bpp / 8);
        }
        return true;
    }
    if (bpp != 8 && bpp != 4)
        return false;

    // Palettized image: palette entries become R,G,B,A.
    if (colorsUsed == 0)
        colorsUsed = 1u << bpp;
    const uint32_t paletteSize = 1u << bpp;
    out->paletteCount = static_cast<uint16_t>(paletteSize);
    out->palette = ScratchAlloc(paletteSize * 4);
    if (!out->palette)
        return false;
    std::memset(out->palette, 0, paletteSize * 4);
    const uint8_t* src = data + 54;
    for (uint32_t i = 0; i < colorsUsed && i < paletteSize; ++i, src += 4) {
        uint8_t* entry = out->palette + i * 4;
        if (IsColorKey(src)) {
            entry[0] = entry[1] = entry[2] = entry[3] = 0;
        } else {
            entry[2] = src[0];
            entry[1] = src[1];
            entry[0] = src[2];
            entry[3] = (out->flags & 1) ? src[3] : 0xFF;
        }
    }
    const uint32_t rowBytes = (width * bpp) / 8;
    const uint32_t srcStride = (rowBytes + 3) & ~3u;
    out->pixels = ScratchAlloc(size_t(rowBytes) * height);
    if (!out->pixels)
        return false;
    for (uint32_t row = 0; row < height; ++row, src += srcStride) {
        uint8_t* dst = out->pixels + size_t(height - 1 - row) * rowBytes;
        if (bpp == 4) {
            // Nibbles are swapped so the first pixel is in the low nibble.
            for (uint32_t i = 0; i < rowBytes; ++i)
                dst[i] = static_cast<uint8_t>((src[i] << 4) | (src[i] >> 4));
        } else {
            std::memcpy(dst, src, rowBytes);
        }
    }
    return true;
}

// @func 0x0050ACAC DecodeTga
bool DecodeTga(const uint8_t* data, uint32_t size, AglDecodedImage* out)
{
    // Uncompressed true-color images only (no color map, image type 2).
    if (size < 18 || data[1] != 0 || data[2] != 2)
        return false;
    out->width = ReadLE16(data + 12);
    out->height = ReadLE16(data + 14);
    const uint8_t bpp = data[16];
    out->bitsPerPixel = bpp;
    const uint8_t descriptor = data[17];
    const uint8_t alphaBits = descriptor & 0x0F;
    if (!((bpp == 24 && alphaBits == 0) || (bpp == 32 && alphaBits == 8)))
        return false;

    const uint32_t width = out->width;
    const uint32_t height = out->height;
    out->pixels = ScratchAlloc(size_t(width) * height * 4);
    if (!out->pixels)
        return false;
    const bool topDown = (descriptor & 0x20) != 0;
    const uint8_t* src = data + 18 + data[0];   // skip the image ID field
    for (uint32_t row = 0; row < height; ++row) {
        const uint32_t dstRow = topDown ? row : height - 1 - row;
        ConvertRow(src, out->pixels + size_t(dstRow) * width * 4, width, bpp == 32);
        src += width * (bpp / 8);
    }
    return true;
}

// @func 0x0051D91C AglTexture::CreateFromMemory
AglTexture* TextureCreateFromMemory(const void* data, uint32_t size, const char* extension, const char* name)
{
    if (!data)
        return nullptr;
    AglTexture* texture = AglTexture::Create();
    texture->SetName(name);
    if (!texture->Load(data, size, extension)) {
        texture->Release();
        return nullptr;
    }
    return texture;
}

// --- default textures ---------------------------------------------------------

// @func 0x004FEF30 GetDefaultTexture
AglTexture* GetDefaultTexture(int index)
{
    return g_defaultTextures[index];
}

void SetTexturePackBuffer(const uint8_t* buffer)
{
    g_packBuffer = buffer;
}

// @func 0x0051DA1C CreateDefaultTextures
void CreateDefaultTextures()
{
    // The console embedded five tiny BMP files; they are generated here.
    // Rows are B,G,R triplets; every row of an image is the same.
    // NOTE: the original falloff image varies by +-1 between rows; one
    // representative row is used for all of them.
    static const uint8_t kFalloffRamp[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 52, 88, 127, 166, 203, 234 };
    uint8_t white[8 * 3], grey[8 * 3], black[8 * 3], normal[8 * 3], falloff[16 * 3];
    for (int x = 0; x < 8; ++x) {
        for (int c = 0; c < 3; ++c) {
            white[x * 3 + c] = 0xFF;
            grey[x * 3 + c] = 0x80;
            black[x * 3 + c] = 0x00;
        }
        normal[x * 3 + 0] = 0xFF;   // B: +Z
        normal[x * 3 + 1] = 0x80;
        normal[x * 3 + 2] = 0x80;
    }
    for (int x = 0; x < 16; ++x)
        falloff[x * 3 + 0] = falloff[x * 3 + 1] = falloff[x * 3 + 2] = kFalloffRamp[x];

    struct Entry {
        const char* fileName;
        const char* name;
        uint32_t width;
        const uint8_t* row;
        int slots[2];
    };
    const Entry entries[] = {
        { "MEM:White.Bmp",   "APE_WHITE",   8,  white,   { kAglTex_White, kAglTex_White2 } },
        { "MEM:Grey.Bmp",    "APE_GREY",    8,  grey,    { kAglTex_Grey, -1 } },
        { "MEM:Falloff.Bmp", "APE_FALLOFF", 16, falloff, { kAglTex_Falloff, -1 } },
        { "MEM:Normal.Bmp",  "APE_NORMAL",  8,  normal,  { kAglTex_Normal, -1 } },
        { "MEM:Black.Bmp",   "APE_BLACK",   8,  black,   { kAglTex_Black, -1 } },
    };
    for (const Entry& e : entries) {
        const std::vector<uint8_t> bmp = MakeBmp(e.width, 8, e.row);
        AglTexture* texture = AglTexture::Create();
        texture->SetName(e.fileName);
        if (!texture->Load(bmp.data(), uint32_t(bmp.size()), "Bmp")) {
            texture->Release();
            continue;
        }
        texture->SetName(e.name);
        for (int slot : e.slots) {
            if (slot >= 0)
                g_defaultTextures[slot] = texture;
        }
    }
}

// --- render target textures -----------------------------------------------------

// @func 0x005370A4 AglTextureImage::CreateRenderTarget
// @func 0x00537040 AglTextureImage::CreateRenderTarget (copy 2)
void TextureCreateRenderTarget(AglTextureImage* image, uint16_t width, uint16_t height, uint32_t format,
                               AglSurface** outSurface)
{
    AglSurface* surface = SurfaceCreate(width, height, format, kAglAA_None, gpu::MemoryLocation::Local);
    TextureFromSurface(image, surface);
    if (outSurface)
        *outSurface = surface;
}

// @func 0x0051E33C AglTexture::CreateColorTarget
AglTexture* TextureCreateColorTarget(uint16_t width, uint16_t height, uint32_t format, AglSurface* depth)
{
    AglTexture* texture = AglTexture::Create();
    texture->SetName("Color-RenderTarget");
    texture->width = width;
    texture->height = height;
    AglSurface* surface = nullptr;
    TextureCreateRenderTarget(&texture->image, width, height, format, &surface);
    texture->attachment = AglRenderSurface::CreateForTexture(nullptr, depth, texture);
    return texture;
}

// @func 0x0051E428 AglTexture::CreateDepthTarget
AglTexture* TextureCreateDepthTarget(uint16_t width, uint16_t height, uint32_t format)
{
    AglTexture* texture = AglTexture::Create();
    texture->SetName("Z-RenderTarget");
    texture->width = width;
    texture->height = height;
    AglSurface* surface = nullptr;
    TextureCreateRenderTarget(&texture->image, width, height, format, &surface);
    texture->attachment = AglRenderSurface::CreateForTexture(nullptr, nullptr, texture);
    return texture;
}

// @func 0x0051E514 AglTexture::CreateFromImage
AglTexture* TextureCreateFromImage(const AglTextureImage& source, AglSurface* depth)
{
    AglSurface* color = source.surface;
    AglTexture* texture = AglTexture::Create();
    texture->SetName("Color-RenderTarget");
    texture->width = color ? color->width : source.desc.width;
    texture->height = color ? color->height : source.desc.height;
    // Shares the source image (and its memory) without owning it.
    texture->image.desc = source.desc;
    MemBlockMakeView(&texture->image.mem, source.mem);
    texture->image.surface = nullptr;
    texture->attachment = AglRenderSurface::CreateForTexture(color, depth, texture);
    return texture;
}

} // namespace agl
