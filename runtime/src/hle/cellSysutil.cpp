// cellSysutil / cellSysutilAvconfExt / cellGame / cellSysmodule
#include <filesystem>
#include <mutex>

#include "../hle.h"
#include "../log.h"
#include "../vfs.h"
#include "loader/sfo.h"

using namespace opentit;
using namespace opentit::rtm;

namespace {

struct Callback {
    u32 func = 0;
    u32 userdata = 0;
};
std::mutex g_cb_mutex;
Callback g_callbacks[4];

const std::map<std::string, loader::SfoValue>& sfo() {
    static std::map<std::string, loader::SfoValue> data = [] {
        std::string err;
        const auto path = (std::filesystem::path(config().game_root) / "PS3_GAME" / "PARAM.SFO").string();
        auto r = loader::read_sfo(path, &err);
        if (!r) LOG_WARN("game", "PARAM.SFO not readable (%s)", err.c_str());
        return r ? *r : std::map<std::string, loader::SfoValue>{};
    }();
    return data;
}

std::string sfo_string(const char* key) {
    auto it = sfo().find(key);
    if (it == sfo().end()) return {};
    if (const auto* s = std::get_if<std::string>(&it->second)) return *s;
    return {};
}
u32 sfo_int(const char* key, u32 def) {
    auto it = sfo().find(key);
    if (it == sfo().end()) return def;
    if (const auto* v = std::get_if<u32>(&it->second)) return *v;
    return def;
}

void write_cstr(u32 dst, const std::string& s, u32 cap) {
    const u32 n = std::min<u32>(static_cast<u32>(s.size()), cap - 1);
    mem::write_bytes(dst, s.data(), n);
    mem::w8(dst + n, 0);
}

}  // namespace

// ---------------------------------------------------------------- callbacks
OPENTIT_HLE(cellSysutil, cellSysutilRegisterCallback) {
    const u32 slot = arg32(ctx, 0);
    if (slot > 3) {
        ret(ctx, 0x8002B101);  // CELL_SYSUTIL_ERROR_VALUE
        return;
    }
    std::lock_guard<std::mutex> lk(g_cb_mutex);
    g_callbacks[slot] = {arg32(ctx, 1), arg32(ctx, 2)};
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellSysutil, cellSysutilUnregisterCallback) {
    const u32 slot = arg32(ctx, 0);
    if (slot <= 3) {
        std::lock_guard<std::mutex> lk(g_cb_mutex);
        g_callbacks[slot] = {};
    }
    ret(ctx, CELL_OK);
}

// No system events (XMB, quit requests...) are generated yet.
OPENTIT_HLE_OK(cellSysutil, cellSysutilCheckCallback)

