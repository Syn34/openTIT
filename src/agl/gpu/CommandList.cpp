// openTIT - Recorded GPU command list (the PC replacement for libgcm command buffers).
//
// The console linked two copies of many libgcm helpers: one that first
// checked for room in the command buffer (calling the overflow callback when
// full) and one "unsafe" copy for callers that had reserved space already.
// A CommandList grows on demand, so both copies are the same method here;
// both addresses are tagged on it.
#include "agl/gpu/CommandList.h"

#include "agl/gpu/CgProgram.h"

#include <cstring>

namespace agl::gpu {

// @func 0x005DF960 cellGcmSetupContextData
void CommandList::Reset(size_t budget, OverflowCallback callback)
{
    m_commands.clear();
    m_data.clear();
    m_budget = budget;
    m_overflow = callback;
}

// @func 0x005E08E8 cellGcmReserveMethodSize
bool CommandList::Reserve(size_t count)
{
    const size_t needed = m_commands.size() + count;
    if (m_budget != 0 && needed > m_budget && m_overflow) {
        if (!m_overflow(*this, count))
            return false;
    }
    m_commands.reserve(needed);
    return true;
}

bool CommandList::Push(const Command& command)
{
    if (m_budget != 0 && m_commands.size() >= m_budget && m_overflow) {
        if (!m_overflow(*this, 1))
            return false;
    }
    m_commands.push_back(command);
    return true;
}

size_t CommandList::PushData(const void* data, size_t bytes)
{
    // Keep payloads 16-byte aligned so they can be read as float vectors.
    const size_t offset = (m_data.size() + 15) & ~size_t(15);
    m_data.resize(offset + bytes);
    if (bytes)
        std::memcpy(m_data.data() + offset, data, bytes);
    return offset;
}

// --- render state ------------------------------------------------------------

// @func 0x005E2A24 cellGcmSetAlphaTestEnable
// @func 0x005E3DB0 cellGcmSetAlphaTestEnable (unchecked)
void CommandList::SetAlphaTestEnable(bool enable)
{
    Push(cmd::AlphaTestEnable{enable});
}

void CommandList::SetAlphaFunc(CompareFunc func, uint32_t ref)
{
    Push(cmd::AlphaFunc{func, ref});
}

// @func 0x005E2AB4 cellGcmSetBlendEnable
// @func 0x005E3DD4 cellGcmSetBlendEnable (unchecked)
void CommandList::SetBlendEnable(bool enable)
{
    Push(cmd::BlendEnable{enable});
}

// @func 0x005E2994 cellGcmSetBlendEnableMrt
void CommandList::SetBlendEnableMrt(bool target1, bool target2, bool target3)
{
    const uint32_t mask = (target1 ? 2u : 0u) | (target2 ? 4u : 0u) | (target3 ? 8u : 0u);
    Push(cmd::BlendEnableMrt{mask});
}

// @func 0x005E2720 cellGcmSetBlendFunc
void CommandList::SetBlendFunc(BlendFactor srcColor, BlendFactor dstColor, BlendFactor srcAlpha, BlendFactor dstAlpha)
{
    Push(cmd::BlendFunc{srcColor, dstColor, srcAlpha, dstAlpha});
}

void CommandList::SetBlendEquation(BlendEquation color, BlendEquation alpha)
{
    Push(cmd::BlendEquationCmd{color, alpha});
}

void CommandList::SetBlendColor(uint32_t argb)
{
    Push(cmd::BlendColor{argb});
}

// @func 0x005E3CFC cellGcmSetColorMask
void CommandList::SetColorMask(uint32_t mask)
{
    Push(cmd::ColorMaskCmd{mask});
}

void CommandList::SetColorMaskMrt(uint32_t mask)
{
    Push(cmd::ColorMaskMrt{mask});
}

// @func 0x005E2C64 cellGcmSetDepthTestEnable
// @func 0x005E3E1C cellGcmSetDepthTestEnable (unchecked)
void CommandList::SetDepthTestEnable(bool enable)
{
    Push(cmd::DepthTestEnable{enable});
}

// @func 0x005E27E4 cellGcmSetDepthFunc
// @func 0x005E3D44 cellGcmSetDepthFunc (unchecked)
void CommandList::SetDepthFunc(CompareFunc func)
{
    Push(cmd::DepthFunc{func});
}

// @func 0x005E2874 cellGcmSetDepthMask
void CommandList::SetDepthMask(bool write)
{
    Push(cmd::DepthMask{write});
}

// @func 0x005E2BD4 cellGcmSetDepthBoundsTestEnable
void CommandList::SetDepthBoundsTestEnable(bool enable)
{
    Push(cmd::DepthBoundsTestEnable{enable});
}

// @func 0x005E341C cellGcmSetDepthBounds
void CommandList::SetDepthBounds(float zMin, float zMax)
{
    Push(cmd::DepthBounds{zMin, zMax});
}

void CommandList::SetStencilTestEnable(bool enable)
{
    Push(cmd::StencilTestEnable{enable});
}

// @func 0x005E3D8C cellGcmSetStencilMask
void CommandList::SetStencilMask(uint32_t mask)
{
    Push(cmd::StencilMask{mask});
}

void CommandList::SetStencilFunc(CompareFunc func, int32_t ref, uint32_t mask)
{
    Push(cmd::StencilFunc{func, ref, mask});
}

void CommandList::SetStencilOp(StencilOp fail, StencilOp depthFail, StencilOp depthPass)
{
    Push(cmd::StencilOpCmd{fail, depthFail, depthPass});
}

// @func 0x005E2B44 cellGcmSetCullFaceEnable
// @func 0x005E3DF8 cellGcmSetCullFaceEnable (unchecked)
void CommandList::SetCullFaceEnable(bool enable)
{
    Push(cmd::CullFaceEnable{enable});
}

// @func 0x005E3D20 cellGcmSetCullFace
void CommandList::SetCullFace(CullFace face)
{
    Push(cmd::CullFaceCmd{face});
}

// @func 0x005E2904 cellGcmSetFrontFace
// @func 0x005E3D68 cellGcmSetFrontFace (unchecked)
void CommandList::SetFrontFace(FrontFace face)
{
    Push(cmd::FrontFaceCmd{face});
}

void CommandList::SetPolygonOffset(bool enable, float factor, float units)
{
    Push(cmd::PolygonOffset{enable, factor, units});
}

void CommandList::SetLineWidth(float width)
{
    Push(cmd::LineWidth{width});
}

// @func 0x005E3E40 cellGcmSetRestartIndexEnable
void CommandList::SetRestartIndexEnable(bool enable)
{
    Push(cmd::RestartIndexEnable{enable});
}

// @func 0x005E3F5C cellGcmSetRestartIndex
void CommandList::SetRestartIndex(uint32_t index)
{
    Push(cmd::RestartIndex{index});
}

// @func 0x005E30D8 cellGcmSetUserClipPlaneControl
void CommandList::SetUserClipPlaneControl(UserClipMode p0, UserClipMode p1, UserClipMode p2,
                                          UserClipMode p3, UserClipMode p4, UserClipMode p5)
{
    Push(cmd::UserClipPlaneControl{{p0, p1, p2, p3, p4, p5}});
}

// @func 0x005E2590 cellGcmSetAntiAliasingControl
// @func 0x005E3C38 cellGcmSetAntiAliasingControl (unchecked)
void CommandList::SetAntiAliasingControl(bool enable, bool alphaToCoverage, bool alphaToOne, uint16_t sampleMask)
{
    Push(cmd::AntiAliasingControl{enable, alphaToCoverage, alphaToOne, sampleMask});
}

// @func 0x005E4068 cellGcmSetViewport
void CommandList::SetViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float zMin, float zMax,
                              const float scale[4], const float offset[4])
{
    cmd::ViewportCmd c{};
    c.viewport.x = x;
    c.viewport.y = y;
    c.viewport.width = width;
    c.viewport.height = height;
    c.viewport.zMin = zMin;
    c.viewport.zMax = zMax;
    for (int i = 0; i < 4; ++i) {
        c.viewport.scale[i] = scale[i];
        c.viewport.offset[i] = offset[i];
    }
    Push(c);
}

