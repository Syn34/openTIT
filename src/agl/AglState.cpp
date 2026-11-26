// openTIT - Agl render context: the command list being recorded and the GPU state shadow.
#include "agl/AglState.h"

#include "agl/AglDisplay.h"
#include "agl/AglDrawList.h"
#include "agl/AglPrimitives.h"
#include "agl/AglResources.h"
#include "agl/AglShader.h"
#include "agl/AglRenderTarget.h"

#include "ape/math/ApeMath.h"

namespace agl {

namespace {

AglRenderState g_state;          // 0x010F1020..0x010F1180
gpu::CommandList g_fallbackList{"AglDefault"};

void SetFullTargetViewport(uint16_t width, uint16_t height)
{
    // The console used a [0, 1] depth range with the viewport transform
    // computed by libgcm for the whole target.
    const float scale[4] = { width * 0.5f, height * -0.5f, 0.5f, 0.0f };
    const float offset[4] = { width * 0.5f, height * 0.5f, 0.5f, 0.0f };
    Cmd().SetViewport(0, 0, width, height, 0.0f, 1.0f, scale, offset);
    Cmd().SetScissor(0, 0, width, height);
}

void ApplyDepthEnable()
{
    // Depth testing and writing follow the presence of a depth target.
    Cmd().SetDepthTestEnable(g_state.depthEnabled);
    Cmd().SetDepthMask(g_state.depthEnabled);
}

void FillColorTarget(int index, const AglSurface* color)
{
    gpu::SurfaceDesc& s = g_state.surface;
    s.color[index] = color->Ptr();
    s.colorPitch[index] = color->pitch;
    s.colorFormat = static_cast<gpu::SurfaceColorFormat>(color->surfaceFormat);
    s.antialias = static_cast<gpu::SurfaceAntialias>(color->antialias);
    s.swizzled = color->type == kAglSurface_Swizzle;
    s.colorTargets = gpu::kTarget_0;
    s.width = color->width;
    s.height = color->height;
    s.x = color->x;
    s.y = color->y;
}

void FillDepthTarget(const AglSurface* depth)
{
    gpu::SurfaceDesc& s = g_state.surface;
    if (depth) {
        s.depthFormat = static_cast<gpu::SurfaceDepthFormat>(depth->surfaceFormat);
        s.depth = depth->Ptr();
        s.depthPitch = depth->pitch;
    } else {
        s.depthFormat = gpu::SurfaceDepthFormat::Z24S8;
        s.depth = gpu::GpuPtr{};
        s.depthPitch = 0x40;
    }
}

} // namespace

AglRenderState& GetRenderState()
{
    return g_state;
}

gpu::CommandList& Cmd()
{
    return g_state.cmd ? *g_state.cmd : g_fallbackList;
}

// @func 0x0053170C AglRenderState::Reset
void ResetRenderState()
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetFrontFace(gpu::FrontFace::CW);
    cmd.SetCullFace(gpu::CullFace::Back);
    cmd.SetCullFaceEnable(true);
    cmd.SetDepthFunc(gpu::CompareFunc::LEqual);
    cmd.SetDepthTestEnable(true);
    cmd.SetRestartIndex(0xFFFF);
    cmd.SetRestartIndexEnable(true);
    cmd.SetColorMask(gpu::kColorMask_All);
    // Then everything off: passes enable what they need.
    cmd.SetCullFaceEnable(false);
    cmd.SetDepthTestEnable(false);
    cmd.SetAlphaTestEnable(false);
    cmd.SetBlendEnable(false);
    // NOTE: the console also wrote an unidentified state method here
    // (0x1E98 with value 0x01000000); it has no equivalent in the abstraction.

    g_state.vertexLayoutDirty = true;
    g_state.streamCount = 0;
    g_state.vertexStride = 0;
    g_state.fragmentProgram = nullptr;
    g_state.enabledAttributes = 0xFFFF;
    g_state.fragmentProgramDirty = false;
    for (auto& texture : g_state.textures)
        texture = nullptr;
    g_state.fogLayer = -1;
    g_state.samplerMinFilter[0] = g_state.samplerMinFilter[1] = 0;