OPENTIT_HLE(cellSysutil, cellSysutilGetSystemParamInt) {
    const u32 id = arg32(ctx, 0);
    s32 v = 0;
    switch (id) {
    case 0x0111: v = 1; break;  // LANG: English (US)
    case 0x0112: v = 1; break;  // ENTER_BUTTON_ASSIGN: cross
    case 0x0114: v = 0; break;  // DATE_FORMAT
    case 0x0115: v = 1; break;  // TIME_FORMAT: 24h
    case 0x0116: v = 0; break;  // TIMEZONE
    case 0x0117: v = 0; break;  // SUMMERTIME
    case 0x0121: v = 0; break;  // GAME_PARENTAL_LEVEL
    case 0x0141: v = 0; break;  // CURRENT_USER_HAS_NP_ACCOUNT
    case 0x0152: v = 1; break;  // PAD_RUMBLE
    default: LOG_DEBUG("sysutil", "GetSystemParamInt(0x%04X) -> 0", id); break;
    }
    mem::w32(arg32(ctx, 1), static_cast<u32>(v));
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- video / audio out
OPENTIT_HLE(cellSysutil, cellVideoOutGetState) {
    // (videoOut, deviceIndex, CellVideoOutState*)
    const u32 out = arg32(ctx, 2);
    mem::w8(out + 0, 2);  // CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED
    mem::w8(out + 1, 1);  // color space RGB
    for (u32 k = 2; k < 8; ++k) mem::w8(out + k, 0);
    mem::w8(out + 8, 2);   // resolution 720p
    mem::w8(out + 9, 1);   // progressive
    mem::w8(out + 10, 0);  // conversion none
    mem::w8(out + 11, 2);  // aspect 16:9
    mem::w8(out + 12, 0);
    mem::w8(out + 13, 0);
    mem::w16(out + 14, 1);  // 59.94 Hz
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellSysutil, cellVideoOutGetResolution) {
    const u32 id = arg32(ctx, 0);
    u16 w = 1280, h = 720;
    switch (id) {
    case 1: w = 1920; h = 1080; break;
    case 2: w = 1280; h = 720; break;
    case 4: w = 720; h = 480; break;
    case 5: w = 720; h = 576; break;
    default: break;
    }
    mem::w16(arg32(ctx, 1), w);
    mem::w16(arg32(ctx, 1) + 2, h);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellSysutil, cellVideoOutGetResolutionAvailability) {
    const u32 id = arg32(ctx, 1);
    ret(ctx, (id == 1 || id == 2 || id == 4 || id == 5) ? 1 : 0);
}

OPENTIT_HLE_OK(cellSysutil, cellVideoOutConfigure)
OPENTIT_HLE_OK(cellSysutil, cellAudioOutConfigure)

OPENTIT_HLE(cellSysutil, cellAudioOutGetSoundAvailability) {
    ret(ctx, 2);  // stereo LPCM
}

OPENTIT_HLE(cellSysutilAvconfExt, cellVideoOutGetScreenSize) {
    // (videoOut, float* screenSize) -> not available on a PC display
    ret(ctx, 0x8002B225);  // CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET
}
OPENTIT_HLE_OK(cellSysutilAvconfExt, cellVideoOutSetGamma)

// ---------------------------------------------------------------- dialogs / cache
OPENTIT_HLE(cellSysutil, cellMsgDialogOpen2) {
    LOG_INFO("dialog", "message: \"%s\" (type 0x%X)", mem::read_string(arg32(ctx, 1)).c_str(), arg32(ctx, 0));
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(cellSysutil, cellMsgDialogOpenErrorCode) {
    LOG_WARN("dialog", "error dialog: 0x%08X", arg32(ctx, 0));
    ret(ctx, CELL_OK);
}
OPENTIT_HLE_OK(cellSysutil, cellMsgDialogClose)
OPENTIT_HLE_OK(cellSysutil, cellMsgDialogAbort)

OPENTIT_HLE(cellSysutil, cellSysCacheMount) {
    // CellSysCacheParam { char cacheId[32]; char getCachePath[1056]; u32 reserved }
    // As on hardware, /dev_hdd1 itself becomes the game's cache directory.
    const u32 p = arg32(ctx, 0);
    const std::string id = mem::read_string(p, 32);
    const std::string dir = sfo_string("TITLE_ID") + "_" + id;
    const bool existed = vfs::mount_hdd1(dir);
    write_cstr(p + 32, "/dev_hdd1", 1056);
    LOG_INFO("sysutil", "system cache '%s' mounted at /dev_hdd1 (%s)", dir.c_str(), existed ? "existing" : "new");
    ret(ctx, existed ? 1 : 0);  // CELL_SYSCACHE_RET_OK_RELAYED / _CLEARED
}

// ---------------------------------------------------------------- cellGame
OPENTIT_HLE(cellGame, cellGameBootCheck) {
    // (u32* type, u32* attributes, CellGameContentSize* size, char dirName[32])
    mem::w32(arg32(ctx, 0), 1);  // CELL_GAME_GAMETYPE_DISC
    mem::w32(arg32(ctx, 1), 0);
    if (const u32 sz = arg32(ctx, 2)) {
        mem::w32(sz, 40 * 1024 * 1024);  // hddFreeSizeKB
        mem::w32(sz + 4, 0);             // sizeKB (not computed)
        mem::w32(sz + 8, 0);             // sysSizeKB
    }
    if (const u32 dir = arg32(ctx, 3)) mem::w8(dir, 0);
    LOG_INFO("game", "boot check: disc game '%s' (%s)", sfo_string("TITLE").c_str(), sfo_string("TITLE_ID").c_str());
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGame, cellGamePatchCheck) { ret(ctx, 0x8002CB22); }  // CELL_GAME_ERROR_NOTPATCH

OPENTIT_HLE(cellGame, cellGameDataCheck) {
    // (type, dirName, CellGameContentSize*)
    const std::string dir = mem::read_string(arg32(ctx, 1), 32);
    if (const u32 sz = arg32(ctx, 2)) {
        mem::w32(sz, 40 * 1024 * 1024);
        mem::w32(sz + 4, 0);
        mem::w32(sz + 8, 0);
    }
    auto host = vfs::translate("/dev_hdd0/game/" + dir);
    std::error_code ec;
    const bool exists = host && std::filesystem::exists(*host, ec);
    ret(ctx, exists ? CELL_OK : 2);  // CELL_GAME_RET_NONE
}

OPENTIT_HLE(cellGame, cellGameContentPermit) {
    write_cstr(arg32(ctx, 0), "/dev_bdvd/PS3_GAME", 128);
    write_cstr(arg32(ctx, 1), "/dev_bdvd/PS3_GAME/USRDIR", 128);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGame, cellGameGetParamInt) {
    const u32 id = arg32(ctx, 0);
    u32 v = 0;
    switch (id) {
    case 102: v = sfo_int("PARENTAL_LEVEL", 0); break;  // CELL_GAME_PARAMID_PARENTAL_LEVEL
    case 103: v = sfo_int("RESOLUTION", 0); break;      // CELL_GAME_PARAMID_RESOLUTION
    case 104: v = sfo_int("SOUND_FORMAT", 0); break;    // CELL_GAME_PARAMID_SOUND_FORMAT
    default: break;
    }
    mem::w32(arg32(ctx, 1), v);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGame, cellGameGetParamString) {
    // (id, char* buf, u32 bufsize)
    const u32 id = arg32(ctx, 0);
    const char* key = nullptr;
    if (id <= 20) key = "TITLE";
    else if (id == 100) key = "TITLE_ID";
    else if (id == 101) key = "VERSION";
    else if (id == 105) key = "PS3_SYSTEM_VER";
    else if (id == 106) key = "APP_VER";
    write_cstr(arg32(ctx, 1), key ? sfo_string(key) : std::string(), std::max<u32>(arg32(ctx, 2), 1));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGame, cellGameContentErrorDialog) {
    LOG_WARN("game", "content error dialog type %u, needed %u KB", arg32(ctx, 0), arg32(ctx, 1));
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- cellSysmodule
// All system modules are HLE'd in-process, so loading always succeeds.
OPENTIT_HLE_OK(cellSysmodule, cellSysmoduleInitialize)
OPENTIT_HLE_OK(cellSysmodule, cellSysmoduleLoadModule)
OPENTIT_HLE_OK(cellSysmodule, cellSysmoduleUnloadModule)
OPENTIT_HLE(cellSysmodule, cellSysmoduleIsLoaded) { ret(ctx, CELL_OK); }  // CELL_SYSMODULE_LOADED
