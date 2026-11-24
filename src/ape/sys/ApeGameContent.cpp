// openTIT - Game content boot check (cellGameBootCheck and friends).
#include "ape/sys/ApeGameContent.h"

#include "ape/core/ApeSystem.h"
#include "ape/sys/ApePlatform.h"
#include "platform/Platform.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <system_error>
#include <vector>

namespace ape {

int32_t g_hddFreeSizeKB = 0;

namespace {

GameContentInfo s_info;

// --- PARAM.SFO ---------------------------------------------------------------

struct SfoValue {
    bool isInt = false;
    int32_t intValue = 0;
    std::string text;
};

struct Sfo {
    bool loaded = false;
    std::string directory;
    std::map<std::string, SfoValue> values;
};

uint32_t LE16(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
uint32_t LE32(const uint8_t* p) { return LE16(p) | (LE16(p + 2) << 16); }

bool ParseSfo(const std::vector<uint8_t>& data, Sfo& sfo)
{
    if (data.size() < 20 || LE32(&data[0]) != 0x46535000u)   // "\0PSF"
        return false;
    const uint32_t keyTable = LE32(&data[8]);
    const uint32_t dataTable = LE32(&data[12]);
    const uint32_t count = LE32(&data[16]);
    if (20ull + count * 16ull > data.size())
        return false;

    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* entry = &data[20 + i * 16];
        const uint32_t keyOffset = keyTable + LE16(entry);
        const uint32_t format = LE16(entry + 2);
        const uint32_t length = LE32(entry + 4);
        const uint32_t valueOffset = dataTable + LE32(entry + 12);
        if (keyOffset >= data.size() || (uint64_t)valueOffset + length > data.size())
            continue;
        const char* key = reinterpret_cast<const char*>(&data[keyOffset]);
        const std::string name(key, strnlen(key, data.size() - keyOffset));

        SfoValue value;
        if (format == 0x0404 && length >= 4) {
            value.isInt = true;
            value.intValue = (int32_t)LE32(&data[valueOffset]);
        } else {
            const char* text = reinterpret_cast<const char*>(&data[valueOffset]);
            value.text.assign(text, strnlen(text, length));
        }
        sfo.values[name] = value;
    }
    return true;
}

const Sfo& GetSfo()
{
    static Sfo s_sfo;
    if (s_sfo.loaded)
        return s_sfo;
    s_sfo.loaded = true;

    const std::filesystem::path root(plat::GetDataRoot());
    const std::filesystem::path candidates[] = { root.parent_path(), root };
    for (const std::filesystem::path& dir : candidates) {
        std::ifstream in(dir / "PARAM.SFO", std::ios::binary);
        if (!in)
            continue;
        const std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (ParseSfo(data, s_sfo)) {
            s_sfo.directory = dir.string();
            break;
        }
    }
    return s_sfo;
}

// PARAM.SFO key for a value id.
std::string ParamKey(uint32_t id)
{
    char key[16];
    switch (id) {
    case kGameParam_TitleCurrent: {
        // The title in the system language, if the content has one.
        std::snprintf(key, sizeof key, "TITLE_%02d", GetSystemLanguage());
        const Sfo& sfo = GetSfo();
        return sfo.values.count(key) ? std::string(key) : std::string("TITLE");
    }
    case kGameParam_TitleDefault:  return "TITLE";
    case kGameParam_TitleId:       return "TITLE_ID";
    case kGameParam_Version:       return "VERSION";
    case kGameParam_ParentalLevel: return "PARENTAL_LEVEL";
    case kGameParam_Resolution:    return "RESOLUTION";
    case kGameParam_SoundFormat:   return "SOUND_FORMAT";
    default:
        if (id >= 2 && id < 2 + 20) {
            std::snprintf(key, sizeof key, "TITLE_%02u", id - 2);
            return key;
        }
        return std::string();
    }
}

void CopyString(char* dst, size_t size, const std::string& src)
{
    std::snprintf(dst, size, "%s", src.c_str());
}

void Print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    plat::LogV(plat::LogLevel::Info, fmt, args);
    va_end(args);
}

// --- console call equivalents ----------------------------------------------------

int32_t BootCheck(GameContentInfo& info)
{
    info.type = kGameType_Disc;
    info.attributes = 0;

    std::error_code ec;
    const std::filesystem::space_info space = std::filesystem::space(plat::GetDataRoot(), ec);
    const uint64_t freeKB = ec ? 0 : space.available / 1024;
    info.size.hddFreeSizeKB = freeKB > (uint64_t)INT_MAX ? INT_MAX : (int32_t)freeKB;
    info.size.sizeKB = -1;
    info.size.sysSizeKB = 0;

    char titleId[32] = {};
    if (GameGetParamString(kGameParam_TitleId, titleId, sizeof titleId) != kGameResult_Ok)
        CopyString(titleId, sizeof titleId, g_engineConfig.titleId ? g_engineConfig.titleId : "");
    CopyString(info.dirName, sizeof info.dirName, titleId);
    return kGameResult_Ok;
}

int32_t ContentPermit(char* contentInfoPath, char* usrdirPath)
{
    const Sfo& sfo = GetSfo();
    std::filesystem::path root(plat::GetDataRoot());
    CopyString(contentInfoPath, 128, sfo.directory.empty() ? root.parent_path().string() : sfo.directory);
    CopyString(usrdirPath, 128, root.string());
    return kGameResult_Ok;
}

void LogIntParams()
{
    static const struct {
        uint32_t id;
        const char* name;
    } kParams[] = {
        { kGameParam_ParentalLevel, "PARENTAL_LEVEL" },
        { kGameParam_Resolution, "RESOLUTION" },
        { kGameParam_SoundFormat, "SOUND_FORMAT" },
    };
    for (const auto& param : kParams) {
        int32_t value = 0;
        const int32_t result = GameGetParamInt(param.id, &value);
        if (result < 0)
            Print("cellGameGetParamInt() Error id:[%s] : 0x%x\n", param.name, (unsigned)result);
        else
            Print("cellGameGetParamInt() OK id:[%s] value = [%d]\n", param.name, value);
    }
}

void LogStringParam(uint32_t id, const char* name)
{
    char text[128] = {};
    const int32_t result = GameGetParamString(id, text, sizeof text);
    if (result < 0)
        Print("cellGameGetParamString() Error id:[%s] : 0x%x\n", name, (unsigned)result);
    else
        Print("cellGameGetParamString() OK id:[%s] string = [%s]\n", name, text);
}

void LogSizes(const GameContentSize& size)
{
    Print("  hddFreeSizeKB = [%d] sizeKB = [%d] sysSizeKB = [%d]\n", size.hddFreeSizeKB, size.sizeKB,
          size.sysSizeKB);
}

int32_t PermitContent(char* contentInfoPath, char* usrdirPath)
{
    const int32_t result = ContentPermit(contentInfoPath, usrdirPath);
    if (result < 0) {
        Print("cellGameContentPermit() Error: 0x%x\n", (unsigned)result);
    } else {
        Print("cellGameContentPermit() OK\n");
        Print("  contentInfoPath:[%s]\n", contentInfoPath);
        Print("  usrdirPath:[%s]\n", usrdirPath);
    }
    return result;
}

} // namespace

