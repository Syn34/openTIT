// openTIT - Game world: level requests, pause menu gate, yes/no prompts and
// the small player block helpers of the game shell (0x004674C4-0x0048EE44).
#include "sly/game/SlyGame.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/world/ApeObj.h"
#include "sly/components/Components.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameCamera.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/GameShellHooks.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/hud/HudElement.h"
#include "sly/game/hud/HudHideoutMenu.h"
#include "sly/game/hud/HudManager.h"
#include "sly/gameplay/Minigame.h"
#include "sly/levels/ai/AIController.h"

namespace sly {

namespace {

// Main states of the game (layer 0). NOTE: only the ones the game shell
// requests by number are named.
constexpr uint8_t kMain_Loading = 1;
// Sub states (layer 1).
constexpr uint8_t kSub_Play = 2;
constexpr uint8_t kSub_HideoutMenu = 6;
constexpr uint8_t kSub_FrontEndMenu = 7;
constexpr uint8_t kSub_Modal = 0x0C;
constexpr uint8_t kSub_FrontEndPrompt = 0x1C;

// Pause menu pages (OpenMenu).
constexpr int kMenu_Prompt = 3;
constexpr int kMenu_Pause = 4;
constexpr int kMenu_Refresh = 0x17;

// The pause menu's start page (HudHideoutMenu::SelectPage). NOTE: page name
// unknown.
constexpr uint32_t kPauseStartPage = 0x0023FDB9u;

// HUD elements. NOTE: their names are unknown (console globals 0x00F64D4C
// and 0x00F64D04).
constexpr uint32_t kPromptElement = 0xA6910223u;
constexpr uint32_t kMenuBlockerElement = 0x0B253467u;

// Pad flag of the pause button (AppPlayer::inputFlags & inputMask).
// NOTE: the button is inferred from its use.
constexpr uint32_t kPadPause = 0x4000;

// Component type of the AI controllers that show the contextual marker.
constexpr uint32_t kComp_ContextualMarker = 0x1B;
// Component type reset when the AI entities are shown or hidden.
constexpr uint32_t kComp_24 = 0x24;

// Sub state data of the travel dialog (layer 1, 0x80 bytes).
struct TravelDialogData {
    uint8_t field_00[0x48];
    uint32_t level;               // +0x48 level to go to
    uint32_t respawnPoint;        // +0x4C one-shot respawn point there
    uint8_t field_50[8];
    bool keepCharacterName;       // +0x58
    entity::Entity* target;       // +0x5C entity the dialog was opened for
};
static_assert(sizeof(TravelDialogData) <= 0x80, "fits the state data block");

bool ElementObjectVisible(const hud::HudObject* element)
{
    return element && element->object && hud::ObjectVisible(element->object);
}

// Pad index of a player entity: the top byte of the flags of an entity
// with class bit 0 (player controlled).
uint8_t PlayerSlotOf(const entity::Entity* player)
{
    return (player && (player->classBits & 1)) ? uint8_t(player->flags >> 24) : 0;
}

void FillPrompt(hud::HudObject* prompt, entity::Entity* player, const char* text, const char* buttons,
                SlyGame::PromptCallback onYes, void* yesUser, SlyGame::PromptCallback onNo, void* noUser,
                bool optionA, bool optionB)
{
    auto* element = static_cast<hud::HudArElement*>(prompt);
    element->SetFieldText(text, 0);
    element->SetFieldText(buttons, 1);
    element->onYes = onYes;
    element->onNo = onNo;
    element->yesUser = yesUser;
    element->noUser = noUser;
    element->player = player;
    element->field_1A4 = optionA;
    element->field_1A5 = optionB;
}

// The prompt element can take a new prompt: it exists, is not busy with
// its intro or update (states 2/3) and its object is not shown.
hud::HudObject* FreePromptElement()
{
    hud::HudObject* prompt = hud::FindType20000(kPromptElement);
    if (!prompt)
        return nullptr;
    const uint32_t state = prompt->animState >> 1;
    if (state == 2 || state == 3 || ElementObjectVisible(prompt))
        return nullptr;
    return prompt;
}

} // namespace

// @func 0x004674C4 SlyGame::RequestLevel
void SlyGame::RequestLevel(uint32_t level, bool flag)
{
    if (level == 0)
        return;
    if (camera)
        camera->SetVisibilityDeferred(false, false);
    pendingLevel = level;
    ChangeState(0, kMain_Loading);
    ChangeState(1, 0);
    ChangeState(2, 0);
    stateFlags = uint8_t((stateFlags & ~0x02) | (flag ? 0x02 : 0));
    flags2D |= 0x20;
    flags2E |= 0xC0;
}

// TODO(0x00470750 SlyGame::OpenMenu): opens a pause menu page (desaturation,
// HUD suspended, message 0x2EB3CC8B to the pause listeners, minigame told,
// letterbox hidden, the hideout menu opened on its start page, pause sound,
// the active pad held) and the per-page set-ups.
void SlyGame::OpenMenu(int page, uint8_t arg)
{
    menu = uint8_t(page);
    field_20 = 0;
    menuArg = arg;
}

// @func 0x00472208 SlyGame::OpenMenu(page)
void SlyGame::OpenMenu(int page)
{
    OpenMenu(page, 0);
}

// @func 0x0048CDF8 SlyGame::RefreshOpenMenu
void SlyGame::RefreshOpenMenu()
{
    if (menu != 0)
        OpenMenu(kMenu_Refresh, 0);
}

// @func 0x0048CEA0 SlyGame::ScaledVolume3
int32_t SlyGame::ScaledVolume3() const
{
    return int32_t(settings.volumes[3] * audio::g_volumeSettings.option44 * 32768.0f);
}

// @func 0x0048981C SlyGame::CanOpenMenu
bool SlyGame::CanOpenMenu(bool forPrompt) const
{
    if (timeSource)
        return false;
    if (GetState(0) == 1 && GetState(1) == kSub_Modal)
        return false;
    if (hud::AnyUpdatingElementVisible() || cutscenePlaying)
        return false;
    if (game::JobManager* jobs = game::GetJobManager()) {
        if (jobs->failingGoal != nullptr || (jobs->streamFlags & 0x20000000u))
            return false;
    }
    if (ElementObjectVisible(hud::FindType40(kMenuBlockerElement)))
        return false;
    if (!forPrompt) {
        if (screenFade.active)
            return false;
        // A screen fade still showing a colour blocks the menu.
        if (g_fadeObject && g_fadeObject->IsVisible() && g_fadeObject->GetColor().a != 0)
            return false;
    }
    if (!GadgetsAllowPause())
        return false;
    return minigame ? minigame->AllowsPause(forPrompt) : true;
}

// @func 0x0048EE44 SlyGame::CheckPauseRequest
bool SlyGame::CheckPauseRequest()
{
    if (!CanOpenMenu(false) || (activePlayer != 0 && activePlayer != 0xFF))
        return true;
    const AppPlayer& pad = GetAppRuntime().players[players[0].padIndex % kLocalPlayerCount];
    if (!(pad.inputFlags & pad.inputMask & kPadPause))
        return true;
    hud::HudObject* pauseMenu = hud::FindActiveType200();
    if (!pauseMenu || ElementObjectVisible(pauseMenu) || menu != 0)
        return true;
    previousActivePlayer = activePlayer;
    activePlayer = 0;
    static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200())->SelectPage(kPauseStartPage);
    OpenMenu(kMenu_Pause, 0);
    return true;
}

