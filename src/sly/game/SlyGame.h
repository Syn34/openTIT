// openTIT - The game world object (0x1286B0 bytes on PS3, global pointer at
// 0x00F65CA0; constructor 0x0045CC18, vtable 0x00B21A00).
//
// The game is a four-layer state machine (gameplay::StateMachine with 0x80
// bytes of state data per layer): layer 0 is the main state (front end,
// in-game, loading, ...), layer 1 the sub state of the main state (menus,
// cutscenes, save screens), layers 2 and 3 are scratch layers of the
// sub states. Main state changes are not immediate: RequestMainState() starts
// a transition (+0x32: fade out, switch, save, fade in) that the update runs;
// sub state requests made during a transition are held until it is over.
//
// The members are listed in console order with their offsets in comments.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/GamePlayer.h"
#include "sly/save/GameSettings.h"
#include "sly/gameplay/StateMachine.h"

#include <cstdint>
#include <cstring>

namespace ape::file { class File; }
namespace ape { class ApeLevelDef; class StringTable; class ApeAnimScriptInstance; class ApeGuiObject; }
namespace agl { class AglRenderTarget; }

namespace sly {

class WorldObject;   // world entities, reconstructed by the world module
class SpawnManager;
namespace goals { class Goal; }
namespace gameplay { class Minigame; }
namespace entity { class Entity; class GameCamera; }
struct LevelParams;

// A deferred call queued on one of the world callback lists: the game
// calls function(owner, context) when it runs the list.
// NOTE: on the console 'function' is a function descriptor; the callers
// pass static functions of their modules.
struct WorldCallback {
    void* function = nullptr;
    void* context = nullptr;
    entity::Entity* owner = nullptr;
};

// Screen colour fade (+0x128290, initialised by 0x007866CC): blends the
// colour of the "Common.Fade" GUI object (g_fadeObject) from `from` to `to`
// over `duration` seconds.
struct ScreenFade {
    // Starts a fade (0x00468C40). Unless `force` is set, a fade to the colour
    // the object already shows (with the visibility the target alpha
    // implies) is applied at once.
    void Start(float duration, ape::xml::Color8 from, ape::xml::Color8 to, bool hideWhenDone, bool force);
    // Advances the fade (0x00469B90).
    void Update(float dt);

    ape::xml::Color8 from{0, 0, 0, 0};   // +0x00
    ape::xml::Color8 to{0, 0, 0, 0};     // +0x04
    float duration = 0.0f;               // +0x08
    float elapsed = 0.0f;                // +0x0C
    bool hideWhenDone = false;           // +0x10 bit 31: hide the fade object once finished
    bool active = false;                 // +0x10 bit 30
};

// A value ramp (+0x1282A4 time scale, +0x1282C0 audio rate; initialised by
// 0x00786724, updated by 0x00787070/0x00787190): `value` moves from
// `start` to `target` over `duration`.
// NOTE: the field names follow their use in SlyGame::TimeScale and
// 0x00468A38; the update functions are outside the game shell range.
struct ValueRamp {
    float start = 1.0f;          // +0x00
    float duration = 0.25f;      // +0x04
    float value = 1.0f;          // +0x08
    float target = 0.0f;         // +0x0C
    float elapsed = 0.0f;        // +0x10
    float field_14 = 0.0f;       // +0x14
    bool active = false;         // +0x18 bit 31
};

// Depth of field animation (+0x1282DC, initialised by 0x00786758,
// updated by 0x004697D8). The DoF effect's focus distance either blends
// from focusStart to focusEnd or follows the distance of `target` to the
// camera; the range is blended the same way, the intensity always.
struct DofAnimation {
    void Update(float dt);   // 0x004697D8

    entity::Entity* target = nullptr;   // +0x00 focus on this entity when set
    float intensityStart = 0.0f;        // +0x04
    float intensityEnd = 1.0f;          // +0x08
    float focusStart = 10.0f;           // +0x0C
    float focusEnd = 20.0f;             // +0x10
    float rangeStart = 10.0f;           // +0x14
    float rangeEnd = 40.0f;             // +0x18
    float duration = 1.0f;              // +0x1C
    uint8_t quality = 1;                // +0x20 1..4 (DoF sample count)
    float elapsed = 0.0f;               // +0x24
    bool disableWhenDone = false;       // +0x28 bit 31
    bool active = false;                // +0x28 bit 29
    bool nearBlur = false;              // +0x28 bit 30: the effect's near-blur switch
    bool hold = false;                  // +0x28 bit 28: keep running after the end
};

// Bloom noise pulse (+0x128308, updated by 0x004695A8): switches the bloom
// effect off and the bloom noise effect on, ramps the noise effect's pulse
// amount from `from` to `to` over `duration`, then switches both back.
// `steps` names the kind of each of the two steps; `step` counts them
// (0xFF: idle).
struct NoisePulse {
    void Update(float dt);   // 0x004695A8

