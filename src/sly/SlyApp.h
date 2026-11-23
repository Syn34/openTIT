// openTIT - Top level application object (the global at 0x010B19E0 on PS3).
#pragma once

#include <cstdint>

namespace sly {

class SlyGame; // the 0x1286B0-byte game world object, not yet reconstructed

// Values returned by the per-frame tick and stored in the global run state.
enum AppState : int32_t {
    kAppState_Starting  = 0,  // before the first frame
    kAppState_Running   = 1,
    kAppState_Suspended = 2,  // system menu open or a system-level pause request
    kAppState_Quit      = 4,
};

// Bits of the global system event mask written by the sysutil handler and by
// a few game systems. Only the sysutil ones are positively identified.
enum SystemEventFlags : uint32_t {
    kSysFlag_ResumeRequest = 0x002,  // NOTE: meaning inferred from the state it produces
    kSysFlag_SystemMenu    = 0x004,  // XMB in-game menu is open
    kSysFlag_ExitGame      = 0x010,  // the system asked the game to exit
    kSysFlag_Suspend80     = 0x080,  // set by game code; exact source unknown
    kSysFlag_Suspend100    = 0x100,  // set by game code; exact source unknown
};

// Startup options. All of them come from the command line (see ParseCommandLine).
struct SlyApp {
    SlyGame* game = nullptr;          // +0x00
    uint32_t field_04 = 0;            // +0x04 cleared by Init
    bool     optO = false;            // +0x08 -o
    bool     field_09 = false;        // +0x09
    bool     optV = false;            // +0x0A -v  (disables a debug feature in Init)
    bool     field_0B = false;        // +0x0B gates a profiler-style object in main
    uint32_t levelNameHash = 0;       // +0x0C -l <level>
    uint32_t episodeNameHash = 0;     // +0x10 -e <episode>
    uint32_t jobNameHash = 0;         // +0x14 -j <job>
    uint32_t tagListHash = 0;         // +0x18 -g <a;b;c>
    bool     optH = false;            // +0x1C -h <0|1>
    bool     optB = false;            // +0x1D -b <0|1>
    char     optQ[0x80] = {};         // +0x1E -q <string>
};

extern SlyApp g_app;
extern int32_t g_appState;
extern uint32_t g_systemEventFlags;

// Removes recognised switches from argv (in place) and records them in g_app.
void ParseCommandLine(int& argc, char** argv);

// Game lifecycle (bodies live in SlyApp.cpp).
bool AppInit(SlyApp& app);
int32_t AppTick(SlyApp& app);
void AppEndFrame(SlyGame* game, int phase);
void AppShutdown(SlyApp& app);

} // namespace sly