// @func 0x00489A60 SlyGame::ShowPrompt
bool SlyGame::ShowPrompt(entity::Entity* player, const char* text, const char* buttons, PromptCallback onYes,
                         void* yesUser, PromptCallback onNo, void* noUser, bool optionA, bool optionB)
{
    if (!g_game->CanOpenMenu(true))
        return false;
    hud::HudObject* prompt = FreePromptElement();
    if (!prompt || menu != 0)
        return false;
    previousActivePlayer = activePlayer;
    activePlayer = PlayerSlotOf(player);
    OpenMenu(kMenu_Prompt, 0);
    FillPrompt(prompt, player, text, buttons, onYes, yesUser, onNo, noUser, optionA, optionB);
    return true;
}

// @func 0x00489C08 SlyGame::ShowFrontEndPrompt
bool SlyGame::ShowFrontEndPrompt(entity::Entity* player, const char* text, const char* buttons,
                                 PromptCallback onYes, void* yesUser, PromptCallback onNo, void* noUser,
                                 bool optionA, bool optionB)
{
    hud::HudObject* prompt = FreePromptElement();
    if (!prompt)
        return false;
    previousActivePlayer = activePlayer;
    activePlayer = PlayerSlotOf(player);
    ChangeState(1, kSub_FrontEndMenu);
    ChangeState(1, kSub_FrontEndPrompt);
    FillPrompt(prompt, player, text, buttons, onYes, yesUser, onNo, noUser, optionA, optionB);
    return true;
}