    enum StepKind : int32_t { kStep_Start = 0, kStep_Ramp = 1, kStep_Restore = 2 };
    int32_t steps[2] = {};       // +0x00
    uint8_t step = 0xFF;         // +0x08
    bool bloomWasOn = false;     // +0x09
    float from = 0.0f;           // +0x0C
    float to = 0.0f;             // +0x10
    float duration = 0.0f;       // +0x14
    float elapsed = 0.0f;        // +0x18
};

// One queued desaturation (+0x128334, 0x1C bytes, up to four, updated by
// 0x004691E8). The newest entry drives the desaturation effect; a finished
// entry stays (holding its end values) unless removeWhenDone is set.
struct DesaturationFade {
    uint8_t blendModeB = 0;      // +0x00 bits 28-31
    uint8_t blendModeA = 0;      // +0x00 bits 24-27
    bool removeWhenDone = false; // +0x00 bit 23
    uint8_t colorFrom[4] = {};   // +0x01 (channels 1-3 used: red, green, blue)
    uint8_t colorTo[4] = {};     // +0x05
    float amountFrom = 0.0f;     // +0x0C
    float amountTo = 0.0f;       // +0x10
    float duration = 0.0f;       // +0x14
    float elapsed = 0.0f;        // +0x18
};

// The queued desaturations (+0x128334, count +0x1283A4).
struct DesaturationQueue {
    static constexpr uint32_t kMaxFades = 4;
    void Update(float dt);   // 0x004691E8

    DesaturationFade fades[kMaxFades];
    uint32_t count = 0;
};

// Blur animation (+0x1283A8, initialised by 0x007867C8, updated by
// 0x00468EF8): ramps the blur effect's amount between two values.
struct BlurAnimation {
    void Update(float dt);   // 0x00468EF8
    // Starts a ramp (0x00470560).
    void Start(float fromAmount, float toAmount, float time, uint32_t passes, bool disableAtEnd);

    float from = 0.0f;           // +0x00
    float to = 1.0f;             // +0x04
    float duration = 1.0f;       // +0x08
    uint32_t quality = 1;        // +0x0C 1..5
    float elapsed = 0.0f;        // +0x10
    bool disableWhenDone = false;// +0x14 bit 31
    bool active = false;         // +0x14 bit 30
};

// A cutscene waiting in the game's queue (+0x12820C). NOTE: only the
// fields the game shell reads are named.
struct QueuedCutscene {
    uint32_t field_00[5] = {};                  // +0x00
    uint32_t scriptName = 0;                    // +0x14
    void* script = nullptr;                     // +0x18
    ape::ApeAnimScriptInstance* instance = nullptr; // +0x1C
    float time = 0.0f;                          // +0x20
    float savedNearClip = 0.0f;                 // +0x24 main view near plane while it plays
    uint32_t sequence = 0;                      // +0x28
    uint32_t field_2C = 0;                      // +0x2C
    uint32_t flags = 0;                         // +0x30 bit 30: hide player, bit 29: finished
};

struct DialogSpeakerEntry {
    uint8_t bytes[12];
};

// Speaker dialog block (+0x1283C0, initialised by 0x007867FC).
// NOTE: only the parts the game shell clears are laid out.
struct DialogState {
    char text[0x41] = {};                   // +0x00
    char sound[0x43] = {};                  // +0x41
    ape::ApeTArray<DialogSpeakerEntry> speakers;   // +0x84 six 12-byte entries
    uint16_t field_8C = 0;                  // +0x8C
    uint32_t field_98 = 0;                  // +0x98
    uint32_t screenHash = 0;                // +0x9C
    uint32_t field_A0 = 0;                  // +0xA0
    uint32_t field_A4 = 0;                  // +0xA4
    uint32_t field_A8 = 0;                  // +0xA8
    uint32_t field_AC = 0;                  // +0xAC
    uint32_t field_B0 = 0;                  // +0xB0
    uint32_t flags = 0;                     // +0xB4 bit 30: busy, bit 29/31 options
    uint32_t mixState = 0;                  // +0xB8 mix state pushed for the dialog
};

class SlyGame : public gameplay::StateMachine {
public:
    // Thirteen callback lists (+0x128480, 0x14-byte arrays). Lists 5..11
    // only live for one frame: they are emptied at the end of every render
    // frame.
    static constexpr int kCallbackListCount = 13;
    static constexpr int kFirstFrameList = 5;
    static constexpr int kFrameListCount = 7;
    static constexpr int kMaxPlayers = 9;
    static constexpr int kSplitViews = 4;

