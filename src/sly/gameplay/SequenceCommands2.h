// openTIT - Sequencer commands, second batch (see Sequencer.h and
// SequenceCommands.h): entity placement, camera views, character actions,
// hits, HUD screens/layers, skins, animation clips and anim scripts.
//
// The console vtable of each class is given in its comment.
#pragma once

#include "sly/gameplay/GameplayWorld.h"
#include "sly/gameplay/Sequencer.h"

#include <cstdint>
#include <vector>

namespace ape { class ApeAnimScriptInstance; }
namespace sly { namespace audio { struct SoundName; struct SoundRequest; class AudioBus; } }

namespace sly {
namespace gameplay {

// Places a sequence entity relative to another one: a local offset and an
// Euler rotation (radians, applied as a quaternion) on a node of the target
// (0x00B20CF8).
class SeqPlaceEntityCommand : public SequenceCommand {
public:
    SeqPlaceEntityCommand(uint32_t entityHash, uint32_t targetHash, uint32_t nodeHash,
                          const ape::Vec3& offset, const ape::Vec3& rotation, uint32_t param40,
                          const ape::Vec3& vector50, const ape::Vec3& vector60, bool flagA,
                          bool flagB, bool flagC, bool flagD);
    ~SeqPlaceEntityCommand() override;
    void Execute() override;
    // @func 0x0077F7E0 SeqPlaceEntityCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    WorldObject* m_target = nullptr;   // +0x14
    uint32_t m_targetHash = 0;         // +0x18
    uint32_t m_nodeHash = 0;           // +0x1C
    ape::Vec3 m_offset{};              // +0x20
    ape::Vec3 m_rotation{};            // +0x30 Euler angles (radians)
    uint32_t m_param40 = 0;            // +0x40
    ape::Vec3 m_vector50{};            // +0x50 NOTE: stored but unused
    ape::Vec3 m_vector60{};            // +0x60 NOTE: stored but unused
    bool m_flagA = false;              // +0x70
    bool m_flagB = false;              // +0x71
    bool m_flagC = false;              // +0x72
    bool m_flagD = false;              // +0x73
};

// Puts the camera at a camera object or a locator (0x00B20D28).
class SeqSetCameraViewCommand : public SequenceCommand {
public:
    SeqSetCameraViewCommand(WorldObject* camera, WorldObject* locator, float fov);
    ~SeqSetCameraViewCommand() override;
    void Execute() override;
    // @func 0x0077F8F8 SeqSetCameraViewCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    WorldObject* m_camera = nullptr;    // +0x0C
    WorldObject* m_locator = nullptr;   // +0x10
    float m_fov = 0.0f;                 // +0x14 <= 0: use the camera's own
};

// Drops the scripted camera, optionally blending back (0x00B20D40).
class SeqCameraReleaseCommand : public SequenceCommand {
public:
    SeqCameraReleaseCommand(float blendTime, bool blocking);
    ~SeqCameraReleaseCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    float m_blendTime = 0.0f;           // +0x0C
    GameCamera* m_camera = nullptr;     // +0x10 NOTE: never set; Update ends at once
};

// Scripted character actions. Each command switches the character to its
// scripted behaviour state, starts one action of that state's sub-machine
// and (when blocking) waits for it before restoring the default state.
// NOTE: the action ids (1-4) come from the state machine; what each action
// does is not decoded, hence the numbered names.

// Action 1 (0x00B20FB0); also drives mounts and creatures.
class SeqCharacterAction1Command : public SequenceCommand {
public:
    SeqCharacterAction1Command(float value, uint32_t entityHash, uint32_t target,
                               uint32_t param18, bool flag, bool blocking);
    ~SeqCharacterAction1Command() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_target = 0;             // +0x14
    uint32_t m_param18 = 0;            // +0x18 NOTE: stored but unused
    float m_value = 0.0f;              // +0x1C
    bool m_flag = false;               // +0x20 bit 31
    bool m_wasIdle = false;            // +0x20 bit 30
};

// Action 2 (0x00B20F80).
class SeqCharacterAction2Command : public SequenceCommand {
public:
    SeqCharacterAction2Command(float valueA, float valueB, uint32_t entityHash, uint32_t target,
                               uint32_t param18, bool flagA, bool flagB, bool blocking);
    ~SeqCharacterAction2Command() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_target = 0;             // +0x14
    uint32_t m_param18 = 0;            // +0x18 NOTE: stored but unused
    float m_valueA = 0.0f;             // +0x1C
    float m_valueB = 0.0f;             // +0x20
    bool m_flagA = false;              // +0x24 bit 31
    bool m_flagB = false;              // +0x24 bit 30
};

// Action 3 (0x00B20F38).
class SeqCharacterAction3Command : public SequenceCommand {
public:
    SeqCharacterAction3Command(uint32_t entityHash, uint32_t target, uint32_t param18, bool flag,
                               bool blocking);
    ~SeqCharacterAction3Command() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_target = 0;             // +0x14
    uint32_t m_param18 = 0;            // +0x18 NOTE: stored but unused
    bool m_flag = false;               // +0x1C
};

// Action 4 (0x00B20D70).
class SeqCharacterAction4Command : public SequenceCommand {
public:
    SeqCharacterAction4Command(uint32_t entityHash, uint32_t target, bool blocking);
    ~SeqCharacterAction4Command() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_target = 0;             // +0x14
};

// Hits a world entity (or one of its parts): characters take a hit pushed
// away from the entity's root, other actors take damage (0x00B20D88).
class SeqHitEntityCommand : public SequenceCommand {
public:
    SeqHitEntityCommand(float force, float lift, uint32_t entityHash, uint32_t partHash,
                        uint32_t param14, int32_t mode);
    ~SeqHitEntityCommand() override;
    void Execute() override;
    // @func 0x0077FB20 SeqHitEntityCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    uint32_t m_entityHash = 0;   // +0x0C
    uint32_t m_partHash = 0;     // +0x10
    uint32_t m_param14 = 0;      // +0x14 NOTE: stored but unused
    float m_force = 0.0f;        // +0x18
    float m_lift = 0.0f;         // +0x1C
    int32_t m_mode = 0;          // +0x20
};

// Gives the player control back or takes it away (0x00B20E00).
class SeqPlayerControlCommand : public SequenceCommand {
public:
    SeqPlayerControlCommand(bool enable, bool refresh);
    ~SeqPlayerControlCommand() override;
    void Execute() override;
    // @func 0x0077FDE8 SeqPlayerControlCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    bool m_enable = false;    // +0x08 bit 23
    bool m_refresh = false;   // +0x08 bit 22
};

// Shows or hides a HUD screen, or the whole HUD (0x00B20E18).
class SeqHudScreenCommand : public SequenceCommand {
public:
    SeqHudScreenCommand(uint32_t screenHash, bool show, bool setPersistent, bool persistent,
                        bool skipToEnd, bool restart, bool blocking);
    ~SeqHudScreenCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_screenHash = 0;     // +0x0C
    bool m_show = false;           // +0x10
    bool m_setPersistent = false;  // +0x11
    bool m_persistent = false;     // +0x12
    bool m_skipToEnd = false;      // +0x13
    bool m_restart = false;        // +0x14
};

// Shows or hides a HUD layer (0x00B20E30).
class SeqHudLayerCommand : public SequenceCommand {
public:
    SeqHudLayerCommand(uint32_t layerHash, bool show, bool setPersistent, bool persistent,
                       bool restart, bool blocking);
    ~SeqHudLayerCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    uint32_t m_layerHash = 0;      // +0x0C
    bool m_show = false;           // +0x10
    bool m_setPersistent = false;  // +0x11
    bool m_persistent = false;     // +0x12
    bool m_restart = false;        // +0x13
};

// Attaches a sequence entity to a skin of another one, optionally switching
// to that skin just for the attachment (0x00B20EF0).
class SeqSkinAttachCommand : public SequenceCommand {
public:
    SeqSkinAttachCommand(uint32_t hostHash, uint32_t childHash, EntitySkin* skin,
                         uint32_t socket, bool restore);
    ~SeqSkinAttachCommand() override;
    void Execute() override;
    // @func 0x00780288 SeqSkinAttachCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    uint32_t m_hostHash = 0;       // +0x0C
    uint32_t m_childHash = 0;      // +0x10
    EntitySkin* m_skin = nullptr;  // +0x14 null: use the active skin
    uint32_t m_socket = 0;         // +0x18
    bool m_restore = false;        // +0x1C
};

// Plays an animation clip on a sequence entity and waits for it
// (0x00B21040).
class SeqPlayClipCommand : public SequenceCommand {
public:
    SeqPlayClipCommand(float blendIn, float speed, uint32_t entityHash, uint32_t clipHash,
                       uint32_t waitClipHash, uint32_t param, bool loop, bool sequenceAnimated,
                       bool blocking, bool showFirst, bool restoreShown);
    ~SeqPlayClipCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10
    uint32_t m_clipHash = 0;           // +0x14
    uint32_t m_waitClipHash = 0;       // +0x18
    float m_blendIn = 0.0f;            // +0x1C
    float m_speed = 0.0f;              // +0x20
    uint32_t m_param = 0;              // +0x24
    // +0x28 flags
    bool m_loop = false;               // bit 31
    bool m_sequenceAnimated = false;   // bit 30
    bool m_wasShown = false;           // bit 29
    bool m_showFirst = false;          // bit 28
    bool m_restoreShown = false;       // bit 27
};

// One variable binding handed to an anim script started by a sequence.
// NOTE: 0x4C-byte records consumed by 0x0049FCC0; layout not decoded.
struct AnimScriptBinding {
    uint8_t data[0x4C] = {};
};
// 0x0049FCC0: binds the variables of an anim script instance.
void AnimScriptBindVariables(ape::ApeAnimScriptInstance* instance,
                             const std::vector<AnimScriptBinding>& bindings, uint32_t param,
                             bool flag, bool flagB);

// Starts an anim script and (when blocking) waits for it (0x00B21070).
class SeqAnimScriptCommand : public SequenceCommand {
public:
    SeqAnimScriptCommand(const std::vector<AnimScriptBinding>& bindings, uint32_t scriptHash,
                         uint32_t libraryHash, bool blocking, uint32_t param, bool flag);
    ~SeqAnimScriptCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    std::vector<AnimScriptBinding> m_bindings;          // +0x0C
    uint32_t m_param = 0;                               // +0x20
    uint32_t m_scriptHash = 0;                          // +0x24
    uint32_t m_libraryHash = 0;                         // +0x28
    ape::ApeAnimScriptInstance* m_instance = nullptr;   // +0x2C
    bool m_flag = false;                                // +0x30
};

// Callback used to find a named child of an entity (0x0043B02C).
struct ChildSearch {
    uint32_t nameHash = 0;
    WorldObject* found = nullptr;
};
bool MatchChildByName(WorldObject* node, void* search);

// Sets render flag 8 of an entity or one of its children (0x00B21208).
class SeqSetRenderFlagCommand : public SequenceCommand {
public:
    SeqSetRenderFlagCommand(WorldObject* entity, uint32_t childHash, bool value, bool flag30);
    ~SeqSetRenderFlagCommand() override;
    void Execute() override;
    // @func 0x00780F08 SeqSetRenderFlagCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_childHash = 0;          // +0x10
    bool m_value = false;              // +0x14 bit 31
    bool m_flag30 = false;             // +0x14 bit 30 NOTE: unused
};

// Switches an entity or one of its children on or off (0x00B21310).
class SeqSetActiveCommand : public SequenceCommand {
public:
    SeqSetActiveCommand(WorldObject* entity, uint32_t childHash, bool active, bool children);
    ~SeqSetActiveCommand() override;
    void Execute() override;
    // @func 0x00781478 SeqSetActiveCommand::Update
    bool Update(float /*dt*/) override { return true; }

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_childHash = 0;          // +0x10
    bool m_active = false;             // +0x14 bit 31
    bool m_children = false;           // +0x14 bit 30
};

// Swaps the animation set of a character or mount (0x00B213E8).
class SeqSwapAnimSetCommand : public SequenceCommand {
public:
    SeqSwapAnimSetCommand(uint32_t entityHash, uint32_t animSetHash, bool permanent,
                          bool stopAction, bool blocking);
    ~SeqSwapAnimSetCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;   // +0x0C
    uint32_t m_entityHash = 0;         // +0x10 0: the player
    uint32_t m_animSetHash = 0;        // +0x14
    bool m_permanent = false;          // +0x18
    uint32_t field_1C = 0;             // +0x1C
    bool m_stopAction = false;         // +0x20
};

// Plays a sound, optionally on a sequence entity or through a level sound
// emitter, and waits for it to end ("SeqPlaySound", 0x00B21100).
class SeqPlaySoundCommand : public SequenceCommand {
public:
    SeqPlaySoundCommand(float param14, audio::SoundName* sound, uint32_t emitter,
                        const char* subtitleKey, uint32_t subtitleParam, uint32_t entityHash,
                        audio::AudioBus* bus, bool blocking, bool forceNew,
                        audio::AudioBus* altBus, uint32_t param74, uint32_t param78);
    ~SeqPlaySoundCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    audio::SoundName* m_sound = nullptr;       // +0x0C
    audio::SoundRequest* m_request = nullptr;  // +0x10
    float m_param14 = 0.0f;                    // +0x14 NOTE: stored but unused
    uint32_t m_emitter = 0;                    // +0x18
    uint32_t m_voice = 0;                      // +0x1C audio::VoiceId
    uint32_t m_entityHash = 0;                 // +0x20
    audio::AudioBus* m_bus = nullptr;          // +0x24
    audio::AudioBus* m_altBus = nullptr;       // +0x28
    char m_subtitleKey[0x40] = {};             // +0x2C
    uint32_t m_subtitleParam = 0;              // +0x70
    uint32_t m_param74 = 0;                    // +0x74 copied into the sound params
    uint32_t m_param78 = 0;                    // +0x78
};

// Streams a line of dialog with a speaker portrait (0x00B21058).
// TODO(0x00432AB4 SeqDialogStreamCommand::Execute, 0x00432E08 ::Update): the
// speaker/HUD side is not reconstructed; Execute only marks the command as
// started and Update ends it at once.
class SeqDialogStreamCommand : public SequenceCommand {
public:
    SeqDialogStreamCommand(float time, float time74, audio::SoundName* sound, const char* name,
                           uint32_t param5C, uint32_t param68, uint32_t param6C, bool flag70,
                           uint32_t param78, bool flag7D, bool flag7C, uint32_t param88,
                           const char* textKey, bool forceNew, uint32_t paramCC,
                           uint32_t paramD0, uint32_t paramD4);
    ~SeqDialogStreamCommand() override;

private:
    audio::SoundName* m_sound = nullptr;       // +0x0C
    audio::SoundRequest* m_request = nullptr;  // +0x10
    uint32_t m_voice = 0;                      // +0x14
    char m_name[0x40] = {};                    // +0x18
    uint32_t m_param5C = 0;                    // +0x5C
    float m_time = 0.0f;                       // +0x64
    uint32_t m_param68 = 0;                    // +0x68
    uint32_t m_param6C = 0;                    // +0x6C
    bool m_flag70 = false;                     // +0x70
    float m_time74 = 0.0f;                     // +0x74
    uint32_t m_param78 = 0;                    // +0x78
    bool m_flag7C = false;                     // +0x7C
    bool m_flag7D = false;                     // +0x7D
    float m_remaining = 0.0f;                  // +0x80
    uint32_t m_param88 = 0;                    // +0x88
    char m_textKey[0x40] = {};                 // +0x8C
    uint32_t m_paramCC = 0;                    // +0xCC
    uint32_t m_paramD0 = 0;                    // +0xD0
    uint32_t m_paramD4 = 0;                    // +0xD4
    WorldObject* m_speaker = nullptr;          // +0xD8
};

} // namespace gameplay
} // namespace sly
