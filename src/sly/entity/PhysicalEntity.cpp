// openTIT - The physical entity base and its definition base (see PhysicalEntity.h).
#include "sly/entity/PhysicalEntity.h"

#include "sly/camera/CameraHooks.h"
#include "sly/components/OffscreenMarker.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHits.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/math/ApeMatrixOps.h"
#include "sly/entity/EntityTuning.h"
#include "ape/anim/ApeAnimScriptVars.h"

#include <cmath>

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
// Game event posted when a physical entity leaves the world (0x00465B30).
constexpr uint32_t kGameEvent_PhysicalReleased = 0x1E;
// Collision layer of the second body (low five bits of its filter word).
constexpr uint32_t kTriggerBodyLayer = 7;
constexpr uint32_t kFilterLayerMask = 0x1F;
// Anim script variables set on launch. NOTE: names unknown, roles from use.
constexpr uint32_t kScriptVar_AimFrame = 0x03CBAB04u;     // matrix
constexpr uint32_t kScriptVar_HasTarget = 0xCD22388Bu;    // int
constexpr uint32_t kScriptVar_LaunchValue = 0xCC2DABB0u;  // float
constexpr uint32_t kScriptState_Start = 0x04B8CC42u;      // "Start"
const ape::Matrix34 kIdentityFrame = {
    {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
};

// Drops a counted entity reference and clears the pointer.
void DropReference(Entity*& entity)
{
    if (entity) {
        entity->Release(false);
        entity = nullptr;
    }
}
} // namespace

// -- definition ------------------------------------------------------------------------

// @func 0x000E9820 PhysicalEntityDef::~PhysicalEntityDef
// @func 0x000E992C PhysicalEntityDef::~PhysicalEntityDef (deleting)
PhysicalEntityDef::~PhysicalEntityDef()
{
    if (collisionShape)
        HavokRelease(collisionShape);
}

// @func 0x000EB040 PhysicalEntityDef::FallAcceleration
float PhysicalEntityDef::FallAcceleration() const
{
    if (!collisionShape)
        return 0.0f;
    return PhysicsWorldGravity() * gravityScale;
}

// @func 0x000E99B0 PhysicalEntityDef::GetBounds
void PhysicalEntityDef::GetBounds(ape::Aabb& out) const
{
    if (!collisionShape) {
        EntityDefBaseBounds(this, out);
        return;
    }
    HavokShapeLocalAabb(collisionShape, out);
}

// -- entity ------------------------------------------------------------------------------

// @func 0x000EB0F8 PhysicalEntity::~PhysicalEntity
// @func 0x000EB1B0 PhysicalEntity::~PhysicalEntity (deleting)
// @func 0x000EB290 PhysicalEntity::~PhysicalEntity (thunk, +0x180)
// @func 0x000EB2B8 PhysicalEntity::~PhysicalEntity (thunk, +0x17C)
// @func 0x000EB1F8 PhysicalEntity::~PhysicalEntity (deleting thunk, +0x180)
// @func 0x000EB244 PhysicalEntity::~PhysicalEntity (deleting thunk, +0x17C)
// The main body is dropped by Release; only the second body and the sound
// are left here.
PhysicalEntity::~PhysicalEntity()
{
    if (triggerBody)
        camera::DestroyHavokEntity(triggerBody);
    audio::SoundNameRelease(impactSound);
}

bool PhysicalEntity::PostUpdate(float dt)
{
    // TODO(0x000EC5B8 PhysicalEntity::PostUpdate): body/transform sync,
    // impact sounds and the carried entity.
    return ScriptedEntity::PostUpdate(dt);
}

