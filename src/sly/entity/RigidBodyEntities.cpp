// openTIT - Rigid body entities: the shared hit reactions and the slot
// overrides of AlterEgoEntity, TargetableA/B and CompoundEntity
// (0x0016EF50-0x00193300). Each class has its own copy of these overrides on
// the console; they differ only in the definition block they read, the
// state their Release enters and a few details noted per function.
#include "sly/entity/PropEntities.h"

#include "sly/entity/AlterEgoBentley.h"

#include "sly/entity/EntityHits.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicalEntity.h"
#include "sly/gameplay/GameplayWorld.h"
#include "sly/gameplay/Minigame.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/PhysicsHooks.h"

#include <algorithm>
#include <cmath>

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
void ApplyImpact(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit)
{
    const float strength = hit.data.impact.push;
    if (IsPushable(body) && strength != 0.0f && settings.impactPush != 0.0f)
        RigidBodyApplyImpulse(body, Scaled(hit.data.impact.direction, -strength * settings.impactPush));
    if (settings.impactScale > 0.0f)
        SetFatal(hit, self.Damage(hit.data.impact.damage * settings.impactScale, hit.source, kType_Melee));
}

void Impact(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit)
{
    if (self.ReceiveHit(hit))
        ApplyImpact(self, body, settings, hit);
}

