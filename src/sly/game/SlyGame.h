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
#include "sly/game/Upgrades.h"
#include "sly/game/goals/JobManager.h"
#include "sly/save/GameSettings.h"
#include "sly/gameplay/StateMachine.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace ape::file { class File; }
namespace ape { struct ApeLevelWorld; }
namespace ape { struct GuiEvent; class ApeLevelDef; class StringTable; class ApeAnimScriptInstance; class ApeAnimScript; class ApeGuiObject; }
namespace agl { class AglRenderTarget; }

namespace sly {

class WorldObject;   // world entities, reconstructed by the world module
class SpawnManager;
namespace goals { class Goal; }
namespace gameplay { class Minigame; class ReturnTreasureMicrogame; }
namespace entity { class Entity; class GameCamera; }
namespace audio { struct MixState; struct SoundName; }
namespace camera { class CameraMode; }
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

// A value ramp (+0x1282A4 slow motion, +0x1282C0 audio rate; initialised
// by 0x00786724, updated by 0x00787070/0x00787190): `value` moves from
// `from` to `to` over `duration`.
// NOTE: the update functions are outside the game shell range; `hold` is
// passed through by Start().
struct ValueRamp {
    // Starts the ramp (0x0048EF50); without a duration the value jumps to
    // `to`.
    void Start(float fromValue, float toValue, float time, float holdTime);

