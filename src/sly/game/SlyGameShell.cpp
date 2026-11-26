// openTIT - Game shell helpers of the game world: options applied to the
// display, the cutscene queue, GUI helpers used by the menus, player blocks.
#include "sly/game/SlyGame.h"

#include "agl/AglDisplay.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/level/ApeGui.h"
#include "ape/sys/ApeFrameClock.h"
#include "sly/game/hud/HudMeter.h"
#include "sly/tuning/GameTuning.h"
#include "sly/entity/Entity.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/AudioSystem.h"
#include "sly/entity/Character.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/LevelParams.h"
#include "sly/gameplay/ReturnTreasure.h"
#include "ape/xml/ApeXml.h"

#include <cmath>
#include <cstring>

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

// TODO(0x00481A88 CreateLevelParams): parses "headlineString", the job
// headline overrides and the level settings.
LevelParams* CreateLevelParams(ape::xml::XmlDocument& document, ape::xml::XmlElement* element)
{
    (void)document;
    (void)element;
    return nullptr;
}

// @func 0x004829F0 SlyGame::LoadLevelParams
void SlyGame::LoadLevelParams(ape::file::File& file)
{
    constexpr uint32_t kLevelParamsElement = 0x1AF675EAu;
    ape::xml::XmlDocument document;
    if (!document.LoadFromStream(file, nullptr))
        return;
    ape::xml::XmlElement* element = document.FirstChildElement();
    if (!element) {
        document.GetDocumentPath();   // NOTE: the console builds a diagnostic and drops it
        return;
    }
    for (; element; element = element->NextSiblingElement()) {
        if (!element->IsValid() || element->GetNameHash() != kLevelParamsElement)
            continue;
        if ((flags1C >> 6) != 0 || levelParams)
            continue;
        if (LevelParams* params = CreateLevelParams(document, element))
            levelParams = params;
    }
}

// @func 0x00476C30 SlyGame::LevelParamsMode1
bool SlyGame::LevelParamsMode1() const
{
    return levelParams && levelParams->field_1544 == 1;
}

// Runs a script of the default library once on its own: the instance is
// registered (and dropped when finished) and given its first frame.
// @func 0x00491000 RunGlobalScript
void RunGlobalScript(uint32_t scriptHash)
{
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
    if (!library)
        return;
    ape::ApeAnimScript* script = library->FindScript(scriptHash);
    if (!script)
        return;
    auto* instance = new ape::ApeAnimScriptInstance(script);
    ape::ApeAnimScriptInstance::Register(instance);
    instance->Update(1.0f / 60.0f);
    instance->SetRemoveWhenFinished(true);
}

// @func 0x00489DF8 SlyGame::StartReturnTreasure
void SlyGame::StartReturnTreasure(float limit, entity::Entity* carrier, entity::Entity* treasure,
                                  uint32_t carrierBone, const ape::Vec3& offset)
{
    if (returnTreasure)
        returnTreasure->Start(limit, carrier, treasure, carrierBone, offset);
}

// @func 0x00489E78 SlyGame::ReturnTreasureActive
bool SlyGame::ReturnTreasureActive() const
{
    return returnTreasure && returnTreasure->IsActive();
}

// @func 0x00489EAC SlyGame::CompleteReturnTreasure
void SlyGame::CompleteReturnTreasure()
{
    if (ReturnTreasureActive())
        returnTreasure->Complete();
}

// @func 0x00489EF4 SlyGame::UpdateReturnTreasurePlacement
void SlyGame::UpdateReturnTreasurePlacement()
{
    if (ReturnTreasureActive())
        returnTreasure->UpdatePlacement();
}

// @func 0x00489F3C SlyGame::StopReturnTreasure
void SlyGame::StopReturnTreasure(bool lost)
{
    if (ReturnTreasureActive())
        returnTreasure->Stop(lost);
}

// @func 0x00489F88 SlyGame::PauseReturnTreasure
void SlyGame::PauseReturnTreasure(bool on)
{
    if (ReturnTreasureActive())
        returnTreasure->SetPaused(on);
}

// @func 0x00489FD4 SlyGame::RefreshReturnTreasureHud
void SlyGame::RefreshReturnTreasureHud()
{
    if (ReturnTreasureActive())
        returnTreasure->RefreshHud();
}

// @func 0x0048A01C SlyGame::SetReturnTreasureHudActive
void SlyGame::SetReturnTreasureHudActive(bool on)
{
    if (ReturnTreasureActive())
        returnTreasure->SetHudActive(on);
}

// @func 0x00488D68 SlyGame::RestartLevelState
void SlyGame::RestartLevelState()
{
    RequestMainState(3);
    if (uint8_t* data = StateData(0))
        std::memset(data, 0, 0x34);
}

// Sub state data of the job-complete screen (sub state 7).
struct JobCompleteData {
    uint8_t field_00[0x44];
    uint32_t field_44;
    uint32_t completion;    // +0x48 what CompleteRunningJob returned
};
static_assert(sizeof(JobCompleteData) <= 0x80, "fits the state data block");

