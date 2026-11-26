// openTIT - Recorded GPU command list (the PC replacement for libgcm command buffers).
//
// On the console Agl wrote RSX method words into libgcm command buffers,
// partly through out-of-line libgcm helpers (cellGcmSetTexture,
// cellGcmSetBlendFunc, cellGcmSetDrawArrays, ...). On PC the same operations
// are recorded as typed, backend-neutral commands; a Device backend executes
// them (see Device.h). Each recording method is named after what it does
// rather than after the RSX method it used to emit.
//
// Variable-sized payloads (constants, inline vertex data, ...) are copied
// into the list, so callers may reuse their buffers immediately. GPU memory
// referenced through GpuPtr (textures, vertex streams, fragment programs) is
// not copied and must stay alive until the list has been executed, exactly
// as on the console.
//
// Lists can call other lists (Call/Return): Agl pre-records small command
// blocks (for example material or post-process state) and calls them from
// the frame list.
#pragma once

#include "agl/gpu/GpuTypes.h"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace agl::gpu {

class CommandList;
struct CgProgram;

namespace cmd {

// --- render state ---
struct AlphaTestEnable   { bool enable; };
struct AlphaFunc         { CompareFunc func; uint32_t ref; };
struct BlendEnable       { bool enable; };
struct BlendEnableMrt    { uint32_t mask; };          // targets 1..3
struct BlendFunc         { BlendFactor srcColor, dstColor, srcAlpha, dstAlpha; };
struct BlendEquationCmd  { BlendEquation color, alpha; };
struct BlendColor        { uint32_t argb; };
struct ColorMaskCmd      { uint32_t mask; };
struct ColorMaskMrt      { uint32_t mask; };
struct DepthTestEnable   { bool enable; };
struct DepthFunc         { CompareFunc func; };
struct DepthMask         { bool write; };
struct DepthBoundsTestEnable { bool enable; };
struct DepthBounds       { float zMin, zMax; };
struct StencilTestEnable { bool enable; };
struct StencilMask       { uint32_t mask; };
struct StencilFunc       { CompareFunc func; int32_t ref; uint32_t mask; };
struct StencilOpCmd      { StencilOp fail, depthFail, depthPass; };
struct CullFaceEnable    { bool enable; };
struct CullFaceCmd       { CullFace face; };
struct FrontFaceCmd      { FrontFace face; };
struct PolygonOffset     { bool enable; float factor, units; };
struct LineWidth         { float width; };            // pixels
struct RestartIndexEnable { bool enable; };
struct RestartIndex      { uint32_t index; };
struct UserClipPlaneControl { UserClipMode plane[kMaxUserClipPlanes]; };
struct AntiAliasingControl { bool enable, alphaToCoverage, alphaToOne; uint16_t sampleMask; };
struct ViewportCmd       { Viewport viewport; };
struct Scissor           { uint16_t x, y, width, height; };
struct Surface           { SurfaceDesc desc; };
struct ClearColor        { uint32_t argb; };
struct ClearDepthStencil { uint32_t value; };          // depth << 8 | stencil
struct ClearSurface      { uint32_t mask; };           // ClearMask

// --- textures ---
struct Texture           { uint8_t unit; TextureDesc desc; };
struct TextureControl    { uint8_t unit; SamplerControl control; };
struct TextureAddress    { uint8_t unit; SamplerAddress address; };
struct TextureFilter     { uint8_t unit; SamplerFilter filter; };
struct TextureBorderColor { uint8_t unit; uint32_t argb; };
struct InvalidateTextureCache { TextureCacheKind kind; };

// --- programs ---
struct VertexProgram     { VertexProgramDesc desc; size_t ucodeOffset; };
struct VertexConstants   { uint32_t first; uint32_t count; size_t dataOffset; };   // count floats
struct TransformBranchBits { uint32_t bits; };
struct FragmentProgram   { FragmentProgramDesc desc; };
struct FragmentProgramAddress { GpuPtr ucode; };

// --- vertex input and drawing ---
struct VertexAttribute   { uint8_t index; uint16_t frequency; uint8_t stride; uint8_t size;
                           VertexAttribType type; GpuPtr memory; };
struct InvalidateVertexCache {};
struct DrawArrays        { PrimitiveType primitive; uint32_t first; uint32_t count; };
struct DrawIndexed       { PrimitiveType primitive; uint32_t count; IndexType indexType; GpuPtr indices; };
struct DrawInline        { PrimitiveType primitive; uint32_t dwordCount; size_t dataOffset; };

// --- synchronisation, reports, flow ---
struct Reference         { uint32_t value; };
struct WaitLabel         { uint8_t index; uint32_t value; };
struct WriteLabel        { uint8_t index; uint32_t value; };
struct TimeStamp         { uint32_t index; };
struct ZPassPixelCountEnable { bool enable; };
struct ClearReport       { ReportType type; };
struct GetReport         { ReportType type; uint32_t index; };
struct ReportLocation    { MemoryLocation location; };
struct Call              { const CommandList* target; };
struct Return            {};
struct Jump              { const CommandList* target; };
struct Nop               { uint32_t count; };
struct Flip              { uint8_t buffer; };
struct InlineTransfer    { GpuPtr dst; uint32_t dwordCount; size_t dataOffset; };
struct CopyImage         { GpuPtr dst; uint32_t dstPitch; uint16_t dstX, dstY;
                           GpuPtr src; uint32_t srcPitch; uint16_t srcX, srcY;
                           uint16_t width, height; uint8_t bytesPerPixel; };

} // namespace cmd

