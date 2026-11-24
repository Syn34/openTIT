// openTIT - Small geometric queries used by the world layer: segment/quad
// intersection (portal quads) and the cell range of the 8x8 light grid.
#include "ape/world/ApeWorldMath.h"

#include <cmath>

namespace ape::world {

ApeLightGrid g_lightGrid;

// Intersects the segment start->end with a planar convex quad (corners in
// winding order). On a hit the segment parameter goes to outT.
// @func 0x005784E8 SegmentIntersectsQuad
bool SegmentIntersectsQuad(const Vec3 corners[4], const Vec3& start, const Vec3& end, float& outT)
{
    const Vec3& c0 = corners[0];
    const Vec3 e1 = {corners[1].x - c0.x, corners[1].y - c0.y, corners[1].z - c0.z, 0.0f};
    const Vec3 e2 = {corners[2].x - c0.x, corners[2].y - c0.y, corners[2].z - c0.z, 0.0f};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x, 0.0f};
    const float inv = 1.0f / std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    n.x *= inv;
    n.y *= inv;
    n.z *= inv;

    const Vec3 dir = {end.x - start.x, end.y - start.y, end.z - start.z, 0.0f};
    const float denom = dir.x * n.x + dir.y * n.y + dir.z * n.z;
    if (std::fabs(denom) < 1e-5f)
        return false;
    const float t = ((c0.x - start.x) * n.x + (c0.y - start.y) * n.y + (c0.z - start.z) * n.z) / denom;
    if (t < 0.0f || t > 1.0f)
        return false;

    const Vec3 hit = {start.x + dir.x * t, start.y + dir.y * t, start.z + dir.z * t, 0.0f};
    for (int i = 0; i < 4; ++i) {
        const Vec3& a = corners[i];
        const Vec3& b = corners[(i + 1) & 3];
        const Vec3 edge = {b.x - a.x, b.y - a.y, b.z - a.z, 0.0f};
        const Vec3 toHit = {hit.x - a.x, hit.y - a.y, hit.z - a.z, 0.0f};
        const float side = (edge.y * toHit.z - edge.z * toHit.y) * n.x +
                           (edge.z * toHit.x - edge.x * toHit.z) * n.y +
                           (edge.x * toHit.y - edge.y * toHit.x) * n.z;
        if (side < 0.0f)
            return false;
    }
    outT = t;
    return true;
}

// Grid cells (x and z, 0..7) covered by a sphere, clamped to the grid.
// @func 0x005820E0 ComputeLightGridCells
void ComputeLightGridCells(const Sphere& sphere, int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ)
{
    const ApeLightGrid& grid = g_lightGrid;
    const float minX = std::fmax(sphere.x - sphere.w, grid.min.x);
    const float maxX = std::fmin(sphere.x + sphere.w, grid.max.x);
    const float minZ = std::fmax(sphere.z - sphere.w, grid.min.z);
    const float maxZ = std::fmin(sphere.z + sphere.w, grid.max.z);

    auto cell = [](float v) {
        if (!(v >= 0.0f))
            return 0.0f;
        return v <= 7.0f ? v : 7.0f;
    };
    outMinX = int(cell((minX - grid.min.x) * grid.invCellSize.x));
    outMaxX = int(cell((maxX - grid.min.x) * grid.invCellSize.x));
    outMinZ = int(cell((minZ - grid.min.z) * grid.invCellSize.z));
    outMaxZ = int(cell((maxZ - grid.min.z) * grid.invCellSize.z));
}

} // namespace ape::world
