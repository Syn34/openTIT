// openTIT - <MonsterCloset> component (type 0x2E, vtable 0x00B1CD58, 0x44
// bytes): keeps a set of spawned entities alive. Each <Spawn> entry of the
// description becomes a slot; a slot whose entity died waits its respawn
// cooldown and then queues itself, and the queue is served one entity at a
// time (a turret owner first turns to the spawn point). Only the "Mine" kind
// spawns anything: a mine projectile fired from the owner's weapon towards the
// slot's spawn point.
#pragma once

#include "sly/components/Components.h"

namespace sly {
namespace components {

class MonsterClosetComponent;

// One spawn slot (vtable 0x00B1CD38, 0x30 bytes).
class MonsterClosetSlot {
public:
    enum Kind : uint32_t { kKind_None = 0, kKind_Mine = 1 };

    MonsterClosetSlot(MonsterClosetComponent* owner, const MonsterClosetSpawn& spawn);
    virtual ~MonsterClosetSlot();

    // Ticks the respawn cooldown once the entity is gone; sets `respawnDue`
    // when it runs out (0x002B43D0).
    void Update(float dt);
    // Kills the current entity and spawns a new one; `initial` places it at
    // the spawn point at once instead of firing it there (0x00251B10).
    void Respawn(bool initial);
    // Kills the current entity (if any) and forgets it.
    void KillEntity();
    // Where the entity goes: the spawn point, else the owner entity.
    const ape::Matrix34& TargetTransform() const;

    MonsterClosetComponent* component = nullptr;   // +0x04
    uint32_t kind = kKind_None;                    // +0x08 Kind
    uint32_t defName = 0;                          // +0x0C
    uint32_t spawnPointName = 0;                   // +0x10
    float respawnCooldown = 0.0f;                  // +0x14
    bool respawnEnabled = true;                    // +0x18
    entity::EntityDef* def = nullptr;              // +0x1C
    void* spawnPoint = nullptr;                    // +0x20 point instance (null: the owner)
    entity::Entity* entity = nullptr;              // +0x24 spawned entity (holds a reference)
    float cooldown = 0.0f;                         // +0x28
    bool respawnDue = false;                       // +0x2C
    bool lost = false;                             // +0x2D the entity was lost at least once
};

class MonsterClosetComponent : public EntityComponent {
public:
    explicit MonsterClosetComponent(const MonsterClosetComponentDesc* desc);
    ~MonsterClosetComponent() override;
    void Update(float dt) override;
    // Disabling kills every slot's entity; enabling respawns them all.
    void SetOption(bool value) override;

    ape::ApeTArray<MonsterClosetSlot*> slots;     // +0x18
    ape::ApeTArray<MonsterClosetSlot*> queue;     // +0x2C slots waiting to respawn, in order
    float queueDelay = 0.0f;                      // +0x40 NOTE: nothing sets it; the queue runs while 0
};

} // namespace components
} // namespace sly
