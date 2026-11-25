// openTIT - <FacePlayer> component (vtable 0x00B1C730).
#include "sly/components/FacePlayer.h"

#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentHooks.h"
#include "sly/entity/EntityHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"

#include <cmath>

namespace sly {
namespace components {

namespace {

constexpr float kEpsilon = 1e-5f;
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};      // 0x01164B40
const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};        // 0x01164B60
const ape::Vec3 kForward = {0.0f, 0.0f, 1.0f, 0.0f};   // 0x01164B70 (assumed)

ape::Matrix34 Identity()
{
    ape::Matrix34 m{};   // 0x01164BD0
    m.xAxis.x = m.yAxis.y = m.zAxis.z = 1.0f;
    return m;
}

ape::Vec3 Cross(const ape::Vec3& a, const ape::Vec3& b)
{
    ape::Vec3 out{};
    ape::Vec3Cross(out, a, b);
    return out;
}

float Dot(const ape::Vec3& a, const ape::Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

bool Differs(const ape::Vec3& a, const ape::Vec3& b)
{
    return std::fabs(a.x - b.x) >= kEpsilon || std::fabs(a.y - b.y) >= kEpsilon || std::fabs(a.z - b.z) >= kEpsilon;
}

ape::Vec3 Negated(const ape::Vec3& v) { return {-v.x, -v.y, -v.z, 0.0f}; }

// Inverse of a transform with orthonormal axes.
ape::Matrix34 InverseOrtho(const ape::Matrix34& m)
{
    ape::Matrix34 inv{};
    inv.xAxis = {m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f};
    inv.yAxis = {m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f};
    inv.zAxis = {m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f};
    const ape::Vec3& t = m.translation;
    inv.translation = {-Dot(t, m.xAxis), -Dot(t, m.yAxis), -Dot(t, m.zAxis), 0.0f};
    return inv;
}

// `v` rotated by the axes of `m` (row vector times the 3x3 part).
ape::Vec3 RotateVector(const ape::Vec3& v, const ape::Matrix34& m)
{
    return {v.x * m.xAxis.x + v.y * m.yAxis.x + v.z * m.zAxis.x,
            v.x * m.xAxis.y + v.y * m.yAxis.y + v.z * m.zAxis.y,
            v.x * m.xAxis.z + v.y * m.yAxis.z + v.z * m.zAxis.z, 0.0f};
}

} // namespace

// @func 0x0025C090 FacePlayerComponent::FacePlayerComponent
FacePlayerComponent::FacePlayerComponent(const FacePlayerComponentDesc* desc) : EntityComponent(desc)
{
    rest = previous = output = Identity();
    facing = kForward;
    rotSpeed = desc->rotSpeed;
    lockY = desc->lockY;
    snapToAxes = desc->flag1D;
    turn = kTurn_None;
    partObject = nullptr;
    bone = nullptr;
    isBone = false;
    ape::world::ApeObj* object = owner ? owner->object : nullptr;
    if (!object)
        return;
    bone = owner->GetBoneByName(desc->part);
    if (bone) {
        isBone = true;
        parentBone = owner->GetBone(bone->def->parent);
        ape::world::Matrix34Mul(rest, *bone->model, object->GetWorld());
    } else {
        partObject = entity::EntityFindObject(owner, desc->part);
        ape::Matrix34Copy(rest, partObject ? partObject->GetWorld() : object->GetWorld());
    }
    previous = rest;
    output = rest;
    facing = rest.zAxis;
}

// @func 0x0028F2B0 FacePlayerComponent::~FacePlayerComponent
// @func 0x0028F3E4 FacePlayerComponent::~FacePlayerComponent (deleting)
FacePlayerComponent::~FacePlayerComponent()
{
    DequeueWorldCallback(1, reinterpret_cast<void*>(&ApplyCallback), this, owner);
}

// @func 0x0028F588 FacePlayerComponent::PostUpdate
void FacePlayerComponent::PostUpdate(float dt)
{
    auto* script = owner ? static_cast<ape::ApeAnimScriptInstance*>(owner->GetAnimScript()) : nullptr;
    if (entity::Entity* player = entity::PlayerEntity(0)) {
        // NOTE: type bit 0 selects the aim point over the position.
        const ape::Vec3 target = (player->typeBits[0] & 1u) ? player->GetAimPoint() : player->Position();
        const bool planar = lockY || snapToAxes;
        ape::Vec3 dir = {target.x - rest.translation.x, target.y - rest.translation.y,
                         target.z - rest.translation.z, 0.0f};
        if (planar)
            dir.y = 0.0f;
        const float lengthSq = Dot(dir, dir);
        if (lengthSq >= kEpsilon) {
            const float inv = 1.0f / std::sqrt(lengthSq);
            dir = {dir.x * inv, dir.y * inv, dir.z * inv, 0.0f};
        } else {
            dir = previous.zAxis;
        }

        if (snapToAxes) {
            // Pick the rest axis (x or z, either sign) closest to the player.
            ape::Vec3 local = {Dot(rest.xAxis, dir), Dot(rest.yAxis, dir), Dot(rest.zAxis, dir), 0.0f};
            const float localSq = Dot(local, local);
            if (localSq >= kEpsilon) {
                const float inv = 1.0f / std::sqrt(localSq);
                local.x *= inv;
                local.z *= inv;
            }
            static const float s_snapAngle = 1.0471976f;   // 60 degrees (0x01068080)
            const float cosine = std::fabs(local.x) < 1.0f ? std::fabs(local.x) : 1.0f;
            if (std::acos(cosine) < s_snapAngle)
                dir = local.x >= 0.0f ? rest.xAxis : Negated(rest.xAxis);
            else
                dir = local.z >= 0.0f ? rest.zAxis : Negated(rest.zAxis);
        }

        if (Differs(facing, dir)) {
            if (rotSpeed <= 0.0f) {
                facing = dir;
            } else {
                ape::Vec3 axis = Cross(facing, dir);
                if (axis.x == kZero.x && axis.y == kZero.y && axis.z == kZero.z) {
                    axis = kUp;
                    const float d = Dot(facing, axis);
                    if (std::fabs(std::fabs(d) - 1.0f) < kEpsilon || std::fabs(d + 1.0f) < kEpsilon)
                        axis = kForward;
                }
                const bool sideBefore = Dot(Cross(facing, axis), dir) > 0.0f;

                // Rotate the facing about the axis by this frame's step.
                const float half = rotSpeed * dt * 0.5f;
                const float axisInv = 1.0f / std::sqrt(Dot(axis, axis));
                const float s = ape::FastSin(half);
                const ape::Quat q = {axis.x * axisInv * s, axis.y * axisInv * s, axis.z * axisInv * s,
                                     ape::FastCos(half)};
                const ape::Quat conj = {-q.x, -q.y, -q.z, q.w};
                const ape::Quat v = {facing.x, facing.y, facing.z, 0.0f};
                ape::Quat tmp{}, rotated{};
                ape::QuatMul(tmp, q, v);
                ape::QuatMul(rotated, tmp, conj);

                ape::Vec3 next = {rotated.x, planar ? 0.0f : rotated.y, rotated.z, 0.0f};
                const float nextSq = Dot(next, next);
                if (nextSq >= kEpsilon) {
                    const float inv = 1.0f / std::sqrt(nextSq);
                    next = {next.x * inv, next.y * inv, next.z * inv, 0.0f};
                } else {
                    // NOTE: the console fills all three components with the old z.
                    next = {facing.z, facing.z, facing.z, 0.0f};
                }
                facing = next;
                const bool sideAfter = Dot(Cross(facing, axis), dir) > 0.0f;
                if (sideBefore != sideAfter)
                    facing = dir;   // stepped past the player

                if (Differs(facing, dir)) {
                    if (turn == kTurn_None) {
                        turn = axis.y > 0.0f ? kTurn_Left : kTurn_Right;
                        if (script) {
                            script->SetStateAndUpdate(kState_TurnBegin);
                            if (turn == kTurn_Left)
                                script->SetStateAndUpdate(kState_TurnBeginLeft);
                            else if (turn == kTurn_Right)
                                script->SetStateAndUpdate(kState_TurnBeginRight);
                        }
                    }
                } else {
                    if (script && turn != kTurn_None) {
                        script->SetStateAndUpdate(kState_TurnEnd);
                        if (turn == kTurn_Left)
                            script->SetStateAndUpdate(kState_TurnEndLeft);
                        else if (turn == kTurn_Right)
                            script->SetStateAndUpdate(kState_TurnEndRight);
                    }
                    turn = kTurn_None;
                }
            }
        }

        output = Identity();
        output.zAxis = facing;
        output.xAxis = Cross(output.yAxis, output.zAxis);
        output.yAxis = Cross(output.zAxis, output.xAxis);
        output.translation = rest.translation;
        ape::Matrix34NormalizeAxes(output);
    }
    if (isBone || partObject)
        QueueWorldCallback(1, reinterpret_cast<void*>(&ApplyCallback), this, owner);
    else
        owner->SetLocalTransform(output);
    previous = output;
}

// @func 0x00290190 FacePlayerComponent::SetOption
void FacePlayerComponent::SetOption(bool value)
{
    enabled = value;
    if (value)
        ResetFacing();
}

// @func 0x00290084 FacePlayerComponent::ResetFacing
void FacePlayerComponent::ResetFacing()
{
    ape::world::ApeObj* object = owner ? owner->object : nullptr;
    if (!object)
        return;
    if (isBone)
        facing = RotateVector(bone->model->zAxis, object->GetWorld());
    else
        facing = (partObject ? partObject : object)->GetWorld().zAxis;
}

// @func 0x002901E0 FacePlayerComponent::ApplyToPart
void FacePlayerComponent::ApplyToPart()
{
    ape::world::ApeObj* object = owner ? owner->object : nullptr;
    if (!object || (!isBone && !partObject))
        return;
    if (isBone) {
        // Model space = world output relative to the object; local = model
        // relative to the parent bone.
        ape::world::Matrix34Mul(*bone->model, output, InverseOrtho(object->GetWorld()));
        ape::Matrix34Copy(*bone->local, *bone->model);
        if (parentBone)
            ape::world::Matrix34Mul(*bone->local, *bone->model, InverseOrtho(*parentBone->model));
        ape::world::ApeSkeleton* skeleton = object->GetSkeleton();
        skeleton->flags |= ape::world::kSkelFlag_Dirty;
        object->GetSkeleton()->UpdateModel();
    } else {
        // World output; the local transform follows from the parent.
        ape::Matrix34 local = output;
        partObject->m_world = output;
        partObject->m_flags &= ~ape::world::kObjFlag_Dirty;   // so MarkDirty queues it again
        if (ape::world::ApeObj* parent = partObject->GetParent())
            ape::world::Matrix34Mul(local, output, InverseOrtho(parent->GetWorld()));
        partObject->SetLocal(local);
        partObject->MarkDirty();
    }
    object->MarkDirty();
    object->UpdateWorld(false, false);
}

// @func 0x002908EC FacePlayerComponent::ApplyCallback
void FacePlayerComponent::ApplyCallback(void*, FacePlayerComponent* component)
{
    component->ApplyToPart();
}

} // namespace components
} // namespace sly
