// openTIT - Actor base: physical parts, physics states, nav obstacles and the
// slot overrides that work on them (vtable 0x00B12898, 0x000E2FF0-0x000E8000).
#include "sly/entity/ActorEntities.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/levels/nav/NavGraph.h"

#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"

#include <cmath>

namespace sly {
namespace entity {

namespace {
// Message that makes an actor rebuild its nav obstacles (sent by scripts
// after moving it). NOTE: name unknown.
constexpr uint32_t kMsg_RefreshNavObstacles = 0xC39AF658u;
constexpr uint32_t kFilterLayerMask = 0x1F;

bool IsZero(const ape::Vec3& v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

bool SameTransform(const ape::Matrix34& a, const ape::Matrix34& b)
{
    auto same = [](const ape::Vec3& p, const ape::Vec3& q) { return p.x == q.x && p.y == q.y && p.z == q.z; };
    return same(a.xAxis, b.xAxis) && same(a.yAxis, b.yAxis) && same(a.zAxis, b.zAxis) &&
           same(a.translation, b.translation);
}
} // namespace

// -- parts -------------------------------------------------------------------------------

// @func 0x000E3534 ActorEntity::PlacePart
// A part that follows a bone of the actor's object sits at the bone's model
// transform in the object's world frame; one that follows a locator object
// at the locator; any other part at `frame`. The part's offset is applied
// in the resulting frame.
void ActorEntity::PlacePart(ape::Matrix34& out, const Part& part, const ape::Matrix34& frame) const
{
    const ape::world::ApeSkeletonBone* bone = (object && part.bone >= 0) ? GetBone(part.bone) : nullptr;
    if (bone)
        ape::world::Matrix34Mul(out, *bone->model, object->GetWorld());
    else if (part.locator)
        out = part.locator->GetWorld();
    else
        out = frame;
    if (!IsZero(part.offset)) {
        const ape::Vec3& o = part.offset;
        out.translation.x += o.x * out.xAxis.x + o.y * out.yAxis.x + o.z * out.zAxis.x;
        out.translation.y += o.x * out.xAxis.y + o.y * out.yAxis.y + o.z * out.zAxis.y;
        out.translation.z += o.x * out.xAxis.z + o.y * out.yAxis.z + o.z * out.zAxis.z;
    }
}

// @func 0x000E7CE8 ActorEntity::PartTransform
void ActorEntity::PartTransform(ape::Matrix34& out, int index, const ape::Matrix34& frame) const
{
    if (!parts || parts->Count() == 0) {
        out = frame;
        return;
    }
    PlacePart(out, (*parts)[index], frame);
}

// @func 0x000E7DC4 ActorEntity::GetAimPoint
ape::Vec3 ActorEntity::GetAimPoint()
{
    if (GetComponent(kComp_Interaction))
        return Position();
    const auto* actorDef = static_cast<const ActorDef*>(def);
    const ape::Matrix34& base = object ? object->GetWorld() : ape::world::kWorldIdentity;
    ape::Matrix34 frame;
    if (parts && parts->Count() != 0)
        PlacePart(frame, (*parts)[0], base);
    else
        frame = base;
    if (!actorDef->parts || actorDef->parts->Count() == 0)
        return frame.translation;
    const ape::Vec3& o = (*actorDef->parts)[0].aimOffset;
    return {o.x * frame.xAxis.x + o.y * frame.yAxis.x + o.z * frame.zAxis.x + frame.translation.x,
            o.x * frame.xAxis.y + o.y * frame.yAxis.y + o.z * frame.zAxis.y + frame.translation.y,
            o.x * frame.xAxis.z + o.y * frame.yAxis.z + o.z * frame.zAxis.z + frame.translation.z, 0.0f};
}

// @func 0x000E5EF0 ActorEntity::SetTransform
// Bodies in the world follow their parts; a body is only moved (and woken)
// when its place actually changes.
void ActorEntity::SetTransform(const ape::Matrix34& m)
{
    Entity::SetTransform(m);
    if (!parts)
        return;
    for (Part& part : *parts) {
        if (!part.body || !RigidBodyInWorld(part.body))
            continue;
        ape::Matrix34 placed;
        PlacePart(placed, part, m);
        if (!SameTransform(placed, RigidBodyTransform(part.body))) {
            RigidBodySetTransform(part.body, placed);
            RigidBodyActivate(part.body);
        }
    }
}

// @func 0x000E764C ActorEntity::ObjectForCollidable
ape::world::ApeObj* ActorEntity::ObjectForCollidable(const void* collidable, int32_t* bone)
{
    if (void* body = CollidableOwnerBody(collidable)) {
        if (parts) {
            for (Part& part : *parts) {
                if (part.body != body)
                    continue;
                if (part.locator) {
                    *bone = -1;
                    return part.locator;
                }
                *bone = part.bone;
                return object;
            }
        }
    }
    return Entity::ObjectForCollidable(collidable, bone);
}

// @func 0x000E74B8 ActorEntity::GetCenter
// A single-bodied actor is centred on its body.
ape::Vec3 ActorEntity::GetCenter()
{
    if (parts && parts->Count() == 1 && (*parts)[0].body)
        return RigidBodyPosition((*parts)[0].body);
    return Entity::GetCenter();
}

// @func 0x000E63C8 ActorEntity::GetRenderId
// The collision group of the bodies (the last body's, when there are several).
uint32_t ActorEntity::GetRenderId()
{
    uint32_t group = collisionGroup;
    if (PhysicsWorldPresent() && parts) {
        for (Part& part : *parts) {
            if (part.body)
                group = RigidBodyFilterInfo(part.body) >> 16;
        }
    }
    return group;
}

// @func 0x000E6448 ActorEntity::SetRenderId
void ActorEntity::SetRenderId(uint32_t id)
{
    if (PhysicsWorldPresent() && parts) {
        for (Part& part : *parts) {
            if (part.body)
                RigidBodySetFilterInfo(part.body, (id << 16) | (RigidBodyFilterInfo(part.body) & kFilterLayerMask));
        }
    }
    collisionGroup = id;
    Entity::SetRenderId(id);
}

// @func 0x000E6558 ActorEntity::GetGravityFactor
// The first body's factor.
float ActorEntity::GetGravityFactor()
{
    if (!PhysicsWorldPresent() || !parts)
        return 0.0f;
    for (Part& part : *parts) {
        if (part.body)
            return RigidBodyGravityFactor(part.body);
    }
    return 0.0f;
}

// @func 0x000E65F0 ActorEntity::Detached
void ActorEntity::Detached(float gravityFactor)
{
    if (PhysicsWorldPresent() && parts) {
        for (Part& part : *parts) {
            if (part.body)
                RigidBodySetGravityFactor(part.body, gravityFactor);
        }
    }
    Entity::Detached(gravityFactor);
}

// @func 0x000E6760 ActorEntity::SetFlag80
void ActorEntity::SetFlag80(bool on, int part)
{
    RemoveNavObstacles();
    flags = on ? (flags | kEnt_80) : (flags & ~uint32_t(kEnt_80));
    const bool onePart = part >= 0 && parts && part < static_cast<int>(parts->Count());
    if (parts) {
        for (size_t i = 0; i < parts->Count(); ++i) {
            if (onePart && static_cast<int>(i) != part)
                continue;
            Part& p = (*parts)[i];
            if (!p.body || RigidBodyInWorld(p.body) != on)
                continue;
            if (on) {
                RigidBodyRemoveFromWorld(p.body);
                continue;
            }
            // Coming back: put the body where its part is now.
            ape::Matrix34 placed;
            PlacePart(placed, p, object ? object->GetWorld() : ape::world::kWorldIdentity);
            if (!SameTransform(placed, RigidBodyTransform(p.body))) {
                RigidBodySetTransform(p.body, placed);
                RigidBodyActivate(p.body);
            }
            if (PhysState() != 0)
                RigidBodyAddToWorld(p.body, ~0u);
        }
    }
    AddNavObstacles();
}

// @func 0x000E7B0C ActorEntity::ApplyVisibility
void ActorEntity::ApplyVisibility(bool visible)
{
    Entity::ApplyVisibility(visible);
    if (!object || !parts)
        return;
    for (Part& part : *parts) {
        if (part.body && RigidBodyInWorld(part.body))
            RigidBodySetMotionType(part.body, visible ? part.motionType : kMotionType_Fixed);
    }
}

// @func 0x000E4720 ActorEntity::Update
// While active an actor that follows its world object takes over the
// object's transform, and every keyframed body is driven to where its part
// is now (velocities chosen so it arrives within the frame).
bool ActorEntity::Update(float dt)
{
    const bool followsObject = (physWord & 1u) != 0;
    if (PhysState() == kActorPhys_Active && object) {
        if (followsObject && !SameTransform(transform, object->GetWorld()))
            Entity::SetTransform(object->GetWorld());
        const ActorDef* d = Def();
        if (dt >= 1.0e-5f && parts && d->parts) {
            const float invDt = 1.0f / dt;
            const ape::Matrix34& frame = followsObject ? transform : object->GetWorld();
            for (size_t i = 0; i < parts->Count(); ++i) {
                Part& part = (*parts)[i];
                if (!((*d->parts)[i].partFlags & ActorDef::kPartFlag_Keyframed))
                    continue;
                if (!part.body || !RigidBodyInWorld(part.body))
                    continue;
                ape::Matrix34 target;
                PlacePart(target, part, frame);
                if (SameTransform(target, RigidBodyTransform(part.body)))
                    continue;
                ape::Quat rotation;
                ape::QuatFromMatrix34(rotation, target);
                const float len = std::sqrt(rotation.x * rotation.x + rotation.y * rotation.y +
                                            rotation.z * rotation.z + rotation.w * rotation.w);
                const float inv = 1.0f / len;
                rotation = { rotation.x * inv, rotation.y * inv, rotation.z * inv, rotation.w * inv };
                if (field_18C >= 0)
                    RigidBodyApplyHardKeyFrame(part.body, target.translation, rotation, invDt);
            }
        }
    }
    return ScriptedEntity::Update(dt);
}

// @func 0x000E4E80 CopyBodyTransform
// The rows of a body's transform as a matrix (the padding lanes are left
// alone).
static void CopyBodyTransform(ape::Matrix34& out, const ape::Matrix34& body)
{
    out.xAxis = { body.xAxis.x, body.xAxis.y, body.xAxis.z, out.xAxis.pad };
    out.yAxis = { body.yAxis.x, body.yAxis.y, body.yAxis.z, out.yAxis.pad };
    out.zAxis = { body.zAxis.x, body.zAxis.y, body.zAxis.z, out.zAxis.pad };
    out.translation = { body.translation.x, body.translation.y, body.translation.z, out.translation.pad };
}

// Inverse of a rigid transform (orthonormal axes).
static ape::Matrix34 InverseRigid(const ape::Matrix34& m)
{
    ape::Matrix34 inv;
    inv.xAxis = { m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f };
    inv.yAxis = { m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f };
    inv.zAxis = { m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f };
    const ape::Vec3& t = m.translation;
    inv.translation = { -(t.x * m.xAxis.x + t.y * m.xAxis.y + t.z * m.xAxis.z),
                        -(t.x * m.yAxis.x + t.y * m.yAxis.y + t.z * m.yAxis.z),
                        -(t.x * m.zAxis.x + t.y * m.zAxis.y + t.z * m.zAxis.z), 0.0f };
    return inv;
}

// @func 0x000E5000 ActorEntity::PostUpdate
// Keyframed parts rebuild their nav obstacle when the actor's animation has
// just finished. Simulated (non-fixed) bodies in the world drive what their
// part follows: a locator takes the body's transform; a bone takes it
// relative to the body of its parent bone's part (root bones stay). Then
// the hierarchy is refreshed. Afterwards the lifetime runs out (state 3),
// state 5 moves on to 6 and a dead actor leaves the world.
bool ActorEntity::PostUpdate(float dt)
{
    if (!ScriptedEntity::PostUpdate(dt))
        return false;
    const ActorDef* d = Def();
    if (object && parts && parts->Count() != 0 && d->parts) {
        bool posed = false;
        for (size_t i = 0; i < parts->Count(); ++i) {
            Part& part = (*parts)[i];
            void* body = part.body;
            if ((*d->parts)[i].partFlags & ActorDef::kPartFlag_Keyframed) {
                if (!body || !PartBlocksNavigation(i))
                    continue;
                auto* layer = static_cast<anim::AnimLayer*>(animController ? AnimFindPlaying(animController, 0) : nullptr);
                if (!layer || !(layer->flags & anim::AnimLayer::kFlag_JustFinished))
                    continue;
                if (levels::NavManager* nav = levels::GetNavManager())
                    levels::RemoveNavObstacle(*nav, part.navObstacle);
                part.navObstacle = nullptr;
                AddPartNavObstacle(i);
                continue;
            }
            if (!body || RigidBodyMotionType(body) == kMotionType_Fixed || !RigidBodyInWorld(body))
                continue;
            if (part.bone < 0 && !part.locator)
                continue;
            posed = true;
            ape::Matrix34 bodyTransform;
            CopyBodyTransform(bodyTransform, RigidBodyTransform(body));
            if (part.bone < 0) {
                ape::world::ApeObj* locator = part.locator;
                locator->m_world = bodyTransform;
                locator->m_flags &= ~uint16_t(ape::world::kObjFlag_Dirty);
                continue;
            }
            ape::world::ApeSkeletonBone* bone = GetBone(part.bone);
            if (!bone)
                continue;
            const int parent = bone->def->parent;
            if (parent == -1)
                continue;
            ape::Matrix34 parentTransform;
            CopyBodyTransform(parentTransform, RigidBodyTransform((*parts)[parent].body));
            ape::world::Matrix34Mul(*bone->local, bodyTransform, InverseRigid(parentTransform));
        }
        if (posed) {
            // Locators keep their new world placement: their local transform
            // is worked out from their parent.
            for (Part& part : *parts) {
                void* body = part.body;
                if (!body || RigidBodyMotionType(body) == kMotionType_Fixed || !RigidBodyInWorld(body))
                    continue;
                if (part.bone >= 0 || !part.locator)
                    continue;
                ape::world::ApeObj* locator = part.locator;
                if (ape::world::ApeObj* parent = locator->GetParent()) {
                    ape::Matrix34 local;
                    ape::world::Matrix34Mul(local, locator->GetWorld(), InverseRigid(parent->GetWorld()));
                    locator->SetLocal(local);
                } else {
                    locator->SetLocal(locator->GetWorld());
                }
                locator->MarkDirty();
            }
            object->UpdateWorld(false, false);
            if ((physWord & 1u) && !SameTransform(transform, object->GetWorld())) {
                Entity::SetTransform(object->GetWorld());
                flags |= kEnt_TransformChanged;
            }
            object->MarkDirty();
            if (ape::world::ApeSkeleton* skeleton = object->GetSkeleton()) {
                skeleton->flags |= ape::world::kSkelFlag_Dirty;
                skeleton->UpdateModel();
            }
        }
    }

    const float elapsed = stateTime + dt;
    stateTime = elapsed;
    switch (PhysState()) {
    case kActorPhys_Gone:
        return false;
    case kActorPhys_Removed:
        SetPhysState(kActorPhys_Gone);
        break;
    case kActorPhys_Enter:
    case kActorPhys_Active:
        if (lifetime >= 0.0f && lifetime <= elapsed)
            SetPhysState(kActorPhys_Remove);
        break;
    default:
        break;
    }
    if (actorFlags & kActorFlag_Dead) {
        actorFlags &= ~uint32_t(kActorFlag_Dead);
        SetPhysState(kActorPhys_Remove);
    }
    return true;
}

// @func 0x000E7810 ActorEntity::ApplyRootMotion
void ActorEntity::ApplyRootMotion(float dt)
{
    if (animController) {
        const ape::Matrix34 delta = EntityRootMotionDelta(this, dt);
        if (!SameTransform(delta, ape::world::kWorldIdentity)) {
            ape::Matrix34 moved;
            ape::world::Matrix34Mul(moved, delta, transform);
            SetLocalTransform(moved);
        }
    }
    auto* layer = animController ? static_cast<anim::AnimController*>(animController)->FindLayer(0) : nullptr;
    if (layer && !layer->IsDone(false))
        return;
    if (!layer || !(layer->flags & anim::AnimLayer::kFlag_Looping))
        flags &= ~uint32_t(kEnt_RootMotion);
}

// -- physics states ------------------------------------------------------------------------

// TODO(0x000E2AB0 ActorEntity::ConstrainPartToBone): builds the Havok
// constraint that holds a part's body at its bone (pivot from the
// definition's part record +0x30, motion type dependent strength).
void ActorEntity::ConstrainPartToBone(int index, const ape::Vec3& velocity)
{
    (void)index; (void)velocity;
}

// @func 0x000E2FF0 ActorEntity::SetPhysState
// States 1 and 7 put the part bodies into the world (with the actor's
// collision group and their nav obstacles) and show the actor; 3 (stored as
// 5) and 4 take them out. The state is kept in bits 1-31 of the word.
void ActorEntity::SetPhysState(int state, uint32_t arg)
{
    (void)arg;
    if (PhysState() == state)
        return;
    if (state == kActorPhys_Enter || state == kActorPhys_Active) {
        if (!(flags & kEnt_80) && parts && parts->Count() != 0 && PhysicsWorldPresent()) {
            const ActorDef* d = Def();
            for (size_t i = 0; i < parts->Count(); ++i) {
                Part& part = (*parts)[i];
                if (!part.body || RigidBodyInWorld(part.body))
                    continue;
                RigidBodySetFilterInfo(part.body,
                                       (collisionGroup << 16) | (RigidBodyFilterInfo(part.body) & kFilterLayerMask));
                RigidBodyAddToWorld(part.body, ~0u);
                if (d->parts && ((*d->parts)[i].partFlags & ActorDef::kPartFlag_BoneConstraint))
                    ConstrainPartToBone(static_cast<int>(i), ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f });
                if (PartBlocksNavigation(i))
                    AddPartNavObstacle(i);
            }
        }
        SetVisible(true, false, true, true);
        if (PhysState() != kActorPhys_InWorld)
            stateTime = 0.0f;
    } else if (state == kActorPhys_Remove || state == kActorPhys_Dead) {
        if (state == kActorPhys_Remove)
            state = kActorPhys_Removed;
        if (parts) {
            levels::NavManager* nav = levels::GetNavManager();
            for (Part& part : *parts) {
                if (!part.body)
                    continue;
                if (RigidBodyInWorld(part.body))
                    RigidBodyRemoveFromWorld(part.body);
                if (nav)
                    levels::RemoveNavObstacle(*nav, part.navObstacle);
                part.navObstacle = nullptr;
            }
        }
    }
    physWord = (static_cast<uint32_t>(state) << 1) | (physWord & 1u);
}

// @func 0x000E4544 ActorEntity::Release
bool ActorEntity::Release(bool kill)
{
    if (kill) {
        SetPhysState(kActorPhys_Dead);
        if (parts) {
            for (Part& part : *parts) {
                if (!part.body)
                    continue;
                for (int i = 0; i < RigidBodyConstraintCount(part.body); ++i)
                    HavokRelease(RigidBodyConstraint(part.body, i));
                RigidBodyClearUserProperties(part.body);
                HavokRelease(part.body);
                part.body = nullptr;
            }
        }
    }
    return ScriptedEntity::Release(kill);
}

// -- nav obstacles -------------------------------------------------------------------------

// Parts with a body block navigation when the definition's nav mode says so
// and the part does not choose another mode.
bool ActorEntity::PartBlocksNavigation(size_t index) const
{
    const ActorDef* d = Def();
    if (!(*parts)[index].body || d->navMode != ActorDef::kNavMode_Obstacle || !d->parts)
        return false;
    const uint32_t partMode = (*d->parts)[index].navMode;
    return partMode == 0 || partMode == ActorDef::kNavMode_Obstacle;
}

// Obstacle of one part: shaped like its body, or the definition's box
// standing on the actor's position.
void ActorEntity::AddPartNavObstacle(size_t index)
{
    levels::NavManager* nav = levels::GetNavManager();
    if (!nav)
        return;
    const ActorDef* d = Def();
    const uint32_t mask = (d->defFlags & ActorDef::kDefFlag_FullNavMask) ? 0xFFFFu : 0xFFFEu;
    Part& part = (*parts)[index];
    if (IsZero(d->navObstacleSize)) {
        part.navObstacle = levels::AddBodyNavObstacle(*nav, part.body, GetDebugName(), mask);
        return;
    }
    levels::NavObstacleDesc desc;
    desc.transform = transform;
    desc.transform.translation.y += d->navObstacleSize.y * 0.5f;
    desc.extents = d->navObstacleSize;
    desc.name = GetDebugName();
    desc.mask = mask;
    part.navObstacle = levels::AddNavObstacle(*nav, desc);
}

// @func 0x000E6D18 ActorEntity::RefreshNavObstacles
void ActorEntity::RefreshNavObstacles()
{
    if ((flags & kEnt_80) || !parts)
        return;
    levels::NavManager* nav = levels::GetNavManager();
    for (size_t i = 0; i < parts->Count(); ++i) {
        if (!PartBlocksNavigation(i))
            continue;
        if (nav)
            levels::RemoveNavObstacle(*nav, (*parts)[i].navObstacle);
        (*parts)[i].navObstacle = nullptr;
        AddPartNavObstacle(i);
    }
}

// @func 0x000E704C ActorEntity::RemoveNavObstacles
void ActorEntity::RemoveNavObstacles()
{
    if ((flags & kEnt_80) || !parts)
        return;
    levels::NavManager* nav = levels::GetNavManager();
    for (size_t i = 0; i < parts->Count(); ++i) {
        if (!PartBlocksNavigation(i))
            continue;
        if (nav)
            levels::RemoveNavObstacle(*nav, (*parts)[i].navObstacle);
        (*parts)[i].navObstacle = nullptr;
    }
}

// @func 0x000E7180 ActorEntity::AddNavObstacles
void ActorEntity::AddNavObstacles()
{
    if ((flags & kEnt_80) || !parts)
        return;
    for (size_t i = 0; i < parts->Count(); ++i) {
        if (PartBlocksNavigation(i))
            AddPartNavObstacle(i);
    }
}

// -- messages and interaction --------------------------------------------------------------

// @func 0x000E5E44 ActorEntity::HandleMessage
void ActorEntity::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_RefreshNavObstacles)
        RefreshNavObstacles();
    ScriptedEntity::HandleMessage(msg);
}

