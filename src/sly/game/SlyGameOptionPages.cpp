// openTIT - Game world: the option menu pages' refresh functions (labels
// and sliders of a page show the current settings).
#include "sly/game/SlyGame.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "sly/tuning/GameTuning.h"
#include "agl/AglDisplay.h"
#include "sly/game/Localization.h"
#include "sly/levels/ARServer.h"
#include "sly/game/hud/HudHideoutMenu.h"
#include "sly/game/hud/HudHooks.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/goals/JobManager.h"

#include <cstring>

#include <algorithm>
#include <cmath>

namespace sly {

namespace {

// GUI object type of the labels (ApeGuiObject::GetType).
constexpr int32_t kGuiType_Label = 3;

// Sets the text of the label `name` under `menu` (other object types are
// left alone).
void SetLabel(ape::ApeGuiObject* menu, const char* name, const char* text)
{
    ape::ApeGuiObject* label = ape::FindGuiObject(name, menu);
    if (label && label->GetType() == kGuiType_Label)
        label->SetText(text, false);
}

// Places a slider's tick along "UI.Slider.PositionMin/Max" for a 0..1 value.
void SetSliderTick(ape::ApeGuiObject* menu, const char* name, float value)
{
    ape::ApeGuiObject* tick = ape::FindGuiObject(name, menu);
    if (!tick)
        return;
    ape::Vec3 position = tick->GetPosition();
    const float minimum = g_uISliderPositionMin.Get();
    position.x = minimum + (g_uISliderPositionMax.Get() - minimum) * value;
    tick->SetPosition(position);
}

const char* OnOff(bool on)
{
    return on ? "$STR_ON" : "$STR_OFF";
}

const char* NormalInverted(bool inverted)
{
    return inverted ? "$STR_INVERTED" : "$STR_NORMAL";
}

} // namespace

// @func 0x0046D398 SlyGame::RefreshControlsPage
// NOTE: the console passes the menu page record (its GUI screen at +0x0C).
void SlyGame::RefreshControlsPage(ape::ApeGuiObject* screen)
{
    ape::ApeGuiObject* menu = ape::FindGuiObject("Menu", screen);
    if (!menu)
        return;
    SetLabel(menu, "CameraX", NormalInverted(settings.flags & GameSettings::kFlag_InvertCameraX));
    SetLabel(menu, "CameraY", NormalInverted(settings.flags & GameSettings::kFlag_InvertCameraY));
    SetLabel(menu, "Vibration", OnOff(settings.flags & GameSettings::kFlag_Vibration));
}

// @func 0x0046D5C8 SlyGame::RefreshAudioPage
void SlyGame::RefreshAudioPage(ape::ApeGuiObject* screen)
{
    ape::ApeGuiObject* menu = ape::FindGuiObject("Menu", screen);
    if (!menu)
        return;
    SetSliderTick(menu, "VolumeSFX.Tick", settings.volumes[0]);
    SetSliderTick(menu, "VolumeMusic.Tick", settings.volumes[1]);
    SetSliderTick(menu, "VolumeVO.Tick", settings.volumes[2]);
    SetSliderTick(menu, "VolumeMovie.Tick", settings.volumes[3]);

    static const char* const kFormats[] = {"$STR_DEFAULT", "$STR_AUDIO_FORMAT_SS", "$STR_AUDIO_FORMAT_SH",
                                           "$STR_AUDIO_FORMAT_DPRII", "5.1", "7.1"};
    static const char* const kCenterSpeaker[] = {"$STR_DEFAULT", "$STR_AUDIO_CSS_SMALL", "$STR_AUDIO_CSS_LARGE"};
    static const char* const kDynamicRange[] = {"$STR_DEFAULT", "$STR_AUDIO_DR_MAX", "$STR_AUDIO_DR_WIDE",
                                                "$STR_AUDIO_DR_NORMAL", "$STR_AUDIO_DR_NARROW",
                                                "$STR_AUDIO_DR_MIDNIGHT"};
    // NOTE: the console does not check the indices.
    if (settings.audioFormat < 6)
        SetLabel(menu, "Format", kFormats[settings.audioFormat]);
    if (settings.centerSpeaker < 3)
        SetLabel(menu, "CenterSpeakerSize", kCenterSpeaker[settings.centerSpeaker]);
    if (settings.dynamicRange < 6)
        SetLabel(menu, "DynamicRange", kDynamicRange[settings.dynamicRange]);
}

} // namespace sly

