// openTIT - Application lifecycle.
#include "sly/SlyApp.h"
#include "sly/frame/FrameServices.h"

#include "ape/core/ApeString.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/input/ApeInputMgr.h"
#include "ape/level/ApeGui.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/tuning/ApeTuning.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/SlyGame.h"

namespace sly {

SlyApp g_app;
int32_t g_appState = kAppState_Starting;
uint32_t g_systemEventFlags = 0;

// TODO: hooks of the original start-up/shutdown that belong to other
// modules: 0x004D35AC (GUI system setup, argument 0), 0x004D39B0 (GUI
// teardown), 0x000AC03C and 0x005DB4BC (engine and system-utility teardown)
// and the NP manager callback registered at the end of AppInit.
static void GuiSystemSetup() {}
static void GuiSystemTeardown() {}
static void EngineTeardown() {}

// Flags cleared at start-up (0x010BA12C, four entries). NOTE: purpose unknown.
static bool g_startupFlags[4];

// @func 0x00463000 AppInit
bool AppInit(SlyApp& app)
{
    AppRuntime& rt = GetAppRuntime();
    rt.frameTime = 1.0f / 60.0f;
    GuiSystemSetup();

    for (int i = 0; i < kLocalPlayerCount; ++i) {
        rt.players[i].index = (uint8_t)i;
        rt.players[i].pad = ape::input::ApeInputMgr::CreatePad();
        rt.cursors[i] = ape::FindGuiObject(ape::TempFormat("Common.Cursors.%02d", i + 1), nullptr);
    }

    ape::LoadTuningFile("Scripts/Tuning.xml", !app.optV);
    app.field_04 = 0;
    for (bool& flag : g_startupFlags)
        flag = true;

    // TODO(0x0045CC18 SlyGame::SlyGame): the real constructor builds the
    // whole game world and is followed by its virtual Init (slot 2).
    g_game = new SlyGame;
    app.game = g_game;

    rt.frameStack = new ape::ApeMemStack(0xE000, 0, false);
    return true;
}

// TODO(0x0045B858 AppTick): runs one game frame and returns the next AppState.
int32_t AppTick(SlyApp& app)
{
    return kAppState_Quit;
}

// TODO(0x000D0298) updates a list of registered objects (calls 0x0028E320
// first, then virtual slot 9 on each entry of the list at 0x01013B70).
static void UpdateRegisteredObjects()
{
}

// TODO(0x002F481C) forwards a value queued during the frame (0x01068840,
// valid flag 0x01068841) to an object reached through the game world.
static void FlushQueuedWorldValue()
{
}

// @func 0x00458BAC AppEndFrame
// phase 0: end of the simulation frame, 1: end of the render frame,
// 2: both (single-threaded frame).
void AppEndFrame(SlyGame* game, int phase)
{
    if (phase == 0 || phase == 2) {
        EndFrameHook();
        SwapDebugBuffersIfRequested();
        FlipFrameCounters();
        UpdateRegisteredObjects();
        FlushQueuedWorldValue();
        if (FrameBufferPair* buffers = GetGameFrameBuffers())
            buffers->Swap();
        RunEndOfFrameCallbacks();
    }
    if (phase == 1 || phase == 2) {
        if (game)
            game->ResetFrameLists();
    }
}

// @func 0x00458940 AppShutdown
void AppShutdown(SlyApp& app)
{
    AppRuntime& rt = GetAppRuntime();
    SetAppShuttingDown(true);

    if (rt.frameStack) {
        rt.frameStack->Reset();
        delete rt.frameStack;
        rt.frameStack = nullptr;
    }

    // NOTE: virtual slot 8 of the pad device with argument 0; read as
    // "stop rumble on motor 0". The pads themselves are owned by the input
    // manager.
    for (AppPlayer& player : rt.players) {
        if (player.pad) {
            player.pad->StopRumble(0);
            player.pad = nullptr;
        }
    }

    if (app.game) {
        delete app.game;
        if (g_game == app.game)
            g_game = nullptr;
        app.game = nullptr;
    }

    GuiSystemTeardown();
    EngineTeardown();
}

} // namespace sly
