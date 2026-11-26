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
#include <vector>

namespace ape::iff { class IffReader; }

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
    // Slot 3: the submesh's name.
    // @func 0x0079ED80 AglSubMesh::GetName
    virtual const char* GetName() const { return name; }
    // Draws the geometry for a render item: uploads the item's bones (when
    // skinned), binds the vertex streams and the fragment program and issues
    // the draw (console vtable slot 4 of the platform submesh).
    virtual void Draw(const struct AglRenderItem* item);
    // Claims the next vertex stream slot (vertices + streamIndex). The
    // first pass named this Lock; the counter at 0x6D is the number of
    // vertex streams (the sphere/quad builders use it as the stream index).
    // @func 0x004FCE70 AglSubMesh::AddVertexStream
    uint8_t AddVertexStream() { return vertexStreamCount++; }
    // Total GPU memory of the buffers, in 16-byte units.
    uint32_t GetMemoryUnits() const;
    // Keeps at most 'maxInfluences' (1..3; 4 leaves the data alone) bones per
    // vertex, drops weights below 4 and renormalises the rest to 255.
    void LimitBoneInfluences(int maxInfluences);

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
    // 0x48: per-vertex skinning data, 8 bytes per vertex: four bone
    // indices then four weights (0..255, summing to 255).
    uint8_t* skinWeights = nullptr;
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
    AglTopology();
    ~AglTopology() override;
    static AglTopology* Create();
    static void Destroy(void* object);
    void Release() override;

    // One drawable piece: geometry and the material it is drawn with.
    struct MeshEntry {
        AglSubMesh* mesh;
        class AglMaterial* material;
    };

    // Names every submesh after the topology.
    void SetName(const char* topologyName);
    // Walks the submeshes' memory sizes (the console's report output was
    // compiled out, so nothing is returned).
    void QueryMemoryUnits() const;
    void SetFlags(bool flag80, bool flag40, bool flag20);
    // LOD distance scale (0 without LOD settings).
    float GetLodScale() const;
    // Appends a submesh drawn with 'material' (both reference counted) and
    // names the submesh after the topology.
    void AddMesh(AglSubMesh* mesh, AglMaterial* material);

    static constexpr int kInlineMeshes = 4;    // entries stored in the object itself
    static constexpr int kMeshBlock = 40;      // entries of the block used beyond that

    uint8_t field_8[0x10 - 8] = {};
    MeshEntry inlineMeshes[kInlineMeshes] = {};   // 0x10
    MeshEntry* meshes = nullptr;            // 0x30
    uint16_t meshCount = 0;                 // 0x34
    uint8_t flags = 0;                      // 0x36 (bits 5..7 set by SetFlags)
    float boundCenter[3] = {};              // 0x40
    float boundRadius = 0.0f;               // 0x4C
    uint32_t hash = 0;                      // 0x50
    const float* lod = nullptr;             // 0x58: LOD settings, scale at [4]
    const char* name = nullptr;             // 0x60
    uint8_t field_64[0x70 - 0x64] = {};
    AglTopology* lodOwner = nullptr;        // 0x54 NOTE: points at the topology itself after construction
    uint32_t field_5C = 0;                  // 0x5C
    // Storage of the entries beyond the inline ones (a pooled 40-entry
    // block on the console).
    std::vector<MeshEntry> meshBlock;
};

// The topology dictionary (0x010DDC80): topologies by name hash, filled as
// mesh records are loaded.
void RegisterTopology(AglTopology* topology);
AglTopology* FindTopology(uint32_t nameHash);
// Per-object light selection state of a render object ("AglLight", 0xF0
// bytes, pooled; the world module creates one per lit instance). Only the
// set-up fields are known.
struct AglLight {
    uint8_t field_00[0xD4] = {};
    uint8_t flags = 0;              // +0xD4 bit 7 cleared on creation
    // +0xD5: frame slot in which the object's lights are re-evaluated
    // (objects are spread over EngineConfig +0x99 frames).
    uint8_t updatePhase = 0;
    uint8_t field_D6 = 0;
    uint8_t field_D7 = 0;
    int32_t field_D8 = -1;
    uint8_t field_DC[0xF0 - 0xDC] = {};
};
AglLight* CreateLight();
void FreeLight(AglLight* light);

// The placeholder mesh used for missing meshes.
AglTopology* GetDefaultMesh();