// Blasts: full effect inside the inner radius, falling off to the outer one.
void ApplyBlast(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit,
                bool positiveOnly)
{
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

void Blast(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit,
           bool positiveOnly)
{
    if (self.ReceiveHit(hit))
        ApplyBlast(self, body, settings, hit, positiveOnly);
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

void RigidBodyEntity::SetBodyVelocity(void* body, const ape::Vec3& velocity)
{
    if (!body || !RigidBodyInWorld(body))
        return;
    uint8_t motion = RigidBodyMotionType(body);
    if (motion == 5 || motion == 4)
        return;
    RigidBodySetLinearVelocity(body, { velocity.x, velocity.y, velocity.z, 0.0f });
}

ape::Vec3 RigidBodyEntity::BodyMassCenter() const
{
    return rigidBody ? RigidBodyCenterOfMass(rigidBody) : Position();
}

// -- definition bounds ------------------------------------------------------------------

namespace {
// Bounds of a collision shape at the identity transform (hkpShape::getAabb,
// converted to an engine box), or the entity default without a shape.
void ShapeBounds(const EntityDef* def, void* shape, ape::Aabb& out)
{
    if (shape)
        HavokShapeLocalAabb(shape, out);
    else
        EntityDefBaseBounds(def, out);
}
} // namespace

// @func 0x00167370 RigidPropDef::GetBounds
void RigidPropDef::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x0016B050 AlterEgoBentleyDef::GetBounds
void AlterEgoBentleyDef::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x00176E80 AlterEgoDef::GetBounds
void AlterEgoDef::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x0017D940 TargetableDefA::GetBounds
void TargetableDefA::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x0018FE60 TargetableDefB::GetBounds
void TargetableDefB::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x001B4650 PushBlockDef::GetBounds
void PushBlockDef::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }
// @func 0x001F9D70 TargetableDefC::GetBounds
// NOTE: also the slot of the definition class at 0x00B18F18 (inherited).
void TargetableDefC::GetBounds(ape::Aabb& out) const { ShapeBounds(this, collisionShape, out); }

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

// @func 0x0017A178 AlterEgoEntity::SendScriptEvent
void AlterEgoEntity::SendScriptEvent(uint32_t event)
{
    if (animScript1A0)
        static_cast<ape::ApeAnimScriptInstance*>(animScript1A0)->SendEvent(event);
    if (animScript1A4)
        static_cast<ape::ApeAnimScriptInstance*>(animScript1A4)->SendEvent(event);
}

// @func 0x001787E0 AlterEgoEntity::GetGravityFactor
float AlterEgoEntity::GetGravityFactor()
{
    if (!PhysicsWorldPresent())
        return 0.0f;
    void* body = GetRenderObject();
    return body ? RigidBodyGravityFactor(body) : 0.0f;
}

// @func 0x001788A0 AlterEgoEntity::Detached
void AlterEgoEntity::Detached(float gravityFactor)
{
    if (PhysicsWorldPresent()) {
        if (void* body = GetRenderObject())
            RigidBodySetGravityFactor(body, gravityFactor);
    }
    Entity::Detached(gravityFactor);
}

// @func 0x00178650 AlterEgoEntity::SetVelocity
void AlterEgoEntity::SetVelocity(const ape::Vec3& velocity) { SetBodyVelocity(rigidBody, velocity); }

// @func 0x00178744 AlterEgoEntity::GetVelocity
ape::Vec3 AlterEgoEntity::GetVelocity() { return BodyVelocity(); }

// @func 0x00179F74 AlterEgoEntity::GetAimPoint
ape::Vec3 AlterEgoEntity::GetAimPoint() { return BodyMassCenter(); }

namespace {
// NOTE: layer meaning unknown; impacts from it are ignored.
constexpr uint32_t kLayer_IgnoredImpacts = 16;
constexpr float kMaxLandingAngle = 1.0471976f;   // 60 degrees

// The impact came from a keyframed or fixed body.
bool HitByStaticBody(const EntityHitRecord& hit)
{
    void* body = HitOtherBody(hit);
    if (!body)
        return false;
    const uint8_t motion = RigidBodyMotionType(body);
    return motion == 5 || motion == 4;
}

// Angle between the contact normal and up.
float ContactAngle(const EntityHitRecord& hit)
{
    return std::acos(std::clamp(hit.data.collision.normalUp, -1.0f, 1.0f));
}

// NOTE: type bits 3 and 74 look like the player characters' (unconfirmed).
bool IsPlayerSide(const Entity* entity)
{
    return entity && (entity->HasTypeBit(3) || entity->HasTypeBit(74));
}

// Collision damage of the settings for the hit's speed (0 below the threshold).
float CollisionAmount(const PropDamageSettings& settings, const EntityHitRecord& hit)
{
    const float speed = hit.data.collision.speed;
    if (speed <= settings.collisionThreshold)
        return 0.0f;
    return std::min((speed - settings.collisionThreshold) * settings.collisionScale, settings.collisionMax);
}
} // namespace

// @func 0x00178990 AlterEgoEntity::OnHitCategory0
void AlterEgoEntity::OnHitCategory0(EntityHitRecord& hit)
{
    if (HitOtherCollisionLayer(hit) == kLayer_IgnoredImpacts || !ReceiveHit(hit))
        return;
    const AlterEgoDef* d = Def();
    const bool hardLanding = (d->deathFlags & (1u << 29)) && HitByStaticBody(hit) &&
                             ContactAngle(hit) < kMaxLandingAngle &&
                             d->damage.collisionThreshold <= hit.data.collision.speed;
    const bool touched = (d->deathFlags & (1u << 30)) && IsPlayerSide(hit.source);
    if (hardLanding || touched) {
        Damage(maxHealth, hit.source, kType_PhysicsImpact);
        hit.flags |= EntityHitRecord::kHit_Fatal;
        return;
    }
    const PropDamageSettings& settings = d->damage;
    if (settings.collisionScale <= 0.0f || settings.collisionMax <= 0.0f)
        return;
    if (hit.data.collision.speed > settings.collisionThreshold)
        propdamage::SetFatal(hit, Damage(CollisionAmount(settings, hit), hit.source, kType_PhysicsImpact));
}

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

namespace {

// Shared body of the flag-80 overrides: the body leaves the world with the
// flag, and returns (with its action) when the entity is in `activeState`.
void ToggleBodyInWorld(void* body, void* action, bool on, bool active)
{
    if (!body || RigidBodyInWorld(body) != on)
        return;
    if (on) {
        RigidBodyRemoveFromWorld(body);
    } else if (active) {
        RigidBodyAddToWorld(body, ~0u);
        if (action)
            PhysicsAddAction(action);
    }
}

// Collision group of the entity's body (slot 36 hands it out), else `fallback`.
uint32_t BodyCollisionGroup(Entity* entity, uint32_t fallback)
{
    if (!PhysicsWorldPresent())
        return fallback;
    void* body = entity->GetRenderObject();
    return body ? RigidBodyCollisionGroup(body) : fallback;
}

constexpr uint32_t kMsg_KillOrder = 0xD77F98D6u;   // NOTE: names unknown
constexpr uint32_t kKillOrder_All = 0x00014FBCu;

} // namespace

// @func 0x00190054 TargetableB::SetTargetState
void TargetableB::SetTargetState(int state)
{
    GetTargetState97();
    EntitySetState(this, 0, state);
    stateHistory[historyIndex] = uint8_t(state);
    OnTargetStateChanged();
}

// @func 0x0018E9FC TargetableB::IsFreeTarget
bool TargetableB::IsFreeTarget(Entity* entity)
{
    if (!(entity->typeBits[0] & 0x200))
        return true;
    auto* target = static_cast<TargetableB*>(entity);
    return !target->holder && (!target->stacking || !target->parent);
}

// @func 0x001918B0 TargetableB::FindObjectByModel
ape::world::ApeObj* TargetableB::FindObjectByModel(ape::world::ApeObj* object, uint32_t model)
{
    if (object->GetDef()->GetAnimHash() == model)
        return object;
    for (ape::world::ApeObj* child = object->GetFirstChild(); child; child = child->GetNextSibling()) {
        if (ape::world::ApeObj* found = FindObjectByModel(child, model))
            return found;
    }
    return nullptr;
}

// @func 0x00194714 TargetableB::GetTargetState
int TargetableB::GetTargetState() { return EntityGetState(this, 0); }

// @func 0x001962A8 TargetableB::GetTargetState97
int TargetableB::GetTargetState97() { return GetTargetState(); }

// TODO(0x001947B0 TargetableB::OnTargetStateChanged)
void TargetableB::OnTargetStateChanged() {}

// @func 0x00192220 TargetableB::SetFlag80
void TargetableB::SetFlag80(bool on)
{
    Entity::SetFlag80(on);
    ToggleBodyInWorld(rigidBody, bodyAction, on, !on && GetTargetState() == 2);
}

// @func 0x001924F8 TargetableB::GetRenderId
uint32_t TargetableB::GetRenderId() { return BodyCollisionGroup(this, collisionGroup); }

// @func 0x001975DC TargetableB::SetRenderId
void TargetableB::SetRenderId(uint32_t id)
{
    if (PhysicsWorldPresent() && rigidBody)
        RigidBodySetCollisionGroup(rigidBody, id);
    collisionGroup = id;
    Entity::SetRenderId(id);
}

namespace {
// State `stateHash` of the anim script a watcher runs, or null.
ape::ApeAnimScriptState* WatcherScriptState(ape::ApeAnimScriptInstance* instance, uint32_t stateHash)
{
    ape::ApeAnimScript* script = instance ? instance->GetScript() : nullptr;
    return script ? script->FindState(stateHash) : nullptr;
}

// NOTE: state names unknown.
constexpr uint32_t kState_WatcherStacked = 0xDC175839u;
constexpr uint32_t kState_WatcherFigure = 0x2DA34781u;
constexpr uint32_t kState_WatcherModel = 0x047034DEu;
constexpr uint32_t kMsgKey_TargetDropped = 0x0C8B8AB8u;
constexpr uint32_t kMsg_TargetDropped = 0x00531BA0u;
constexpr uint64_t kClass_Carried = 1ull << 17;
} // namespace

// @func 0x001943F0 TargetableB::AddWatcher
bool TargetableB::AddWatcher(void* watcher)
{
    Entity::AddWatcher(watcher);
    if (!stacking)
        return false;
    auto* instance = static_cast<ape::ApeAnimScriptInstance*>(static_cast<Entity*>(watcher)->GetAnimScript());
    if (!instance)
        return false;
    ape::ApeAnimScriptState* state = WatcherScriptState(instance, kState_WatcherStacked);
    if (!state || !object || !state->HasEvent(instance, object->GetNameHash()))
        return false;
    EntityPostScriptMessage(this, kMsgKey_TargetDropped, kMsg_TargetDropped, 0);
    if (holder)
        holder->held = nullptr;
    field_270 = 0;
    holder = nullptr;
    parent = nullptr;
    classBits &= ~kClass_Carried;
    Damage(maxHealth, nullptr, 0);
    return true;
}

// @func 0x0018E384 TargetableA::AddWatcher
bool TargetableA::AddWatcher(void* watcher)
{
    Entity::AddWatcher(watcher);
    auto* instance = static_cast<ape::ApeAnimScriptInstance*>(static_cast<Entity*>(watcher)->GetAnimScript());
    if (!instance)
        return true;
    ape::ApeAnimScriptState* state = nullptr;
    if (modelName == 0)
        state = WatcherScriptState(instance, kState_WatcherFigure);
    if (!state)
        state = WatcherScriptState(instance, kState_WatcherModel);
    if (!state)
        return true;
    if (!held)
        instance->SetStateAndUpdate(state);
    return false;
}

// @func 0x00197150 TargetableB::HandleMessage
void TargetableB::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_KillOrder && msg.argument == kKillOrder_All && !IgnoresKillOrder(this))
        Damage(maxHealth, nullptr, 0);
}

