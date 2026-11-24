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
#include "agl/AglTypes.h"
#include "agl/gpu/GpuTypes.h"

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
// A drawable piece of a mesh: vertex and index buffers, the primitive
// type, an optional bone table for skinning and a bounding sphere.
class AglSubMesh : public AglResource {
public:
    AglSubMesh();
    ~AglSubMesh() override;

    static AglSubMesh* Create();
    static void Destroy(void* object);
    void Release() override;
    // Claims the next vertex stream slot (vertices + streamIndex). The
    // first pass named this Lock; the counter at 0x6D is the number of
    // vertex streams (the sphere/quad builders use it as the stream index).
    // @func 0x004FCE70 AglSubMesh::AddVertexStream
    uint8_t AddVertexStream() { return vertexStreamCount++; }
    // Total GPU memory of the buffers, in 16-byte units.
    uint32_t GetMemoryUnits() const;

    uint16_t resourceFlags = 0;
    uint32_t id = 0;
    uint32_t field_C = 0;
    AglResource* owner = nullptr;           // 0x10: material, released with the submesh
    const char* name = "SubMeshName:Not set";
    AglMemBlock vertices;                   // 0x18
    uint32_t vertexCount = 0;               // 0x28
    AglMemBlock indices;                    // 0x2C
    uint32_t field_3C = 1;                  // index format (1: 16-bit)
    uint32_t indexCount = 0;                // 0x40
    const struct AglVertexDecl* vertexDecl = nullptr;   // 0x44
    void* field_48 = nullptr;               // engine heap block owned by the submesh
    uint32_t primitive = 6;                 // gpu::PrimitiveType (triangle strips by default)
    uint32_t serial = 0;                    // creation order
    void* skinMatrices = nullptr;           // 0x54: bone table, null when not skinned
    float field_58 = 1.0f;
    float boundCenter[3] = {};              // 0x5C
    float boundRadius = 0.0f;               // 0x68
    uint8_t field_6C = 0;
    uint8_t vertexStreamCount = 0;          // 0x6D
    void* platformData = nullptr;           // 0x70: GPU side buffers (platform subclass)
};

class AglTopology : public AglResource {
public:
    static AglTopology* Create();
    static void Destroy(void* object);
    void Release() override;

    // One drawable piece: geometry and the material slot it uses.
    struct MeshEntry {
        AglSubMesh* mesh;
        uint32_t material;
    };

    // Names every submesh after the topology.
    void SetName(const char* topologyName);
    // Walks the submeshes' memory sizes (the console's report output was
    // compiled out, so nothing is returned).
    void QueryMemoryUnits() const;
    void SetFlags(bool flag80, bool flag40, bool flag20);
    // LOD distance scale (0 without LOD settings).
    float GetLodScale() const;

    uint8_t field_8[0x30 - 8] = {};
    MeshEntry* meshes = nullptr;            // 0x30
    uint16_t meshCount = 0;                 // 0x34
    uint8_t flags = 0;                      // 0x36 (bits 5..7 set by SetFlags)
    float boundCenter[3] = {};              // 0x40
    float boundRadius = 0.0f;               // 0x4C
    uint32_t hash = 0;                      // 0x50
    const float* lod = nullptr;             // 0x58: LOD settings, scale at [4]
    const char* name = nullptr;             // 0x60
    uint8_t field_64[0x70 - 0x64] = {};
};

class AglShaderClass;
struct AglMaterialAnim;

constexpr int kMaxMaterialTextures = 5;

// Surface description used when drawing: the shader class that draws it,
// up to five textures, colors, the blend/depth/cull state and a block of
// per-material float constants the shader class fills in.
class AglMaterial : public AglResource {
public:
    AglMaterial();
    ~AglMaterial() override;

    static AglMaterial* Create();
    static void Destroy(void* object);
    void Release() override;
    // A new material drawn by 'shaderClass'.
    static AglMaterial* CreateWithClass(AglShaderClass* shaderClass);
    // A new material drawn by the shader class registered under 'classHash'
    // (null if there is no such class).
    static AglMaterial* CreateForClassHash(uint32_t classHash);

