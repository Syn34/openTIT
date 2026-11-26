// openTIT - Sequencer commands, second batch (see SequenceCommands2.h).
#include "sly/gameplay/SequenceCommands2.h"
#include "sly/game/Upgrades.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMath.h"
#include "sly/game/WorldInterface.h"
#include "ape/core/ApeString.h"
#include "sly/game/AudioSystem.h"

#include <cmath>
#include <cstring>

namespace sly {
namespace gameplay {

namespace {

// Name hash of the HUD as a whole (the same hash addresses the camera in
// entity commands).
constexpr uint32_t kWholeHudHash = 0xFD81;

// Anim script state every scripted instance ends with ("End").
constexpr uint32_t kAnimStateEnd = ape::AnimHash("End");

// Parameter blocks of the scripted actions (see CharacterStartAction).
// NOTE: console offsets in the comments.
struct Action1Params {
    uint32_t target;      // +0x00
    float value;          // +0x04
    bool flag;            // +0x08
    bool nonBlocking;     // +0x10
    bool wasIdle;         // +0x11 (characters only)
};
struct Action2Params {
    uint32_t target;      // +0x00
    float valueA;         // +0x04
    float valueB;         // +0x08
    bool flagA;           // +0x0C bit 31
    bool flagB;           // +0x0C bit 30
};
struct Action3Params {
    uint32_t target;      // +0x00
    bool flag;            // +0x21
};
struct Action4Params {
    uint32_t target;      // +0x00
    bool nonBlocking;     // +0x0C
};

// Quaternion of Euler angles applied in X, Y, Z order.
ape::Vec4 QuatFromEuler(const ape::Vec3& angles)
{
    const float hx = angles.x * 0.5f;
    const float hy = angles.y * 0.5f;
    const float hz = angles.z * 0.5f;
    const float cx = ape::FastCos(hx), sx = ape::FastSin(hx);
    const float cy = ape::FastCos(hy), sy = ape::FastSin(hy);
    const float cz = ape::FastCos(hz), sz = ape::FastSin(hz);

    ape::Vec4 q;
    q.x = sy * sz * cx + cy * cz * sx;
    q.y = cz * sy * cx + cy * sz * sx;
    q.z = cy * sz * cx - cz * sy * sx;
    q.w = cy * cz * cx - sy * sz * sx;
    return q;
}

// Entity a render/activity command works on: the entity itself or its child
// with the given name.
WorldObject* ResolveChild(WorldObject* entity, uint32_t childHash)
{
    if (childHash != 0) {
        ChildSearch search;
        search.nameHash = childHash;
        WalkEntityChildren(entity, &MatchChildByName, &search);
        if (search.found != nullptr)
            entity = search.found;
    }
    return entity;
}

// Shared Update of the character action commands 2-4.
bool FinishCharacterAction(WorldObject* entity, bool restoreHeadTracking)
{
    if (entity == nullptr || !EntityIsCharacter(entity) || !CharacterHasActiveState(entity))
        return true;
    if (CharacterActionRunning(entity))
        return false;
    if (restoreHeadTracking)
        CharacterEnableHeadTracking(entity, true);
    CharacterActivateState(entity, CharacterStateSlot::Default);
    return true;
}

// Shares a request already playing 'sound' (adding a reference) or queues
// a new one for immediate playback; null for single-part sounds.
audio::SoundRequest* AcquireSoundRequest(audio::SoundName* sound, bool forceNew)
{
    if (!audio::SoundNameHasVariants(&sound))
        return nullptr;

    audio::SoundQueue& queue = audio::Requests();
    audio::SoundRequest* request = nullptr;
    if (!forceNew) {
        request = queue.FindPlaying(&sound);
        if (request != nullptr)
            ++request->refCount;
    }
    if (request == nullptr) {
        audio::SoundParams params;
        request = queue.RequestImmediate(sound, &params);
        audio::SetRequestBusA(params, nullptr);
        audio::SetRequestBusB(params, nullptr);
    }
    if (request != nullptr)
        request->field_65 = true;
    return request;
}

void CopyKey(char* dst, size_t size, const char* src)
{
    dst[0] = '\0';
    if (src != nullptr && src[0] != '\0')
        ape::StrNCopy(dst, src, size);
}

} // namespace

// The bindings go through ApplyScriptBindings (0x0049FCC0). NOTE: the
// console passes the source script instance in `param`; the port's command
// keeps it as a plain word, so `var` bindings have no source here.
void AnimScriptBindVariables(ape::ApeAnimScriptInstance* instance, const std::vector<AnimScriptBinding>& bindings,
                             uint32_t param, bool flag, bool flagB)
{
    (void)param;
    ApplyScriptBindings(instance, bindings.data(), bindings.size(), nullptr, flag, flagB);
}

// ---------------------------------------------------------------------------
// SeqPlaceEntityCommand
// ---------------------------------------------------------------------------

// @func 0x0042CCD4 SeqPlaceEntityCommand::SeqPlaceEntityCommand
SeqPlaceEntityCommand::SeqPlaceEntityCommand(uint32_t entityHash, uint32_t targetHash,
                                             uint32_t nodeHash, const ape::Vec3& offset,
                                             const ape::Vec3& rotation, uint32_t param40,
                                             const ape::Vec3& vector50, const ape::Vec3& vector60,
                                             bool flagA, bool flagB, bool flagC, bool flagD)
    : SequenceCommand(true), m_entityHash(entityHash), m_targetHash(targetHash),
      m_nodeHash(nodeHash), m_offset(offset), m_rotation(rotation), m_param40(param40),
      m_vector50(vector50), m_vector60(vector60), m_flagA(flagA), m_flagB(flagB), m_flagC(flagC),
      m_flagD(flagD)
{
}

// @func 0x0077F758 SeqPlaceEntityCommand::~SeqPlaceEntityCommand
// @func 0x0077F788 SeqPlaceEntityCommand::~SeqPlaceEntityCommand (deleting)
SeqPlaceEntityCommand::~SeqPlaceEntityCommand() = default;

// @func 0x0042CE48 SeqPlaceEntityCommand::Execute
void SeqPlaceEntityCommand::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (m_entity != nullptr) {
        m_target = FindSequenceEntity(m_targetHash);
        if (m_target != nullptr) {
            if (WorldObject* linked = EntityLinkedParent(m_target))
                EntityAttach(linked, m_target);

            EntityPlacement placement;
            placement.position = m_offset;
            placement.rotation = QuatFromEuler(m_rotation);
            placement.nodeHash = m_nodeHash;
            placement.field_28 = m_param40;
            placement.flag31 = m_flagA;
            placement.flag25 = m_flagB;
            placement.flag28 = m_flagC;
            placement.flag29 = m_flagD;
            placement.relativeTo = m_target;
            EntityApplyPlacement(m_entity, placement);
            EntityCommitPlacement(m_entity);
        }
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqSetCameraViewCommand
// ---------------------------------------------------------------------------

// @func 0x0042D20C SeqSetCameraViewCommand::SeqSetCameraViewCommand
SeqSetCameraViewCommand::SeqSetCameraViewCommand(WorldObject* camera, WorldObject* locator,
                                                 float fov)
    : SequenceCommand(true), m_camera(camera), m_locator(locator), m_fov(fov)
{
}

// @func 0x0077F870 SeqSetCameraViewCommand::~SeqSetCameraViewCommand
// @func 0x0077F8A0 SeqSetCameraViewCommand::~SeqSetCameraViewCommand (deleting)
SeqSetCameraViewCommand::~SeqSetCameraViewCommand() = default;

// @func 0x0042D288 SeqSetCameraViewCommand::Execute
void SeqSetCameraViewCommand::Execute()
{
    if (m_camera != nullptr || m_locator != nullptr) {
        ResetCameraManager();

        CameraView view;
        float cameraFov = view.fov;
        if (m_camera != nullptr) {
            // Camera objects look down -Z: flip the X and Z axes.
            if (const ape::Matrix34* m = CameraObjectMatrix(m_camera, &cameraFov)) {
                view.transform = *m;
                ape::Vec3Scale(view.transform.xAxis, m->xAxis, -1.0f);
                ape::Vec3Scale(view.transform.zAxis, m->zAxis, -1.0f);
            }
        } else if (const ape::Matrix34* m = LocatorMatrix(m_locator)) {
            view.transform = *m;
        }

        view.fov = m_fov > 0.0f ? m_fov : cameraFov;
        const uint8_t* color = DefaultCameraColor();
        for (int i = 0; i < 4; ++i)
            view.color[i] = color[i];
        SetCameraView(view);
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqCameraReleaseCommand
// ---------------------------------------------------------------------------

// @func 0x0042D464 SeqCameraReleaseCommand::SeqCameraReleaseCommand
SeqCameraReleaseCommand::SeqCameraReleaseCommand(float blendTime, bool blocking)
    : SequenceCommand(blocking), m_blendTime(blendTime)
{
}

// @func 0x0077F900 SeqCameraReleaseCommand::~SeqCameraReleaseCommand
// @func 0x0077F930 SeqCameraReleaseCommand::~SeqCameraReleaseCommand (deleting)
SeqCameraReleaseCommand::~SeqCameraReleaseCommand() = default;

// @func 0x0042D4C8 SeqCameraReleaseCommand::Execute
void SeqCameraReleaseCommand::Execute()
{
    // With a blend the top camera is popped; without one the first scripted
    // camera on the stack is killed outright.
    for (GameCamera* camera = TopCamera(); camera != nullptr; camera = NextCamera(camera)) {
        if (CameraType(camera) == kCameraTypeScripted) {
            if (m_blendTime > 0.0f)
                PopCameraBlended(m_blendTime);
            else
                KillCamera(camera);
            break;
        }
        if (m_blendTime > 0.0f) {
            PopCameraBlended(m_blendTime);
            break;
        }
    }
    MarkStarted();
}

// @func 0x0042D670 SeqCameraReleaseCommand::Update
bool SeqCameraReleaseCommand::Update(float /*dt*/)
{
    for (GameCamera* camera = TopCamera(); camera != nullptr; camera = NextCamera(camera)) {
        if (camera == m_camera)
            return false;
    }
    m_camera = nullptr;
    return true;
}

// ---------------------------------------------------------------------------
// Character actions
// ---------------------------------------------------------------------------

// @func 0x00431748 SeqCharacterAction1Command::SeqCharacterAction1Command
SeqCharacterAction1Command::SeqCharacterAction1Command(float value, uint32_t entityHash,
                                                       uint32_t target, uint32_t param18,
                                                       bool flag, bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_target(target), m_param18(param18),
      m_value(value), m_flag(flag), m_wasIdle(false)
{
}

// @func 0x007805C8 SeqCharacterAction1Command::~SeqCharacterAction1Command
// @func 0x007805F8 SeqCharacterAction1Command::~SeqCharacterAction1Command (deleting)
SeqCharacterAction1Command::~SeqCharacterAction1Command() = default;

// @func 0x00431800 SeqCharacterAction1Command::Execute
void SeqCharacterAction1Command::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    WorldObject* entity = m_entity;
    if (entity != nullptr) {
        if (EntityIsCharacter(entity)) {
            const bool wasIdle = CharacterActiveStateIs(entity, CharacterStateSlot::Idle);
            CharacterActivateState(entity, CharacterStateSlot::Scripted);
            CharacterEnableHeadTracking(entity, false);
            CharacterEnableBody(entity, true, false);
            CharacterResetMovement(entity);
            if (auto* params = static_cast<Action1Params*>(CharacterStartAction(entity, 1))) {
                params->target = m_target;
                params->value = m_value;
                params->flag = m_flag;
                params->wasIdle = wasIdle;
                params->nonBlocking = !IsBlocking();
                m_wasIdle = wasIdle;
            }
        } else if (EntityIsMount(entity) || EntityIsCreature(entity)) {
            if (auto* params = static_cast<Action1Params*>(EntityStartAction(entity, 1))) {
                params->target = m_target;
                params->value = m_value;
                params->flag = m_flag;
                params->nonBlocking = !IsBlocking();
            }
        }
    }
    MarkStarted();
}

// @func 0x00431BE0 SeqCharacterAction1Command::Update
bool SeqCharacterAction1Command::Update(float /*dt*/)
{
    WorldObject* entity = m_entity;
    if (entity == nullptr)
        return true;

    if (EntityIsCharacter(entity)) {
        if (!CharacterHasActiveState(entity))
            return true;
        if (CharacterActionRunning(entity))
            return false;
        CharacterEnableHeadTracking(entity, true);
        CharacterEnableBody(entity, true, true);
        if (m_wasIdle)
            CharacterReturnToIdle(entity);
        else
            CharacterActivateState(entity, CharacterStateSlot::Default);
        return true;
    }

    if (EntityIsMount(entity) || EntityIsCreature(entity)) {
        if (!EntityHasAction(entity))
            return true;
        if (EntityActionRunning(entity))
            return false;
        EntityFinishAction(entity);
    }
    return true;
}

// @func 0x004306F4 SeqCharacterAction2Command::SeqCharacterAction2Command
SeqCharacterAction2Command::SeqCharacterAction2Command(float valueA, float valueB,
                                                       uint32_t entityHash, uint32_t target,
                                                       uint32_t param18, bool flagA, bool flagB,
                                                       bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_target(target), m_param18(param18),
      m_valueA(valueA), m_valueB(valueB), m_flagA(flagA), m_flagB(flagB)
{
}

// @func 0x007804B8 SeqCharacterAction2Command::~SeqCharacterAction2Command
// @func 0x007804E8 SeqCharacterAction2Command::~SeqCharacterAction2Command (deleting)
SeqCharacterAction2Command::~SeqCharacterAction2Command() = default;

// @func 0x004307CC SeqCharacterAction2Command::Execute
void SeqCharacterAction2Command::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (m_entity != nullptr && EntityIsCharacter(m_entity)) {
        CharacterActivateState(m_entity, CharacterStateSlot::Scripted);
        CharacterEnableHeadTracking(m_entity, false);
        if (auto* params = static_cast<Action2Params*>(CharacterStartAction(m_entity, 2))) {
            params->target = m_target;
            params->valueA = m_valueA;
            params->valueB = m_valueB;
            params->flagA = m_flagA;
            params->flagB = m_flagB;
        }
    }
    MarkStarted();
}

// @func 0x00430940 SeqCharacterAction2Command::Update
bool SeqCharacterAction2Command::Update(float /*dt*/)
{
    return FinishCharacterAction(m_entity, true);
}

// @func 0x0042FACC SeqCharacterAction3Command::SeqCharacterAction3Command
SeqCharacterAction3Command::SeqCharacterAction3Command(uint32_t entityHash, uint32_t target,
                                                       uint32_t param18, bool flag, bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_target(target), m_param18(param18),
      m_flag(flag)
{
}

// @func 0x007803A0 SeqCharacterAction3Command::~SeqCharacterAction3Command
// @func 0x007803D0 SeqCharacterAction3Command::~SeqCharacterAction3Command (deleting)
SeqCharacterAction3Command::~SeqCharacterAction3Command() = default;

// @func 0x0042FB5C SeqCharacterAction3Command::Execute
void SeqCharacterAction3Command::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (m_entity != nullptr && EntityIsCharacter(m_entity)) {
        CharacterActivateState(m_entity, CharacterStateSlot::Scripted);
        CharacterEnableHeadTracking(m_entity, false);
        if (auto* params = static_cast<Action3Params*>(CharacterStartAction(m_entity, 3))) {
            params->target = m_target;
            params->flag = m_flag;
        }
    }
    MarkStarted();
}

// @func 0x0042FCA4 SeqCharacterAction3Command::Update
bool SeqCharacterAction3Command::Update(float /*dt*/)
{
    return FinishCharacterAction(m_entity, true);
}

// @func 0x0042D8BC SeqCharacterAction4Command::SeqCharacterAction4Command
SeqCharacterAction4Command::SeqCharacterAction4Command(uint32_t entityHash, uint32_t target,
                                                       bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_target(target)
{
}

// @func 0x0077FA10 SeqCharacterAction4Command::~SeqCharacterAction4Command
// @func 0x0077FA40 SeqCharacterAction4Command::~SeqCharacterAction4Command (deleting)
SeqCharacterAction4Command::~SeqCharacterAction4Command() = default;

// @func 0x0042D92C SeqCharacterAction4Command::Execute
void SeqCharacterAction4Command::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (m_entity != nullptr && EntityIsCharacter(m_entity)) {
        CharacterActivateState(m_entity, CharacterStateSlot::Scripted);
        if (auto* params = static_cast<Action4Params*>(CharacterStartAction(m_entity, 4))) {
            params->target = m_target;
            params->nonBlocking = !IsBlocking();
        }
    }
    MarkStarted();
}

// @func 0x0042DA04 SeqCharacterAction4Command::Update
bool SeqCharacterAction4Command::Update(float /*dt*/)
{
    return FinishCharacterAction(m_entity, false);
}

// ---------------------------------------------------------------------------
// SeqHitEntityCommand
// ---------------------------------------------------------------------------

// @func 0x0042DAB4 SeqHitEntityCommand::SeqHitEntityCommand
SeqHitEntityCommand::SeqHitEntityCommand(float force, float lift, uint32_t entityHash,
                                         uint32_t partHash, uint32_t param14, int32_t mode)
    : SequenceCommand(true), m_entityHash(entityHash), m_partHash(partHash), m_param14(param14),
      m_force(force), m_lift(lift), m_mode(mode)
{
}

// @func 0x0077FA98 SeqHitEntityCommand::~SeqHitEntityCommand
// @func 0x0077FAC8 SeqHitEntityCommand::~SeqHitEntityCommand (deleting)
SeqHitEntityCommand::~SeqHitEntityCommand() = default;

// @func 0x0042DB60 SeqHitEntityCommand::Execute
void SeqHitEntityCommand::Execute()
{
    WorldObject* victim = FindWorldEntity(m_entityHash);
    if (m_partHash != 0)
        victim = victim != nullptr ? EntityFindChild(victim, m_partHash) : nullptr;

    if (victim != nullptr && EntityIsInWorld(victim)) {
        if (EntityIsCharacter(victim)) {
            HitInfo hit;
            hit.mode = m_mode;
            hit.force = m_force;
            hit.lift = m_lift;

            // Modes 0-2 push the victim away from the entity's root at a
            // 60 degree upward angle; other modes carry no direction.
            WorldObject* root = FindWorldEntity(m_entityHash);
            if (root != nullptr && (m_mode == 0 || m_mode == 1 || m_mode == 2)) {
                const ape::Vec3 rootPos = EntityPosition(root);
                const ape::Vec3 victimPos = EntityPosition(victim);
                const float dx = rootPos.x - victimPos.x;
                const float dz = rootPos.z - victimPos.z;
                const float invLength = 1.0f / std::sqrt(dx * dx + dz * dz);
                const float elevation = 1.0471976f;   // 60 degrees
                const float horizontal = ape::FastCos(elevation);
                hit.direction.x = dx * invLength * horizontal;
                hit.direction.y = ape::FastSin(elevation);
                hit.direction.z = dz * invLength * horizontal;
            }
            CharacterApplyHit(victim, hit);
        } else if (EntityIsActor(victim)) {
            ActorDamage(victim, m_force, m_mode);
        }
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqPlayerControlCommand
// ---------------------------------------------------------------------------

// @func 0x0042E3AC SeqPlayerControlCommand::SeqPlayerControlCommand
SeqPlayerControlCommand::SeqPlayerControlCommand(bool enable, bool refresh)
    : SequenceCommand(true), m_enable(enable), m_refresh(refresh)
{
}

// @func 0x0077FD60 SeqPlayerControlCommand::~SeqPlayerControlCommand
// @func 0x0077FD90 SeqPlayerControlCommand::~SeqPlayerControlCommand (deleting)
SeqPlayerControlCommand::~SeqPlayerControlCommand() = default;

// @func 0x0042E41C SeqPlayerControlCommand::Execute
void SeqPlayerControlCommand::Execute()
{
    WorldObject* player = PlayerCharacter(0);
    if (player != nullptr) {
        if (EntityIsCharacter(player))
            CharacterEnableDefaultState(player, m_enable);
        else if (EntityIsVehicle(player))
            VehicleEnableControl(player, m_enable);
        else if (EntityIsRiding(player))
            RiderEnableControl(player, m_enable);
    }
    if (m_refresh && player != nullptr)
        EntityRefresh(player);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqHudScreenCommand
// ---------------------------------------------------------------------------

// @func 0x0042E618 SeqHudScreenCommand::SeqHudScreenCommand
SeqHudScreenCommand::SeqHudScreenCommand(uint32_t screenHash, bool show, bool setPersistent,
                                         bool persistent, bool skipToEnd, bool restart,
                                         bool blocking)
    : SequenceCommand(blocking), m_screenHash(screenHash), m_show(show),
      m_setPersistent(setPersistent), m_persistent(persistent), m_skipToEnd(skipToEnd),
      m_restart(restart)
{
}

// @func 0x0077FDF0 SeqHudScreenCommand::~SeqHudScreenCommand
// @func 0x0077FE20 SeqHudScreenCommand::~SeqHudScreenCommand (deleting)
SeqHudScreenCommand::~SeqHudScreenCommand() = default;

// @func 0x0042E6C0 SeqHudScreenCommand::Execute
void SeqHudScreenCommand::Execute()
{
    if (m_screenHash == kWholeHudHash) {
        // NOTE: this path never marks the command as started (as in the
        // original), so it is executed again until the sequence moves on.
        if (m_show)
            HudShowAll();
        else
            HudHideAll();
        return;
    }

    if (m_screenHash != 0) {
        // An unknown screen only produced a (compiled out) warning.
        if (HudScreen* screen = FindHudScreenByHash(m_screenHash)) {
            if (m_setPersistent)
                HudScreenSetPersistent(screen, m_persistent);
            if (m_show) {
                if (m_restart)
                    HudScreenRestart(screen);
                HudScreenShow(screen, true);
                if (m_skipToEnd)
                    HudScreenSkipToEnd(screen);
            } else {
                HudScreenShow(screen, false);
            }
        }
    }
    MarkStarted();
}

// @func 0x0042E860 SeqHudScreenCommand::Update
bool SeqHudScreenCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// SeqHudLayerCommand
// ---------------------------------------------------------------------------

// @func 0x0042E868 SeqHudLayerCommand::SeqHudLayerCommand
SeqHudLayerCommand::SeqHudLayerCommand(uint32_t layerHash, bool show, bool setPersistent,
                                       bool persistent, bool restart, bool blocking)
    : SequenceCommand(blocking), m_layerHash(layerHash), m_show(show),
      m_setPersistent(setPersistent), m_persistent(persistent), m_restart(restart)
{
}

// @func 0x0077FE78 SeqHudLayerCommand::~SeqHudLayerCommand
// @func 0x0077FEA8 SeqHudLayerCommand::~SeqHudLayerCommand (deleting)
SeqHudLayerCommand::~SeqHudLayerCommand() = default;

// @func 0x0042E8F8 SeqHudLayerCommand::Execute
void SeqHudLayerCommand::Execute()
{
    if (m_layerHash != 0) {
        if (HudLayer* layer = FindHudLayerByHash(m_layerHash)) {
            if (m_setPersistent)
                HudLayerSetPersistent(layer, m_persistent);
            if (m_show) {
                if (m_restart)
                    HudLayerRestart(layer);
                HudLayerShow(layer, true);
            } else {
                HudLayerShow(layer, false);
            }
        }
    }
    MarkStarted();
}

// @func 0x0042E9E8 SeqHudLayerCommand::Update
bool SeqHudLayerCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// SeqSkinAttachCommand
// ---------------------------------------------------------------------------

// @func 0x0042F588 SeqSkinAttachCommand::SeqSkinAttachCommand
SeqSkinAttachCommand::SeqSkinAttachCommand(uint32_t hostHash, uint32_t childHash,
                                           EntitySkin* skin, uint32_t socket, bool restore)
    : SequenceCommand(true), m_hostHash(hostHash), m_childHash(childHash), m_skin(skin),
      m_socket(socket), m_restore(restore)
{
}

// @func 0x00780200 SeqSkinAttachCommand::~SeqSkinAttachCommand
// @func 0x00780230 SeqSkinAttachCommand::~SeqSkinAttachCommand (deleting)
SeqSkinAttachCommand::~SeqSkinAttachCommand() = default;

// @func 0x0042F620 SeqSkinAttachCommand::Execute
void SeqSkinAttachCommand::Execute()
{
    // NOTE: every early return below leaves the command unstarted, as in the
    // original; it is retried next frame.
    WorldObject* host = FindSequenceEntity(m_hostHash);
    if (host == nullptr)
        return;

    EntitySkin* skin = nullptr;
    EntitySkin* previous = nullptr;
    if (m_skin == nullptr) {
        skin = EntityActiveSkin(host);
        if (skin == nullptr)
            return;
        previous = SkinPrevious(skin);
        if (previous == nullptr)
            return;
    } else {
        if (EntitySkin* current = EntityActiveSkin(host))
            previous = SkinPrevious(current);
        if (!EntitySwitchSkin(host, m_skin))
            return;
        skin = EntityActiveSkin(host);
        if (skin == nullptr)
            return;
    }

    WorldObject* child = FindSequenceEntity(m_childHash);
    if (child == nullptr || !EntityAcceptsAttachments(child))
        return;

    SkinAttach(skin, child, m_socket);
    if (m_restore) {
        if (previous == nullptr) {
            EntityRemoveSkin(host, skin);
            EntitySwitchSkin(host, nullptr);
        } else {
            EntitySwitchSkin(host, previous);
        }
    }
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqPlayClipCommand
// ---------------------------------------------------------------------------

// @func 0x004321A4 SeqPlayClipCommand::SeqPlayClipCommand
SeqPlayClipCommand::SeqPlayClipCommand(float blendIn, float speed, uint32_t entityHash,
                                       uint32_t clipHash, uint32_t waitClipHash, uint32_t param,
                                       bool loop, bool sequenceAnimated, bool blocking,
                                       bool showFirst, bool restoreShown)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_clipHash(clipHash),
      m_waitClipHash(waitClipHash), m_blendIn(blendIn), m_speed(speed), m_param(param),
      m_loop(loop), m_sequenceAnimated(sequenceAnimated), m_showFirst(showFirst),
      m_restoreShown(restoreShown)
{
}

// @func 0x004322AC SeqPlayClipCommand::~SeqPlayClipCommand
// @func 0x004322DC SeqPlayClipCommand::~SeqPlayClipCommand (deleting)
SeqPlayClipCommand::~SeqPlayClipCommand() = default;

// @func 0x00432338 SeqPlayClipCommand::Execute
void SeqPlayClipCommand::Execute()
{
    m_entity = FindSequenceEntity(m_entityHash);
    if (WorldObject* entity = m_entity) {
        const bool played = EntityPlayClip(entity, m_clipHash, m_waitClipHash, m_blendIn, m_speed,
                                           m_loop, m_param);
        if (!played) {
            EntityClipFailed(entity);
        } else {
            m_wasShown = EntityIsShown(entity);
            if (m_showFirst)
                EntitySetShown(entity, true);
            EntitySetSequenceAnimated(entity, m_sequenceAnimated);
            if (EntityIsCharacter(entity))
                CharacterResetAnimState(entity);
        }
        EntityResetAnimTime(entity);
    }
    MarkStarted();
}

// @func 0x004325C0 SeqPlayClipCommand::Update
bool SeqPlayClipCommand::Update(float /*dt*/)
{
    if (m_entity == nullptr)
        return true;
    if (!EntityClipDone(m_entity, m_waitClipHash))
        return false;
    if (m_restoreShown) {
        EntitySetShown(m_entity, m_wasShown);
        EntitySetSequenceAnimated(m_entity, false);
    }
    return true;
}

// ---------------------------------------------------------------------------
// SeqAnimScriptCommand
// ---------------------------------------------------------------------------

// @func 0x00433120 SeqAnimScriptCommand::SeqAnimScriptCommand
SeqAnimScriptCommand::SeqAnimScriptCommand(const std::vector<AnimScriptBinding>& bindings,
                                           uint32_t scriptHash, uint32_t libraryHash,
                                           bool blocking, uint32_t param, bool flag)
    : SequenceCommand(blocking), m_bindings(bindings), m_param(param), m_scriptHash(scriptHash),
      m_libraryHash(libraryHash), m_flag(flag)
{
}

// @func 0x007808F8 SeqAnimScriptCommand::~SeqAnimScriptCommand
// @func 0x00780960 SeqAnimScriptCommand::~SeqAnimScriptCommand (deleting)
SeqAnimScriptCommand::~SeqAnimScriptCommand() = default;

// @func 0x004332B0 SeqAnimScriptCommand::Execute
void SeqAnimScriptCommand::Execute()
{
    ape::ApeAnimScript* script =
        ape::ApeAnimScriptLibrary::FindScriptAnywhere(m_scriptHash, m_libraryHash, true);
    if (script != nullptr) {
        m_instance = new ape::ApeAnimScriptInstance(script);
        AnimScriptBindVariables(m_instance, m_bindings, m_param, true, m_flag);
        // Scripts nobody waits for are dropped as soon as they finish.
        m_instance->SetRemoveWhenFinished(!IsBlocking());
        ape::ApeAnimScriptInstance::Register(m_instance);
    }
    MarkStarted();
}

// @func 0x004333A4 SeqAnimScriptCommand::Update
bool SeqAnimScriptCommand::Update(float /*dt*/)
{
    if (m_instance == nullptr)
        return true;
    // NOTE: the frame the script finishes still reports "busy"; the command
    // completes on the next update.
    if (m_instance->IsFinished(false)) {
        m_instance->SetState(kAnimStateEnd);
        m_instance->SetRemoveWhenFinished(true);
        m_instance = nullptr;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Child lookup and render/activity commands
// ---------------------------------------------------------------------------

// @func 0x0043B02C MatchChildByName
bool MatchChildByName(WorldObject* node, void* user)
{
    auto* search = static_cast<ChildSearch*>(user);
    const uint32_t key = WorldObjectKey(node);
    bool match = key == search->nameHash;
    // Objects without a registered key are compared by their name.
    if (!match && key == 0)
        match = ape::HashNameNoCase(WorldObjectName(node)) == search->nameHash;
    if (match) {
        search->found = node;
        return false;   // stop the walk
    }
    return true;
}

// @func 0x00438348 SeqSetRenderFlagCommand::SeqSetRenderFlagCommand
SeqSetRenderFlagCommand::SeqSetRenderFlagCommand(WorldObject* entity, uint32_t childHash,
                                                 bool value, bool flag30)
    : SequenceCommand(true), m_entity(entity), m_childHash(childHash), m_value(value),
      m_flag30(flag30)
{
}

// @func 0x00780E80 SeqSetRenderFlagCommand::~SeqSetRenderFlagCommand
// @func 0x00780EB0 SeqSetRenderFlagCommand::~SeqSetRenderFlagCommand (deleting)
SeqSetRenderFlagCommand::~SeqSetRenderFlagCommand() = default;

// @func 0x004383E0 SeqSetRenderFlagCommand::Execute
void SeqSetRenderFlagCommand::Execute()
{
    if (WorldObject* entity = ResolveChild(m_entity, m_childHash))
        EntitySetRenderFlag8(entity, m_value);
    MarkStarted();
}

// @func 0x004390AC SeqSetActiveCommand::SeqSetActiveCommand
SeqSetActiveCommand::SeqSetActiveCommand(WorldObject* entity, uint32_t childHash, bool active,
                                         bool children)
    : SequenceCommand(true), m_entity(entity), m_childHash(childHash), m_active(active),
      m_children(children)
{
}

// @func 0x007813F0 SeqSetActiveCommand::~SeqSetActiveCommand
// @func 0x00781420 SeqSetActiveCommand::~SeqSetActiveCommand (deleting)
SeqSetActiveCommand::~SeqSetActiveCommand() = default;

// @func 0x00439144 SeqSetActiveCommand::Execute
void SeqSetActiveCommand::Execute()
{
    if (WorldObject* entity = ResolveChild(m_entity, m_childHash))
        EntitySetActive(entity, m_active, m_children);
    MarkStarted();
}

// ---------------------------------------------------------------------------
// SeqSwapAnimSetCommand
// ---------------------------------------------------------------------------

// @func 0x0043A84C SeqSwapAnimSetCommand::SeqSwapAnimSetCommand
SeqSwapAnimSetCommand::SeqSwapAnimSetCommand(uint32_t entityHash, uint32_t animSetHash,
                                             bool permanent, bool stopAction, bool blocking)
    : SequenceCommand(blocking), m_entityHash(entityHash), m_animSetHash(animSetHash),
      m_permanent(permanent), m_stopAction(stopAction)
{
}

// @func 0x007818B8 SeqSwapAnimSetCommand::~SeqSwapAnimSetCommand
// @func 0x007818E8 SeqSwapAnimSetCommand::~SeqSwapAnimSetCommand (deleting)
SeqSwapAnimSetCommand::~SeqSwapAnimSetCommand() = default;

// @func 0x0043A8E4 SeqSwapAnimSetCommand::Execute
void SeqSwapAnimSetCommand::Execute()
{
    if (m_animSetHash != 0) {
        m_entity = m_entityHash != 0 ? FindSequenceEntity(m_entityHash) : PlayerCharacter(0);
        WorldObject* entity = m_entity;
        if (entity != nullptr) {
            if (EntityIsCharacter(entity)) {
                if (void* animSet = FindAnimSet(m_animSetHash)) {
                    CharacterSwapAnimSet(entity, animSet, !m_permanent);
                    if (!m_permanent)
                        EntitySetAnimSetOverridden(entity, true);
                    if (m_permanent && m_stopAction) {
                        CharacterStopAction(entity);
                        CharacterRefreshBody(entity);
                    }
                }
            } else if (EntityIsMount(entity)) {
                if (void* animSet = FindAnimSet(m_animSetHash))
                    MountSwapAnimSet(entity, animSet, !m_permanent);
            }
        }
    }
    MarkStarted();
}

// @func 0x0043AAB8 SeqSwapAnimSetCommand::Update
bool SeqSwapAnimSetCommand::Update(float /*dt*/)
{
    // Wait while the character is in behaviour state 5.
    // NOTE: state 5 is presumably the anim-set transition state.
    WorldObject* entity = m_entity;
    if (entity == nullptr || !EntityIsCharacter(entity))
        return true;
    if (CharacterStateId(entity) != 5)
        EntitySetAnimSetOverridden(entity, false);
    return CharacterStateId(entity) != 5;
}

// ---------------------------------------------------------------------------
// SeqPlaySoundCommand
// ---------------------------------------------------------------------------

// @func 0x004342DC SeqPlaySoundCommand::SeqPlaySoundCommand
SeqPlaySoundCommand::SeqPlaySoundCommand(float param14, audio::SoundName* sound, uint32_t emitter,
                                         const char* subtitleKey, uint32_t subtitleParam,
                                         uint32_t entityHash, audio::AudioBus* bus,
                                         bool blocking, bool forceNew, audio::AudioBus* altBus,
                                         uint32_t param74, uint32_t param78)
    : SequenceCommand(blocking), m_sound(sound), m_param14(param14), m_emitter(emitter),
      m_entityHash(entityHash), m_bus(bus), m_altBus(altBus), m_subtitleParam(subtitleParam),
      m_param74(param74), m_param78(param78)
{
    CopyKey(m_subtitleKey, sizeof(m_subtitleKey), subtitleKey);
    if (m_sound == nullptr)
        return;
    audio::SoundNameAddRef(m_sound);
    m_request = AcquireSoundRequest(m_sound, forceNew);
    m_voice = 0;
}

// @func 0x00434598 SeqPlaySoundCommand::~SeqPlaySoundCommand
// @func 0x004345F8 SeqPlaySoundCommand::~SeqPlaySoundCommand (deleting)
SeqPlaySoundCommand::~SeqPlaySoundCommand()
{
    audio::SoundNameRelease(m_sound);
    audio::Requests().Release(m_request);
}

// @func 0x00434674 SeqPlaySoundCommand::Execute
void SeqPlaySoundCommand::Execute()
{
    audio::SoundParams params;
    const float volume = 1.0f;
    std::memcpy(&params.field_20, &volume, sizeof(volume));
    params.field_30 = m_param74;
    params.field_34 = m_param78;

    if (m_emitter != 0) {
        EmitterStart(m_emitter);
    } else {
        WorldObject* entity = nullptr;
        audio::AudioBus* bus = m_bus;
        if (m_entityHash != 0) {
            // An unknown entity only produced a warning ("SeqPlaySound
            // references unknown entity %s"); the sound then plays in 2D.
            entity = FindSequenceEntity(m_entityHash);
            if (entity != nullptr && EntityUsesAltSoundBus(entity))
                bus = m_altBus;
        }

        if (m_request == nullptr) {
            m_voice = entity == nullptr ? PlaySound2D(m_sound, params)
                                        : EntityPlaySound(entity, m_sound, params, bus);
        } else {
            StreamSystemIdle(m_request);
            if (!m_request->hasVoice)
                PrepareSoundRequest();
            m_voice = entity == nullptr ? PlaySoundRequest(m_request, params)
                                        : EntityPlaySoundRequest(entity, m_request, params, bus);
        }
    }

    if (m_subtitleKey[0] != '\0' && SubtitlesEnabled())
        ShowSubtitle(m_subtitleKey, m_subtitleParam, m_request);
    MarkStarted();

    audio::SetRequestBusA(params, nullptr);
    audio::SetRequestBusB(params, nullptr);
}

// @func 0x00434950 SeqPlaySoundCommand::Update
bool SeqPlaySoundCommand::Update(float /*dt*/)
{
    if (m_emitter != 0)
        return !EmitterPlaying(m_emitter);

    if (audio::IsVoicePlaying(m_voice))
        return false;
    audio::StopVoice(m_voice, true);
    audio::Requests().Release(m_request);
    m_request = nullptr;
    m_voice = 0;
    return true;
}

// ---------------------------------------------------------------------------
// SeqDialogStreamCommand
// ---------------------------------------------------------------------------

// @func 0x0043267C SeqDialogStreamCommand::SeqDialogStreamCommand
SeqDialogStreamCommand::SeqDialogStreamCommand(float time, float time74, audio::SoundName* sound,
                                               const char* name, uint32_t param5C,
                                               uint32_t param68, uint32_t param6C, bool flag70,
                                               uint32_t param78, bool flag7D, bool flag7C,
                                               uint32_t param88, const char* textKey,
                                               bool forceNew, uint32_t paramCC, uint32_t paramD0,
                                               uint32_t paramD4)
    : SequenceCommand(true), m_sound(sound), m_param5C(param5C), m_time(time),
      m_param68(param68), m_param6C(param6C), m_flag70(flag70), m_time74(time74),
      m_param78(param78), m_flag7C(flag7C), m_flag7D(flag7D), m_remaining(time),
      m_param88(param88), m_paramCC(paramCC), m_paramD0(paramD0), m_paramD4(paramD4)
{
    CopyKey(m_name, sizeof(m_name), name);
    if (m_sound != nullptr)
        audio::SoundNameAddRef(m_sound);
    m_request = AcquireSoundRequest(m_sound, forceNew);
    m_voice = 0;

    m_textKey[0] = '\0';
    if (textKey != nullptr && textKey[0] != '\0') {
        ape::StrNCopy(m_textKey, textKey, sizeof(m_textKey));
        m_textKey[sizeof(m_textKey) - 1] = '\0';
    }
}

// @func 0x004329CC SeqDialogStreamCommand::~SeqDialogStreamCommand
// @func 0x00432A2C SeqDialogStreamCommand::~SeqDialogStreamCommand (deleting)
SeqDialogStreamCommand::~SeqDialogStreamCommand()
{
    audio::SoundNameRelease(m_sound);
    audio::Requests().Release(m_request);
}

} // namespace gameplay
} // namespace sly
