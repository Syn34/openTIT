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
        kHit_Accepted   = 0x0100,  // bit 8: set by a receiver when the hit landed
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

    // Category payload from +0x20. NOTE: only categories 1 and 3 are
    // decoded; the queue copies the whole block.
    struct Knockback {                   // category 3
        float amount;                    // +0x20
        float knockback;                 // +0x24
    };
    struct Impact {                      // category 1
        ape::Vec3 direction;             // +0x20 away from the source
        ape::Vec3 direction2;            // +0x30 (same value from impacts)
        float damage;                    // +0x40
        uint32_t field_44;
        uint32_t flags_48;               // bits 30/31 only
        uint32_t field_4C;
    };
    union {
        Knockback knock;
        Impact impact;
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
