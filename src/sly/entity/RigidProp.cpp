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

// Damage types per hit category (shared with the other entity classes).
// NOTE: names unknown.
constexpr uint32_t kType_PhysicsImpact = 0x80F43146u;
constexpr uint32_t kType_Melee = 0x045D9554u;
constexpr uint32_t kType_Blast = 0x03C599D8u;
constexpr uint32_t kType_Environment = 0x811B1D5Eu;

// Messages that pay out the prop's score to whoever destroyed it.
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
constexpr uint32_t kMsg_B94AE51B = 0xB94AE51Bu;   // NOTE: name unknown
constexpr uint32_t kComp_Scoring = 0x28;
// Entity type bit 1: entities that record what they attack (+0x174).
constexpr uint32_t kType_Attacker = 0x02;

// Bodies that a push cannot move.
bool IsPushable(void* body)
{
    if (!body)
        return false;
    const uint8_t motion = RigidBodyMotionType(body);
    return motion != 5 && motion != 4;
}

void SetFatal(EntityHitRecord& hit, bool alive)
{
    hit.flags = alive ? uint16_t(hit.flags & ~EntityHitRecord::kHit_Fatal)
                      : uint16_t(hit.flags | EntityHitRecord::kHit_Fatal);
}

ape::Vec3 Scaled(const ape::Vec3& v, float s) { return { v.x * s, v.y * s, v.z * s, 0.0f }; }

} // namespace

// @func 0x001683F0 RigidProp::Release
bool RigidProp::Release(bool kill)
{
    if (kill) {
        SetPropState(kProp_Removed, 0);
        if (bodyAction)
            HavokRelease(bodyAction);
        if (rigidBody) {
            if (RigidBodyInWorld(rigidBody))
                RigidBodyRemoveFromWorld(rigidBody);
            RigidBodyClearUserProperties(rigidBody);
            HavokRelease(rigidBody);
        }
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
    return rigidBody ? RigidBodyLinearVelocity(rigidBody) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x001691D4 RigidProp::GetMassCenter
ape::Vec3 RigidProp::GetMassCenter()
{
    return rigidBody ? RigidBodyCenterOfMass(rigidBody) : Position();
}

// @func 0x001687B0 RigidProp::CanBeDamaged
bool RigidProp::CanBeDamaged() const
{
    const RigidPropDef* d = Def();
    return d->collisionScale != 0.0f || d->impactScale != 0.0f || d->blastScale != 0.0f ||
           d->environmentScale != 0.0f || d->projectileScale != 0.0f;
}

// @func 0x00168810 RigidProp::OnHitCategory0
// Collisions hurt above a speed threshold, in proportion to the excess
// speed, up to a maximum.
void RigidProp::OnHitCategory0(EntityHitRecord& hit)
{
    const RigidPropDef* d = Def();
    if (!ReceiveHit(hit) || d->collisionScale <= 0.0f || d->collisionMax <= 0.0f)
        return;
    const float speed = hit.data.collision.speed;
    if (speed <= d->collisionThreshold)
        return;
    const float amount = std::min((speed - d->collisionThreshold) * d->collisionScale, d->collisionMax);
    SetFatal(hit, Damage(amount, hit.source, kType_PhysicsImpact));
}

// @func 0x00168950 RigidProp::OnHitCategory1
// NOTE: the console's push code here has undecodable instruction data in the
// reference; it is written like the blast and environment pushes.
void RigidProp::OnHitCategory1(EntityHitRecord& hit)
{
    const RigidPropDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    const float strength = hit.data.impact.push;
    if (IsPushable(rigidBody) && strength != 0.0f && d->impactPush != 0.0f)
        RigidBodyApplyImpulse(rigidBody, Scaled(hit.data.impact.direction, -strength * d->impactPush));
    if (d->impactScale > 0.0f)
        SetFatal(hit, Damage(hit.data.impact.damage * d->impactScale, hit.source, kType_Melee));
}

// @func 0x00168BD0 RigidProp::OnHitCategory2
void RigidProp::OnHitCategory2(EntityHitRecord& hit)
{
    const RigidPropDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    const auto& blast = hit.data.blast;
    float falloff = 1.0f;
    if (blast.innerRadius < blast.distance && blast.innerRadius < blast.outerRadius)
        falloff = (blast.outerRadius - blast.distance) / (blast.outerRadius - blast.innerRadius);
    if (IsPushable(rigidBody) && blast.force != 0.0f && d->blastPush > 0.0f)
        RigidBodyApplyImpulse(rigidBody, Scaled(blast.pushDirection, blast.force * d->blastPush * falloff));
    if (d->blastScale > 0.0f)
        Damage(blast.damage * falloff * d->blastScale, hit.source, kType_Blast);
    hit.flags |= EntityHitRecord::kHit_Fatal;
}

// @func 0x00168E50 RigidProp::OnHitCategory3
void RigidProp::OnHitCategory3(EntityHitRecord& hit)
{
    const RigidPropDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    if (IsPushable(rigidBody) && hit.data.knock.knockback != 0.0f && d->environmentPush != 0.0f)
        RigidBodyApplyImpulse(rigidBody, Scaled(hit.vec10, hit.data.knock.knockback * d->environmentPush));
    if (d->environmentScale > 0.0f)
        SetFatal(hit, Damage(hit.data.knock.amount * d->environmentScale, hit.source, kType_Environment));
}

// @func 0x00169040 RigidProp::OnHitCategory4
// Projectiles hurt with the hit's own reaction as the damage type.
void RigidProp::OnHitCategory4(EntityHitRecord& hit)
{
    const RigidPropDef* d = Def();
    if (!ReceiveHit(hit) || d->projectileScale <= 0.0f)
        return;
    SetFatal(hit, Damage(hit.data.projectile.damage * d->projectileScale, hit.source, hit.reaction));
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
    return Def()->projectileScale;
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