// @func 0x00192370 TargetableB::SetVelocity
void TargetableB::SetVelocity(const ape::Vec3& velocity) { SetBodyVelocity(rigidBody, velocity); }

// @func 0x00192464 TargetableB::GetVelocity
ape::Vec3 TargetableB::GetVelocity() { return BodyVelocity(); }

// @func 0x001931C0 TargetableB::GetAimPoint
ape::Vec3 TargetableB::GetAimPoint() { return BodyMassCenter(); }

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

// @func 0x0017DB34 TargetableA::SetTargetState
void TargetableA::SetTargetState(int state)
{
    GetTargetState97();
    EntitySetState(this, 0, state);
    stateHistory[historyIndex] = uint8_t(state);
    OnTargetStateChanged();
}

// TODO(0x00188388 TargetableA::OnTargetStateChanged)
void TargetableA::OnTargetStateChanged() {}

// @func 0x00184934 TargetableA::SetFlag80
void TargetableA::SetFlag80(bool on)
{
    Entity::SetFlag80(on);
    ToggleBodyInWorld(rigidBody, bodyAction, on, !on && GetTargetState() == 3);
}

// @func 0x00184C08 TargetableA::GetRenderId
uint32_t TargetableA::GetRenderId() { return BodyCollisionGroup(this, collisionGroup); }

