// openTIT - Ape engine platform bring-up: system modules, language, memory
// regions and the per-frame platform hooks.
//
// On the console these talk to cellSysmodule, cellSysutil and cellGcm. On PC
// there are no system modules to load and no RSX mapping, so most of them
// reduce to bookkeeping; the engine visible results (success codes, language
// index, pool budgets, log output) are kept.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

// System language codes (CELL_SYSUTIL_LANG_*).
enum SystemLanguage : int {
    kSysLang_Japanese = 0,
    kSysLang_EnglishUS,
    kSysLang_French,
    kSysLang_Spanish,
    kSysLang_German,
    kSysLang_Italian,
    kSysLang_Dutch,
    kSysLang_PortuguesePT,
    kSysLang_Russian,
    kSysLang_Korean,
    kSysLang_ChineseT,
    kSysLang_ChineseS,
    kSysLang_Finnish,
    kSysLang_Swedish,
    kSysLang_Danish,
    kSysLang_Norwegian,
    kSysLang_Polish,
    kSysLang_PortugueseBR,
    kSysLang_EnglishGB,
    kSysLang_Turkish,
};

// The game's own language indices.
enum GameLanguage : int {
    kGameLang_English = 0,
    kGameLang_French = 1,
    kGameLang_Italian = 2,
    kGameLang_German = 3,
    kGameLang_Spanish = 4,
    kGameLang_Dutch = 5,
    kGameLang_Portuguese = 6,
    kGameLang_Finnish = 7,
    kGameLang_Swedish = 8,
    kGameLang_Danish = 9,
    kGameLang_Norwegian = 10,
    kGameLang_Polish = 11,
    kGameLang_Russian = 12,
    kGameLang_EnglishUK = 16,
    kGameLang_PortugueseBR = 17,
    kGameLang_Japanese = 18,
    kGameLang_Korean = 19,
};

// The system language reported to the game. The console asks the system
// settings; on PC it defaults to US English and can be overridden here (for
// example from a command line switch).
void SetSystemLanguage(int sysLanguage);
int GetSystemLanguage();

// Game language for the current system language. Languages the game has no
// text for (Chinese, Turkish) fall back to English.
int GetGameLanguage();
// Hook called when the game switches language. Does nothing in the shipping
// build.
void OnGameLanguageChanged(int gameLanguage);
// Game language picked by PlatformSystemInit (0x010C5A48 on the console).
extern int g_gameLanguage;

// Loads a system library module, accumulating the time spent in
// GetModuleLoadTimeUsec(). Returns true on success.
bool LoadSysModule(uint16_t moduleId);
uint64_t GetModuleLoadTimeUsec();

// Platform layer bring-up / teardown / per-frame hook (called by the engine
// system init, shutdown and update).
bool PlatformSystemInit();
bool PlatformSystemShutdown();
bool PlatformSystemUpdate();
// Per-frame update of the core engine group.
bool CoreSystemUpdate();
// Per-frame update of the engine services: the core/platform layer, then
// the graphics layer. Always true.
bool EngineUpdate();

// File system teardown hooks run by the core shutdown, after the input
// devices are gone: the platform file layer (counterpart of the file
// module's PlatformFileSystemInit) and the asynchronous I/O stack. Both are
// empty / successful in the shipping build.
void PlatformFileSystemShutdown();
bool AsyncFileSystemShutdown();

// Memory regions handed to the engine heaps for memory classes 4 and 5.
// On PC the pools are host backed; only their budgets are set.
void InitHeapA(void* base, size_t size);
void InitHeapB(void* base, size_t size);
// Releases a block that was mapped for the GPU (memory class 6).
void FreeMappedMemory(void* ptr);

} // namespace ape
