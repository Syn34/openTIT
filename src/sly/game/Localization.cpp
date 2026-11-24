// openTIT - Languages and the localised GUI string table.
#include "sly/game/Localization.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/xml/ApeStringTable.h"
#include "sly/game/AudioSystem.h"

namespace sly {

namespace {

// 0x00AE6E3C: {code, file suffix, native name} per language.
const LanguageInfo kLanguages[kLanguageCount] = {
    {"English", "English", "en"},
    {"French", "Fran\xC3\xA7" "ais", "fr"},
    {"Italian", "Italiano", "it"},
    {"German", "Deutsch", "de"},
    {"Spanish", "Espa\xC3\xB1ol", "es"},
    {"Dutch", "Nederlands", "nl"},
    {"Portuguese", "Portugu\xC3\xAAs", "pt"},
    {"Finnish", "Suomi", "fi"},
    {"Swedish", "Svenska", "sv"},
    {"Danish", "Dansk", "da"},
    {"Norwegian", "Norsk", "no"},
    {"Polish", "Polski", "pl"},
    {"Russian", "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9", "ru"},
    {"Turkish", "T\xC3\xBCrk\xC3\xA7" "e", "tr"},
    {"Czech", "\xC4\x8C" "e\xC5\xA1tina", "cs"},
    {"Greek", "\xCE\x95\xCE\xBB\xCE\xBB\xCE\xB7\xCE\xBD\xCE\xB9\xCE\xBA\xCE\xAC", "el"},
    {"EnglishUk", "English", "uk"},
    {"PortugueseBr", "Portugu\xC3\xAAs", "bp"},
    {"Japanese", "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", "ja"},
    {"Korean", "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4", "kr"},
};

// 0x00F65B90: voice-over folder per language. Languages without their own
// recording use the English one.
const char* const kVoiceOverFolders[kLanguageCount] = {
    "English", "French", "Italian", "German", "Spanish", "Dutch", "Portuguese",
    "Finnish", "Swedish", "Danish", "Norwegian", "Polish", "Russian", "Turkish",
    "Czech", "Greek", "English", "BrazilianPortuguese", "English", "English",
};

// 0x00F65D90 / 0x00F65DD0: languages offered by this build, default first.
const int32_t kSupportedLanguages[] = {
    kLang_EnglishUk, kLang_French, kLang_Italian, kLang_German, kLang_Spanish,
    kLang_Dutch, kLang_Portuguese, kLang_Finnish, kLang_Swedish, kLang_Danish,
    kLang_Norwegian, kLang_Polish, kLang_Russian, kLang_Turkish, kLang_Czech,
    kLang_Greek,
};
constexpr int kSupportedCount = (int)(sizeof(kSupportedLanguages) / sizeof(kSupportedLanguages[0]));

int32_t g_currentLanguage = kLang_English;   // 0x010C5A48
bool g_forceLanguage = false;                // 0x010B8A64
int32_t g_systemLanguage = kLang_English;

ape::StringTable* CreateStringTable(int32_t language)
{
    return new ape::StringTable(
        ape::TempFormat("GUI/Strings_%s.str", GetLanguageInfo(language).fileSuffix));
}

} // namespace

const LanguageInfo& GetLanguageInfo(int32_t language)
{
    if (language < 0 || language >= kLanguageCount)
        language = kLang_English;
    return kLanguages[language];
}

const char* VoiceOverFolder(int32_t language)
{
    if (language < 0 || language >= kLanguageCount)
        language = kLang_English;
    return kVoiceOverFolders[language];
}

int32_t CurrentLanguage()
{
    return g_currentLanguage;
}

void SetCurrentLanguage(int32_t language)
{
    g_currentLanguage = language;
}

// @func 0x0048CEE0 VoiceOverLanguage
int32_t VoiceOverLanguage()
{
    return g_currentLanguage == kLang_EnglishUk ? kLang_English : g_currentLanguage;
}

// @func 0x00483A24 DefaultLanguage
int32_t DefaultLanguage()
{
    return kSupportedLanguages[0];
}

bool IsLanguageSupported(int32_t language)
{
    for (int i = 0; i < kSupportedCount; ++i) {
        if (kSupportedLanguages[i] == language)
            return true;
    }
    return false;
}

// TODO: system language comes from the platform's system-utility layer.
int32_t GetSystemLanguage()
{
    return g_systemLanguage;
}

void ApplySystemLanguage(int32_t language)
{
    g_systemLanguage = language;
}

void SetForceLanguage(bool force)
{
    g_forceLanguage = force;
}

// @func 0x0046E89C SetLanguage
void SetLanguage(LocalizationState& state, int32_t language)
{
    if (!g_forceLanguage)
        language = GetSystemLanguage();
    if (!IsLanguageSupported(language)) {
        const int32_t system = GetSystemLanguage();
        language = IsLanguageSupported(system) ? system : DefaultLanguage();
    }

    ApplySystemLanguage(language);
    if (g_currentLanguage != language) {
        g_currentLanguage = language;
        delete state.strings;
        state.strings = CreateStringTable(language);
        ape::SetGuiStringTable(state.strings);
        audio::UpdateVoiceOverPath();
    }
    if (state.savedLanguage != (uint8_t)language) {
        state.savedLanguage = (uint8_t)language;
        state.saveDirty = true;
    }
}

// Part of the GUI start-up (0x004678B8): picks the current language if it
// is supported, the default otherwise, and loads its strings.
ape::StringTable* LoadLanguageStrings()
{
    const int32_t language = IsLanguageSupported(g_currentLanguage) ? g_currentLanguage
                                                                   : DefaultLanguage();
    ape::StringTable* table = CreateStringTable(language);
    ape::SetGuiStringTable(table);
    return table;
}

} // namespace sly