// @func 0x000E7FF0 ActorEntity::CanInteractWith
// Only while the actor is in physics state 1 or 7 and up. Kinds 0 and 2
// are refused when interactions are switched off or with the partner at
// +0x190, and their priority is scaled by the definition; kind 2 is always
// fine while a carry slot is free (otherwise refused when the definition
// only allows carrying); kind 3 goes straight to the base checks.
bool ActorEntity::CanInteractWith(Entity* other, int kind, float* priority)
{
    const int state = PhysState();
    if (state < kActorPhys_Active && state != kActorPhys_Enter)
        return false;
    const ActorDef* d = Def();
    switch (kind) {
    case 0:
        break;
    case 2:
        if (EntityComponent* carry = GetComponent(kComp_Carry)) {
            if (CarryCompFreeSlot(carry) != -1)
                return true;
        }
        if (d->defFlags & ActorDef::kDefFlag_CarryOnly)
            return false;
        break;
    case 3:
        return Entity::CanInteractWith(other, kind, priority);
    default:
        return false;
    }
    if (actorFlags & kActorFlag_NoInteraction)
        return false;
    if (other && other == field_190)
        return false;
    if (priority)
        *priority = d->interactionPriority * 5.0f * *priority;
    return Entity::CanInteractWith(other, kind, priority);
}