    // Replaces texture 'index' (reference counted).
    void SetTexture(AglTexture* texture, int index);
    // Appends a texture layer.
    void AddTexture(AglTexture* texture);
    // Address of float constant 'index', growing the constant block as needed.
    float* GetConstant(uint32_t index);
    // Copies textures, colors, render state, constants and animation of
    // 'source' (the shader class is not copied).
    void CopyFrom(const AglMaterial& source);
    // A new material with the same class and settings.
    static AglMaterial* Clone(const AglMaterial& source);

    enum BlendMode {
        kBlend_None = 0,         // opaque (or an unrecognised combination)
        kBlend_Alpha = 2,        // src alpha, 1 - src alpha
        kBlend_Additive = 3,     // src alpha, one
        kBlend_Subtractive = 4,  // src alpha, one, reverse subtract
    };
    BlendMode GetBlendMode() const;

    enum Flags : uint8_t {
        kFlag_08  = 0x08,
        kFlag_10  = 0x10,       // NOTE: selects the "_Lit" variant of some classes
        kFlag_20  = 0x20,
        kFlag_40  = 0x40,
        kFlag_Lit = 0x80,       // receives dynamic lights
    };

    uint16_t resourceFlags = 0;
    uint32_t id = 0;
    uint32_t field_C = 0;
    AglShaderClass* shaderClass = nullptr;
    AglTexture* textures[kMaxMaterialTextures] = {};
    uint32_t field_28 = 0;
    uint8_t textureCount = 0;              // texture layers in use
    uint8_t field_2D = 0;
    uint16_t sortKey = 0;                  // draw order key (creation order by default)
    uint16_t textureSortKey = 0;           // serial of texture 0
    float color0[4] = {};                  // 0x40
    float color1[4] = {};                  // 0x50
    float field_60 = 1.0f;
    uint8_t flags = kFlag_20 | kFlag_40 | kFlag_Lit;
    uint8_t field_65 = 0;
    // Blend state (word 0x68 and the two halves at 0x6C).
    bool blendEnable = false;
    gpu::BlendFactor blendSrc = gpu::BlendFactor::SrcAlpha;
    uint16_t field_68 = 0;                 // low bits of the blend word, not decoded
    gpu::BlendEquation blendEquation = gpu::BlendEquation::Add;
    gpu::BlendFactor blendDst = gpu::BlendFactor::OneMinusSrcAlpha;
    // Depth state (word 0x70).
    bool depthTest = true;
    gpu::CompareFunc depthFunc = gpu::CompareFunc::LEqual;
    uint16_t field_70 = 0;                 // low bits of the depth word, not decoded
    // Front face that is culled away; 0 disables culling.
    uint16_t cullFrontFace = static_cast<uint16_t>(gpu::FrontFace::CW);
    uint8_t field_78 = 2;                  // NOTE: top nibble of word 0x78, meaning unknown
    uint8_t constantCapacity = 0;          // floats allocated in 'constants'
    float* constants = nullptr;
    AglMaterialAnim* anim = nullptr;       // UV animation (selects the "_UVX" variants)
    uint32_t field_84[3] = {};
};

// Shared materials of the debug/primitive drawing code (0: vertex color,
// 1: vertex color double sided, 2 and 3: other primitive classes, 4 and 5:
// textured with default texture 0 or 1). All alpha blended; created on
// first use.
AglMaterial* GetBuiltinMaterial(int index);

// A set of render targets (color and/or depth surface) with its clear
// settings, optionally exposed as a texture. Not to be confused with
// AglSurface, the image itself.
class AglRenderSurface : public AglResource {
public:
    AglRenderSurface();
    ~AglRenderSurface() override;

    static AglRenderSurface* Create();
    // Full-surface viewport and scissor for callers that pass none (each
    // pointer is set to a shared rectangle when null).
    void GetDefaultRects(const uint16_t** viewport, const uint16_t** scissor) const;
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

} // namespace agl