// @func 0x000E9BA0 PhysicalEntity::SetPhysState
// Entering puts the body (with the entity's collision group), its actions
// and the second body into the physics world and shows the entity;
// kPhys_Removed takes both bodies out again. Other states are only recorded.
void PhysicalEntity::SetPhysState(int state, uint32_t arg)
{
    (void)arg;   // NOTE: passed by every caller (0), never read
    if (physState == state)
        return;
    if (state == kPhys_Enter) {
        SetVisible(true, true, true, true);
        if (body) {
            RigidBodySetFilterInfo(body, (renderId << 16) | (RigidBodyFilterInfo(body) & kFilterLayerMask));
            if (!RigidBodyInWorld(body)) {
                RigidBodyAddToWorld(body, ~0u);
                if (action)
                    PhysicsAddAction(action);
                if (action2)
                    PhysicsAddAction(action2);
                if (triggerBody) {
                    RigidBodySetFilterInfo(triggerBody, (renderId << 16) | kTriggerBodyLayer);
                    RigidBodyAddToWorld(triggerBody, ~0u);
                }
            }
        }
        state = kPhys_InWorld;
    } else if (state == kPhys_Removed) {
        if (body && RigidBodyInWorld(body))
            RigidBodyRemoveFromWorld(body);
        if (triggerBody && RigidBodyInWorld(triggerBody))
            RigidBodyRemoveFromWorld(triggerBody);
    }
    physState = state;
}

// @func 0x000EFFC8 PhysicalEntity::InteractionDisabled
bool PhysicalEntity::InteractionDisabled() const
{
    return (Def()->defFlags & PhysicalEntityDef::kDefFlag_NoInteraction) != 0;
}

// @func 0x000F1D0C PhysicalEntity::OnStep
// @func 0x000F1D80 PhysicalEntity::OnStep (thunk, +0x17C)
void PhysicalEntity::OnStep(const void* event)
{
    if (physState != kPhys_InWorld) {
        ContactEventDisable(event);
        return;
    }
    HandleBodyContact(event, body);
    HandleTriggerContact(event, triggerBody);
}

// TODO(0x000F0E48 PhysicalEntity::HandleBodyContact): impacts of the main
// body (owner/target filtering, reflection, damage, impact sounds).
bool PhysicalEntity::HandleBodyContact(const void* event, void* ownBody)
{
    (void)event;
    return ownBody != nullptr;
}

// @func 0x000EFFE4 PhysicalEntity::HandleTriggerContact
// The trigger body never collides; when it touches another physical
// entity (type bits 1 and 13) both are destroyed outright.
bool PhysicalEntity::HandleTriggerContact(const void* event, void* ownBody)
{
    constexpr uint32_t kDamage_Destroy = 0xA77EB5E9u;   // NOTE: name unknown
    if (!ownBody)
        return false;
    void* other = ContactEventBody(event, 0);
    if (other == ownBody)
        other = ContactEventBody(event, 1);
    else if (ContactEventBody(event, 1) != ownBody)
        return false;
    ContactEventDisable(event);
    auto* owner = static_cast<Entity*>(RigidBodyOwner(other));
    if (RigidBodyOwnerType(other) == kBodyOwner_Entity && owner && (owner->typeBits[0] & 0x2) &&
        (owner->typeBits[0] & 0x2000) && owner != this) {
        auto* physical = static_cast<PhysicalEntity*>(owner);
        physical->Damage(physical->maxHealth, nullptr, kDamage_Destroy);
        Damage(maxHealth, nullptr, kDamage_Destroy);
    }
    return true;
}

// @func 0x000ED35C PhysicalEntity::Release
bool PhysicalEntity::Release(bool kill)
{
    if (kill) {
        GameWorldPostEntityEvent(0xFF, kGameEvent_PhysicalReleased, this);
        DropReference(target);
        SetPhysState(kPhys_Removed);
        if (action)
            HavokRelease(action);
        if (action2)
            HavokRelease(action2);
        if (body) {
            RigidBodyClearUserProperties(body);
            HavokRelease(body);
            body = nullptr;
        }
        DropReference(owner);
        if (carried && carried->IsAlive())
            carried->Release(true);
        carried = nullptr;
    }
    return ScriptedEntity::Release(kill);
}

// @func 0x000ED190 PhysicalEntity::SetTransform
void PhysicalEntity::SetTransform(const ape::Matrix34& m)
{
    Entity::SetTransform(m);
    if (body)
        RigidBodySetTransform(body, m);
}

