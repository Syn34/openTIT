// openTIT - Ape engine cubic Bezier splines (0x0055EC98-0x00560A40).
//
// A spline is an array of control points. Each point has a position and two
// tangent handles stored relative to it; segment i is the cubic Bezier
//     P0 = p[i].position
//     P1 = p[i].position   + p[i].outTangent
//     P2 = p[i+1].position + p[i+1].inTangent
//     P3 = p[i+1].position
// After ComputeLength() every point also carries its normalised arc length
// position along the spline (0 at the first point, 1 at the last), and the
// curve is evaluated by that parameter.
//
// Closed splines get one extra point at allocation time (the caller copies
// the first point there) and wrap the evaluation parameter into [0, 1).
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape {

// A single cubic Bezier segment (only x/y/z of each control point are used).
struct BezierSegment {
    Vec3 p[4];
};

// Point on the curve at parameter t in [0, 1].
void BezierEvaluate(float t, Vec3& out, const BezierSegment& curve);
// Unit length tangent at t. t is clamped to [0.0001, 0.9999] so the
// derivative never degenerates at a collapsed end handle.
void BezierTangent(float t, Vec3& out, const BezierSegment& curve);
// de Casteljau subdivision at t = 0.5.
void BezierSplit(const BezierSegment& curve, BezierSegment& first, BezierSegment& second);
// Arc length estimate: the control polygon length, refined by recursive
// halving while (polygon - chord)^2 exceeds the tolerance (halved per level,
// never below 1e-5).
float BezierLength(float tolerance, const BezierSegment& curve);
// Approximate closest point: the curve is sampled as `steps` line pieces and
// the point is projected onto each. Returns the curve parameter, the distance
// goes to `outDistance`.
float BezierClosestParam(const BezierSegment& curve, const Vec3& point, float& outDistance, uint32_t steps);

struct alignas(16) SplinePoint {
    Vec3 position;
    Vec3 inTangent;      // handle towards the previous point, relative to position
    Vec3 outTangent;     // handle towards the next point, relative to position
    float param;         // normalised arc length position, set by ComputeLength()
    float field_34[3];   // NOTE: unused by the spline code, filled in by its owners
};
static_assert(sizeof(SplinePoint) == 64, "spline points are 64 bytes on the console");

class ApeSpline {
public:
    // @func 0x0055F244 ApeSpline::ApeSpline
    ApeSpline() = default;
    ~ApeSpline();

    ApeSpline(const ApeSpline&) = delete;
    ApeSpline& operator=(const ApeSpline&) = delete;

    // Releases the old points and allocates `numPoints` zeroed ones (one more
    // when closed). The point count is set to the allocated number.
    void Init(uint32_t numPoints, bool closed, bool flag);

    // Measures every segment, stores the normalised parameter of each point
    // and the total length.
    void ComputeLength();

    // Box and sphere enclosing the control point positions. With a single
    // point the box is grown to include it (the caller initialises it) and
    // the sphere is that point with radius 0; with no point nothing changes.
    // Otherwise the box is replaced and the sphere is centred on the average
    // position.
    void GetBounds(Aabb& box, Sphere& sphere) const;

    // Evaluation by normalised parameter. The search for the segment starts
    // at `startSegment` and walks forwards (or backwards when `reverse`),
    // wrapping around. The segment found is remembered as the next search
    // start and returned (-1 if t lies outside the spline).
    int EvaluatePosition(float t, Vec3& outPos, uint32_t startSegment, bool reverse);
    int EvaluateTangent(float t, Vec3& outTangent, uint32_t startSegment, bool reverse);
    int Evaluate(float t, Vec3& outPos, Vec3& outTangent, uint32_t startSegment, bool reverse);

    // Same, starting at the segment found by the previous evaluation.
    int EvaluatePosition(float t, Vec3& outPos, bool reverse = false);
    int EvaluateTangent(float t, Vec3& outTangent, bool reverse = false);
    int Evaluate(float t, Vec3& outPos, Vec3& outTangent, bool reverse = false);

    // Parameter of the point on the spline closest to `point`, searching the
    // segments [firstSegment, endSegment). Optionally returns the segment and
    // the distance.
    float FindClosestParam(const Vec3& point, uint32_t firstSegment, uint32_t endSegment,
                           uint32_t* outSegment, float* outDistance) const;
    // Same over the whole spline.
    float FindClosestParam(const Vec3& point, uint32_t* outSegment, float* outDistance) const;

    uint32_t GetNumPoints() const { return m_numPoints; }
    uint32_t GetCapacity() const { return m_capacity; }
    bool IsClosed() const { return m_closed; }
    bool GetFlag() const { return m_flag; }
    float GetLength() const { return m_length; }
    SplinePoint* GetPoints() { return m_points; }
    const SplinePoint* GetPoints() const { return m_points; }
    SplinePoint& GetPoint(uint32_t i) { return m_points[i]; }
    const SplinePoint& GetPoint(uint32_t i) const { return m_points[i]; }

    // Bezier control points of segment i (points i and i + 1).
    void GetSegment(uint32_t i, BezierSegment& out) const;

private:
    // Finds the segment containing t; returns -1 when there is none.
    int FindSegment(float& t, uint32_t startSegment, bool reverse, float& outLocal);

    // The console packs these four into one word: count (15 bits),
    // capacity (15 bits), flag, closed.
    uint32_t m_numPoints = 0;
    uint32_t m_capacity = 0;
    bool m_flag = false;        // NOTE: stored by Init, never read by the spline code
    bool m_closed = false;
    SplinePoint* m_points = nullptr;
    float m_length = 0.0f;
    uint32_t m_lastSegment = 0; // start of the next segment search
};

// Returned by evaluations of splines with fewer than two points.
// NOTE: the console reads these from globals at 0x01164B40 (position) and
// 0x01164B60 (tangent). The first is the zero vector; the second is set up by
// a static initialiser that has not been found, the unit Y axis is a guess.
extern const Vec3 kSplineDefaultPosition;
extern const Vec3 kSplineDefaultTangent;

} // namespace ape
