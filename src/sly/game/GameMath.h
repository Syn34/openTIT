// openTIT - Small math helpers of the game layer: quadratic roots, ballistic
// launch velocity, motion profiles, frame-rate independent smoothing,
// segment/plane intersection, polygon area and a spline search.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::world { class ApeObj; class ApeSplineInstance; }

namespace sly {

// Real roots of a*x^2 + b*x + c in ascending order; returns their count
// (0, 1 for a double root, or 2).
int SolveQuadratic(float a, float b, float c, float roots[2]);

// Initial velocity that carries a body from `from` to `to` in `time`
// seconds under the constant acceleration `gravity` (zero for time <= 0).
void LaunchVelocity(float time, ape::Vec3& out, const ape::Vec3& from, const ape::Vec3& to,
                    const ape::Vec3& gravity);

// Distance fraction covered at time fraction t by a move that accelerates
// during the first `ramp` of the time, cruises, and decelerates during the
// last `ramp`.
float RampedMoveFraction(float t, float ramp);

// Smoothing factor `factor` (per 60 Hz frame) raised to the number of frames
// in dt (1..6), quantised to 1/400 steps. Factors outside (0.0025, 0.9975)
// are returned unchanged.
float DampFactor(float factor, float dt);
// Moves `from` towards `to` by `rate` per 60 Hz frame (dt aware).
float ApproachExp(float from, float to, float rate, float dt);
void ApproachExp2(float out[2], float rate, float dt, const float from[2], const float to[2]);
void ApproachExp3(ape::Vec3& out, float rate, float dt, const ape::Vec3& from, const ape::Vec3& to);
// Blend that keeps (1 - damped) of the way: from + (to - from) * (1 - DampFactor(keep, dt)).
float BlendKeep(float from, float to, float keep, float dt);

// Point where the segment a-b crosses the plane (n.p + w = 0); false when the
// segment is parallel to the plane.
bool SegmentPlaneIntersection(const ape::Vec4& plane, const ape::Vec3& a, const ape::Vec3& b, ape::Vec3& out);

// Orientation whose z axis is `forward`: the x axis is up x forward and the
// y axis completes the frame (translation untouched). When forward is
// vertical, the matrix's current x axis (or the unit x axis) is used as the
// reference instead of the world up.
void OrientAlong(const ape::Vec3& forward, ape::Matrix34& m);

// Fixed capacity ring of recent positions (storage owned by the caller).
struct Vec3History {
    ape::Vec3* begin = nullptr;
    ape::Vec3* end = nullptr;      // one past the storage
    ape::Vec3* write = nullptr;    // next slot to fill
    ape::Vec3* oldest = nullptr;
    uint32_t capacity = 0;
    uint32_t count = 0;
};
// Adds a sample (dropping the oldest when full) and returns the average.
void PushAndAverage(ape::Vec3& average, Vec3History& history, const ape::Vec3& sample);

// Squared distance between the segments a0-a1 and b0-b1.
float SegmentSegmentDistanceSq(const ape::Vec3& a0, const ape::Vec3& a1, const ape::Vec3& b0, const ape::Vec3& b1);
// True when the two segments come within `radius` of each other.
bool SegmentsWithin(float radius, const ape::Vec3& a0, const ape::Vec3& a1, const ape::Vec3& b0, const ape::Vec3& b1);

// Signed area of a polygon in the XZ plane.
float PolygonAreaXZ(const ape::Vec3* points, uint32_t count);

// Triangulates a simple polygon in the XZ plane by ear clipping. The
// polygon repeats its first point at the end; three points per triangle
// are appended to `triangles`.
void TriangulatePolygonXZ(const ape::ApeTArray<ape::Vec3>& polygon, ape::ApeTArray<ape::Vec3>& triangles);

// Spline instance named `nameHash` among `first`, its following siblings and
// all their descendants (depth first).
ape::world::ApeSplineInstance* FindSplineInHierarchy(ape::world::ApeObj* first, uint32_t nameHash);

} // namespace sly
