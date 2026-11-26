// openTIT - The options block kept in the settings save.
#include "sly/save/GameSettings.h"

#include "ape/file/ApeFile.h"
#include "sly/game/Localization.h"
#include "sly/save/SaveGame.h"

namespace sly {

namespace {

// Save icon shown by the system save browser (0x0101278C / size 0x01012790).
void* g_saveIcon = nullptr;
uint32_t g_saveIconSize = 0;
bool g_saveAvailable = false;                  // 0x0101278A

// TODO(game world +0x1286A4): the options menu keeps an object there while
// the display page is open (released through its deleting destructor). It is
// not modelled yet.
void ReleaseDisplayPreviewObject() {}

} // namespace

// @func 0x000ADC28 GameSettings::ResetAudio
void GameSettings::ResetAudio()
{
    for (float& volume : volumes)
        volume = 1.0f;
}

// @func 0x000ADC40 GameSettings::ResetControls
void GameSettings::ResetControls()
{
    flags = (flags & ~kControlFlagsMask) | kControlDefaults;
}

// @func 0x000ADC5C GameSettings::ResetDisplay
void GameSettings::ResetDisplay()
{
    brightness = 0.5f;
    flags = (flags & ~kDisplayFlagsMask) | kDisplayDefaults;
    screenScale = 1.0f;
    ReleaseDisplayPreviewObject();
}

// @func 0x000ADCF8 GameSettings::Reset
void GameSettings::Reset()
{
    *this = GameSettings{};
    ResetDisplay();
    ResetAudio();
    ResetControls();
    language = static_cast<uint8_t>(DefaultLanguage());
}

// @func 0x000AE4A4 SaveSystemInit
void SaveSystemInit()
{
    g_saveIcon = ape::file::LoadFile("System/Save/ICON0.PNG", &g_saveIconSize);
    // NOTE: the console loads the save-data, trophy and two more system
    // modules and reports saving unavailable if one fails; the PC save layer
    // (SaveGame.cpp) only needs a writable folder.
    g_saveAvailable = true;
}

// @func 0x000AE660 SaveSystemShutdown
void SaveSystemShutdown()
{
    g_saveAvailable = false;
    if (g_saveIcon) {
        ape::file::FreeFileData(g_saveIcon);
        g_saveIcon = nullptr;
        g_saveIconSize = 0;
    }
}

bool SaveSystemAvailable() { return g_saveAvailable; }

// @func 0x000AE784 SaveToLooseFile
int32_t SaveToLooseFile(const char* /*path*/, const void* /*data*/, uint32_t /*size*/)
{
    return kSave_Failed;
}

} // namespace sly