void CommandList::SetScissor(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    Push(cmd::Scissor{x, y, width, height});
}

// @func 0x005E47A0 cellGcmSetSurface
void CommandList::SetSurface(const SurfaceDesc& surface)
{
    Push(cmd::Surface{surface});
}

// @func 0x005E3CB4 cellGcmSetClearColor
void CommandList::SetClearColor(uint32_t argb)
{
    Push(cmd::ClearColor{argb});
}

// @func 0x005E3CD8 cellGcmSetClearDepthStencil
void CommandList::SetClearDepthStencil(uint32_t value)
{
    Push(cmd::ClearDepthStencil{value});
}

// @func 0x005E3C74 cellGcmSetClearSurface
void CommandList::ClearSurface(uint32_t mask)
{
    Push(cmd::ClearSurface{mask & kClear_All});
}

// --- textures ------------------------------------------------------------------

// @func 0x005E0EAC cellGcmSetTexture
// @func 0x005E4514 cellGcmSetTexture (unchecked)
void CommandList::SetTexture(uint8_t unit, const TextureDesc& texture)
{
    Push(cmd::Texture{unit, texture});
}

// @func 0x005E2F44 cellGcmSetTextureControl
// @func 0x005E3F18 cellGcmSetTextureControl (unchecked)
void CommandList::SetTextureControl(uint8_t unit, bool enable, uint16_t minLod, uint16_t maxLod, uint8_t maxAniso)
{
    SamplerControl control;
    control.enable = enable;
    control.minLod = static_cast<uint16_t>(minLod & 0xFFF);
    control.maxLod = static_cast<uint16_t>(maxLod & 0xFFF);
    control.maxAniso = maxAniso;
    Push(cmd::TextureControl{unit, control});
}

