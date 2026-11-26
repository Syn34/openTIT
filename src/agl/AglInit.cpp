// openTIT - Agl start-up: the graphics layer's part of SystemInit (flag 4).
#include "agl/AglInit.h"

#include "agl/AglDeferred.h"
#include "agl/AglDisplay.h"
#include "agl/AglDrawList.h"
#include "agl/AglOcclusion.h"
#include "agl/AglParticleProps.h"
#include "agl/AglPostProcShaders.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "agl/AglStencilShadow.h"
#include "agl/AglTexture.h"

#include "ape/core/ApeSystem.h"
#include "ape/file/ZipArchive.h"
#include "ape/sys/ApeIff.h"

#include <atomic>

namespace agl {

namespace {

using ape::iff::MakeId;
using ape::iff::RegisterChunkHandler;

AglPoolSizes g_poolSizes;

const AglTextureImage* g_displayImages[2] = {};   // 0x010F0E74, 0x010F0E84
AglTexture* g_displayTextures[2] = {};            // 0x00F67500, 0x00F67504

AglParticleGeometry* g_particleGeometry = nullptr;   // 0x010DDBBC
// Cleared when the particle system starts (0x010DDC00, written with an
// atomic store). NOTE: probably the particle job's completion counter.
std::atomic<uint32_t> g_particleJobCounter{0};

constexpr const char* kDefaultShaderArchive = "System\\Shaders.Zip";
constexpr const char* kShaderMountPrefix = "System\\Shaders\\";

// Engine configuration sizes: 0 selects the default, negative values none.
int32_t ConfiguredSize(int32_t value, int32_t fallback)
{
    if (value == 0)
        return fallback;
    return value > 0 ? value : 0;
}

// Particle quads: position, texture coordinate and colour (0x00F66EEC).
const AglVertexStream kParticleStreams[] = {
    { 0, static_cast<uint8_t>(gpu::VertexAttribType::Float), 3, 0, 0, false },
    { 8, static_cast<uint8_t>(gpu::VertexAttribType::Float), 2, 16, 0, false },
    { 3, static_cast<uint8_t>(gpu::VertexAttribType::UByteNorm), 4, 12, 0, false },
    { 0, 0, 0, 0, 0, false },
};
constexpr uint32_t kParticleStride = 24;

// TODO(0x0054A490 ShadowEdgeChunkHandler): stencil shadow edge data of a
// mesh. Skipped until the shadow volume builder is ported.
int ShadowEdgeChunkHandler(ape::iff::IffReader* /*reader*/, void* /*arg*/)
{
    return 1;
}

} // namespace

const AglPoolSizes& GetPoolSizes()
{
    return g_poolSizes;
}

AglParticleGeometry* GetParticleGeometry()
{
    return g_particleGeometry;
}

void SetDisplayImages(const AglTextureImage* mono, const AglTextureImage* stereo)
{
    g_displayImages[0] = mono;
    g_displayImages[1] = stereo;
}

AglTexture* GetDisplayTexture(bool stereo)
{
    return g_displayTextures[stereo ? 1 : 0];
}

// TODO(0x0053AC38 InitDisplay): video mode selection (resolution, 3D TV
// detection, gamma), display buffer allocation and the GPU context set-up.
// The PC device needs none of it; the display images stay unset.
bool InitDisplay()
{
    return true;
}

// @func 0x0051DF3C InitTextureSystem
// Texture pool, the name dictionary and the texture record chunks, then the
// built-in textures.
void InitTextureSystem()
{
    g_poolSizes.textures = ConfiguredSize(int16_t(ape::g_engineConfig.textureCapacity), 0x400);
    RegisterChunkHandler(MakeId('T', 'X', 'R', 'F'), TexturePathChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('T', 'E', 'X', 'R'), TextureChunkHandler, 0, nullptr, ape::iff::kChunk_Flag10);
    RegisterChunkHandler(MakeId('T', 'X', 'R', 'H'), TextureHeaderChunkHandler, 0, nullptr, ape::iff::kChunk_Flag10);
    RegisterChunkHandler(MakeId('T', 'I', 'N', 'C'), TextureFileChunkHandler, 0, nullptr, ape::iff::kChunk_Flag10);
    // Image formats this build cannot load use the default image (DXTN is
    // replaced by the real loader in RegisterTextureImageChunks).
    RegisterChunkHandler(MakeId('D', 'X', 'T', 'N'), TextureDefaultChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('T', 'D', 'D', 'S'), TextureDefaultChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('T', 'T', 'G', 'A'), TextureDefaultChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('T', 'B', 'M', 'P'), TextureDefaultChunkHandler, 0, nullptr, 0);
    CreateDefaultTextures();
    CreateDisplayTexture();
}

// @func 0x00542804 CreateDisplayTexture
// Wraps the display image of the current output mode in a texture.
void CreateDisplayTexture()
{
    const int mode = GetDisplaySettings().stereoOutput ? 1 : 0;
    if (!g_displayTextures[mode] && g_displayImages[mode])
        g_displayTextures[mode] = TextureCreateFromImage(*g_displayImages[mode], nullptr);
}

// @func 0x00542704 RegisterTextureImageChunks
void RegisterTextureImageChunks()
{
    InitTextureSystem();
    for (int mode = 0; mode < 2; ++mode) {
        if (!g_displayTextures[mode] && g_displayImages[mode])
            g_displayTextures[mode] = TextureCreateFromImage(*g_displayImages[mode], nullptr);
    }
    RegisterChunkHandler(MakeId('T', 'G', 'T', 'F'), TextureGtfChunkHandler, 0, nullptr,
                         ape::iff::kChunk_Flag10 | ape::iff::kChunk_QuietReplace);
    RegisterChunkHandler(MakeId('D', 'X', 'T', 'N'), TextureDdsChunkHandler, 0, nullptr, ape::iff::kChunk_QuietReplace);
    RegisterChunkHandler(MakeId('T', 'B', 'L', 'K'), TextureBlockChunkHandler, 0, nullptr, ape::iff::kChunk_QuietReplace);
}

// @func 0x0050F308 InitMaterialSystem
// Material pool and chunks, then everything that needs the shader archive:
// shader classes, the pass shaders and the built-in materials. The archive
// is unmounted again afterwards.
bool InitMaterialSystem()
{
    g_poolSizes.materials = ConfiguredSize(int16_t(ape::g_engineConfig.materialCapacity), 0x100);
    RegisterChunkHandler(MakeId('M', 'A', 'T', 'L'), MaterialChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'T', 'L', 'H'), MaterialHeaderChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'T', 'L', 'C'), MaterialClassChunkHandler, 0, nullptr, 0);

    const ape::EngineConfig& config = ape::g_engineConfig;
    int archive = 0;
    if (config.shaderArchiveStream) {
        archive = ape::file::MountZipStream(static_cast<ape::file::File*>(config.shaderArchiveStream),
                                            kShaderMountPrefix);
    } else {
        const char* path = config.shaderArchivePath ? config.shaderArchivePath : kDefaultShaderArchive;
        archive = ape::file::MountZipArchive(path, kShaderMountPrefix);
    }
    if (archive == 0)
        return false;

    RegisterShaderClasses();
    LoadResolveShaders();
    LoadCommonPostProcShaders();
    LoadStencilShadowShaders();
    InitShadowMapShaders();
    ape::file::UnmountZipArchive(archive);
    for (int i = 0; i < 6; ++i)
        GetBuiltinMaterial(i);
    return true;
}