namespace sly {

namespace {

// Greys out a control that the TV cannot use: no pad focus and a low alpha.
void DimControl(ape::ApeGuiObject* object)
{
    if (!object)
        return;
    object->SetFlag30(false);
    ape::xml::Color8 color = object->GetColor();
    color.a = 0x41;
    object->SetColor(color);
}

} // namespace

// @func 0x0046C988 SlyGame::RefreshDisplayPage
// Subtitles, brightness (applied to the display at once), radar, the 3D
// strength slider and switch (greyed out without a 3D TV), language and
// the AR server switch.
void SlyGame::RefreshDisplayPage(ape::ApeGuiObject* screen)
{
    constexpr uint32_t kFlag_Subtitles = 1u << 27;
    constexpr uint32_t kFlag_Stereo = 1u << 26;
    constexpr uint32_t kFlag_Radar = 1u << 25;
    constexpr uint32_t kFlag_ArServer = 1u << 24;
    const bool stereoTv = agl::GetDisplaySettings().stereoAvailable;

    ape::ApeGuiObject* menu = ape::FindGuiObject("Menu", screen);
    if (menu) {
        SetLabel(menu, "Subtitles", OnOff(settings.flags & kFlag_Subtitles));
        SetSliderTick(menu, "Brightness.Tick", settings.brightness);
        ApplyBrightness();
        SetLabel(menu, "Radar", OnOff(settings.flags & kFlag_Radar));
        if (!stereoTv) {
            DimControl(ape::FindGuiObject("Strength", menu));
            ape::ApeGuiObject* state = ape::FindGuiObject("State", menu);
            if (state && state->GetType() == kGuiType_Label)
                state->SetText(OnOff(settings.flags & kFlag_Stereo), false);
            DimControl(state);
        } else {
            const float strength = std::clamp(settings.screenScale, 0.0f, 1.0f);
            if (std::fabs(strength - settings.screenScale) > 1e-5f)
                field_1054[0] = 1;   // the options changed
            settings.screenScale = strength;
            SetSliderTick(menu, "Strength.Tick", strength);
            SetLabel(menu, "State", OnOff(settings.flags & kFlag_Stereo));
        }
        if (settings.language < kLanguageCount)
            SetLabel(menu, "Language", GetLanguageInfo(settings.language).nativeName);
        if (ape::ApeGuiObject* ar = ape::FindGuiObject("ARServer", menu); ar && ar->GetType() == kGuiType_Label) {
            ar->SetVisible(true);
            const bool on = (settings.flags & kFlag_ArServer) && levels::ARDiscoveredHost() == 0;
            ar->SetText(OnOff(on), false);
        }
        if (stereoTv)
            ApplyStereoOptions();
    }
    ape::ApeGuiObject* data = ape::FindGuiObject("Data", screen);
    if (data) {
        if (ape::ApeGuiObject* ar = ape::FindGuiObject("ARServer", data))
            ar->SetVisible(true);
    }
    if (!stereoTv) {
        // NOTE: the console looks these up under "Data" (null scope when
        // there is none).
        DimControl(ape::FindGuiObject("Strength", data));
        DimControl(ape::FindGuiObject("State", data));
    }
}

} // namespace sly

namespace sly {

// @func 0x0046F944 SlyGame::OnControlsPageAccept
void SlyGame::OnControlsPageAccept(const ape::GuiEvent& event)
{
    // Option objects of the controls page. NOTE: their names are unknown.
    constexpr uint32_t kObj_Vibration = 0xA1370272u;
    constexpr uint32_t kObj_CameraX = 0x4B8A2DD3u;
    constexpr uint32_t kObj_CameraY = 0x4B8A2DD4u;
    constexpr uint32_t kGuiEvent_Refresh = 0x10;
    if (!event.object)
        return;
    uint32_t flag = 0;
    switch (event.object->nameHash) {
    case kObj_Vibration: flag = GameSettings::kFlag_Vibration; break;
    case kObj_CameraX: flag = GameSettings::kFlag_InvertCameraX; break;
    case kObj_CameraY: flag = GameSettings::kFlag_InvertCameraY; break;
    default: return;
    }
    settings.flags ^= flag;
    hud::PlayHudSound(toggleSound);
    field_1054[0] = 1;   // the options changed
    ape::ApeGuiObject* menuGui = nullptr;
    if (auto* hideout = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200()))
        menuGui = hideout->gui;
    ape::SendGuiEvent(kGuiEvent_Refresh, event.pad, menuGui, 0);
}

} // namespace sly

