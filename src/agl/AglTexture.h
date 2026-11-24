// openTIT - Agl texture resources: creation, loading (BMP, TGA, GTF) and default textures.
#pragma once

#include "agl/AglDeferred.h"
#include "agl/AglTypes.h"

#include <cstdint>

namespace agl {

class AglResource;

// Pixels decoded from a BMP or TGA file (held on the engine scratch stack).
struct AglDecodedImage {
    uint8_t* pixels = nullptr;     // A,R,G,B bytes per pixel (or palette indices)
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t bitsPerPixel = 0;
    uint8_t* palette = nullptr;    // R,G,B,A entries for indexed images
    uint16_t paletteCount = 0;
    uint16_t flags = 0;            // bit 0: palette alpha is valid
};

// Header of a texture stored in a level package.
struct AglTextureHeader {
    uint32_t nameHash;
    uint16_t width;
    uint16_t height;
    uint8_t field_8[3];
    char name[32];
    uint32_t packOffset;           // pixel data offset in the package buffer
    uint32_t packSize;
};

class AglTextureBase : public AglRefCounted {
public:
    AglTextureBase();
    virtual ~AglTextureBase();

    AglTextureBase(const AglTextureBase&) = delete;
    AglTextureBase& operator=(const AglTextureBase&) = delete;

    // Name, hash and size from a level package header.
    void SetHeader(const AglTextureHeader& header);
    // Texture coordinate wrap modes (gpu::TextureWrap values) for s, t, r.
    void SetWrap(uint16_t wrapS, uint16_t wrapT, uint16_t wrapR);

    uint16_t resourceFlags = 0;
    uint32_t id = 0;
    uint32_t field_C = 0;
    char name[32] = {};
    uint32_t nameHash = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    AglTextureImage image;
    AglResource* attachment = nullptr;   // render surface wrapping this texture (released with it)
    uint32_t serial = 0;
    // NOTE: default sampling parameters; their exact meaning is not known
    // yet (the loaders set [0], [1], [2] and bit 0 of [6]).
    uint16_t sampler[7] = {};
};

class AglTexture : public AglTextureBase {
public:
    AglTexture();
    ~AglTexture() override;

    static AglTexture* Create();
    static void Destroy(void* texture);
    void Release();

    void SetName(const char* textureName);
    // Uses the image of default texture 0 when loading failed.
    bool UseDefaultImage();
    // Applies a level package header and its pixel data.
    void ApplyHeader(const AglTextureHeader& header);
    // Builds a 32-bit texture from decoded pixels (sizes must be multiples of 8).
    bool CreateFromPixels(const AglDecodedImage& decoded);
    // Uses a GTF image (copied to GPU memory unless 'inPlace').
    bool LoadGtf(const void* data, uint32_t size, bool inPlace);
    // Loads an image file by extension ("BMP", "TGA" or "GTF").
    bool Load(const void* data, uint32_t size, const char* extension);
};

bool DecodeBmp(const uint8_t* data, uint32_t size, AglDecodedImage* out);
bool DecodeTga(const uint8_t* data, uint32_t size, AglDecodedImage* out);

// Creates a named texture from an image file in memory (null on failure).
AglTexture* TextureCreateFromMemory(const void* data, uint32_t size, const char* extension, const char* name);

// Default textures: 0/1 white, 2 grey, 3 falloff ramp, 4 flat normal, 5 black.
enum AglDefaultTexture {
    kAglTex_White = 0,
    kAglTex_White2 = 1,
    kAglTex_Grey = 2,
    kAglTex_Falloff = 3,
    kAglTex_Normal = 4,
    kAglTex_Black = 5,
    kAglTex_DefaultCount = 6,
};
void CreateDefaultTextures();
AglTexture* GetDefaultTexture(int index);

// Render target textures. The image memory is a new local surface; the
// texture gets an AglRenderSurface attachment so it can be rendered to.
void TextureCreateRenderTarget(AglTextureImage* image, uint16_t width, uint16_t height, uint32_t format,
                               AglSurface** outSurface);
AglTexture* TextureCreateColorTarget(uint16_t width, uint16_t height, uint32_t format, AglSurface* depth);
AglTexture* TextureCreateDepthTarget(uint16_t width, uint16_t height, uint32_t format);
// A texture sharing another texture's image, rendered to through 'depth'.
AglTexture* TextureCreateFromImage(const AglTextureImage& source, AglSurface* depth);

// Textures known by name hash (the level texture dictionary).
void RegisterTextureName(AglTexture* texture);
void UnregisterTextureName(AglTexture* texture);
AglTexture* FindTexture(uint32_t nameHash);

// Package buffer used by ApplyHeader (set while a level package loads).
void SetTexturePackBuffer(const uint8_t* buffer);

} // namespace agl
