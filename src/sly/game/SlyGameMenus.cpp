// openTIT - Game world: level requests, pause menu gate, yes/no prompts and
// the small player block helpers of the game shell (0x004674C4-0x0048EE44).
#include "sly/game/SlyGame.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiScript.h"
#include "ape/world/ApeObj.h"
#include "sly/components/Components.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameCamera.h"
#include "sly/camera/CameraModes.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/GameShellHooks.h"
#include "sly/game/LevelParams.h"
#include "sly/game/Subtitles.h"
#include "sly/entity/Character.h"
#include "sly/gameplay/Sequencer.h"
#include "agl/AglRenderTarget.h"
#include "ape/anim/ApeAnimScript.h"
#include "sly/gameplay/ReturnTreasure.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/SoundZones.h"
#include "sly/entity/Awards.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/goals/Goal.h"
#include "sly/game/FileStreaming.h"
#include "sly/save/SaveGame.h"
#include "sly/game/GameMessages.h"
#include "ape/level/ApeLevelScene.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include "sly/game/AudioSystem.h"
#include "sly/game/hud/HudElement.h"
#include "sly/game/hud/HudHideoutMenu.h"
#include "sly/game/hud/HudManager.h"
#include "sly/gameplay/Minigame.h"
#include "sly/tuning/GameTuning.h"
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
// Entity type bit of the vehicles (type word +0xC8 bit 10).
constexpr uint32_t kType_Vehicle = 74;
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
    if (playerCameraMode)
        playerCameraMode->Activate();
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