// @func 0x000ED680 PhysicalEntity::SetVelocity
void PhysicalEntity::SetVelocity(const ape::Vec3& velocity)
{
    if (body)
        RigidBodySetLinearVelocity(body, velocity);
}

// @func 0x000ED5D8 PhysicalEntity::GetVelocity
ape::Vec3 PhysicalEntity::GetVelocity()
{
    return body ? RigidBodyLinearVelocity(body) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x000ED74C PhysicalEntity::HandleMessage
// Forgets the target or the owner when it is killed.
void PhysicalEntity::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Killed) {
        Entity* sender = MessageSenderEntity(msg);
        if (sender && sender == target) {
            SetTarget(nullptr);
        } else if (sender && sender == owner) {
            DropReference(owner);
        }
    }
    ScriptedEntity::HandleMessage(msg);
}

// @func 0x000EB2F0 PhysicalEntity::SetTarget
// The offscreen marker keeps its own reference (taken only while the
// target is alive).
void PhysicalEntity::SetTarget(Entity* newTarget)
{
    if (newTarget == target)
        return;
    if (target)
        target->Release(false);
    target = newTarget;
    if (target)
        target->AddRef();
    if (marker) {
        if (marker->target2)
            marker->target2->Release(false);
        marker->target2 = target;
        if (target && target->IsAlive())
            target->AddRef();
    }
}

// @func 0x000EF25C PhysicalEntity::AimAt
void PhysicalEntity::AimAt(Entity* target)
{
    ape::Matrix34 aim = kIdentityFrame;
    if (target)
        aim.translation = target->GetAimPoint();
    SetCarriedTransform(aim);
}

// @func 0x000EF3C8 PhysicalEntity::UpdateFlightTime
void PhysicalEntity::UpdateFlightTime()
{
    const ape::Vec3 velocity = GetVelocity();
    const ape::Vec3& aim = carriedTransform.translation;
    const ape::Vec3& from = Position();
    const float groundSpeed = std::sqrt(velocity.z * velocity.z + velocity.x * velocity.x);
    const float rise = aim.y - from.y;
    if (groundSpeed > 1e-05f) {
        const float dx = aim.x - from.x;
        const float dz = aim.z - from.z;
        flightTime = std::sqrt(dz * dz + dx * dx) / groundSpeed;
        return;
    }
    // NOTE: the console reads the gravity of the world the body is in.
    const float gravity = PhysicsWorldGravity() * Def()->gravityScale;
    if (std::fabs(velocity.y) * 0.1f < std::fabs(gravity)) {
        const float a = velocity.y / gravity;
        const float disc = a * a - (rise * 2.0f) / gravity;
        if (disc >= 0.0f)
            flightTime = a + std::sqrt(disc);
    } else {
        // Hardly any gravity: the rise at the vertical speed, corrected to
        // first order.
        flightTime = (rise / velocity.y) * ((gravity * rise) / (velocity.y * 2.0f * velocity.y) + 1.0f);
    }
}

// @func 0x000EF564 PhysicalEntity::OnLaunched
void PhysicalEntity::OnLaunched()
{
    UpdateFlightTime();
    if (!animScript)
        return;
    if (ape::ApeAnimScriptVar* var = animScript->FindVar(kScriptVar_AimFrame)) {
        if (var->GetType() == ape::AnimScriptVarType::Matrix)
            static_cast<ape::ApeAnimScriptMatrixVar*>(var)->SetMatrix(carriedTransform);
    }
    if (ape::ApeAnimScriptVar* var = animScript->FindVar(kScriptVar_HasTarget)) {
        if (var->GetType() == ape::AnimScriptVarType::Int)
            static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(target != nullptr ? 1 : 0);
    }
    if (ape::ApeAnimScriptVar* var = animScript->FindVar(kScriptVar_LaunchValue)) {
        if (var->GetType() == ape::AnimScriptVarType::Float)
            static_cast<ape::ApeAnimScriptFloatVar*>(var)->SetFloat(Def()->launchValue);
    }
    animScript->SetStateAndUpdate(kScriptState_Start);
}

