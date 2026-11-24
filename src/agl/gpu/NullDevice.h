// openTIT - Null GPU backend: validates and accounts for commands, draws nothing.
//
// Lets the game run with no graphics API. Every executed command updates a
// shadow of the GPU state, and draws are checked against it (programs bound,
// valid texture units and attribute streams, a render target set, ...).
// Problems are counted in DeviceStats::validationErrors and the first few are
// reported on stdout. Labels, references, reports and time stamps behave as
// if the GPU had completed the work immediately.
#pragma once

#include "agl/gpu/Device.h"

#include <array>
#include <chrono>

namespace agl::gpu {

class NullDevice final : public Device {
public:
    NullDevice();

    const char* GetName() const override { return "null"; }

    void Execute(const CommandList& list) override;
    void WaitIdle() override {}

    bool SetDisplayBuffer(uint8_t id, const DisplayBuffer& buffer) override;
    void SetFlipMode(FlipMode mode) override { m_flipMode = mode; }
    bool IsFlipDone() const override { return m_flipDone; }
    void ResetFlipStatus() override { m_flipDone = false; }
    uint8_t GetCurrentDisplayBuffer() const override { return m_currentBuffer; }
    void SetGamma(float gamma) override { m_gamma = gamma; }

    uint32_t GetReference() const override { return m_reference; }
    uint32_t GetLabel(uint8_t index) const override { return m_labels[index]; }
    void SetLabel(uint8_t index, uint32_t value) override { m_labels[index] = value; }
    uint64_t GetTimeStamp(uint32_t index) const override;
    uint32_t GetReport(uint32_t index) const override;

    const DeviceStats& GetStats() const override { return m_stats; }
    void ResetStats() override { m_stats = DeviceStats{}; }

private:
    struct Shadow {
        bool vertexProgram = false;
        bool fragmentProgram = false;
        bool surface = false;
        uint32_t vertexInputMask = 0;
        uint32_t enabledAttributes = 0;
        uint32_t boundTextures = 0;
        SurfaceDesc surfaceDesc;
        bool zpassCounting = false;
    };

    void Run(const CommandList& list, int depth);
    void Report(const char* what);
    void ValidateDraw(PrimitiveType primitive, uint32_t count);
    uint64_t Now() const;

    Shadow m_state;
    DeviceStats m_stats;
    std::array<DisplayBuffer, 8> m_displayBuffers{};
    std::array<uint32_t, kMaxLabels> m_labels{};
    std::array<uint64_t, kMaxReportSlots> m_timeStamps{};
    std::array<uint32_t, kMaxReportSlots> m_reports{};
    uint32_t m_reference = 0;
    uint32_t m_zpassPixels = 0;
    FlipMode m_flipMode = FlipMode::VSync;
    bool m_flipDone = true;
    uint8_t m_currentBuffer = 0;
    float m_gamma = 1.0f;
    std::chrono::steady_clock::time_point m_start;
};

} // namespace agl::gpu
