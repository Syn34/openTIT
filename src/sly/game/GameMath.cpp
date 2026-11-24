// openTIT - Small math helpers of the game layer.
#include "sly/game/GameMath.h"

#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"

#include <algorithm>
#include <cmath>

namespace sly {

// @func 0x004A25B0 SolveQuadratic
int SolveQuadratic(float a, float b, float c, float roots[2])
{
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
        return 0;
    const float center = b / (2.0f * a);
    const float spread = std::sqrt(discriminant) / (2.0f * a);
    if (std::fabs(spread) < 0.0001f) {
        roots[0] = -center;
        return 1;
    }
    roots[0] = spread - center;
    roots[1] = -center - spread;
    if (roots[1] < roots[0])
        std::swap(roots[0], roots[1]);
    return 2;
}

// @func 0x004A3068 LaunchVelocity
void LaunchVelocity(float time, ape::Vec3& out, const ape::Vec3& from, const ape::Vec3& to,
                    const ape::Vec3& gravity)
{
    if (!(time > 0.0f)) {
        out = ape::world::kWorldZero;
        return;
    }
    const float inv = 1.0f / time;
    const float halfT2 = 0.5f * time * time;
    out.x = ((to.x - from.x) - halfT2 * gravity.x) * inv;
    out.y = ((to.y - from.y) - halfT2 * gravity.y) * inv;
    out.z = ((to.z - from.z) - halfT2 * gravity.z) * inv;
}

// @func 0x004A57E8 RampedMoveFraction
float RampedMoveFraction(float t, float ramp)
{
    const float cruise = 1.0f - ramp;
    if (t < ramp)
        return (0.5f * t * t) / (ramp * cruise);
    if (t > cruise) {
        const float rest = 1.0f - t;
        return 1.0f - (rest * 0.5f * rest) / (ramp * cruise);
    }
    return (t - 0.5f * ramp) / cruise;
}

// The console reads a 400 x 6 table holding (i / 400)^frames; the power is
// computed directly here.
// @func 0x004A8320 DampFactor
float DampFactor(float factor, float dt)
{
    if (factor < 0.0025f || factor > 0.9975f)
        return factor;
    const int frames = std::min(std::max(int(dt * 60.0f + 0.5f), 1), 6);
    const float quantised = float(int(factor * 400.0f)) / 400.0f;
    return std::pow(quantised, float(frames));
}

// @func 0x004A83D8 ApproachExp
float ApproachExp(float from, float to, float rate, float dt)
{
    const float keep = DampFactor(1.0f - rate, dt);
    return (to - from) * (1.0f - keep) + from;
}

// @func 0x004A84A8 ApproachExp2
void ApproachExp2(float out[2], float rate, float dt, const float from[2], const float to[2])
{
    const float move = 1.0f - DampFactor(1.0f - rate, dt);
    out[0] = (to[0] - from[0]) * move + from[0];
    out[1] = (to[1] - from[1]) * move + from[1];
}

// @func 0x004A8598 ApproachExp3
void ApproachExp3(ape::Vec3& out, float rate, float dt, const ape::Vec3& from, const ape::Vec3& to)
{
    const float move = 1.0f - DampFactor(1.0f - rate, dt);
    out.x = (to.x - from.x) * move + from.x;
    out.y = (to.y - from.y) * move + from.y;
    out.z = (to.z - from.z) * move + from.z;
}

// @func 0x004A86A0 BlendKeep
float BlendKeep(float from, float to, float keep, float dt)
{
    return (to - from) * (1.0f - DampFactor(keep, dt)) + from;
}

// @func 0x004A6C50 SegmentPlaneIntersection
bool SegmentPlaneIntersection(const ape::Vec4& plane, const ape::Vec3& a, const ape::Vec3& b, ape::Vec3& out)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    const float denom = plane.x * dx + plane.y * dy + plane.z * dz;
    if (std::fabs(denom) < 1e-5f)
        return false;
    const float s = (plane.x * a.x + plane.y * a.y + plane.z * a.z + plane.w) / denom;
    out.x = a.x - s * dx;
    out.y = a.y - s * dy;
    out.z = a.z - s * dz;
    return true;
}

