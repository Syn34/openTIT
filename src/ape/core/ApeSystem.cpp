// openTIT - Ape engine bring-up.
#include "ape/core/ApeSystem.h"
#include "ape/sys/ApePlatform.h"
#include "ape/core/ApeThread.h"
#include "ape/world/ApeWorldInit.h"

namespace ape {

EngineConfig g_engineConfig;
bool g_engineFlag_10F0EA8 = false;

// TODO(0x005DB1D0 SystemInit): only the thread registration of the core group
// and the platform initialiser (0x0056D87C) are reimplemented. The original
// then calls the core engine init (0x0055DC18), the Agl graphics init
// (0x0050F520, flag 4), the world module init (flag 2, ported) and the
// debug draw init (0x0056A944, flag 8), and creates the general heap sized
// by EngineConfig::field_1A4.
bool SystemInit(uint32_t flags)
{
    if (flags & kInit_Core) {
        RegisterMainThread();
        if (!PlatformSystemInit())
            return false;
    }
    if ((flags & kInit_Graphics) && !world::InitWorld())
        return false;
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
