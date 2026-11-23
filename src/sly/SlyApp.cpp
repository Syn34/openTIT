// openTIT - Application lifecycle.
#include "sly/SlyApp.h"

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

// TODO(0x00458BAC AppEndFrame)
void AppEndFrame(SlyGame* game, int phase)
{
}

// TODO(0x00458940 AppShutdown)
void AppShutdown(SlyApp& app)
{
}

} // namespace sly
