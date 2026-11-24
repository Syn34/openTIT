// openTIT - Sequencer commands (see SequenceCommands.h).
#include "sly/gameplay/SequenceCommands.h"

#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"
#include "sly/gameplay/Minigame.h"

#include <cmath>
#include <cstring>

namespace sly {
namespace gameplay {

namespace {

// Name hash used by commands that address the camera instead of an entity.
constexpr uint32_t kCameraEntityHash = 0xFD81;

// Localisation keys of the transition prompt.
constexpr const char* kTransitionPromptText = "$STR_CONTROL_OPTIONS_DLG";
constexpr const char* kYesNoButtons = "$BUTTON_X $STR_YES $BUTTON_CIRCLE $STR_NO";

// Shares a request that is already playing the sound, or queues a new one
// for immediate playback.
audio::SoundRequest* FindOrRequestSound(audio::SoundName* sound, bool forceNew)
{
    audio::SoundQueue& queue = audio::Requests();
    audio::SoundRequest* request = queue.FindPlaying(&sound);
    if (request == nullptr || forceNew) {
        audio::SoundParams params;
        request = queue.RequestImmediate(sound, &params);
        // Destruction of the temporary parameter block.
        audio::SetRequestBusA(params, nullptr);
        audio::SetRequestBusB(params, nullptr);
    }
    return request;
}

void CopyName(char* dst, size_t dstSize, const char* src)
{
    if (src != nullptr && src[0] != '\0')
        ape::StrCopyBounded(dst, src, dstSize);
}

} // namespace

// ---------------------------------------------------------------------------
// SeqMixStateCommand
// ---------------------------------------------------------------------------

// @func 0x0042CC10 SeqMixStateCommand::SeqMixStateCommand
SeqMixStateCommand::SeqMixStateCommand(float blendTime, bool push, const audio::MixState* state)
    : SequenceCommand(true), m_state(state), m_push(push), m_blendTime(blendTime)
{
}

// @func 0x0077F6D0 SeqMixStateCommand::~SeqMixStateCommand
// @func 0x0077F700 SeqMixStateCommand::~SeqMixStateCommand (deleting)
SeqMixStateCommand::~SeqMixStateCommand() = default;

// @func 0x0042CC80 SeqMixStateCommand::Execute
void SeqMixStateCommand::Execute()
{
    // NOTE: the command never marks itself as started; it is deleted right
    // after this call because Update() reports completion at once.
    if (m_push)
        audio::PushMixState(m_state, m_blendTime);
    else
        audio::RemoveMixState(m_state, m_blendTime);
}

// ---------------------------------------------------------------------------
// SeqPostMessageCommand
// ---------------------------------------------------------------------------

// @func 0x0042D13C SeqPostMessageCommand::SeqPostMessageCommand
SeqPostMessageCommand::SeqPostMessageCommand(uint32_t listenerKey, uint32_t message,
                                             uint32_t argument)
    : SequenceCommand(true), m_listenerKey(listenerKey), m_message(message), m_argument(argument)
{
}

// @func 0x0077F7E8 SeqPostMessageCommand::~SeqPostMessageCommand
// @func 0x0077F818 SeqPostMessageCommand::~SeqPostMessageCommand (deleting)
SeqPostMessageCommand::~SeqPostMessageCommand() = default;

// @func 0x0042D1AC SeqPostMessageCommand::Execute
void SeqPostMessageCommand::Execute()
{
    if (m_message != 0)
        PostScriptMessage(nullptr, m_listenerKey, m_message, m_argument);
    MarkStarted();
}

// @func 0x0042D204 SeqPostMessageCommand::Update
bool SeqPostMessageCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// SeqCameraShakeCommand
// ---------------------------------------------------------------------------

// @func 0x0042D72C SeqCameraShakeCommand::SeqCameraShakeCommand
SeqCameraShakeCommand::SeqCameraShakeCommand(uint32_t entityHash, const CameraShakeParams& params,
                                             bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_params(params)
{
    // NOTE: the original copies the second value into the third slot as
    // well (the third value of the caller is dropped); kept as is.
    m_params.values[2] = params.values[1];
}

// @func 0x0077F988 SeqCameraShakeCommand::~SeqCameraShakeCommand
// @func 0x0077F9B8 SeqCameraShakeCommand::~SeqCameraShakeCommand (deleting)
SeqCameraShakeCommand::~SeqCameraShakeCommand() = default;

// @func 0x0042D814 SeqCameraShakeCommand::Execute
void SeqCameraShakeCommand::Execute()
{
    if (WorldObject* entity = FindSequenceEntity(m_entityHash))
        m_params.position = EntityPosition(entity);
    StartCameraShake(m_params);
    MarkStarted();
}

// @func 0x0042D89C SeqCameraShakeCommand::Update
bool SeqCameraShakeCommand::Update(float /*dt*/)
{
    return !AnyCameraShakeActive();
}

// ---------------------------------------------------------------------------
// SeqAttachCommand
// ---------------------------------------------------------------------------

// @func 0x0042DDDC SeqAttachCommand::SeqAttachCommand
SeqAttachCommand::SeqAttachCommand(uint32_t childHash, uint32_t parentHash)
    : SequenceCommand(true), m_childHash(childHash), m_parentHash(parentHash)
{
}

// @func 0x0077FB28 SeqAttachCommand::~SeqAttachCommand
// @func 0x0077FB58 SeqAttachCommand::~SeqAttachCommand (deleting)
SeqAttachCommand::~SeqAttachCommand() = default;

// @func 0x0042DE54 SeqAttachCommand::Execute
void SeqAttachCommand::Execute()
{
    m_child = FindSequenceEntity(m_childHash);
    if (m_child != nullptr) {
        m_parent = FindSequenceEntity(m_parentHash);
        if (m_parent != nullptr)
            EntityAttach(m_child, m_parent);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqEnableComponentCommand
// ---------------------------------------------------------------------------

// @func 0x0042DED0 SeqEnableComponentCommand::SeqEnableComponentCommand
SeqEnableComponentCommand::SeqEnableComponentCommand(uint32_t entityHash, uint32_t componentHash,
                                                     bool enable)
    : SequenceCommand(true), m_entityHash(entityHash), m_componentHash(componentHash),
      m_enable(enable)
{
}

// @func 0x0077FBB8 SeqEnableComponentCommand::~SeqEnableComponentCommand
// @func 0x0077FBE8 SeqEnableComponentCommand::~SeqEnableComponentCommand (deleting)
SeqEnableComponentCommand::~SeqEnableComponentCommand() = default;

// @func 0x0042DF40 SeqEnableComponentCommand::Execute
void SeqEnableComponentCommand::Execute()
{
    // A missing entity or component only produced a (compiled out) warning.
    if (WorldObject* entity = FindSequenceEntity(m_entityHash))
        EntityEnableComponent(entity, m_componentHash, m_enable);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqScreenFadeCommand
// ---------------------------------------------------------------------------

// @func 0x0042E040 SeqScreenFadeCommand::SeqScreenFadeCommand
SeqScreenFadeCommand::SeqScreenFadeCommand(float red, float green, float target, float duration,
                                           bool fadeOut)
    : SequenceCommand(true), m_fadeOut(fadeOut), m_red(red), m_green(green), m_target(target),
      m_start(1.0f), m_duration(duration), m_elapsed(0.0f)
{
}

// @func 0x0077FC48 SeqScreenFadeCommand::~SeqScreenFadeCommand
// @func 0x0077FC78 SeqScreenFadeCommand::~SeqScreenFadeCommand (deleting)
SeqScreenFadeCommand::~SeqScreenFadeCommand() = default;

// @func 0x0042E0E0 SeqScreenFadeCommand::Execute
void SeqScreenFadeCommand::Execute()
{
    m_elapsed = 0.0f;
    m_start = CurrentScreenFade();
    if (!m_fadeOut)
        m_target = 1.0f;
    MarkStarted();
}

// @func 0x0042E140 SeqScreenFadeCommand::Update
bool SeqScreenFadeCommand::Update(float dt)
{
    m_elapsed += dt;
    if (m_elapsed >= m_duration) {
        SetScreenFade(m_red, m_green, m_target, m_fadeOut);
        return true;
    }

    const float t = m_elapsed / m_duration;
    const float range = std::fabs(m_target - m_start);
    const float level = m_fadeOut ? 1.0f - t * range : t * range + m_start;
    SetScreenFade(m_red, m_green, level, true);
    return false;
}

// ---------------------------------------------------------------------------
// SeqEnablePhysicsCommand
// ---------------------------------------------------------------------------

// @func 0x0042E20C SeqEnablePhysicsCommand::SeqEnablePhysicsCommand
SeqEnablePhysicsCommand::SeqEnablePhysicsCommand(uint32_t entityHash, bool enable, bool alsoBody)
    : SequenceCommand(true), m_entityHash(entityHash), m_enable(enable), m_alsoBody(alsoBody)
{
}

// @func 0x0077FCD0 SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand
// @func 0x0077FD00 SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand (deleting)
SeqEnablePhysicsCommand::~SeqEnablePhysicsCommand() = default;

// @func 0x0042E27C SeqEnablePhysicsCommand::Execute
void SeqEnablePhysicsCommand::Execute()
{
    WorldObject* entity = FindSequenceEntity(m_entityHash);
    // Only characters have physics; anything else was a warning.
    if (entity != nullptr && EntityIsCharacter(entity))
        CharacterEnablePhysics(entity, m_enable, m_alsoBody);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqEndSequenceCommand
// ---------------------------------------------------------------------------

// @func 0x0042E9F0 SeqEndSequenceCommand::SeqEndSequenceCommand
SeqEndSequenceCommand::SeqEndSequenceCommand()
    : SequenceCommand(true)
{
}

// @func 0x0077FF00 SeqEndSequenceCommand::~SeqEndSequenceCommand
// @func 0x0077FF30 SeqEndSequenceCommand::~SeqEndSequenceCommand (deleting)
SeqEndSequenceCommand::~SeqEndSequenceCommand() = default;

// @func 0x0042EA30 SeqEndSequenceCommand::Execute
void SeqEndSequenceCommand::Execute()
{
    if (m_sequence != nullptr)
        m_sequence->finished = true;
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqPlayAnimationCommand
// ---------------------------------------------------------------------------

// @func 0x0042EA6C SeqPlayAnimationCommand::SeqPlayAnimationCommand
SeqPlayAnimationCommand::SeqPlayAnimationCommand(uint32_t entityHash, uint32_t animHash, bool loop,
                                                 bool hideFirst)
    : SequenceCommand(true), m_entityHash(entityHash), m_animHash(animHash), m_loop(loop),
      m_hideFirst(hideFirst)
{
}

// @func 0x0077FF88 SeqPlayAnimationCommand::~SeqPlayAnimationCommand
// @func 0x0077FFB8 SeqPlayAnimationCommand::~SeqPlayAnimationCommand (deleting)
SeqPlayAnimationCommand::~SeqPlayAnimationCommand() = default;

// @func 0x0042EAEC SeqPlayAnimationCommand::Execute
void SeqPlayAnimationCommand::Execute()
{
    if (WorldObject* entity = FindSequenceEntity(m_entityHash)) {
        if (EntityIsShown(entity) && m_hideFirst)
            EntitySetShown(entity, false);
        EntityPlayAnimation(entity, m_animHash, m_loop);
    }
    MarkStarted();
}

// @func 0x0042EBB8 SeqPlayAnimationCommand::Update
bool SeqPlayAnimationCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// SeqSuppressAudioCommand
// ---------------------------------------------------------------------------

// @func 0x0042EBC0 SeqSuppressAudioCommand::SeqSuppressAudioCommand
SeqSuppressAudioCommand::SeqSuppressAudioCommand(uint32_t entityHash, bool suppress)
    : SequenceCommand(false), m_entityHash(entityHash), m_suppress(suppress)
{
}

// @func 0x0077F568 SeqSuppressAudioCommand::~SeqSuppressAudioCommand
// @func 0x00780010 SeqSuppressAudioCommand::~SeqSuppressAudioCommand (deleting)
SeqSuppressAudioCommand::~SeqSuppressAudioCommand() = default;

// @func 0x0042EC20 SeqSuppressAudioCommand::Execute
void SeqSuppressAudioCommand::Execute()
{
    if (m_entityHash != 0) {
        if (WorldObject* entity = FindSequenceEntity(m_entityHash))
            EntitySuppressAudio(entity, m_suppress);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqEnqueueStreamCommand
// ---------------------------------------------------------------------------

// @func 0x0042ECB8 SeqEnqueueStreamCommand::SeqEnqueueStreamCommand
SeqEnqueueStreamCommand::SeqEnqueueStreamCommand(audio::SoundName* sound, uint32_t param10,
                                                 uint32_t param14, bool forceNew, bool blocking,
                                                 bool queueNow)
    : SequenceCommand(blocking), m_sound(sound), m_param10(param10), m_param14(param14),
      m_forceNew(forceNew)
{
    if (m_sound != nullptr)
        audio::SoundNameAddRef(m_sound);
    if (m_request == nullptr && queueNow)
        m_request = FindOrRequestSound(m_sound, m_forceNew);
}

// @func 0x0042EE90 SeqEnqueueStreamCommand::~SeqEnqueueStreamCommand
// @func 0x0042EEF0 SeqEnqueueStreamCommand::~SeqEnqueueStreamCommand (deleting)
SeqEnqueueStreamCommand::~SeqEnqueueStreamCommand()
{
    audio::Requests().Release(m_request);
    audio::SoundNameRelease(m_sound);
}

// @func 0x0042EF6C SeqEnqueueStreamCommand::Execute
void SeqEnqueueStreamCommand::Execute()
{
    // NOTE: never marks itself as started; repeated calls are harmless once
    // the request exists.
    if (m_request == nullptr)
        m_request = FindOrRequestSound(m_sound, m_forceNew);
}

// @func 0x0042F0B4 SeqEnqueueStreamCommand::Update
bool SeqEnqueueStreamCommand::Update(float /*dt*/)
{
    if (m_request != nullptr && !StreamSystemIdle()) {
        // Wait while the request holds a voice. A request without one is
        // not waited for (the original warns that it has not started
        // buffering).
        if (m_request->hasVoice)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// SeqColorFilterCommand
// ---------------------------------------------------------------------------

// @func 0x0042F144 SeqColorFilterCommand::SeqColorFilterCommand
SeqColorFilterCommand::SeqColorFilterCommand(float red, float green, float blue, uint32_t mode,
                                             bool hold, bool blocking)
    : SequenceCommand(blocking), m_red(red), m_green(green), m_blue(blue), m_mode(mode),
      m_hold(hold)
{
}

// @func 0x00780068 SeqColorFilterCommand::~SeqColorFilterCommand
// @func 0x00780098 SeqColorFilterCommand::~SeqColorFilterCommand (deleting)
SeqColorFilterCommand::~SeqColorFilterCommand() = default;

// @func 0x0042F1D4 SeqColorFilterCommand::Execute
void SeqColorFilterCommand::Execute()
{
    StartColorFilter(m_red, m_green, m_blue, m_mode, m_hold);
    MarkStarted();
}

// @func 0x0042F240 SeqColorFilterCommand::Update
bool SeqColorFilterCommand::Update(float /*dt*/)
{
    return !ColorFilterBusy();
}

// ---------------------------------------------------------------------------
// SeqDepthOfFieldCommand
// ---------------------------------------------------------------------------

// @func 0x0042F264 SeqDepthOfFieldCommand::SeqDepthOfFieldCommand
SeqDepthOfFieldCommand::SeqDepthOfFieldCommand(float nearStart, float nearEnd, float farStart,
                                               float farEnd, bool both, bool blocking)
    : SequenceCommand(blocking), m_values{nearStart, nearEnd, farStart, farEnd}, m_both(both)
{
}

// @func 0x007800F0 SeqDepthOfFieldCommand::~SeqDepthOfFieldCommand
// @func 0x00780120 SeqDepthOfFieldCommand::~SeqDepthOfFieldCommand (deleting)
SeqDepthOfFieldCommand::~SeqDepthOfFieldCommand() = default;

// @func 0x0042F2F4 SeqDepthOfFieldCommand::Execute
void SeqDepthOfFieldCommand::Execute()
{
    StartDepthOfField(m_values[0], m_values[1], m_values[2], m_values[3], m_both);
    MarkStarted();
}

// @func 0x0042F398 SeqDepthOfFieldCommand::Update
bool SeqDepthOfFieldCommand::Update(float /*dt*/)
{
    return !DepthOfFieldBusy();
}

// ---------------------------------------------------------------------------
// SeqFocusPullCommand
// ---------------------------------------------------------------------------

// @func 0x0042F3BC SeqFocusPullCommand::SeqFocusPullCommand
SeqFocusPullCommand::SeqFocusPullCommand(const float params[7], uint32_t targetHash,
                                         uint32_t mode, bool flag30, bool flag31, bool blocking)
    : SequenceCommand(blocking), m_targetHash(targetHash), m_mode(mode), m_flag30(flag30),
      m_flag31(flag31)
{
    std::memcpy(m_params, params, sizeof(m_params));
}

// @func 0x00780178 SeqFocusPullCommand::~SeqFocusPullCommand
// @func 0x007801A8 SeqFocusPullCommand::~SeqFocusPullCommand (deleting)
SeqFocusPullCommand::~SeqFocusPullCommand() = default;

// @func 0x0042F4BC SeqFocusPullCommand::Execute
void SeqFocusPullCommand::Execute()
{
    StartFocusPull(m_params, FindSequenceEntity(m_targetHash), m_mode, m_flag30, m_flag31);
    MarkStarted();
}

// @func 0x0042F564 SeqFocusPullCommand::Update
bool SeqFocusPullCommand::Update(float /*dt*/)
{
    return !FocusPullBusy();
}

// ---------------------------------------------------------------------------
// SeqCameraEffectCommand
// ---------------------------------------------------------------------------

// @func 0x0042F854 SeqCameraEffectCommand::SeqCameraEffectCommand
SeqCameraEffectCommand::SeqCameraEffectCommand(const CameraEffectParams& params, bool blocking)
    : SequenceCommand(blocking), m_params(params), m_handle(0)
{
}

// @func 0x00780290 SeqCameraEffectCommand::~SeqCameraEffectCommand
// @func 0x007802C0 SeqCameraEffectCommand::~SeqCameraEffectCommand (deleting)
SeqCameraEffectCommand::~SeqCameraEffectCommand() = default;

// @func 0x0042F92C SeqCameraEffectCommand::Execute
void SeqCameraEffectCommand::Execute()
{
    if (m_params.words[2] != 0)
        m_handle = StartCameraEffect(m_params);
    MarkStarted();
}

// @func 0x0042F990 SeqCameraEffectCommand::Update
bool SeqCameraEffectCommand::Update(float /*dt*/)
{
    return !CameraEffectActive(m_handle);
}

// ---------------------------------------------------------------------------
// SeqAmbientTintCommand
// ---------------------------------------------------------------------------

// @func 0x0042FA20 SeqAmbientTintCommand::SeqAmbientTintCommand
SeqAmbientTintCommand::SeqAmbientTintCommand(float red, float green, float blue)
    : SequenceCommand(true), m_red(red), m_green(green), m_blue(blue)
{
}

// @func 0x00780318 SeqAmbientTintCommand::~SeqAmbientTintCommand
// @func 0x00780348 SeqAmbientTintCommand::~SeqAmbientTintCommand (deleting)
SeqAmbientTintCommand::~SeqAmbientTintCommand() = default;

// @func 0x0042FA90 SeqAmbientTintCommand::Execute
void SeqAmbientTintCommand::Execute()
{
    // NOTE: not marked as started; Update() finishes it at once.
    SetAmbientTint(m_red, m_green, m_blue);
}

// ---------------------------------------------------------------------------
// SeqRemoveEntityCommand
// ---------------------------------------------------------------------------

// @func 0x0042FDC8 SeqRemoveEntityCommand::SeqRemoveEntityCommand
SeqRemoveEntityCommand::SeqRemoveEntityCommand(uint32_t entityHash, bool playDespawn)
    : SequenceCommand(true), m_entityHash(entityHash), m_playDespawn(playDespawn)
{
}

// @func 0x00780428 SeqRemoveEntityCommand::~SeqRemoveEntityCommand
// @func 0x00780458 SeqRemoveEntityCommand::~SeqRemoveEntityCommand (deleting)
SeqRemoveEntityCommand::~SeqRemoveEntityCommand() = default;

// @func 0x0042FE28 SeqRemoveEntityCommand::Execute
void SeqRemoveEntityCommand::Execute()
{
    if (WorldObject* entity = FindSequenceEntity(m_entityHash)) {
        UnregisterSequenceEntity(FindSequenceEntityOwner(entity), entity);
        if (m_playDespawn && EntityIsActor(entity))
            EntityDespawn(entity);
        else
            EntityRelease(entity, true);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqWaitCommand
// ---------------------------------------------------------------------------

// @func 0x00431EEC SeqWaitCommand::SeqWaitCommand
SeqWaitCommand::SeqWaitCommand(float duration)
    : SequenceCommand(true), m_duration(duration), m_elapsed(0.0f)
{
}

// @func 0x00780650 SeqWaitCommand::~SeqWaitCommand
// @func 0x00780680 SeqWaitCommand::~SeqWaitCommand (deleting)
SeqWaitCommand::~SeqWaitCommand() = default;

// @func 0x00431F4C SeqWaitCommand::Execute
void SeqWaitCommand::Execute()
{
    m_elapsed = 0.0f;
    MarkStarted();
}

// @func 0x00431F78 SeqWaitCommand::Update
bool SeqWaitCommand::Update(float dt)
{
    m_elapsed += dt;
    return m_duration <= m_elapsed;
}

// ---------------------------------------------------------------------------
// Game world flag toggles
// ---------------------------------------------------------------------------

// @func 0x00431FA8 SeqHideHudCommand::SeqHideHudCommand
SeqHideHudCommand::SeqHideHudCommand(bool hide)
    : SequenceCommand(true), m_value(hide)
{
}

// @func 0x007806D8 SeqHideHudCommand::~SeqHideHudCommand
// @func 0x00780708 SeqHideHudCommand::~SeqHideHudCommand (deleting)
SeqHideHudCommand::~SeqHideHudCommand() = default;

// @func 0x00431FF8 SeqHideHudCommand::Execute
void SeqHideHudCommand::Execute()
{
    uint32_t& flags = GetWorldFlags().flags28;
    flags = m_value ? (flags | 0x2u) : (flags & ~0x2u);
}

// @func 0x0043202C SeqShowHudCommand::SeqShowHudCommand
SeqShowHudCommand::SeqShowHudCommand(bool show)
    : SequenceCommand(false), m_value(show)
{
}

// @func 0x00780760 SeqShowHudCommand::~SeqShowHudCommand
// @func 0x00780790 SeqShowHudCommand::~SeqShowHudCommand (deleting)
SeqShowHudCommand::~SeqShowHudCommand() = default;

// @func 0x0043207C SeqShowHudCommand::Execute
void SeqShowHudCommand::Execute()
{
    SetHudVisible(m_value);
}

// @func 0x004320B0 SeqLockInputCommand::SeqLockInputCommand
SeqLockInputCommand::SeqLockInputCommand(bool lock)
    : SequenceCommand(true), m_value(lock)
{
}

// @func 0x007807E8 SeqLockInputCommand::~SeqLockInputCommand
// @func 0x00780818 SeqLockInputCommand::~SeqLockInputCommand (deleting)
SeqLockInputCommand::~SeqLockInputCommand() = default;

// @func 0x00432100 SeqLockInputCommand::Execute
void SeqLockInputCommand::Execute()
{
    uint32_t& flags = GetWorldFlags().flags28;
    flags = m_value ? (flags | 0x4u) : (flags & ~0x4u);
}

// @func 0x00432134 SeqHudCounterCommand::SeqHudCounterCommand
SeqHudCounterCommand::SeqHudCounterCommand(bool visible)
    : SequenceCommand(true), m_value(visible)
{
}

// @func 0x00780870 SeqHudCounterCommand::~SeqHudCounterCommand
// @func 0x007808A0 SeqHudCounterCommand::~SeqHudCounterCommand (deleting)
SeqHudCounterCommand::~SeqHudCounterCommand() = default;

// @func 0x00432184 SeqHudCounterCommand::Execute
void SeqHudCounterCommand::Execute()
{
    SetHudCounterVisible(m_value);
}

// ---------------------------------------------------------------------------
// SeqMessageBoxCommand
// ---------------------------------------------------------------------------

// @func 0x00433938 SeqMessageBoxCommand::SeqMessageBoxCommand
SeqMessageBoxCommand::SeqMessageBoxCommand(const char* text, const int32_t pos[2],
                                           const int32_t size[2], bool blocking)
    : SequenceCommand(blocking), m_pos{pos[0], pos[1]}, m_size{size[0], size[1]}
{
    CopyName(m_text, 0x7F, text);
}

// @func 0x004339E8 SeqMessageBoxCommand::~SeqMessageBoxCommand
// @func 0x00433A18 SeqMessageBoxCommand::~SeqMessageBoxCommand (deleting)
SeqMessageBoxCommand::~SeqMessageBoxCommand() = default;

// @func 0x00433A70 SeqMessageBoxCommand::Execute
void SeqMessageBoxCommand::Execute()
{
    ShowMessageBox(m_text, m_pos, m_size);
    MarkStarted();
}

// @func 0x00433AD0 SeqMessageBoxCommand::Update
bool SeqMessageBoxCommand::Update(float /*dt*/)
{
    return GameMode() != kGameModeModalDialog;
}

// ---------------------------------------------------------------------------
// SeqExposureRampCommand
// ---------------------------------------------------------------------------

// @func 0x004340D8 SeqExposureRampCommand::SeqExposureRampCommand
SeqExposureRampCommand::SeqExposureRampCommand(float time, const uint8_t from[4],
                                               const uint8_t to[4], bool hold, bool blocking)
    : SequenceCommand(blocking), m_time(time), m_hold(hold)
{
    std::memcpy(m_from, from, sizeof(m_from));
    std::memcpy(m_to, to, sizeof(m_to));
}

// @func 0x004341B0 SeqExposureRampCommand::~SeqExposureRampCommand
// @func 0x004341E0 SeqExposureRampCommand::~SeqExposureRampCommand (deleting)
SeqExposureRampCommand::~SeqExposureRampCommand() = default;

// @func 0x00434238 SeqExposureRampCommand::Execute
void SeqExposureRampCommand::Execute()
{
    StartExposureRamp(m_time, m_from, m_to, m_hold);
    MarkStarted();
}

// @func 0x004342B8 SeqExposureRampCommand::Update
bool SeqExposureRampCommand::Update(float /*dt*/)
{
    return !ExposureRampBusy();
}

// ---------------------------------------------------------------------------
// SeqWaitForButtonCommand
// ---------------------------------------------------------------------------

// @func 0x00434A10 SeqWaitForButtonCommand::SeqWaitForButtonCommand
SeqWaitForButtonCommand::SeqWaitForButtonCommand(float timeout, int player, uint32_t buttonMask,
                                                 uint32_t listenerKey, uint32_t message)
    : SequenceCommand(true), m_player(player), m_buttonMask(buttonMask),
      m_listenerKey(listenerKey), m_message(message), m_timeout(timeout)
{
}

// @func 0x0077F598 SeqWaitForButtonCommand::~SeqWaitForButtonCommand
// @func 0x00780AC8 SeqWaitForButtonCommand::~SeqWaitForButtonCommand (deleting)
SeqWaitForButtonCommand::~SeqWaitForButtonCommand() = default;

// @func 0x00434AB8 SeqWaitForButtonCommand::Execute
void SeqWaitForButtonCommand::Execute()
{
    MarkStarted();
    m_elapsed = 0.0f;
}

// @func 0x00434AF8 SeqWaitForButtonCommand::Update
bool SeqWaitForButtonCommand::Update(float dt)
{
    if (m_timeout > 0.0f && m_timeout <= m_elapsed) {
        if (m_message == 0)
            return true;
        // NOTE: the message is posted again every frame after the timeout
        // until the button is pressed.
        PostScriptMessage(nullptr, m_listenerKey, m_message, 0);
        m_timedOut = true;
    }

    m_elapsed += dt;

    WorldObject* character = PlayerCharacter(m_player);
    if (character == nullptr || !EntityIsCharacter(character) || m_buttonMask == 0)
        return false;
    const uint32_t accepted = CharacterAcceptedButtons(character, m_buttonMask);
    return accepted != 0 && PadPressed(PlayerPadIndex(m_player), accepted);
}

// ---------------------------------------------------------------------------
// SeqTransitionPromptCommand
// ---------------------------------------------------------------------------

// @func 0x00434CA8 SeqTransitionPromptCommand::SeqTransitionPromptCommand
SeqTransitionPromptCommand::SeqTransitionPromptCommand(uint32_t target)
    : SequenceCommand(true), m_target(target), m_done(false)
{
}

// @func 0x0077F5C8 SeqTransitionPromptCommand::~SeqTransitionPromptCommand
// @func 0x00780B20 SeqTransitionPromptCommand::~SeqTransitionPromptCommand (deleting)
SeqTransitionPromptCommand::~SeqTransitionPromptCommand() = default;

// @func 0x00434D08 SeqTransitionPromptCommand::Execute
void SeqTransitionPromptCommand::Execute()
{
    ShowYesNoPrompt(PlayerCharacter(0), kTransitionPromptText, kYesNoButtons, &OnYes, &OnNo,
                    this);
    MarkStarted();
}

// @func 0x00434D8C SeqTransitionPromptCommand::Update
bool SeqTransitionPromptCommand::Update(float /*dt*/)
{
    return m_done;
}

// @func 0x00434D94 SeqTransitionPromptCommand::OnYes
void SeqTransitionPromptCommand::OnYes(void* user)
{
    auto* self = static_cast<SeqTransitionPromptCommand*>(user);
    RequestGameTransition(self->m_target);
    self->m_done = true;
}

// @func 0x00434E00 SeqTransitionPromptCommand::OnNo
void SeqTransitionPromptCommand::OnNo(void* user)
{
    static_cast<SeqTransitionPromptCommand*>(user)->m_done = true;
}

// ---------------------------------------------------------------------------
// SeqMoveEntityCommand
// ---------------------------------------------------------------------------

// @func 0x00437A1C SeqMoveEntityCommand::SeqMoveEntityCommand
SeqMoveEntityCommand::SeqMoveEntityCommand(float time, uint32_t entityHash, uint32_t a,
                                           uint32_t b, uint32_t c, bool quiet, bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_a(a), m_b(b), m_c(c), m_time(time),
      m_quiet(quiet)
{
}

// @func 0x00780C60 SeqMoveEntityCommand::~SeqMoveEntityCommand
// @func 0x00780C90 SeqMoveEntityCommand::~SeqMoveEntityCommand (deleting)
SeqMoveEntityCommand::~SeqMoveEntityCommand() = default;

// @func 0x00437ACC SeqMoveEntityCommand::Execute
void SeqMoveEntityCommand::Execute()
{
    if (m_entityHash == kCameraEntityHash) {
        StartCameraMover(m_a, m_b, m_c, m_time);
    } else {
        m_entity = FindSequenceEntity(m_entityHash);
        if (m_entity != nullptr)
            EntityStartMover(m_entity, m_a, m_b, m_c, m_time);
    }
    MarkStarted();
}

// @func 0x00437BB0 SeqMoveEntityCommand::Update
bool SeqMoveEntityCommand::Update(float /*dt*/)
{
    if (m_entity == nullptr)
        return true;
    return EntityMoverDone(m_entity);
}

// ---------------------------------------------------------------------------
// SeqEnableScriptedEntitiesCommand
// ---------------------------------------------------------------------------

// @func 0x00437C0C SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand
SeqEnableScriptedEntitiesCommand::SeqEnableScriptedEntitiesCommand(bool enable)
    : SequenceCommand(false), m_enable(enable)
{
}

// @func 0x00780CE8 SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand
// @func 0x00780D18 SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand (deleting)
SeqEnableScriptedEntitiesCommand::~SeqEnableScriptedEntitiesCommand() = default;

// @func 0x00437C5C SeqEnableScriptedEntitiesCommand::Execute
void SeqEnableScriptedEntitiesCommand::Execute()
{
    const int count = WorldEntityCount();
    for (int i = 0; i < count; ++i) {
        WorldObject* entity = WorldEntityAt(i);
        if (entity == nullptr || !EntityHasController(entity))
            continue;
        if (EntityControllerIsScripted(entity))
            EntityEnableUpdate(entity, m_enable);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqMinigameControlCommand
// ---------------------------------------------------------------------------

// @func 0x00437D58 SeqMinigameControlCommand::SeqMinigameControlCommand
SeqMinigameControlCommand::SeqMinigameControlCommand(bool resetAll, bool resetScore,
                                                     bool resetTimer, bool showIntro,
                                                     bool startPlayer, bool resetRound)
    : SequenceCommand(false), m_resetAll(resetAll), m_resetScore(resetScore),
      m_resetTimer(resetTimer), m_showIntro(showIntro), m_startPlayer(startPlayer),
      m_resetRound(resetRound)
{
}

// @func 0x00780D70 SeqMinigameControlCommand::~SeqMinigameControlCommand
// @func 0x00780DA0 SeqMinigameControlCommand::~SeqMinigameControlCommand (deleting)
SeqMinigameControlCommand::~SeqMinigameControlCommand() = default;

// @func 0x00437E08 SeqMinigameControlCommand::Execute
void SeqMinigameControlCommand::Execute()
{
    // NOTE: not marked as started; the command is non-blocking and deleted
    // right after this call.
    Minigame* game = ActiveMinigame();
    if (game == nullptr || !game->AcceptsScriptControl())
        return;
    if (m_resetAll)
        game->ClearRequests();
    if (m_resetScore)
        game->RequestScoreReset();
    if (m_resetTimer)
        game->RequestTimerReset();
    if (m_showIntro)
        game->PlayIntro();
    if (m_startPlayer)
        game->StartPlayer();
    if (m_resetRound)
        game->RequestFullReset();
}

// ---------------------------------------------------------------------------
// SeqHudTimerCommand
// ---------------------------------------------------------------------------

// @func 0x00438498 SeqHudTimerCommand::SeqHudTimerCommand
SeqHudTimerCommand::SeqHudTimerCommand(float seconds)
    : SequenceCommand(true), m_seconds(seconds)
{
}

// @func 0x00780F10 SeqHudTimerCommand::~SeqHudTimerCommand
// @func 0x00780F40 SeqHudTimerCommand::~SeqHudTimerCommand (deleting)
SeqHudTimerCommand::~SeqHudTimerCommand() = default;

// @func 0x004384E8 SeqHudTimerCommand::Execute
void SeqHudTimerCommand::Execute()
{
    SetHudTimer(m_seconds);
}

// ---------------------------------------------------------------------------
// Game world byte flags
// ---------------------------------------------------------------------------

// @func 0x00438508 SeqCutsceneFlagCommand::SeqCutsceneFlagCommand
SeqCutsceneFlagCommand::SeqCutsceneFlagCommand(bool value)
    : SequenceCommand(true), m_value(value)
{
}

// @func 0x00780F98 SeqCutsceneFlagCommand::~SeqCutsceneFlagCommand
// @func 0x00780FC8 SeqCutsceneFlagCommand::~SeqCutsceneFlagCommand (deleting)
SeqCutsceneFlagCommand::~SeqCutsceneFlagCommand() = default;

// @func 0x00438558 SeqCutsceneFlagCommand::Execute
void SeqCutsceneFlagCommand::Execute()
{
    uint8_t& flags = GetWorldFlags().flags2D;
    flags = static_cast<uint8_t>((flags & ~0x20u) | (m_value ? 0x20u : 0u));
    MarkStarted();
}

// @func 0x00438594 SeqCutsceneFlagCommand::Update
bool SeqCutsceneFlagCommand::Update(float /*dt*/)
{
    return true;
}

// @func 0x0043859C SeqPausePlayerCommand::SeqPausePlayerCommand
SeqPausePlayerCommand::SeqPausePlayerCommand(bool value)
    : SequenceCommand(true), m_value(value)
{
}

// @func 0x00781020 SeqPausePlayerCommand::~SeqPausePlayerCommand
// @func 0x00781050 SeqPausePlayerCommand::~SeqPausePlayerCommand (deleting)
SeqPausePlayerCommand::~SeqPausePlayerCommand() = default;

// @func 0x004385EC SeqPausePlayerCommand::Execute
void SeqPausePlayerCommand::Execute()
{
    uint8_t& flags = GetWorldFlags().flags2E;
    flags = static_cast<uint8_t>((flags & ~0x80u) | (m_value ? 0x80u : 0u));
    MarkStarted();
}

// @func 0x00438628 SeqPausePlayerCommand::Update
bool SeqPausePlayerCommand::Update(float /*dt*/)
{
    return true;
}

// @func 0x00438630 SeqPauseAiCommand::SeqPauseAiCommand
SeqPauseAiCommand::SeqPauseAiCommand(bool value)
    : SequenceCommand(true), m_value(value)
{
}

// @func 0x007810A8 SeqPauseAiCommand::~SeqPauseAiCommand
// @func 0x007810D8 SeqPauseAiCommand::~SeqPauseAiCommand (deleting)
SeqPauseAiCommand::~SeqPauseAiCommand() = default;

// @func 0x00438680 SeqPauseAiCommand::Execute
void SeqPauseAiCommand::Execute()
{
    uint8_t& flags = GetWorldFlags().flags2E;
    flags = static_cast<uint8_t>((flags & ~0x40u) | (m_value ? 0x40u : 0u));
    MarkStarted();
}

// @func 0x004386BC SeqPauseAiCommand::Update
bool SeqPauseAiCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// SeqSetEntityCommand
// ---------------------------------------------------------------------------

// @func 0x004386C4 SeqSetEntityCommand::SeqSetEntityCommand
SeqSetEntityCommand::SeqSetEntityCommand(WorldObject* entity, uint32_t alias, bool show,
                                         bool hideOnRelease, bool keepShownState, bool resetPose,
                                         bool neverShow, bool keepAnimSet, bool clearAiState,
                                         bool transferIfRegistered)
    : SequenceCommand(true), m_entity(entity), m_alias(alias), m_show(show),
      m_hideOnRelease(hideOnRelease), m_keepShownState(keepShownState), m_resetPose(resetPose),
      m_neverShow(neverShow), m_keepAnimSet(keepAnimSet), m_clearAiState(clearAiState),
      m_transfer(transferIfRegistered)
{
    if (m_entity != nullptr)
        EntityAddRef(m_entity);
}

// @func 0x00781130 SeqSetEntityCommand::~SeqSetEntityCommand
// @func 0x00781160 SeqSetEntityCommand::~SeqSetEntityCommand (deleting)
SeqSetEntityCommand::~SeqSetEntityCommand() = default;

// @func 0x004387DC SeqSetEntityCommand::Execute
void SeqSetEntityCommand::Execute()
{
    SequenceEntityOptions options;
    options.alias = m_alias;
    options.show = m_show;
    options.hideOnRelease = m_hideOnRelease;
    options.keepShownState = m_keepShownState;
    options.resetPose = m_resetPose;
    options.neverShow = m_neverShow;
    options.keepAnimSet = m_keepAnimSet;
    options.clearAiState = m_clearAiState;
    options.transferIfRegistered = m_transfer;
    RegisterSequenceEntity(m_sequence, m_entity, options);
    MarkStarted();

    // The table holds its own reference now.
    if (m_entity != nullptr) {
        EntityRelease(m_entity, false);
        m_entity = nullptr;
    }
}

// ---------------------------------------------------------------------------
// SeqEmitterParamCommand
// ---------------------------------------------------------------------------

// @func 0x004388A0 SeqEmitterParamCommand::SeqEmitterParamCommand
SeqEmitterParamCommand::SeqEmitterParamCommand(uint32_t entityHash, uint32_t value)
    : SequenceCommand(true), m_entityHash(entityHash), m_value(value)
{
}

// @func 0x007811C0 SeqEmitterParamCommand::~SeqEmitterParamCommand
// @func 0x007811F0 SeqEmitterParamCommand::~SeqEmitterParamCommand (deleting)
SeqEmitterParamCommand::~SeqEmitterParamCommand() = default;

// @func 0x00438900 SeqEmitterParamCommand::Execute
void SeqEmitterParamCommand::Execute()
{
    if (m_entityHash != 0) {
        if (WorldObject* entity = FindSequenceEntity(m_entityHash))
            EntitySetEmitterParam(entity, m_value);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqMusicCommand
// ---------------------------------------------------------------------------

// @func 0x00438B84 SeqMusicCommand::SeqMusicCommand
SeqMusicCommand::SeqMusicCommand(audio::SoundName* sound, bool play, bool flag11)
    : SequenceCommand(false), m_sound(sound), m_play(play), m_flag11(flag11)
{
    if (m_sound != nullptr)
        audio::SoundNameAddRef(m_sound);
}

// @func 0x00438C0C SeqMusicCommand::~SeqMusicCommand
// @func 0x00438C58 SeqMusicCommand::~SeqMusicCommand (deleting)
SeqMusicCommand::~SeqMusicCommand()
{
    audio::SoundNameRelease(m_sound);
}

// @func 0x00438CCC SeqMusicCommand::Execute
void SeqMusicCommand::Execute()
{
    PlayMusicStream(m_sound, m_play);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqSetEntityVisibleCommand
// ---------------------------------------------------------------------------

// @func 0x00438D14 SeqSetEntityVisibleCommand::SeqSetEntityVisibleCommand
SeqSetEntityVisibleCommand::SeqSetEntityVisibleCommand(uint32_t entityHash, uint32_t childHash,
                                                       bool visible, bool shadows, bool children,
                                                       bool effects, bool optional)
    : SequenceCommand(true), m_entityHash(entityHash), m_childHash(childHash), m_visible(visible),
      m_shadows(shadows), m_children(children), m_effects(effects), m_optional(optional)
{
}

// @func 0x007812D0 SeqSetEntityVisibleCommand::~SeqSetEntityVisibleCommand
// @func 0x00781300 SeqSetEntityVisibleCommand::~SeqSetEntityVisibleCommand (deleting)
SeqSetEntityVisibleCommand::~SeqSetEntityVisibleCommand() = default;

// @func 0x00438DDC SeqSetEntityVisibleCommand::Execute
void SeqSetEntityVisibleCommand::Execute()
{
    WorldObject* entity = FindSequenceEntity(m_entityHash);
    if (entity != nullptr && m_childHash != 0)
        entity = EntityFindChild(entity, m_childHash);
    // A missing entity is reported unless the command is marked optional.
    if (entity != nullptr)
        EntitySetVisibility(entity, m_visible, m_shadows, m_children, m_effects);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqSetEntityUpdateCommand
// ---------------------------------------------------------------------------

// @func 0x00438F3C SeqSetEntityUpdateCommand::SeqSetEntityUpdateCommand
SeqSetEntityUpdateCommand::SeqSetEntityUpdateCommand(uint32_t entityHash, bool update)
    : SequenceCommand(true), m_entityHash(entityHash), m_update(update)
{
}

// @func 0x00781360 SeqSetEntityUpdateCommand::~SeqSetEntityUpdateCommand
// @func 0x00781390 SeqSetEntityUpdateCommand::~SeqSetEntityUpdateCommand (deleting)
SeqSetEntityUpdateCommand::~SeqSetEntityUpdateCommand() = default;

// @func 0x00438FB4 SeqSetEntityUpdateCommand::Execute
void SeqSetEntityUpdateCommand::Execute()
{
    if (WorldObject* entity = FindSequenceEntity(m_entityHash)) {
        if (EntityHasUpdateHook(entity))
            EntitySetUpdating(entity, m_update);
        EntityEnableUpdate(entity, m_update);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqStartAnimationCommand
// ---------------------------------------------------------------------------

// @func 0x00439C28 SeqStartAnimationCommand::SeqStartAnimationCommand
SeqStartAnimationCommand::SeqStartAnimationCommand(uint32_t entityHash, uint32_t animHash,
                                                   bool loop)
    : SequenceCommand(false), m_entityHash(entityHash), m_animHash(animHash), m_loop(loop)
{
}

// @func 0x00781510 SeqStartAnimationCommand::~SeqStartAnimationCommand
// @func 0x00781540 SeqStartAnimationCommand::~SeqStartAnimationCommand (deleting)
SeqStartAnimationCommand::~SeqStartAnimationCommand() = default;

// @func 0x00439CB0 SeqStartAnimationCommand::Execute
void SeqStartAnimationCommand::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (m_entity != nullptr)
        EntityPlayAnimation(m_entity, m_animHash, m_loop);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqTimeScaleCommand
// ---------------------------------------------------------------------------

// @func 0x00439D48 SeqTimeScaleCommand::SeqTimeScaleCommand
SeqTimeScaleCommand::SeqTimeScaleCommand(int kind, float scale)
    : SequenceCommand(true), m_kind(kind), m_scale(scale)
{
}

// @func 0x00781598 SeqTimeScaleCommand::~SeqTimeScaleCommand
// @func 0x007815C8 SeqTimeScaleCommand::~SeqTimeScaleCommand (deleting)
SeqTimeScaleCommand::~SeqTimeScaleCommand() = default;

// @func 0x00439DA8 SeqTimeScaleCommand::Execute
void SeqTimeScaleCommand::Execute()
{
    if (m_kind == 1)
        SetGameTimeScale(m_scale);
}

// ---------------------------------------------------------------------------
// SeqLevelHeadlineCommand
// ---------------------------------------------------------------------------

// @func 0x00439DC4 SeqLevelHeadlineCommand::SeqLevelHeadlineCommand
SeqLevelHeadlineCommand::SeqLevelHeadlineCommand()
    : SequenceCommand(true)
{
}

// @func 0x00781620 SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand
// @func 0x00781650 SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand (deleting)
SeqLevelHeadlineCommand::~SeqLevelHeadlineCommand() = default;

// @func 0x00439E04 SeqLevelHeadlineCommand::Execute
void SeqLevelHeadlineCommand::Execute()
{
    ShowLevelHeadline();
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqStartSequenceCommand
// ---------------------------------------------------------------------------

// @func 0x0043A620 SeqStartSequenceCommand::SeqStartSequenceCommand
SeqStartSequenceCommand::SeqStartSequenceCommand(uint32_t sequenceHash)
    : SequenceCommand(true), m_sequenceHash(sequenceHash)
{
}

// @func 0x00781740 SeqStartSequenceCommand::~SeqStartSequenceCommand
// @func 0x00781770 SeqStartSequenceCommand::~SeqStartSequenceCommand (deleting)
SeqStartSequenceCommand::~SeqStartSequenceCommand() = default;

// @func 0x0043A670 SeqStartSequenceCommand::Execute
void SeqStartSequenceCommand::Execute()
{
    if (m_sequenceHash != 0) {
        if (Sequence* sequence = FindSequence(m_sequenceHash)) {
            sequence->started = true;
            QueueStartedSequence(sequence);
        }
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqStopSoundCommand
// ---------------------------------------------------------------------------

// @func 0x0043A6DC SeqStopSoundCommand::SeqStopSoundCommand
SeqStopSoundCommand::SeqStopSoundCommand(uint32_t categoryHash, uint32_t trackKey, bool force)
    : SequenceCommand(true), m_categoryHash(categoryHash), m_trackKey(trackKey), m_force(force)
{
}

// @func 0x007817D0 SeqStopSoundCommand::~SeqStopSoundCommand
// @func 0x00781800 SeqStopSoundCommand::~SeqStopSoundCommand (deleting)
SeqStopSoundCommand::~SeqStopSoundCommand() = default;

// @func 0x0043A74C SeqStopSoundCommand::Execute
void SeqStopSoundCommand::Execute()
{
    if (m_trackKey != 0)
        audio::StopTrackedVoice(m_trackKey);
    else if (m_categoryHash != 0)
        audio::StopCategoryByHash(m_categoryHash, m_force);
    else
        StopAllSounds();
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqStopMusicCommand
// ---------------------------------------------------------------------------

// @func 0x0043A7BC SeqStopMusicCommand::SeqStopMusicCommand
SeqStopMusicCommand::SeqStopMusicCommand(bool release)
    : SequenceCommand(false), m_release(release)
{
}

// @func 0x0077F6A0 SeqStopMusicCommand::~SeqStopMusicCommand
// @func 0x00781860 SeqStopMusicCommand::~SeqStopMusicCommand (deleting)
SeqStopMusicCommand::~SeqStopMusicCommand() = default;

// @func 0x0043A80C SeqStopMusicCommand::Execute
void SeqStopMusicCommand::Execute()
{
    audio::StopMusic(m_release);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqWaitForSequencesCommand
// ---------------------------------------------------------------------------

// @func 0x0043ABA0 SeqWaitForSequencesCommand::SeqWaitForSequencesCommand
SeqWaitForSequencesCommand::SeqWaitForSequencesCommand(const std::vector<uint32_t>& sequenceHashes)
    : SequenceCommand(true), m_sequenceHashes(sequenceHashes)
{
}

// @func 0x00781940 SeqWaitForSequencesCommand::~SeqWaitForSequencesCommand
// @func 0x007819A8 SeqWaitForSequencesCommand::~SeqWaitForSequencesCommand (deleting)
SeqWaitForSequencesCommand::~SeqWaitForSequencesCommand() = default;

// @func 0x0043ACB8 SeqWaitForSequencesCommand::Update
bool SeqWaitForSequencesCommand::Update(float /*dt*/)
{
    bool done = true;
    for (int i = 0; i < SequenceCount(); ++i) {
        const Sequence* sequence = gameplay::GetSequence(i);
        for (uint32_t hash : m_sequenceHashes) {
            if (sequence->nameHash == hash && !sequence->commands.empty())
                done = false;
        }
    }
    return done;
}

// ---------------------------------------------------------------------------
// SeqWaitForSoundCommand
// ---------------------------------------------------------------------------

// @func 0x0043AD8C SeqWaitForSoundCommand::SeqWaitForSoundCommand
SeqWaitForSoundCommand::SeqWaitForSoundCommand(uint32_t categoryHash, uint32_t trackKey)
    : SequenceCommand(true), m_category(audio::CategoryFromHash(categoryHash)),
      m_trackKey(trackKey)
{
}

// @func 0x00781A28 SeqWaitForSoundCommand::~SeqWaitForSoundCommand
// @func 0x00781A58 SeqWaitForSoundCommand::~SeqWaitForSoundCommand (deleting)
SeqWaitForSoundCommand::~SeqWaitForSoundCommand() = default;

// @func 0x0043AE04 SeqWaitForSoundCommand::Update
bool SeqWaitForSoundCommand::Update(float /*dt*/)
{
    const bool playing = m_trackKey == 0 ? IsSoundCategoryPlaying(m_category)
                                         : IsTrackedVoicePlaying(m_trackKey);
    return !playing;
}

} // namespace gameplay
} // namespace sly
