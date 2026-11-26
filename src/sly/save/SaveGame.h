// openTIT - Save games and settings.
//
// The console stores progress through the system save-data utility on a
// worker thread; the game polls each operation every frame until it
// reports success or failure. The PC version keeps that polling contract
// but stores each save as a folder of plain files under <data>/savedata/.
//
// Every operation returns:
//   kSave_Busy    (1) while running (the first call starts it),
//   kSave_Success (2) once, then the system returns to idle,
//   kSave_Failed  (3) once, then the system returns to idle.
// A call while another operation is running just reports kSave_Busy.
#pragma once

#include <cstddef>
#include <cstdint>

namespace sly {

enum SaveResult : int32_t {
    kSave_Idle    = 0,
    kSave_Busy    = 1,
    kSave_Success = 2,
    kSave_Failed  = 3,
};

// Why the last load failed (0 = no specific reason).
enum SaveFailReason : int32_t {
    kSaveFail_None    = 0,
    kSaveFail_NoData  = 3,
    kSaveFail_Corrupt = 5,
};

constexpr size_t kSaveGameSize = 0xFD8;   // serialised progress block
constexpr size_t kSettingsSize = 0x24;    // serialised options block

int32_t SaveGameToList(const void* data, size_t size);
int32_t LoadGameFromList(void* out);
int32_t AutoSaveGame(const void* data, size_t size);
int32_t DeleteGame();
int32_t ContinueGame(void* out);
// Free space the first save needs, in KiB: -2 = not queried yet, -1 = querying.
int32_t RequiredSpace();
int32_t SaveSettings(const void* data);
int32_t LoadSettings(void* out);

SaveFailReason GetLastSaveFailReason();
bool IsSaveInProgress();           // drives the "saving" icon
// The operation state (SaveResult; console global 0x00F9D1A8).
int32_t SaveSystemState();
bool HasSavedThisSession();
const char* GetCurrentSaveFolder();

} // namespace sly