// @func 0x00548BC0 RegisterShadowEdgeChunk
void RegisterShadowEdgeChunk()
{
    RegisterChunkHandler(MakeId('M', 'E', 'D', 'G'), ShadowEdgeChunkHandler, 0, nullptr, 0);
}

// @func 0x0050EF04 InitMeshSystem
// Topology pool and dictionary, the mesh container chunks and the raw
// memory block chunk.
void InitMeshSystem()
{
    g_poolSizes.topologies = ConfiguredSize(int16_t(ape::g_engineConfig.topologyCapacity), 0x400);
    RegisterChunkHandler(MakeId('M', 'S', 'H', 'R'), MeshIncludeChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'E', 'S', 'H'), MeshChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'S', 'H', 'H'), TopologyHeaderChunkHandler, 0, nullptr, 0);
    RegisterMemoryBlockChunk();
}

// @func 0x0050ECD4 InitDrawList
// The two debug vertex declarations and the game's draw list.
bool InitDrawList()
{
    GetDebugVertexDecl(false);
    GetDebugVertexDecl(true);
    const ape::EngineConfig& config = ape::g_engineConfig;
    const int32_t packetBytes = ConfiguredSize(int32_t(config.field_150), 0x20000);
    const int32_t vertices = ConfiguredSize(int32_t(config.field_14C), 0x6000);
    CreateMainDrawList(size_t(packetBytes), uint32_t(vertices));
    return true;
}

