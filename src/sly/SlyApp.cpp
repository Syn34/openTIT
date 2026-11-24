// openTIT - Application lifecycle.
#include "sly/SlyApp.h"
#include "sly/frame/FrameServices.h"

namespace sly {

SlyApp g_app;
int32_t g_appState = kAppState_Starting;
uint32_t g_systemEventFlags = 0;

// TODO(0x00463000 AppInit): creates the SlyGame world (0x1286B0 bytes), the
// two player cursors ("Common.Cursors.%02d"), loads Scripts/Tuning.xml and
// registers the NP manager callback.
bool AppInit(SlyApp& app)
{
    app.field_04 = 0;
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
        // TODO: the console clears the counts of seven per-frame lists inside
        // the game world (+0x1284EC, stride 0x14). SlyGame is not
        // reconstructed yet.
        (void)game;
    }
}

// TODO(0x00458940 AppShutdown)
void AppShutdown(SlyApp& app)
{
}

} // namespace sly
