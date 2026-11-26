// openTIT - Queries against the game physics world.
//
// Game code asks the physics world questions (what does a ray hit, where does
// a moving body stop) through these functions instead of reaching into Havok.
// On the console they wrap hkpWorld::castRay with the closest-hit or all-hits
// collectors (0x0092E510) on the level's physics world (game world
// +0x1240) and resolve the hit collidable to the entity that owns it: the
// collidable's entity (broadphase type 1) or phantom (type 2) carries the
// property 0xA7334D4D = 0x7A30D7E3 ("is an entity") and the entity pointer
// under property 0x91C730A5.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace entity { class Entity; }
namespace physics {

// One ray hit.
struct RayHit {
    float fraction = 1.0f;            // along the ray (0 = start, 1 = end)
    ape::Vec3 normal{};               // surface normal at the hit
    uint32_t collisionLayer = 0;      // low five bits of the collidable's filter word
    entity::Entity* entity = nullptr; // entity owning the collidable, null for level geometry
    void* body = nullptr;             // the hit rigid body / phantom (opaque)
};

// Casts a ray from `from` to `to` and reports every hit (in no particular
// order), at most `maxHits`; returns the number reported.
int CastRayAll(const ape::Vec3& from, const ape::Vec3& to, RayHit* hits, int maxHits);
// Casts a ray and reports the nearest hit accepted by `filterInfo` (a
// collision filter word; 0 accepts everything). False when nothing is hit.
bool CastRayClosest(const ape::Vec3& from, const ape::Vec3& to, uint32_t filterInfo, RayHit& hit);

} // namespace physics
} // namespace sly
