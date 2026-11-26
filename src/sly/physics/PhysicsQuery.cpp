// openTIT - Queries against the game physics world (see PhysicsQuery.h).
//
// NOTE: the PC build has no physics world yet; every query reports "nothing
// hit" until the physics layer provides one.
#include "sly/physics/PhysicsQuery.h"

namespace sly {
namespace physics {

int CastRayAll(const ape::Vec3& from, const ape::Vec3& to, RayHit* hits, int maxHits)
{
    (void)from; (void)to; (void)hits; (void)maxHits;
    return 0;
}

bool CastRayClosest(const ape::Vec3& from, const ape::Vec3& to, uint32_t filterInfo, RayHit& hit)
{
    (void)from; (void)to; (void)filterInfo;
    hit = RayHit{};
    return false;
}

} // namespace physics
} // namespace sly