    float from = 1.0f;           // +0x00
    float to = 0.25f;            // +0x04
    float value = 1.0f;          // +0x08
    float duration = 0.0f;       // +0x0C
    float hold = 0.0f;           // +0x10
    float elapsed = 0.0f;        // +0x14
    bool active = false;         // +0x18 bit 31
};

// Depth of field animation (+0x1282DC, initialised by 0x00786758,
// updated by 0x004697D8). The DoF effect's focus distance either blends
// from focusStart to focusEnd or follows the distance of `target` to the
// camera; the range is blended the same way, the intensity always.
struct DofAnimation {
    void Update(float dt);   // 0x004697D8
    // Starts an animation (0x0048EF9C).
    void Start(float intensityFrom, float intensityTo, float focusFrom, float focusTo, float rangeFrom,
               float rangeTo, float time, entity::Entity* focusTarget, uint8_t samples, bool disableAtEnd,
               bool nearBlurOn, bool holdAtEnd);

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
    // Finishes any running pulse, then ramps the noise in from its current
    // amount to 1 over `time` (bloom settings copied to the noise effect)
    // (0x0048F008) / out to 0, restoring the bloom (0x0048F158).
    void FadeIn(float time);
    void FadeOut(float time);

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

// A cutscene waiting in the game's queue (+0x12820C): its script, the
// variable bindings handed to the script instance and the sequence that
// must end with it.
struct QueuedCutscene {
    std::vector<TextField> bindings;            // +0x00 (an ApeTArray on the console)
    ape::ApeAnimScriptInstance* source = nullptr; // +0x14 instance `var` bindings copy from
    ape::ApeAnimScript* script = nullptr;       // +0x18
    ape::ApeAnimScriptInstance* instance = nullptr; // +0x1C
    float time = 0.0f;                          // +0x20
    float savedNearClip = 1.0f;                 // +0x24 main view near plane while it plays
    uint32_t sequence = 0;                      // +0x28 the script's name (its sequence has it too)
    int32_t field_2C = 0;                       // +0x2C caller's value. NOTE: a spawn point index for scripts
    // +0x30: bit 31 caller's flag, bit 30 player back to Sly, bit 29 finished.
    uint32_t flags = 0x80000000u;
};

// A speaker of the dialog (12 bytes on the console: name, entity, flags).
struct DialogSpeakerEntry {
    uint32_t nameHash = 0;
    void* entity = nullptr;     // NOTE: the sequence entity (WorldObject)
    uint8_t flags = 0;
};

// Speaker dialog block (+0x1283C0, initialised by 0x007867FC).
// NOTE: only the parts the game shell clears are laid out.
struct DialogState {
    char text[0x41] = {};                   // +0x00
    char sound[0x43] = {};                  // +0x41
    ape::ApeTArray<DialogSpeakerEntry> speakers;   // +0x84 six 12-byte entries
    uint16_t field_8C = 0;                  // +0x8C
    uint32_t field_98 = 0;                  // +0x98 caller's value (HUD screen of the speakers)
    uint32_t screenHash = 0;                // +0x9C
    uint32_t field_A0 = 0;                  // +0xA0
    float duration = 0.0f;                  // +0xA4 seconds the dialog stays (0: until answered)
    uint32_t field_A8 = 0;                  // +0xA8
    float countRate = 0.0f;                 // +0xAC duration / "FreezeScreen.CountDuration"
    uint32_t field_B0 = 0;                  // +0xB0
    uint32_t flags = 0;                     // +0xB4 bit 30: busy, bit 31: caller's option, bit 29 cleared on start
    const audio::MixState* mixState = nullptr;   // +0xB8 mix state pushed for the dialog
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
    // The return-treasure microgame (each call does nothing without the
    // microgame object, the later ones also while it is not running).
    void StartReturnTreasure(float timeLimit, entity::Entity* carrier, entity::Entity* treasure,
                             uint32_t carrierBone, const ape::Vec3& offset);
    bool ReturnTreasureActive() const;
    void CompleteReturnTreasure();
    void UpdateReturnTreasurePlacement();
    void StopReturnTreasure(bool lost);
    void PauseReturnTreasure(bool paused);
    void RefreshReturnTreasureHud();
    void SetReturnTreasureHudActive(bool active);
    // A game event happened to `player`: during play (main state 1, sub state
    // 2 or 3, no queued cutscene) it goes to the player's event log and the
    // job manager; while cutscenes are queued it waits in player 0's
    // deferred log. Event 1 about the entity the time source tracks stops
    // the tracking.
    void RecordEvent(uint32_t player, const GameEvent& event);
    // The running job ended: the job manager records it, then the game
    // restarts the episode, shows the job-complete screen (sub state 7, the
    // completion in sub state data +0x48) or returns to play (sub state 2).
    bool OnJobFinished();
    // The level starts over: main state 3 with fresh main state data.
    void RestartLevelState();
    // Ends the running job and goes back to the front end (main state 0,
    // sub state 7), hiding the letterbox.
    void QuitToFrontEnd();
    // After skipping a cinematic: the "Audio.PostCinematicSkipMusic" track
    // (or silence) and the state flags of a skipped cutscene.
    void OnCinematicSkipped();
    // After the save data loaded: extras not earned are locked again, the
    // job the save was made in is started again (or the level of its save
    // point asked for, or the first job), and the "Audio.PostSaveDataLoadMusic"
    // track starts.
    void ResumeSavedGame();
    // Notes in the save data where play would resume: inside a job its
    // episode and the episode's hideout; otherwise the running job and the
    // tracked goal too, unless this is the game's last goal (0x00475E20).
    void RecordResumePoint();
    // The running episode is the last active one, it has at most one job
    // left, and the player's goals are all complete but at most one, which
    // does not hold back the next scripts (0x00475C28).
    bool OnFinalGoal() const;
    // Autosave at the end of a job. After the game's last goal the episode
    // is marked finished, its job complete, play resumes in its hideout and
    // its end movies unlock (the third only with every trophy) (0x0048888C).
    void AutoSaveAfterJob();
    // Frees the level's parameters unless the level stays loaded (+0x1C
    // bits 6-7) (0x0047E1D4).
    void ReleaseLevelParams();
    // Back to play from the pause menu: the "resume" message goes to its
    // listeners, the minigame's slot 10 runs, the HUD comes back, the menu
    // closes, the audio groups and binocucoms resume and rumble follows the
    // vibration option again (0x0047DFC8).
    void ResumeFromPause();
    // Follows the audio rate ramp: every voice plays at its rate, and while
    // it is slowed the music's low-pass cut-off fades down to
    // "Audio.SlowTimeMusicLPFFrequency" (back up to 23999 Hz at normal
    // speed) over "Audio.SlowTimeMusicLPFCrossfadeDuration" (0x00468A38).
    void UpdateSlowTimeAudio(float dt);
    // Walks the levels `level` includes, recursively. NOTE: nothing is done
    // with them on the console (the work was compiled out) (0x0047C68C).
    void VisitIncludedLevels(const ape::ApeLevelDef* level);
    // A player is down: meter B snaps and a running return-treasure
    // microgame loses the treasure if the player carried it.
    void OnPlayerDown(uint32_t player);
    // The menus' yes/no answer: sub state data byte 0 set, answer in
    // state flag bit 0. NOTE: name from use.
    void AnswerYes();
    void AnswerNo();
    // Moves a player's character to a transform: entity and world object
    // transforms, state reset, and for characters the settle/restore-point
    // updates.
    void PlacePlayer(GamePlayer& player, const ape::Matrix34& transform);
    void PlacePlayer(uint32_t player, const ape::Matrix34& transform);
    // Aims the physics of a player's character at a point (characters only).
    void SetPlayerPhysicsTarget(uint32_t player, const ape::Vec3& point);
    // The power meter of a player on the HUD (second bar of meter B).
    void ShowPlayerPower(uint32_t player) const;

