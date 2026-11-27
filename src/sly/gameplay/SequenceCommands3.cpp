// openTIT - Destructors and trivial slots of the unported sequence commands
// (see SequenceCommands3.h).
#include "sly/gameplay/SequenceCommands3.h"

#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "crt/libc/Libc.h"
#include "sly/gameplay/GameplayWorld.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace gameplay {

namespace {

// Up axis the look-at camera keeps level against. NOTE: the console reads it
// from 0x01164B60, filled by a static initialiser that was not found; the
// world is Y-up.
constexpr ape::Vec3 kLookAtUp{0.0f, 1.0f, 0.0f, 0.0f};
constexpr float kLookAtDefaultFov = 53.5f;
constexpr float kRadToDeg = 57.2957763671875f;

ape::Vec3 Normalized(const ape::Vec3& v)
{
    const float inv = 1.0f / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {v.x * inv, v.y * inv, v.z * inv, v.pad};
}

} // namespace

// ---------------------------------------------------------------------------
// SeqLookAtCommand
// ---------------------------------------------------------------------------

// @func 0x0042FF28 SeqLookAtCommand::SeqLookAtCommand
SeqLookAtCommand::SeqLookAtCommand(float duration, float turnRate, float fov, uint32_t entityHash,
                                   uint32_t targetHash, uint32_t entityPointHash,
                                   uint32_t targetPointHash)
    : SequenceCommand(true), m_entityHash(entityHash), m_targetHash(targetHash),
      m_entityPointHash(entityPointHash), m_targetPointHash(targetPointHash),
      m_duration(duration), m_turnRate(turnRate), m_fov(fov)
{
}

// @func 0x00430050 SeqLookAtCommand::~SeqLookAtCommand
// @func 0x00430080 SeqLookAtCommand::~SeqLookAtCommand (deleting)
SeqLookAtCommand::~SeqLookAtCommand() = default;

// Resolves the start and the target (point instances first, then sequence
// entities) and takes the camera's start position and orientation from the
// start object.
// @func 0x004300D8 SeqLookAtCommand::Execute
void SeqLookAtCommand::Execute()
{
    using ape::world::ApePointInstance;
    using ape::world::ApeTInstanceList;

    m_entityPoint = ApeTInstanceList<ApePointInstance>::Find(m_entityPointHash);
    bool resolved = m_entityPoint != nullptr;
    if (!resolved) {
        m_entity = FindSequenceEntity(m_entityHash);
        resolved = m_entity != nullptr;
    }
    if (resolved) {
        m_targetPoint = ApeTInstanceList<ApePointInstance>::Find(m_targetPointHash);
        if (m_targetPoint == nullptr) {
            m_target = FindSequenceEntity(m_targetHash);
            resolved = m_target != nullptr;
        }
    }
    if (resolved) {
        const ape::Matrix34 start =
            m_entityPoint != nullptr ? m_entityPoint->GetLocal() : EntityTransform(m_entity);
        m_position = start.translation;
        ape::QuatFromMatrix34(m_rotation, start);
    }
    ResetCameraManager();
    MarkStarted();
}

// @func 0x004302C0 SeqLookAtCommand::Update
bool SeqLookAtCommand::Update(float dt)
{
    ape::Matrix34 view;
    ape::Matrix34FromQuat(view, m_rotation, m_position);

    if (m_target != nullptr || m_targetPoint != nullptr) {
        const ape::Vec3 targetPos =
            m_targetPoint != nullptr ? m_targetPoint->GetLocal().translation : EntityPosition(m_target);
        const ape::Vec3 toTarget = Normalized({targetPos.x - m_position.x, targetPos.y - m_position.y,
                                               targetPos.z - m_position.z, 0.0f});
        ape::Vec3 forward = toTarget;
        if (m_turnRate > 0.0f && !m_aligned) {
            // Turn by at most turnRate full turns per second.
            const float cosine = std::clamp(ape::Vec3Dot(toTarget, view.zAxis), -1.0f, 1.0f);
            const float angle = crt::ACosF(cosine) * kRadToDeg;
            const float maxStep = dt * m_turnRate * 360.0f;
            if (angle <= maxStep) {
                m_aligned = true;
            } else {
                const float t = std::clamp(maxStep / angle, 0.0f, 1.0f);
                const ape::Vec3& z = view.zAxis;
                forward = Normalized({z.x + (toTarget.x - z.x) * t, z.y + (toTarget.y - z.y) * t,
                                      z.z + (toTarget.z - z.z) * t, 0.0f});
            }
        }
        // Rebuild a level basis around the new forward axis.
        view.zAxis = forward;
        ape::Vec3Cross(view.xAxis, kLookAtUp, forward);
        ape::Vec3Cross(view.yAxis, forward, view.xAxis);
        ape::Matrix34NormalizeAxes(view);
        ape::QuatFromMatrix34(m_rotation, view);
    }

    CameraView cameraView;
    cameraView.transform = view;
    cameraView.fov = m_fov > 0.0f ? m_fov : kLookAtDefaultFov;
    const uint8_t* color = DefaultCameraColor();
    std::copy(color, color + 4, cameraView.color);
    SetCameraView(cameraView);

    // NOTE: only a target entity makes the command wait; a target point
    // finishes after the first frame.
    if (m_target == nullptr)
        return true;
    if (m_elapsed > m_duration)
        return true;
    m_elapsed += dt;
    return false;
}

