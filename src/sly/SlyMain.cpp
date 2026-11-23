// openTIT - Game entry point: engine configuration, boot sequence and main loop.
#include "sly/SlyApp.h"
#include "sly/LoadingScreen.h"

#include "agl/AglRenderThread.h"
#include "ape/file/ApeFile.h"
#include "ape/file/AsyncFile.h"
#include "ape/core/ApeNetwork.h"
#include "ape/core/ApeString.h"
#include "ape/core/ApeSysUtil.h"
#include "ape/core/ApeSystem.h"
#include "platform/Platform.h"

#include <cstdint>
#include <cstring>

namespace sly {

namespace {

// Objects whose release is postponed until the GPU no longer uses them.
void* g_pendingRelease = nullptr;      // 0x010B7818
uint8_t g_pendingReleaseDelay = 0;     // 0x010B781C frames to wait
// NOTE: bit 7 of 0x010B7820. When set the frame is ticked without
// overlapping it with the render thread; the writer is not reconstructed.
bool g_tickWithoutRenderOverlap = false;
uint64_t g_bootTimeBase = 0;           // 0x01164890

// Triple-buffered per-frame scratch owned by the game world (+0x1285E4).
struct FrameRing {
    void* buffers[3];
    void* current;
    void* previous;
    uint8_t pad;
    uint8_t index;
};

// TODO: SlyGame is not reconstructed yet, so there is no ring to rotate.
FrameRing* GetFrameRing(SlyGame* game)
{
    return nullptr;
}

void AdvanceFrameRing(SlyGame* game)
{
    FrameRing* ring = GetFrameRing(game);
    if (!ring)
        return;
    ring->previous = ring->current;
    ring->index = (uint8_t)((ring->index + 1) % 3);
    ring->current = ring->buffers[ring->index];
}

// TODO(0x00596A8C) per-frame update of an engine subsystem (phase argument).
void EngineSubsystemEndFrame(int phase)
{
}

// TODO(0x0051096C) wraps 0x0053C4A0 with a "running" flag at 0x010C5A71.
void PostFrameUpdate()
{
}

// TODO(0x0049AE40 RenderFrame): render-thread entry, draws the game world.
void RenderFrame(void* userdata)
{
}

// TODO(0x004FD214) constructor of the object at 0x011DE300; only built when
// SlyApp::field_0B is set.
void ConstructOptionalProfiler()
{
}

// Fills g_engineConfig with the values this game ships with. Inline at the
// top of main() in the original.
void ConfigureEngine(ape::EngineConfig& c)
{
    c.gameName  = "Sly4";
    c.titleId   = "BCES01284";
    c.field_024 = false;
    c.field_025 = false;
    c.field_038 = 0.25f;
    c.field_05C = 1;
    c.field_088 = 300.0f;
    c.field_08C = 50.0f;
    c.field_099 = 4;
    c.field_09A = 4;
    c.field_09B = 0x10;
    c.field_09C = 1;
    c.field_09D = 1;
    c.field_0A1 = false;
    c.field_0A2 = false;
    c.field_0A3 = true;
    c.field_0A4 = 0x300000;
    c.field_0A8 = 20;
    c.field_0AC = 0.5f;
    c.field_144 = 0x1F4000;
    c.field_148 = 0;
    c.field_14C = 0x1000;
    c.field_150 = 0x2000;
    c.field_154 = 0x4C00;
    c.field_158 = 0x6000;
    c.field_16E = 0x500;
    c.field_178 = 0x400;
    c.field_17C = 0x500;
    c.field_18E = 0x1000;
    c.field_1A4 = 0x20000;
    c.field_1A8 = 0x60000;
    // Object pool capacities at +0x1AC..+0x1C8 (index = (offset - 0x1AC) / 2).
    static const uint16_t kCapacities[15] = {
        4000, 1000, 1000, 0 /* +0x1B2 untouched */, 1000, 0x10A0, 0 /* +0x1B8 untouched */,
        400, 400, 1000, 1000, 1000, 1000, 50, 50 };
    std::memcpy(c.capacities_1AC, kCapacities, sizeof(kCapacities));
    c.field_1CC = 0x700000;
    c.field_1D0 = 0x1900000;
    c.field_1D4 = 0xC8000;
    c.field_1D8 = 0x800000;
    c.field_1DC = 0xC0000;
    c.field_1E0 = 0x400000;
    c.field_1E4 = 0x12C000;
    c.field_1E8 = 0;
    c.field_1EC = 0x9800;
    c.field_1F0 = 0x400;
    c.field_1F4 = 40;
    c.field_1F8 = 400;
    c.field_1FC = 0x400000;
    c.field_200 = 0x400000;
    c.field_210 = 0xABF3AE6F1D488483ull;
    ape::g_engineFlag_10F0EA8 = false;
}

// Pumps system events and folds the resulting flags into g_appState.
// Returns false when the application must stop. Inline after every boot
// step in the original.
bool PollSystemState()
{
    ape::SysUtilCheckCallbacks();
    if (g_systemEventFlags & kSysFlag_ExitGame) {
        g_appState = kAppState_Quit;
        return false;
    }
    if (g_systemEventFlags & (kSysFlag_SystemMenu | kSysFlag_Suspend80 | kSysFlag_Suspend100))
        g_appState = kAppState_Suspended;
    else if (g_systemEventFlags & kSysFlag_ResumeRequest)
        g_appState = kAppState_Running;
    else if (g_appState == kAppState_Quit)
        return false;
    return true;
}

void ReleasePendingObject()
{
    if (g_pendingReleaseDelay < 2) {
        if (g_pendingRelease) {
            agl::ScheduleDeferredRelease(g_pendingRelease);
            g_pendingRelease = nullptr;
            g_pendingReleaseDelay = 0;
        }
    } else {
        --g_pendingReleaseDelay;
    }
}

[[noreturn]] void ShutdownAndExit()
{
    AppShutdown(g_app);
    ape::PostGameShutdownHook();
    plat::ExitProcess(0);
}

} // namespace

// @func 0x0049AED8 OnSystemEvent
void OnSystemEvent(uint32_t event, uint32_t param, void* userdata)
{
    switch (event) {
    case plat::kSystemEvent_RequestExitGame:
        g_systemEventFlags |= kSysFlag_ExitGame;
        break;
    case plat::kSystemEvent_SystemMenuOpen:
        g_systemEventFlags |= kSysFlag_SystemMenu;
        break;
    case plat::kSystemEvent_SystemMenuClose:
        g_systemEventFlags &= ~kSysFlag_SystemMenu;
        break;
    default:
        break;
    }
}

// @func 0x0046320C ParseCommandLine
void ParseCommandLine(int& argc, char** argv)
{
    g_app.optO = false;
    g_app.field_09 = false;
    g_app.optV = false;
    g_app.field_0B = false;

    for (int i = 0; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg[0] != '-' && arg[0] != '/')
            continue;

        const char option = arg[1];
        const bool hasNext = i + 1 < argc;
        const char* next = hasNext ? argv[i + 1] : nullptr;
        // Switches with a value accept it glued on ("-lLevel") or as the next argument.
        const char* glued = arg[2] ? arg + 2 : nullptr;
        int consumed = 1;

        auto takeValue = [&]() -> const char* {
            if (glued)
                return glued;
            if (!hasNext)
                return nullptr;
            consumed = 2;
            return next;
        };

        switch (option) {
        case '?':
            plat::ExitProcess(0);
        case 'b': case 'B':
            if (hasNext) { g_app.optB = next[0] == '1'; consumed = 2; }
            break;
        case 'c': case 'C':
            ape::g_engineConfig.field_0A1 = true;
            break;
        case 'e': case 'E':
            if (const char* v = takeValue()) g_app.episodeNameHash = ape::HashNameNoCase(v);
            break;
        case 'g': case 'G':
            if (const char* v = takeValue()) g_app.tagListHash = ape::HashNameListSum(v);
            break;
        case 'h': case 'H':
            if (hasNext) { g_app.optH = next[0] == '1'; consumed = 2; }
            break;
        case 'j': case 'J':
            if (const char* v = takeValue()) g_app.jobNameHash = ape::HashNameNoCase(v);
            break;
        case 'l': case 'L':
            if (const char* v = takeValue()) g_app.levelNameHash = ape::HashNameNoCase(v);
            break;
        case 'o': case 'O':
            g_app.optO = true;
            break;
        case 'q': case 'Q':
            if (const char* v = takeValue()) ape::StrCopyBounded(g_app.optQ, v, sizeof(g_app.optQ));
            break;
        case 't': case 'T':
            ape::g_engineFlag_10F0EA8 = true;
            break;
        case 'v': case 'V':
            g_app.optV = true;
            break;
        default:
            break;
        }

        // Remove the switch (and its value) from the argument list so the
        // remaining arguments are left for other consumers.
        for (int k = i; k + consumed < argc; ++k)
            argv[k] = argv[k + consumed];
        argc -= consumed;
        --i;
    }
}