namespace sly {

// @func 0x0046EA98 SlyGame::OnMovesPageEvent
void SlyGame::OnMovesPageEvent(const ape::GuiEvent& event)
{
    constexpr uint32_t kGuiEvent_FocusGained = 1;
    constexpr uint32_t kGuiEvent_FocusLost = 2;
    constexpr uint32_t kGuiEvent_Right = 10;
    constexpr uint32_t kGuiEvent_Left = 11;
    constexpr uint32_t kGuiEvent_Refresh = 0x10;
    constexpr uint8_t kCategoryCount = 4;
    if (!event.object || (event.type != kGuiEvent_Right && event.type != kGuiEvent_Left))
        return;
    hud::PlayHudSound(toggleSound);
    if (event.type == kGuiEvent_Right)
        movesCategory = uint8_t((movesCategory + 1) % kCategoryCount);
    else
        movesCategory = movesCategory != 0 ? uint8_t(movesCategory - 1) : uint8_t(kCategoryCount - 1);
    movesPage = 0;
    field_1285C7 = 0;

    const uint8_t pad = players[activePlayer % kMaxPlayers].padIndex;
    if (menuFocus)
        ape::SendGuiEvent(kGuiEvent_FocusLost, pad, menuFocus, 0);
    ape::ApeGuiObject* menuGui = nullptr;
    if (auto* hideout = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200()))
        menuGui = hideout->gui;
    ape::SendGuiEvent(kGuiEvent_Refresh, event.pad, menuGui, 0);

    menuFocus = FindFocusableGuiObject(event.object->parent);
    if (!menuFocus)
        return;
    // Both pointers jump to the focused object (screen centre -1..1).
    const ape::Vec3 centre = menuFocus->GetCenter();
    for (AppPlayer& player : GetAppRuntime().players) {
        float screen[2];
        ape::GetGuiScreenSize(screen);
        const float x = centre.x * 2.0f / screen[0] - 1.0f;
        const float y = centre.y * 2.0f / screen[1] - 1.0f;
        std::memcpy(&player.field_D0[0], &x, sizeof x);
        std::memcpy(&player.field_D0[1], &y, sizeof y);
        menuFocusPad = pad;
        ape::SendGuiEvent(kGuiEvent_FocusGained, pad, menuFocus, 0);
        ape::SetGuiSlot(pad, menuFocus);
    }
}

} // namespace sly

namespace sly {

// @func 0x0046EFB8 SlyGame::OnAudioPageEvent
void SlyGame::OnAudioPageEvent(const ape::GuiEvent& event)
{
    // Option objects of the audio page. NOTE: their names are unknown.
    constexpr uint32_t kObj_Format = 0x7BF2EBF7u;
    constexpr uint32_t kObj_Volume1 = 0x4CF5FB2Bu;
    constexpr uint32_t kObj_Volume3 = 0x4CF34C36u;
    constexpr uint32_t kObj_Volume0 = 0x273D570Bu;
    constexpr uint32_t kObj_CenterSpeaker = 0x1431E4EBu;
    constexpr uint32_t kObj_Volume2 = 0xBF338753u;
    constexpr uint32_t kObj_DynamicRange = 0xADCB105Eu;
    constexpr uint32_t kGuiEvent_Right = 10;
    constexpr uint32_t kGuiEvent_Refresh = 0x10;
    if (!event.object)
        return;
    const bool up = event.type == kGuiEvent_Right;

    // Moves a volume one slider step; returns false when it did not change.
    auto stepVolume = [&](float& volume) {
        const float before = volume;
        const float step = 1.0f / float(g_uISliderNumSteps.Get());
        volume = std::clamp(up ? volume + step : volume - step, 0.0f, 1.0f);
        hud::PlayHudSound(before == volume ? sliderStopSound : sliderSound);
        return before != volume;
    };
    // Cycles a choice over `count` options.
    auto cycle = [&](uint8_t& choice, uint8_t count) {
        choice = up ? uint8_t((choice + 1) % count) : (choice != 0 ? uint8_t(choice - 1) : uint8_t(count - 1));
        hud::PlayHudSound(toggleSound);
    };

    audio::AudioVolumeSettings& scales = audio::g_volumeSettings;
    switch (event.object->nameHash) {
    case kObj_Format:
        cycle(settings.audioFormat, 6);
        audio::SetSpeakerFormat(settings.audioFormat);
        break;
    case kObj_Volume1:
        stepVolume(settings.volumes[1]);
        scales.masterScale = settings.volumes[1];
        break;
    case kObj_Volume3:
        stepVolume(settings.volumes[3]);
        break;
    case kObj_Volume0:
        stepVolume(settings.volumes[0]);
        scales.sfxScale = settings.volumes[0];
        break;
    case kObj_CenterSpeaker:
        cycle(settings.centerSpeaker, 3);
        audio::SetCenterSpeakerSize(settings.centerSpeaker);
        break;
    case kObj_Volume2:
        stepVolume(settings.volumes[2]);
        scales.musicScale = settings.volumes[2];
        break;
    case kObj_DynamicRange:
        cycle(settings.dynamicRange, 6);
        audio::SetDynamicRange(settings.dynamicRange);
        break;
    default:
        return;
    }
    field_1054[0] = 1;   // the options changed
    ape::ApeGuiObject* menuGui = nullptr;
    if (auto* hideout = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200()))
        menuGui = hideout->gui;
    ape::SendGuiEvent(kGuiEvent_Refresh, event.pad, menuGui, 0);
}

} // namespace sly

