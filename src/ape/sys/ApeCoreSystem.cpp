// openTIT - Ape engine core system start-up and shutdown.
#include "ape/sys/ApeCoreSystem.h"

#include "ape/file/ApeFile.h"
#include "ape/file/ZipArchive.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/input/ApeInputMgr.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/sys/ApePlatform.h"
#include "ape/sys/ApeWorker.h"
#include "ape/world/ApeTags.h"
#include "platform/Platform.h"

namespace ape {

namespace {

bool s_coreRunning = false;          // 0x01164880
plat::Mutex s_coreMutex;             // 0x01164CD8
uint32_t s_profileChunkSize = 0;     // 0x01164E48

// Root chunks of every engine data file.
constexpr uint32_t kFormChunk = iff::MakeId('F', 'O', 'R', 'M');
constexpr uint32_t kPlatformChunk = iff::MakeId('P', 'L', 'A', 'T');
constexpr uint32_t kProfileChunk = iff::MakeId('P', 'R', 'O', 'F');
constexpr uint32_t kProfileEndChunk = iff::MakeId('P', 'R', 'F', 'E');
constexpr uint32_t kProfileNameChunk = iff::MakeId('P', 'R', 'F', 'N');
constexpr uint32_t kRootChunkFlags = 0x18;

// "FORM": a container; its children are parsed in place.
// @func 0x007A50B4 FormChunkHandler
int FormChunkHandler(iff::IffReader* reader, void* arg)
{
    return reader->ParseChunks(arg) ? 1 : 0;
}

// "PLAT": three platform bytes; marks the file as written for this platform.
// @func 0x007A50D4 PlatformChunkHandler
int PlatformChunkHandler(iff::IffReader* reader, void*)
{
    uint8_t version = 0, platform = 0, variant = 0;
    reader->ReadU8(&version);
    reader->ReadU8(&platform);
    reader->ReadU8(&variant);
    reader->flags |= iff::kIffReader_Flag02;
    return 1;
}

// "PROF": a profiling section; remembers its size and parses it.
// @func 0x007A5208 ProfileChunkHandler
int ProfileChunkHandler(iff::IffReader* reader, void* arg)
{
    s_profileChunkSize = reader->GetChunkSize();
    return reader->ParseChunks(arg) ? 1 : 0;
}

// @func 0x007A527C ProfileEndChunkHandler
int ProfileEndChunkHandler(iff::IffReader*, void*)
{
    return 1;
}

// "PRFN": profile name, skipped. NOTE: the console reads the whole rest of
// the chunk into a 0x400-byte stack buffer at once; it is read in pieces here.
// @func 0x007A5284 ProfileNameChunkHandler
int ProfileNameChunkHandler(iff::IffReader* reader, void*)
{
    uint8_t buffer[0x400];
    uint32_t remaining = reader->GetChunkRemaining();
    while (remaining) {
        const uint32_t n = remaining < sizeof(buffer) ? remaining : uint32_t(sizeof(buffer));
        reader->Read(buffer, n);
        remaining -= n;
    }
    return 1;
}

} // namespace

// The console builds its FIOS stack here (media layer, RAM cache, scheduler
// and a mounted image of the game data). The PC file layer (plat::File via
// ape::file) needs no set-up.
// @func 0x00570E08 FileSchedulerInit
bool FileSchedulerInit()
{
    return true;
}

// @func 0x0055DC18 CoreSystemInit
bool CoreSystemInit()
{
    s_coreRunning = true;
    GetMemStack();   // creates the shared temporary stack
    (void)s_coreMutex;

    if (!CreateWorkerThreads() || !FileSchedulerInit() || !file::MountImageArchive())
        return false;

    input::PadSystemInit();
    input::SetControllerRecordMode(0);

    iff::RegisterChunkHandler(kFormChunk, FormChunkHandler, 0, nullptr, kRootChunkFlags);
    iff::RegisterChunkHandler(kPlatformChunk, PlatformChunkHandler, 0, nullptr, kRootChunkFlags);
    iff::RegisterChunkHandler(kProfileChunk, ProfileChunkHandler, 0, nullptr, kRootChunkFlags);
    iff::RegisterChunkHandler(kProfileEndChunk, ProfileEndChunkHandler, 0, nullptr, kRootChunkFlags);
    iff::RegisterChunkHandler(kProfileNameChunk, ProfileNameChunkHandler, 0, nullptr, kRootChunkFlags);
    iff::SetUnknownChunkHandler(nullptr);
    iff::g_iffReaderFlag = false;

    return world::InitTagSystem();
}

// @func 0x0055E7C0 CoreSystemShutdown
bool CoreSystemShutdown()
{
    if (!input::ApeInputMgr::Shutdown())
        return false;
    if (file::GetImageArchiveHandle() != 0)
        file::UnmountZipArchive(file::GetImageArchiveHandle());
    PlatformFileSystemShutdown();
    if (!AsyncFileSystemShutdown())
        return false;
    DestroyWorkerThreads();
    s_coreRunning = false;
    return true;
}

bool IsCoreSystemRunning()
{
    return s_coreRunning;
}

} // namespace ape