    // -- level requests, menus and prompts (SlyGameMenus.cpp) -------------------
    // Asks for level `level`: main state 1 (loading) through a transition,
    // sub states 0. `flag` goes to state flag bit 1. NOTE: the meaning of
    // that bit is not known; nearly every caller passes false.
    void RequestLevel(uint32_t level, bool flag);
    // Opens pause menu page `menu` (0x00470750; the page names are in the
    // table 0x00F65E70). `arg` is kept in +0x21.
    void OpenMenu(int menu, uint8_t arg);
    void OpenMenu(int menu);
    // Re-sends event 0x17 to the open menu. NOTE: name from use.
    void RefreshOpenMenu();
    // Whether a menu or a prompt (`forPrompt`) may open now: no time source,
    // no modal sub state, no updating HUD element, no cutscene, no job
    // transition, no blocking HUD element, (menus only) no screen fade, the
    // gadgets and the minigame allow it.
    bool CanOpenMenu(bool forPrompt) const;
    // Opens the pause menu when the active player presses start (pad
    // flags bit 14) and nothing blocks it. Always true.
    bool CheckPauseRequest();
    // Yes/no prompt for `player` (its pad becomes the active one): texts of
    // the prompt element's two fields, the answer callbacks with their user
    // data and two option flags. In game it opens as menu page 3; the front
    // end variant (ShowFrontEndPrompt) switches to sub state 7 then 0x1C.
    using PromptCallback = void (*)(void* user);
    bool ShowPrompt(entity::Entity* player, const char* text, const char* buttons, PromptCallback onYes,
                    void* yesUser, PromptCallback onNo, void* noUser, bool optionA, bool optionB);
    bool ShowFrontEndPrompt(entity::Entity* player, const char* text, const char* buttons, PromptCallback onYes,
                            void* yesUser, PromptCallback onNo, void* noUser, bool optionA, bool optionB);
    // Makes `player` the one whose pad drives the menus and opens menu
    // page 4 on the hideout menu's start page.
    void SetActivePlayer(uint8_t player);
    // After a prompt of the hideout menu: sub state 6 with its data byte 1
    // set, the menu GUI cleared.
    void ContinueAfterPrompt();
    // Sub state 2 (play), after telling the entity the travel dialog
    // targets (sub state data +0x5C) that it was declined.
    static void DeclineTravel();
    // Travel dialog accepted: ends the running job when the hideout of its
    // episode is the target, requests the level (sub state data +0x48), the
    // respawn point (+0x4C) and, when +0x58 is set, keeps the name of the
    // character's definition as the pending level name.
    static void AcceptTravel();
    // The front end volume value: option volume 3 scaled to 0..32768.
    // (declared above as ScaledVolume3)
    // Releases the entities the update borrowed for the split views
    // (+0x128610 records): hidden, released, killed when flagged.
    void ReleaseBorrowedEntities();
    // A player's HUD screen (`name`; the HUD keeps player 0's only), with
    // the contextual marker refreshed when `refreshMarker` is set.
    void ShowPlayerHudScreen(uint32_t player, const char* name, bool refreshMarker);
    // The contextual marker of player 0's character (its definition's
    // marker instance) is handed to every AI controller of type 0x1B.
    void RefreshContextualMarker(uint32_t player);
    // Every HUD element back to its start; player 0's gadget is put away.
    void ResetPlayerHud();
    // Meter B variant of a player; a character also tells its gadget
    // (variant on while it is not in the idle state 0/6 or a debug switch
    // forces it).
    void SetPlayerMeterVariant(uint32_t player, bool on);
    // The camera updates once more and re-applies its view; the player
    // camera mode (+0x128584) activates again.
    void RefreshCamera();
    // Throws or drops (`drop`) a player's character towards `target`.
    void ThrowPlayer(uint32_t player, const ape::Vec3& target, bool drop);
    // A player block switches to another character (0x00484BE0).
    void SwitchPlayerCharacter(GamePlayer& player, uint32_t def, const ape::Matrix34* transform, bool a,
                               bool b, bool c, bool d);
    void SwitchPlayerCharacter(uint32_t player, uint32_t def, const ape::Matrix34* transform, bool a, bool b,
                               bool c, bool d);
    // The camera follows a player block's character: the camera modes are
    // cleared and a mode for the character is pushed (vehicles pick theirs
    // from their definition; "Camera.DefaultOrbit" selects the orbit mode
    // for characters) (0x00484858).
    void AttachCameraTo(GamePlayer& player);
    void AttachCameraToPlayer(uint32_t player);
    // Modal message box (sub state 0x0C) with one text, or a series of text
    // pages; both pause the audio. The pages take two colours, two options
    // and can fade the screen. Only while "Game.MoviesEnabled" is set.
    void ShowMessageBox(const char* text, const int32_t position[2], const int32_t size[2]);
    void ShowTextPages(const char (*pages)[0x80], int count, const int32_t position[2], const int32_t size[2],
                       bool fade, bool optionA, const uint8_t colorA[4], const uint8_t colorB[4], bool optionB);
    // Starts the speaker dialog (sub state 8, the frozen screen): speakers
    // (at most six), text and sound keys, the caller's value, how long it
    // stays and an option bit; its mix state is pushed. False while one runs.
    bool StartSpeakerDialog(const DialogSpeakerEntry* speakers, uint32_t count, const char* text,
                            const char* sound, uint32_t value, float duration, bool option);
    // Cutscene mode of the player and the world: time scales back to 1, the
    // player's gadget away and targeting off, AI sightings cleared, the
    // characters hidden, the subtitle cleared and the camera's depth of
    // field off (all reversed when `on` is false).
    void SetCutsceneMode(bool on);
    // Cutscene start/end for the HUD: cutscene mode, the return-treasure
    // microgame paused, the HUD reset (start) or re-enabled with the
    // letterbox shut down (end).
    void SetCutsceneHud(bool on);
    // Runs the queued cutscenes in order (each waits for the one before it
    // and for its sequence); finished ones leave the queue and, once it is
    // empty, the deferred game events go to the job manager and play
    // resumes (0x0048B810).
    void UpdateCutsceneQueue(float dt);
    // One step of the level world (only while it exists; a zero step counts
    // as 1/60 s): file streaming, the HUD, callback list 0 (emptied before
    // and after), the instance render lists, callback list 12 and the
    // world's own update (0x0047C2B8).
    void UpdateWorld(float dt);
    // Drops every queued cutscene (a modal box over one is told it ended,
    // the main view gets its near plane back) and clears the queue flags
    // (0x0047E3CC).
    void ClearCutsceneQueue();
    // Queues script `script` of library `library` as a cutscene (at most
    // four; only while "Game.CutSceneEnable" is set) with its variable
    // bindings and their source instance. `resume` (or a failing goal)
    // resumes play when the queue empties; `playing` marks a cutscene as
    // playing at once (0x0046C3B8).
    void QueueCutscene(uint32_t script, uint32_t library, bool flag, const TextField* bindings, size_t count,
                       int32_t value, ape::ApeAnimScriptInstance* source, bool resume, bool becomeSly, bool playing);
    // Pops every camera mode (followed targets dropped), deleting the
    // player camera mode (0x004845F0). NOTE: the console passes a player
    // block it does not use.
    void ClearCameraModes();
    // Leaves for the hideout of the current episode (ending the running
    // job), or for the level's default hideout outside episodes.
    static void ReturnToHideout();
    // Ends the dialog (the frozen screen with a speaker, +0x1283C0) when one
    // is up: sub state 2 (play), or 0x0B with `toFrontEnd`; its mix state
    // fades out and the block is cleared.
    void EndDialog(bool toFrontEnd);
    // The screen shows a held picture instead of the running game.
    bool HoldsFrame();
    // Option menu pages: the labels and sliders of a page's "Menu" object
    // show the current settings (SlyGameOptionPages.cpp).
    void RefreshControlsPage(ape::ApeGuiObject* screen);
    void RefreshAudioPage(ape::ApeGuiObject* screen);
    void RefreshDisplayPage(ape::ApeGuiObject* screen);
    // Accept on the controls page: the camera X/Y inversion or vibration
    // option of the chosen object toggles, the toggle sound plays and the
    // page is asked to refresh (GUI event 0x10).
    void OnControlsPageAccept(const ape::GuiEvent& event);
    // Moves page: left/right (GUI events 11/10) switch the move category
    // (wrapping over four), the page refreshes and the focus moves to the
    // first focusable object near the chosen one, the pointers with it.
    void OnMovesPageEvent(const ape::GuiEvent& event);
    // Audio page: left/right (GUI events 11/10) on a slider moves its volume
    // by one "UI.Slider.NumSteps" step (the effect/voice/music scales follow
    // at once), on a choice cycles the speaker format, centre speaker size or
    // dynamic range; the page refreshes.
    void OnAudioPageEvent(const ape::GuiEvent& event);
    // Shows the running level's headline (see SlyGameMenus.cpp).
    void ShowLevelHeadline(bool force);
    // Marks the running level visited for the all-levels trophy.
    void RecordLevelVisit();
    // Full screen vertical colour gradient behind everything.
    void DrawScreenGradient(agl::AglRenderTarget* view, const uint8_t top[4], const uint8_t bottom[4]);