// @func 0x005721B4 GetHddFreeSizeKB
int32_t GetHddFreeSizeKB()
{
    return g_hddFreeSizeKB;
}

const GameContentInfo& GetGameContentInfo()
{
    return s_info;
}

int32_t GameGetParamInt(uint32_t id, int32_t* value)
{
    const Sfo& sfo = GetSfo();
    const auto it = sfo.values.find(ParamKey(id));
    if (it == sfo.values.end() || !it->second.isInt)
        return kGameError_NotFound;
    *value = it->second.intValue;
    return kGameResult_Ok;
}

int32_t GameGetParamString(uint32_t id, char* buffer, uint32_t bufferSize)
{
    const Sfo& sfo = GetSfo();
    const auto it = sfo.values.find(ParamKey(id));
    if (it == sfo.values.end() || it->second.isInt)
        return kGameError_NotFound;
    CopyString(buffer, bufferSize, it->second.text);
    return kGameResult_Ok;
}

// @func 0x0056C8E4 GameBootCheck
int32_t GameBootCheck()
{
    GameContentInfo& info = s_info;
    info = GameContentInfo();

    int32_t result = BootCheck(info);
    if (result < 0) {
        Print("cellGameBootCheck() Error: 0x%x\n", (unsigned)result);
        return result;
    }

    Print("cellGameBootCheck() OK\n");
    Print("  get_type = [%d]\n", info.type);
    if (info.type == kGameType_Disc)
        Print("    CELL_GAME_GAMETYPE_DISC\n");
    if (info.type == kGameType_Hdd)
        Print("    CELL_GAME_GAMETYPE_HDD\n");

    static const struct {
        uint32_t bit;
        const char* text;
    } kAttributes[] = {
        { kGameAttr_Patch, "    CELL_GAME_ATTRIBUTE_PATCH\n" },
        { kGameAttr_AppHome, "    CELL_GAME_ATTRIBUTE_APP_HOME\n" },
        { kGameAttr_Debug, "    CELL_GAME_ATTRIBUTE_DEBUG\n" },
        { kGameAttr_XmbBuy, "    CELL_GAME_ATTRIBUTE_XMBBUY\n" },
        { kGameAttr_Commerce2Browser, "    CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER\n" },
        { kGameAttr_InviteMessage, "    CELL_GAME_ATTRIBUTE_INVITE_MESSAGE\n" },
        { kGameAttr_CustomDataMessage, "    CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE\n" },
        { kGameAttr_WebBrowser, "    CELL_GAME_ATTRIBUTE_WEB_BROWSER\n" },
    };
    Print("  get_attributes = [0x%08x]\n", info.attributes);
    for (const auto& attribute : kAttributes) {
        if (info.attributes & attribute.bit)
            Print("%s", attribute.text);
    }
    Print("  dirName = [%s]\n", info.dirName);
    LogSizes(info.size);
    g_hddFreeSizeKB = info.size.hddFreeSizeKB;

    LogIntParams();
    LogStringParam(kGameParam_TitleCurrent, "TITLE_CURRENT");
    LogStringParam(kGameParam_TitleDefault, "TITLE_DEFAULT");
    LogStringParam(kGameParam_TitleId, "TITLE_ID");

    result = PermitContent(info.contentInfoPath, info.usrdirPath);
    if (result < 0 || !(info.attributes & kGameAttr_Patch))
        return result;

    // Booted as a patch: describe the patch content as well. The PC build
    // never reports the patch attribute, this is kept for completeness.
    info.size = GameContentSize();
    Print("cellGamePatchCheck() OK\n");
    LogSizes(info.size);
    LogIntParams();
    static const char* const kTitleNames[] = {
        "TITLE_CURRENT", "TITLE_DEFAULT", "TITLE_JAPANESE", "TITLE_ENGLISH", "TITLE_FRENCH",
        "TITLE_SPANISH", "TITLE_GERMAN", "TITLE_ITALIAN", "TITLE_DUTCH", "TITLE_PORTUGUESE",
        "TITLE_RUSSIAN", "TITLE_KOREAN", "TITLE_CHINESE_T", "TITLE_CHINESE_S", "TITLE_FINNISH",
        "TITLE_SWEDISH", "TITLE_DANISH", "TITLE_NORWEGIAN",
    };
    for (uint32_t id = 0; id < sizeof kTitleNames / sizeof kTitleNames[0]; ++id)
        LogStringParam(id, kTitleNames[id]);
    LogStringParam(kGameParam_TitleId, "TITLE_ID");
    LogStringParam(kGameParam_Version, "VERSION");
    return PermitContent(info.patchContentInfoPath, info.patchUsrdirPath);
}

} // namespace ape
