// openTIT - Null GPU backend: validates and accounts for commands, draws nothing.
#include "agl/gpu/NullDevice.h"

#include <cstdio>
#include <type_traits>

namespace agl::gpu {

namespace {

constexpr int kMaxCallDepth = 8;          // the RSX supported one level; Agl never nests deeper
constexpr uint32_t kMaxReportedErrors = 16;

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

bool IsValidPrimitive(PrimitiveType p)
{
    return p >= PrimitiveType::Points && p <= PrimitiveType::Polygon;
}

} // namespace

NullDevice::NullDevice()
    : m_start(std::chrono::steady_clock::now())
{
}

uint64_t NullDevice::Now() const
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - m_start).count());
}

uint64_t NullDevice::GetTimeStamp(uint32_t index) const
{
    return index < m_timeStamps.size() ? m_timeStamps[index] : 0;
}

uint32_t NullDevice::GetReport(uint32_t index) const
{
    return index < m_reports.size() ? m_reports[index] : 0;
}

bool NullDevice::SetDisplayBuffer(uint8_t id, const DisplayBuffer& buffer)
{
    if (id >= m_displayBuffers.size() || buffer.memory.IsNull() || buffer.width == 0 || buffer.height == 0)
        return false;
    m_displayBuffers[id] = buffer;
    return true;
}

void NullDevice::Report(const char* what)
{
    if (m_stats.validationErrors++ < kMaxReportedErrors)
        std::printf("gpu(null): %s\n", what);
}

void NullDevice::ValidateDraw(PrimitiveType primitive, uint32_t count)
{
    ++m_stats.drawCalls;
    m_stats.vertices += count;
    if (!IsValidPrimitive(primitive))
        Report("draw with an invalid primitive type");
    if (!m_state.vertexProgram)
        Report("draw without a vertex program");
    if (!m_state.fragmentProgram)
        Report("draw without a fragment program");
    if (!m_state.surface)
        Report("draw without a render surface");
    // Every attribute the vertex program reads needs a stream (inline draws
    // supply their own data, so only check when streams are used at all).
    if (m_state.enabledAttributes && (m_state.vertexInputMask & ~m_state.enabledAttributes))
        Report("vertex program reads an attribute with no stream");
    if (m_state.zpassCounting)
        m_zpassPixels += count;   // a stand-in for the samples that would pass
}

void NullDevice::Execute(const CommandList& list)
{
    ++m_stats.listsExecuted;
    Run(list, 0);
}

void NullDevice::Run(const CommandList& list, int depth)
{
    if (depth > kMaxCallDepth) {
        Report("command list call depth exceeded");
        return;
    }
    for (const Command& command : list.GetCommands()) {
        ++m_stats.commands;
        bool stop = false;
        std::visit(Overloaded{
            [&](const cmd::Texture& c) {
                ++m_stats.textureBinds;
                if (c.unit >= kMaxTextureUnits) {
                    Report("texture unit out of range");
                    return;
                }
                if (c.desc.memory.IsNull() || c.desc.width == 0 || c.desc.height == 0)
                    Report("texture with no image");
                m_state.boundTextures |= 1u << c.unit;
            },
            [&](const cmd::TextureControl& c) {
                if (c.unit >= kMaxTextureUnits)
                    Report("texture unit out of range");
                else if (!c.control.enable)
                    m_state.boundTextures &= ~(1u << c.unit);
            },
            [&](const cmd::VertexProgram& c) {
                ++m_stats.programBinds;
                if (c.desc.instructionCount == 0)
                    Report("empty vertex program");
                m_state.vertexProgram = true;
                m_state.vertexInputMask = c.desc.inputMask;
            },
            [&](const cmd::VertexConstants& c) {
                if (c.first * 4 + c.count > uint32_t(kMaxVertexConstants) * 4)
                    Report("vertex constants out of range");
            },
            [&](const cmd::FragmentProgram& c) {
                ++m_stats.programBinds;
                if (c.desc.ucode.IsNull() || c.desc.ucodeSize == 0)
                    Report("fragment program with no microcode");
                m_state.fragmentProgram = true;
            },
            [&](const cmd::FragmentProgramAddress& c) {
                if (!m_state.fragmentProgram)
                    Report("fragment program address set before the program");
                if (c.ucode.IsNull())
                    Report("fragment program moved to null memory");
            },
            [&](const cmd::VertexAttribute& c) {
                if (c.index >= kMaxVertexAttributes) {
                    Report("vertex attribute out of range");
                    return;
                }
                if (c.size == 0)
                    m_state.enabledAttributes &= ~(1u << c.index);
                else if (c.memory.IsNull())
                    Report("vertex stream with no memory");
                else
                    m_state.enabledAttributes |= 1u << c.index;
            },
            [&](const cmd::Surface& c) {
                m_state.surface = true;
                m_state.surfaceDesc = c.desc;
                if (c.desc.width == 0 || c.desc.height == 0)
                    Report("empty render surface");
            },
            [&](const cmd::ClearSurface&) { ++m_stats.clears; },
            [&](const cmd::DrawArrays& c) { ValidateDraw(c.primitive, c.count); },
            [&](const cmd::DrawIndexed& c) {
                if (c.indices.IsNull())
                    Report("indexed draw with no index buffer");
                ValidateDraw(c.primitive, c.count);
            },
            [&](const cmd::DrawInline& c) { ValidateDraw(c.primitive, c.dwordCount); },
            [&](const cmd::Reference& c) { m_reference = c.value; },
            [&](const cmd::WaitLabel& c) {
                // Work is complete as soon as it is executed, so a wait on a
                // value nobody wrote can never be satisfied.
                if (m_labels[c.index] != c.value && c.index != 1)
                    Report("wait on a label that was never written");
            },
            [&](const cmd::WriteLabel& c) { m_labels[c.index] = c.value; },
            [&](const cmd::TimeStamp& c) {
                if (c.index < m_timeStamps.size())
                    m_timeStamps[c.index] = Now();
            },
            [&](const cmd::ZPassPixelCountEnable& c) { m_state.zpassCounting = c.enable; },
            [&](const cmd::ClearReport&) { m_zpassPixels = 0; },
            [&](const cmd::GetReport& c) {
                if (c.index < m_reports.size())
                    m_reports[c.index] = m_zpassPixels;
            },
            [&](const cmd::Call& c) {
                if (c.target)
                    Run(*c.target, depth + 1);
            },
            [&](const cmd::Return&) { stop = true; },
            [&](const cmd::Jump& c) {
                if (c.target)
                    Run(*c.target, depth);
                stop = true;
            },
            [&](const cmd::Flip& c) {
                ++m_stats.flips;
                if (m_displayBuffers[c.buffer].memory.IsNull())
                    Report("flip to an unregistered display buffer");
                m_currentBuffer = c.buffer;
                m_flipDone = true;
            },
            [&](const cmd::InlineTransfer& c) {
                if (c.dst.IsNull())
                    Report("inline transfer to null memory");
            },
            [&](const cmd::CopyImage& c) {
                if (c.dst.IsNull() || c.src.IsNull())
                    Report("image copy with null memory");
            },
            [&](const auto&) { ++m_stats.stateChanges; },
        }, command);
        if (stop)
            break;
    }
}

} // namespace agl::gpu