// ---------------------------------------------------------------------------
// SeqMoveCameraCommand
// ---------------------------------------------------------------------------

namespace {

// Interpolates between unit quaternions along the shorter arc; nearly equal
// rotations are blended linearly (unnormalised, as on the console).
ape::Quat QuatSlerp(const ape::Quat& from, const ape::Quat& to, float t)
{
    float cosine = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
    ape::Quat target = to;
    if (cosine < 0.0f) {
        cosine = -cosine;
        target = {-to.x, -to.y, -to.z, -to.w};
    }
    if (cosine > 0.99f) {
        return {from.x + (target.x - from.x) * t, from.y + (target.y - from.y) * t,
                from.z + (target.z - from.z) * t, from.w + (target.w - from.w) * t};
    }
    // Unit quaternion orthogonal to `from` in the plane of both.
    ape::Quat ortho{target.x - from.x * cosine, target.y - from.y * cosine,
                    target.z - from.z * cosine, target.w - from.w * cosine};
    const float inv = 1.0f / std::sqrt(ortho.x * ortho.x + ortho.y * ortho.y + ortho.z * ortho.z +
                                       ortho.w * ortho.w);
    const float angle = t * crt::ACosF(std::clamp(cosine, -1.0f, 1.0f));
    const float c = ape::FastCos(angle);
    const float s = ape::FastSin(angle) * inv;
    return {from.x * c + ortho.x * s, from.y * c + ortho.y * s, from.z * c + ortho.z * s,
            from.w * c + ortho.w * s};
}

// Fraction of a timed phase; phases without a duration are complete.
float PhaseFraction(float elapsed, float duration)
{
    return duration > 0.0f ? elapsed / duration : 1.0f;
}

float Advance(float elapsed, float dt, float duration)
{
    return std::max(std::min(elapsed + dt, duration), 0.0f);
}

constexpr float kRotationTolerance = 1e-5f;

void PushCameraView(const ape::Matrix34& transform, float fov)
{
    CameraView view;
    view.transform = transform;
    view.fov = fov;
    const uint8_t* color = DefaultCameraColor();
    std::copy(color, color + 4, view.color);
    SetCameraView(view);
}

} // namespace

// @func 0x00430A64 SeqMoveCameraCommand::SeqMoveCameraCommand
SeqMoveCameraCommand::SeqMoveCameraCommand(float fov, float fovTime, float moveDelay,
                                           float moveTime, float turnTime, uint32_t startPoint,
                                           uint32_t endPoint, uint32_t lookPoint)
    : SequenceCommand(true), m_targetFov(fov), m_fovTime(fovTime), m_moveDelay(moveDelay),
      m_moveTime(moveTime), m_turnTime(turnTime)
{
    using ape::world::ApePointInstance;
    using ape::world::ApeTInstanceList;
    m_startPoint = ApeTInstanceList<ApePointInstance>::Find(startPoint);
    m_endPoint = ApeTInstanceList<ApePointInstance>::Find(endPoint);
    m_lookPoint = ApeTInstanceList<ApePointInstance>::Find(lookPoint);
}

// @func 0x00780540 SeqMoveCameraCommand::~SeqMoveCameraCommand
// @func 0x00780570 SeqMoveCameraCommand::~SeqMoveCameraCommand (deleting)
SeqMoveCameraCommand::~SeqMoveCameraCommand() = default;