// @func 0x00747C34 ActorEntity::Interface180Slot6
// @func 0x00747C80 ActorEntity::Interface180Slot6 (thunk, +0x180)
void ActorEntity::Interface180Slot6(void* body)
{
    if (Def()->defFlags & ActorDef::kDefFlag_KeepListeners)
        return;
    RigidBodyRemoveContactListener(body, static_cast<EntityInterface17C*>(this));
}

// @func 0x00747B90 ActorEntity::GetRenderObject
ape::world::ApeObj* ActorEntity::GetRenderObject()
{
    if (!parts || parts->Count() != 1)
        return nullptr;
    return static_cast<ape::world::ApeObj*>((*parts)[0].body);
}

// @func 0x00747B40 ActorEntity::Slot41
int ActorEntity::Slot41() { return Def()->navMode != ActorDef::kNavMode_None ? 1 : 0; }

// @func 0x00747B68 ActorEntity::Slot42
int ActorEntity::Slot42() { return Def()->navMode == ActorDef::kNavMode_Obstacle ? 1 : 0; }

// @func 0x000E96A0 ActorEntity::FirstBodyCollisionLayer
bool ActorEntity::FirstBodyCollisionLayer(uint32_t& layer) const
{
    if (!PhysicsWorldPresent() || !parts)
        return false;
    for (const Part& part : *parts) {
        if (part.body) {
            layer = RigidBodyFilterInfo(part.body) & 0x1F;
            return true;
        }
    }
    return false;
}

