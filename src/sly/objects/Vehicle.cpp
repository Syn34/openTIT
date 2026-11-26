// openTIT - Vehicle definition and entity base (0x00226E80-0x00229C94).
#include "sly/objects/Vehicle.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/objects/CharacterMoveTuning.h"
#include "sly/objects/VehicleHooks.h"
#include "sly/objects/VehicleTuning.h"

#include <algorithm>
#include <cstdlib>

namespace sly {
namespace objects {

using entity::Entity;
using entity::EntityHitRecord;

namespace {

// Damage types handed to Damage() per hit category (the same values the
// actor base uses).
constexpr uint32_t kType_Collision = 0x80F43146u;
constexpr uint32_t kType_Impact = 0x045D9554u;
constexpr uint32_t kType_Blast = 0x03C599D8u;
constexpr uint32_t kType_Melee = 0x811B1D5Eu;
// Melee reaction that uses the definition's second melee scale.
constexpr uint32_t kReaction_MeleeSpecial = 0x04E8CCF7u;

constexpr uint32_t kScript_Destroyed = 0xD2C2058Du;   // NOTE: state names unknown
constexpr uint32_t kScript_Hit = 0x707A34ABu;

// Damage large enough to destroy any vehicle ("Entity.Vehicle.DestroyMe").
constexpr float kDestroyDamage = 666666.0f;

enum Layer0State { kState_Alive = 0, kState_Destroyed = 1 };
enum Layer1State { kDestroy_Start = 0, kDestroy_Animating = 1, kDestroy_Done = 3 };

// Entity class bits read here.
constexpr uint64_t kClass_00000001 = 0x00000001u;
constexpr uint64_t kClass_00000020 = 0x00000020u;
constexpr uint64_t kClass_Invulnerable = 0x20000u | 0x40000u;

// Entity flag bits 22-23: the team a vehicle (or its attacker) belongs to.
uint32_t TeamOf(const Entity* e) { return (e->flags >> 22) & 3; }

} // namespace

// ===========================================================================
// VehicleDef

// @func 0x00226E80 VehicleDef::~VehicleDef
// @func 0x00226EE8 VehicleDef::~VehicleDef (deleting)
VehicleDef::~VehicleDef()
{
    std::free(displayName);
    displayName = nullptr;
}

// @func 0x00226F70 VehicleDef::GetDisplayName
const char* VehicleDef::GetDisplayName() const
{
    return (displayName && *displayName) ? displayName : name;
}

// ===========================================================================
// VehicleEntity

// @func 0x00228AC0 VehicleEntity::VehicleEntity
// NOTE: the scripted entity base constructor (0x000E02D8) runs first on the
// console and takes the definition from the spawn parameters.
VehicleEntity::VehicleEntity(VehicleDef* vehicleDef)
{
    def = vehicleDef;
    invulnerableTimer = 0.0f;
    vehicleFlags = (vehicleFlags & ~uint32_t(kVehicle_FlashPending)) | kVehicle_Flag80000000 |
                   kVehicle_Flag20000000;
    if (classBits & kClass_00000001)
        renderId = 0;
    flags = (flags & ~(3u << 22)) | ((vehicleDef->team & 3) << 22);
    if (vehicleDef->defFlags & 0x80000000u)
        classBits |= kClass_00000020;
    else
        classBits &= ~kClass_00000020;
    entity::EntitySetState(this, 0, kState_Alive);
}

// @func 0x00228B84 VehicleEntity::~VehicleEntity
// @func 0x00228BB4 VehicleEntity::~VehicleEntity (deleting)
VehicleEntity::~VehicleEntity() = default;

// @func 0x00228C0C VehicleEntity::Release
bool VehicleEntity::Release(bool kill)
{
    return ScriptedEntity::Release(kill);
}

// @func 0x00228C40 VehicleEntity::Update
bool VehicleEntity::Update(float dt)
{
    if (!ScriptedEntity::Update(dt))
        return false;
    if (entity::EntityGetState(this, 0) == kState_Destroyed) {
        if (entity::EntityGetState(this, 1) == kDestroy_Start) {
            OnDestroyed();
            entity::EntitySetState(this, 1, kDestroy_Animating);
        }
        return true;
    }
    // Debug switch: destroy the vehicle (the switch resets itself).
    if (g_vehicleDestroyMe.Get()) {
        g_vehicleDestroyMe.SetBool(false);
        Damage(kDestroyDamage, nullptr, 0);
    }
    if (invulnerableTimer > 0.0f)
        invulnerableTimer = std::max(invulnerableTimer - dt, 0.0f);
    if (vehicleFlags & kVehicle_FlashPending) {
        OnHitReaction();
        vehicleFlags &= ~uint32_t(kVehicle_FlashPending);
    }
    return true;
}

// @func 0x00228E38 VehicleEntity::PostUpdate
bool VehicleEntity::PostUpdate(float dt)
{
    if (!ScriptedEntity::PostUpdate(dt))
        return false;
    if (entity::EntityGetState(this, 0) == kState_Destroyed &&
        entity::EntityGetState(this, 1) == kDestroy_Animating && StopAnim(0, false)) {
        // The destruction animation is over: the wreck has no health left.
        health = std::clamp(0.0f, 0.0f, std::max(maxHealth, 0.0f));
        entity::EntitySetState(this, 1, kDestroy_Done);
    }
    return health > 0.0f;
}

// @func 0x00228FA0 VehicleEntity::GetLocatorPosition
// Without a locator: the definition's aim point in the vehicle's frame.
ape::Vec3 VehicleEntity::GetLocatorPosition(const ape::Matrix34* locator)
{
    if (locator)
        return locator->translation;
    const ape::Vec3& p = Def()->aimOffset;
    const ape::Matrix34& m = transform;
    return {m.translation.x + p.x * m.xAxis.x + p.y * m.yAxis.x + p.z * m.zAxis.x,
            m.translation.y + p.x * m.xAxis.y + p.y * m.yAxis.y + p.z * m.zAxis.y,
            m.translation.z + p.x * m.xAxis.z + p.y * m.yAxis.z + p.z * m.zAxis.z, 0.0f};
}

bool VehicleEntity::CanTakeHit() const
{
    if (g_cheatInvulnerable.Get() || (classBits & kClass_Invulnerable))
        return false;
    if (entity::EntityComponent* blocker = entity::EntityComponent03(this))
        if (VehicleComponent03Blocks(blocker))
            return false;
    return invulnerableTimer <= 0.0f;
}

void VehicleEntity::ApplyHitDamage(EntityHitRecord& hit, float amount, uint32_t type)
{
    const bool alive = Damage(amount, hit.source, type);
    hit.flags = alive ? uint16_t(hit.flags & ~EntityHitRecord::kHit_Fatal)
                      : uint16_t(hit.flags | EntityHitRecord::kHit_Fatal);
}

// @func 0x00229088 VehicleEntity::OnHitCategory0
// Collisions hurt above a speed threshold, in proportion to the excess
// speed, up to a maximum.
void VehicleEntity::OnHitCategory0(EntityHitRecord& hit)
{
    if (!CanTakeHit() || !ReceiveHit(hit))
        return;
    const VehicleDef* d = Def();
    if (d->collisionScale <= 0.0f || d->collisionMax <= 0.0f)
        return;
    const float speed = hit.data.collision.speed;
    if (speed <= d->collisionThreshold)
        return;
    ApplyHitDamage(hit, std::min((speed - d->collisionThreshold) * d->collisionScale, d->collisionMax),
                   kType_Collision);
}

// @func 0x002291F0 VehicleEntity::OnHitCategory1
void VehicleEntity::OnHitCategory1(EntityHitRecord& hit)
{
    if (!CanTakeHit() || !ReceiveHit(hit) || Def()->impactScale <= 0.0f)
        return;
    ApplyHitDamage(hit, hit.data.impact.damage * Def()->impactScale, kType_Impact);
}

// @func 0x00229320 VehicleEntity::OnHitCategory2
// Blasts do full damage inside the inner radius and fall off linearly to the
// outer radius. The hit is always reported as fatal (the console ignores
// the damage result here).
void VehicleEntity::OnHitCategory2(EntityHitRecord& hit)
{
    if (!CanTakeHit() || !ReceiveHit(hit))
        return;
    const EntityHitRecord::Blast& blast = hit.data.blast;
    float falloff = 1.0f;
    if (blast.innerRadius < blast.distance && blast.innerRadius < blast.outerRadius)
        falloff = (blast.outerRadius - blast.distance) / (blast.outerRadius - blast.innerRadius);
    const float amount = blast.damage * falloff * Def()->blastScale;
    if (amount > 0.0f)
        Damage(amount, hit.source, kType_Blast);
    hit.flags |= EntityHitRecord::kHit_Fatal;
}

// @func 0x00229490 VehicleEntity::OnHitCategory3
void VehicleEntity::OnHitCategory3(EntityHitRecord& hit)
{
    if (!CanTakeHit() || !ReceiveHit(hit))
        return;
    const float scale = hit.reaction == kReaction_MeleeSpecial ? Def()->meleeScaleSpecial : Def()->meleeScale;
    if (scale <= 0.0f)
        return;
    ApplyHitDamage(hit, hit.data.knock.amount * scale, hit.reaction ? hit.reaction : kType_Melee);
}

// @func 0x002295F8 VehicleEntity::OnHitCategory4
void VehicleEntity::OnHitCategory4(EntityHitRecord& hit)
{
    if (!CanTakeHit() || !ReceiveHit(hit) || Def()->projectileScale <= 0.0f)
        return;
    ApplyHitDamage(hit, hit.data.projectile.damage * Def()->projectileScale, hit.reaction);
}

// @func 0x00229738 VehicleEntity::Damage
// A hit that would destroy the vehicle starts the destruction states and
// leaves a sliver of health (at most 1) for the destruction sequence; other
// hits make it invulnerable for a moment and queue the hit reaction.
bool VehicleEntity::Damage(float amount, Entity* source, uint32_t type)
{
    if (source == this || g_cheatInvulnerable.Get() || (classBits & kClass_Invulnerable))
        return true;
    if (entity::EntityComponent* blocker = entity::EntityComponent03(this))
        if (VehicleComponent03Blocks(blocker))
            return true;
    if (invulnerableTimer > 0.0f || amount <= 0.0f || entity::EntityGetState(this, 0) == kState_Destroyed)
        return true;

    const float current = health;
    if (current > 0.0f) {
        if (amount >= current) {
            entity::EntitySetState(this, 0, kState_Destroyed);
            entity::EntitySetState(this, 1, kDestroy_Start);
            // The destroyed state records the damage type that caused it.
            if (uint8_t* data = entity::EntityStateData(this, 0))
                *reinterpret_cast<uint32_t*>(data) = type;
            if (field_16C)
                entity::DamageRecordSetType(field_16C, type);
            const float sliver = std::min(current * 0.01f, 1.0f);
            ScriptedEntity::Damage(current - sliver, source, type);
        } else {
            ScriptedEntity::Damage(amount, source, type);
            invulnerableTimer = Def()->hitInvulnerableTime;
            vehicleFlags |= kVehicle_FlashPending;
        }
        // Game event 0x10 (0x00465494) for the vehicle's player.
        entity::GameWorldPostEntityEvent(uint8_t(flags >> 24), 0x10, this);
    }
    return health > 0.0f;
}

// @func 0x00229A0C VehicleEntity::ProjectileDamageScale
float VehicleEntity::ProjectileDamageScale() const
{
    return Def()->projectileScale;
}

// @func 0x00229A18 VehicleEntity::InteractionNeeds
uint32_t VehicleEntity::InteractionNeeds(int kind, Entity* other)
{
    (void)kind;
    (void)other;
    return 0;
}

// @func 0x00229A20 VehicleEntity::InteractionOffer
uint32_t VehicleEntity::InteractionOffer(int kind, Entity* other)
{
    (void)kind;
    (void)other;
    return (classBits & kClass_00000001) ? 1u : 0u;
}

// @func 0x00229A48 VehicleEntity::CanInteractWith
// Only kinds 0 and 1 while intact; never with a member of the same team,
// and kind 1 never at all.
bool VehicleEntity::CanInteractWith(Entity* other, int kind, float* priority)
{
    if (uint32_t(kind) >= 2 || health <= 0.0f)
        return false;
    if (other && (other->typeBits[2] & 0x400) && TeamOf(this) == TeamOf(other))
        return false;
    return kind == 0 ? Entity::CanInteractWith(other, kind, priority) : false;
}

// @func 0x00229B30 VehicleEntity::OnDestroyed
void VehicleEntity::OnDestroyed()
{
    if (Def()->destroyAnim)
        VehiclePlayClip(this, Def()->destroyAnim);
    if (animScript)
        animScript->SetStateAndUpdate(kScript_Destroyed);
}

// @func 0x00229C5C VehicleEntity::OnHitReaction
void VehicleEntity::OnHitReaction()
{
    if (animScript)
        animScript->SetStateAndUpdate(kScript_Hit);
}

// @func 0x00229C94 VehicleEntity::IsPlayerTeamVehicle
bool VehicleEntity::IsPlayerTeamVehicle() const
{
    return (classBits & kClass_00000020) && TeamOf(this) == 1;
}

// @func 0x0075D624 VehicleEntity::Slot94
ape::Vec3 VehicleEntity::Slot94()
{
    const ape::Vec3& p = object->GetLocal().translation;
    return { p.x, p.y, p.z, 0.0f };
}

// @func 0x0075D64C VehicleEntity::Slot95
ape::Vec3 VehicleEntity::Slot95() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

// @func 0x0075D670 VehicleEntity::Slot96
void VehicleEntity::Slot96() {}

// @func 0x0075D674 VehicleEntity::Slot97
void VehicleEntity::Slot97() {}

// @func 0x0075D680 VehicleEntity::Slot98
float VehicleEntity::Slot98() { return 0.0f; }

// @func 0x0075D690 VehicleEntity::SetFlag80000000
void VehicleEntity::SetFlag80000000(bool on)
{
    vehicleFlags = on ? (vehicleFlags | kVehicle_Flag80000000) : (vehicleFlags & ~uint32_t(kVehicle_Flag80000000));
}

} // namespace objects
} // namespace sly