// Closest points of two segments with the parameters clamped to [0, 1].
// @func 0x004A44C8 SegmentSegmentDistanceSq
float SegmentSegmentDistanceSq(const ape::Vec3& a0, const ape::Vec3& a1, const ape::Vec3& b0, const ape::Vec3& b1)
{
    constexpr float kSmall = 1e-5f;
    const float ux = a1.x - a0.x, uy = a1.y - a0.y, uz = a1.z - a0.z;
    const float vx = b1.x - b0.x, vy = b1.y - b0.y, vz = b1.z - b0.z;
    const float wx = a0.x - b0.x, wy = a0.y - b0.y, wz = a0.z - b0.z;
    const float a = ux * ux + uy * uy + uz * uz;
    const float b = ux * vx + uy * vy + uz * vz;
    const float c = vx * vx + vy * vy + vz * vz;
    const float d = ux * wx + uy * wy + uz * wz;
    const float e = vx * wx + vy * wy + vz * wz;
    const float denom = a * c - b * b;

    float sN, sD = denom, tN, tD = c;
    if (denom < kSmall) {
        // Parallel: take the start of the first segment.
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
    } else {
        sN = b * e - c * d;
        tN = a * e - b * d;
        if (sN < 0.0f) {
            sN = 0.0f;
            tN = e;
        } else if (sN > sD) {
            sN = sD;
            tN = e + b;
        }
    }
    if (tN < 0.0f) {
        tN = 0.0f;
        if (-d < 0.0f) {
            sN = 0.0f;
        } else if (-d > a) {
            sN = sD;
        } else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {
        tN = tD;
        if (b - d < 0.0f) {
            sN = 0.0f;
        } else if (b - d > a) {
            sN = sD;
        } else {
            sN = b - d;
            sD = a;
        }
    }
    const float sc = std::fabs(sN) < kSmall ? 0.0f : sN / sD;
    const float tc = std::fabs(tN) < kSmall ? 0.0f : tN / tD;
    const float dx = wx + sc * ux - tc * vx;
    const float dy = wy + sc * uy - tc * vy;
    const float dz = wz + sc * uz - tc * vz;
    return dx * dx + dy * dy + dz * dz;
}

// @func 0x004A4704 SegmentsWithin
bool SegmentsWithin(float radius, const ape::Vec3& a0, const ape::Vec3& a1, const ape::Vec3& b0, const ape::Vec3& b1)
{
    return SegmentSegmentDistanceSq(a0, a1, b0, b1) <= radius * radius;
}

// @func 0x004A7130 PolygonAreaXZ
float PolygonAreaXZ(const ape::Vec3* points, uint32_t count)
{
    float sum = 0.0f;
    uint32_t previous = count - 1;
    for (uint32_t i = 0; i < count; ++i) {
        sum += points[previous].x * points[i].z - points[i].x * points[previous].z;
        previous = i;
    }
    return sum * 0.5f;
}

// @func 0x004A21F8 FindSplineInHierarchy
ape::world::ApeSplineInstance* FindSplineInHierarchy(ape::world::ApeObj* first, uint32_t nameHash)
{
    if (!first || !nameHash)
        return nullptr;
    for (ape::world::ApeObj* obj = first; obj; obj = obj->GetNextSibling()) {
        if (obj->GetType() == ape::world::kObjType_SplineInstance && obj->GetNameHash() == nameHash)
            return static_cast<ape::world::ApeSplineInstance*>(obj);
        if (auto* found = FindSplineInHierarchy(obj->GetFirstChild(), nameHash))
            return found;
    }
    return nullptr;
}

} // namespace sly