// @func 0x000EB420 PhysicalEntity::SweepTo
void PhysicalEntity::SweepTo(const ape::Matrix34& from, const ape::Vec3& to, ape::Matrix34& out, bool orient)
{
    out = from;
    out.translation = {to.x, to.y, to.z, from.translation.pad};
    if (!body || !RigidBodyInWorld(body))
        return;
    const PhysicalEntityDef* def = Def();
    if (RigidBodySweep(body, from, to, out) && (def->defFlags & PhysicalEntityDef::kDefFlag_DieOnHit))
        Damage(maxHealth, nullptr, 0);
    if (!(def->defFlags & PhysicalEntityDef::kDefFlag_FaceMotion) || !orient)
        return;
    const ape::Vec3 at = out.translation;
    ape::Vec3 motion = {at.x - from.translation.x, at.y - from.translation.y, at.z - from.translation.z, 0.0f};
    if (std::fabs(motion.x) < 1e-05f && std::fabs(motion.y) < 1e-05f && std::fabs(motion.z) < 1e-05f)
        return;
    // Ease the forward axis towards the motion.
    float damping = g_entityProjectileOrientDampening;
    if (damping > 1000.0f)
        damping = 1000.0f;
    if (damping < 1.0f)
        damping = 1.0f;
    const float step = (1.0f / damping) / std::sqrt(motion.z * motion.z + motion.x * motion.x + motion.y * motion.y);
    ape::Vec3 forward = {motion.x * step + from.zAxis.x, motion.y * step + from.zAxis.y,
                         motion.z * step + from.zAxis.z, 0.0f};
    const float inv = 1.0f / std::sqrt(forward.z * forward.z + forward.x * forward.x + forward.y * forward.y);
    forward = {forward.x * inv, forward.y * inv, forward.z * inv, 0.0f};
    auto isZero = [](const ape::Vec3& v) {
        return std::fabs(v.x) < 1e-05f && std::fabs(v.y) < 1e-05f && std::fabs(v.z) < 1e-05f;
    };
    if (isZero(forward))
        forward = out.zAxis;
    auto cross = [](const ape::Vec3& a, const ape::Vec3& b) {
        return ape::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f};
    };
    ape::Matrix34 axes = out;
    axes.xAxis = cross(out.yAxis, forward);
    if (isZero(axes.xAxis)) {
        // Parallel to the up axis: use the world axis least aligned with forward.
        ape::Vec3 helper;
        const float ax = std::fabs(forward.x), ay = std::fabs(forward.y), az = std::fabs(forward.z);
        if (ay > az && ax > az)
            helper = {0.0f, 0.0f, 1.0f, 0.0f};
        else if (ax > ay)
            helper = {0.0f, 1.0f, 0.0f, 0.0f};
        else
            helper = {1.0f, 0.0f, 0.0f, 0.0f};
        axes.xAxis = cross(helper, forward);
    }
    axes.zAxis = forward;
    ape::Matrix34NormalizeAxes(axes);
    const ape::Vec3 side = axes.xAxis;
    out.xAxis = {side.x, side.y, side.z, out.xAxis.pad};
    out.yAxis = cross(forward, side);
    out.zAxis = {forward.x, forward.y, forward.z, out.zAxis.pad};
    out.translation = at;
}