    g_state.stencilFront = AglStencilState();
    g_state.stencilBack = AglStencilState();
    for (auto& target : g_state.colorTargets)
        target = nullptr;
    g_state.depthTarget = nullptr;

    // Default surface: one 64x64 A8R8G8B8 target, no depth.
    gpu::SurfaceDesc& s = g_state.surface;
    s = gpu::SurfaceDesc();
    s.colorFormat = gpu::SurfaceColorFormat::A8R8G8B8;
    s.colorTargets = gpu::kTarget_0;
    for (int i = 0; i < gpu::kMaxColorTargets; ++i)
        s.colorPitch[i] = 0x40;
    s.depthFormat = gpu::SurfaceDepthFormat::Z24S8;
    s.depthPitch = 0x40;
    s.width = 0x40;
    s.height = 0x40;
}

// @func 0x00532090 AglRenderState::SetRenderTargets
void SetRenderTargets(const AglSurface* depth, const AglSurface* color, int index)
{
    if (g_state.colorTargets[index] == color && g_state.depthTarget == depth)
        return;
    g_state.colorTargets[index] = color;

    gpu::SurfaceDesc& s = g_state.surface;
    if (color) {
        FillColorTarget(index, color);
    } else {
        // Depth-only rendering.
        if (index == 0)
            s.colorTargets = gpu::kTarget_None;
        s.color[index] = gpu::GpuPtr{};
        s.colorPitch[index] = 0x40;
        if (depth) {
            s.width = depth->width;
            s.height = depth->height;
            s.x = depth->x;
            s.y = depth->y;
        }
    }
    FillDepthTarget(depth);
    g_state.depthTarget = depth;
    Cmd().SetSurface(s);

    const AglSurface* sized = color ? color : depth;
    if (sized)
        SetFullTargetViewport(sized->width, sized->height);

    g_state.depthEnabled = depth != nullptr;
    ApplyDepthEnable();
}

// @func 0x005373A8 AglRenderState::SetColorTarget
void SetColorTarget(const AglSurface* color, int index)
{
    if (g_state.colorTargets[index] == color)
        return;
    g_state.colorTargets[index] = color;
    if (!color)
        return;
    FillColorTarget(index, color);
    Cmd().SetSurface(g_state.surface);
    SetFullTargetViewport(color->width, color->height);
}

// @func 0x0053755C AglRenderState::SetDepthTarget
void SetDepthTarget(const AglSurface* depth)
{
    if (g_state.depthTarget == depth)
        return;
    FillDepthTarget(depth);
    g_state.depthTarget = depth;
    Cmd().SetSurface(g_state.surface);
    g_state.depthEnabled = depth != nullptr;
    ApplyDepthEnable();
}

// @func 0x00532450 AglRenderState::SetViewportRect
void SetViewportRect(const uint16_t rect[4])
{
    const uint16_t x = rect[0];
    const uint16_t y = rect[1];
    const uint16_t w = rect[2];
    const uint16_t h = rect[3];
    const float scale[4] = { w * 0.5f, h * -0.5f, 0.5f, 0.0f };
    const float offset[4] = { x + w * 0.5f, y + h * 0.5f, 0.5f, 0.0f };
    Cmd().SetViewport(x, y, w, h, 0.0f, 1.0f, scale, offset);
    Cmd().SetScissor(x, y, w, h);
}

// @func 0x00535294 AglRenderState::SetDefault3DState
void SetDefault3DState()
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetDepthTestEnable(g_state.depthEnabled);
    cmd.SetDepthFunc(gpu::CompareFunc::LEqual);
    cmd.SetDepthMask(g_state.depthEnabled);
    cmd.SetCullFaceEnable(true);
    cmd.SetFrontFace(gpu::FrontFace::CCW);
}

