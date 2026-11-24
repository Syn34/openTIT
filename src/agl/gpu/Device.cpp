// openTIT - GPU device interface (backend boundary of the Agl graphics layer).
#include "agl/gpu/Device.h"

#include "agl/gpu/NullDevice.h"

#include <memory>

namespace agl::gpu {

namespace {

std::unique_ptr<Device> g_device;
size_t g_commandBudget = 0;

} // namespace

Device* GetDevice()
{
    return g_device.get();
}

// @func 0x005E0530 cellGcmInit
bool InitDevice(size_t commandBudget)
{
    // Only the null backend exists so far; a real backend would be chosen
    // here (command line or configuration).
    g_device = std::make_unique<NullDevice>();
    g_commandBudget = commandBudget;
    return g_device != nullptr;
}

void ShutdownDevice()
{
    g_device.reset();
}

// @func 0x005E17BC cellGcmFlush
void Flush(CommandList& list)
{
    if (g_device)
        g_device->Execute(list);
    list.Reset(g_commandBudget);
}

// @func 0x005E3B84 cellGcmFinish
void Finish(CommandList& list, uint32_t reference)
{
    list.SetReference(reference);
    Flush(list);
    if (!g_device)
        return;
    // The console spun on the control register until the GPU reported the
    // reference value.
    while (g_device->GetReference() != reference)
        g_device->WaitIdle();
}

} // namespace agl::gpu
