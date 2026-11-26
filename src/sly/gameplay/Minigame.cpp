// openTIT - The level's active game-mode object (see Minigame.h).
#include "sly/gameplay/Minigame.h"

#include "sly/gameplay/GameplayWorld.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/game/SlyGame.h"
#include "sly/game/hud/HudManager.h"

namespace sly {
namespace gameplay {

namespace {

constexpr uint32_t kScriptState_End = 0x00010CBBu;   // "End"
constexpr float kScriptFirstStep = 0.0167f;

// Lets the running director script finish its "End" state and leave.
void EndScript(ape::ApeAnimScriptInstance*& script)
{
    if (!script)
        return;
    script->SetState(kScriptState_End);
    script->SetRemoveWhenFinished(true);
    script = nullptr;
}

} // namespace

// @func 0x00490B80 Minigame::Begin
void Minigame::Begin()
{
    StateMachine::Begin();
    if (runFlags & kRun_Started)
        return;
    EndScript(script);
    if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(scriptLibrary)) {
        if (ape::ApeAnimScript* source = library->FindScript(scriptName)) {
            script = new ape::ApeAnimScriptInstance(source);
            script->SetRemoveWhenFinished(false);
            ape::ApeAnimScriptInstance::Register(script);
            script->Update(kScriptFirstStep);
        }
    }
    if (Slot14())
        hud::SetAllEnabled(false, false);
    Reset();
    runFlags = uint8_t((runFlags & ~kRun_SavedFlag) | (game->field_12828C ? kRun_SavedFlag : 0));
    game->field_12828C = 0;
    runFlags |= kRun_Started | kRun_HudHidden;
}

// @func 0x00490E40 Minigame::Stop
void Minigame::Stop()
{
    SetState(0, kNoState);
    if (!(runFlags & kRun_Started))
        return;
    if (Slot14())
        hud::SetAllEnabled(true, false);
    Slot21();
    EndScript(script);
    GetGame()->field_12828C = (runFlags & kRun_SavedFlag) ? 1 : 0;
    runFlags &= ~kRun_Started;
}

// @func 0x00490F88 Minigame::SetHudHidden
void Minigame::SetHudHidden(bool hidden)
{
    runFlags = uint8_t((runFlags & ~kRun_HudHidden) | (hidden ? kRun_HudHidden : 0));
    if (Slot14())
        hud::SetAllEnabled(!hidden, false);
}

// @func 0x003A4010 Minigame::ClearRequests
void Minigame::ClearRequests()
{
    resetRequests = 0;
}

// @func 0x003A4000 Minigame::RequestFullReset
void Minigame::RequestFullReset()
{
    resetRequests |= kMinigameRequest_All;
}

// @func 0x003AAAE0 Minigame::RequestScoreReset
void Minigame::RequestScoreReset()
{
    resetRequests |= kMinigameRequest_Score;
}

// @func 0x003AAAF0 Minigame::RequestTimerReset
void Minigame::RequestTimerReset()
{
    resetRequests |= kMinigameRequest_Timer;
}

// @func 0x003AAB08 Minigame::PlayIntro
void Minigame::PlayIntro()
{
    GuiPlayAnimation(-1.0f, introGui, nullptr, true);
}

// @func 0x003AAB54 Minigame::StartPlayer
void Minigame::StartPlayer()
{
    if (player != nullptr)
        EntityActivate(player);
}

} // namespace gameplay
} // namespace sly