// @func 0x005355D8 AglRenderState::Begin2DPass
void Begin2DPass(bool disableBlend, uint32_t clearConstant)
{
    gpu::CommandList& cmd = Cmd();
    if (disableBlend)
        cmd.SetBlendEnable(false);
    cmd.SetDepthTestEnable(false);
    cmd.SetDepthMask(false);
    cmd.SetAlphaTestEnable(false);
    cmd.SetCullFaceEnable(false);
    // The pass's vertex constant 'clearConstant' starts out as zero.
    const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmd.SetVertexProgramConstants(clearConstant, 4, zero);
}

// @func 0x00536AC0 AglRenderState::BindTexture
void BindTexture(const AglTextureImage* texture, uint32_t unit, int sampler)
{
    gpu::CommandList& cmd = Cmd();
    const uint8_t u = static_cast<uint8_t>(unit);
    if (!texture) {
        g_state.textures[unit] = nullptr;
        cmd.SetTextureControl(u, false, 0, 0, 0);
        return;
    }
    if (g_state.textures[unit] != texture) {
        g_state.textures[unit] = texture;
        cmd.SetTexture(u, texture->desc);
        // All mip levels: max LOD is (levels - 1) in 4.8 fixed point.
        cmd.SetTextureControl(u, true, 0, static_cast<uint16_t>((texture->desc.mipmaps - 1) * 0x100), 0);
    }
    cmd.SetTextureAddress(u, gpu::TextureWrap::ClampToEdge, gpu::TextureWrap::ClampToEdge,
                          gpu::TextureWrap::ClampToEdge, 0, 0, 0);
    const uint8_t minFilter = g_state.samplerMinFilter[sampler & 1];
    const uint8_t magFilter = g_state.samplerMagFilter[sampler & 1];
    cmd.SetTextureFilter(u, 0,
                         minFilter ? static_cast<gpu::TextureMinFilter>(minFilter) : gpu::TextureMinFilter::Linear,
                         magFilter ? static_cast<gpu::TextureMagFilter>(magFilter) : gpu::TextureMagFilter::Linear,
                         1);
}

// @func 0x00531F80 AglRenderState::Clear
void ClearTargets(float depth, uint32_t mask, uint32_t color, uint32_t stencil, uint32_t stencilMask)
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetClearColor(color);
    // 24-bit depth; exactly 1.0 maps to the maximum value.
    const uint32_t depth24 = depth != 1.0f ? static_cast<uint32_t>(depth * 16777215.0f) : 0xFFFFFFu;
    cmd.SetClearDepthStencil((depth24 << 8) | (stencil & 0xFF));
    cmd.SetStencilMask(stencilMask);
    cmd.ClearSurface(mask);
    cmd.SetStencilMask(g_state.stencilFront.writeMask);
}

void CommitFragmentProgram()
{
    AglFragmentProgram* fp = g_state.fragmentProgram;
    if (g_state.fragmentProgramDirty) {
        g_state.fragmentProgramDirty = false;
        if (fp) {
            Cmd().SetFragmentProgram(fp->desc);
            UploadFragmentConstants(fp);
        }
    } else if (fp && fp->dirtyMask != 0) {
        UploadFragmentConstants(fp);
    }
}

