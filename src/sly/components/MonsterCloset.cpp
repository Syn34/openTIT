// openTIT - <MonsterCloset> component (0x00251B10-0x00252258,
// 0x002B4264-0x002B4E78).
#include "sly/components/MonsterCloset.h"

#include "sly/components/ComponentHooks.h"
#include "sly/entity/Projectiles.h"
#include "sly/entity/ScriptedEntity.h"

namespace sly {
namespace components {

namespace {
constexpr uint32_t kState_Fire = 0x0020EE36u;          // "Fire"
constexpr uint32_t kDefFlag_Projectile = 0x2000u;      // EntityDef::classBits
constexpr uint32_t kTypeBit_Scripted = 0x2u;           // Entity::typeBits[0]
constexpr uint32_t kTypeBit_Mine = 0x20000u;           // Entity::typeBits[0] (EntityProjectileMine)
constexpr uint32_t kTypeBit_Turret = 0x2000000u;       // Entity::typeBits[1]

entity::EntityDef* ProjectileDef(entity::EntityDef* def)
{
    return def && (def->classBits & kDefFlag_Projectile) ? def : nullptr;
}
} // namespace

// -- slots ----------------------------------------------------------------------------

MonsterClosetSlot::MonsterClosetSlot(MonsterClosetComponent* owner, const MonsterClosetSpawn& spawn)
    : component(owner)
    , kind(spawn.type)
    , defName(spawn.def)
    , spawnPointName(spawn.spawnPoint)
    , respawnCooldown(spawn.respawnCooldown)
    , respawnEnabled(spawn.respawnEnabled)
{
    if (spawnPointName)
        spawnPoint = FindPointInstance(spawnPointName);
    def = entity::FindEntityDef(defName);
    if (def)
        Respawn(true);
}

// @func 0x002B4264 MonsterClosetSlot::~MonsterClosetSlot
// @func 0x002B4308 MonsterClosetSlot::~MonsterClosetSlot (deleting)
MonsterClosetSlot::~MonsterClosetSlot()
{
    if (entity && entity->IsAlive()) {
        entity->Release(false);
        entity->Release(true);
        entity = nullptr;
    }
}

const ape::Matrix34& MonsterClosetSlot::TargetTransform() const
{
    return spawnPoint ? PointInstanceTransform(spawnPoint) : component->owner->transform;
}

void MonsterClosetSlot::KillEntity()
{
    if (!entity)
        return;
    entity->Release(false);
    // A scripted entity is also damaged to death so it plays its death.
    if (entity->typeBits[0] & kTypeBit_Scripted) {
        auto* scripted = static_cast<entity::ScriptedEntity*>(entity);
        scripted->Damage(scripted->maxHealth, nullptr, 0);
    }
    entity = nullptr;
}

// @func 0x002B43D0 MonsterClosetSlot::Update
void MonsterClosetSlot::Update(float dt)
{
    if (entity && !entity->IsAlive()) {
        entity->Release(false);
        entity = nullptr;
        cooldown = respawnCooldown;
        lost = true;
    }
    if (!entity && respawnEnabled) {
        cooldown -= dt;
        if (cooldown <= 0.0f)
            respawnDue = true;
    }
}

// @func 0x00251B10 MonsterClosetSlot::Respawn
void MonsterClosetSlot::Respawn(bool initial)
{
    KillEntity();
    respawnDue = false;
    cooldown = 0.0f;

    if (lost && !respawnEnabled)
        return;
    if (kind != kKind_Mine || !ProjectileDef(def))
        return;
    entity::Entity* shooter = component->owner;
    EntityComponent* skin = entity::EntitySkin(shooter);
    if (!skin)
        return;

    const ape::Matrix34 target = TargetTransform();
    ape::Matrix34 muzzle = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
    SkinWeaponMuzzle(skin, muzzle, SkinActiveWeaponSlot(skin));

    // The first spawn appears at the target; later ones are fired from the
    // weapon towards it.
    const ape::Matrix34 start = initial ? target : muzzle;
    const ape::Vec3 velocity = {target.translation.x - start.translation.x,
                                target.translation.y - start.translation.y,
                                target.translation.z - start.translation.z, 0.0f};
    shooter->SetTimeScale(kState_Fire, true);

    entity::Entity* spawned = SpawnWeaponProjectile(0.0f, ProjectileDef(def), shooter, start, velocity, target);
    if (!spawned)
        return;
    if (!initial && (spawned->typeBits[0] & kTypeBit_Mine))
        static_cast<entity::EntityProjectileMine*>(spawned)->SetTargetPoint(target.translation);
    spawned->AddRef();
    entity = spawned;
}

// -- component ------------------------------------------------------------------------

// @func 0x00251F64 MonsterClosetComponent::MonsterClosetComponent
MonsterClosetComponent::MonsterClosetComponent(const MonsterClosetComponentDesc* desc)
    : EntityComponent(desc)
{
    slots.Init(0x20, nullptr, true);
    queue.Init(0x20, nullptr, true);
    for (const MonsterClosetSpawn& spawn : desc->spawns)
        slots.PushBack(new MonsterClosetSlot(this, spawn));
}

// @func 0x002B4830 MonsterClosetComponent::~MonsterClosetComponent
// @func 0x002B49FC MonsterClosetComponent::~MonsterClosetComponent (deleting)
MonsterClosetComponent::~MonsterClosetComponent()
{
    for (MonsterClosetSlot* slot : slots)
        delete slot;
    slots.Clear();
}

// @func 0x002B4A48 MonsterClosetComponent::Update
void MonsterClosetComponent::Update(float dt)
{
    for (MonsterClosetSlot* slot : slots) {
        slot->Update(dt);
        if (slot->respawnDue && queue.FindLast(slot) < 0)
            queue.PushBack(slot);
    }

    queueDelay -= dt;
    if (queueDelay < 0.0f)
        queueDelay = 0.0f;
    while (queue.Count() != 0 && queueDelay == 0.0f) {
        MonsterClosetSlot* next = queue[0];
        // A turret owner first turns towards the spawn point.
        if (owner && (owner->typeBits[1] & kTypeBit_Turret)) {
            TurretAimAt(owner, next->TargetTransform().translation);
            if (!TurretIsAimed(owner))
                return;
        }
        next->Respawn(false);
        for (uint32_t i = 1; i < queue.Count(); ++i)
            queue[i - 1] = queue[i];
        queue.PopBack();
    }
}

// @func 0x002B4D14 MonsterClosetComponent::SetOption
void MonsterClosetComponent::SetOption(bool value)
{
    if (value != enabled) {
        for (MonsterClosetSlot* slot : slots) {
            if (value)
                slot->Respawn(true);
            else
                slot->KillEntity();
        }
    }
    enabled = value;
}

} // namespace components
} // namespace sly
