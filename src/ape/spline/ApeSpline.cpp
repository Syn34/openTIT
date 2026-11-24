// openTIT - Ape engine cubic Bezier splines.
#include "ape/spline/ApeSpline.h"

#include "ape/memory/ApeMem.h"

#include <cfloat>
#include <cmath>
#include <cstring>

namespace ape {

const Vec3 kSplineDefaultPosition = { 0.0f, 0.0f, 0.0f, 0.0f };
const Vec3 kSplineDefaultTangent = { 0.0f, 1.0f, 0.0f, 0.0f };

namespace {

Vec3 Make(float x, float y, float z) { return Vec3{ x, y, z, 0.0f }; }
Vec3 Add(const Vec3& a, const Vec3& b) { return Make(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 Sub(const Vec3& a, const Vec3& b) { return Make(a.x - b.x, a.y - b.y, a.z - b.z); }
Vec3 Scale(const Vec3& a, float s) { return Make(a.x * s, a.y * s, a.z * s); }
Vec3 Mid(const Vec3& a, const Vec3& b) { return Scale(Add(a, b), 0.5f); }
float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float Length(const Vec3& a) { return std::sqrt(Dot(a, a)); }
float Distance(const Vec3& a, const Vec3& b) { return Length(Sub(a, b)); }

void SetXYZ(Vec3& out, const Vec3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
}

// Weighted sum of the four control points.
Vec3 Combine(const BezierSegment& c, float w0, float w1, float w2, float w3)
{
    return Make(c.p[0].x * w0 + c.p[1].x * w1 + c.p[2].x * w2 + c.p[3].x * w3,
                c.p[0].y * w0 + c.p[1].y * w1 + c.p[2].y * w2 + c.p[3].y * w3,
                c.p[0].z * w0 + c.p[1].z * w1 + c.p[2].z * w2 + c.p[3].z * w3);
}

Vec3 PointAt(const BezierSegment& c, float t)
{
    const float s = 1.0f - t;
    return Combine(c, s * s * s, 3.0f * t * s * s, 3.0f * t * t * s, t * t * t);
}

constexpr float kArcLengthTolerance = 1e-5f;
constexpr uint32_t kClosestPointSteps = 8;

} // namespace

// -- Bezier segments ---------------------------------------------------------

// @func 0x0055EC98 BezierEvaluate
void BezierEvaluate(float t, Vec3& out, const BezierSegment& curve)
{
    SetXYZ(out, PointAt(curve, t));
}

// @func 0x005602F8 BezierTangent
void BezierTangent(float t, Vec3& out, const BezierSegment& curve)
{
    t = std::fmin(std::fmax(t, 0.0001f), 0.9999f);
    const float s = 1.0f - t;
    // Derivative of the Bernstein basis.
    const Vec3 d = Combine(curve, -3.0f * s * s, 3.0f * s * s - 6.0f * t * s,
                           6.0f * t * s - 3.0f * t * t, 3.0f * t * t);
    SetXYZ(out, Scale(d, 1.0f / Length(d)));
}

// @func 0x0055F8D0 BezierSplit
void BezierSplit(const BezierSegment& c, BezierSegment& first, BezierSegment& second)
{
    const Vec3 p01 = Mid(c.p[0], c.p[1]);
    const Vec3 p12 = Mid(c.p[1], c.p[2]);
    const Vec3 p23 = Mid(c.p[2], c.p[3]);
    const Vec3 p012 = Mid(p01, p12);
    const Vec3 p123 = Mid(p12, p23);
    const Vec3 middle = Mid(p012, p123);

    SetXYZ(first.p[0], c.p[0]);
    SetXYZ(first.p[1], p01);
    SetXYZ(first.p[2], p012);
    SetXYZ(first.p[3], middle);
    SetXYZ(second.p[0], middle);
    SetXYZ(second.p[1], p123);
    SetXYZ(second.p[2], p23);
    SetXYZ(second.p[3], c.p[3]);
}

// @func 0x0055FA30 BezierLength
float BezierLength(float tolerance, const BezierSegment& curve)
{
    float polygon = 0.0f;
    for (int i = 0; i < 3; ++i)
        polygon += Distance(curve.p[i + 1], curve.p[i]);
    const float error = polygon - Distance(curve.p[3], curve.p[0]);
    if (error * error <= tolerance)
        return polygon;

    BezierSegment first, second;
    BezierSplit(curve, first, second);
    const float halfTolerance = std::fmax(tolerance * 0.5f, kArcLengthTolerance);
    return BezierLength(halfTolerance, first) + BezierLength(halfTolerance, second);
}

// @func 0x0055ED60 BezierClosestParam
float BezierClosestParam(const BezierSegment& curve, const Vec3& point, float& outDistance, uint32_t steps)
{
    const float step = 1.0f / (float)steps;
    float bestDistance = FLT_MAX;
    float bestParam = 0.0f;
    float t = 0.0f;
    Vec3 start = curve.p[0];

    for (uint32_t i = 0; i < steps; ++i) {
        const Vec3 end = (i < steps - 1) ? PointAt(curve, t + step) : curve.p[3];

        // Direction of this piece (left unnormalised when it is degenerate).
        Vec3 dir = Sub(end, start);
        const float pieceLength = Length(dir);
        if (pieceLength >= 1e-5f)
            dir = Scale(dir, 1.0f / pieceLength);

        float param;
        Vec3 offset = Sub(point, end);
        if (Dot(dir, offset) < 0.0f) {
            // The point projects before the end of the piece.
            offset = Sub(point, start);
            const float along = Dot(dir, offset);
            param = t;
            if (along > 0.0f) {
                offset = Sub(point, Add(start, Scale(dir, along)));
                param = t + step * along / pieceLength;
            }
        } else {
            param = t + step;
        }

        const float distance = Length(offset);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestParam = param;
        }
        t += step;
        start = end;
    }

    outDistance = bestDistance;
    return bestParam;
}

// -- ApeSpline ----------------------------------------------------------------

// @func 0x0055F27C ApeSpline::~ApeSpline
ApeSpline::~ApeSpline()
{
    SysFree(m_points, 0, 0);
}

// @func 0x0055F2BC ApeSpline::Init
void ApeSpline::Init(uint32_t numPoints, bool closed, bool flag)
{
    m_capacity = numPoints & 0x7FFF;
    m_closed = closed;
    m_flag = flag;
    SysFree(m_points, 0, 0);
    m_points = nullptr;

    if (m_capacity != 0) {
        if (m_closed)
            m_capacity = (m_capacity + 1) & 0x7FFF;   // room for the closing copy of point 0
        const size_t bytes = m_capacity * sizeof(SplinePoint);
        m_points = static_cast<SplinePoint*>(SysAlloc(16, bytes, 0, 0));
        std::memset(static_cast<void*>(m_points), 0, bytes);
    }
    m_numPoints = m_capacity;
}

void ApeSpline::GetSegment(uint32_t i, BezierSegment& out) const
{
    const SplinePoint& a = m_points[i];
    const SplinePoint& b = m_points[i + 1];
    out.p[0] = a.position;
    out.p[1] = Add(a.position, a.outTangent);
    out.p[2] = Add(b.position, b.inTangent);
    out.p[3] = b.position;
}

// @func 0x0055FB88 ApeSpline::ComputeLength
void ApeSpline::ComputeLength()
{
    float total = 0.0f;
    if (m_numPoints > 1) {
        // Cumulative length at every point...
        uint32_t i = 0;
        for (; i < m_numPoints - 1; ++i) {
            m_points[i].param = total;
            BezierSegment segment;
            GetSegment(i, segment);
            total += BezierLength(kArcLengthTolerance, segment);
        }
        m_points[i].param = total;

        // ...normalised to [0, 1]. A zero length spline is spread evenly.
        for (i = 0; i < m_numPoints; ++i) {
            if (total == 0.0f)
                m_points[i].param = (float)i / (float)(int)(m_numPoints - 1);
            else
                m_points[i].param = m_points[i].param / total;
        }
        m_points[m_numPoints - 1].param = 1.0f;
    }
    m_length = total;
    m_lastSegment = 0;
}

// @func 0x0055FE00 ApeSpline::GetBounds
void ApeSpline::GetBounds(Aabb& box, Sphere& sphere) const
{
    if (m_numPoints < 2) {
        if (m_numPoints == 1) {
            const Vec3& p = m_points[0].position;
            AabbAddPoint(box, p);
            sphere = Sphere{ p.x, p.y, p.z, 0.0f };
        }
        return;
    }

    Vec3 minimum = Make(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maximum = Make(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    Vec3 sum = kSplineDefaultPosition;
    for (uint32_t i = 0; i < m_numPoints; ++i) {
        const Vec3& p = m_points[i].position;
        sum = Add(sum, p);
        minimum = Make(std::fmin(minimum.x, p.x), std::fmin(minimum.y, p.y), std::fmin(minimum.z, p.z));
        maximum = Make(std::fmax(maximum.x, p.x), std::fmax(maximum.y, p.y), std::fmax(maximum.z, p.z));
    }
    SetXYZ(box.min, minimum);
    SetXYZ(box.max, maximum);

    const float count = (float)m_numPoints;
    const Vec3 center = Make(sum.x / count, sum.y / count, sum.z / count);
    float maxDistanceSq = 0.0f;
    for (uint32_t i = 0; i < m_numPoints; ++i) {
        const Vec3 d = Sub(m_points[i].position, center);
        maxDistanceSq = std::fmax(maxDistanceSq, Dot(d, d));
    }
    sphere = Sphere{ center.x, center.y, center.z, std::sqrt(maxDistanceSq) };
}

int ApeSpline::FindSegment(float& t, uint32_t startSegment, bool reverse, float& outLocal)
{
    if (m_closed)
        t = t - std::trunc(t);

    const uint32_t numSegments = m_numPoints - 1;
    // NOTE: the console trusts the start segment; a stale one (after the
    // point count shrank) would read past the array, so it is clamped here.
    uint32_t segment = (startSegment < numSegments) ? startSegment : 0;
    const uint32_t first = segment;
    do {
        const float t0 = m_points[segment].param;
        const float t1 = m_points[segment + 1].param;
        if (t0 <= t && t <= t1) {
            m_lastSegment = segment;
            outLocal = (t - t0) / (t1 - t0);
            return (int)segment;
        }
        if (reverse) {
            // NOTE: the console wraps to numSegments here, one past the last
            // segment; the last real segment is used instead.
            segment = (segment == 0) ? numSegments - 1 : segment - 1;
        } else {
            segment = (segment + 1 >= numSegments) ? 0 : segment + 1;
        }
    } while (segment != first);
    return -1;
}

// @func 0x00560058 ApeSpline::EvaluatePosition
int ApeSpline::EvaluatePosition(float t, Vec3& outPos, uint32_t startSegment, bool reverse)
{
    if (m_numPoints < 2) {
        SetXYZ(outPos, m_numPoints == 0 ? kSplineDefaultPosition : m_points[0].position);
        return 0;
    }
    float local = 0.0f;
    const int segment = FindSegment(t, startSegment, reverse, local);
    BezierSegment curve;
    if (segment < 0) {
        // NOTE: the console evaluates "segment -1" here (reading before the
        // array). The nearer end point of the spline is returned instead.
        SetXYZ(outPos, t < m_points[0].param ? m_points[0].position : m_points[m_numPoints - 1].position);
        return segment;
    }
    GetSegment((uint32_t)segment, curve);
    BezierEvaluate(local, outPos, curve);
    return segment;
}

// @func 0x00560430 ApeSpline::EvaluateTangent
int ApeSpline::EvaluateTangent(float t, Vec3& outTangent, uint32_t startSegment, bool reverse)
{
    if (m_numPoints < 2) {
        SetXYZ(outTangent, kSplineDefaultTangent);
        return 0;
    }
    float local = 0.0f;
    const int segment = FindSegment(t, startSegment, reverse, local);
    BezierSegment curve;
    // NOTE: see EvaluatePosition for the out-of-range case.
    GetSegment(segment < 0 ? (t < m_points[0].param ? 0 : m_numPoints - 2) : (uint32_t)segment, curve);
    BezierTangent(segment < 0 ? (t < m_points[0].param ? 0.0f : 1.0f) : local, outTangent, curve);
    return segment;
}

// @func 0x005606C8 ApeSpline::Evaluate
int ApeSpline::Evaluate(float t, Vec3& outPos, Vec3& outTangent, uint32_t startSegment, bool reverse)
{
    if (m_numPoints < 2) {
        SetXYZ(outPos, m_numPoints == 0 ? kSplineDefaultPosition : m_points[0].position);
        SetXYZ(outTangent, kSplineDefaultTangent);
        return 0;
    }
    float local = 0.0f;
    const int segment = FindSegment(t, startSegment, reverse, local);
    BezierSegment curve;
    if (segment < 0) {
        // NOTE: see EvaluatePosition for the out-of-range case.
        const bool before = t < m_points[0].param;
        GetSegment(before ? 0 : m_numPoints - 2, curve);
        local = before ? 0.0f : 1.0f;
    } else {
        GetSegment((uint32_t)segment, curve);
    }
    BezierEvaluate(local, outPos, curve);
    BezierTangent(local, outTangent, curve);
    return segment;
}

// @func 0x005602BC ApeSpline::EvaluatePosition
int ApeSpline::EvaluatePosition(float t, Vec3& outPos, bool reverse)
{
    return EvaluatePosition(t, outPos, m_lastSegment, reverse);
}

// @func 0x00560690 ApeSpline::EvaluateTangent
int ApeSpline::EvaluateTangent(float t, Vec3& outTangent, bool reverse)
{
    return EvaluateTangent(t, outTangent, m_lastSegment, reverse);
}

// @func 0x00560A10 ApeSpline::Evaluate
int ApeSpline::Evaluate(float t, Vec3& outPos, Vec3& outTangent, bool reverse)
{
    return Evaluate(t, outPos, outTangent, m_lastSegment, reverse);
}

// @func 0x0055F008 ApeSpline::FindClosestParam
float ApeSpline::FindClosestParam(const Vec3& point, uint32_t firstSegment, uint32_t endSegment,
                                  uint32_t* outSegment, float* outDistance) const
{
    uint32_t bestSegment = 0xFFFFFFFFu;
    float bestDistance = FLT_MAX;
    float bestParam = 0.0f;

    for (uint32_t i = firstSegment; i < endSegment; ++i) {
        BezierSegment curve;
        GetSegment(i, curve);
        float distance;
        const float local = BezierClosestParam(curve, point, distance, kClosestPointSteps);
        if (distance < bestDistance) {
            const float t0 = m_points[i].param;
            const float t1 = m_points[i + 1].param;
            bestParam = Clamp(local * (t1 - t0) + t0, 0.0f, 1.0f);
            bestDistance = distance;
            bestSegment = i;
        }
    }

    if (outSegment)
        *outSegment = bestSegment;
    if (outDistance)
        *outDistance = bestDistance;
    return bestParam;
}

// @func 0x00560A40 ApeSpline::FindClosestParam
float ApeSpline::FindClosestParam(const Vec3& point, uint32_t* outSegment, float* outDistance) const
{
    if (m_numPoints >= 2)
        return FindClosestParam(point, 0, m_numPoints - 1, outSegment, outDistance);

    if (outSegment)
        *outSegment = 0;
    // NOTE: the console reads point 0 even for an empty spline.
    if (outDistance)
        *outDistance = m_numPoints ? Distance(m_points[0].position, point) : 0.0f;
    return 0.0f;
}

} // namespace ape