    // Main-state transition steps (+0x32). 0 means none; 1..11 are the
    // steps of a running transition (sub state requests are deferred).
    static constexpr uint8_t kTransition_None = 0;
    static constexpr uint8_t kTransition_Start = 1;
    static constexpr uint8_t kTransition_Last = 11;

    explicit SlyGame(uint8_t initialState = 0);
    ~SlyGame() override;

    // Slot 8: the requested main state while a transition is running,
    // otherwise the current state of the layer.
    uint8_t GetState(uint32_t layer) const override;
    // Slot 9: the game update.
    bool OnMessage(const Message& message) override;

    // -- state requests -----------------------------------------------------
    bool InTransition() const { return transition != kTransition_None && transition <= kTransition_Last; }
    // Starts a transition to main state `state` (immediate when the game
    // has no main state yet).
    void RequestMainState(uint8_t state);
    // Sets the sub state, or keeps it for the end of the running transition.
    void RequestSubState(uint8_t state);
    // State change of any layer through the request rules (0x004665C0).
    bool ChangeState(uint32_t layer, uint8_t state);

    // Resets the per-frame lists filled during rendering.
    void ResetFrameLists();
    // Queues function(owner, context) on callback list 'list'.
    void AddCallback(int list, void* function, void* context, entity::Entity* owner);
    // Removes every entry of list 'list' with this context and owner.
    // NOTE: the console passes the function too but does not compare it.
    void RemoveCallback(int list, void* context, entity::Entity* owner);
    // Drops every callback owned by 'owner' from all the lists (entity
    // teardown).
    void ForgetEntity(entity::Entity* owner);
    // Calls every callback of list 'list' (in the order queued); with
    // 'clear' the list is emptied afterwards.
    void RunCallbacks(int list, float dt, bool clear);

    // -- the level's game mode (+0x1281F4) ------------------------------------
    // Stops and destroys the running minigame, then installs 'next' (may be
    // null). The new minigame is not started.
    void SetMinigame(gameplay::Minigame* next);
    // Stops and destroys the running minigame.
    void EndMinigame();
    // Shows or hides the HUD through the running minigame (if any).
    void SetMinigameHudVisible(bool visible);
    // Player meter of the HUD. NOTE: the console passes the player index;
    // the HUD keeps a single player meter, so it is unused.
    void SnapPlayerMeter(uint32_t player);
    void SetPlayerMeterState(uint32_t player, bool on);
    // Snaps the HUD meter that follows 'owner' to its current value.
    void SnapMeterOf(const entity::Entity* owner);
    // Requests a reload of the running level at the next update (state bit
    // 0x40); 'flag' goes to state bit 0x20. NOTE: the flag's meaning is not
    // known (the job manager and the level script command pass false).
    void RequestLevelRestart(bool flag);
    // Name of the running level ("" before a level is loaded).
    const char* CurrentLevelName() const;

    // Timestamp stamped into every game event: the raw bits of the time
    // spent in the current main state (the console reads the first word of
    // the state machine's timer array).
    uint32_t EventTime() const
    {
        const float t = TimeInState(0);
        uint32_t bits;
        std::memcpy(&bits, &t, sizeof bits);
        return bits;
    }

    // -- players ---------------------------------------------------------------
    GamePlayer& Player(int index) { return players[index]; }
    const GamePlayer& Player(int index) const { return players[index]; }
    // Character of a game player (null when the slot has none).
    entity::Entity* PlayerCharacter(int index = 0) const { return players[index].character; }
    // The player block of the active player (+0x1E).
    GamePlayer& ActivePlayer() { return players[activePlayer]; }