// @func 0x005E2E54 cellGcmSetTextureAddress
// @func 0x005E3EC8 cellGcmSetTextureAddress (unchecked)
void CommandList::SetTextureAddress(uint8_t unit, TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR,
                                    uint8_t unsignedRemap, uint8_t depthCompare, uint8_t gamma)
{
    SamplerAddress address;
    address.wrapS = wrapS;
    address.wrapT = wrapT;
    address.wrapR = wrapR;
    address.unsignedRemap = unsignedRemap;
    address.depthCompare = depthCompare;
    address.gamma = gamma;
    Push(cmd::TextureAddress{unit, address});
}

// @func 0x005E2D84 cellGcmSetTextureFilter
// @func 0x005E3E88 cellGcmSetTextureFilter (unchecked)
void CommandList::SetTextureFilter(uint8_t unit, uint16_t lodBias, TextureMinFilter min, TextureMagFilter mag,
                                   uint8_t convolution)
{
    SamplerFilter filter;
    filter.lodBias = static_cast<uint16_t>(lodBias & 0x1FFF);
    filter.min = min;
    filter.mag = mag;
    filter.convolution = convolution;
    Push(cmd::TextureFilter{unit, filter});
}

void CommandList::SetTextureBorderColor(uint8_t unit, uint32_t argb)
{
    Push(cmd::TextureBorderColor{unit, argb});
}

// @func 0x005E2CF4 cellGcmSetInvalidateTextureCache
// @func 0x005E3E64 cellGcmSetInvalidateTextureCache (unchecked)
void CommandList::InvalidateTextureCache(TextureCacheKind kind)
{
    Push(cmd::InvalidateTextureCache{kind});
}

// --- programs ------------------------------------------------------------------

void CommandList::SetVertexProgram(const VertexProgramDesc& program)
{
    cmd::VertexProgram c{program, 0};
    // The console copied the microcode into the command stream; do the same
    // so the list stays valid even if the program image is released.
    c.ucodeOffset = PushData(program.ucode, size_t(program.instructionCount) * 16);
    c.desc.ucode = nullptr;
    Push(c);
}

