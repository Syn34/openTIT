// openTIT - Save games and settings.
#include "sly/save/SaveGame.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/xml/ApeStringTable.h"
#include "platform/Platform.h"
#include "sly/game/goals/JobManager.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace sly {

namespace {

namespace fs = std::filesystem;
using game::JobManager;
using game::Job;
using game::Episode;
using game::PlayerProgress;
using game::GetJobManager;

const char* const kSavePrefix    = "BCUS98247_SAVE_";
const char* const kSaveSlotName  = "BCUS98247_SAVE_SLOT_";
const char* const kSettingsDir   = "BCUS98247_SETTINGS";
const char* const kGameFile      = "FIELD";
const char* const kSettingsFile  = "CONFIG";
const char* const kParamFile     = "PARAM.TXT";

std::atomic<int32_t> g_state{ kSave_Idle };          // 0x00F9D1A8
std::atomic<int32_t> g_requiredSpace{ -2 };          // 0x00F645B0
std::atomic<bool> g_spaceQueryRunning{ false };      // 0x00F645B4
std::atomic<bool> g_saveInProgress{ false };         // 0x00F64538
std::atomic<bool> g_savedThisSession{ false };       // 0x01012788
SaveFailReason g_failReason = kSaveFail_None;        // 0x00F9D1B8

uint8_t g_buffer[kSaveGameSize];                     // 0x010127A0
char g_currentFolder[0x40];                          // 0x00F9D705
// Save description shown by the system browser (title / subtitle / details).
char g_title[0x80];                                  // 0x00F9D1C5
char g_subtitle[0x80];                               // 0x00F9D245
char g_details[0x400];                               // 0x00F9D2C5

fs::path SaveRoot()
{
    return fs::path(plat::ToHostPath("savedata"));
}

bool WriteFile(const fs::path& path, const void* data, size_t size)
{
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    FILE* f = std::fopen(path.string().c_str(), "wb");
    if (!f)
        return false;
    const bool ok = std::fwrite(data, 1, size, f) == size;
    return std::fclose(f) == 0 && ok;
}

// Returns false (and sets the fail reason) if the file is missing or short.
bool ReadFile(const fs::path& path, void* out, size_t size)
{
    FILE* f = std::fopen(path.string().c_str(), "rb");
    if (!f) {
        g_failReason = kSaveFail_NoData;
        return false;
    }
    const bool ok = std::fread(out, 1, size, f) == size;
    std::fclose(f);
    if (!ok)
        g_failReason = kSaveFail_Corrupt;
    return ok;
}

bool WriteSave(const char* folder, const char* fileName, const void* data, size_t size)
{
    const fs::path dir = SaveRoot() / folder;
    const std::string param = std::string(g_title) + "\n" + g_subtitle + "\n" + g_details + "\n";
    return WriteFile(dir / fileName, data, size) && WriteFile(dir / kParamFile, param.data(), param.size());
}

// Starts `worker` on its own thread unless an operation is already running.
// Afterwards translates the shared state into this call's result.
template <class Start>
int32_t PollOperation(Start start)
{
    int32_t state = g_state.load();
    if (state == kSave_Idle) {
        g_state = kSave_Busy;
        start();
        return g_state.load();
    }
    if (state == kSave_Success) {
        g_state = kSave_Idle;
        return kSave_Success;
    }
    if (state == kSave_Failed) {
        g_state = kSave_Idle;
        return kSave_Failed;
    }
    return state;
}

void RunWorker(void (*worker)())
{
    std::thread(worker).detach();
}

// Whole percent of a completion figure (the console floors the float).
int32_t WholePercent(float value)
{
    return static_cast<int32_t>(std::floor(value));
}

// Fills the title, subtitle and details the system save browser shows for
// `progress`: the game name, the episode (and job) last played, and the
// completion figures. Without a string table every line gets a placeholder.
// @func 0x000AE78C BuildSaveDescription
void BuildSaveDescription(const PlayerProgress* progress)
{
    const ape::StringTable* strings = ape::GetGuiStringTable();
    if (!strings || !progress) {
        ape::StrCopyBounded(g_title, "[Missing Title String]", sizeof(g_title));
        ape::StrCopyBounded(g_subtitle, "[Missing Subtitle String]", sizeof(g_subtitle));
        ape::StrCopyBounded(g_details, "[Missing Details String]", sizeof(g_details));
        // NOTE: the console also fills a fourth "save text" line here; the PC
        // save folder has no use for it.
        return;
    }

    ape::StrCopyBounded(g_title, strings->Find("SYSTEM_GAMENAME", "[Missing Title String]", false),
                        sizeof(g_title));

    JobManager* jobs = GetJobManager();
    const Job* episode = nullptr;
    if (progress->lastJobId)
        episode = jobs->FindJobById(progress->lastJobId);
    if (!episode)
        episode = jobs->LastStartedJob();
    if (!episode)
        episode = jobs->jobs;

    if (!episode) {
        ape::StrCopyBounded(g_subtitle, strings->Find("UnknownTitleName", "[Missing Location String]", false),
                            sizeof(g_subtitle));
    } else {
        // Record names carry a one-character prefix in front of the string id.
        const char* episodeName = strings->Find(episode->name + 1, episode->name, episode->field_33B != 0);
        const Episode* job = progress->lastEpisodeName ? jobs->FindEpisode(progress->lastEpisodeName) : nullptr;
        if (job) {
            const char* jobName = strings->Find(job->name + 1, job->name, job->field_1F8 != 0);
            ape::StrPrintf(g_subtitle, sizeof(g_subtitle), "%s - %s", episodeName, jobName);
        } else {
            ape::StrCopyBounded(g_subtitle, episodeName, sizeof(g_subtitle));
        }
    }

    const char* format = strings->Find("STR_SAVE_DETAILS", "[Missing Details String]", false);
    const int32_t completion = WholePercent(jobs->GameCompletion(progress));
    const int32_t bottles = WholePercent(jobs->BottlePercent(progress));
    const int32_t treasures = WholePercent(jobs->TreasurePercent(progress));
    const int32_t safes = WholePercent(jobs->SafePercent(progress));
    // NOTE: the console also evaluates the mask percentage but does not pass
    // it to the format.
    (void)jobs->MaskPercent(progress);
    ape::StrPrintf(g_details, sizeof(g_details), format, completion, progress->coins, bottles, treasures,
                   safes);
}

// Newest folder whose name starts with `prefix`, or "" if none.
std::string NewestSaveFolder(const char* prefix)
{
    std::error_code ec;
    std::string best;
    fs::file_time_type bestTime{};
    for (const fs::directory_entry& e : fs::directory_iterator(SaveRoot(), ec)) {
        const std::string name = e.path().filename().string();
        if (!e.is_directory(ec) || name.compare(0, std::strlen(prefix), prefix) != 0)
            continue;
        const fs::file_time_type t = e.last_write_time(ec);
        if (best.empty() || t > bestTime) {
            best = name;
            bestTime = t;
        }
    }
    return best;
}

// First unused "BCUS98247_SAVE_SLOT_<n>" folder name.
std::string NewSlotFolder()
{
    std::error_code ec;
    for (int i = 0;; ++i) {
        std::string name = kSaveSlotName + std::to_string(i);
        if (!fs::exists(SaveRoot() / name, ec))
            return name;
    }
}

// -- workers -------------------------------------------------------------------

// @func 0x000B01F4 ListLoadWorker
// The console lets the player pick a save in the system browser; the PC
// version loads the most recently written save.
void ListLoadWorker()
{
    std::memset(g_buffer, 0, sizeof(g_buffer));
    const std::string folder = NewestSaveFolder(kSavePrefix);
    if (!folder.empty() && ReadFile(SaveRoot() / folder / kGameFile, g_buffer, kSaveGameSize)) {
        ape::StrCopyBounded(g_currentFolder, folder.c_str(), sizeof(g_currentFolder));
        g_state = kSave_Success;
        return;
    }
    if (folder.empty())
        g_failReason = kSaveFail_NoData;
    g_state = kSave_Failed;
}

// @func 0x000B0828 ListSaveWorker
// Saves into the current folder, or a new slot if nothing was loaded/saved yet.
void ListSaveWorker()
{
    const std::string folder = g_currentFolder[0] ? std::string(g_currentFolder) : NewSlotFolder();
    if (WriteSave(folder.c_str(), kGameFile, g_buffer, kSaveGameSize)) {
        ape::StrCopyBounded(g_currentFolder, folder.c_str(), sizeof(g_currentFolder));
        g_savedThisSession = true;
        g_state = kSave_Success;
    } else {
        g_state = kSave_Failed;
    }
}

// @func 0x000B0A04 AutoSaveWorker
void AutoSaveWorker()
{
    const bool ok = g_currentFolder[0] && WriteSave(g_currentFolder, kGameFile, g_buffer, kSaveGameSize);
    g_state = ok ? kSave_Success : kSave_Failed;
}

// @func 0x000B0B84 DeleteWorker
// The console opens the system delete dialog; on PC the current save folder is removed.
void DeleteWorker()
{
    if (g_currentFolder[0]) {
        std::error_code ec;
        fs::remove_all(SaveRoot() / g_currentFolder, ec);
        g_currentFolder[0] = '\0';
    }
    g_state = kSave_Success;
}

// @func 0x000B0BCC ContinueWorker
void ContinueWorker()
{
    std::memset(g_buffer, 0, sizeof(g_buffer));
    const bool ok = g_currentFolder[0] &&
                    ReadFile(SaveRoot() / g_currentFolder / kGameFile, g_buffer, kSaveGameSize);
    g_state = ok ? kSave_Success : kSave_Failed;
}

// @func 0x000B03E0 SettingsLoadWorker
void SettingsLoadWorker()
{
    std::memset(g_buffer, 0, sizeof(g_buffer));
    // Give a running free-space query a moment to finish first (the console
    // waits for it with a timeout because both use the save utility).
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (g_spaceQueryRunning && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const bool ok = ReadFile(SaveRoot() / kSettingsDir / kSettingsFile, g_buffer, kSettingsSize);
    g_state = ok ? kSave_Success : kSave_Failed;
}

// @func 0x000B06A0 SettingsSaveWorker
void SettingsSaveWorker()
{
    const bool ok = WriteSave(kSettingsDir, kSettingsFile, g_buffer, kSettingsSize);
    g_state = ok ? kSave_Success : kSave_Failed;
}

// @func 0x000B1F74 RequiredSpaceWorker
// The console probes the save utility for the space a first save needs.
// Disk space is not a constraint on PC, so nothing extra is required.
void RequiredSpaceWorker()
{
    g_spaceQueryRunning = true;
    g_requiredSpace = 0;
    g_spaceQueryRunning = false;
}

} // namespace

// @func 0x000AED78 SaveGameToList
int32_t SaveGameToList(const void* data, size_t size)
{
    const int32_t state = g_state.load();
    if (state == kSave_Idle && !data)
        return kSave_Failed;
    if (state == kSave_Success || state == kSave_Failed)
        g_saveInProgress = false;
    return PollOperation([&] {
        g_saveInProgress = true;
        std::memcpy(g_buffer, data, size < kSaveGameSize ? size : kSaveGameSize);
        BuildSaveDescription(static_cast<const PlayerProgress*>(data));
        RunWorker(ListSaveWorker);
    });
}

// @func 0x000AEEC0 LoadGameFromList
int32_t LoadGameFromList(void* out)
{
    const int32_t state = g_state.load();
    if (state == kSave_Success)
        std::memcpy(out, g_buffer, kSaveGameSize);
    return PollOperation([] {
        g_failReason = kSaveFail_None;
        RunWorker(ListLoadWorker);
    });
}

// @func 0x000AEF98 AutoSaveGame
int32_t AutoSaveGame(const void* data, size_t size)
{
    const int32_t state = g_state.load();
    if (state == kSave_Idle && !data)
        return kSave_Failed;
    if (state == kSave_Success || state == kSave_Failed)
        g_saveInProgress = false;
    return PollOperation([&] {
        g_saveInProgress = true;
        std::memcpy(g_buffer, data, size < kSaveGameSize ? size : kSaveGameSize);
        BuildSaveDescription(static_cast<const PlayerProgress*>(data));
        RunWorker(AutoSaveWorker);
    });
}

// @func 0x000AF0E4 DeleteGame
int32_t DeleteGame()
{
    return PollOperation([] {
        g_savedThisSession = false;
        RunWorker(DeleteWorker);
    });
}

// @func 0x000AF1A8 ContinueGame
int32_t ContinueGame(void* out)
{
    if (g_state.load() == kSave_Success)
        std::memcpy(out, g_buffer, kSaveGameSize);
    return PollOperation([] { RunWorker(ContinueWorker); });
}

// @func 0x000AF27C RequiredSpace
int32_t RequiredSpace()
{
    if (g_requiredSpace == -2) {
        g_requiredSpace = -1;
        RunWorker(RequiredSpaceWorker);
    }
    return g_requiredSpace;
}

// @func 0x000AF2E8 SaveSettings
int32_t SaveSettings(const void* data)
{
    const int32_t state = g_state.load();
    if (state == kSave_Success || state == kSave_Failed)
        g_saveInProgress = false;
    return PollOperation([&] {
        g_failReason = kSaveFail_None;
        g_saveInProgress = true;
        std::memcpy(g_buffer, data, kSettingsSize);
        // TODO: localise through the string table (SYSTEM_SETTINGS / SYSTEM_SETTINGSINFO).
        ape::StrCopyBounded(g_title, "Sly Cooper Thieves in Time", sizeof(g_title));
        ape::StrCopyBounded(g_subtitle, "Game Settings", sizeof(g_subtitle));
        ape::StrCopyBounded(g_details, "Player configuration and preferences.", sizeof(g_details));
        RunWorker(SettingsSaveWorker);
    });
}

// @func 0x000AF530 LoadSettings
int32_t LoadSettings(void* out)
{
    if (g_state.load() == kSave_Success)
        std::memcpy(out, g_buffer, kSettingsSize);
    return PollOperation([] { RunWorker(SettingsLoadWorker); });
}

SaveFailReason GetLastSaveFailReason() { return g_failReason; }
bool IsSaveInProgress() { return g_saveInProgress; }
bool HasSavedThisSession() { return g_savedThisSession; }
const char* GetCurrentSaveFolder() { return g_currentFolder; }

} // namespace sly