// @func 0x001882B8 TargetableA::GetTargetState
int TargetableA::GetTargetState() { return EntityGetState(this, 0); }

// @func 0x0018CDDC TargetableA::GetTargetState97
int TargetableA::GetTargetState97() { return GetTargetState(); }

// @func 0x00185B04 TargetableA::InteractionOffer
uint32_t TargetableA::InteractionOffer(int kind, Entity* /*other*/) { return Def()->targetingFlags[kind]; }

// @func 0x00185AEC TargetableA::InteractionNeeds
uint32_t TargetableA::InteractionNeeds(int kind, Entity* /*other*/) { return Def()->targetingFlags[kind]; }

// @func 0x00185C9C TargetableA::FindModelPart
ape::world::ApeObj* TargetableA::FindModelPart() const
{
    if (!modelName)
        return nullptr;
    for (ape::world::ApeObj* child = model->GetFirstChild(); child; child = child->GetNextSibling()) {
        if (child->GetDef()->GetAnimHash() == modelName)
            return child;
    }
    return nullptr;
}

// @func 0x00184A80 TargetableA::SetVelocity
void TargetableA::SetVelocity(const ape::Vec3& velocity) { SetBodyVelocity(rigidBody, velocity); }

// @func 0x00184B74 TargetableA::GetVelocity
ape::Vec3 TargetableA::GetVelocity() { return BodyVelocity(); }