    // -- data ------------------------------------------------------------------
    uint8_t flags1C = 0;                        // +0x1C bit 5: autosave allowed
    uint8_t localPlayerCount = 1;               // +0x1D
    uint8_t activePlayer = 0xFF;                // +0x1E player whose pad drives the menus
    uint8_t menu = 0;                           // +0x1F open pause menu page (0: none)
    uint8_t field_20 = 0;                       // +0x20 cleared when a menu page opens
    uint8_t menuArg = 0;                        // +0x21 argument of the open page
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
    // +0x50..+0x1030: the save data ("save.dat"): the profile's progress
    // record (indexed by players[0].index on the console, stride 0xFD8) and
    // its pending counters.
    game::PlayerProgress progress;              // +0x50
    uint8_t pendingCounts[8] = {};              // +0x1028
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
    ape::ApeLevelWorld* levelWorld = nullptr;   // +0x1281C4 the level's scene and cameras
    uint32_t field_1281C8[3] = {};              // +0x1281C8
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
    bool cutsceneSkipRequested = false;         // +0x128222 NOTE: set when the first cutscene is queued (the HUD switches over)
    // +0x128224 NOTE: an object that overrides the frame time (+0x3C) and
    // tracks an entity (+0x158..+0x160); class unknown.
    struct TimeSource {
        uint8_t field_00[0x3C];
        float deltaTime;                        // +0x3C
        uint8_t field_40[0x118];
        uint32_t field_158;                     // +0x158
        entity::Entity* trackedEntity;          // +0x15C
        bool tracking;                          // +0x160 bit 31
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
    gameplay::ReturnTreasureMicrogame* returnTreasure = nullptr; // +0x12847C
    ape::ApeTArray<WorldCallback> callbackLists[kCallbackListCount]; // +0x128480
    camera::CameraMode* playerCameraMode = nullptr; // +0x128584 the camera mode following the player (owned)
    float timeScaleA = 1.0f;                    // +0x128588
    float timeScaleATarget = -1.0f;             // +0x12858C
    uint8_t field_128590 = 0;                   // +0x128590
    float timeScaleB = 1.0f;                    // +0x128594
    float timeScaleBTarget = -1.0f;             // +0x128598
    uint8_t field_12859C = 0;                   // +0x12859C
    uint32_t hudScreenHash = 0;                 // +0x1285A0 HUD screen of the current sub state
    uint8_t field_1285A4 = 0;                   // +0x1285A4
    bool cutsceneHudLocked = false;             // +0x1285A5 NOTE: cutscene HUD changes are skipped while set
    uint32_t finishedJobName = 0;               // +0x1285A8 name of the job that ended last
    ape::ApeGuiObject* menuFocus = nullptr;     // +0x1285AC GUI object with the menu focus
    uint32_t field_1285B0[4] = {};              // +0x1285B0
    int32_t menuFocusPad = -1;                  // +0x1285C0 pad that has the menu focus
    uint8_t field_1285C4 = 0;                   // +0x1285C4
    uint8_t movesCategory = 0;                  // +0x1285C5 moves page: category (0..3)
    uint8_t movesPage = 0;                      // +0x1285C6 moves page: page of five moves
    uint8_t field_1285C7 = 0;                   // +0x1285C7
    uint8_t field_1285C8[2] = {};               // +0x1285C8
    uint32_t field_1285CC[6] = {};              // +0x1285CC
    void* field_1285E4 = nullptr;               // +0x1285E4
    uint32_t field_1285EC = 0;                  // +0x1285EC
    uint32_t pauseSound = 0;                    // +0x1285F0
    uint32_t resumeSound = 0;                   // +0x1285F4
    audio::SoundName* toggleSound = nullptr;    // +0x128604 menu option toggle sound
    audio::SoundName* sliderSound = nullptr;    // +0x128608 a slider moved
    audio::SoundName* sliderStopSound = nullptr; // +0x12860C a slider is already at its end
    // Entities borrowed for the split views (+0x128610 count, 0x24-byte
    // records from +0x128614 holding up to four entities at +0x14).
    // NOTE: the rest of a record is not read by the game shell.
    struct BorrowedEntities {
        uint8_t field_00[0x14];
        entity::Entity* entities[4];
    };
    uint8_t borrowedCount = 0;                  // +0x128610
    BorrowedEntities borrowed[4] = {};          // +0x128614 NOTE: capacity unknown
    void* frameObject = nullptr;                // +0x1286A4 updated first every frame (slot 2)

