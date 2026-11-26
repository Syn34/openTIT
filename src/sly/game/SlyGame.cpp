// openTIT - Game world object: the parts owned by the game shell.
#include "sly/game/SlyGame.h"

#include "ape/core/ApeSystem.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/world/ApeBase.h"
#include "sly/entity/Entity.h"
#include "sly/gameplay/Minigame.h"
#include "sly/game/hud/HudManager.h"

namespace sly {

SlyGame* g_game = nullptr;

// TODO(0x0045CC18 SlyGame::SlyGame): besides the members (default
// initialisers here), the console constructor creates the render targets
// ("SplitRender%d", "HUD_3D", "PostHUD_3D", "PostHUD_2D"), registers the
// script variables and commands (sly::levels::RegisterLevelScriptCommands),
// the level definition loaders and IFF chunk handlers (GOAF/GOAH/GOAD,
// ENTF/ENTH/ENTD, LVLF/LVLH/LVLD), the stream types, and creates the job,
// effect, GUI and AI managers.
SlyGame::SlyGame(uint8_t initialState)
    : gameplay::StateMachine(4, 0x80, initialState),
      previousMainState(initialState),
      requestedMainState(initialState)
{
    field_128230 = ape::world::kWorldIdentity;
    cutsceneQueue.Init(4, nullptr, true);
}

// TODO(0x00466860 SlyGame::~SlyGame): saves "save.dat" when requested,
// ends the dialog, destroys the managers, the scene and the render targets.
SlyGame::~SlyGame()
{
    for (GamePlayer& player : players) {
        if (player.character) {
            player.character->Release(false);
            player.character = nullptr;
        }
    }
}

// @func 0x0046B5DC SlyGame::GetState
uint8_t SlyGame::GetState(uint32_t layer) const
{
    return layer == 0 ? requestedMainState : m_states[layer];
}

// TODO(0x00491710 SlyGame::OnMessage): the game shell update (front end,
// menus, loading, in-game, cutscenes).
bool SlyGame::OnMessage(const Message& message)
{
    (void)message;
    return true;
}

void SlyGame::RequestMainState(uint8_t state)
{
    previousMainState = m_states[0];
    if (previousMainState == kNoState) {
        requestedMainState = state;
        previousMainState = state;
        gameplay::StateMachine::SetState(0, state);
        return;
    }
    requestedMainState = state;
    requestedSubState = 0;
    transition = kTransition_Start;
    transitionTime = 0.0f;
}

void SlyGame::RequestSubState(uint8_t state)
{
    previousMainState = m_states[0];
    if (InTransition())
        requestedSubState = state;
    else
        gameplay::StateMachine::SetState(1, state);
}

// @func 0x004665C0 SlyGame::ChangeState
bool SlyGame::ChangeState(uint32_t layer, uint8_t state)
{
    previousMainState = m_states[0];
    if (layer == 0 && previousMainState != kNoState && state != kNoState) {
        requestedMainState = state;
        requestedSubState = 0;
        transition = kTransition_Start;
        transitionTime = 0.0f;
        return true;
    }
    if (layer == 1 && InTransition()) {
        requestedSubState = state;
        return true;
    }
    if (layer == 0) {
        requestedMainState = state;
        previousMainState = state;
    }
    return gameplay::StateMachine::SetState(layer, state);
}

// @func 0x0047C530 SlyGame::SetMinigame
void SlyGame::SetMinigame(gameplay::Minigame* next)
{
    if (minigame) {
        minigame->Stop();
        delete minigame;
    }
    minigame = next;
}

// @func 0x0047C490 SlyGame::EndMinigame
void SlyGame::EndMinigame()
{
    if (!minigame)
        return;
    minigame->Stop();
    delete minigame;
    minigame = nullptr;
}

// @func 0x004897AC SlyGame::SetMinigameHudVisible
void SlyGame::SetMinigameHudVisible(bool visible)
{
    if (minigame)
        minigame->SetHudHidden(!visible);
}

// @func 0x0047BC40 SlyGame::SnapPlayerMeter
void SlyGame::SnapPlayerMeter(uint32_t /*player*/)
{
    hud::SnapMeterB();
}

// @func 0x0047BE60 SlyGame::SetPlayerMeterState
void SlyGame::SetPlayerMeterState(uint32_t /*player*/, bool on)
{
    hud::SetMeterBState(on);
}

// @func 0x0047C1F0 SlyGame::SnapMeterOf
void SlyGame::SnapMeterOf(const entity::Entity* owner)
{
    hud::SnapMeterOf(owner);
}

// @func 0x004896F8 SlyGame::RequestLevelRestart
void SlyGame::RequestLevelRestart(bool flag)
{
    stateFlags = uint8_t((stateFlags & ~0x20) | (flag ? 0x20 : 0) | 0x40);
}

// @func 0x0048A4C0 RequestLevelRestart
void RequestLevelRestart()
{
    g_game->RequestLevelRestart(false);
}

// @func 0x0048C82C SetLodYInfluence
void SetLodYInfluence(float influence)
{
    ape::g_engineConfig.lodYInfluence = influence;
}

// @func 0x0048C840 ApplyLevelLodYInfluence
void ApplyLevelLodYInfluence(const SlyGame& game)
{
    ape::g_engineConfig.lodYInfluence = game.levelDef ? game.levelDef->lodYInfluence : 1.0f;
}

// @func 0x0047C46C SlyGame::CurrentLevelName
const char* SlyGame::CurrentLevelName() const
{
    return levelDef ? levelDef->name : "";
}

// @func 0x00458B50 SlyGame::ResetFrameLists
void SlyGame::ResetFrameLists()
{
    for (int i = 0; i < kFrameListCount; ++i)
        callbackLists[kFirstFrameList + i].Clear();
}

// @func 0x0046B5F8 SlyGame::AddCallback
void SlyGame::AddCallback(int list, void* function, void* context, entity::Entity* owner)
{
    WorldCallback entry;
    entry.function = function;
    entry.context = context;
    entry.owner = owner;
    callbackLists[list].PushBack(entry);
}

// @func 0x0046B7A4 SlyGame::RemoveCallback
void SlyGame::RemoveCallback(int list, void* context, entity::Entity* owner)
{
    ape::ApeTArray<WorldCallback>& callbacks = callbackLists[list];
    for (size_t i = 0; i < callbacks.Count();) {
        if (callbacks[i].owner == owner && callbacks[i].context == context)
            callbacks.RemoveAtSwap(i);   // re-test the entry moved into slot i
        else
            ++i;
    }
}

// @func 0x0045900C SlyGame::RunCallbacks
void SlyGame::RunCallbacks(int list, float dt, bool clear)
{
    // NOTE: the console passes the frame time in a float register next to
    // the context; the port's callbacks take (unused, context) only.
    (void)dt;
    using Fn = void (*)(void* unused, void* context);
    ape::ApeTArray<WorldCallback>& callbacks = callbackLists[list];
    for (size_t i = 0; i < callbacks.Count(); ++i) {
        const WorldCallback& entry = callbacks[i];
        reinterpret_cast<Fn>(entry.function)(nullptr, entry.context);
    }
    if (clear)
        callbacks.Clear();
}

// @func 0x0046B6CC SlyGame::ForgetEntity
void SlyGame::ForgetEntity(entity::Entity* owner)
{
    for (ape::ApeTArray<WorldCallback>& callbacks : callbackLists) {
        for (size_t i = 0; i < callbacks.Count();) {
            if (callbacks[i].owner == owner)
                callbacks.RemoveAtSwap(i);
            else
                ++i;
        }
    }
}

} // namespace sly