using Command = std::variant<
    cmd::AlphaTestEnable, cmd::AlphaFunc, cmd::BlendEnable, cmd::BlendEnableMrt, cmd::BlendFunc,
    cmd::BlendEquationCmd, cmd::BlendColor, cmd::ColorMaskCmd, cmd::ColorMaskMrt,
    cmd::DepthTestEnable, cmd::DepthFunc, cmd::DepthMask, cmd::DepthBoundsTestEnable, cmd::DepthBounds,
    cmd::StencilTestEnable, cmd::StencilMask, cmd::StencilFunc, cmd::StencilOpCmd,
    cmd::CullFaceEnable, cmd::CullFaceCmd, cmd::FrontFaceCmd, cmd::PolygonOffset, cmd::LineWidth,
    cmd::RestartIndexEnable, cmd::RestartIndex, cmd::UserClipPlaneControl, cmd::AntiAliasingControl,
    cmd::ViewportCmd, cmd::Scissor, cmd::Surface, cmd::ClearColor, cmd::ClearDepthStencil, cmd::ClearSurface,
    cmd::Texture, cmd::TextureControl, cmd::TextureAddress, cmd::TextureFilter, cmd::TextureBorderColor,
    cmd::InvalidateTextureCache,
    cmd::VertexProgram, cmd::VertexConstants, cmd::TransformBranchBits, cmd::FragmentProgram,
    cmd::FragmentProgramAddress,
    cmd::VertexAttribute, cmd::InvalidateVertexCache, cmd::DrawArrays, cmd::DrawIndexed, cmd::DrawInline,
    cmd::Reference, cmd::WaitLabel, cmd::WriteLabel, cmd::TimeStamp, cmd::ZPassPixelCountEnable,
    cmd::ClearReport, cmd::GetReport, cmd::ReportLocation, cmd::Call, cmd::Return, cmd::Jump, cmd::Nop,
    cmd::Flip, cmd::InlineTransfer, cmd::CopyImage>;

class CommandList {
public:
    // Called when the list grows past its budget (see SetBudget). The console
    // version was called when the fixed-size command buffer ran out of space;
    // returning false aborts the command that was being recorded.
    using OverflowCallback = bool (*)(CommandList& list, size_t commandsNeeded);

    CommandList() = default;
    explicit CommandList(const char* name) : m_name(name) {}

    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    // Empties the list for re-recording and installs the overflow handler.
    void Reset(size_t budget = 0, OverflowCallback callback = nullptr);
    // Makes sure 'count' more commands fit without triggering the overflow handler.
    bool Reserve(size_t count);
    void SetBudget(size_t budget) { m_budget = budget; }

    const char* GetName() const { return m_name; }
    size_t GetCommandCount() const { return m_commands.size(); }
    const std::vector<Command>& GetCommands() const { return m_commands; }
    const uint8_t* GetData(size_t offset) const { return m_data.data() + offset; }
    size_t GetDataSize() const { return m_data.size(); }