// @func 0x00463658 SlyMain
int SlyMain(int argc, char** argv)
{
    ConfigureEngine(ape::g_engineConfig);
    ParseCommandLine(argc, argv);
    ape::SysUtilRegisterCallback(0, OnSystemEvent, nullptr);

    ape::SystemInit(ape::kInit_Core);
    if (!PollSystemState())
        return 0;

    ape::NpInit(0x20000);
    if (ape::file::FileExists("Sly4.psarc")) {
        ape::file::AsyncFileParams params;
        params.flags = 0;
        params.urgent = true;
        ape::file::MountDeviceArchive("Sly4.psarc", params);
    }
    if (!PollSystemState())
        return 0;

    // The shader archive is read into memory and exposed as an in-memory
    // file ("RamDisk") while the remaining subsystems, which load shaders,
    // start up.
    uint32_t shadersSize = 0;
    void* shaders = ape::file::LoadFile("System\\Shaders.Zip", &shadersSize);
    if (!PollSystemState())
        return 0;

    ape::file::File* ramDisk = ape::file::CreateMemoryFile(shaders, shadersSize, "RamDisk");
    ape::SystemInit(ape::kInit_AllButCore);
    if (!PollSystemState())
        return 0;

    if (ramDisk)
        ramDisk->Close();
    ape::file::FreeFileData(shaders);

    agl::RenderThreadCreate(RenderFrame, nullptr);
    if (g_app.field_0B)
        ConstructOptionalProfiler();

    ape::PreGameInitHook();
    LoadingScreenStart(true);
    if (!PollSystemState())
        return 0;

    AppInit(g_app);
    if (!PollSystemState())
        ShutdownAndExit();

    LoadingScreenStop();
    while ((g_bootTimeBase = plat::GetTimeBase()) == 0) {
    }
    g_appState = kAppState_Starting;
    PollSystemState();

    static bool s_firstFrameDone = false;
    while (g_appState == kAppState_Starting || g_appState == kAppState_Running ||
           g_appState == kAppState_Suspended) {
        if (g_loadingScreenRequest)
            LoadingScreenStart(false);
        g_loadingScreenRequest = false;

        bool overlapped = false;
        if (!s_firstFrameDone || g_tickWithoutRenderOverlap) {
            s_firstFrameDone = true;
            g_appState = AppTick(g_app);
        } else {
            // Normally the previous frame renders while this one is simulated.
            overlapped = true;
            if (!ape::g_engineConfig.field_022) {
                agl::RenderThreadKick();
                g_appState = AppTick(g_app);
                agl::RenderThreadWait();
            } else {
                agl::RenderThreadKick();
                agl::RenderThreadWait();
                g_appState = AppTick(g_app);
            }
        }

        ReleasePendingObject();
        AppEndFrame(g_app.game, 2);
        AdvanceFrameRing(g_app.game);
        EngineSubsystemEndFrame(2);
        agl::EndFrame(2);
        if (overlapped)
            PostFrameUpdate();

        if (g_appState != kAppState_Running && g_appState != kAppState_Suspended)
            break;
    }

    ShutdownAndExit();
}

} // namespace sly
