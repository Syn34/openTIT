// openTIT - Game content boot check (cellGameBootCheck and friends).
//
// At start-up the engine asks the system which content it was booted from,
// logs the PARAM.SFO values it cares about and obtains the content paths
// (PS3_GAME and PS3_GAME/USRDIR). On PC the content is the extracted game
// directory: the USRDIR contents are the platform data root and PARAM.SFO is
// read from its parent directory (or from the data root itself) when present.
#pragma once

#include <cstdint>

namespace ape {

enum GameContentType : int {
    kGameType_Disc = 1,   // CELL_GAME_GAMETYPE_DISC
    kGameType_Hdd = 2,    // CELL_GAME_GAMETYPE_HDD
};

enum GameContentAttribute : uint32_t {
    kGameAttr_Patch               = 0x001,
    kGameAttr_AppHome             = 0x002,
    kGameAttr_Debug               = 0x004,
    kGameAttr_XmbBuy              = 0x008,
    kGameAttr_Commerce2Browser    = 0x010,
    kGameAttr_InviteMessage       = 0x020,
    kGameAttr_CustomDataMessage   = 0x040,
    kGameAttr_WebBrowser          = 0x100,
};

// PARAM.SFO value ids (CELL_GAME_PARAMID_*).
enum GameParamId : uint32_t {
    kGameParam_TitleCurrent  = 0,   // ids 0..17: TITLE in each language
    kGameParam_TitleDefault  = 1,
    kGameParam_TitleId       = 100,
    kGameParam_Version       = 101,
    kGameParam_ParentalLevel = 102,
    kGameParam_Resolution    = 103,
    kGameParam_SoundFormat   = 104,
};

constexpr int32_t kGameResult_Ok = 0;
// NOTE: CELL_GAME_ERROR_NOTFOUND; the PC build reports every lookup failure
// with it.
constexpr int32_t kGameError_NotFound = (int32_t)0x8002CB04;

struct GameContentSize {
    int32_t hddFreeSizeKB = 0;
    int32_t sizeKB = -1;      // -1: not calculated (CELL_GAME_SIZEKB_NOTCALC)
    int32_t sysSizeKB = 0;
};

struct GameContentInfo {
    int type = kGameType_Disc;
    uint32_t attributes = 0;
    GameContentSize size;
    char dirName[32] = {};
    char contentInfoPath[128] = {};   // directory with PARAM.SFO
    char usrdirPath[128] = {};        // the game data directory
    char patchContentInfoPath[128] = {};
    char patchUsrdirPath[128] = {};
};

// Runs the boot check, logs the content description and stores the content
// paths. Returns the result of the last step (0 or an error code).
int32_t GameBootCheck();

// Results of the last GameBootCheck().
const GameContentInfo& GetGameContentInfo();
// Free space reported by the boot check (0x00F88768 on the console).
extern int32_t g_hddFreeSizeKB;
int32_t GetHddFreeSizeKB();

// PARAM.SFO lookups (cellGameGetParamInt / cellGameGetParamString).
int32_t GameGetParamInt(uint32_t id, int32_t* value);
int32_t GameGetParamString(uint32_t id, char* buffer, uint32_t bufferSize);

} // namespace ape
