// openTIT - Ape engine input manager: owns the input devices.
#pragma once

#include "ape/input/ApeInputDevice.h"
#include "ape/memory/ApeTFreeList.h"

namespace ape::input {

class ApeInputMgr {
public:
    // Creates the device for the next pad port (ports are handed out in
    // creation order). Returns null if the pool is exhausted.
    static ApePadDevice* CreatePad();
    // Polls every device, then honours a pending controller recording stop.
    static void Update();
    // Destroys every device and shuts the pad library down.
    static bool Shutdown();

    // Calls fn(ApeInputDevice*) for every live device, newest first.
    template <class Fn>
    static void ForEachDevice(Fn&& fn);

private:
    static ApeFreeList& GetAllocator();
};

// Pad library bring-up / shutdown (cellPadInit / cellPadEnd on the console).
// Shutdown is a no-op that reports success in the shipping build.
bool PadSystemInit();
bool PadSystemShutdown();

template <class Fn>
void ApeInputMgr::ForEachDevice(Fn&& fn)
{
    ApeFreeList& pool = GetAllocator();
    for (ApeFreeList::Node* node = pool.First(); node;) {
        ApeFreeList::Node* next = node->next;
        fn(static_cast<ApeInputDevice*>(ApeFreeList::NodeData(node)));
        node = next;
    }
}

} // namespace ape::input
