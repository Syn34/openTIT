// openTIT - The physical entity base and its definition base (see PhysicalEntity.h).
#include "sly/entity/PhysicalEntity.h"

#include "sly/camera/CameraHooks.h"
#include "sly/components/OffscreenMarker.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHits.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
// Game event posted when a physical entity leaves the world (0x00465B30).
constexpr uint32_t kGameEvent_PhysicalReleased = 0x1E;
// Collision layer of the second body (low five bits of its filter word).
constexpr uint32_t kTriggerBodyLayer = 7;
constexpr uint32_t kFilterLayerMask = 0x1F;

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