// @func 0x00185AC8 TargetableA::GetAimPoint
// The figure reports its own position, not the body's.
ape::Vec3 TargetableA::GetAimPoint() { return Position(); }

// @func 0x00184CC0 TargetableA::CanBeDamaged
bool TargetableA::CanBeDamaged() const { return Def()->damage.CanHurt(); }

// @func 0x00184D20 TargetableA::OnHitCategory0
void TargetableA::OnHitCategory0(EntityHitRecord& hit) { HitCollision(Def()->damage, hit); }

// @func 0x001850F0 TargetableA::OnHitCategory2
void TargetableA::OnHitCategory2(EntityHitRecord& hit) { HitBlast(Def()->damage, hit, false); }

// @func 0x00184E60 TargetableA::OnHitCategory1
void TargetableA::OnHitCategory1(EntityHitRecord& hit)
{
    if (impactCooldown > 0.0f)
        return;
    impactCooldown = 0.25f;
    propdamage::Impact(*this, rigidBody, Def()->damage, hit);
}

// @func 0x00185370 TargetableA::OnHitCategory3
// Hits with damage bits push with a fixed strength and use their own damage type.
void TargetableA::OnHitCategory3(EntityHitRecord& hit)
{
    constexpr uint32_t kType_EnvironmentBits = 0x044DC6ABu;   // NOTE: name unknown
    if (environmentCooldown > 0.0f)
        return;
    environmentCooldown = 0.25f;
    if (!ReceiveHit(hit))
        return;
    if (hit.damageBits != 0)
        hit.data.knock.knockback = 20.0f;
    const PropDamageSettings& settings = Def()->damage;
    if (IsPushable(rigidBody) && hit.data.knock.knockback != 0.0f && settings.environmentPush != 0.0f)
        RigidBodyApplyImpulse(rigidBody, Scaled(hit.vec10, hit.data.knock.knockback * settings.environmentPush));
    if (settings.environmentScale > 0.0f)
        propdamage::SetFatal(hit, Damage(hit.data.knock.amount * settings.environmentScale, hit.source,
                                         hit.damageBits != 0 ? kType_EnvironmentBits : kType_Environment));
}

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

namespace {
// Health within 0..maxHealth (as ScriptedEntity::AddHealth keeps it).
float ClampHealth(float value, float maxHealth)
{
    const float capped = std::min(value, maxHealth);
    return capped > 0.0f ? capped : 0.0f;
}
} // namespace

// @func 0x0016F350 CompoundEntity::AddHealth
void CompoundEntity::AddHealth(float amount)
{
    if (playerIndex != 1) {
        health = ClampHealth(health + amount, maxHealth);
        return;
    }
    if (IsSinglePlayerMode())
        return;
    auto* lead = static_cast<ScriptedEntity*>(PlayerEntity(1 - playerIndex));
    if (!lead)
        return;
    lead->AddHealth(amount);
    health = ClampHealth(lead->health, maxHealth);
}