// NOTE: the vertex array binding shared by mesh and draw list draws
// (0x0079E0C0 on the console, outside this range): the streams of 'decl'
// that the bound vertex program reads are pointed at 'base'; attributes the
// previous binding enabled and this one does not use are disabled.
void BindVertexArrays(const AglVertexDecl* decl, gpu::GpuPtr base)
{
    AglRenderState& state = GetRenderState();
    if (state.boundVertexDecl != decl)
        state.boundVertexBuffer = nullptr;
    state.boundVertexDecl = decl;
    if (!decl || state.boundVertexBuffer == base.data)
        return;
    gpu::CommandList& cmd = Cmd();
    state.boundVertexBuffer = base.data;
    // Only the attributes both the layout and the bound program use.
    const uint16_t programInputs = state.vertexProgram ? state.vertexProgram->attributeInputMask : 0xFFFF;
    const uint16_t used = uint16_t(programInputs & decl->attributeMask);
    for (const AglVertexStream* stream = decl->streams; stream->size != 0; ++stream) {
        const uint32_t attribute = stream->attribute + stream->attributeOffset;
        if (stream->skip || !(used & (1u << attribute)))
            continue;
        gpu::GpuPtr at = base;
        at.data = static_cast<const uint8_t*>(base.data) + stream->offset;
        cmd.SetVertexAttribute(uint8_t(attribute), 0, uint8_t(decl->stride), stream->size,
                               static_cast<gpu::VertexAttribType>(stream->type), at);
    }
    uint16_t stale = uint16_t((state.meshAttributes | used) ^ used);
    state.meshAttributes = used;
    for (int attribute = gpu::kMaxVertexAttributes - 1; stale != 0 && attribute >= 0; --attribute) {
        if (stale & (1u << attribute)) {
            stale = uint16_t(stale & ~(1u << attribute));
            cmd.DisableVertexAttribute(uint8_t(attribute));
        }
    }
}

// @func 0x0053539C AglRenderState::CommitDrawState
void CommitDrawState()
{
    gpu::CommandList& cmd = Cmd();
    CommitFragmentProgram();

    if (!g_state.vertexLayoutDirty)
        return;
    g_state.vertexLayoutDirty = false;
    const uint16_t provided = g_state.attributeMask;
    for (uint32_t i = 0; i < g_state.streamCount; ++i) {
        const AglVertexStream& stream = g_state.streams[i];
        const uint32_t attribute = stream.attribute + stream.attributeOffset;
        if (stream.skip || !(provided & (1u << attribute)))
            continue;
        gpu::GpuPtr memory = g_state.vertexBuffer;
        memory.data = static_cast<const uint8_t*>(memory.data) + stream.offset;
        memory.location = gpu::MemoryLocation::Main;
        cmd.SetVertexAttribute(static_cast<uint8_t>(attribute), 0, static_cast<uint8_t>(g_state.vertexStride),
                               stream.size, static_cast<gpu::VertexAttribType>(stream.type), memory);
    }
    // Attributes enabled by the previous layout but not by this one.
    uint16_t stale = static_cast<uint16_t>((g_state.enabledAttributes | provided) ^ provided);
    for (int attribute = gpu::kMaxVertexAttributes - 1; stale != 0 && attribute >= 0; --attribute) {
        if (stale & (1u << attribute)) {
            stale = static_cast<uint16_t>(stale & ~(1u << attribute));
            cmd.DisableVertexAttribute(static_cast<uint8_t>(attribute));
        }
    }
    g_state.enabledAttributes = provided;
}

// @func 0x00537B7C FindVertexStream
const AglVertexStream* FindVertexStream(const AglVertexStream* streams, uint32_t attribute,
                                        uint32_t attributeOffset)
{
    for (; streams->size != 0; ++streams) {
        if (streams->attribute == attribute && streams->attributeOffset == attributeOffset)
            return streams;
    }
    return nullptr;
}

// @func 0x0053C7EC AglRenderer::BindTexture
void ApiBindTexture(const AglTextureImage* texture, uint32_t unit, int sampler)
{
    BindTexture(texture, unit, sampler);
}

// @func 0x0053C80C AglRenderer::Begin2DPass
void ApiBegin2DPass(bool disableBlend, uint32_t clearConstant)
{
    Begin2DPass(disableBlend, clearConstant);
}

// @func 0x0053C830 AglRenderer::SetDefault3DState
// @func 0x0054338C AglRenderer::SetDefault3DState (copy 2)
void ApiSetDefault3DState()
{
    SetDefault3DState();
}

// @func 0x0053C850 AglRenderer::CommitDrawState
void ApiCommitDrawState()
{
    CommitDrawState();
}

