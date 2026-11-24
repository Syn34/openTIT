// openTIT - Languages: the supported-language list, the localised GUI string
// table and the folders used for voice-over streams.
#pragma once

#include <cstdint>

namespace ape { class StringTable; }

namespace sly {

class SlyGame;

constexpr int kLanguageCount = 20;

enum Language : int32_t {
    kLang_English = 0,
    kLang_French,
    kLang_Italian,
    kLang_German,
    kLang_Spanish,
    kLang_Dutch,
    kLang_Portuguese,
    kLang_Finnish,
    kLang_Swedish,
    kLang_Danish,
    kLang_Norwegian,
    kLang_Polish,
    kLang_Russian,
    kLang_Turkish,
    kLang_Czech,
    kLang_Greek,
    kLang_EnglishUk,
    kLang_PortugueseBr,
    kLang_Japanese,
    kLang_Korean,
};

struct LanguageInfo {
    const char* fileSuffix;   // used in "GUI/Strings_%s.str"
    const char* nativeName;   // shown in the language menu (UTF-8)
    const char* code;         // short language code (stored first on the console)
};

const LanguageInfo& GetLanguageInfo(int32_t language);
// Folder under Audio/Stream/VO used for a language's voice-over.
const char* VoiceOverFolder(int32_t language);

// Current language (0x010C5A48).
int32_t CurrentLanguage();
void SetCurrentLanguage(int32_t language);
// Language whose voice-over is used (UK English shares the English VO).
int32_t VoiceOverLanguage();
// First entry of the supported-language list.
int32_t DefaultLanguage();
bool IsLanguageSupported(int32_t language);

// The system language as reported by the platform. NOTE: provided by the
// system-utility layer on the console (0x005DBC60/0x005DBC84); stubbed.
int32_t GetSystemLanguage();
void ApplySystemLanguage(int32_t language);

// Owner of the GUI string table (the game world on the console).
struct LocalizationState {
    ape::StringTable* strings = nullptr;  // game +0x24
    uint8_t savedLanguage = 0;            // game +0x1050 value kept in the save
    bool saveDirty = false;               // game +0x1054
};

// Switches the game language (or the system one unless the debug override
// is set), reloading the string table and the VO folder when it changes.
void SetLanguage(LocalizationState& state, int32_t language);
// Loads the GUI string table for the current language.
ape::StringTable* LoadLanguageStrings();
// Debug override that keeps the requested language (0x010B8A64).
void SetForceLanguage(bool force);

} // namespace sly
