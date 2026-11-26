// openTIT - Ape engine bring-up.
#include "ape/core/ApeSystem.h"
#include "ape/sys/ApePlatform.h"
#include "ape/core/ApeThread.h"
#include "ape/world/ApeWorldInit.h"
#include "ape/sys/ApeCoreSystem.h"
#include "ape/sys/ApeFrameClock.h"
#include "ape/memory/ApeDebugPrim.h"
#include "ape/memory/ApeMemStack.h"
#include "agl/AglInit.h"

namespace ape {

EngineConfig g_engineConfig;
bool g_engineFlag_10F0EA8 = false;

// @func 0x005DB1D0 SystemInit
bool SystemInit(uint32_t flags)
{
    if (flags & kInit_Core) {
        RegisterMainThread();
        if (!PlatformSystemInit() || !CoreSystemInit())
            return false;
    }
    if ((flags & kInit_Agl) && !agl::AglInit())
        return false;
    if ((flags & kInit_Graphics) && !world::InitWorld())
        return false;
    if ((flags & kInit_Debug) && !ApeDebug::Init())
        return false;

    // NOTE: the console also publishes the configuration, a runtime context
    // (0x0118E0D0) and the frame clock in a table of engine globals
    // (0x011972E0..0x011972E8) for code outside the main executable.
    FrameClock& clock = GetFrameClock();
    clock.field_0C = g_engineConfig.field_014;
    static bool scratchCreated = false;
    if (!scratchCreated) {
        scratchCreated = true;
        int32_t size = int32_t(g_engineConfig.field_1A4);
        if (size == 0)
            size = 0x8000;
        else if (size < 0)
            size = 0;
        clock.scratchStack = new ApeMemStack(size_t(size), kMemClass_Default, false);
    }
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