// @func 0x001857A0 TargetableA::Damage
bool TargetableA::Damage(float amount, Entity* source, uint32_t type)
{
    constexpr int kState_Destroyed = 11;
    constexpr uint32_t kVar_LastAttackerDef = 0xC493B59Du;   // NOTE: name unknown
    if (GetTargetState() == kState_Destroyed)
        return false;
    if (amount <= 0.0f || GetTargetState() == 2)
        return true;
    if (source && (source->typeBits[0] & kType_IgnoredByFigures))
        return true;
    if (source && (source->typeBits[0] & 0x2000)) {
        Entity* owner = static_cast<PhysicalEntity*>(source)->owner;
        if (owner && (owner->typeBits[0] & kType_IgnoredByFigures))
            return true;
    }
    // The script learns which definition the attacker was built from.
    ape::ApeAnimScriptVar* var = animScript ? animScript->FindVar(kVar_LastAttackerDef) : nullptr;
    if (var && var->GetType() == ape::AnimScriptVarType::Int && source && (source->flags & kEnt_Alive))
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(int32_t(source->def ? source->def->nameHash : 0));

    auto* lead = static_cast<ScriptedEntity*>(PlayerEntity(0));
    if (lead && lead != this)
        health = ClampHealth(lead->health, maxHealth);
    if (health > 0.0f) {
        ScriptedEntity::Damage(amount, source, type);
        field_670 = 0;
        if (health == 0.0f)
            SetTargetState(kState_Destroyed);
    }
    if (lead && lead != this)
        lead->health = ClampHealth(health, lead->maxHealth);
    return health > 0.0f;
}

// @func 0x0018E8E8 TargetableA::AddHealth
void TargetableA::AddHealth(float amount)
{
    if (healthMode != 1) {
        health = ClampHealth(health + amount, maxHealth);
        return;
    }
    if (!healthSource)
        return;
    healthSource->AddHealth(amount);
    health = ClampHealth(healthSource->health, maxHealth);
}

// @func 0x00170418 CompoundEntity::Damage
bool CompoundEntity::Damage(float amount, Entity* source, uint32_t type)
{
    if (amount <= 0.0f)
        return true;
    const uint32_t oldFlags = buffFlags;
    field_2EC = 0;
    buffFlags |= 0x4000000;
    if (source && (source->typeBits[0] & 0x400))
        return true;
    if (oldFlags & 0x2000000)
        return health > 0.0f;

    auto* lead = static_cast<ScriptedEntity*>(PlayerEntity(0));
    if (lead && lead != this)
        health = ClampHealth(lead->health, maxHealth);
    if (health >= 0.0f) {
        ScriptedEntity::Damage(amount, source, type);
        if (health == 0.0f) {
            SetPropState(12, 0);
            gameplay::Minigame* minigame = gameplay::ActiveMinigame();
            if (minigame && (minigame->flags1C & 0x20))
                minigame->field_105 = 0;
        }
    }
    if (lead && lead != this)
        lead->health = ClampHealth(health, lead->maxHealth);
    return health > 0.0f;
}

// @func 0x0016F04C CompoundEntity::SetFlag80
void CompoundEntity::SetFlag80(bool on)
{
    Entity::SetFlag80(on);
    ToggleBodyInWorld(rigidBody, bodyAction, on, propState == 6);
}

// @func 0x0016F1C0 CompoundEntity::SetVelocity
void CompoundEntity::SetVelocity(const ape::Vec3& velocity) { SetBodyVelocity(rigidBody, velocity); }

// @func 0x0016F2B4 CompoundEntity::GetVelocity
ape::Vec3 CompoundEntity::GetVelocity() { return BodyVelocity(); }

float CompoundEntity::HeadingTo(const ape::Vec3& point) const
{
    // NOTE: the console converts with 3.1415 rather than pi.
    float degrees = std::atan2(point.y - Position().y, point.x - Position().x) * 180.0f / 3.1415f;
    if (degrees < 0.0f)
        degrees += 360.0f;
    if (degrees > 360.0f)
        degrees -= 360.0f;
    return degrees;
}