// @func 0x0048A0E4 SlyGame::SetActivePlayer
void SlyGame::SetActivePlayer(uint8_t player)
{
    previousActivePlayer = activePlayer;
    activePlayer = player;
    static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200())->SelectPage(kPauseStartPage);
    OpenMenu(kMenu_Pause, 0);
}

// @func 0x0048970C SlyGame::ContinueAfterPrompt
void SlyGame::ContinueAfterPrompt()
{
    ChangeState(1, kSub_HideoutMenu);
    if (uint8_t* data = StateData(1))
        data[1] = 1;
    if (auto* hideout = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200()))
        hideout->SetGui(nullptr);
}

// @func 0x0048A26C SlyGame::DeclineTravel
void SlyGame::DeclineTravel()
{
    auto* data = reinterpret_cast<TravelDialogData*>(g_game->StateData(1));
    if (data && data->target)
        TravelTargetDeclined(data->target);
    g_game->ChangeState(1, kSub_Play);
}

// @func 0x0048A144 SlyGame::AcceptTravel
void SlyGame::AcceptTravel()
{
    SlyGame* game = g_game;
    auto* data = reinterpret_cast<TravelDialogData*>(game->StateData(1));
    if (!data)
        return;
    uint32_t level = data->level;
    if (level != 0) {
        game::JobManager* jobs = game::GetJobManager();
        game::Job* episode = jobs ? jobs->CurrentEpisode() : nullptr;
        const uint32_t hideout = episode ? episode->nameHash : 0;
        if (jobs && hideout == level && jobs->instance) {
            jobs->FinishJobInstance();
            level = data->level;
        }
    }
    game->RequestLevel(level, false);
    game->field_128160 = data->respawnPoint;
    if (data->keepCharacterName && game->pendingLevelName[0] == '\0') {
        entity::Entity* character = game->pausedEntities[0];
        if (character && character->def)
            ape::StrCopyBounded(game->pendingLevelName, character->def->name, sizeof game->pendingLevelName);
    }
}

// @func 0x0046B874 SlyGame::ReleaseBorrowedEntities
void SlyGame::ReleaseBorrowedEntities()
{
    for (uint32_t i = 0; i < borrowedCount; ++i) {
        for (entity::Entity*& entity : borrowed[i].entities) {
            if (!entity)
                break;
            entity->SetVisible(false, true, true, true);
            entity->Release(false);
            if (entity->flags & (1u << 18))
                entity->Release(true);
            entity = nullptr;
        }
    }
    borrowedCount = 0;
}

// @func 0x0047C168 SlyGame::ShowPlayerHudScreen
void SlyGame::ShowPlayerHudScreen(uint32_t player, const char* name, bool refreshMarker)
{
    if (!name || name[0] == '\0')
        return;
    // The HUD keeps player 0's screen only.
    hud::RequestScreen((player & 0xFF) != 0, name);
    if (refreshMarker)
        RefreshContextualMarker(player);
}

// @func 0x0047BFD4 SlyGame::RefreshContextualMarker
void SlyGame::RefreshContextualMarker(uint32_t player)
{
    if (player != 0)
        return;
    uint32_t characterName = 0;
    if (entity::Entity* character = players[0].character) {
        characterName = character->def->DefSlot5();
        if (characterName == 0)
            characterName = character->def->DefSlot4();
    }
    ape::world::ApeObj* marker = nullptr;
    if (const uint32_t markerName = ContextualMarkerFor(characterName)) {
        marker = ape::world::FindInstance(markerName, 0);
        if (marker)
            marker->SetVisible(false, true);
    } else {
        (void)ape::TempHexString(characterName);   // compiled-out warning
    }
    for (const auto& [name, component] : components::LiveComponents()) {
        (void)name;
        if (component && component->type == kComp_ContextualMarker)
            AiSetContextualMarker(component, marker);
    }
}

// @func 0x0047C218 SlyGame::ResetPlayerHud
void SlyGame::ResetPlayerHud()
{
    hud::ResetHudElements(false);
    entity::Entity* character = players[0].character;
    if (character && character->HasTypeBit(3)) {
        if (entity::EntityComponent* gadget = entity::EntityGadget(character))
            GadgetPutAway(gadget, false, true, false);
    }
}

// @func 0x0047BCF4 SlyGame::SetPlayerMeterVariant
void SlyGame::SetPlayerMeterVariant(uint32_t player, bool on)
{
    entity::Entity* character = g_game->players[player].character;
    if (character && character->HasTypeBit(3)) {
        entity::EntityComponent* gadget = entity::EntityGadget(character);
        const bool forced = AiDebugMeterSwitch();
        const bool busy = CharacterState(character, 0) != 0 || CharacterState(character, 1) != 6;
        on = (on || forced) && busy;
        if (gadget)
            GadgetSetMeterVariant(gadget, on);
    }
    // NOTE: the HUD keeps a single meter B; the console passes the player.
    hud::SetMeterBVariant(on);
}

