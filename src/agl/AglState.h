// openTIT - Agl render context: the command list being recorded and the GPU state shadow.
//
// Agl records one frame at a time into a command list and keeps a shadow of
// the state it has set (render targets, bound textures, current fragment
// program, vertex stream layout, stencil state) so redundant changes can be
// skipped and deferred state (fragment program, vertex streams) can be
// committed right before a draw.
#pragma once

#include "agl/AglTypes.h"
#include "agl/gpu/CommandList.h"

#include <cstdint>

namespace agl {

struct AglFragmentProgram;

// One attribute stream of the current vertex layout.
struct AglVertexStream {
    uint32_t attribute = 0;      // attribute register
    uint8_t type = 0;            // gpu::VertexAttribType
    uint8_t size = 0;            // components
    uint8_t offset = 0;          // byte offset inside a vertex
    uint8_t attributeOffset = 0; // added to 'attribute' (multi-register attributes)
    bool skip = false;           // stream supplied by other means
};

struct AglStencilState {
    gpu::CompareFunc func = gpu::CompareFunc::Always;
    int32_t ref = 0;
    uint32_t mask = 0xFFFFFFFF;
    uint32_t writeMask = 0xFFFFFFFF;
    gpu::StencilOp fail = gpu::StencilOp::Keep;
    gpu::StencilOp depthFail = gpu::StencilOp::Keep;
    gpu::StencilOp depthPass = gpu::StencilOp::Keep;
};

constexpr int kMaxVertexStreams = 16;
constexpr int kMaxBoundTextures = 8;

struct AglRenderState {
    gpu::CommandList* cmd = nullptr;                 // list being recorded

    // Vertex input.
    AglVertexStream streams[kMaxVertexStreams];
    uint32_t streamCount = 0;
    uint16_t vertexStride = 0;
    uint16_t attributeMask = 0;                      // attributes the layout provides
    uint16_t enabledAttributes = 0xFFFF;             // attributes enabled on the GPU
    bool vertexLayoutDirty = false;
    gpu::GpuPtr vertexBuffer;

    // Programs.
    AglFragmentProgram* fragmentProgram = nullptr;
    bool fragmentProgramDirty = false;
    uint32_t frame = 0;                              // patch buffer generation

    // Render targets.
    const AglSurface* colorTargets[gpu::kMaxColorTargets] = {};
    const AglSurface* depthTarget = nullptr;
    bool depthEnabled = false;                       // a depth target is bound
    gpu::SurfaceDesc surface;

    // Textures and samplers.
    const AglTextureImage* textures[kMaxBoundTextures] = {};
    uint8_t samplerMinFilter[2] = {};
    uint8_t samplerMagFilter[2] = {};
    int32_t field_1178 = -1;

    AglStencilState stencilFront;
    AglStencilState stencilBack;

    // Draw state caches of the renderer loop (0x010F10A4..0x010F10C0, 0x010F10F8).
    uint32_t field_10A4 = 0;
    const void* boundVertexBuffer = nullptr;         // 0x010F10A8: vertex memory of the last mesh draw
    const struct AglVertexDecl* boundVertexDecl = nullptr; // 0x010F10B0
    const struct AglVertexProgram* vertexProgram = nullptr; // 0x010F10B4: program of the bound technique
    const void* vertexProgramBlock = nullptr;        // 0x010F10B8: its pre-recorded program block
    const void* material = nullptr;                  // 0x010F10BC
    uint16_t meshAttributes = 0xFFFF;                // 0x010F10C0: attributes enabled by mesh draws
    uint32_t field_10F8 = 0;
    bool materialDirty = false;                      // 0x010F10FC bit 31
};

AglRenderState& GetRenderState();
gpu::CommandList& Cmd();

// Resets the state shadow and the GPU to Agl's defaults (start of frame).
void ResetRenderState();
// Forgets every cached state (after code that drove the GPU directly), so
// the next draw sets everything again.
void InvalidateRenderState();
// Particles that only draw where stencil bit 0x20 is set (masked emitters).
void ApplyParticleStencilMask(bool masked);

// Binds the color and depth surfaces of a render surface (the back buffer
// picks the buffers of the current eye) and optionally clears them.
void BindRenderSurface(const class AglRenderSurface* surface, bool clear);
// Binds render targets. 'color' goes to target 'index'; the depth target is
// shared. Null color uses the depth target's size for the viewport.
void SetRenderTargets(const AglSurface* depth, const AglSurface* color, int index);
void SetColorTarget(const AglSurface* color, int index);
void SetDepthTarget(const AglSurface* depth);
// Viewport and scissor for a rectangle of the current target.
void SetViewportRect(const uint16_t rect[4]);
// Restores the default depth/cull state for 3D drawing.
void SetDefault3DState();
// Prepares a full-screen 2D pass: no depth, no culling, no alpha test, and
// vertex constant register 'clearConstant' set to zero.
void Begin2DPass(bool disableBlend, uint32_t clearConstant);
// Binds 'texture' to 'unit' with clamp addressing and the filter of sampler 'sampler'.
void BindTexture(const AglTextureImage* texture, uint32_t unit, int sampler);
// Clears the bound targets.
void ClearTargets(float depth, uint32_t mask, uint32_t color, uint32_t stencil, uint32_t stencilMask);
// Commits deferred program and vertex stream state before a draw.
void CommitDrawState();
// The fragment program part of it: binds the current fragment program when
// it changed and uploads its changed constants (also run by mesh draws).
void CommitFragmentProgram();
// Finds the stream feeding 'attribute' (+ 'attributeOffset') in a layout
// table terminated by an entry with size 0.
const AglVertexStream* FindVertexStream(const AglVertexStream* streams, uint32_t attribute,
                                        uint32_t attributeOffset);

// Entry points of the renderer interface. The console exported these as
// thin wrappers around the functions above.
void ApiBindTexture(const AglTextureImage* texture, uint32_t unit, int sampler);
void ApiBegin2DPass(bool disableBlend, uint32_t clearConstant);
void ApiSetDefault3DState();
void ApiCommitDrawState();

} // namespace agl