namespace {

// Number of constant registers and floats per register used by a Cg
// float/half vector or matrix type, false for other types.
bool GetNumericTypeShape(uint32_t type, uint32_t* rows, uint32_t* columns)
{
    // Each family starts with the 4 vector types followed by the 16
    // RxC matrix types (1x1 .. 4x4).
    static const uint32_t kFamilies[] = { kCgType_Float, kCgType_Float + 20 };   // float, half
    for (uint32_t base : kFamilies) {
        if (type < base || type >= base + 20)
            continue;
        const uint32_t i = type - base;
        if (i < 4) {
            *rows = 1;
            *columns = i + 1;
        } else {
            *rows = (i - 4) / 4 + 1;
            *columns = (i - 4) % 4 + 1;
        }
        return true;
    }
    return false;
}

// @func 0x005E1814 cellGcmSetVertexProgramParameterDefault
// @func 0x005E5334 cellGcmSetVertexProgramParameterDefault (unchecked)
// Uploads the default value of one vertex program uniform to its constant
// registers (one register per matrix row, unused components zero).
void SetVertexParameterDefault(CommandList& list, const CgProgram& program, const CgParameter& param)
{
    uint32_t rows = 0;
    uint32_t columns = 0;
    // Parameters without a register (resource CG_UNDEFINED) are skipped.
    constexpr uint32_t kCgResourceUndefined = 0xCB8;
    if (param.resource == kCgResourceUndefined)
        return;
    const float* values = GetParameterDefaults(&program, &param);
    if (!values || !GetNumericTypeShape(param.type, &rows, &columns))
        return;
    float registers[16] = {};
    for (uint32_t r = 0; r < rows; ++r)
        for (uint32_t c = 0; c < columns; ++c)
            registers[r * 4 + c] = values[r * columns + c];
    list.SetVertexProgramConstants(param.resourceIndex, rows * 4, registers);
}

} // namespace

// @func 0x005E20AC cellGcmSetVertexProgram
// @func 0x005E5914 cellGcmSetVertexProgram (unchecked)
void CommandList::SetVertexProgram(const CgProgram& program, const void* ucode)
{
    VertexProgramDesc desc;
    if (!GetVertexProgramDesc(&program, &desc))
        return;
    if (ucode)
        desc.ucode = static_cast<const uint32_t*>(ucode);
    SetVertexProgram(desc);

    // Uniforms and constants with default values are initialised right away.
    const CgParameter* params = GetParameters(&program);
    for (uint32_t i = 0; i < program.parameterCount; ++i) {
        const CgParameter& p = params[i];
        if (p.defaultValue != 0 && (p.variability == kCgVar_Uniform || p.variability == kCgVar_Constant))
            SetVertexParameterDefault(*this, program, p);
    }
}

// @func 0x005E4124 cellGcmSetVertexProgramConstants
// @func 0x005E5CB0 cellGcmSetVertexProgramConstants (unchecked)
void CommandList::SetVertexProgramConstants(uint32_t first, uint32_t count, const float* values)
{
    if (count == 0)
        return;
    const size_t offset = PushData(values, count * sizeof(float));
    Push(cmd::VertexConstants{first, count, offset});
}

// @func 0x005E3F80 cellGcmSetTransformBranchBits
void CommandList::SetTransformBranchBits(uint32_t bits)
{
    Push(cmd::TransformBranchBits{bits});
}

// @func 0x005E42A0 cellGcmSetFragmentProgram
// @func 0x005E0E5C cellGcmSetFragmentProgramLoad
void CommandList::SetFragmentProgram(const FragmentProgramDesc& program)
{
    FragmentProgramDesc desc = program;
    // The hardware needs at least two registers.
    if (desc.registerCount < 2)
        desc.registerCount = 2;
    Push(cmd::FragmentProgram{desc});
}

// @func 0x005E073C cellGcmSetFragmentProgramOffset
void CommandList::SetFragmentProgramAddress(GpuPtr ucode)
{
    Push(cmd::FragmentProgramAddress{ucode});
}

// --- vertex input and drawing ----------------------------------------------------

// @func 0x005E0BA4 cellGcmSetVertexDataArray
// @func 0x005E4394 cellGcmSetVertexDataArray (unchecked)
void CommandList::SetVertexAttribute(uint8_t index, uint16_t frequency, uint8_t stride, uint8_t size,
                                     VertexAttribType type, GpuPtr memory)
{
    Push(cmd::VertexAttribute{index, frequency, stride, size, type, memory});
}

void CommandList::DisableVertexAttribute(uint8_t index)
{
    // A stream of size 0 disables the attribute.
    Push(cmd::VertexAttribute{index, 0, 0, 0, VertexAttribType::Float, GpuPtr{}});
}

// @func 0x005E3014 cellGcmSetInvalidateVertexCache
void CommandList::InvalidateVertexCache()
{
    Push(cmd::InvalidateVertexCache{});
}

// @func 0x005E1480 cellGcmSetDrawArrays
// @func 0x005E4D98 cellGcmSetDrawArrays (unchecked)
void CommandList::DrawArrays(PrimitiveType primitive, uint32_t first, uint32_t count)
{
    // The console split large draws into batches of 256 vertices; a
    // backend can draw the whole range at once.
    if (count == 0)
        return;
    Push(cmd::DrawArrays{primitive, first, count});
}

