// openTIT - Ape engine input manager: owns the input devices.
#include "ape/input/ApeInputMgr.h"


#include <new>

namespace ape::input {

namespace {
// Every device type is carved from one pool sized for the largest device
// (0x128 bytes, 4 per block on the console).
constexpr uint32_t kDevicesPerBlock = 4;
}

// The pool is created on first use (inlined into every caller on the console).
ApeFreeList& ApeInputMgr::GetAllocator()
{
    static ApeFreeList* s_pool = nullptr;
    if (!s_pool) {
        s_pool = new ApeFreeList();
        s_pool->SetName("static ApeTFreeList<ApeInputDevice> &ApeInputMgr::GetAllocator()");
        s_pool->Init(sizeof(ApePadDevice), kDevicesPerBlock, nullptr, false);
    }
    return *s_pool;
}

// @func 0x0056324C ApeInputMgr::CreatePad
ApePadDevice* ApeInputMgr::CreatePad()
{
    void* storage = GetAllocator().Alloc();
    return storage ? ::new (storage) ApePadDevice() : nullptr;
}

// @func 0x005630D4 ApeInputMgr::Update
void ApeInputMgr::Update()
{
    ForEachDevice([](ApeInputDevice* device) { device->Poll(); });
    if (ConsumeControllerRecordStop())
        SetControllerRecordMode(kRecord_Off);
}

// @func 0x0055E56C ApeInputMgr::Shutdown
bool ApeInputMgr::Shutdown()
{
    ApeFreeList& pool = GetAllocator();
    ForEachDevice([&pool](ApeInputDevice* device) {
        device->~ApeInputDevice();
        pool.Free(device);
    });
    PadSystemShutdown();
    return true;
}

// Clears the pad state shared by the devices and starts the pad library
// (cellPadInit with 4 pads on the console).
// @func 0x0056F63C PadSystemInit
bool PadSystemInit()
{
    ResetPadSharedState();
    return true;
}

// @func 0x0056F690 PadSystemShutdown
bool PadSystemShutdown()
{
    return true;
}

} // namespace ape::input