// @func 0x0051279C InvalidateRenderState
void InvalidateRenderState()
{
    AglRenderState& st = g_state;
    st.field_10A4 = 0;
    st.boundVertexBuffer = nullptr;
    st.boundVertexDecl = nullptr;
    st.vertexProgram = nullptr;
    st.vertexProgramBlock = nullptr;
    st.material = nullptr;
    st.meshAttributes = 0xFFFF;
    st.field_10F8 = 0;
    for (auto& texture : st.textures)
        texture = nullptr;
    st.fogLayer = -1;
    st.samplerMinFilter[0] = st.samplerMinFilter[1] = 0;
    st.materialDirty = false;
    st.stencilFront = AglStencilState();
    st.stencilBack = AglStencilState();
    GetDrawList().InvalidateCache();
}

// @func 0x004FCC34 UploadViewSpaceMatrix
void UploadViewSpaceMatrix(const ape::Matrix34& viewSpace)
{
    const ape::Matrix44& p = GetViewConstants().projection;
    const ape::Vec3* rows[4] = { &viewSpace.xAxis, &viewSpace.yAxis, &viewSpace.zAxis, &viewSpace.translation };
    float m[4][4];
    for (int i = 0; i < 4; ++i) {
        // The axes are directions; the translation row carries w = 1.
        const ape::Vec3& r = *rows[i];
        const float w = i == 3 ? 1.0f : 0.0f;
        m[i][0] = r.x * p.rows[0].x + r.y * p.rows[1].x + r.z * p.rows[2].x + w * p.rows[3].x;
        m[i][1] = r.x * p.rows[0].y + r.y * p.rows[1].y + r.z * p.rows[2].y + w * p.rows[3].y;
        m[i][2] = r.x * p.rows[0].z + r.y * p.rows[1].z + r.z * p.rows[2].z + w * p.rows[3].z;
        m[i][3] = r.x * p.rows[0].w + r.y * p.rows[1].w + r.z * p.rows[2].w + w * p.rows[3].w;
    }
    float transposed[16];
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            transposed[c * 4 + r] = m[r][c];
    Cmd().SetVertexProgramConstants(0, 16, transposed);
}

// @func 0x005011A0 ApplyParticleStencilMask
void ApplyParticleStencilMask(bool masked)
{
    if (!masked)
        return;
    gpu::CommandList& cmd = Cmd();
    cmd.SetStencilTestEnable(true);
    AglStencilState& front = g_state.stencilFront;
    front.fail = front.depthFail = front.depthPass = gpu::StencilOp::Keep;
    front.func = gpu::CompareFunc::Equal;
    front.ref = 0x20;
    front.mask = 0x20;
    front.writeMask = 0xFF;
    cmd.SetStencilOp(front.fail, front.depthFail, front.depthPass);
    cmd.SetStencilMask(front.writeMask);
    cmd.SetStencilFunc(front.func, front.ref, front.mask);
}

// TODO(0x0053B668 AglRenderSurface::Bind)
// The console version also chooses the buffers of the current eye for the
// back buffer in stereo mode and enables multisampling for it.
void BindRenderSurface(const AglRenderSurface* surface, bool clear)
{
    if (!surface)
        return;
    SetRenderTargets(surface->depth, surface->color, 0);
    gpu::CommandList& cmd = Cmd();
    cmd.SetColorMask(surface->colorMask);
    cmd.SetDepthTestEnable(g_state.depthEnabled);
    cmd.SetDepthFunc(gpu::CompareFunc::LEqual);
    const uint16_t rect[4] = { 0, 0, static_cast<uint16_t>(surface->width), static_cast<uint16_t>(surface->height) };
    SetViewportRect(rect);
    if (clear && surface->clearMask != 0) {
        const uint32_t color = 0xFF000000u | (uint32_t(surface->clearColor[0]) << 16) |
                               (uint32_t(surface->clearColor[1]) << 8) | surface->clearColor[2];
        ClearTargets(1.0f, surface->clearMask, color, 0, 0xFF);
    }
}

} // namespace agl
