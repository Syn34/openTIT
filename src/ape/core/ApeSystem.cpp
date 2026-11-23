// openTIT - Ape engine bring-up.
#include "ape/core/ApeSystem.h"
#include "ape/core/ApeThread.h"

namespace ape {

EngineConfig g_engineConfig;
bool g_engineFlag_10F0EA8 = false;

// TODO(0x005DB1D0 SystemInit): only the thread registration of the core group
// is reimplemented. The original then calls the core (0x0056D87C, 0x0055DC18),
// audio (0x0050F520), graphics (0x00588828) and input (0x0056A944) initialisers
// and creates the general heap sized by EngineConfig::field_1A4.
bool SystemInit(uint32_t flags)
{
    if (flags & kInit_Core)
        RegisterMainThread();
    return true;
}

// @func 0x004A8AC0 PreGameInitHook
void PreGameInitHook()
{
    // Intentionally empty in the shipping build.
}

// @func 0x004A8AC4 PostGameShutdownHook
void PostGameShutdownHook()
{
    // Intentionally empty in the shipping build.
}

} // namespace ape