// @func 0x005E4F64 cellGcmSetDrawIndexArray
void CommandList::DrawIndexArray(PrimitiveType primitive, uint32_t count, IndexType indexType, GpuPtr indices)
{
    if (count == 0)
        return;
    Push(cmd::DrawIndexed{primitive, count, indexType, indices});
}

// @func 0x005E51AC cellGcmSetDrawInlineArray (unchecked)
void CommandList::DrawInlineArray(PrimitiveType primitive, uint32_t dwordCount, const void* data)
{
    // The vertex cache is flushed first: inline vertices bypass the streams.
    InvalidateVertexCache();
    if (dwordCount == 0)
        return;
    const size_t offset = PushData(data, dwordCount * 4);
    Push(cmd::DrawInline{primitive, dwordCount, offset});
}

// --- synchronisation, reports, flow control --------------------------------------

// @func 0x005E0570 cellGcmSetReferenceCommand
void CommandList::SetReference(uint32_t value)
{
    Push(cmd::Reference{value});
}

// @func 0x005E0688 cellGcmSetWaitLabel
void CommandList::WaitLabel(uint8_t index, uint32_t value)
{
    Push(cmd::WaitLabel{index, value});
}

// @func 0x005E0E50 cellGcmSetWaitFlip
void CommandList::WaitFlip()
{
    // libgcm signals flips through label 1.
    WaitLabel(1, 0);
}

// @func 0x005E2650 cellGcmSetWriteBackEndLabel
void CommandList::WriteBackEndLabel(uint8_t index, uint32_t value)
{
    Push(cmd::WriteLabel{index, value});
}

// @func 0x005E24F8 cellGcmSetTimeStamp
void CommandList::SetTimeStamp(uint32_t index)
{
    Push(cmd::TimeStamp{index});
}

// @func 0x005E31B8 cellGcmSetZpassPixelCountEnable
void CommandList::SetZPassPixelCountEnable(bool enable)
{
    Push(cmd::ZPassPixelCountEnable{enable});
}

// @func 0x005E3248 cellGcmSetClearReport
void CommandList::ClearReport(ReportType type)
{
    Push(cmd::ClearReport{type});
}

// @func 0x005E32D8 cellGcmSetReport
void CommandList::GetReport(ReportType type, uint32_t index)
{
    Push(cmd::GetReport{type, index});
}

// @func 0x005E337C cellGcmSetReportLocation
void CommandList::SetReportLocation(MemoryLocation location)
{
    Push(cmd::ReportLocation{location});
}

// @func 0x005E3C00 cellGcmSetCallCommand
void CommandList::Call(const CommandList& target)
{
    Push(cmd::Call{&target});
}

// @func 0x005E3C1C cellGcmSetReturnCommand
void CommandList::Return()
{
    Push(cmd::Return{});
}

// @func 0x005E0600 cellGcmSetJumpCommand
void CommandList::Jump(const CommandList& target)
{
    Push(cmd::Jump{&target});
}

// @func 0x005E0AF0 cellGcmSetNopCommand
void CommandList::InsertNops(uint32_t count)
{
    if (count)
        Push(cmd::Nop{count});
}

// @func 0x005DFA38 cellGcmSetFlip
bool CommandList::Flip(uint8_t buffer)
{
    // Up to 8 display buffers can be registered.
    if (buffer > 7)
        return false;
    return Push(cmd::Flip{buffer});
}

// @func 0x005E0EB4 cellGcmSetInlineTransfer
void CommandList::InlineTransfer(GpuPtr dst, const uint32_t* data, uint32_t dwordCount)
{
    if (dwordCount == 0)
        return;
    const size_t offset = PushData(data, dwordCount * 4);
    Push(cmd::InlineTransfer{dst, dwordCount, offset});
}

// @func 0x005E104C cellGcmSetTransferImage
// @func 0x005E47AC cellGcmSetTransferImage (unchecked)
void CommandList::CopyImage(const cmd::CopyImage& copy)
{
    // The console split copies into 1024x1024 tiles; that is a hardware
    // limit a backend handles itself.
    if (copy.width == 0 || copy.height == 0)
        return;
    Push(copy);
}

} // namespace agl::gpu