// @func 0x00484530 SlyGame::RefreshCamera
void SlyGame::RefreshCamera()
{
    if (!camera)
        return;
    camera->PostUpdate(0.0f);
    camera->RefreshView();
    if (field_128584)
        FrameObjectRefresh(field_128584);
}

// @func 0x00488078 SlyGame::ThrowPlayer
void SlyGame::ThrowPlayer(uint32_t player, const ape::Vec3& target, bool drop)
{
    entity::Entity* character = players[player].character;
    if (!character || !character->HasTypeBit(3))
        return;
    if (drop)
        CharacterDropTowards(character, target);
    else
        CharacterThrowTowards(character, target);
}

// TODO(0x00484BE0 SlyGame::SwitchPlayerCharacter): releases the block's
// character, spawns one of definition `def` at the transform (or the
// default spawn of its slot), moves the camera, HUD and gadgets over.
void SlyGame::SwitchPlayerCharacter(GamePlayer& player, uint32_t def, const ape::Matrix34* transform, bool a,
                                    bool b, bool c, bool d)
{
    (void)player; (void)def; (void)transform; (void)a; (void)b; (void)c; (void)d;
}

// @func 0x00488118 SlyGame::SwitchPlayerCharacter(index)
void SlyGame::SwitchPlayerCharacter(uint32_t player, uint32_t def, const ape::Matrix34* transform, bool a,
                                    bool b, bool c, bool d)
{
    SwitchPlayerCharacter(players[player], def, transform, a, b, c, d);
}

// TODO(0x00484858 SlyGame::AttachCameraTo): the game camera's follow mode
// targets the block's character and restarts its blend.
void SlyGame::AttachCameraTo(GamePlayer& player)
{
    (void)player;
}

// @func 0x00488A78 SlyGame::AttachCameraToPlayer
void SlyGame::AttachCameraToPlayer(uint32_t player)
{
    if (camera)
        AttachCameraTo(players[player]);
}

// @func 0x0048A068 SetAiEntitiesVisible
// Shows or hides the AI entities; player 0's component of type 0x24 goes
// back to its start first.
void SetAiEntitiesVisible(bool visible)
{
    if (entity::Entity* character = g_game->players[0].character) {
        if (entity::EntityComponent* component = character->GetComponent(kComp_24))
            Component24Restart(component, true);
    }
    if (levels::ai::AIManager* manager = levels::ai::GetAIManager())
        manager->SetEntitiesVisible(visible);
}

} // namespace sly

namespace sly {

// @func 0x004902B8 ShowCharacterHint
// Plays the intro of HUD element 0xB7E98FE4 (console global 0x00F64D08;
// NOTE: the element's name is unknown, the characters show it).
void ShowCharacterHint()
{
    hud::ShowByName(0xB7E98FE4u, true, false, false);
}

} // namespace sly

namespace sly {

// NOTE: a word the menus of pages 0x0E/0x0F set (0x010BBF58); its writer is
// not ported, so it stays 0.
uint32_t g_menuBackdropReady = 0;

// @func 0x00457654 SlyGame::HoldsFrame
// Whether the screen shows a held picture instead of the running game: the
// modal sub state (0x0C), the play states 2/3, the front end sub states 6,
// 0x18 and 0x19 (with a deeper state) hold it while their state data's
// first word is set; menu pages 0x0E/0x0F while their backdrop is ready.
// NOTE: name and reading from use.
bool SlyGame::HoldsFrame()
{
    auto firstWord = [this](uint32_t layer) {
        uint32_t word = 0;
        if (const uint8_t* data = StateData(layer))
            std::memcpy(&word, data, sizeof word);
        return word;
    };
    if (GetState(1) == 0x0C)
        return firstWord(1) != 0;
    if (GetState(0) == 2 || GetState(0) == 3)
        return firstWord(0) != 0;
    if (GetState(0) == 0 && GetState(1) == 6)
        return firstWord(1) != 0;
    if (GetState(0) == 0 && (GetState(1) == 0x19 || GetState(1) == 0x18) && GetState(2) > 0)
        return firstWord(1) != 0;
    if (menu != 0x0F && menu != 0x0E)
        return false;
    return field_20 != 0 && g_menuBackdropReady != 0;
}

} // namespace sly
