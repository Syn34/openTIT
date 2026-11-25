// openTIT - Rigid body entities: the shared hit reactions and the slot
// overrides of AlterEgoEntity, TargetableA/B and CompoundEntity
// (0x0016EF50-0x00193300). Each class has its own copy of these overrides on
// the console; they differ only in the definition block they read, the
// state their Release enters and a few details noted per function.
#include "sly/entity/PropEntities.h"

#include "sly/entity/EntityHits.h"
#include "sly/entity/GameEntityHooks.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/CharacterHooks.h"
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
constexpr uint32_t kType_Projectile = 0xA77EB5E9u;

// Entity type bit 8: sources whose projectiles a TargetableA ignores.
constexpr uint32_t kType_IgnoredByFigures = 0x100;

// Bodies that a push cannot move (keyframed or fixed).
bool IsPushable(void* body)
{
    if (!body)
        return false;
    const uint8_t motion = RigidBodyMotionType(body);
    return motion != 5 && motion != 4;
}

ape::Vec3 Scaled(const ape::Vec3& v, float s) { return { v.x * s, v.y * s, v.z * s, 0.0f }; }

} // namespace

namespace propdamage {

void SetFatal(EntityHitRecord& hit, bool alive)
{
    hit.flags = alive ? uint16_t(hit.flags & ~EntityHitRecord::kHit_Fatal)
                      : uint16_t(hit.flags | EntityHitRecord::kHit_Fatal);
}

// Collisions hurt above a speed threshold, in proportion to the excess
// speed, up to a maximum.
void Collision(ScriptedEntity& self, const PropDamageSettings& settings, EntityHitRecord& hit)
{
    if (!self.ReceiveHit(hit) || settings.collisionScale <= 0.0f || settings.collisionMax <= 0.0f)
        return;
    const float speed = hit.data.collision.speed;
    if (speed <= settings.collisionThreshold)
        return;
    const float amount = std::min((speed - settings.collisionThreshold) * settings.collisionScale,
                                  settings.collisionMax);
    SetFatal(hit, self.Damage(amount, hit.source, kType_PhysicsImpact));
}

// NOTE: the console's push code of this reaction has undecodable instruction
// data in the reference; it is written like the blast and environment pushes
// (against the hit direction).
void Impact(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit)
{
    if (!self.ReceiveHit(hit))
        return;
    const float strength = hit.data.impact.push;
    if (IsPushable(body) && strength != 0.0f && settings.impactPush != 0.0f)
        RigidBodyApplyImpulse(body, Scaled(hit.data.impact.direction, -strength * settings.impactPush));
    if (settings.impactScale > 0.0f)
        SetFatal(hit, self.Damage(hit.data.impact.damage * settings.impactScale, hit.source, kType_Melee));
}

// Blasts: full effect inside the inner radius, falling off to the outer one.
void Blast(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit,
           bool positiveOnly)
{
    if (!self.ReceiveHit(hit))
        return;
    const auto& blast = hit.data.blast;
    float falloff = 1.0f;
    if (blast.innerRadius < blast.distance && blast.innerRadius < blast.outerRadius)
        falloff = (blast.outerRadius - blast.distance) / (blast.outerRadius - blast.innerRadius);
    if (IsPushable(body) && blast.force != 0.0f && settings.blastPush > 0.0f)
        RigidBodyApplyImpulse(body, Scaled(blast.pushDirection, blast.force * settings.blastPush * falloff));
    const float amount = blast.damage * falloff * settings.blastScale;
    if (positiveOnly ? amount > 0.0f : settings.blastScale > 0.0f)
        self.Damage(amount, hit.source, kType_Blast);
    hit.flags |= EntityHitRecord::kHit_Fatal;
}

void Environment(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit)
{
    if (!self.ReceiveHit(hit))
        return;
    if (IsPushable(body) && hit.data.knock.knockback != 0.0f && settings.environmentPush != 0.0f)
        RigidBodyApplyImpulse(body, Scaled(hit.vec10, hit.data.knock.knockback * settings.environmentPush));
    if (settings.environmentScale > 0.0f)
        SetFatal(hit, self.Damage(hit.data.knock.amount * settings.environmentScale, hit.source, kType_Environment));
}

void Projectile(ScriptedEntity& self, const PropDamageSettings& settings, EntityHitRecord& hit, uint32_t type)
{
    if (!self.ReceiveHit(hit) || settings.projectileScale <= 0.0f)
        return;
    SetFatal(hit, self.Damage(hit.data.projectile.damage * settings.projectileScale, hit.source, type));
}

} // namespace propdamage

