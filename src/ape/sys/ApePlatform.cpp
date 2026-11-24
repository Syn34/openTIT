// openTIT - Ape engine platform bring-up.
#include "ape/sys/ApePlatform.h"

#include "ape/memory/ApeMem.h"
#include "platform/Platform.h"

namespace ape {

int g_gameLanguage = kGameLang_English;

namespace {

int s_systemLanguage = kSysLang_EnglishUS;
uint64_t s_moduleLoadTimeUsec = 0;   // 0x010F0F88

// CELL_SYSMODULE_* ids loaded at start-up, as passed by the game (0x0E is the
// file system, 0x10 GCM; the others are not identified here).
const uint16_t kStartupModules[] = { 0x17, 0x1C, 0x10, 0x0E, 0x31, 0x09, 0x3E };
// Unloaded at shutdown (0x09 stays loaded).
const uint16_t kShutdownModules[] = { 0x17, 0x1C, 0x10, 0x0E, 0x31, 0x3E };

} // namespace

void SetSystemLanguage(int sysLanguage)
{
    s_systemLanguage = sysLanguage;
}

int GetSystemLanguage()
{
    return s_systemLanguage;
}

// @func 0x0056BE30 GetGameLanguage
int GetGameLanguage()
{
    switch (GetSystemLanguage()) {
    case kSysLang_Japanese:     return kGameLang_Japanese;
    case kSysLang_EnglishUS:    return kGameLang_English;
    case kSysLang_French:       return kGameLang_French;
    case kSysLang_Spanish:      return kGameLang_Spanish;
    case kSysLang_German:       return kGameLang_German;
    case kSysLang_Italian:      return kGameLang_Italian;
    case kSysLang_Dutch:        return kGameLang_Dutch;
    case kSysLang_PortuguesePT: return kGameLang_Portuguese;
    case kSysLang_Russian:      return kGameLang_Russian;
    case kSysLang_Korean:       return kGameLang_Korean;
    case kSysLang_Finnish:      return kGameLang_Finnish;
    case kSysLang_Swedish:      return kGameLang_Swedish;
    case kSysLang_Danish:       return kGameLang_Danish;
    case kSysLang_Norwegian:    return kGameLang_Norwegian;
    case kSysLang_Polish:       return kGameLang_Polish;
    case kSysLang_PortugueseBR: return kGameLang_PortugueseBR;
    case kSysLang_EnglishGB:    return kGameLang_EnglishUK;
    default:                    return kGameLang_English;
    }
}

// NOTE: the console version queries the system language for every index
// outside the set of known game languages and throws the answer away; it has
// no other effect.
// @func 0x0056BF84 OnGameLanguageChanged
void OnGameLanguageChanged(int /*gameLanguage*/)
{
}

// @func 0x0056C020 LoadSysModule
bool LoadSysModule(uint16_t moduleId)
{
    const uint64_t start = plat::GetSystemTimeUsec();
    // There are no system modules on PC: every load succeeds.
    (void)moduleId;
    s_moduleLoadTimeUsec += plat::GetSystemTimeUsec() - start;
    return true;
}

uint64_t GetModuleLoadTimeUsec()
{
    return s_moduleLoadTimeUsec;
}

// NOTE: the console version also prints the executable's segment layout and
// user memory sizes, and brings up SPURS and the two SPU job chains
// (0x0056D630, not reconstructed: the PC build has no SPUs).
// @func 0x0056D87C PlatformSystemInit
bool PlatformSystemInit()
{
    plat::Log(plat::LogLevel::Info, "\n\n\n\n");
    plat::Log(plat::LogLevel::Info,
              "===============================================================================\n");
    for (uint16_t id : kStartupModules)
        LoadSysModule(id);
    g_gameLanguage = GetGameLanguage();
    return true;
}

// NOTE: on the console this also shuts the SPU job chains and SPURS down and
// terminates NP if it was started.
// @func 0x0056DCEC PlatformSystemShutdown
bool PlatformSystemShutdown()
{
    plat::Log(plat::LogLevel::Info, "## libgplatformGlobals.pSpurs : ApeSpurs SUCCEEDED ##\n");
    for (uint16_t id : kShutdownModules)
        (void)id;   // nothing to unload on PC
    return true;
}

// @func 0x0056DF78 PlatformSystemUpdate
bool PlatformSystemUpdate()
{
    return true;
}

// NOTE: name guessed from the caller (0x005DBB30, which runs this and then
// the audio update).
// @func 0x0055E284 CoreSystemUpdate
bool CoreSystemUpdate()
{
    PlatformSystemUpdate();
    return true;
}

// @func 0x00570B90 PlatformFileSystemShutdown
void PlatformFileSystemShutdown()
{
}

// @func 0x00571164 AsyncFileSystemShutdown
bool AsyncFileSystemShutdown()
{
    return true;
}

// The console builds a dlmalloc mspace in [base, base + size - 16) and
// resets the pool statistics to the region size.
// @func 0x0056C428 InitHeapA
void InitHeapA(void* /*base*/, size_t size)
{
    SetPoolBudget(kMemPool_HeapA, (int64_t)size);
}

// Same for the second heap, after mapping the region into the RSX address
// space (cellGcmMapMainMemory).
// @func 0x0056C49C InitHeapB
void InitHeapB(void* /*base*/, size_t size)
{
    SetPoolBudget(kMemPool_HeapB, (int64_t)size);
}

// @func 0x0056C8A4 FreeMappedMemory
void FreeMappedMemory(void* ptr)
{
    // cellGcmUnmapEaIoAddress() first on the console.
    SysFree(ptr, 6, 0);
}

} // namespace ape
