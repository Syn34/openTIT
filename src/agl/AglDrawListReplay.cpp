// openTIT - Agl draw list replay: the render thread side of the draw list
// (0x00514CC8-0x005165D0).
//
// The render thread walks the packets recorded since the start of the view
// and turns them into GPU commands: camera and world matrices become vertex
// constants 0..3, materials select and begin their technique, the state
// packets set blend/depth/cull/stencil/alpha state directly (recording it in
// the material state cache so the next material does not re-send equal
// state), and primitives are drawn from the frame's vertex area with the
// debug vertex layout.
#include "agl/AglDrawList.h"

#include "agl/AglDrawBucket.h"
#include "agl/AglPrimitives.h"
#include "agl/AglResources.h"
#include "agl/AglShader.h"
#include "agl/AglShaderClass.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

namespace agl {

namespace {

constexpr size_t kHeaderBytes = 12;

// Render item handed to SelectTechnique for draw list materials (the
// console keeps one at 0x00F66FB8): identity, no lights.
AglRenderItem& ReplayItem()
{
    static AglRenderItem item = [] {
        AglRenderItem r;
        r.axisX[0] = 1.0f;
        r.axisY[1] = 1.0f;
        r.axisZ[2] = 1.0f;
        return r;
    }();
    return item;
}

// (world * toClip), transposed into vertex constants 0..3.
void UploadClipMatrix(const ape::Matrix44& world, const ape::Matrix44& toClip)
{
    ape::Matrix44 product;
    ape::Matrix44Mul(product, world, toClip);
    float transposed[16];
    for (int c = 0; c < 4; ++c) {
        transposed[c * 4 + 0] = (&product.rows[0].x)[c];
        transposed[c * 4 + 1] = (&product.rows[1].x)[c];
        transposed[c * 4 + 2] = (&product.rows[2].x)[c];
        transposed[c * 4 + 3] = (&product.rows[3].x)[c];
    }
    Cmd().SetVertexProgramConstants(0, 16, transposed);
}

// Every vertex attribute off, so the next mesh binds its streams again.
void ResetVertexArrays()
{
    gpu::CommandList& cmd = Cmd();
    for (int attribute = 0; attribute < gpu::kMaxVertexAttributes; ++attribute)
        cmd.DisableVertexAttribute(uint8_t(attribute));
    AglRenderState& st = GetRenderState();
    st.boundVertexBuffer = nullptr;
    st.meshAttributes = 0;
}

void ApplyStencilFunc(const AglStencilState& s)
{
    Cmd().SetStencilFunc(s.func, s.ref, s.mask);
}

} // namespace

// @func 0x00514CC8 AglDrawList::ReadAppendPacket
void AglDrawList::ReadAppendPacket(const AglDrawList** list, uint32_t* start) const
{
    Read(kHeaderBytes, 4);
    *list = ReadValue<const AglDrawList*>();
    *start = ReadValue<uint32_t>();
}

// @func 0x00514D68 AglDrawList::ReplayStatePacket
// A non-zero state byte pulls the geometry slightly towards the camera
// (decals, outlines): a constant polygon offset of 1/10000 of the depth
// buffer's range. Without a depth buffer of a known format it is off.
void AglDrawList::ReplayStatePacket()
{
    Read(kHeaderBytes, 4);
    const uint8_t state = ReadValue<uint8_t>();
    gpu::CommandList& cmd = Cmd();
    const AglRenderState& st = GetRenderState();
    if (state == 0 || !st.depthEnabled || !st.depthTarget) {
        cmd.SetPolygonOffset(false, 0.0f, 0.0f);
        return;
    }
    float depthRange;
    switch (st.depthTarget->format) {
    case kAglFmt_ShadowZ16:
    case kAglFmt_Z16:
        depthRange = 65536.0f;
        break;
    case kAglFmt_ShadowZ24S8:
    case kAglFmt_D24S8:
        depthRange = 16777216.0f;
        break;
    default:
        // NOTE: the console sets a "warned" flag (0x011DE2C4) here; the
        // message itself was stripped.
        cmd.SetPolygonOffset(false, 0.0f, 0.0f);
        return;
    }
    cmd.SetPolygonOffset(true, 0.0f, depthRange * -0.0001f);
}

// @func 0x00515018 AglDrawList::ReplayLineWidthPacket
void AglDrawList::ReplayLineWidthPacket() const
{
    Read(kHeaderBytes, 4);
    Cmd().SetLineWidth(ReadValue<float>());
}

// @func 0x005150A8 AglDrawList::ReplayAlphaTestPacket
void AglDrawList::ReplayAlphaTestPacket() const
{
    Read(kHeaderBytes, 4);
    const uint32_t word = ReadValue<uint32_t>();
    gpu::CommandList& cmd = Cmd();
    cmd.SetAlphaTestEnable((word >> 24) != 0);
    cmd.SetAlphaFunc(static_cast<gpu::CompareFunc>(word & 0xFFFF), (word >> 16) & 0xFF);
}

// @func 0x00515168 AglDrawList::ReadBlendPacket
void AglDrawList::ReadBlendPacket(uint32_t* src, uint32_t* dst, uint32_t* equation, bool* enable) const
{
    Read(kHeaderBytes, 4);
    const AglDrawBlendPacket packet = ReadValue<AglDrawBlendPacket>();
    *src = packet.src;
    *dst = packet.dst;
    *equation = packet.equation;
    *enable = packet.enable != 0;
}

// @func 0x005151E8 AglDrawList::ReplayStencilTestPacket
// Stencil test on with the packet's function and reference (the mask is
// kept), or off with the function reset to "always".
void AglDrawList::ReplayStencilTestPacket() const
{
    Read(kHeaderBytes, 4);
    const uint32_t word = ReadValue<uint32_t>();
    AglStencilState& front = GetRenderState().stencilFront;
    gpu::CommandList& cmd = Cmd();
    if ((word >> 24) == 0) {
        cmd.SetStencilTestEnable(false);
        front.func = gpu::CompareFunc::Always;
    } else {
        cmd.SetStencilTestEnable(true);
        front.func = static_cast<gpu::CompareFunc>(word & 0xFFFF);
        front.ref = int32_t((word >> 16) & 0xFF);
    }
    ApplyStencilFunc(front);
}

// @func 0x00515350 AglDrawList::ReplayStencilWritePacket
// Stencil writes: every drawn pixel stores the packet's reference value
// (replace on pass and fail, keep on depth fail, all bits written, test
// always passes); off keeps the buffer unchanged.
void AglDrawList::ReplayStencilWritePacket() const
{
    Read(kHeaderBytes, 4);
    const uint32_t word = ReadValue<uint32_t>();
    AglStencilState& front = GetRenderState().stencilFront;
    gpu::CommandList& cmd = Cmd();
    front.depthFail = gpu::StencilOp::Keep;
    if ((word >> 24) == 0) {
        front.fail = gpu::StencilOp::Keep;
        front.depthPass = gpu::StencilOp::Keep;
        cmd.SetStencilOp(front.fail, front.depthFail, front.depthPass);
        return;
    }
    front.fail = gpu::StencilOp::Replace;
    front.depthPass = gpu::StencilOp::Replace;
    cmd.SetStencilOp(front.fail, front.depthFail, front.depthPass);
    front.func = gpu::CompareFunc::Always;
    front.writeMask = 0xFF;
    cmd.SetStencilMask(front.writeMask);
    front.ref = int32_t((word >> 16) & 0xFF);
    ApplyStencilFunc(front);
}

// @func 0x00515544 AglDrawList::ReadTexturePacket
AglTexture* AglDrawList::ReadTexturePacket(int* slot) const
{
    Read(kHeaderBytes, 4);
    AglTexture* texture = ReadValue<AglTexture*>();
    *slot = ReadValue<int32_t>();
    return texture;
}

// @func 0x005155C4 AglDrawList::ReplayFragmentConstPacket
void AglDrawList::ReplayFragmentConstPacket() const
{
    Read(kHeaderBytes, 4);
    const uint32_t slot = ReadValue<uint32_t>();
    float value[4];
    std::memcpy(value, Read(sizeof(value), 4), sizeof(value));
    SetShaderConstant(slot, value);
}

// @func 0x00515688 AglDrawList::ReplayVertexConstPacket
void AglDrawList::ReplayVertexConstPacket() const
{
    Read(kHeaderBytes, 4);
    const uint32_t reg = ReadValue<uint32_t>();
    float value[4];
    std::memcpy(value, Read(sizeof(value), 4), sizeof(value));
    Cmd().SetVertexProgramConstants(reg, 4, value);
}

// @func 0x00515784 AglDrawList::UploadScreenMatrix
void AglDrawList::UploadScreenMatrix(const ape::Matrix44& world, const ape::Matrix44& projection) const
{
    UploadClipMatrix(world, projection);
}

// @func 0x005158B4 AglDrawList::UploadWorldMatrix
void AglDrawList::UploadWorldMatrix(const ape::Matrix44& world, const ape::Matrix44& viewProjection) const
{
    UploadClipMatrix(world, viewProjection);
}

// @func 0x005159E8 AglDrawList::Replay
void AglDrawList::Replay(AglDrawReplayContext& context)
{
    AglMaterial* material = nullptr;    // material and technique of the last material packet
    AglTechnique* technique = nullptr;
    gpu::CommandList& cmd = Cmd();

    const auto readMatrix = [this] {
        Read(kHeaderBytes, 4);
        ape::Matrix44 m;
        std::memcpy(&m, Read(sizeof(m), 16), sizeof(m));
        return m;
    };
    const auto vertexArea = [this] {
        return gpu::GpuPtr{ m_vertexBuffers[m_replayBuffer].data(), gpu::MemoryLocation::Main };
    };
    // Material packets: the technique the material's class picks for a
    // plain item replaces the previous one.
    const auto switchMaterial = [&](bool withState) {
        Read(kHeaderBytes, 4);
        AglMaterial* next = ReadValue<AglMaterial*>();
        AglTechnique* nextTechnique = next->shaderClass->SelectTechnique(next, &ReplayItem());
        if (material)
            technique->End(material);
        if (withState) {
            ApplyMaterial(next, nextTechnique);
        } else {
            BindTechniquePrograms(nextTechnique);
            nextTechnique->BeginMaterial(next);
        }
        if (next != material)
            BindVertexArrays(GetDebugVertexDecl(true), vertexArea());
        material = next;
        technique = nextTechnique;
    };

    for (bool done = false; !done;) {
        // NOTE: the console relies on the end packets alone; stopping at
        // the write position as well keeps an unterminated list safe.
        if (PlaceBlock(m_readCursor, kHeaderBytes, 4) == PlaceBlock(m_cursor, kHeaderBytes, 4))
            break;
        const uint8_t* header = m_packets.data() + PlaceBlock(m_readCursor, kHeaderBytes, 4);
        uint32_t words[3];
        std::memcpy(words, header, sizeof(words));
        const uint8_t op = uint8_t(words[0] >> 24);

        switch (op) {
        case kAglDraw_Projection:
            context.projection = readMatrix();
            break;
        case kAglDraw_View:
            context.view = readMatrix();
            ape::Matrix44Mul(context.viewProjection, context.view, context.projection);
            break;
        case kAglDraw_World:
            UploadWorldMatrix(readMatrix(), context.viewProjection);
            break;
        case kAglDraw_ScreenWorld:
            UploadScreenMatrix(readMatrix(), context.projection);
            break;
        case kAglDraw_Material:
            switchMaterial(true);
            break;
        case kAglDraw_MaterialShader:
            switchMaterial(false);
            break;
        case kAglDraw_VertexConst:
            ReplayVertexConstPacket();
            break;
        case kAglDraw_FragmentConst:
            ReplayFragmentConstPacket();
            break;
        case kAglDraw_Texture: {
            int slot = 0;
            AglTexture* texture = ReadTexturePacket(&slot);
            // NOTE: the console binds with the texture's own filter and
            // wrap settings (0x0079DF78); BindTexture uses the sampler defaults.
            BindTexture(texture ? &texture->image : nullptr, uint32_t(slot), 0);
            break;
        }
        case kAglDraw_Scissor: {
            Read(kHeaderBytes, 4);
            uint16_t rect[4];
            std::memcpy(rect, Read(sizeof(rect), 4), sizeof(rect));
            cmd.SetScissor(rect[0], rect[1], rect[2], rect[3]);
            break;
        }
        case kAglDraw_Depth: {
            Read(kHeaderBytes, 4);
            const AglDrawDepthPacket packet = ReadValue<AglDrawDepthPacket>();
            SetDepthStateCached(static_cast<gpu::CompareFunc>(packet.func), packet.write == 1);
            break;
        }
        case kAglDraw_Blend: {
            uint32_t src = 0, dst = 0, equation = 0;
            bool enable = false;
            ReadBlendPacket(&src, &dst, &equation, &enable);
            SetBlendStateCached(enable, static_cast<gpu::BlendFactor>(src), static_cast<gpu::BlendFactor>(dst),
                                static_cast<gpu::BlendEquation>(equation));
            break;
        }
        case kAglDraw_Cull:
            Read(kHeaderBytes, 4);
            SetCullStateCached(uint16_t(ReadValue<uint32_t>()));
            break;
        case kAglDraw_StencilWrite:
            ReplayStencilWritePacket();
            break;
        case kAglDraw_StencilTest:
            ReplayStencilTestPacket();
            break;
        case kAglDraw_AlphaTest:
            ReplayAlphaTestPacket();
            break;
        case kAglDraw_LineWidth:
            ReplayLineWidthPacket();
            break;
        case kAglDraw_State:
            ReplayStatePacket();
            break;
        case kAglDraw_AppendList: {
            const AglDrawList* list = nullptr;
            uint32_t start = 0;
            ReadAppendPacket(&list, &start);
            ResetVertexArrays();
            list->SetReadPosition(start);
            // The appended list starts from this list's camera.
            AglDrawReplayContext nested = context;
            const_cast<AglDrawList*>(list)->Replay(nested);
            material = nullptr;
            technique = nullptr;
            break;
        }
        case kAglDraw_Primitive: {
            Read(kHeaderBytes, 4);
            const auto primitive = static_cast<gpu::PrimitiveType>(words[0] & 0xFF);
            const uint32_t count = words[1] >> 16;
            const uint32_t first = words[2];
            CommitFragmentProgram();
            cmd.DrawArrays(primitive, first, count);
            break;
        }
        case kAglDraw_Marker:
            Read(kHeaderBytes, 4);
            done = true;
            break;
        case kAglDraw_End:
            // The end of the list returns at once (the technique is not ended).
            return;
        default:
            // NOTE: unknown packets would stall the console loop; skip them.
            Read(kHeaderBytes, 4);
            break;
        }
    }

    if (material)
        technique->End(material);
    if (m_state != 0) {
        m_state = 0;
        cmd.SetPolygonOffset(false, 0.0f, 0.0f);
    }
    ResetVertexArrays();
}

// @func 0x005165D0 ReplayMainDrawList
// Plays the game's draw list from its read position.
void ReplayMainDrawList()
{
    AglDrawReplayContext context;
    GetDrawList().Replay(context);
}

} // namespace agl