    // -- small accessors of the game update (0x00787830-0x00787D5C) ------------
    // Sets or clears bits of the world flags (+0x28; bit 1 HUD hidden, bit 2
    // input locked, see gameplay::WorldFlags).
    // @func 0x00787B80 SlyGame::SetWorldFlags
    void SetWorldFlags(uint32_t mask, bool on) { worldFlags = on ? (worldFlags | mask) : (worldFlags & ~mask); }
    // The sub-state word (+0x48) only counts while +0x2C bit 7 is set.
    // NOTE: names guessed from the update's use (values 0-3).
    // @func 0x00787914 SlyGame::SubState
    uint32_t SubState() const { return (stateFlags & 0x80) ? subState : 0; }
    // @func 0x00787938 SlyGame::SetSubState
    void SetSubState(uint32_t state)
    {
        if (stateFlags & 0x80)
            subState = state;
    }
    // Exposure ramp (+0x128290) is idle.
    // @func 0x00787830 SlyGame::Flag1282A0Clear
    bool Flag1282A0Clear() const { return !screenFade.active; }
    // Game speed: the two scales (+0x128588, +0x128594), with the slow-motion
    // ramp value (+0x1282AC) while it is active (+0x1282BC bit 31).
    // @func 0x007877D0 SlyGame::TimeScale
    float TimeScale() const
    {
        float scale = timeScaleA * timeScaleB;
        if (timeRamp.active)
            scale *= timeRamp.value;
        return scale;
    }
    // @func 0x00787D44 SlyGame::EndSlowMotion
    void EndSlowMotion() { timeRamp.active = false; }
    // +0x1282D8 bit 31: the audio rate ramp.
    // @func 0x00787D5C SlyGame::ClearFlag1282D8
    void ClearFlag1282D8() { audioRamp.active = false; }
    // Queues a blur animation (+0x1283A8): amounts from/to, duration, the
    // quality word and the `flag` bit (disable when done); bit 30 marks the
    // request pending.
    // @func 0x00787CCC SlyGame::RequestBlend
    void RequestBlend(float a, float b, float c, uint32_t word, bool flag)
    {
        blur.Start(a, b, c, word, flag);
    }

    // -- post effects of the main view ----------------------------------------
    // Depth of field settings, ignored while a DoF animation runs. The
    // intensity is scaled by "Game.IGC.DOFIntensity" and switches the effect
    // on when positive; the range is clamped to 0.01..999.
    void SetDofIntensity(float intensity);
    void SetDofRange(float range);
    void SetDofFocus(float distance);
    void SetDofNearBlur(bool on);
    // The running level's depth of field ("dofEnable" ...), unless the game
    // camera applies its own post effects.
    void ApplyLevelDof();

    // -- options -----------------------------------------------------------------
    // Display gamma from the brightness option (0x0046C8D0).
    void ApplyBrightness() const;
    // Stereo 3D depth and switch from the display options, when the TV
    // supports 3D (0x0046C830).
    void ApplyStereoOptions() const;
    // The fourth volume option scaled to 0..32768 by the master volume.
    int32_t ScaledVolume3() const;
    // A queued cutscene plays `sequence` (+0x28 of the queue entries).
    bool IsSequenceQueued(uint32_t sequence) const;
    // Shows or hides the cutscene letterbox ("InGame.Cutscene.Letterbox").
    static void SetLetterbox(bool on);
    // Frame time of the game: the frame clock's (or the time source's while
    // one is installed), scaled by the slow-motion ramp while it runs.
    float FrameTime() const;
    // Weight of the camera in the audio listener position (0: the player,
    // 1: the camera), set at once or blended over `time` seconds along
    // `curve`.
    void SetListenerCameraWeight(float weight, float time, float curve);
    // Reads the level parameters from the level data document (the first
    // "LevelParams" element; once per level, not in the front end).
    void LoadLevelParams(ape::file::File& file);
    // The level parameters select the special mode 1.
    bool LevelParamsMode1() const;
    // The power meter of a player on the HUD (second bar of meter B).
    void ShowPlayerPower(uint32_t player) const;