// @func 0x00484858 SlyGame::AttachCameraTo
void SlyGame::AttachCameraTo(GamePlayer& player)
{
    if (!camera)
        return;
    ClearCameraModes();
    playerCameraMode = nullptr;
    entity::Entity* character = player.character;
    if (!character)
        return;
    // NOTE: the console constructors of the default, vehicle and Havok
    // vehicle modes (0x000FF0D8, 0x0010DE30, 0x00111410) take the target;
    // the port's modes get it through SetTarget.
    camera::CameraMode* mode = nullptr;
    if (character->HasTypeBit(kType_Vehicle)) {
        switch (VehicleDefCameraKind(character->def)) {
        case 0: {
            auto* vehicle = new camera::VehicleCameraMode(camera);
            vehicle->SetTarget(character);
            mode = vehicle;
            break;
        }
        case 1: {
            auto* follow = new camera::DefaultCameraMode(camera);
            follow->SetTarget(character);
            mode = follow;
            break;
        }
        case 2: {
            auto* havok = new camera::HavokVehicleCameraMode(camera);
            havok->SetTarget(character);
            mode = havok;
            break;
        }
        case 3:
            mode = new camera::OrthoCameraMode(VehicleDefCameraFov(character->def), camera, character);
            break;
        default:
            return;
        }
    } else {
        if (!character->HasTypeBit(3))
            return;
        if (g_cameraDefaultOrbit.Get()) {
            mode = new camera::OrbitCameraMode(camera, character, camera::kOrbit_Standard);
        } else {
            auto* follow = new camera::DefaultCameraMode(camera);
            follow->SetTarget(character);
            mode = follow;
        }
    }
    playerCameraMode = mode;
    camera->PushMode(-1.0f, mode, false);
    RefreshCamera();
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

namespace sly {

namespace {

// A GUI popup over the game (system notices): the object shown and what it
// replaced while it is up (0x010BB9E4..0x010BB9F5; the object is the app's
// word +0x04 on the console).
struct GuiPopup {
    ape::GuiPadFilter savedPadFilter = nullptr;   // 0x010BB9E4
    void* savedPadUser = nullptr;                 // 0x010BB9E8
    ape::GuiFilter savedFilter = nullptr;         // 0x010BB9EC
    void* savedFilterUser = nullptr;              // 0x010BB9F0
    bool fadeWasVisible = false;                  // 0x010BB9F4
    bool loadingWasVisible = false;               // 0x010BB9F5
    ape::ApeGuiObject* object = nullptr;          // app +0x04
};
GuiPopup g_guiPopup;

// GUI event sent to the popup when it closes. NOTE: name unknown.
constexpr uint32_t kGuiEvent_PopupClosed = 0x0E;
// GUI event of the accept button.
constexpr uint32_t kGuiEvent_Accept = 5;

void SetGuiObjectVisible(const char* path, bool visible)
{
    if (ape::ApeGuiObject* object = ape::FindGuiObject(path, nullptr))
        object->SetVisible(visible);
}

} // namespace

// @func 0x004645AC CloseGuiPopup
// Takes the popup down: the front end and in-game screens come back, the
// fade and loading screens get their earlier visibility, the audio pause
// follows the pause menu again and the GUI handlers are restored.
void CloseGuiPopup()
{
    GuiPopup& popup = g_guiPopup;
    SetGuiObjectVisible("FrontEnd", true);
    SetGuiObjectVisible("InGame", true);
    SetGuiObjectVisible("Common.Fade", popup.fadeWasVisible);
    SetGuiObjectVisible("Common.Loading", popup.loadingWasVisible);
    if (popup.object)
        popup.object->SetVisible(false);
    audio::SetPauseAll(g_game && g_game->menu != 0);
    // NOTE: the console also clears the word 0x010BA024 here (its readers
    // are not ported).
    ape::SendGuiEvent(kGuiEvent_PopupClosed, 0xFFFFFFFFu, popup.object, 0);
    ape::SetGuiFilter(popup.savedFilter, popup.savedFilterUser);
    ape::SetGuiPadFilter(popup.savedPadFilter, popup.savedPadUser);
    popup.object = nullptr;
}

// @func 0x00464804 GuiPopupEventFilter
// GUI handler while a popup is up: accepting on any object closes it.
bool GuiPopupEventFilter(const ape::GuiEvent* event)
{
    if (g_guiPopup.object && event->type == kGuiEvent_Accept && event->object)
        CloseGuiPopup();
    return true;
}

} // namespace sly

namespace sly {

// @func 0x00466678 SlyGame::EndDialog
void SlyGame::EndDialog(bool toFrontEnd)
{
    constexpr uint32_t kDialogBusy = 0x40000000u;
    if (!(dialog.flags & kDialogBusy))
        return;
    dialog.flags &= ~kDialogBusy;
    ChangeState(1, toFrontEnd ? 0x0B : 2);
    ChangeState(2, 0);
    if (dialog.mixState)
        audio::RemoveMixState(dialog.mixState, g_audioFreezeScreenCrossfadeDuration.Get());
    dialog.text[0] = '\0';
    dialog.field_98 = 0;
    dialog.sound[0] = '\0';
    dialog.field_8C = 0;
    dialog.screenHash = 0;
    dialog.field_A0 = 0;
    dialog.duration = 0.0f;
    dialog.field_A8 = 0;
    dialog.countRate = 0.0f;
    dialog.field_B0 = 0;
    dialog.flags &= 0x1FFFFFFFu;
    dialog.mixState = nullptr;
}

} // namespace sly

namespace sly {

// @func 0x00482B20 SlyGame::ShowLevelHeadline
// The running level's headline (the running job's override when there is
// one), once per profile and level unless forced; the profile's unlock
// entry of the level records that it was shown.
void SlyGame::ShowLevelHeadline(bool force)
{
    constexpr uint32_t kDefaultHeadlineElement = 0x633E5DF3u;   // 0x00F64CEC. NOTE: name unknown
    if (pendingLevelHash == 0 || !levelParams || localPlayerCount == 0)
        return;
    game::PlayerProgress::UnlockEntry* entry = nullptr;
    for (game::PlayerProgress::UnlockEntry& unlock : progress.unlocks) {
        if (unlock.name == 0)
            break;
        if (unlock.name == pendingLevelHash)
            entry = &unlock;
    }
    if (!entry)
        return;
    // The first byte of the entry's state word: headline shown.
    uint8_t shown = uint8_t(entry->state >> 24);
    if (shown != 0 && !g_gameAlwaysShowLevelHeadline.Get() && !force)
        return;
    shown = 1;
    entry->state = (entry->state & 0x00FFFFFFu) | (uint32_t(shown) << 24);

    const char* text = levelParams->headline[0] != '\0'
                           ? levelParams->headline
                           : "<unknown headline string! Check level params for level!>";
    game::JobManager* jobs = game::GetJobManager();
    if (game::Episode* job = jobs ? jobs->instance : nullptr) {
        for (const LevelParams::JobHeadline& headline : levelParams->jobHeadlines) {
            if (headline.jobName == job->nameHash) {
                text = headline.text;
                break;
            }
        }
    }
    const uint32_t element = levelParams->hash1584 ? levelParams->hash1584 : kDefaultHeadlineElement;
    static const uint8_t kNoTint[4] = {0, 0, 0, 0};   // 0x01164CA4
    hud::ShowTextByName(-1.0f, element, text, true, true, nullptr, kNoTint);
}

} // namespace sly

namespace sly {

namespace {

// The levels that count for the "visit everything" trophy (0x00AE91E8).
// NOTE: level names unknown; the hashes are their name hashes.
constexpr uint32_t kTrophyLevels[] = {
    0xA70C9716u, 0xE3DEF6B5u, 0x467B8C48u, 0x2E01E5EBu, 0x95841725u, 0x2291A3C2u, 0x59CC1B15u,
    0xBF4B1637u, 0x350D543Bu, 0x63910843u, 0x716016C8u, 0x85EA7462u, 0xDE07516Fu, 0x2986D493u,
    0xF285665Bu, 0xC25ED889u, 0xF4596EECu, 0x4A2AD4EAu, 0xFCA16EEFu, 0xAEAE3F00u, 0xBC676503u,
    0xACAA7E76u, 0x1D17219Bu, 0xD44B7705u, 0x29E1A78Cu, 0xA3907718u, 0xE3541252u, 0x3FEEDCAFu,
    0xFAC4F502u, 0xC7B84D81u, 0x4192876Du, 0x1AD96E63u, 0x40556837u, 0xA0355EFBu, 0x0C44F02Du,
};
constexpr uint32_t kTrophyLevelCount = sizeof(kTrophyLevels) / sizeof(kTrophyLevels[0]);
// Award of the trophy. NOTE: award name unknown.
constexpr uint32_t kAward_AllLevels = 0x0023FE97u;

} // namespace

// @func 0x0048BF28 SlyGame::RecordLevelVisit
// Marks the running level as visited in the profile (one bit per level of
// the trophy list, progress +0x138); visiting the last one awards the
// trophy.
void SlyGame::RecordLevelVisit()
{
    const uint64_t before = progress.field_138;
    for (uint32_t i = 0; i < kTrophyLevelCount; ++i) {
        if (kTrophyLevels[i] == pendingLevelHash) {
            progress.field_138 |= uint64_t(1) << i;
            break;
        }
    }
    const uint64_t visited = progress.field_138;
    if (visited == before || visited == 0)
        return;
    uint32_t count = 0;
    for (uint64_t bits = visited; bits; bits &= bits - 1)
        ++count;
    if (count >= kTrophyLevelCount)
        AwardTrophyByName(kAward_AllLevels);
}

} // namespace sly

namespace sly {

namespace {

// Sub state data of the modal box (layer 1).
// NOTE: the console keeps the page pointers as 32-bit words from +0x04; the
// rest of the block keeps its offsets here.
struct ModalBoxData {
    uint8_t field_00[4];
    const char* pages[4];         // +0x04 (console: four words)
    int32_t pageCount;            // +0x14 (in the port after the pointers)
    int32_t page;                 // +0x18 the page shown
    uint8_t field_1C[8];
    int32_t position[2];          // +0x24
    int32_t size[2];              // +0x2C
    uint8_t colorA[4];            // +0x34
    uint8_t colorB[4];            // +0x38
    // +0x3C: bit 29 opened from sub state 0x0B, bit 27/26 options, bit 25
    // the headline element was showing (it was refreshed away).
    uint32_t flags;
};
static_assert(sizeof(ModalBoxData) <= 0x80, "fits the state data block");

constexpr uint8_t kSub_SpeakerDialog = 8;
constexpr uint8_t kSub_FrontEndPause = 0x0B;
constexpr uint32_t kHeadlineElement = 0x633E5DF3u;   // 0x00F64CEC, see ShowLevelHeadline

// Common start of both boxes: sub state 0x0C, the audio paused.
ModalBoxData* OpenModalBox(SlyGame& game, bool& fromFrontEnd)
{
    fromFrontEnd = game.GetState(1) == kSub_FrontEndPause;
    game.ChangeState(1, kSub_Modal);
    game.ChangeState(2, 0);
    audio::SetPauseAll(true);
    return reinterpret_cast<ModalBoxData*>(game.StateData(1));
}

} // namespace

// @func 0x00488DF4 SlyGame::ShowMessageBox
void SlyGame::ShowMessageBox(const char* text, const int32_t position[2], const int32_t size[2])
{
    if (!g_gameMoviesEnabled.Get())
        return;
    bool fromFrontEnd = false;
    ModalBoxData* data = OpenModalBox(*this, fromFrontEnd);
    if (!data)
        return;
    data->pages[0] = text;
    data->pageCount = 1;
    data->page = 0;
    data->position[0] = position[0];
    data->position[1] = position[1];
    data->size[0] = size[0];
    data->size[1] = size[1];
    const uint8_t black[4] = {0, 0, 0, 0xFF};
    const uint8_t clear[4] = {0, 0, 0, 0};
    std::memcpy(data->colorA, black, 4);
    std::memcpy(data->colorB, clear, 4);
    data->flags = (data->flags & 0xCFFFFFFFu) | (fromFrontEnd ? 0x20000000u : 0u) | 0x0C000000u;
    const uint32_t headlineName = (levelParams && levelParams->hash1584) ? levelParams->hash1584 : kHeadlineElement;
    if (hud::HudObject* headline = hud::Find(headlineName)) {
        if (ElementObjectVisible(headline))
            data->flags |= 0x02000000u;
        headline->Refresh();
    }
    // NOTE: the console fades between two colour words of its stack that are
    // not set up here; the fade keeps the object's colours.
    const ape::xml::Color8 none{0, 0, 0, 0};
    screenFade.Start(g_gameFadeTime.Get(), none, none, false, false);
}

// @func 0x004890A0 SlyGame::ShowTextPages
void SlyGame::ShowTextPages(const char (*pages)[0x80], int count, const int32_t position[2], const int32_t size[2],
                            bool fade, bool optionA, const uint8_t colorA[4], const uint8_t colorB[4], bool optionB)
{
    if (!g_gameMoviesEnabled.Get())
        return;
    bool fromFrontEnd = false;
    ModalBoxData* data = OpenModalBox(*this, fromFrontEnd);
    if (!data)
        return;
    const int kept = count < 4 ? count : 4;   // NOTE: the console does not check the count
    for (int i = 0; i < kept; ++i)
        data->pages[i] = pages[i];
    data->pageCount = count;
    data->page = 0;
    data->position[0] = position[0];
    data->position[1] = position[1];
    data->size[0] = size[0];
    data->size[1] = size[1];
    std::memcpy(data->colorA, colorA, 4);
    std::memcpy(data->colorB, colorB, 4);
    data->flags = (data->flags & 0xC3FFFFFFu) | (fromFrontEnd ? 0x20000000u : 0u) | (optionA ? 0x08000000u : 0u) |
                  (optionB ? 0x04000000u : 0u);
    if (fade) {
        const ape::xml::Color8 none{0, 0, 0, 0};   // NOTE: see ShowMessageBox
        screenFade.Start(g_gameFadeTime.Get(), none, none, false, false);
    }
}

// @func 0x00489328 SlyGame::StartSpeakerDialog
bool SlyGame::StartSpeakerDialog(const DialogSpeakerEntry* speakers, uint32_t count, const char* text,
                                 const char* sound, uint32_t value, float duration, bool option)
{
    constexpr uint32_t kBusy = 0x40000000u;
    if (dialog.flags & kBusy)
        return false;
    dialog.speakers.Clear();
    for (uint32_t i = 0; i < count && i < 6; ++i)
        dialog.speakers.PushBack(speakers[i]);
    dialog.field_98 = value;
    if (text && text[0] != '\0')
        ape::StrCopyBounded(dialog.text, text, 0x40);
    if (sound && sound[0] != '\0')
        ape::StrCopyBounded(dialog.sound, sound, 0x40);
    dialog.duration = duration;
    dialog.field_A8 = 0;
    dialog.countRate = 0.0f;
    dialog.field_B0 = 0;
    if (duration > 0.0f) {
        const float countDuration = g_gameMonetaryCollectFreezeScreenCountDuration.Get();
        dialog.countRate = countDuration > 0.0f ? duration / countDuration : 0.0f;
    }
    dialog.flags = (option ? 0x80000000u : 0u) | (dialog.flags & 0x1FFFFFFFu) | kBusy;
    const bool fromFrontEnd = GetState(1) == kSub_FrontEndPause;
    ChangeState(1, kSub_SpeakerDialog);
    ChangeState(2, 0);
    if (uint8_t* data = StateData(1))
        data[0] = fromFrontEnd ? 1 : 0;
    dialog.mixState = audio::FindMixState(g_audioFreezeScreenMixState.Get());
    if (dialog.mixState)
        audio::PushMixState(dialog.mixState, g_audioFreezeScreenCrossfadeDuration.Get());
    return true;
}

} // namespace sly

namespace sly {

// Letterbox visibility saved when a menu opens (0x00F65D8C).
bool g_letterboxWasVisible = false;

namespace {
// Class bit of an entity under cutscene control (classBits bit 8).
constexpr uint64_t kClass_InCutscene = 0x100;
// Component types touched by the cutscene mode.
constexpr uint32_t kComp_AnimDriver = 0x17;
constexpr int kAnimDriver_PlayingScript = 7;
} // namespace

// @func 0x0048B330 SlyGame::SetCutsceneMode
void SlyGame::SetCutsceneMode(bool on)
{
    entity::Entity* player = players[0].character;
    if (on) {
        timeScaleA = 1.0f;
        timeScaleATarget = -1.0f;
        field_128590 = 0;
        timeScaleB = 1.0f;
        timeScaleBTarget = -1.0f;
        field_12859C = 0;
        if (player && player->HasTypeBit(3)) {
            if (entity::EntityComponent* gadget = entity::EntityGadget(player))
                GadgetPutAway(gadget, false, true, false);
        }
    }
    if (player) {
        player->classBits = on ? (player->classBits | kClass_InCutscene) : (player->classBits & ~kClass_InCutscene);
        if (entity::EntityComponent* targeting = entity::EntityTargeting(player)) {
            targeting->SetOption(!on);
            if (on)
                game::TargetingClearTargets(targeting);
            else
                game::TargetingSetEnabled(targeting, false);
        }
        if (entity::EntityComponent* component = player->GetComponent(kComp_24))
            Component24Restart(component, false);
        if (on) {
            for (entity::EntityComponent* c = player->GetComponent(kComp_AnimDriver); c;
                 c = player->GetComponent(kComp_AnimDriver, c)) {
                if (entity::AnimDriverState(c) == kAnimDriver_PlayingScript)
                    game::AnimDriverStopScript(c);
            }
        }
    }
    // NOTE: leaving the cutscene calls an empty AI manager method.
    if (on) {
        if (levels::ai::AIManager* manager = levels::ai::GetAIManager())
            manager->ClearSightings();
    }
    const uint32_t count = entity::EntityCount(entity::kList_All);
    for (uint32_t i = 0; i < count; ++i) {
        entity::Entity* e = entity::EntityAt(entity::kList_All, i);
        if (e && e->HasTypeBit(3))
            e->SetVisibilityDeferred(!on, false);
    }
    ClearSubtitle();
    if (camera) {
        camera->cameraFlags = on ? (camera->cameraFlags & ~entity::kCamFlag_PostFxDof)
                                 : (camera->cameraFlags | entity::kCamFlag_PostFxDof);
    }
}

// @func 0x0048B674 SlyGame::SetCutsceneHud
void SlyGame::SetCutsceneHud(bool on)
{
    if (cutsceneHudLocked)
        return;
    SetCutsceneMode(on);
    if (returnTreasure && returnTreasure->IsActive())
        returnTreasure->SetPaused(on);
    if (!on) {
        hud::SetAllEnabled(true, false);
        if (ape::ApeGuiObject* letterbox = ape::FindGuiObject("InGame.Cutscene.Letterbox", nullptr)) {
            ape::StopGuiScripts("", letterbox);
            ape::PlayGuiScript("Letterbox_Shutdown", letterbox, true, false);
        }
    } else {
        hud::ResetHudElements(false);
        entity::Entity* player = players[0].character;
        if (player && player->HasTypeBit(3)) {
            if (entity::EntityComponent* gadget = entity::EntityGadget(player))
                GadgetPutAway(gadget, false, true, false);
        }
    }
}

// @func 0x0048A2F8 SlyGame::ReturnToHideout
void SlyGame::ReturnToHideout()
{
    SlyGame* game = g_game;
    game::JobManager* jobs = game::GetJobManager();
    game::Job* episode = jobs ? jobs->CurrentEpisode() : nullptr;
    if (episode && episode->nameHash != 0) {
        if (jobs->instance)
            jobs->FinishJobInstance();
        g_letterboxWasVisible = false;
        entity::Entity* player = game->players[0].character;
        if (player && player->HasTypeBit(3)) {
            if (entity::EntityComponent* gadget = entity::EntityGadget(player))
                GadgetPutAway(gadget, false, true, false);
        }
        game->pendingLevelName[0] = '\0';
        episode = jobs->CurrentEpisode();
        game->RequestLevel(episode ? episode->nameHash : 0, false);
        return;
    }
    // Outside an episode: the level's own hideout (level parameter
    // 0x10B8A275).
    const uint32_t hideout = game->levelParams ? game->levelParams->hashes158C[4] : 0;
    if (hideout != 0) {
        g_letterboxWasVisible = false;
        game->pendingLevelName[0] = '\0';
        if (jobs)
            jobs->EndJob(true);
        game->RequestLevel(hideout, false);
    }
}

} // namespace sly

namespace sly {

// @func 0x0048B810 SlyGame::UpdateCutsceneQueue
void SlyGame::UpdateCutsceneQueue(float dt)
{
    // HUD screen that freezes the cutscenes while it is busy. NOTE: name
    // unknown (console global 0x00F64CFC).
    constexpr uint32_t kCutscenePauseScreen = 0xC2AB44C8u;
    constexpr uint32_t kFinished = 0x20000000u;
    constexpr uint32_t kBecomeSly = 0x40000000u;
    constexpr uint32_t kModalSkip = 0x40000000u;   // modal box data +0x3C bit 30

    if (menu != 0)
        return;
    if (cutsceneSkipRequested) {
        cutsceneSkipRequested = false;
        SetCutsceneHud(true);
    }

    // The modal box (sub state 0x0C) is told that the cutscene under it ended.
    auto noteModalBox = [this]() {
        if (GetState(1) != kSub_Modal)
            return;
        if (auto* data = reinterpret_cast<ModalBoxData*>(StateData(1)))
            data->flags |= kModalSkip;
    };

    for (size_t i = 0; i < cutsceneQueue.Count(); ++i) {
        QueuedCutscene* cutscene = cutsceneQueue[i];
        if (hud::HudObject* blocker = hud::FindType2(kCutscenePauseScreen); blocker && (blocker->animState >> 1) != 0)
            dt = 0.0f;
        if (!cutscene->instance) {
            entity::Entity* player = players[0].character;
            if ((cutscene->flags & kBecomeSly) && player && player->HasTypeBit(3) && (player->classBits & 0x20))
                static_cast<entity::Character*>(player)->BecomeSly();
            cutscene->instance = new ape::ApeAnimScriptInstance(cutscene->script);
            ApplyScriptBindings(cutscene->instance, cutscene->bindings.data(), cutscene->bindings.size(),
                                cutscene->source, false, false);
            if (mainView) {
                cutscene->savedNearClip = mainView->zNear;
                mainView->SetNear(g_gameCutSceneNearZClip.Get());
            }
        }
        if (!cutscene->instance || (cutscene->flags & kFinished))
            continue;
        cutscene->time += dt;
        cutscene->instance->Update(dt);
        if (!cutscene->instance->IsFinished(false) || gameplay::IsSequenceRunning(cutscene->sequence))
            break;   // later cutscenes wait for this one
        noteModalBox();
        if (mainView)
            mainView->SetNear(cutscene->savedNearClip);
        if (!g_debugIGCLoop.Get()) {
            cutscene->flags |= kFinished;
        } else {
            // Debug loop: the cutscene starts over at once.
            delete cutscene->instance;
            cutscene->instance = nullptr;
            --i;
        }
    }

    for (size_t i = 0; i < cutsceneQueue.Count(); ++i) {
        QueuedCutscene* cutscene = cutsceneQueue[i];
        if (!(cutscene->flags & kFinished))
            continue;
        cutscene->flags &= ~kFinished;
        delete cutscene->instance;
        cutscene->instance = nullptr;
        delete cutscene;
        cutsceneQueue.RemoveAt(i);
        if (cutsceneQueue.Count() == 0) {
            // The game events that came in during the cutscenes reach the
            // job manager now. NOTE: the console leaves the log's read index
            // where it was.
            if (game::JobManager* jobs = game::GetJobManager()) {
                GameEventRing<64>& log = players[0].deferredEvents;
                for (uint16_t e = log.read; e != log.write; e = uint16_t((e + 1) % 64))
                    jobs->OnGameEvent(0, log.events[e]);
            }
            cutscenePlaying = false;
            flags2D |= 0x20;
            if (!resumeAfterCutscenes) {
                SetCutsceneHud(false);
                return;
            }
            resumeAfterCutscenes = false;
            ClearCameraModes();
            playerCameraMode = nullptr;
            if (camera) {
                camera->SetVisibilityDeferred(false, false);
                camera->SetRenderEnabled(false);
            }
            for (entity::Entity* entity : pausedEntities) {
                if (entity)
                    entity->SetActive(true);
            }
            return;
        }
        --i;
    }
}

// @func 0x004845F0 SlyGame::ClearCameraModes
void SlyGame::ClearCameraModes()
{
    if (!camera)
        return;
    while (camera::CameraMode* mode = camera->TopMode()) {
        switch (mode->GetType()) {
        case 1:
            CameraMode1SetTarget(mode, nullptr);
            break;
        case camera::kCamMode_Default:
            static_cast<camera::DefaultCameraMode*>(mode)->SetTarget(nullptr);
            break;
        case camera::kCamMode_Spline:
            static_cast<camera::SplineCameraMode*>(mode)->SetTarget(nullptr);
            break;
        case camera::kCamMode_Vehicle:
            static_cast<camera::VehicleCameraMode*>(mode)->SetTarget(nullptr);
            break;
        case camera::kCamMode_HavokVehicle:
            static_cast<camera::HavokVehicleCameraMode*>(mode)->SetTarget(nullptr);
            break;
        default:
            break;
        }
        if (mode == camera->animMode)
            camera->animMode = nullptr;
        camera->RemoveMode(-1.0f, mode);
        if (mode == playerCameraMode) {
            delete mode;
            playerCameraMode = nullptr;
        }
    }
}

} // namespace sly

namespace sly {

// @func 0x00486460 DefaultSpawnTransform
// Where character slot `slot` starts without a placement: the point
// instance named by "Game.PlayerSpawnPointName<n>", else the identity at
// "Game.PlayerSpawnPos".
void DefaultSpawnTransform(int slot, ape::Matrix34* out)
{
    static const ape::TuningHash* const kPointNames[SlyGame::kMaxPlayers] = {
        &g_gamePlayerSpawnPointName,  &g_gamePlayerSpawnPointName2, &g_gamePlayerSpawnPointName3,
        &g_gamePlayerSpawnPointName4, &g_gamePlayerSpawnPointName5, &g_gamePlayerSpawnPointName6,
        &g_gamePlayerSpawnPointName7, &g_gamePlayerSpawnPointName8, &g_gamePlayerSpawnPointName9,
    };
    if (slot >= 0 && slot < SlyGame::kMaxPlayers) {
        if (const uint32_t pointName = kPointNames[slot]->Get()) {
            if (ape::world::ApeObj* point = ape::world::FindObject(pointName, ape::world::kObjType_PointInstance)) {
                *out = point->GetLocal();
                return;
            }
        }
    }
    *out = ape::world::kWorldIdentity;
    const float* position = g_gamePlayerSpawnPos.Get();
    out->translation = ape::Vec3{position[0], position[1], position[2], out->translation.pad};
}

} // namespace sly

namespace sly {

// @func 0x0046C3B8 SlyGame::QueueCutscene
void SlyGame::QueueCutscene(uint32_t script, uint32_t library, bool flag, const TextField* bindings, size_t count,
                            int32_t value, ape::ApeAnimScriptInstance* source, bool resume, bool becomeSly,
                            bool playing)
{
    constexpr size_t kMaxQueued = 4;
    if (!g_gameCutSceneEnable.Get() || script == 0)
        return;
    ape::ApeAnimScript* found = ape::ApeAnimScriptLibrary::FindScriptAnywhere(script, library, true);
    if (!found) {
        (void)ape::TempHexString(script);    // compiled-out warning
        (void)ape::TempHexString(library);
        return;
    }
    if (cutsceneQueue.Count() >= kMaxQueued)
        return;
    if (cutsceneQueue.Count() == 0)
        cutsceneSkipRequested = true;
    game::JobManager* jobs = game::GetJobManager();
    resumeAfterCutscenes = resumeAfterCutscenes || resume || (jobs && jobs->failingGoal);
    cutscenePlaying = cutscenePlaying || playing;

    auto* cutscene = new QueuedCutscene();
    cutscene->sequence = script;
    cutscene->script = found;
    cutscene->field_2C = value;
    cutscene->source = source;
    cutscene->flags = (flag ? 0x80000000u : 0u) | (becomeSly ? 0x40000000u : 0u);
    if (bindings)
        cutscene->bindings.assign(bindings, bindings + count);
    cutsceneQueue.PushBack(cutscene);
}

} // namespace sly

namespace sly {

bool g_preorderUnlocked = false;

const ExtraDef kExtras[kExtraCount] = {
    {5, "$EXTRA_PARAGLIDER_MONKEY", "$EXTRA_PARAGLIDER_MONKEY_DESC"},
    {10, "$EXTRA_PARAGLIDER_CAMO", "$EXTRA_PARAGLIDER_CAMO_DESC"},
    {15, "$EXTRA_UL_SLY", "$EXTRA_UL_SLY_DESC"},
    {20, "$EXTRA_UL_BENTLEY", "$EXTRA_UL_BENTLEY_DESC"},
    {30, "$EXTRA_UL_MURRAY", "$EXTRA_UL_MURRAY_DESC"},
    {40, "$EXTRA_UL_CARMELITA", "$EXTRA_UL_CARMELITA_DESC"},
    {50, "$EXTRA_RATCHET_WRENCH", "$EXTRA_RATCHET_WRENCH_DESC"},
    {60, "$EXTRA_COLE_AMP", "$EXTRA_COLE_AMP_DESC"},
};

namespace {
bool IsPreorderExtra(int index) { return index == 0 || index == 1 || index == 4; }
// NOTE: the first job of a new game (name not recovered).
constexpr uint32_t kFirstJob = 0x3D264A9Bu;
constexpr uint32_t kResumeAllGroupsButMenu = 0xFFBFFFFFu;
} // namespace

// @func 0x0046C7E0 ExtraEarned
bool ExtraEarned(int index, uint32_t masks, uint32_t needed)
{
    if (IsPreorderExtra(index) && g_preorderUnlocked)
        return true;
    return masks >= needed;
}

// @func 0x00472234 SlyGame::ResumeSavedGame
void SlyGame::ResumeSavedGame()
{
    flags2D |= 0x90;
    game::JobManager* jobs = game::GetJobManager();
    const uint32_t masks = jobs ? static_cast<uint32_t>(jobs->CountCollectedMasks()) : 0;
    for (int i = 0; i < kExtraCount; ++i) {
        if (!ExtraEarned(i, masks, kExtras[i].masksNeeded))
            progress.extraStates[i] &= ~0x02;
    }
    if (menu != 0)
        audio::ResumeGroups(kResumeAllGroupsButMenu);

    if (jobs) {
        bool started = false;
        if (progress.lastEpisodeName != 0) {
            started = jobs->StartJobFromHubByName(progress.lastEpisodeName, progress.lastGoalName, false, false,
                                                  true, false, true);
        }
        if (!started) {
            if (progress.lastLevel != 0 && jobs->HasJob(progress.lastLevel)) {
                jobs->SelectJobByValue(progress.lastJobId);
                RequestLevel(progress.lastLevel, false);
            } else {
                jobs->StartJobFromHubByName(kFirstJob, 0, true, false, true, false, false);
            }
        }
    }

    audio::SoundName* music = audio::CreateSoundName(g_audioPostSaveDataLoadMusic.Get(), 0);
    if (audio::SoundNameHasDef(music))
        audio::SetPendingMusic(music, false, false);
    else
        audio::StopMusic(true);
    audio::SoundNameRelease(music);
}

} // namespace sly

namespace sly {

// @func 0x0046E760 ActiveHideoutMenuGui
ape::ApeGuiObject* ActiveHideoutMenuGui()
{
    // The active type-0x200 element is always a hideout menu.
    auto* menu = static_cast<hud::HudHideoutMenu*>(hud::FindActiveType200());
    return menu ? menu->gui : nullptr;
}

} // namespace sly

namespace sly {

// @func 0x00475C28 SlyGame::OnFinalGoal
bool SlyGame::OnFinalGoal() const
{
    game::JobManager* jobs = game::GetJobManager();
    game::Job* job = jobs ? jobs->currentJob : nullptr;
    if (!job)
        return false;
    for (uint32_t i = job->episode + 1u; i < jobs->jobCount; ++i) {
        if (jobs->jobs[i].active)
            return false;
    }

    (void)job->CurrentStage();    // NOTE: result unused on the console
    uint32_t stagesDone = 0;
    for (uint32_t i = 0; i < job->stageCount; ++i) {
        if (job->stages[i].State() == 2)
            ++stagesDone;
    }
    if (job->stageCount > stagesDone + 1)
        return false;

    const GamePlayer& player = players[0];
    const uint32_t goalCount = player.goals.Count();
    uint32_t goalsDone = 0;
    goals::Goal* open = nullptr;
    for (uint32_t i = 0; i < goalCount; ++i) {
        goals::Goal* goal = player.goals[i];
        uint32_t word = 0;
        std::memcpy(&word, &player.field_20074[goal->saveSlot * 4], sizeof(word));
        if ((word >> 28) == goals::kGoal_Complete)
            ++goalsDone;
        else
            open = goal;
    }
    if (goalCount > goalsDone + 1)
        return false;
    if (goalsDone + 1 != goalCount || !open)
        return true;
    return !open->BlocksNextScripts();
}

// @func 0x00475E20 SlyGame::RecordResumePoint
void SlyGame::RecordResumePoint()
{
    game::JobManager* jobs = game::GetJobManager();
    if (!jobs)
        return;
    if (game::IsInJob()) {
        if (game::Job* job = jobs->FindJobById(progress.currentJobValue)) {
            progress.lastJobId = progress.currentJobValue;
            progress.lastLevel = job->nameHash;
        } else {
            progress.lastJobId = 0;
            progress.lastLevel = 0;
        }
        progress.lastEpisodeName = 0;
        progress.lastGoalName = 0;
        return;
    }
    progress.lastJobId = jobs->currentJob ? jobs->currentJob->field_2D4 : 0;
    progress.lastLevel = jobs->currentJob ? jobs->currentJob->nameHash : 0;
    const bool final = OnFinalGoal();
    progress.lastEpisodeName = (!final && jobs->instance) ? jobs->instance->nameHash : 0;
    progress.lastGoalName = (!final && jobs->trackedGoal) ? jobs->trackedGoal->nameHash : 0;
}

} // namespace sly

namespace sly {

// @func 0x0047E3CC SlyGame::ClearCutsceneQueue
void SlyGame::ClearCutsceneQueue()
{
    constexpr uint32_t kModalSkip = 0x40000000u;   // modal box data +0x3C bit 30
    for (size_t i = 0; i < cutsceneQueue.Count(); ++i) {
        QueuedCutscene* cutscene = cutsceneQueue[i];
        if (GetState(1) == kSub_Modal) {
            if (auto* data = reinterpret_cast<ModalBoxData*>(StateData(1)))
                data->flags |= kModalSkip;
        }
        if (mainView)
            mainView->SetNear(cutscene->savedNearClip);
        delete cutscene->instance;
        cutscene->instance = nullptr;
        delete cutscene;
    }
    cutsceneQueue.Clear();
    resumeAfterCutscenes = false;
    cutscenePlaying = false;
    cutsceneSkipRequested = false;
}

} // namespace sly

namespace sly {
namespace entity { void ClearInstanceRenderLists(); }

// @func 0x0047C2B8 SlyGame::UpdateWorld
void SlyGame::UpdateWorld(float dt)
{
    constexpr int kFrameList = 0;
    constexpr int kWorldList = 12;
    if (!levelWorld)
        return;
    if (dt <= 0.0f)
        dt = 1.0f / 60.0f;
    game::UpdateFileStreaming();
    callbackLists[kFrameList].Clear();
    HudFrameBegin(dt);
    RunCallbacks(kFrameList, dt, true);
    entity::ClearInstanceRenderLists();
    // NOTE: the console hands these callbacks no frame time.
    RunCallbacks(kWorldList, 0.0f, false);
    HudFrameEnd(dt);
    levelWorld->Update(dt, nullptr);
}

} // namespace sly

namespace sly {

// @func 0x0048888C SlyGame::AutoSaveAfterJob
void SlyGame::AutoSaveAfterJob()
{
    constexpr uint16_t kEpisodeStateMask = 0x18;
    constexpr uint16_t kEpisodeFinished = 0x10;
    constexpr uint8_t kJobComplete = 2;
    if (OnFinalGoal()) {
        game::JobManager* jobs = game::GetJobManager();
        game::Job* job = jobs ? jobs->currentJob : nullptr;
        if (job && jobs->instance) {
            progress.jobStates[jobs->instance->index] = kJobComplete;
            progress.lastLevel = job->nameHash;
            progress.lastEpisodeName = 0;
            progress.lastGoalName = 0;
            uint16_t& flags = progress.episodes[job->episode].flags;
            flags = uint16_t((flags & ~kEpisodeStateMask) | kEpisodeFinished);
            game::UnlockByKey(ape::HashNameNoCase(job->endMovie));
            game::UnlockByKey(ape::HashNameNoCase(job->endMovie2));
            (void)CountUnlockedTrophies();   // NOTE: results unused on the console
            (void)TrophyTotal();
            if (TrophyTotal() == CountUnlockedTrophies())
                game::UnlockByKey(ape::HashNameNoCase(job->endMovie3));
        }
    }
    // A busy save system gets one more try.
    if (AutoSaveGame(&progress, sizeof(progress)) != kSave_Busy)
        AutoSaveGame(&progress, sizeof(progress));
}

// @func 0x0047E1D4 SlyGame::ReleaseLevelParams
void SlyGame::ReleaseLevelParams()
{
    if ((flags1C >> 6) != 0)
        return;
    delete levelParams;
    levelParams = nullptr;
}

} // namespace sly

namespace sly {

// @func 0x0047DFC8 SlyGame::ResumeFromPause
void SlyGame::ResumeFromPause()
{
    // NOTE: key and message names unknown.
    constexpr uint32_t kResumeKey = 0x846AF162u;
    constexpr uint32_t kResumeMessage = 0x560A2252u;
    constexpr uint32_t kResumeAllGroupsButMenu = 0xFFBFFFFFu;

    SetEngineUpdateMasks(0xFF);
    PostToListeners(nullptr, kResumeKey, kResumeMessage, 0);
    if (minigame)
        minigame->Slot10();
    hud::ResumeHud();
    if (SaveSystemState() != kSave_Busy)
        hud::HideBusyElement();
    flags2D |= 0x08;
    OpenMenu(0, 0);
    audio::ResumeGroups(kResumeAllGroupsButMenu);
    SetGadgetsPaused(false);
    const bool vibration = (settings.flags & GameSettings::kFlag_Vibration) != 0;
    for (AppPlayer& player : GetAppRuntime().players)
        player.SetRumblePaused(!vibration);
}

} // namespace sly

namespace sly {

// @func 0x00468A38 SlyGame::UpdateSlowTimeAudio
void SlyGame::UpdateSlowTimeAudio(float dt)
{
    constexpr float kEpsilon = 1e-5f;
    constexpr float kOpenCutoff = 23999.0f;
    if (std::fabs(dt) < kEpsilon)
        return;
    const float rate = audioRamp.active ? audioRamp.value : 1.0f;
    audio::SetGlobalPlaybackRate(rate);
    const audio::VoiceId music = audio::MusicVoice();
    if (!audio::IsVoiceValid(music))
        return;

    const float slowCutoff = g_audioSlowTimeMusicLPFFrequency.Get();
    const float fade = g_audioSlowTimeMusicLPFCrossfadeDuration.Get();
    if (std::fabs(rate - 1.0f) < kEpsilon) {
        if (musicFilterCutoff >= kOpenCutoff)
            return;
        if (std::fabs(fade) < kEpsilon)
            musicFilterCutoff = kOpenCutoff;
        else
            musicFilterCutoff = std::min(kOpenCutoff, musicFilterCutoff + (kOpenCutoff - slowCutoff) / fade * dt);
    } else {
        if (musicFilterCutoff <= slowCutoff)
            return;
        if (std::fabs(fade) < kEpsilon)
            musicFilterCutoff = fade;   // NOTE: the console stores the duration here, not the cut-off
        else
            musicFilterCutoff = std::max(slowCutoff, musicFilterCutoff + (slowCutoff - kOpenCutoff) / fade * dt);
    }
    audio::SetVoiceLowPass(music, musicFilterCutoff);
}

} // namespace sly
