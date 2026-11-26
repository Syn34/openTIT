// openTIT - Agl start-up: the graphics layer's part of SystemInit (flag 4).
#pragma once

#include "agl/AglPrimitives.h"
#include "agl/AglTypes.h"

#include <cstdint>

namespace agl {

// Brings up the graphics layer: display, texture/material/mesh systems,
// shaders, draw list, particle and outline systems, deferred destruction
// and occlusion queries. Returns false when a required step fails.
bool AglInit();

// The steps, in the order AglInit runs them.
bool InitDisplay();
void InitTextureSystem();
void RegisterTextureImageChunks();
void CreateDisplayTexture();
bool InitMaterialSystem();
void RegisterShadowEdgeChunk();
void InitMeshSystem();
bool InitDrawList();
void InitParticleSystem();
void InitRenderScratch();
void InitStencilShadows();
bool InitOutlines();

// Pool sizes the start-up read from the engine configuration. The PC
// resource classes allocate from the heap, so these are informational.
struct AglPoolSizes {
    int32_t renderTargets = 0;    // 0x00F8F110
    int32_t materials = 0;        // 0x00F8F114
    int32_t textures = 0;         // 0x00F8F118
    int32_t field_F8F11C = 0;     // 0x00F8F11C (fixed 4)
    int32_t subMeshes = 0;        // 0x00F8F120
    int32_t topologies = 0;       // 0x00F8F124
    int32_t emitterProps = 0;     // 0x00F8F128
    int32_t field_10DDC84 = 0;    // 0x010DDC84
    int32_t deferredObjects = 0;  // 0x010DDC88 (fixed 0x400)
};
const AglPoolSizes& GetPoolSizes();

// The display images (0x010F0E74 mono, 0x010F0E84 frame packed stereo) that
// the display set-up provides and CreateDisplayTexture wraps in textures.
struct AglTextureImage;
class AglTexture;
void SetDisplayImages(const AglTextureImage* mono, const AglTextureImage* stereo);
AglTexture* GetDisplayTexture(bool stereo);

// Double buffered particle quad geometry (0x280-byte console object at
// 0x010DDBBC): vertices of 24 bytes (position, colour, texture coordinate)
// and 16-bit indices, one buffer pair per frame in flight.
struct AglParticleGeometry {
    AglVertexDecl decl;                 // +0x00
    AglMemBlock vertices[2];            // +0x80
    AglMemBlock indices[2];             // +0xA0
    uint32_t vertexBytes = 0;           // +0xC8
    uint32_t indexBytes = 0;            // +0xCC
    uint32_t vertexCapacity = 0;        // +0xD0
    uint32_t indexCount = 0;            // +0xD4
    uint32_t stride = 0;                // +0xD8
    uint32_t field_DC = 0;              // +0xDC copy of the vertex byte size
    uint32_t field_E0 = 0;              // +0xE0 copy of the index byte size
    uint32_t used[7] = {};              // +0xE4..+0xFC NOTE: per-frame fill counters (guess)
    uint32_t field_100 = 1;             // +0x100
};
AglParticleGeometry* GetParticleGeometry();

} // namespace agl
