// openTIT - Hit records and the deferred hit queue.
//
// A hit (damage, knockback, projectile impact) is described by a record of
// 0x80 bytes (base vtable 0x00B0DADC). Bits 13-15 of the flag word give its
// category; scripted entities receive each category through their own
// virtual slot (83..87). Hits that arrive while the physics step runs are
// queued (0x01022080 count, records from 0x01022100, at most 0x200) and
// delivered once it is over.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace entity {

class Entity;
class ScriptedEntity;

struct EntityHitRecord {
    enum Flags : uint16_t {
        kHit_Fatal      = 0x0100,  // bit 8: set by actors when the hit killed them
        kHit_Deferrable = 0x0200,  // bit 9: may be queued during the physics step
        kHit_FromPlayer = 0x1000,  // bit 12
    };

    // Category (bits 13-15 of the flags). NOTE: the meaning of the
    // categories is not decoded; 3 is the plain knockback hit.
    int Category() const { return flags >> 13; }

    Entity* source = nullptr;            // +0x04
    uint32_t reaction = 0;               // +0x08
    uint16_t damageBits = 0;             // +0x0C matched against the receiver's mask
    uint16_t flags = 3u << 13;           // +0x0E
    ape::Vec3 vec10{};                   // +0x10 category 3: direction; category 1: hit position

    // Category payload from +0x20. NOTE: the category names are guesses
    // from the fields actors read; the queue copies the whole block.
    struct Knockback {                   // category 3
        float amount;                    // +0x20
        float knockback;                 // +0x24
    };
    struct Impact {                      // category 1
        ape::Vec3 direction;             // +0x20 away from the source
        ape::Vec3 direction2;            // +0x30 (same value from impacts)
        float damage;                    // +0x40
        float push;                      // +0x44 push strength
        uint32_t flags_48;               // bits 30/31 only
        uint32_t field_4C;
    };
    struct Collision {                   // category 0
        uint32_t field_20[4];
        float speed;                     // +0x30 compared with the actor's threshold
    };
    struct Blast {                       // category 2
        uint32_t field_20[4];
        float outerRadius;               // +0x30
        float innerRadius;               // +0x34 full damage inside
        uint32_t field_38;
        float damage;                    // +0x3C
        float force;                     // +0x40 push strength
        uint32_t field_44[3];
        ape::Vec3 pushDirection;         // +0x50
        float distance;                  // +0x60 of the receiver from the centre
    };
    struct Projectile {                  // category 4
        uint32_t field_20[5];
        float damage;                    // +0x34
    };
    union {
        Knockback knock;
        Impact impact;
        Collision collision;
        Blast blast;
        Projectile projectile;
        uint32_t raw[18];                // +0x20..+0x67
    } data = {};
};

// Queues a copy of `hit` for `target` (dropped when the queue is full).
void QueueDeferredHit(ScriptedEntity* target, const EntityHitRecord& hit);
// Forgets every queued hit for `target`.
void DropDeferredHits(const ScriptedEntity* target);
// Delivers the queued hits to their (still alive) targets and empties the
// queue (0x000CF214).
void FlushDeferredHits();

} // namespace entity
} // namespace sly