// @func 0x0016F570 CompoundEntity::OnHitCategory0
// Collisions hurt above a speed threshold (without the deferred-hit check
// of the other entities) and record their direction.
void CompoundEntity::OnHitCategory0(EntityHitRecord& hit)
{
    const PropDamageSettings& settings = DamageSettings();
    if (settings.collisionScale <= 0.0f || settings.collisionMax <= 0.0f)
        return;
    const float speed = hit.data.collision.speed;
    if (speed <= settings.collisionThreshold)
        return;
    const float amount = std::min((speed - settings.collisionThreshold) * settings.collisionScale,
                                  settings.collisionMax);
    propdamage::SetFatal(hit, Damage(amount, hit.source, kType_PhysicsImpact));
    hitHeading = int(HeadingTo(hit.vec10));
}

// @func 0x0016F730 CompoundEntity::OnHitCategory1
void CompoundEntity::OnHitCategory1(EntityHitRecord& hit)
{
    constexpr uint32_t kDef_MarksBuff = 0x659234F5u;   // NOTE: definition name unknown
    if (hit.source && hit.source->def && hit.source->def->nameHash == kDef_MarksBuff)
        buffFlags |= 0x800000;
    const PropDamageSettings& settings = DamageSettings();
    propdamage::ApplyImpact(*this, rigidBody, settings, hit);
    if (settings.impactScale > 0.0f)
        hitHeading = int(HeadingTo(hit.vec10));
}

// @func 0x0016FA70 CompoundEntity::OnHitCategory2
void CompoundEntity::OnHitCategory2(EntityHitRecord& hit)
{
    const PropDamageSettings& settings = DamageSettings();
    propdamage::ApplyBlast(*this, rigidBody, settings, hit, false);
    if (settings.blastScale > 0.0f)
        hitHeading = int(HeadingTo(hit.vec10));
}

// @func 0x001702C8 CompoundEntity::OnHitCategory4
// Projectiles from type-10 sources pass through.
void CompoundEntity::OnHitCategory4(EntityHitRecord& hit)
{
    Entity* source = hit.source;
    if (source && (source->typeBits[0] & 0x400))
        return;
    const float scale = DamageSettings().projectileScale;
    if (scale <= 0.0f)
        return;
    propdamage::SetFatal(hit, Damage(hit.data.projectile.damage * scale, source, hit.reaction));
    hitHeading = int(HeadingTo(hit.vec10));
}

// @func 0x00175B60 CompoundEntity::SetPartLevel
void CompoundEntity::SetPartLevel(uint32_t level)
{
    for (uint32_t i = 0; i <= level; ++i)
        parts[i]->Enter(this);
    partLevel = level;
}

// @func 0x0016DCF0 CompoundEntity::ForgetHavokObject
void CompoundEntity::ForgetHavokObject(void* object)
{
    if (!object)
        return;
    for (int32_t i = 0; i < field_1FC.size; ++i) {
        if (field_1FC.data[i] == object) {
            field_1FC.data[i] = nullptr;
            return;
        }
    }
}

// @func 0x00173208 CompoundEntity::UpdateScalePulse
void CompoundEntity::UpdateScalePulse(float dt)
{
    if (pulseTimer == 0.0f)
        return;
    pulseTimer = std::max(pulseTimer - dt, 0.0f);
    const float progress = 1.0f - pulseTimer / pulseTime;
    const float remaining = 1.0f - progress;
    const float base = pulseBase * remaining + progress;
    // NOTE: the console uses 3.1415 for pi here.
    const float wave = ape::FastSin(progress * 3.1415f * 2.0f * pulseFrequency);
    pulseScale = remaining * remaining * wave * wave * pulseAmplitude + base;
    pulseBase = pulseBase * 0.05f + 0.95f;
    const ape::Vec3 scale = { pulseScale, pulseScale, pulseScale, 0.0f };
    SetScale(scale, false);
}

