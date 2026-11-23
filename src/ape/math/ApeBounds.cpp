// openTIT - Ape engine bounding box / sphere routines (0x00556F10-0x00557A20).
#include "ape/math/ApeBounds.h"

#include <cmath>

namespace ape {

namespace {

// Selection written the way the engine does it ("a - b < 0"), which keeps
// the current bound when the incoming value is NaN.
float KeepMin(float current, float value)
{
    return (current - value < 0.0f) ? current : value;
}

float KeepMax(float current, float value)
{
    return (current - value < 0.0f) ? value : current;
}

bool AllGreaterEqual(const Vec3& a, const Vec3& b)
{
    return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}

void SetXYZ(Vec3& v, float x, float y, float z)
{
    v.x = x;
    v.y = y;
    v.z = z;
}

// (p, 1) * m on the x/y/z lanes.
void TransformPoint(Vec3& out, float x, float y, float z, const Matrix34& m)
{
    const float rx = m.xAxis.x * x + m.yAxis.x * y + m.zAxis.x * z + m.translation.x;
    const float ry = m.xAxis.y * x + m.yAxis.y * y + m.zAxis.y * z + m.translation.y;
    const float rz = m.xAxis.z * x + m.yAxis.z * y + m.zAxis.z * z + m.translation.z;
    SetXYZ(out, rx, ry, rz);
}

float AxisLength(const Vec3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

} // namespace

// @func 0x00556F10 AabbFromSphere
void AabbFromSphere(Aabb& out, const Sphere& sphere)
{
    const float r = sphere.w;
    SetXYZ(out.min, sphere.x - r, sphere.y - r, sphere.z - r);
    SetXYZ(out.max, sphere.x + r, sphere.y + r, sphere.z + r);
}

// @func 0x00556F60 AabbFromPoint
void AabbFromPoint(Aabb& out, const Vec3& p)
{
    SetXYZ(out.min, p.x, p.y, p.z);
    SetXYZ(out.max, p.x, p.y, p.z);
}

// @func 0x00556F98 AabbCenter
Vec3& AabbCenter(const Aabb& box, Vec3& out)
{
    SetXYZ(out,
           (box.max.x - box.min.x) * 0.5f + box.min.x,
           (box.max.y - box.min.y) * 0.5f + box.min.y,
           (box.max.z - box.min.z) * 0.5f + box.min.z);
    return out;
}

// @func 0x00556FE8 AabbAddPoint
void AabbAddPoint(Aabb& box, const Vec3& p)
{
    SetXYZ(box.min, KeepMin(box.min.x, p.x), KeepMin(box.min.y, p.y), KeepMin(box.min.z, p.z));
    SetXYZ(box.max, KeepMax(box.max.x, p.x), KeepMax(box.max.y, p.y), KeepMax(box.max.z, p.z));
}

// @func 0x00557064 AabbAddBox
void AabbAddBox(Aabb& box, const Aabb& other)
{
    SetXYZ(box.min, KeepMin(box.min.x, other.min.x), KeepMin(box.min.y, other.min.y),
           KeepMin(box.min.z, other.min.z));
    SetXYZ(box.max, KeepMax(box.max.x, other.max.x), KeepMax(box.max.y, other.max.y),
           KeepMax(box.max.z, other.max.z));
}

// @func 0x00557284 AabbTranslate
void AabbTranslate(Aabb& box, const Vec3& offset)
{
    AabbTranslated(box, box, offset);
}

// @func 0x005572E8 AabbTranslated
void AabbTranslated(Aabb& out, const Aabb& box, const Vec3& offset)
{
    SetXYZ(out.min, box.min.x + offset.x, box.min.y + offset.y, box.min.z + offset.z);
    SetXYZ(out.max, box.max.x + offset.x, box.max.y + offset.y, box.max.z + offset.z);
}

// @func 0x00557358 AabbScale
void AabbScale(Aabb& box, const Vec3& scale)
{
    SetXYZ(box.min, box.min.x * scale.x, box.min.y * scale.y, box.min.z * scale.z);
    SetXYZ(box.max, box.max.x * scale.x, box.max.y * scale.y, box.max.z * scale.z);
}

// @func 0x005573D0 AabbContainsPoint
bool AabbContainsPoint(const Aabb& box, const Vec3& p)
{
    return AllGreaterEqual(p, box.min) && AllGreaterEqual(box.max, p);
}

// @func 0x00557430 AabbContainsBox
bool AabbContainsBox(const Aabb& box, const Aabb& inner)
{
    return AllGreaterEqual(inner.min, box.min) && AllGreaterEqual(box.max, inner.max);
}

// @func 0x005574A0 AabbOverlaps
bool AabbOverlaps(const Aabb& a, const Aabb& b)
{
    return a.min.x <= b.max.x && b.min.x <= a.max.x &&
           a.min.y <= b.max.y && b.min.y <= a.max.y &&
           a.min.z <= b.max.z && b.min.z <= a.max.z;
}

// @func 0x00557568 AabbRandomPoint
void AabbRandomPoint(const Aabb& box, Vec3& out)
{
    const float rx = RandomUnit(g_gameRandom);
    const float ry = RandomUnit(g_gameRandom);
    const float rz = RandomUnit(g_gameRandom);
    SetXYZ(out,
           (box.max.x - box.min.x) * rx + box.min.x,
           (box.max.y - box.min.y) * ry + box.min.y,
           (box.max.z - box.min.z) * rz + box.min.z);
}

// @func 0x005577E0 SphereFromAabb
// @func 0x00557870 SphereFromAabb (identical second copy)
void SphereFromAabb(Sphere& out, const Aabb& box)
{
    Vec3 center;
    AabbCenter(box, center);
    const float dx = box.max.x - center.x;
    const float dy = box.max.y - center.y;
    const float dz = box.max.z - center.z;
    out.x = center.x;
    out.y = center.y;
    out.z = center.z;
    out.w = std::sqrt(dx * dx + dy * dy + dz * dz);
}

// @func 0x005570F0 AabbTransform
void AabbTransform(Aabb& out, const Aabb& box, const Matrix34& m)
{
    const float cx = (box.max.x + box.min.x) * 0.5f;
    const float cy = (box.max.y + box.min.y) * 0.5f;
    const float cz = (box.max.z + box.min.z) * 0.5f;
    const float ex = (box.max.x - box.min.x) * 0.5f;
    const float ey = (box.max.y - box.min.y) * 0.5f;
    const float ez = (box.max.z - box.min.z) * 0.5f;

    Vec3 center;
    TransformPoint(center, cx, cy, cz, m);
    // Extents of the rotated box: |R|^T * e.
    const float rx = std::fabs(m.xAxis.x) * ex + std::fabs(m.yAxis.x) * ey + std::fabs(m.zAxis.x) * ez;
    const float ry = std::fabs(m.xAxis.y) * ex + std::fabs(m.yAxis.y) * ey + std::fabs(m.zAxis.y) * ez;
    const float rz = std::fabs(m.xAxis.z) * ex + std::fabs(m.yAxis.z) * ey + std::fabs(m.zAxis.z) * ez;

    SetXYZ(out.min, center.x - rx, center.y - ry, center.z - rz);
    SetXYZ(out.max, center.x + rx, center.y + ry, center.z + rz);
}

// @func 0x005571C0 AabbTransformInPlace
void AabbTransformInPlace(Aabb& box, const Matrix34& m)
{
    AabbTransform(box, Aabb(box), m);
}

// @func 0x00557690 AabbTransformCorners
void AabbTransformCorners(const Aabb& box, Vec3 out[8], const Matrix34& m)
{
    for (int i = 0; i < 8; ++i) {
        const float x = (i & 1) ? box.min.x : box.max.x;
        const float y = (i & 2) ? box.min.y : box.max.y;
        const float z = (i & 4) ? box.min.z : box.max.z;
        TransformPoint(out[i], x, y, z, m);
    }
}

// @func 0x00557900 SphereTransform
void SphereTransform(Sphere& sphere, const Matrix34& m, bool scaleRadius)
{
    Vec3 center;
    TransformPoint(center, sphere.x, sphere.y, sphere.z, m);
    sphere.x = center.x;
    sphere.y = center.y;
    sphere.z = center.z;
    if (scaleRadius) {
        float scale = AxisLength(m.xAxis);
        const float ly = AxisLength(m.yAxis);
        if (scale - ly < 0.0f)
            scale = ly;
        const float lz = AxisLength(m.zAxis);
        if (lz - scale >= 0.0f)
            scale = lz;
        sphere.w *= scale;
    }
}

// @func 0x00557A20 Vec3Accumulate
void Vec3Accumulate(Vec3& a, const Vec3& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

} // namespace ape
