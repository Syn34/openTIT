// openTIT - Pooled, reference counted Agl resource objects (meshes, materials, targets).
//
// Every resource type lives in its own pool (ApeTListAllocator<T>). Releasing
// the last reference queues the object for deferred destruction
// (AglDeferred.h); destruction runs the destructor and returns the storage
// to the pool.
//
// NOTE: only the lifetime management of these classes is reimplemented so
// far; their contents are placeholders sized like the console objects.
#pragma once

#include "agl/AglDeferred.h"

#include <cstddef>
#include <cstdint>

namespace agl {

struct AglSurface;
class AglTexture;

class AglResource : public AglRefCounted {
public:
    AglResource() = default;
    virtual ~AglResource() = default;
    // Drops a reference; the last one schedules deferred destruction.
    virtual void Release() = 0;

    AglResource(const AglResource&) = delete;
    AglResource& operator=(const AglResource&) = delete;
};

// Console object sizes (bytes), used to size the placeholder payloads.
class AglSubMesh : public AglResource {
public:
    static AglSubMesh* Create();
    static void Destroy(void* object);
    void Release() override;

    uint8_t field_8[0x74 - 8] = {};
};

class AglTopology : public AglResource {
public:
    static AglTopology* Create();
    static void Destroy(void* object);
    void Release() override;

    uint8_t field_8[0x70 - 8] = {};
};

class AglMaterial : public AglResource {
public:
    static AglMaterial* Create();
    static void Destroy(void* object);
    void Release() override;

    uint8_t field_8[0x90 - 8] = {};
};

// A set of render targets (color and/or depth surface) with its clear
// settings, optionally exposed as a texture. Not to be confused with
// AglSurface, the image itself.
class AglRenderSurface : public AglResource {
public:
    AglRenderSurface();

    static AglRenderSurface* Create();
    static void Destroy(void* object);
    void Release() override;
    // Wraps 'texture' (and the given surfaces) in a new render surface.
    static AglRenderSurface* CreateForTexture(AglSurface* color, AglSurface* depth, AglTexture* texture);

    enum Flags : uint32_t {
        kFlag_HasColor = 1u << 26,
        kFlag_HasDepth = 1u << 27,
        kFlag_Texture  = 1u << 28,
    };

    uint16_t resourceFlags = 0;
    uint32_t id = 0;
    uint32_t field_C = 0;
    AglSurface* color = nullptr;
    AglSurface* color2 = nullptr;
    AglSurface* depth = nullptr;
    AglTexture* texture = nullptr;
    uint8_t clearColor[4] = { 0x40, 0x40, 0x80, 0x00 };
    float clearDepth = 1.0f;
    uint8_t clearStencil = 0;
    uint32_t clearMask = 0;              // gpu::ClearMask
    uint32_t colorMask = 0x01010101;
    uint32_t flags = 0x400000;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t field_40[18] = {};
    uint8_t field_88 = 0;
    uint8_t field_89 = 1;
    uint8_t field_8A = 0;
    uint32_t field_8C = 0;
    uint32_t field_90 = 0;
    int8_t field_94 = 0;                 // sign bit tested by the display code
    uint32_t field_98 = 0;
};

class AglRenderTarget : public AglResource {
public:
    static AglRenderTarget* Create();
    static void Destroy(void* object);
    void Release() override;

    uint8_t field_8[0x730 - 8] = {};
};

} // namespace agl