// @func 0x000F1E00 PhysicalEntity::UpdateFromBody
void PhysicalEntity::UpdateFromBody(float dt)
{
    if (!body)
        return;
    const ape::Matrix34 bodyFrame = RigidBodyTransform(body);
    if (!(Def()->defFlags & PhysicalEntityDef::kDefFlag_FaceMotion)) {
        transform.xAxis = {bodyFrame.xAxis.x, bodyFrame.xAxis.y, bodyFrame.xAxis.z, transform.xAxis.pad};
        transform.yAxis = {bodyFrame.yAxis.x, bodyFrame.yAxis.y, bodyFrame.yAxis.z, transform.yAxis.pad};
        transform.zAxis = {bodyFrame.zAxis.x, bodyFrame.zAxis.y, bodyFrame.zAxis.z, transform.zAxis.pad};
        transform.translation = {bodyFrame.translation.x, bodyFrame.translation.y, bodyFrame.translation.z,
                                 transform.translation.pad};
        return;
    }
    auto isZero = [](const ape::Vec3& v) {
        return std::fabs(v.x) < 1e-05f && std::fabs(v.y) < 1e-05f && std::fabs(v.z) < 1e-05f;
    };
    auto cross = [](const ape::Vec3& a, const ape::Vec3& b) {
        return ape::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f};
    };
    // Forward along the velocity (the current forward when at rest), upright
    // to the current up axis.
    ape::Vec3 forward = RigidBodyLinearVelocity(body);
    if (isZero(forward))
        forward = transform.zAxis;
    ape::Vec3 side = cross(transform.yAxis, forward);
    if (isZero(side)) {
        ape::Vec3 helper;
        const float ax = std::fabs(forward.x), ay = std::fabs(forward.y), az = std::fabs(forward.z);
        if (ay > az && ax > az)
            helper = {0.0f, 0.0f, 1.0f, 0.0f};
        else if (ax > ay)
            helper = {0.0f, 1.0f, 0.0f, 0.0f};
        else
            helper = {1.0f, 0.0f, 0.0f, 0.0f};
        side = cross(helper, forward);
    }
    ape::Matrix34 frame;
    frame.xAxis = side;
    frame.yAxis = cross(forward, side);
    frame.zAxis = forward;
    frame.translation = bodyFrame.translation;
    ape::Matrix34NormalizeAxes(frame);
    ape::Matrix34 roll;
    ape::Matrix34RotationZ(Def()->rollRate * dt, roll);
    ape::Matrix34PreMultiply(frame, roll);
    transform.xAxis = {frame.xAxis.x, frame.xAxis.y, frame.xAxis.z, transform.xAxis.pad};
    transform.yAxis = {frame.yAxis.x, frame.yAxis.y, frame.yAxis.z, transform.yAxis.pad};
    transform.zAxis = {frame.zAxis.x, frame.zAxis.y, frame.zAxis.z, transform.zAxis.pad};
    transform.translation = {frame.translation.x, frame.translation.y, frame.translation.z,
                             transform.translation.pad};
}

// @func 0x000EDD74 PhysicalEntity::SetCarriedTransform
void PhysicalEntity::SetCarriedTransform(const ape::Matrix34& m)
{
    carriedTransform.xAxis = { m.xAxis.x, m.xAxis.y, m.xAxis.z, carriedTransform.xAxis.pad };
    carriedTransform.yAxis = { m.yAxis.x, m.yAxis.y, m.yAxis.z, carriedTransform.yAxis.pad };
    carriedTransform.zAxis = { m.zAxis.x, m.zAxis.y, m.zAxis.z, carriedTransform.zAxis.pad };
    carriedTransform.translation = { m.translation.x, m.translation.y, m.translation.z,
                                     carriedTransform.translation.pad };
    if (carried)
        carried->SetTransform(carriedTransform);
}

// @func 0x000EDBA4 PhysicalEntity::CanInteractWith
// Only interaction kinds 0-2, and only for an owned object or with a
// partner; the definition can rule out kind 2 or everything.
bool PhysicalEntity::CanInteractWith(Entity* other, int kind, float* priority)
{
    const uint32_t flags = Def()->defFlags;
    if (kind == 2 && (flags & PhysicalEntityDef::kDefFlag_NoKind2Interaction))
        return false;
    if (kind < 0 || kind >= 3)
        return false;
    if (!other && !owner)
        return false;
    if (flags & PhysicalEntityDef::kDefFlag_NoInteraction)
        return false;
    return Entity::CanInteractWith(other, kind, priority);
}