// Takes over the camera at the start point (or where the camera is) and
// works out the orientation that faces the look point.
// @func 0x00430BE8 SeqMoveCameraCommand::Execute
void SeqMoveCameraCommand::Execute()
{
    if (HasCameraManager()) {
        ResetCameraManager();
        ape::Matrix34 start = m_startPoint != nullptr ? m_startPoint->GetLocal() : CameraManagerTransform();
        if (!ape::Matrix34RotationEqual(kRotationTolerance, start, ape::world::kWorldIdentity))
            ape::Matrix34NormalizeAxes(start);

        const float cameraFov = CameraManagerFov();
        m_startFov = cameraFov;
        m_startPosition = start.translation;
        ape::QuatFromMatrix34(m_startRotation, start);

        if (m_lookPoint != nullptr) {
            const ape::Vec3& target = m_lookPoint->GetLocal().translation;
            ape::Matrix34 look = ape::world::kWorldIdentity;
            look.zAxis = Normalized({target.x - m_startPosition.x, target.y - m_startPosition.y,
                                     target.z - m_startPosition.z, 0.0f});
            ape::Vec3Cross(look.xAxis, kLookAtUp, look.zAxis);
            ape::Vec3Cross(look.yAxis, look.zAxis, look.xAxis);
            ape::Matrix34NormalizeAxes(look);
            ape::QuatFromMatrix34(m_lookRotation, look);
        }
        PushCameraView(start, cameraFov);
    }
    MarkStarted();
}

// Blends the field of view, the position (after the move delay) and the
// orientation towards their targets; done once every phase has run out.
// @func 0x00431030 SeqMoveCameraCommand::Update
bool SeqMoveCameraCommand::Update(float dt)
{
    if (HasCameraManager()) {
        ape::Matrix34 view = CameraManagerTransform();
        float fov = CameraManagerFov();
        if (m_targetFov > 0.0f) {
            const float t = std::clamp(PhaseFraction(m_fovElapsed, m_fovTime), 0.0f, 1.0f);
            fov = m_startFov + (m_targetFov - m_startFov) * t;
        }
        if (m_endPoint != nullptr) {
            const float t = std::clamp(PhaseFraction(m_moveElapsed, m_moveTime), 0.0f, 1.0f);
            const ape::Vec3& end = m_endPoint->GetLocal().translation;
            view.translation = {m_startPosition.x + (end.x - m_startPosition.x) * t,
                                m_startPosition.y + (end.y - m_startPosition.y) * t,
                                m_startPosition.z + (end.z - m_startPosition.z) * t, view.translation.pad};
        }
        if (m_lookPoint != nullptr) {
            const ape::Quat rotation =
                QuatSlerp(m_startRotation, m_lookRotation, PhaseFraction(m_turnElapsed, m_turnTime));
            const ape::Vec3 position = view.translation;
            ape::Matrix34FromQuat(view, rotation, position);
        }
        if (!ape::Matrix34RotationEqual(kRotationTolerance, view, ape::world::kWorldIdentity))
            ape::Matrix34NormalizeAxes(view);
        PushCameraView(view, fov);

        m_fovElapsed = Advance(m_fovElapsed, dt, m_fovTime);
        if (m_delayElapsed >= m_moveDelay)
            m_moveElapsed = Advance(m_moveElapsed, dt, m_moveTime);
        m_delayElapsed = Advance(m_delayElapsed, dt, m_moveDelay);
        m_turnElapsed = Advance(m_turnElapsed, dt, m_turnTime);
    }
    return m_fovElapsed >= m_fovTime && m_delayElapsed >= m_moveDelay && m_moveElapsed >= m_moveTime &&
           m_turnElapsed >= m_turnTime;
}

// @func 0x0077F5F8 SeqArcQteCommand::~SeqArcQteCommand
// @func 0x00780B78 SeqArcQteCommand::~SeqArcQteCommand (deleting)
SeqArcQteCommand::~SeqArcQteCommand() = default;

// @func 0x0077F628 SeqActorQteCommand::~SeqActorQteCommand
// @func 0x00780BD0 SeqActorQteCommand::~SeqActorQteCommand (deleting)
SeqActorQteCommand::~SeqActorQteCommand() = default;

// @func 0x00780DF8 SeqHudTypeCommand::~SeqHudTypeCommand
// @func 0x00780E28 SeqHudTypeCommand::~SeqHudTypeCommand (deleting)
SeqHudTypeCommand::~SeqHudTypeCommand() = default;

// @func 0x00781480 SeqPlacementCommand::~SeqPlacementCommand
// @func 0x007814B0 SeqPlacementCommand::~SeqPlacementCommand (deleting)
SeqPlacementCommand::~SeqPlacementCommand() = default;

// @func 0x00781508 SeqPlacementCommand::Update
bool SeqPlacementCommand::Update(float dt)
{
    (void)dt;
    return true;
}

// @func 0x007816B0 SeqSpawnEntityCommand::~SeqSpawnEntityCommand
// @func 0x007816E0 SeqSpawnEntityCommand::~SeqSpawnEntityCommand (deleting)
SeqSpawnEntityCommand::~SeqSpawnEntityCommand() = default;

// @func 0x00781738 SeqSpawnEntityCommand::Update
bool SeqSpawnEntityCommand::Update(float dt)
{
    (void)dt;
    return true;
}

} // namespace gameplay
} // namespace sly
