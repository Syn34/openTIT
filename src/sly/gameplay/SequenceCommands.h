// openTIT - Sequencer commands (see Sequencer.h).
//
// Each command is created by the script bindings, queued with
// AddSequenceCommand() and deleted by its sequence once it has finished.
// Commands that name an entity look it up in the sequence entity table at
// Execute() time, so the entity has to be registered first ("SetEntity").
// The console vtable of each class is given in its comment.
#pragma once

#include "sly/gameplay/GameplayWorld.h"
#include "sly/gameplay/Sequencer.h"

#include <cstdint>
#include <vector>

namespace sly {
namespace audio { struct MixState; struct SoundName; struct SoundRequest; }

namespace gameplay {

// Pushes or removes an audio mix state (0x00B20CE0).
class SeqMixStateCommand : public SequenceCommand {
public:
    SeqMixStateCommand(float blendTime, bool push, const audio::MixState* state);
    void Execute() override;

private:
    const audio::MixState* m_state = nullptr;   // +0x0C
    bool m_push = false;                        // +0x10
    float m_blendTime = 0.0f;                   // +0x14
};

// Posts a script message to every listener registered under a key
// (0x00B20D10).
class SeqPostMessageCommand : public SequenceCommand {
public:
    SeqPostMessageCommand(uint32_t listenerKey, uint32_t message, uint32_t argument);
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_listenerKey = 0;   // +0x0C
    uint32_t m_message = 0;       // +0x10
    uint32_t m_argument = 0;      // +0x14
};

// Starts a camera shake, optionally centred on an entity, and waits until
// no shake is active any more (0x00B20D58).
class SeqCameraShakeCommand : public SequenceCommand {
public:
    SeqCameraShakeCommand(uint32_t entityHash, const CameraShakeParams& params, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_entityHash = 0;    // +0x0C
    CameraShakeParams m_params;   // +0x10
};

// Attaches one sequence entity to another (0x00B20DA0).
class SeqAttachCommand : public SequenceCommand {
public:
    SeqAttachCommand(uint32_t childHash, uint32_t parentHash);
    void Execute() override;

private:
    WorldObject* m_child = nullptr;   // +0x0C
    uint32_t m_childHash = 0;         // +0x10
    WorldObject* m_parent = nullptr;  // +0x14
    uint32_t m_parentHash = 0;        // +0x18
};

// Enables or disables a component of a sequence entity (0x00B20DB8).
class SeqEnableComponentCommand : public SequenceCommand {
public:
    SeqEnableComponentCommand(uint32_t entityHash, uint32_t componentHash, bool enable);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;      // +0x0C
    uint32_t m_componentHash = 0;   // +0x10
    bool m_enable = false;          // +0x14
};

// Fades the screen over time (0x00B20DD0).
class SeqScreenFadeCommand : public SequenceCommand {
public:
    SeqScreenFadeCommand(float red, float green, float target, float duration, bool fadeOut);
    void Execute() override;
    bool Update(float dt) override;

private:
    bool m_fadeOut = false;       // +0x09
    float m_red = 0.0f;           // +0x0C
    float m_green = 0.0f;         // +0x10
    float m_target = 0.0f;        // +0x14
    float m_start = 1.0f;         // +0x18
    float m_duration = 0.0f;      // +0x1C
    float m_elapsed = 0.0f;       // +0x20
};

// Enables or disables the physics of a character (0x00B20DE8).
class SeqEnablePhysicsCommand : public SequenceCommand {
public:
    SeqEnablePhysicsCommand(uint32_t entityHash, bool enable, bool alsoBody);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    bool m_enable = false;       // +0x10
    bool m_alsoBody = false;     // +0x11
};

// Ends the sequence the command belongs to (0x00B20E48).
class SeqEndSequenceCommand : public SequenceCommand {
public:
    SeqEndSequenceCommand();
    void Execute() override;
};

// Plays an animation on a sequence entity (0x00B20E60).
class SeqPlayAnimationCommand : public SequenceCommand {
public:
    SeqPlayAnimationCommand(uint32_t entityHash, uint32_t animHash, bool loop, bool hideFirst);
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    uint32_t m_animHash = 0;     // +0x10
    bool m_loop = false;         // +0x14
    bool m_hideFirst = false;    // +0x15
};

// Mutes or unmutes the sounds of a sequence entity (0x00B20E78).
class SeqSuppressAudioCommand : public SequenceCommand {
public:
    SeqSuppressAudioCommand(uint32_t entityHash, bool suppress);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    bool m_suppress = false;     // +0x10
};

// Queues a music/dialog stream and optionally waits until it buffers
// (0x00B20E90, "SeqEnqueueStream").
class SeqEnqueueStreamCommand : public SequenceCommand {
public:
    SeqEnqueueStreamCommand(audio::SoundName* sound, uint32_t param10, uint32_t param14,
                            bool forceNew, bool blocking, bool queueNow);
    ~SeqEnqueueStreamCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    audio::SoundName* m_sound = nullptr;        // +0x0C (one reference held)
    uint32_t m_param10 = 0;                     // +0x10 NOTE: unused by the command itself
    uint32_t m_param14 = 0;                     // +0x14 NOTE: unused by the command itself
    bool m_forceNew = false;                    // +0x18
    audio::SoundRequest* m_request = nullptr;   // +0x1C
};

// Starts a colour filter and waits for it to finish (0x00B20EA8).
class SeqColorFilterCommand : public SequenceCommand {
public:
    SeqColorFilterCommand(float red, float green, float blue, uint32_t mode, bool hold,
                          bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    float m_red = 0.0f;      // +0x0C
    float m_green = 0.0f;    // +0x10
    float m_blue = 0.0f;     // +0x14
    uint32_t m_mode = 0;     // +0x18
    bool m_hold = false;     // +0x1C
};

// Blends the depth of field (0x00B20EC0).
class SeqDepthOfFieldCommand : public SequenceCommand {
public:
    SeqDepthOfFieldCommand(float nearStart, float nearEnd, float farStart, float farEnd,
                           bool both, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    float m_values[4] = {};   // +0x0C
    bool m_both = false;      // +0x1C
};

// Pulls the focus to a sequence entity (0x00B20ED8).
class SeqFocusPullCommand : public SequenceCommand {
public:
    SeqFocusPullCommand(const float params[7], uint32_t targetHash, uint32_t mode,
                        bool flag30, bool flag31, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_targetHash = 0;   // +0x0C
    float m_params[7] = {};      // +0x10
    uint32_t m_mode = 0;         // +0x2C
    bool m_flag30 = false;       // +0x30
    bool m_flag31 = false;       // +0x31
};

// Starts a camera effect and waits while it runs (0x00B20F08).
class SeqCameraEffectCommand : public SequenceCommand {
public:
    SeqCameraEffectCommand(const CameraEffectParams& params, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    CameraEffectParams m_params;   // +0x0C (0x74 bytes)
    uint32_t m_handle = 0;         // +0x80
};

// Sets the ambient tint (0x00B20F20).
class SeqAmbientTintCommand : public SequenceCommand {
public:
    SeqAmbientTintCommand(float red, float green, float blue);
    void Execute() override;

private:
    float m_red = 0.0f;     // +0x0C
    float m_green = 0.0f;   // +0x10
    float m_blue = 0.0f;    // +0x14
};

// Removes a sequence entity from the world (0x00B20F50).
class SeqRemoveEntityCommand : public SequenceCommand {
public:
    SeqRemoveEntityCommand(uint32_t entityHash, bool playDespawn);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    bool m_playDespawn = false;  // +0x10
};

// Waits for a number of seconds (0x00B20FC8).
class SeqWaitCommand : public SequenceCommand {
public:
    explicit SeqWaitCommand(float duration);
    void Execute() override;
    bool Update(float dt) override;

private:
    float m_duration = 0.0f;   // +0x0C
    float m_elapsed = 0.0f;    // +0x10
};

// Game world flag toggles (0x00B20FE0, 0x00B20FF8, 0x00B21010, 0x00B21028).
class SeqHideHudCommand : public SequenceCommand {
public:
    explicit SeqHideHudCommand(bool hide);
    void Execute() override;

private:
    bool m_value = false;   // +0x09
};

class SeqShowHudCommand : public SequenceCommand {
public:
    explicit SeqShowHudCommand(bool show);
    void Execute() override;

private:
    bool m_value = false;   // +0x09
};

class SeqLockInputCommand : public SequenceCommand {
public:
    explicit SeqLockInputCommand(bool lock);
    void Execute() override;

private:
    bool m_value = false;   // +0x09
};

class SeqHudCounterCommand : public SequenceCommand {
public:
    explicit SeqHudCounterCommand(bool visible);
    void Execute() override;

private:
    bool m_value = false;   // +0x09
};

// Shows a message box and waits while a modal dialog is open (0x00B210B8).
class SeqMessageBoxCommand : public SequenceCommand {
public:
    SeqMessageBoxCommand(const char* text, const int32_t pos[2], const int32_t size[2],
                         bool blocking);
    ~SeqMessageBoxCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    char m_text[0x80] = {};      // +0x09
    int32_t m_pos[2] = {};       // +0x8C
    int32_t m_size[2] = {};      // +0x94
};

// Ramps the exposure between two colours (0x00B210E8).
class SeqExposureRampCommand : public SequenceCommand {
public:
    SeqExposureRampCommand(float time, const uint8_t from[4], const uint8_t to[4], bool hold,
                           bool blocking);
    ~SeqExposureRampCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    uint8_t m_from[4] = {};   // +0x09
    uint8_t m_to[4] = {};     // +0x0D
    float m_time = 0.0f;      // +0x14
    bool m_hold = false;      // +0x18 bit 31
};

// Waits for a button press, optionally posting a message after a timeout
// (0x00B21118).
class SeqWaitForButtonCommand : public SequenceCommand {
public:
    SeqWaitForButtonCommand(float timeout, int player, uint32_t buttonMask,
                            uint32_t listenerKey, uint32_t message);
    void Execute() override;
    bool Update(float dt) override;

private:
    int m_player = 0;              // +0x0C
    uint32_t m_buttonMask = 0;     // +0x10
    uint32_t m_listenerKey = 0;    // +0x14
    uint32_t m_message = 0;        // +0x18
    float m_timeout = 0.0f;        // +0x1C
    float m_elapsed = 0.0f;        // +0x20
    bool m_timedOut = false;       // +0x24
};

// Asks the player to confirm a transition and performs it on "yes"
// (0x00B21130).
class SeqTransitionPromptCommand : public SequenceCommand {
public:
    explicit SeqTransitionPromptCommand(uint32_t target);
    void Execute() override;
    bool Update(float dt) override;

private:
    static void OnYes(void* user);
    static void OnNo(void* user);

