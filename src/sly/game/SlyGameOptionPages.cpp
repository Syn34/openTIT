// openTIT - Game world: the option menu pages' refresh functions (labels
// and sliders of a page show the current settings).
#include "sly/game/SlyGame.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "sly/tuning/GameTuning.h"
#include "agl/AglDisplay.h"
#include "sly/game/Localization.h"
#include "sly/levels/ARServer.h"

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