class AglShaderClass;
// UV animation of a material (0x98 bytes): scroll in U, scroll in V and a
// rotation, each a channel whose value either advances on its own or is set
// by game code (the HUD meters drive their fill this way).
struct AglMaterialAnim {
    // One animated value (U offset, V offset or rotation in turns).
    struct Channel {                       // 0x30 bytes
        float speed = 0.0f;                // +0x00 units per second (period length for the sine)
        float step = 0.0f;                 // +0x04 value is quantised to 1/step when non-zero
        float range = 0.0f;                // +0x08 wrap length / ping-pong range / sine amplitude
        float waitMin = 0.0f;              // +0x0C random pause between bursts
        float waitMax = 0.0f;              // +0x10
        float activeTime = 0.0f;           // +0x14 length of a burst (0: always moving)
        float pivot[2] = {};               // +0x18 rotation centre (rotation channel)
        uint32_t flags = 0;                // +0x20 bits 12-19: phase, 20-27: motion, 28: sine
        float value = 0.0f;                // +0x24 current offset (turns for the rotation)
        float timer = 0.0f;                // +0x28 time left in the current phase
        float sineTime = 0.0f;             // +0x2C
        static constexpr uint32_t kPhaseShift = 12;
        static constexpr uint32_t kPhaseMask = 0xFFu << kPhaseShift;
        static constexpr uint32_t kModeShift = 20;
        static constexpr uint32_t kModeMask = 0xFFu << kModeShift;
        static constexpr uint32_t kFlag_Sine = 1u << 28;
        // Motion modes.
        static constexpr uint32_t kMode_Wrap = 0;       // value runs through [0, range) and wraps
        static constexpr uint32_t kMode_PingPong = 1;   // bounces between 0 and range (or a sine)
        static constexpr uint32_t kMode_Manual = 2;     // set by game code
        static constexpr uint32_t kMode_Off = 3;
        // Phases.
        static constexpr int kPhase_Waiting = 0;        // paused for a random time
        static constexpr int kPhase_Active = 1;         // moving (for activeTime when set)
        static constexpr int kPhase_Stopped = 2;
        uint32_t Mode() const { return (flags & kModeMask) >> kModeShift; }
        uint32_t Phase() const { return (flags & kPhaseMask) >> kPhaseShift; }
        // Stops the channel's own motion; game code sets `value` from now on.
        void SetManual()
        {
            speed = 0.0f;
            flags = (flags & ~kModeMask) | (kMode_Manual << kModeShift);
        }
        // Enters a phase: waiting picks a random pause in [waitMin, waitMax].
        void SetPhase(int phase);
        // Advances the channel by `dt` seconds.
        void Advance(float dt);
    };
    uint32_t frameStamp = 0;               // +0x00 render frame of the last update
    uint8_t wrapU = 0;                     // +0x04 U follows the fractional part of V (flip books)
    uint8_t field_05[3] = {};
    Channel u;                             // +0x08
    Channel v;                             // +0x38
    Channel rotation;                      // +0x68

    // Settings of one channel as stored in a material animation chunk.
    struct ChannelParams {
        float speed = 0.0f;       // 0 switches the channel off
        float steps = 0.0f;
        float limit = 0.0f;
        float delayMin = 0.0f;
        float delayMax = 0.0f;
        float playTime = 0.0f;
        float pivot[2] = {};      // rotation channel only
        uint8_t mode = 0;         // Channel::kMode_*
        uint8_t channel = 0;      // 0 U, 1 V, 2 rotation
        uint8_t flags = 0;
    };

    AglMaterialAnim();
    // Configures one channel. When both scrolls are stepped and V is the
    // slower one, U is driven by V instead (flip book frames in rows).
    void Configure(const ChannelParams& params);
    void CopyFrom(const AglMaterialAnim& source);
    // Advances the three channels once per rendered frame.
    void Update();
    // The quantised U and V offsets.
    void GetOffset(float out[2]) const;
};
static_assert(sizeof(AglMaterialAnim) == 0x98, "AglMaterialAnim is 0x98 bytes");

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
    // Slot 3: the material goes by the name of its first texture (null
    // without one).
    virtual const char* GetName() const;
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
    // Fills the material from a material header (texture, colours, blend,
    // cull, wrap, depth and sort settings) and lets the shader class named
    // there set it up, falling back to the default class.
    void Setup(struct AglMaterialDesc* desc);

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
    uint32_t nameHash = 0;                 // 0x28 (FindGeomMaterial looks materials up by it)
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
// A live material (still referenced) with that name hash, or null.
AglMaterial* FindLiveMaterial(uint32_t nameHash);

// A new unlit material for sprites and screen quads: textured with 'texture',
// or vertex colored when 'texture' is null. Blend state is left at the
// material defaults.
AglMaterial* CreateSpriteMaterial(AglTexture* texture);

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

// Chunk handlers of raw RSX data blocks ("TBLK" textures, "MBLK") and the
// MBLK registration.
int TextureBlockChunkHandler(ape::iff::IffReader* reader, void* arg);
int MemoryBlockChunkHandler(ape::iff::IffReader* reader, void* arg);
void RegisterMemoryBlockChunk();

// Mesh file chunk handlers (AglLoaders.cpp), registered by the Agl start-up.
int SubMeshChunkHandler(ape::iff::IffReader* reader, void* arg);         // "SMSH"
int SubMeshHeaderChunkHandler(ape::iff::IffReader* reader, void* arg);   // "MHDR"
int VertexDataChunkHandler(ape::iff::IffReader* reader, void* arg);      // "MVTX"
int SubMeshIndexChunkHandler(ape::iff::IffReader* reader, void* arg);    // "MIDX"
int VertexDescChunkHandler(ape::iff::IffReader* reader, void* arg);      // "VTXD"
int BoneTableChunkHandler(ape::iff::IffReader* reader, void* arg);       // "MPAL"
int SkinWeightChunkHandler(ape::iff::IffReader* reader, void* arg);      // "VINF"
int MeshIncludeChunkHandler(ape::iff::IffReader* reader, void* arg);     // "MSHR"
int MeshChunkHandler(ape::iff::IffReader* reader, void* arg);            // "MESH"
int TopologyHeaderChunkHandler(ape::iff::IffReader* reader, void* arg);  // "MSHH"
// Material chunk handlers (AglMaterial.cpp).
int MaterialChunkHandler(ape::iff::IffReader* reader, void* arg);        // "MATL"
int MaterialHeaderChunkHandler(ape::iff::IffReader* reader, void* arg);  // "MTLH"
int MaterialClassChunkHandler(ape::iff::IffReader* reader, void* arg);   // "MTLC"

} // namespace agl
