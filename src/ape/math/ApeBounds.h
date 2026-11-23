// openTIT - Ape engine bounding box / sphere routines (0x00556F10-0x00557A20).
//
// NOTE: these live in the memory module's address range of the executable
// but are plain math; see NOTES.md (they belong next to ape/math/ApeMath).
//
// Only the x/y/z lanes of the 16-byte vectors are read or written (the
// console vector code also stored junk into the unused 4th lanes).
#pragma once

#include "ape/math/ApeMath.h"

namespace ape {

struct alignas(16) Aabb {
    Vec3 min;
    Vec3 max;
};

// Bounding sphere packed as (center.x, center.y, center.z, radius).
using Sphere = Vec4;

void  AabbFromSphere(Aabb& out, const Sphere& sphere);
void  AabbFromPoint(Aabb& out, const Vec3& p);
Vec3& AabbCenter(const Aabb& box, Vec3& out);
// Grows the box to include p / another box.
void  AabbAddPoint(Aabb& box, const Vec3& p);
void  AabbAddBox(Aabb& box, const Aabb& other);
void  AabbTranslate(Aabb& box, const Vec3& offset);
void  AabbTranslated(Aabb& out, const Aabb& box, const Vec3& offset);
// Component-wise scale of both corners.
void  AabbScale(Aabb& box, const Vec3& scale);
// Inclusive containment / overlap tests.
bool  AabbContainsPoint(const Aabb& box, const Vec3& p);
bool  AabbContainsBox(const Aabb& box, const Aabb& inner);
bool  AabbOverlaps(const Aabb& a, const Aabb& b);
// Uniformly distributed point inside the box (uses g_gameRandom).
void  AabbRandomPoint(const Aabb& box, Vec3& out);
// Sphere through the box corners, centred on the box centre.
void  SphereFromAabb(Sphere& out, const Aabb& box);

// Box enclosing 'box' transformed by m (centre/extent method).
void  AabbTransform(Aabb& out, const Aabb& box, const Matrix34& m);
void  AabbTransformInPlace(Aabb& box, const Matrix34& m);
// The eight corners of 'box' transformed by m. Corner i takes min.x when
// bit 0 of i is set (max.x otherwise), min.y for bit 1 and min.z for bit 2.
void  AabbTransformCorners(const Aabb& box, Vec3 out[8], const Matrix34& m);
// Transforms the centre; when scaleRadius is set the radius is multiplied by
// the largest axis length of m.
void  SphereTransform(Sphere& sphere, const Matrix34& m, bool scaleRadius);

// a += b on the x/y/z lanes.
void  Vec3Accumulate(Vec3& a, const Vec3& b);

} // namespace ape