// @func 0x0050E614 InitParticleSystem
// Emitter property pool and chunks, and the particle quad buffers: room for
// 'n' vertices (n / 4 quads) and 3n / 2 indices per frame in flight.
void InitParticleSystem()
{
    g_poolSizes.emitterProps = ConfiguredSize(int16_t(ape::g_engineConfig.emitterPropsCapacity), 0x400);
    RegisterParticleChunkHandlers();

    const uint32_t vertices = uint32_t(ConfiguredSize(int32_t(ape::g_engineConfig.field_158), 0x5000));
    auto* geometry = new AglParticleGeometry();
    geometry->vertexCapacity = vertices;
    geometry->vertexBytes = vertices * kParticleStride;
    geometry->indexBytes = (vertices * 3) & ~1u;
    geometry->indexCount = vertices * 3 / 2;
    geometry->stride = kParticleStride;
    geometry->field_DC = geometry->vertexBytes;
    geometry->field_E0 = geometry->indexBytes;
    VertexDeclInit(&geometry->decl, kParticleStreams, kParticleStride);
    for (int buffer = 0; buffer < 2; ++buffer) {
        MemBlockAlloc(&geometry->vertices[buffer], geometry->vertexBytes, 0x80, gpu::MemoryLocation::Main, 2);
        MemBlockAlloc(&geometry->indices[buffer], geometry->indexBytes, 0x80, gpu::MemoryLocation::Main, 2);
    }
    g_particleGeometry = geometry;
    g_particleJobCounter.store(0);
}

// TODO(0x0050E410 InitRenderScratch): render thread scratch buffers
// (EngineConfig +0x144/+0x148), the render stack (0x010DD718), the view
// constant block set-up 0x0050D744, 0x00546EF4, a 0x50-byte block at
// 0x00F66FB8 and the stereo layout (0x0053C9F8). Console specific for the
// most part; the PC renderer allocates its own.
void InitRenderScratch()
{
}

// @func 0x0054BF24 InitStencilShadows
void InitStencilShadows()
{
    InitStencilShadowSystem();
}

// TODO(0x0050D2E0 InitOutlines): outline chunks ("OUTL", "OUTB", "OUTA",
// "OUTV", "OUTE", "OUTO", "OUTW", "OTLH") and the outline vertex buffers.
bool InitOutlines()
{
    return true;
}

// @func 0x0050F520 AglInit
bool AglInit()
{
    if (!InitDisplay())
        return false;

    AglDisplaySettings& display = GetDisplaySettings();
    display.aspect = display.widescreen ? 16.0f / 9.0f : 4.0f / 3.0f;
    if (display.maxLights == 0)
        display.maxLights = 20;

    const ape::EngineConfig& config = ape::g_engineConfig;
    static bool renderTargetPoolCreated = false;
    if (!renderTargetPoolCreated) {
        renderTargetPoolCreated = true;
        g_poolSizes.renderTargets = ConfiguredSize(int16_t(config.renderTargetCapacity), 0x400);
    }

    RegisterTextureImageChunks();
    if (!InitMaterialSystem())
        return false;
    RegisterShadowEdgeChunk();

    g_poolSizes.subMeshes = ConfiguredSize(int16_t(config.subMeshCapacity), 0x400);
    RegisterChunkHandler(MakeId('S', 'M', 'S', 'H'), SubMeshChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'V', 'T', 'X'), VertexDataChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'I', 'D', 'X'), SubMeshIndexChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('V', 'T', 'X', 'D'), VertexDescChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'P', 'A', 'L'), BoneTableChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('V', 'I', 'N', 'F'), SkinWeightChunkHandler, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('M', 'H', 'D', 'R'), SubMeshHeaderChunkHandler, 0, nullptr, 0);

    InitMeshSystem();
    InitDrawList();

    g_poolSizes.field_10DDC84 = ConfiguredSize(int16_t(config.field_18E), 0x80);
    g_poolSizes.field_F8F11C = 4;
    g_poolSizes.deferredObjects = 0x400;
    StartDeferredDestruction();

    InitParticleSystem();
    InitRenderScratch();
    InitStencilShadows();
    InitOutlines();
    InitOcclusionQueries();

    // NOTE: 0x010C5A72 is set here and 0x010C5A08/0x010C5A0C cleared; their
    // readers are not identified (0x010C5A08 is the deferred release list
    // of AglRenderThread.cpp, already empty at this point).
    return true;
}

} // namespace agl