// @func 0x000ED898 PhysicalEntity::OnHitCategory1
void PhysicalEntity::OnHitCategory1(EntityHitRecord& hit)
{
    const float amount = Def()->impactScale * hit.data.impact.damage;
    if (amount > 0.0f)
        Damage(amount, hit.source, hit.damageBits);
}

// @func 0x000ED928 PhysicalEntity::OnHitCategory2
void PhysicalEntity::OnHitCategory2(EntityHitRecord& hit)
{
    const float amount = Def()->blastScale * hit.data.blast.damage;
    if (amount > 0.0f)
        Damage(amount, hit.source, hit.damageBits);
}

// @func 0x000ED9B8 PhysicalEntity::OnHitCategory3
void PhysicalEntity::OnHitCategory3(EntityHitRecord& hit)
{
    const float amount = Def()->meleeScale * hit.data.knock.amount;
    if (amount > 0.0f)
        Damage(amount, hit.source, hit.damageBits);
}

// @func 0x000EDA40 PhysicalEntity::OnHitCategory4
void PhysicalEntity::OnHitCategory4(EntityHitRecord& hit)
{
    const float amount = ProjectileDamageScale() * hit.data.projectile.damage;
    if (amount > 0.0f)
        Damage(amount, hit.source, hit.damageBits);
}

// @func 0x000EDAF0 PhysicalEntity::Damage
// Running out of health switches the physics state to kPhys_Dead.
bool PhysicalEntity::Damage(float amount, Entity* source, uint32_t type)
{
    if (amount <= 0.0f)
        return true;
    ScriptedEntity::Damage(amount, source, type);
    if (health <= 0.0f)
        SetPhysState(kPhys_Dead);
    return health > 0.0f;
}

// @func 0x000EDB98 PhysicalEntity::ProjectileDamageScale
float PhysicalEntity::ProjectileDamageScale() const
{
    return Def()->projectileScale;
}

// -- small overrides shared by every physical entity class (0x00748238-0x00748288)

// @func 0x00748238 PhysicalEntity::GetRenderObject
ape::world::ApeObj* PhysicalEntity::GetRenderObject() { return static_cast<ape::world::ApeObj*>(body); }

// @func 0x00748240 PhysicalEntity::OnHitCategory0
void PhysicalEntity::OnHitCategory0(EntityHitRecord& hit) { (void)hit; }

// @func 0x00748244 PhysicalEntity::Interface17CSlot7
// @func 0x00748248 PhysicalEntity::Interface17CSlot7 (thunk, +0x17C)
void PhysicalEntity::Interface17CSlot7() {}

// @func 0x0074824C PhysicalEntity::Interface17CSlot6
// @func 0x00748250 PhysicalEntity::Interface17CSlot6 (thunk, +0x17C)
void PhysicalEntity::Interface17CSlot6() {}

// @func 0x00748254 PhysicalEntity::Slot99
void PhysicalEntity::Slot99() {}

// @func 0x00748258 PhysicalEntity::Interface180Slot6
// @func 0x00748288 PhysicalEntity::Interface180Slot6 (thunk, +0x180)
void PhysicalEntity::Interface180Slot6(void* deleted)
{
    RigidBodyRemoveContactListener(deleted, static_cast<EntityInterface17C*>(this));
}

// @func 0x007482B8 PlainPhysicalEntity::~PlainPhysicalEntity
// @func 0x007482F8 PlainPhysicalEntity::~PlainPhysicalEntity (deleting)
// @func 0x0074848C PlainPhysicalEntity::~PlainPhysicalEntity (thunk, +0x17C)
// @func 0x007483D4 PlainPhysicalEntity::~PlainPhysicalEntity (deleting thunk, +0x17C)
// @func 0x00748444 PlainPhysicalEntity::~PlainPhysicalEntity (thunk, +0x180)
// @func 0x00748364 PlainPhysicalEntity::~PlainPhysicalEntity (deleting thunk, +0x180)
PlainPhysicalEntity::~PlainPhysicalEntity() = default;

} // namespace entity
} // namespace sly
