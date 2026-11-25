// openTIT - RigidProp: a scripted entity with a Havok rigid body built from
// its definition's collision object (vtable 0x00B143A8, 0x00168114-0x0016A358).
// Hits push the body and hurt the prop by the definition's per-category
// scales; destroyed props stay in state 7.
#include "sly/entity/PropEntities.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHits.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"

#include <algorithm>

namespace sly {
namespace entity {

namespace {

// Messages that pay out the prop's score to whoever destroyed it.
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
constexpr uint32_t kMsg_B94AE51B = 0xB94AE51Bu;   // NOTE: name unknown
constexpr uint32_t kComp_Scoring = 0x28;
// Entity type bit 1: entities that record what they attack (+0x174).
constexpr uint32_t kType_Attacker = 0x02;

} // namespace

// @func 0x001683F0 RigidProp::Release
bool RigidProp::Release(bool kill)
{
    if (kill) {
        SetPropState(kProp_Removed, 0);
        ReleaseBody(bodyAction);
    }
    return ScriptedEntity::Release(kill);
}

// @func 0x001684EC RigidProp::SetFlag80
// The flag takes the body out of the physics world; clearing it puts a
// dynamic prop's body back.
void RigidProp::SetFlag80(bool on)
{
    Entity::SetFlag80(on);
    if (!rigidBody || RigidBodyInWorld(rigidBody) != on)
        return;
    if (on) {
        RigidBodyRemoveFromWorld(rigidBody);
    } else if (propState == kProp_Dynamic) {
        RigidBodyAddToWorld(rigidBody, ~0u);
        if (bodyAction)
            PhysicsAddAction(bodyAction);
    }
}

// @func 0x0016A358 RigidProp::HandleMessage
// When the entity that destroyed the prop reports the kill, the prop's
// score goes to player 0.
void RigidProp::HandleMessage(const EntityMessage& msg)
{
    Entity* sender = MessageSenderEntity(msg);
    if ((msg.id != kMsg_Killed && msg.id != kMsg_B94AE51B) || !sender || !(sender->typeBits[0] & kType_Attacker))
        return;
    if (AttackTargetOf(sender) != this)
        return;
    if (EntityComponent* scoring = sender->GetComponent(kComp_Scoring)) {
        const int points = int(ScoringValue(scoring));
        ScoringAward(scoring, points, true);
        AddPlayerScore(0, points);
    }
}

// @func 0x00168714 RigidProp::GetVelocity
ape::Vec3 RigidProp::GetVelocity()
{
    return BodyVelocity();
}

// @func 0x001691D4 RigidProp::GetAimPoint
ape::Vec3 RigidProp::GetAimPoint()
{
    return BodyMassCenter();
}

// @func 0x001687B0 RigidProp::CanBeDamaged
bool RigidProp::CanBeDamaged() const
{
    return Def()->damage.CanHurt();
}

// @func 0x00168810 RigidProp::OnHitCategory0
void RigidProp::OnHitCategory0(EntityHitRecord& hit)
{
    HitCollision(Def()->damage, hit);
}

// @func 0x00168950 RigidProp::OnHitCategory1
void RigidProp::OnHitCategory1(EntityHitRecord& hit)
{
    HitImpact(Def()->damage, hit);
}

// @func 0x00168BD0 RigidProp::OnHitCategory2
void RigidProp::OnHitCategory2(EntityHitRecord& hit)
{
    HitBlast(Def()->damage, hit, false);
}

// @func 0x00168E50 RigidProp::OnHitCategory3
void RigidProp::OnHitCategory3(EntityHitRecord& hit)
{
    HitEnvironment(Def()->damage, hit);
}

// @func 0x00169040 RigidProp::OnHitCategory4
// Projectiles hurt with the hit's own reaction as the damage type.
void RigidProp::OnHitCategory4(EntityHitRecord& hit)
{
    HitProjectile(Def()->damage, hit, hit.reaction);
}

// @func 0x001690F8 RigidProp::Damage
bool RigidProp::Damage(float amount, Entity* source, uint32_t type)
{
    if (amount <= 0.0f)
        return true;
    if (health > 0.0f) {
        ScriptedEntity::Damage(amount, source, type);
        if (health == 0.0f)
            SetPropState(kProp_Destroyed, 0);
    }
    return health > 0.0f;
}

// @func 0x001691C8 RigidProp::ProjectileDamageScale
float RigidProp::ProjectileDamageScale() const
{
    return Def()->damage.projectileScale;
}

// @func 0x00167570 RigidProp::SetPropState
void RigidProp::SetPropState(int32_t state, uint32_t arg)
{
    (void)arg;
    if (state == propState)
        return;
    switch (state) {
    case kProp_Dynamic:
        if (rigidBody && !RigidBodyInWorld(rigidBody) && !(flags & kEnt_80)) {
            RigidBodyAddToWorld(rigidBody, collisionGroup);
            if (bodyAction)
                PhysicsAddAction(bodyAction);
        }
        field_190 = 0;
        field_194 = 0;
        break;
    case 2:
    case 3:
    case kProp_Removed:
        state = kProp_Removed;
        if (rigidBody && RigidBodyInWorld(rigidBody))
            RigidBodyRemoveFromWorld(rigidBody);
        field_1C0 = 0;
        field_1C1 = 0;
        field_1C4 = 0;
        break;
    default:
        break;
    }
    propState = state;
}

} // namespace entity
} // namespace sly
