// openTIT - Game shell helpers of the game world: options applied to the
// display, the cutscene queue, GUI helpers used by the menus, player blocks.
#include "sly/game/SlyGame.h"

#include "agl/AglDisplay.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/level/ApeGui.h"
#include "ape/sys/ApeFrameClock.h"
#include "sly/game/hud/HudMeter.h"
#include "sly/tuning/GameTuning.h"
#include "sly/entity/Entity.h"
#include "sly/game/AppRuntime.h"

#include <cmath>

namespace sly {

// @func 0x00470560 BlurAnimation::Start
void BlurAnimation::Start(float fromAmount, float toAmount, float time, uint32_t passes, bool disableAtEnd)
{
    from = fromAmount;
    to = toAmount;
    duration = time;
    quality = passes;
    elapsed = 0.0f;
    disableWhenDone = disableAtEnd;
    active = true;
}

// @func 0x00470590 SlyGame::SetLetterbox
void SlyGame::SetLetterbox(bool on)
{
    if (ape::ApeGuiObject* letterbox = ape::FindGuiObject("InGame.Cutscene.Letterbox", nullptr))
        letterbox->SetVisible(on);
}

// Shows or hides the language choice of a menu screen ("Data.Language" at
// the root and "Menu.Language" under `screen`).
// @func 0x004704A8 ShowLanguageMenu
void ShowLanguageMenu(ape::ApeGuiObject* screen, bool visible)
{
    // NOTE: the console passes the scope of the first lookup in a register
    // it does not set up; the root is assumed.
    if (ape::ApeGuiObject* data = ape::FindGuiObject("Data.Language", nullptr))
        data->SetVisible(visible);
    if (ape::ApeGuiObject* menu = ape::FindGuiObject("Menu.Language", screen))
        menu->SetVisible(visible);
}

// First object (depth first, siblings in order) of a GUI tree that can take
// the pad focus and is visible.
// @func 0x0046E7B0 FindFocusableGuiObject
ape::ApeGuiObject* FindFocusableGuiObject(ape::ApeGuiObject* object)
{
    for (; object; object = object->nextSibling) {
        if (object->GetFlag30() && object->IsVisible())
            return object;
        for (ape::ApeGuiObject* child = object->firstChild; child; child = child->nextSibling) {
            if (ape::ApeGuiObject* found = FindFocusableGuiObject(child))
                return found;
        }
    }
    return nullptr;
}

// @func 0x0046C8D0 SlyGame::ApplyBrightness
void SlyGame::ApplyBrightness() const
{
    // Brightness 0..1 maps to a gamma of 1.2..0.8.
    float gamma = (1.0f - settings.brightness) * 0.40000004f + 0.8f;
    if (1.2f - gamma < 0.0f)
        gamma = 1.2f;
    if (gamma - 0.8f < 0.0f)
        gamma = 0.8f;
    agl::SetDisplayGamma(gamma);
}

// @func 0x0046C830 SlyGame::ApplyStereoOptions
void SlyGame::ApplyStereoOptions() const
{
    if (!agl::GetDisplaySettings().stereoAvailable)
        return;
    agl::SetStereoConvergence(settings.screenScale * 0.09f + 0.01f);
    agl::SetStereoEnabled((settings.flags & (1u << 26)) != 0);
}

// The HUD uses its corrected layouts on a 4:3 output other than 720 lines.
// NOTE: name from the "...480Correct" HUD tuning it selects.
// @func 0x0046C7B0 IsSdHudLayout
bool IsSdHudLayout()
{
    const agl::AglDisplaySettings& display = agl::GetDisplaySettings();
    return display.height != 720 && !display.widescreen;
}

// @func 0x0046C750 SlyGame::IsSequenceQueued
bool SlyGame::IsSequenceQueued(uint32_t sequence) const
{
    for (const QueuedCutscene* entry : cutsceneQueue) {
        if (entry->sequence == sequence)
            return true;
    }
    return false;
}

// @func 0x00465C50 GamePlayer::ReleaseCharacter
void GamePlayer::ReleaseCharacter()
{
    if (character) {
        character->Release(false);
        character = nullptr;
    }
}

// @func 0x0045B160 AppPlayer::GetAnalog
float AppPlayer::GetAnalog(uint32_t index) const
{
    if (!pad || !(analogMask & (1u << (index & 31))))
        return 0.0f;
    return index < uint32_t(ape::input::kAnalog_Count) ? pad->GetAnalog(int(index)) : 0.0f;
}

// @func 0x0045B820 AppPlayer::IsPadConnected
bool AppPlayer::IsPadConnected() const
{
    return pad && pad->IsConnected();
}

// @func 0x004843A4 SlyGame::FrameTime
float SlyGame::FrameTime() const
{
    float dt = timeSource ? timeSource->deltaTime : ape::GetFrameClock().deltaTime;
    if (timeRamp.active)
        dt *= timeRamp.value;
    return dt;
}

// @func 0x00484488 SlyGame::SetListenerCameraWeight
void SlyGame::SetListenerCameraWeight(float weight, float time, float curve)
{
    if (std::fabs(time) < 1e-5f) {
        listenerCameraWeight = weight;
        time = 0.0f;
    } else {
        listenerWeightStart = listenerCameraWeight;
        listenerWeightTarget = weight;
        listenerWeightCurve = curve;
        listenerWeightProgress = 0.0f;
    }
    listenerWeightTime = time;
}

// @func 0x0047BBEC SlyGame::ShowPlayerPower
void SlyGame::ShowPlayerPower(uint32_t player) const
{
    const GamePlayer& block = players[player];
    // NOTE: the console passes the player index as well; the HUD keeps a
    // single meter B.
    hud::SetMeterBSecondFill(block.meter / block.meterMax);
}

// @func 0x0047BE90 ContextualMarkerFor
uint32_t ContextualMarkerFor(uint32_t characterHash)
{
    switch (characterHash) {
    case 0x00014120u: return g_gameContextualSly.Get();            // "Sly"
    case 0x88397820u: return g_gameContextualMurray.Get();
    case 0x1D8C2817u: return g_gameContextualBentley.Get();
    case 0x72CC3493u: return g_gameContextualRioichi.Get();
    case 0x933D1EE0u: return g_gameContextualKidCooper.Get();
    case 0xCAC02187u: return g_gameContextualCavemanCooper.Get();
    case 0xE08BB723u: return g_gameContextualSirGalleth.Get();
    case 0x04B05162u: return g_gameContextualSalim.Get();
    case 0xDF0611F9u: return g_gameContextualCarmelita.Get();     // "CarmelitaFox"
    default: return g_gameContextualSly.Get();
    }
}

} // namespace sly