// @func 0x00747584 ActorDef::DefSlot6
uint32_t ActorDef::DefSlot6() const
{
    switch (navMode) {
    case 0x9260C26Eu:   // "Static"
    case 0x41EB0AA3u:
    case 0x29846DDAu:
    case 0x4438A28Fu:   // "Obstacle"
        return 1;
    default:
        return 0;
    }
}

// @func 0x00747CD4 PlainActor::~PlainActor
// @func 0x00747D88 PlainActor::~PlainActor (deleting)
// @func 0x007480AC PlainActor::~PlainActor (thunk, +0x17C)
// @func 0x00747F24 PlainActor::~PlainActor (deleting thunk, +0x17C)
// @func 0x00747FF4 PlainActor::~PlainActor (thunk, +0x180)
// @func 0x00747E54 PlainActor::~PlainActor (deleting thunk, +0x180)
PlainActor::~PlainActor() = default;

// @func 0x00756CD4 Safe::~Safe
// @func 0x00756DCC Safe::~Safe (deleting)
// @func 0x0075720C Safe::~Safe (thunk, +0x17C)
// @func 0x00756FF8 Safe::~Safe (deleting thunk, +0x17C)
// @func 0x00757110 Safe::~Safe (thunk, +0x180)
// @func 0x00756EE0 Safe::~Safe (deleting thunk, +0x180)
Safe::~Safe() = default;

} // namespace entity
} // namespace sly