    // -- data ------------------------------------------------------------------
    uint8_t flags1C = 0;                        // +0x1C bit 5: autosave allowed
    uint8_t localPlayerCount = 1;               // +0x1D
    uint8_t activePlayer = 0xFF;                // +0x1E player whose pad drives the menus
    bool audioPaused = false;                   // +0x1F
    uint8_t field_20 = 0;                       // +0x20
    uint8_t field_22 = 0;                       // +0x22
    ape::StringTable* strings = nullptr;        // +0x24 GUI/Strings_<language>.str
    uint32_t worldFlags = 0;                    // +0x28
    uint8_t stateFlags = 0;                     // +0x2C NOTE: bit 5/6 toggled by GUI code; bit 7 see SubState
    uint8_t flags2D = 0x29;                     // +0x2D bit 1: reset request, bit 2: cutscene queue, bit 5
    uint8_t flags2E = 0xC0;                     // +0x2E
    uint8_t previousMainState = 0;              // +0x2F state of layer 0 when the last request was made
    uint8_t requestedMainState = 0;             // +0x30
    uint8_t requestedSubState = 0;              // +0x31
    uint8_t transition = 0x0F;                  // +0x32
    uint8_t previousActivePlayer = 0xFF;        // +0x33
    uint8_t field_34 = 0xFF;                    // +0x34
    uint8_t field_35 = 0xFF;                    // +0x35
    float transitionTime = 0.0f;                // +0x38
    float autoSaveTimer = 0.0f;                 // +0x3C
    uint32_t field_40 = 0;                      // +0x40
    uint8_t field_44 = 1;                       // +0x44
    uint32_t subState = 0;                      // +0x48
    // +0x50: save data (0xFE0 bytes written as "save.dat").
    GameSettings settings;                      // +0x1030 the options block
    uint8_t field_1054[4] = {};                 // +0x1054
    uint8_t field_1058 = 0;                     // +0x1058
    uint32_t field_105C = 0;                    // +0x105C
    uint32_t field_1060 = 0;                    // +0x1060
    uint8_t field_1064 = 0;                     // +0x1064
    uint32_t field_1068 = 0;                    // +0x1068
    GamePlayer players[kMaxPlayers];            // +0x1070
    entity::Entity* pausedEntities[kMaxPlayers] = {}; // +0x1280F0 NOTE: reactivated after cutscenes
    uint32_t pendingLevelHash = 0;              // +0x128114
    uint32_t pendingLevel = 0;                  // +0x128118
    char pendingLevelName[0x40] = {};           // +0x12811C
    uint32_t field_12815C = 0;                  // +0x12815C
    uint32_t field_128160 = 0;                  // +0x128160
    LevelParams* levelParams = nullptr;         // +0x128164
    agl::AglRenderTarget* mainView = nullptr;       // +0x128168 the game camera's render target
    agl::AglRenderTarget* hud3dView = nullptr;      // +0x12816C "HUD_3D"
    agl::AglRenderTarget* postHud3dView = nullptr;  // +0x128170 "PostHUD_3D"
    agl::AglRenderTarget* postHud2dView = nullptr;  // +0x128174 "PostHUD_2D"
    uint32_t field_128178[2] = {};              // +0x128178
    agl::AglRenderTarget* splitViews[kSplitViews] = {}; // +0x128184 "SplitRender%d"
    uint32_t field_128194[3] = {};              // +0x128194
    uint32_t field_1281A4 = 0;                  // +0x1281A4
    ape::Vec3 hudCameraPos = {0.0f, 0.0f, 0.0f, 0.0f}; // +0x1281B0 UI.3D.CameraPos
    ape::ApeLevelDef* levelDef = nullptr;       // +0x1281C0 the running level
    uint32_t field_1281C4[4] = {};              // +0x1281C4
    entity::GameCamera* camera = nullptr;       // +0x1281E0 the game camera
    uint32_t splitViewOwners[kSplitViews] = {}; // +0x1281E4
    gameplay::Minigame* minigame = nullptr;     // +0x1281F4 the level's game mode
    uint32_t field_1281F8 = 0;                  // +0x1281F8
    void* guiCommon = nullptr;                  // +0x1281FC UI_Common GUI resource
    void* guiScene = nullptr;                   // +0x128200 GUI/GUI_Common.san
    void* guiScreens = nullptr;                 // +0x128204 front end / in-game GUI resource
    float field_128208 = 0.0f;                  // +0x128208
    ape::ApeTArray<QueuedCutscene*> cutsceneQueue; // +0x12820C queued cutscenes (count +0x128214)
    bool resumeAfterCutscenes = false;          // +0x128220
    bool cutscenePlaying = false;               // +0x128221
    bool cutsceneSkipRequested = false;         // +0x128222
    // +0x128224 NOTE: an object that overrides the frame time (+0x3C) and
    // tracks an entity (+0x158..+0x160); class unknown.
    struct TimeSource {
        uint8_t field_00[0x3C];
        float deltaTime;                        // +0x3C
    };
    TimeSource* timeSource = nullptr;           // +0x128224
    ape::Matrix34 field_128230;                 // +0x128230 identity
    ape::Vec3 listenerPos = {0.0f, 0.0f, 0.0f, 0.0f}; // +0x128260 audio listener position
    float listenerCameraWeight = 1.0f;          // +0x128270
    float listenerWeightTarget = 0.0f;          // +0x128274
    float listenerWeightStart = 0.0f;           // +0x128278
    float listenerWeightProgress = 0.0f;        // +0x12827C 0..1
    float listenerWeightTime = 0.0f;            // +0x128280 0: no blend running
    float listenerWeightCurve = 0.0f;           // +0x128284
    float musicFilterCutoff = 23999.0f;         // +0x128288 slow-time music low-pass frequency
    // NOTE: saved and cleared while a minigame runs, restored when it stops.
    uint8_t field_12828C = 1;                   // +0x12828C
    uint8_t field_12828D = 1;                   // +0x12828D
    ScreenFade screenFade;                      // +0x128290
    ValueRamp timeRamp;                         // +0x1282A4
    ValueRamp audioRamp;                        // +0x1282C0
    DofAnimation dof;                           // +0x1282DC
    NoisePulse noisePulse;                      // +0x128308
    uint32_t field_128324[3] = {};              // +0x128324
    DesaturationQueue desaturation;             // +0x128334
    BlurAnimation blur;                         // +0x1283A8
    DialogState dialog;                         // +0x1283C0
    void* sequencer = nullptr;                  // +0x12847C NOTE: debug net server (flag +0xA4 bit 31)
    ape::ApeTArray<WorldCallback> callbackLists[kCallbackListCount]; // +0x128480
    uint32_t field_128584 = 0;                  // +0x128584
    float timeScaleA = 1.0f;                    // +0x128588
    float timeScaleATarget = -1.0f;             // +0x12858C
    uint8_t field_128590 = 0;                   // +0x128590
    float timeScaleB = 1.0f;                    // +0x128594
    float timeScaleBTarget = -1.0f;             // +0x128598
    uint8_t field_12859C = 0;                   // +0x12859C
    uint32_t hudScreenHash = 0;                 // +0x1285A0 HUD screen of the current sub state
    uint8_t field_1285A4 = 0;                   // +0x1285A4
    uint32_t field_1285A8[3] = {};              // +0x1285A8
    uint32_t field_1285B0[4] = {};              // +0x1285B0
    int32_t field_1285C0 = -1;                  // +0x1285C0
    uint8_t field_1285C4[6] = {};               // +0x1285C4
    uint32_t field_1285CC[6] = {};              // +0x1285CC
    void* field_1285E4 = nullptr;               // +0x1285E4
    uint32_t field_1285EC = 0;                  // +0x1285EC
    uint32_t pauseSound = 0;                    // +0x1285F0
    uint32_t resumeSound = 0;                   // +0x1285F4
    uint8_t field_128610 = 0;                   // +0x128610
    void* frameObject = nullptr;                // +0x1286A4 updated first every frame (slot 2)

    SpawnManager* spawnManager = nullptr;       // NOTE: owner of the spawner groups; slot unknown
};

// The single game world (0x00F65CA0).
extern SlyGame* g_game;
// The "Common.Fade" GUI object the screen fade drives (0x00F65C88).
extern ape::ApeGuiObject* g_fadeObject;

// GUI helpers of the menus (0x004704A8, 0x0046E7B0).
void ShowLanguageMenu(ape::ApeGuiObject* screen, bool visible);
ape::ApeGuiObject* FindFocusableGuiObject(ape::ApeGuiObject* object);
// The HUD uses its corrected (SD 4:3) layouts (0x0046C7B0).
bool IsSdHudLayout();
// Contextual marker definition of a playable character (by the hash of its
// name, "Game.Contextual.<name>"); Sly's for any other.
uint32_t ContextualMarkerFor(uint32_t characterHash);

// Requests a plain reload of the running level.
void RequestLevelRestart();
// Level of detail weight of height differences (EngineConfig +0x3C): set
// directly, or from the running level's "lodYInfluence" (1 without a level).
void SetLodYInfluence(float influence);
void ApplyLevelLodYInfluence(const SlyGame& game);

} // namespace sly
