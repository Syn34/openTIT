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
        } else {
            tD = denom;
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

namespace {
ape::Vec3 Cross(const ape::Vec3& a, const ape::Vec3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f};
}
ape::Vec3 Normalized(const ape::Vec3& v)
{
    const float inv = 1.0f / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {v.x * inv, v.y * inv, v.z * inv, 0.0f};
}
bool IsFiniteVec(const ape::Vec3& v)
{
    return std::fabs(v.x - v.x) < 1e-5f && std::fabs(v.y - v.y) < 1e-5f && std::fabs(v.z - v.z) < 1e-5f;
}
} // namespace

// @func 0x004A54F8 OrientAlong
void OrientAlong(const ape::Vec3& forward, ape::Matrix34& m)
{
    static const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};
    // NOTE: the console falls back to a global (0x01164B50) assumed to be
    // the unit x axis.
    static const ape::Vec3 kUnitX = {1.0f, 0.0f, 0.0f, 0.0f};
    const float upDot = forward.x * kUp.x + forward.y * kUp.y + forward.z * kUp.z;
    const ape::Vec3 dir = {forward.x, forward.y, forward.z, 0.0f};
    if (std::fabs(std::fabs(upDot) - 1.0f) < 1e-5f || std::fabs(upDot + 1.0f) < 1e-5f) {
        const ape::Vec3 reference = IsFiniteVec(m.xAxis) ? m.xAxis : kUnitX;
        m.yAxis = Normalized(Cross(dir, reference));
        m.zAxis = dir;
        m.xAxis = Normalized(Cross(m.yAxis, m.zAxis));
    } else {
        m.xAxis = Normalized(Cross(kUp, dir));
        m.yAxis = Normalized(Cross(dir, m.xAxis));
        m.zAxis = dir;
    }
}

// @func 0x004A69C0 PushAndAverage
void PushAndAverage(ape::Vec3& average, Vec3History& history, const ape::Vec3& sample)
{
    auto advance = [&history](ape::Vec3* p) { return p + 1 < history.end ? p + 1 : history.begin; };
    if (history.count == history.capacity) {
        --history.count;
        history.oldest = advance(history.oldest);
    }
    if (history.count != history.capacity) {
        ++history.count;
        ape::Vec3* slot = history.write;
        history.write = advance(slot);
        if (slot)
            *slot = {sample.x, sample.y, sample.z, slot->pad};
    }
    average = ape::world::kWorldZero;
    float samples = 0.0f;
    if (history.count) {
        ape::Vec3* p = history.oldest;
        do {
            average.x += p->x;
            average.y += p->y;
            average.z += p->z;
            samples += 1.0f;
            p = advance(p);
        } while (p != history.write);
    }
    if (samples > 1.0f) {
        const float inv = 1.0f / samples;
        average.x *= inv;
        average.y *= inv;
        average.z *= inv;
    }
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
