// openTIT - GPU device interface (backend boundary of the Agl graphics layer).
//
// A Device executes recorded CommandLists and owns the display: the
// registered display buffers, flips and the GPU-written labels, reports and
// time stamps that the console exposed through libgcm (cellGcmGetLabelAddress,
// cellGcmGetTimeStamp, cellGcmGetReport, the control register's reference
// value, ...).
//
// The only backend in the tree is NullDevice (NullDevice.h), which validates
// and accounts for the commands without drawing anything. A D3D, GL or Vulkan
// backend implements the same interface.
#pragma once

#include "agl/gpu/CommandList.h"
#include "agl/gpu/GpuTypes.h"

#include <cstdint>

namespace agl::gpu {

struct DisplayBuffer {
    GpuPtr memory;
    uint32_t pitch = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

enum class FlipMode : uint8_t {
    VSync  = 1,
    HSync  = 2,   // flip immediately (tearing allowed)
};

struct DeviceStats {
    uint32_t listsExecuted = 0;
    uint32_t commands = 0;
    uint32_t drawCalls = 0;
    uint64_t vertices = 0;
    uint32_t stateChanges = 0;
    uint32_t textureBinds = 0;
    uint32_t programBinds = 0;
    uint32_t clears = 0;
    uint32_t flips = 0;
    uint32_t validationErrors = 0;
};

class Device {
public:
    virtual ~Device() = default;

    virtual const char* GetName() const = 0;

    // Runs every command of 'list' (and of the lists it calls).
    virtual void Execute(const CommandList& list) = 0;
    // Blocks until all executed work has completed.
    virtual void WaitIdle() = 0;

    // Display.
    virtual bool SetDisplayBuffer(uint8_t id, const DisplayBuffer& buffer) = 0;
    virtual void SetFlipMode(FlipMode mode) = 0;
    // True once the flip queued last has been shown.
    virtual bool IsFlipDone() const = 0;
    virtual void ResetFlipStatus() = 0;
    virtual uint8_t GetCurrentDisplayBuffer() const = 0;
    // Display gamma (1.0 = neutral).
    virtual void SetGamma(float gamma) = 0;

    // Values written by the GPU while executing lists.
    virtual uint32_t GetReference() const = 0;
    virtual uint32_t GetLabel(uint8_t index) const = 0;
    virtual void SetLabel(uint8_t index, uint32_t value) = 0;
    virtual uint64_t GetTimeStamp(uint32_t index) const = 0;
    virtual uint32_t GetReport(uint32_t index) const = 0;

    virtual const DeviceStats& GetStats() const = 0;
    virtual void ResetStats() = 0;
};

// The device Agl renders with (null until InitDevice).
Device* GetDevice();

// Brings up the GPU: selects the backend (the null backend when no other is
// available) and makes it current. 'commandBudget' is the default command
// budget of the frame lists (0 = unlimited).
bool InitDevice(size_t commandBudget);
void ShutdownDevice();

// Submits the commands recorded so far and empties the list.
void Flush(CommandList& list);
// Writes 'reference' at the end of 'list', submits it and waits until the
// GPU has reached it.
void Finish(CommandList& list, uint32_t reference);

} // namespace agl::gpu