// @func 0x0016CDE8 CompoundEntity::PlayPowerUpFx
void CompoundEntity::PlayPowerUpFx(uint32_t fx)
{
    constexpr uint32_t kFx_Death = 0x03DE9A54u;   // the "killed" event's effect
    if (health > 0.0f || lastFx != kFx_Death) {
        PlayCharacterFx(GetCharacterComponent(), fx, 1.0f, 0.25f);
        lastFx = fx;
    }
}

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
        thrower = nullptr;
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

// @func 0x001FB370 TargetableCBase::SetVelocity
void TargetableCBase::SetVelocity(const ape::Vec3& velocity) { RigidBodyEntity::SetBodyVelocity(rigidBody, velocity); }

// @func 0x001FB464 TargetableCBase::GetVelocity
ape::Vec3 TargetableCBase::GetVelocity()
{
    return rigidBody ? RigidBodyLinearVelocity(rigidBody) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x001FC570 TargetableCBase::GetAimPoint
ape::Vec3 TargetableCBase::GetAimPoint()
{
    if (EntityComponent1F(this))
        return GetCenter();
    if (!rigidBody)
        return Position();
    const ape::Vec3& offset = Def()->aimOffset;
    if (offset.x == 0.0f && offset.y == 0.0f && offset.z == 0.0f)
        return RigidBodyCenterOfMass(rigidBody);
    return GetCenter();
}

// @func 0x001FB888 TargetableCBase::CanBeDamaged
bool TargetableCBase::CanBeDamaged() const { return Def()->damage.CanHurt(); }

// @func 0x001FB910 TargetableCBase::OnHitCategory0
// Hits queued during the physics step only predict whether they will kill.
void TargetableCBase::OnHitCategory0(EntityHitRecord& hit)
{
    if (HitOtherCollisionLayer(hit) == kLayer_IgnoredImpacts)
        return;
    const TargetableDefC* d = Def();
    Entity* source = hit.source;
    const bool playerSide = IsPlayerSide(source);
    bool kill;
    if ((targetFlags >> 8 & 3) != 0 ||
        (playerSide && (d->hitFlags & (1u << 29)) && (source->classBits & 1) && source != thrower) ||
        (playerSide && (d->hitFlags & (1u << 30)) && source != thrower)) {
        kill = true;
    } else {
        kill = (d->hitFlags & (1u << 28)) && HitByStaticBody(hit) && ContactAngle(hit) < kMaxLandingAngle &&
               d->damage.collisionThreshold <= hit.data.collision.speed;
    }
    if (kill)
        hit.flags |= EntityHitRecord::kHit_Fatal;

    const float amount = CollisionAmount(d->damage, hit);
    if (!ReceiveHit(hit)) {
        if (hit.flags & EntityHitRecord::kHit_Deferrable)
            propdamage::SetFatal(hit, amount < health);
    } else if (kill) {
        Damage(maxHealth, source, kType_PhysicsImpact);
    } else if (amount > 0.0f) {
        propdamage::SetFatal(hit, Damage(amount, source, kType_PhysicsImpact));
    }
}

// @func 0x001FBC80 TargetableCBase::OnHitCategory1
void TargetableCBase::OnHitCategory1(EntityHitRecord& hit)
{
    const TargetableDefC* d = Def();
    if ((d->hitFlags & (1u << 29)) && (hit.damageBits & 0x1000))
        return;
    const float amount = hit.data.impact.damage * d->damage.impactScale;
    if (!ReceiveHit(hit)) {
        if (hit.flags & EntityHitRecord::kHit_Deferrable)
            propdamage::SetFatal(hit, amount < health);
        return;
    }
    const float strength = hit.data.impact.push;
    if (IsPushable(rigidBody) && strength != 0.0f && d->damage.impactPush != 0.0f)
        RigidBodyApplyImpulse(rigidBody, Scaled(hit.data.impact.direction, -strength * d->damage.impactPush));
    if (amount > 0.0f)
        propdamage::SetFatal(hit, Damage(amount, hit.source, kType_Melee));
    RecordMeleeHit(this, hit);
}

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