    uint32_t m_target = 0;   // +0x0C
    bool m_done = false;     // +0x10
};

// Starts the scripted mover of an entity and waits for it (0x00B211A8).
class SeqMoveEntityCommand : public SequenceCommand {
public:
    SeqMoveEntityCommand(float time, uint32_t entityHash, uint32_t a, uint32_t b, uint32_t c,
                         bool quiet, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_entityHash = 0;      // +0x0C (0xFD81 = the camera)
    uint32_t m_a = 0;               // +0x10
    uint32_t m_b = 0;               // +0x14
    uint32_t m_c = 0;               // +0x18
    float m_time = 0.0f;            // +0x1C
    WorldObject* m_entity = nullptr;// +0x20
    bool m_quiet = false;           // +0x24
};

// Enables updating of every scripted-controller entity (0x00B211C0).
class SeqEnableScriptedEntitiesCommand : public SequenceCommand {
public:
    explicit SeqEnableScriptedEntitiesCommand(bool enable);
    void Execute() override;

private:
    bool m_enable = false;   // +0x09
};

// Sends reset requests to the active minigame (0x00B211D8).
class SeqMinigameControlCommand : public SequenceCommand {
public:
    SeqMinigameControlCommand(bool resetAll, bool resetScore, bool resetTimer, bool showIntro,
                              bool startPlayer, bool resetRound);
    void Execute() override;

private:
    bool m_resetAll = false;     // +0x08 bit 23
    bool m_resetScore = false;   // bit 22
    bool m_resetTimer = false;   // bit 21
    bool m_showIntro = false;    // bit 20
    bool m_startPlayer = false;  // bit 19
    bool m_resetRound = false;   // bit 18
};

// Sets the HUD timer (0x00B21220).
class SeqHudTimerCommand : public SequenceCommand {
public:
    explicit SeqHudTimerCommand(float seconds);
    void Execute() override;

private:
    float m_seconds = 0.0f;   // +0x0C
};

// Game world byte flags (0x00B21238 cutscene, 0x00B21250 player pause,
// 0x00B21268 AI pause).
class SeqCutsceneFlagCommand : public SequenceCommand {
public:
    explicit SeqCutsceneFlagCommand(bool value);
    void Execute() override;
    bool Update(float dt) override;

private:
    bool m_value = false;   // +0x09
};

class SeqPausePlayerCommand : public SequenceCommand {
public:
    explicit SeqPausePlayerCommand(bool value);
    void Execute() override;
    bool Update(float dt) override;

private:
    bool m_value = false;   // +0x09
};

class SeqPauseAiCommand : public SequenceCommand {
public:
    explicit SeqPauseAiCommand(bool value);
    void Execute() override;
    bool Update(float dt) override;

private:
    bool m_value = false;   // +0x09
};

// Registers a world entity with the sequence ("SetEntity", 0x00B21280).
class SeqSetEntityCommand : public SequenceCommand {
public:
    SeqSetEntityCommand(WorldObject* entity, uint32_t alias, bool show, bool hideOnRelease,
                        bool keepShownState, bool resetPose, bool neverShow, bool keepAnimSet,
                        bool clearAiState, bool transferIfRegistered);
    void Execute() override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C (one reference held)
    uint32_t m_alias = 0;              // +0x10
    bool m_show = false;               // +0x14
    bool m_hideOnRelease = false;      // +0x15
    bool m_keepShownState = false;     // +0x16
    bool m_resetPose = false;          // +0x17
    bool m_neverShow = false;          // +0x18
    bool m_keepAnimSet = false;        // +0x19
    bool m_clearAiState = false;       // +0x1A
    bool m_transfer = false;           // +0x1B
};

// Sets a parameter of an entity's particle emitter (0x00B21298).
class SeqEmitterParamCommand : public SequenceCommand {
public:
    SeqEmitterParamCommand(uint32_t entityHash, uint32_t value);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    uint32_t m_value = 0;        // +0x10
};

// Keeps a stream name referenced and starts/stops music (0x00B212C8).
class SeqMusicCommand : public SequenceCommand {
public:
    SeqMusicCommand(audio::SoundName* sound, bool play, bool flag11);
    ~SeqMusicCommand() override;
    void Execute() override;

private:
    audio::SoundName* m_sound = nullptr;   // +0x0C (one reference held)
    bool m_play = false;         // +0x10
    bool m_flag11 = false;       // +0x11 NOTE: not used by Execute
};

// Shows, hides or toggles parts of a sequence entity (0x00B212E0).
class SeqSetEntityVisibleCommand : public SequenceCommand {
public:
    SeqSetEntityVisibleCommand(uint32_t entityHash, uint32_t childHash, bool visible,
                               bool shadows, bool children, bool effects, bool optional);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    uint32_t m_childHash = 0;    // +0x10
    bool m_visible = false;      // +0x14 bit 31
    bool m_shadows = false;      // bit 30
    bool m_children = false;     // bit 29
    bool m_effects = false;      // bit 28
    bool m_optional = false;     // bit 27: no warning if missing
};

// Turns per-frame updating of a sequence entity on or off (0x00B212F8).
class SeqSetEntityUpdateCommand : public SequenceCommand {
public:
    SeqSetEntityUpdateCommand(uint32_t entityHash, bool update);
    void Execute() override;

private:
    uint32_t m_entityHash = 0;   // +0x0C
    bool m_update = false;       // +0x10 bit 31
};

// Plays an animation on an entity without waiting (0x00B21340).
class SeqStartAnimationCommand : public SequenceCommand {
public:
    SeqStartAnimationCommand(uint32_t entityHash, uint32_t animHash, bool loop);
    void Execute() override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_animHash = 0;           // +0x14
    bool m_loop = false;               // +0x18 bit 31
};

// Sets the gameplay time scale (0x00B21358).
class SeqTimeScaleCommand : public SequenceCommand {
public:
    SeqTimeScaleCommand(int kind, float scale);
    void Execute() override;

private:
    int m_kind = 0;         // +0x0C only kind 1 is handled
    float m_scale = 0.0f;   // +0x10
};

// Shows the level headline (0x00B21370).
class SeqLevelHeadlineCommand : public SequenceCommand {
public:
    SeqLevelHeadlineCommand();
    void Execute() override;
};

// Starts another (already built) sequence (0x00B213A0).
class SeqStartSequenceCommand : public SequenceCommand {
public:
    explicit SeqStartSequenceCommand(uint32_t sequenceHash);
    void Execute() override;

private:
    uint32_t m_sequenceHash = 0;   // +0x0C
};

// Stops sounds: a tracked voice, a category or everything (0x00B213B8).
class SeqStopSoundCommand : public SequenceCommand {
public:
    SeqStopSoundCommand(uint32_t categoryHash, uint32_t trackKey, bool force);
    void Execute() override;

private:
    uint32_t m_categoryHash = 0;   // +0x0C
    uint32_t m_trackKey = 0;       // +0x10
    bool m_force = false;          // +0x14
};

// Stops the music (0x00B213D0).
class SeqStopMusicCommand : public SequenceCommand {
public:
    explicit SeqStopMusicCommand(bool release);
    void Execute() override;

private:
    bool m_release = false;   // +0x09
};

// Waits until none of the listed sequences has queued commands (0x00B21400).
class SeqWaitForSequencesCommand : public SequenceCommand {
public:
    explicit SeqWaitForSequencesCommand(const std::vector<uint32_t>& sequenceHashes);
    bool Update(float dt) override;

private:
    std::vector<uint32_t> m_sequenceHashes;   // +0x0C
};

// Waits until a sound category (or a tracked voice) is silent (0x00B21418).
class SeqWaitForSoundCommand : public SequenceCommand {
public:
    SeqWaitForSoundCommand(uint32_t categoryHash, uint32_t trackKey);
    bool Update(float dt) override;

private:
    int m_category = 0;      // +0x0C
    uint32_t m_trackKey = 0; // +0x10
};

} // namespace gameplay
} // namespace sly