// @func 0x0048EC44 SlyGame::OnJobFinished
bool SlyGame::OnJobFinished()
{
    SetLetterbox(false);
    game::JobManager* jobs = game::GetJobManager();
    finishedJobName = (jobs && jobs->instance) ? jobs->instance->nameHash : 0;
    const uint32_t completion = jobs ? jobs->CompleteRunningJob() : 0;
    progress.AddPendingCount(pendingCounts[players[0].index & 7]);

    if (jobs && !game::ProgressFlag149() && !game::IsInJob() && jobs->currentJob &&
        jobs->currentJob->ProgressState() == 2) {
        jobs->RestartEpisodeById(jobs->currentJob->field_2D4);
    } else if (completion == 0) {
        RequestSubState(2);
    } else {
        RequestSubState(7);
        if (auto* data = reinterpret_cast<JobCompleteData*>(StateData(1))) {
            data->field_44 = 0;
            data->completion = completion;
        }
    }
    return false;
}

// @func 0x00482D14 SlyGame::RecordEvent
void SlyGame::RecordEvent(uint32_t player, const GameEvent& event)
{
    if (event.type == kEvent_01 && timeSource) {
        entity::Entity* tracked = timeSource->trackedEntity;
        if (tracked && (tracked->flags & (1u << 18)) && tracked->nameHash == event.objectId) {
            timeSource->field_158 = 0;
            timeSource->trackedEntity = nullptr;
            timeSource->tracking = false;
        }
    }

    const bool playing = transition != 2 && GetState(0) == 1 && (GetState(1) == 2 || GetState(1) == 3);
    if (playing && cutsceneQueue.Count() == 0) {
        if (player < localPlayerCount)
            players[player].eventLog.Push(event);
        if (game::JobManager* jobs = game::GetJobManager())
            jobs->OnGameEvent(int(player), event);
        return;
    }
    if (cutsceneQueue.Count() != 0 && (flags2D & 0x04))
        players[0].deferredEvents.Push(event);
}

// @func 0x0048A57C SlyGame::QuitToFrontEnd
void SlyGame::QuitToFrontEnd()
{
    if (game::JobManager* jobs = game::GetJobManager())
        jobs->EndJob(false);
    if (ape::ApeGuiObject* letterbox = ape::FindGuiObject("InGame.Cutscene.Letterbox", nullptr))
        letterbox->SetVisible(false);
    RequestMainState(0);
    RequestSubState(7);
}

// @func 0x0048A4E0 SlyGame::OnCinematicSkipped
void SlyGame::OnCinematicSkipped()
{
    audio::SoundName* music = audio::CreateSoundName(g_audioPostCinematicSkipMusic.Get(), 0);
    if (audio::SoundNameHasDef(music))
        audio::SetPendingMusic(music, false, false);
    else
        audio::StopMusic(true);
    audio::SoundNameRelease(music);
    stateFlags = uint8_t((stateFlags & ~0x20) | 0x60);
}

// @func 0x0047BC68 SlyGame::OnPlayerDown
void SlyGame::OnPlayerDown(uint32_t player)
{
    // NOTE: the console passes the player index to the HUD; it keeps a single
    // meter B.
    hud::SnapMeterB();
    if (ReturnTreasureActive())
        returnTreasure->OnEntityRemoved(players[player].character);
}

// @func 0x0048C7DC SlyGame::AnswerYes
void SlyGame::AnswerYes()
{
    if (uint8_t* data = StateData(1))
        data[0] = 1;
    stateFlags |= 0x01;
}

// @func 0x0048C804 SlyGame::AnswerNo
void SlyGame::AnswerNo()
{
    if (uint8_t* data = StateData(1))
        data[0] = 1;
    stateFlags &= ~0x01;
}

// @func 0x00488004 SlyGame::SetPlayerPhysicsTarget
void SlyGame::SetPlayerPhysicsTarget(uint32_t player, const ape::Vec3& point)
{
    entity::Entity* character = players[player].character;
    if (character && character->HasTypeBit(3))
        static_cast<entity::Character*>(character)->SetPhysicsTarget(point);
}

// @func 0x0048C6EC SetDarkScreen
void SetDarkScreen(float distance, float ramp, float minValue, bool enabled)
{
    g_gameDarkScreenEnabled.SetBool(enabled);
    g_gameDarkScreenDistance.SetFloat(distance);
    g_gameDarkScreenRamp.SetFloat(ramp);
    g_gameDarkScreenMinValue.SetFloat(minValue);
}

// @func 0x00467C4C TrophyCommId
const char* TrophyCommId()
{
    return "NPWR03581";
}

// @func 0x00488ADC PlayerPlayingScript
bool PlayerPlayingScript()
{
    entity::Entity* character = g_game ? g_game->PlayerCharacter(0) : nullptr;
    if (!character)
        return false;
    for (entity::EntityComponent* c = character->GetComponent(entity::kComp_17); c;
         c = character->GetComponent(entity::kComp_17, c)) {
        if (entity::AnimDriverState(c) == 7)
            return true;
    }
    return false;
}

} // namespace sly