    // --- render state ---
    void SetAlphaTestEnable(bool enable);
    void SetAlphaFunc(CompareFunc func, uint32_t ref);
    void SetBlendEnable(bool enable);
    void SetBlendEnableMrt(bool target1, bool target2, bool target3);
    void SetBlendFunc(BlendFactor srcColor, BlendFactor dstColor, BlendFactor srcAlpha, BlendFactor dstAlpha);
    void SetBlendEquation(BlendEquation color, BlendEquation alpha);
    void SetBlendColor(uint32_t argb);
    void SetColorMask(uint32_t mask);
    void SetColorMaskMrt(uint32_t mask);
    void SetDepthTestEnable(bool enable);
    void SetDepthFunc(CompareFunc func);
    void SetDepthMask(bool write);
    void SetDepthBoundsTestEnable(bool enable);
    void SetDepthBounds(float zMin, float zMax);
    void SetStencilTestEnable(bool enable);
    void SetStencilMask(uint32_t mask);
    void SetStencilFunc(CompareFunc func, int32_t ref, uint32_t mask);
    void SetStencilOp(StencilOp fail, StencilOp depthFail, StencilOp depthPass);
    void SetCullFaceEnable(bool enable);
    void SetCullFace(CullFace face);
    void SetFrontFace(FrontFace face);
    void SetPolygonOffset(bool enable, float factor, float units);
    void SetLineWidth(float width);
    void SetRestartIndexEnable(bool enable);
    void SetRestartIndex(uint32_t index);
    void SetUserClipPlaneControl(UserClipMode p0, UserClipMode p1, UserClipMode p2,
                                 UserClipMode p3, UserClipMode p4, UserClipMode p5);
    void SetAntiAliasingControl(bool enable, bool alphaToCoverage, bool alphaToOne, uint16_t sampleMask);
    void SetViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float zMin, float zMax,
                     const float scale[4], const float offset[4]);
    void SetScissor(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void SetSurface(const SurfaceDesc& surface);
    void SetClearColor(uint32_t argb);
    void SetClearDepthStencil(uint32_t value);
    void ClearSurface(uint32_t mask);

    // --- textures ---
    void SetTexture(uint8_t unit, const TextureDesc& texture);
    void SetTextureControl(uint8_t unit, bool enable, uint16_t minLod, uint16_t maxLod, uint8_t maxAniso);
    void SetTextureAddress(uint8_t unit, TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR,
                           uint8_t unsignedRemap, uint8_t depthCompare, uint8_t gamma);
    void SetTextureFilter(uint8_t unit, uint16_t lodBias, TextureMinFilter min, TextureMagFilter mag,
                          uint8_t convolution);
    void SetTextureBorderColor(uint8_t unit, uint32_t argb);
    void InvalidateTextureCache(TextureCacheKind kind);

    // --- programs ---
    // Loads a vertex program and makes it current.
    void SetVertexProgram(const VertexProgramDesc& program);
    // Loads a vertex program described by a Cg binary, then applies the
    // default values of its uniform parameters.
    void SetVertexProgram(const CgProgram& program, const void* ucode);
    // Uploads 'count' floats to the vertex constant registers starting at
    // register 'first' (4 floats per register).
    void SetVertexProgramConstants(uint32_t first, uint32_t count, const float* values);
    void SetTransformBranchBits(uint32_t bits);
    void SetFragmentProgram(const FragmentProgramDesc& program);
    // Moves the current fragment program to another copy of its microcode
    // (used after patching embedded constants); the rest of the program
    // state stays as set by SetFragmentProgram.
    void SetFragmentProgramAddress(GpuPtr ucode);

    // --- vertex input and drawing ---
    void SetVertexAttribute(uint8_t index, uint16_t frequency, uint8_t stride, uint8_t size,
                            VertexAttribType type, GpuPtr memory);
    void DisableVertexAttribute(uint8_t index);
    void InvalidateVertexCache();
    void DrawArrays(PrimitiveType primitive, uint32_t first, uint32_t count);
    void DrawIndexArray(PrimitiveType primitive, uint32_t count, IndexType indexType, GpuPtr indices);
    // Draws vertices whose attribute data is embedded in the list.
    void DrawInlineArray(PrimitiveType primitive, uint32_t dwordCount, const void* data);

    // --- synchronisation, reports, flow control ---
    void SetReference(uint32_t value);
    void WaitLabel(uint8_t index, uint32_t value);
    void WaitFlip();
    void WriteBackEndLabel(uint8_t index, uint32_t value);
    void SetTimeStamp(uint32_t index);
    void SetZPassPixelCountEnable(bool enable);
    void ClearReport(ReportType type);
    void GetReport(ReportType type, uint32_t index);
    void SetReportLocation(MemoryLocation location);
    void Call(const CommandList& target);
    void Return();
    void Jump(const CommandList& target);
    void InsertNops(uint32_t count);
    bool Flip(uint8_t buffer);
    void InlineTransfer(GpuPtr dst, const uint32_t* data, uint32_t dwordCount);
    void CopyImage(const cmd::CopyImage& copy);

private:
    bool Push(const Command& command);
    size_t PushData(const void* data, size_t bytes);

    const char* m_name = "";
    std::vector<Command> m_commands;
    std::vector<uint8_t> m_data;
    size_t m_budget = 0;
    OverflowCallback m_overflow = nullptr;
};

} // namespace agl::gpu
