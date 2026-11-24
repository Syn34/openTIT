// openTIT - Application run-time state helpers.
#include "sly/game/AppRuntime.h"

#include "ape/core/ApeSystem.h"
#include "sly/SlyApp.h"

namespace crt {
[[noreturn]] void ExitImmediate(int code);
}

namespace sly {

namespace {
AppRuntime g_appRuntime;
bool g_shuttingDown = false;      // 0x010C5A45
bool g_skipCheckEnabled = false;  // 0x00F65C84
} // namespace

AppRuntime& GetAppRuntime()
{
    return g_appRuntime;
}

bool AppIsShuttingDown()
{
    return g_shuttingDown;
}

void SetAppShuttingDown(bool value)
{
    g_shuttingDown = value;
}

// @func 0x00458B1C AppExit
void AppExit()
{
    AppShutdown(g_app);
    ape::PostGameShutdownHook();
    crt::ExitImmediate(0);
}

// @func 0x004645A0 AppSetPauseRequest
void AppSetPauseRequest(bool requested, uint32_t value)
{
    g_appRuntime.pauseRequested = requested;
    g_appRuntime.pauseValue = value;
}

void SetSkipCheckEnabled(bool enabled)
{
    g_skipCheckEnabled = enabled;
}

// @func 0x0048BEC8 AnyPlayerRequestsSkip
bool AnyPlayerRequestsSkip()
{
    if (!g_skipCheckEnabled)
        return false;
    for (const AppPlayer& player : g_appRuntime.players) {
        if (player.inputFlags & player.inputMask & 0x40u)
            return true;
    }
    return false;
}

} // namespace sly
