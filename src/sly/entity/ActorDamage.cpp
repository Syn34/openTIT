// openTIT - Damage handling of the actor base (vtable 0x00B12898,
// 0x000E7FD4-0x000E8880).
#include "sly/entity/ActorEntities.h"

#include "ape/anim/ApeAnimScript.h"

#include <algorithm>

namespace sly {
namespace entity {

namespace {

// Damage types the actor hands to Damage() per category. NOTE: names unknown
// (0x80F43146 is also the impact message's reaction).
constexpr uint32_t kType_Collision = 0x80F43146u;
constexpr uint32_t kType_Impact = 0x045D9554u;
constexpr uint32_t kType_Blast = 0x03C599D8u;
constexpr uint32_t kType_Melee = 0x811B1D5Eu;

} // namespace

// @func 0x000E7730 ActorEntity::SetActive
// NOTE: the anim script runs while the flag is off (instance bit 8 is set
// to the inverse), which suggests kEnt_Active really means "frozen" for
// actors.
void ActorEntity::SetActive(bool active)
{
    if (animScript)
        animScript->SetActive(!active);
    flags = active ? (flags | kEnt_Active) : (flags & ~uint32_t(kEnt_Active));
    SetActiveVisual(active);
    if (active)
        flags = (flags & ~uint32_t(kEnt_4000)) | kEnt_8000;
    else
        flags &= ~uint32_t(kEnt_4000 | kEnt_8000);
}

// @func 0x000E7FD4 ActorEntity::InteractionNeeds
uint32_t ActorEntity::InteractionNeeds(int kind, Entity* other)
{
    (void)other;
    return Def()->interactionNeeds[kind];
}

void ActorEntity::ApplyHitDamage(EntityHitRecord& hit, float amount, uint32_t type)
{
    const bool alive = Damage(amount, hit.source, type);
    hit.flags = alive ? uint16_t(hit.flags & ~EntityHitRecord::kHit_Fatal)
                      : uint16_t(hit.flags | EntityHitRecord::kHit_Fatal);
}

// @func 0x000E8258 ActorEntity::CanBeDamaged
bool ActorEntity::CanBeDamaged() const
{
    const ActorDef* d = Def();
    const bool noCategory = d->collisionScale == 0.0f && d->impactScale == 0.0f && d->blastScale == 0.0f &&
                            d->meleeScale == 0.0f && d->projectileScale == 0.0f;
    const bool noAttacker = d->attackerScale[0] == 0.0f && d->attackerScale[1] == 0.0f &&
                            d->attackerScale[2] == 0.0f && d->attackerScale[3] == 0.0f;
    return !noAttacker && !noCategory;
}

// @func 0x000E8310 ActorEntity::OnHitCategory0
// Collisions hurt above a speed threshold, in proportion to the excess
// speed, up to a maximum.
void ActorEntity::OnHitCategory0(EntityHitRecord& hit)
{
    const ActorDef* d = Def();
    if (!ReceiveHit(hit) || d->collisionScale <= 0.0f || d->collisionMax <= 0.0f)
        return;
    const float speed = hit.data.collision.speed;
    if (speed <= d->collisionThreshold)
        return;
    const float amount = std::min((speed - d->collisionThreshold) * d->collisionScale, d->collisionMax);
    ApplyHitDamage(hit, amount, kType_Collision);
}

// @func 0x000E8408 ActorEntity::OnHitCategory1
void ActorEntity::OnHitCategory1(EntityHitRecord& hit)
{
    const ActorDef* d = Def();
    if (!ReceiveHit(hit) || d->impactScale <= 0.0f)
        return;
    ApplyHitDamage(hit, hit.data.impact.damage * d->impactScale, kType_Impact);
}

// @func 0x000E84C0 ActorEntity::OnHitCategory2
// Blasts do full damage inside the inner radius and fall off linearly to
// the outer radius.
void ActorEntity::OnHitCategory2(EntityHitRecord& hit)
{
    const ActorDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    const EntityHitRecord::Blast& blast = hit.data.blast;
    float falloff = 1.0f;
    if (blast.innerRadius < blast.distance && blast.innerRadius < blast.outerRadius)
        falloff = (blast.outerRadius - blast.distance) / (blast.outerRadius - blast.innerRadius);
    if (d->blastScale > 0.0f)
        ApplyHitDamage(hit, blast.damage * falloff * d->blastScale, kType_Blast);
}

// @func 0x000E85C0 ActorEntity::OnHitCategory3
void ActorEntity::OnHitCategory3(EntityHitRecord& hit)
{
    const ActorDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    const float amount = hit.data.knock.amount * d->meleeScale;
    if (amount > 0.0f)
        ApplyHitDamage(hit, amount, kType_Melee);
}

// @func 0x000E8678 ActorEntity::OnHitCategory4
void ActorEntity::OnHitCategory4(EntityHitRecord& hit)
{
    const ActorDef* d = Def();
    if (!ReceiveHit(hit))
        return;
    if (projectileFilter && !projectileFilter(this, hit))
        return;
    if (d->projectileScale > 0.0f)
        ApplyHitDamage(hit, hit.data.projectile.damage * d->projectileScale, hit.reaction);
}

// @func 0x000E8768 ActorEntity::Damage
// Scales the damage by the attacker's class (flag bits 22-23); running out
// of health sets bit 31 of the actor flags.
bool ActorEntity::Damage(float amount, Entity* source, uint32_t type)
{
    if (amount <= 0.0f)
        return true;
    float scale = 1.0f;
    if (source)
        scale = Def()->attackerScale[(source->flags >> 22) & 3];
    ScriptedEntity::Damage(amount * scale, source, type);
    if (health <= 0.0f)
        actorFlags |= 0x80000000u;
    return health > 0.0f;
}

// @func 0x000E886C ActorEntity::ProjectileDamageScale
float ActorEntity::ProjectileDamageScale() const
{
    return Def()->projectileScale;
}

} // namespace entity
} // namespace sly
