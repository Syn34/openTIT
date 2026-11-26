// openTIT - Application run-time state helpers.
#include "sly/game/AppRuntime.h"

#include "ape/core/ApeSystem.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/level/ApeGui.h"
#include "sly/SlyApp.h"

#include <cstring>

namespace crt {
[[noreturn]] void ExitImmediate(int code);
}

namespace sly {

namespace {
AppRuntime g_appRuntime;
bool g_shuttingDown = false;      // 0x010C5A45
bool g_skipCheckEnabled = false;  // 0x00F65C84
} // namespace

// @func 0x00457100 AppPlayer::~AppPlayer
// @func 0x004643F8 AppPlayer::~AppPlayer (deleting)
AppPlayer::~AppPlayer()
{
    if (pad) {
        pad->StopRumble(0);
        pad = nullptr;
    }
}

// @func 0x00464488 AppPlayer::Rumble
void AppPlayer::Rumble(float intensity, float duration)
{
    if (pad && pad->IsRumbleEnabled())
        pad->StartRumble(intensity, duration, 0);
}

// @func 0x004644E0 AppPlayer::StopRumble
void AppPlayer::StopRumble()
{
    if (pad)
        pad->StopRumble(0);
}

// @func 0x0046452C AppPlayer::SetRumblePaused
void AppPlayer::SetRumblePaused(bool paused)
{
    if (pad)
        pad->SetRumblePaused(paused);
}

// @func 0x00464578 AppPlayer::SetField_D0
void AppPlayer::SetField_D0(const uint32_t value[2])
{
    field_D0[0] = value[0];
    field_D0[1] = value[1];
}

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

// @func 0x00464794 GuiPadFilterForPlayers
bool GuiPadFilterForPlayers(uint32_t pad, ape::GuiPadInput* input)
{
    if (pad >= uint32_t(kLocalPlayerCount))
        return false;
    const AppPlayer& player = GetAppRuntime().players[pad];
    input->buttons = player.inputFlags;
    input->edges = player.inputMask;
    // NOTE: the word at +0xC4 is handed over as the input flags; the port
    // calls it the analog mask.
    input->flags = player.analogMask;
    std::memcpy(&input->cursor[0], &player.field_D0[0], sizeof(float));
    std::memcpy(&input->cursor[1], &player.field_D0[1], sizeof(float));
    if (player.field_B5 & 0x80)
        input->flags |= ape::kGuiPadInput_Cursor;
    return true;
}

} // namespace sly