// -- shared helpers ------------------------------------------------------------------

void RigidBodyEntity::ReleaseBody(void* action)
{
    if (action)
        HavokRelease(action);
    if (rigidBody) {
        if (RigidBodyInWorld(rigidBody))
            RigidBodyRemoveFromWorld(rigidBody);
        RigidBodyClearUserProperties(rigidBody);
        HavokRelease(rigidBody);
    }
}

ape::Vec3 RigidBodyEntity::BodyVelocity() const
{
    return rigidBody ? RigidBodyLinearVelocity(rigidBody) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

ape::Vec3 RigidBodyEntity::BodyMassCenter() const
{
    return rigidBody ? RigidBodyCenterOfMass(rigidBody) : Position();
}

// -- AlterEgoEntity (vtable 0x00B14968; damage block at definition +0xC8) -------------

// @func 0x001783E0 AlterEgoEntity::Release
bool AlterEgoEntity::Release(bool kill)
{
    if (kill) {
        SetAlterState(4);
        if (rigidBody) {
            // The constraints the body holds go first.
            for (int i = 0; i < RigidBodyConstraintCount(rigidBody); ++i)
                HavokRelease(RigidBodyConstraint(rigidBody, i));
            RigidBodyClearUserProperties(rigidBody);
            HavokRelease(rigidBody);
        }
        if (animScript1A4) {
            OrphanAndReleaseAnimScript(animScript1A4);
            animScript1A4 = nullptr;
        }
        if (animScript1A0) {
            OrphanAndReleaseAnimScript(animScript1A0);
            // NOTE: the console clears +0x1A4 here again, leaving +0x1A0 set.
            animScript1A4 = nullptr;
        }
    }
    return ScriptedEntity::Release(kill);
}

// @func 0x00178548 AlterEgoEntity::SetFlag80
void AlterEgoEntity::SetFlag80(bool on)
{
    Entity::SetFlag80(on);
    if (!rigidBody || RigidBodyInWorld(rigidBody) != on)
        return;
    if (on)
        RigidBodyRemoveFromWorld(rigidBody);
    else if (alterState != 0)
        RigidBodyAddToWorld(rigidBody, ~0u);
}

// @func 0x0017734C AlterEgoEntity::SetAlterState
void AlterEgoEntity::SetAlterState(int32_t state)
{
    if (state == alterState)
        return;
    switch (state) {
    case 1:
        SetVisible(true, true, true, true);
        if (rigidBody && !RigidBodyInWorld(rigidBody))
            RigidBodyAddToWorld(rigidBody, collisionGroup);
        else if (rigidBody)
            RigidBodySetFilterGroup(rigidBody, collisionGroup);
        field_18C = 0;
        break;
    case 3:
        field_18C = 0;
        break;
    case 4:
        if (rigidBody && RigidBodyInWorld(rigidBody))
            RigidBodyRemoveFromWorld(rigidBody);
        break;
    default:
        break;
    }
    alterState = state;
}

// @func 0x00178744 AlterEgoEntity::GetVelocity
ape::Vec3 AlterEgoEntity::GetVelocity() { return BodyVelocity(); }

// @func 0x00179F74 AlterEgoEntity::GetMassCenter
ape::Vec3 AlterEgoEntity::GetMassCenter() { return BodyMassCenter(); }

// @func 0x00178CA0 AlterEgoEntity::OnHitCategory1
void AlterEgoEntity::OnHitCategory1(EntityHitRecord& hit) { HitImpact(Def()->damage, hit); }

// @func 0x00178F20 AlterEgoEntity::OnHitCategory2
void AlterEgoEntity::OnHitCategory2(EntityHitRecord& hit) { HitBlast(Def()->damage, hit, false); }

// @func 0x001791A0 AlterEgoEntity::OnHitCategory3
void AlterEgoEntity::OnHitCategory3(EntityHitRecord& hit) { HitEnvironment(Def()->damage, hit); }

// @func 0x00179390 AlterEgoEntity::OnHitCategory4
void AlterEgoEntity::OnHitCategory4(EntityHitRecord& hit) { HitProjectile(Def()->damage, hit, hit.reaction); }

// @func 0x00179F68 AlterEgoEntity::ProjectileDamageScale
float AlterEgoEntity::ProjectileDamageScale() const { return Def()->damage.projectileScale; }

// -- TargetableB (vtable 0x00B14DE0; damage block at definition +0xC4) ----------------

// @func 0x0019211C TargetableB::Release
bool TargetableB::Release(bool kill)
{
    if (kill) {
        SetTargetState(5);
        ReleaseBody(bodyAction);
    }
    return ScriptedEntity::Release(kill);
}

// TODO(0x00190054 TargetableB::SetTargetState)
void TargetableB::SetTargetState(int state) { (void)state; }

// @func 0x00192464 TargetableB::GetVelocity
ape::Vec3 TargetableB::GetVelocity() { return BodyVelocity(); }

// @func 0x001931C0 TargetableB::GetMassCenter
ape::Vec3 TargetableB::GetMassCenter() { return BodyMassCenter(); }

// @func 0x001925B0 TargetableB::CanBeDamaged
bool TargetableB::CanBeDamaged() const { return Def()->damage.CanHurt(); }

// @func 0x00192610 TargetableB::OnHitCategory0
void TargetableB::OnHitCategory0(EntityHitRecord& hit) { HitCollision(Def()->damage, hit); }

// @func 0x00192750 TargetableB::OnHitCategory1
void TargetableB::OnHitCategory1(EntityHitRecord& hit) { HitImpact(Def()->damage, hit); }

// @func 0x001929D0 TargetableB::OnHitCategory2
// Unlike the props, only positive blast damage is applied.
void TargetableB::OnHitCategory2(EntityHitRecord& hit) { HitBlast(Def()->damage, hit, true); }

// @func 0x00192C50 TargetableB::OnHitCategory3
void TargetableB::OnHitCategory3(EntityHitRecord& hit) { HitEnvironment(Def()->damage, hit); }

// @func 0x00192E40 TargetableB::OnHitCategory4
void TargetableB::OnHitCategory4(EntityHitRecord& hit) { HitProjectile(Def()->damage, hit, kType_Projectile); }

// @func 0x001931B4 TargetableB::ProjectileDamageScale
float TargetableB::ProjectileDamageScale() const { return Def()->damage.projectileScale; }

// -- TargetableA (vtable 0x00B14B98; damage block at definition +0xC4) ----------------

// @func 0x00184830 TargetableA::Release
bool TargetableA::Release(bool kill)
{
    if (kill) {
        SetTargetState(6);
        ReleaseBody(bodyAction);
    }
    return ScriptedEntity::Release(kill);
}

// TODO(0x0017DB34 TargetableA::SetTargetState)
void TargetableA::SetTargetState(int state) { (void)state; }

// @func 0x001882B8 TargetableA::GetTargetState
int TargetableA::GetTargetState() { return EntityGetState(this, 0); }

// @func 0x0018CDDC TargetableA::GetTargetState97
int TargetableA::GetTargetState97() { return GetTargetState(); }

// @func 0x00185B04 TargetableA::InteractionOffer
uint32_t TargetableA::InteractionOffer(int kind, Entity* /*other*/) { return Def()->targetingFlags[kind]; }

// @func 0x00185AEC TargetableA::InteractionNeeds
uint32_t TargetableA::InteractionNeeds(int kind, Entity* /*other*/) { return Def()->targetingFlags[kind]; }

// @func 0x00184B74 TargetableA::GetVelocity
ape::Vec3 TargetableA::GetVelocity() { return BodyVelocity(); }

// @func 0x00185AC8 TargetableA::GetMassCenter
// The figure reports its own position, not the body's.
ape::Vec3 TargetableA::GetMassCenter() { return Position(); }

// @func 0x00184CC0 TargetableA::CanBeDamaged
bool TargetableA::CanBeDamaged() const { return Def()->damage.CanHurt(); }

// @func 0x00184D20 TargetableA::OnHitCategory0
void TargetableA::OnHitCategory0(EntityHitRecord& hit) { HitCollision(Def()->damage, hit); }

// @func 0x001850F0 TargetableA::OnHitCategory2
void TargetableA::OnHitCategory2(EntityHitRecord& hit) { HitBlast(Def()->damage, hit, false); }

// @func 0x001856B0 TargetableA::OnHitCategory4
// The figure ignores its own projectiles and those of type-8 sources.
void TargetableA::OnHitCategory4(EntityHitRecord& hit)
{
    Entity* source = hit.source;
    if (source == this || (source && (source->typeBits[0] & kType_IgnoredByFigures)))
        return;
    HitProjectile(Def()->damage, hit, kType_Projectile);
}

// @func 0x00185ABC TargetableA::ProjectileDamageScale
float TargetableA::ProjectileDamageScale() const { return Def()->damage.projectileScale; }

// -- CompoundEntity (vtable 0x00B14730; damage block at definition +0xC4) -------------

const PropDamageSettings& CompoundEntity::DamageSettings() const
{
    return static_cast<const CompoundEntityDef*>(def)->damage;
}

// @func 0x0016EF50 CompoundEntity::Release
bool CompoundEntity::Release(bool kill)
{
    if (kill) {
        SetPropState(9, 0);
        ReleaseBody(bodyAction);
    }
    return ScriptedEntity::Release(kill);
}

// TODO(0x0016BC0C CompoundEntity::SetPropState)
void CompoundEntity::SetPropState(int32_t state, uint32_t arg) { (void)arg; propState = state; }

// @func 0x0016F2B4 CompoundEntity::GetVelocity
ape::Vec3 CompoundEntity::GetVelocity() { return BodyVelocity(); }

// @func 0x0016F4F8 CompoundEntity::CanBeDamaged
bool CompoundEntity::CanBeDamaged() const { return DamageSettings().CanHurt(); }

// @func 0x00170630 CompoundEntity::ProjectileDamageScale
float CompoundEntity::ProjectileDamageScale() const { return DamageSettings().projectileScale; }

// -- PushBlock (vtable 0x00B160A8) ----------------------------------------------------

// @func 0x001B8D84 PushBlock::GetVelocity
ape::Vec3 PushBlock::GetVelocity()
{
    return rigidBody ? RigidBodyLinearVelocity(rigidBody) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x001B8DF8 PushBlock::OnSuspend
void PushBlock::OnSuspend()
{
    ScriptedEntity::OnSuspend();
    savedVelocity = { 0.0f, 0.0f, 0.0f, savedVelocity.pad };
    if (rigidBody && RigidBodyInWorld(rigidBody)) {
        const ape::Vec3 velocity = RigidBodyLinearVelocity(rigidBody);
        savedVelocity = { velocity.x, velocity.y, velocity.z, savedVelocity.pad };
        RigidBodySetLinearVelocity(rigidBody, { 0.0f, 0.0f, 0.0f, 0.0f });
    }
}

// @func 0x001B900C PushBlock::CanInteractWith
bool PushBlock::CanInteractWith(Entity* other, int kind)
{
    if ((kind == 0 && blockFlags < 0) || kind == 4)
        return false;
    return Entity::CanInteractWith(other, kind);
}

// -- TargetableCBase (vtable 0x00B18B48; damage block at definition +0xE8) -----------

namespace {
// Environment reaction that hurts at full strength. NOTE: name unknown.
constexpr uint32_t kReaction_FullStrength = 0x04E8CCF7u;
} // namespace

// @func 0x001FAE1C TargetableCBase::Release
bool TargetableCBase::Release(bool kill)
{
    if (kill) {
        SetTargetState(4, 0);
        if (rigidBody) {
            for (int i = 0; i < RigidBodyConstraintCount(rigidBody); ++i)
                HavokRelease(RigidBodyConstraint(rigidBody, i));
            RigidBodyClearUserProperties(rigidBody);
            HavokRelease(rigidBody);
            rigidBody = nullptr;
        }
        field_1AC = 0;
    }
    return ScriptedEntity::Release(kill);
}

// TODO(0x001F9F70 TargetableCBase::SetTargetState)
void TargetableCBase::SetTargetState(int32_t state, uint32_t arg) { (void)state; (void)arg; }

// @func 0x001FB770 TargetableCBase::GetGravityFactor
// NOTE: Entity slot 36 ("GetRenderObject") hands out the physics body here.
float TargetableCBase::GetGravityFactor()
{
    if (attachParent) {
        const EntityChildLink* link = attachParent->FindChildLink(this);
        return link ? link->restoreBlend : 0.0f;
    }
    void* body = GetRenderObject();
    return body ? RigidBodyGravityFactor(body) : 0.0f;
}

// @func 0x001FB804 TargetableCBase::Detached
void TargetableCBase::Detached(float gravityFactor)
{
    if (void* body = GetRenderObject())
        RigidBodySetGravityFactor(body, gravityFactor);
    Entity::Detached(gravityFactor);
}

// @func 0x001FB464 TargetableCBase::GetVelocity
ape::Vec3 TargetableCBase::GetVelocity()
{
    return rigidBody ? RigidBodyLinearVelocity(rigidBody) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x001FB888 TargetableCBase::CanBeDamaged
bool TargetableCBase::CanBeDamaged() const { return Def()->damage.CanHurt(); }

// @func 0x001FBF80 TargetableCBase::OnHitCategory2
void TargetableCBase::OnHitCategory2(EntityHitRecord& hit)
{
    propdamage::Blast(*this, rigidBody, Def()->damage, hit, false);
}

// @func 0x001FC200 TargetableCBase::OnHitCategory3
// Like the standard environment reaction, except that one reaction always
// hurts at full strength and only positive damage is applied.
void TargetableCBase::OnHitCategory3(EntityHitRecord& hit)
{
    const PropDamageSettings& settings = Def()->damage;
    if (!ReceiveHit(hit))
        return;
    if (rigidBody && RigidBodyMotionType(rigidBody) != 5 && RigidBodyMotionType(rigidBody) != 4 &&
        hit.data.knock.knockback != 0.0f && settings.environmentPush != 0.0f) {
        const float s = hit.data.knock.knockback * settings.environmentPush;
        RigidBodyApplyImpulse(rigidBody, { hit.vec10.x * s, hit.vec10.y * s, hit.vec10.z * s, 0.0f });
    }
    const float scale = hit.reaction == kReaction_FullStrength ? 1.0f : settings.environmentScale;
    const float amount = hit.data.knock.amount * scale;
    if (amount > 0.0f)
        propdamage::SetFatal(hit, Damage(amount, hit.source, kType_Environment));
}

// @func 0x001FC410 TargetableCBase::OnHitCategory4
void TargetableCBase::OnHitCategory4(EntityHitRecord& hit)
{
    propdamage::Projectile(*this, Def()->damage, hit, hit.reaction);
}

// @func 0x001FC4C8 TargetableCBase::Damage
bool TargetableCBase::Damage(float amount, Entity* source, uint32_t type)
{
    if (amount <= 0.0f)
        return true;
    ScriptedEntity::Damage(amount, source, type);
    if (health == 0.0f)
        targetFlags |= 0x400;
    return health > 0.0f;
}

// @func 0x001FC564 TargetableCBase::ProjectileDamageScale
float TargetableCBase::ProjectileDamageScale() const { return Def()->damage.projectileScale; }

} // namespace entity
} // namespace sly
