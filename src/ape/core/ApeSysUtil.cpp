// openTIT - Ape engine wrappers around the system utility callbacks.
#include "ape/core/ApeSysUtil.h"

namespace ape {

namespace {
plat::Mutex& SysUtilLock()
{
    static plat::Mutex s_lock;
    return s_lock;
}
}

// @func 0x0056C1E8 SysUtilRegisterCallback
bool SysUtilRegisterCallback(int slot, plat::SystemEventCallback callback, void* userdata)
{
    plat::ScopedLock lock(SysUtilLock());
    return plat::RegisterSystemCallback(slot, callback, userdata);
}

// @func 0x0056C274 SysUtilUnregisterCallback
bool SysUtilUnregisterCallback(int slot)
{
    plat::ScopedLock lock(SysUtilLock());
    return plat::UnregisterSystemCallback(slot);
}

// @func 0x0056C184 SysUtilCheckCallbacks
void SysUtilCheckCallbacks()
{
    plat::ScopedLock lock(SysUtilLock());
    plat::DispatchSystemCallbacks();
}

} // namespace ape