namespace sly {

// @func 0x0046E438 SlyGame::RefreshMoviesPage
// NOTE: the console passes the menu page record (its GUI screen at +0x0C).
void SlyGame::RefreshMoviesPage(ape::ApeGuiObject* screen)
{
    constexpr uint8_t kSub_FrontEndExtras = 0x18;   // NOTE: name from use
    constexpr uint8_t kMenu_Movies = 0x0E;
    constexpr uint32_t kMoviesPerPage = 8;
    constexpr uint32_t kGroup_Cinematics = 0;
    constexpr uint32_t kGroup_Other = 1;

    ape::ApeTArray<game::Unlockable*> movies;
    const bool cinematics = (GetState(0) == 0 && GetState(1) == kSub_FrontEndExtras) || menu == kMenu_Movies;
    game::CollectUnlockables(cinematics ? kGroup_Cinematics : kGroup_Other, movies);

    ape::ApeGuiObject* list = ape::FindGuiObject("Moves", screen);
    if (!list)
        return;
    for (uint32_t i = 0; i < kMoviesPerPage; ++i) {
        ape::ApeGuiObject* label = ape::FindGuiObject(ape::TempFormat("Move%02d", i + 1), list);
        if (!label || label->GetType() != kGuiType_Label)
            continue;
        const uint32_t index = moviesPage * kMoviesPerPage + i;
        if (index < movies.Count()) {
            const game::Unlockable* movie = movies[index];
            const bool known = game::IsUnlocked(static_cast<uint32_t>(movie->index)) || movie->defaultUnlocked;
            label->SetText(known ? movie->movieName : "$STR_UNKNOWN", false);
            label->SetFlag30(true);
        } else {
            label->SetText("", false);
            label->SetFlag30(false);
        }
    }
}

} // namespace sly

namespace sly {

// @func 0x00472DF8 SlyGame::OnMoviesPageEvent
void SlyGame::OnMoviesPageEvent(const ape::GuiEvent& event)
{
    constexpr uint32_t kGuiEvent_FocusGained = 1;
    constexpr uint32_t kGuiEvent_FocusLost = 2;
    constexpr uint32_t kGuiEvent_Right = 10;
    constexpr uint32_t kGuiEvent_Left = 11;
    constexpr uint32_t kGuiEvent_Refresh = 0x10;
    // NOTE: both directions do the same on the console.
    if (!event.object || (event.type != kGuiEvent_Right && event.type != kGuiEvent_Left))
        return;
    moviesPage = 0;
    field_1285C9 = 0;

    const uint8_t pad = players[activePlayer % kMaxPlayers].padIndex;
    if (menuFocus)
        ape::SendGuiEvent(kGuiEvent_FocusLost, pad, menuFocus, 0);
    ape::ApeGuiObject* menuGui = nullptr;
    if (auto* hideout = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200()))
        menuGui = hideout->gui;
    ape::SendGuiEvent(kGuiEvent_Refresh, event.pad, menuGui, 0);

    menuFocus = FindFocusableGuiObject(event.object->parent);
    if (!menuFocus)
        return;
    const ape::Vec3 centre = menuFocus->GetCenter();
    for (AppPlayer& player : GetAppRuntime().players) {
        float screen[2];
        ape::GetGuiScreenSize(screen);
        const float x = centre.x * 2.0f / screen[0] - 1.0f;
        const float y = centre.y * 2.0f / screen[1] - 1.0f;
        std::memcpy(&player.field_D0[0], &x, sizeof x);
        std::memcpy(&player.field_D0[1], &y, sizeof y);
        menuFocusPad = pad;
        ape::SendGuiEvent(kGuiEvent_FocusGained, pad, menuFocus, 0);
        ape::SetGuiSlot(pad, menuFocus);
    }
}

} // namespace sly