    SpawnManager* spawnManager = nullptr;       // NOTE: owner of the spawner groups; slot unknown
};

// The single game world (0x00F65CA0).
extern SlyGame* g_game;
// Set at start-up when the pre-order unlock key file is present (0x010BB849;
// 0x00467E68 reads "/Preorder/UnlockKey.txt.edat").
extern bool g_preorderUnlocked;

// The extras ("$EXTRA_PARAGLIDER_MONKEY" ... "$EXTRA_COLE_AMP", table
// 0x00AE6A60): each is earned with a number of masks (5, 10, ... 60); the
// pre-order ones (0, 1 and 4) come with the unlock key too.
constexpr int kExtraCount = 8;
struct ExtraDef {
    uint32_t masksNeeded;
    const char* name;
    const char* desc;
};
extern const ExtraDef kExtras[kExtraCount];
// Extra `index` is earned with `masks` masks of `needed` (0x0046C7E0).
bool ExtraEarned(int index, uint32_t masks, uint32_t needed);
// GUI of the active hideout menu (its +0x154), or null (0x0046E760).
ape::ApeGuiObject* ActiveHideoutMenuGui();
// The "Common.Fade" GUI object the screen fade drives (0x00F65C88).
extern ape::ApeGuiObject* g_fadeObject;

// GUI helpers of the menus (0x004704A8, 0x0046E7B0).
void ShowLanguageMenu(ape::ApeGuiObject* screen, bool visible);
ape::ApeGuiObject* FindFocusableGuiObject(ape::ApeGuiObject* object);
// The HUD uses its corrected (SD 4:3) layouts (0x0046C7B0).
bool IsSdHudLayout();
// The dark screen settings ("Game.DarkScreen.*") (0x0048C6EC).
void SetDarkScreen(float distance, float ramp, float minValue, bool enabled);
// The trophy communication id ("NPWR03581").
const char* TrophyCommId();
// The player character plays a script (its animation driver component is
// in state 7, "playing script").
bool PlayerPlayingScript();
// Runs a script of the default library once, on its own (0x00491000).
void RunGlobalScript(uint32_t scriptHash);
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
