// openTIT - Ape engine wrappers around the system utility callbacks.
#pragma once

#include "platform/Platform.h"

namespace ape {

// Registers a system event handler in one of the platform callback slots.
// Serialised against SysUtilCheckCallbacks() with the engine's sysutil lock.
bool SysUtilRegisterCallback(int slot, plat::SystemEventCallback callback, void* userdata);

// Pumps pending system events into the registered handlers.
void SysUtilCheckCallbacks();

} // namespace ape
