// openTIT - Ape engine scalar/vector math helpers.
#include "ape/math/ApeMath.h"

#include "crt/libc/Libc.h"

#include <cmath>

namespace ape {

RandomState g_gameRandom = { 0, 0 };

// @func 0x00734DCC Max
float Max(float a, float b)
{
    return (a - b < 0.0f) ? b : a;
}

// @func 0x007558D4 Min
float Min(float a, float b)
{
    return (b - a < 0.0f) ? b : a;
}

// @func 0x0074C45C MaxOf
float MaxOf(const float& a, const float& b)
{
    return (a - b < 0.0f) ? b : a;
}

// @func 0x0074CFD4 MinOf
float MinOf(const float& a, const float& b)
{
    return (a - b < 0.0f) ? a : b;
}

// @func 0x0074C708 Saturate
float Saturate(float value)
{
    if (1.0f - value < 0.0f)
        value = 1.0f;
    if (value < 0.0f)
        value = 0.0f;
    return value;
}

// @func 0x007352E8 SafeACos
float SafeACos(float cosine)
{
    if (cosine + 1.0f < 0.0f)
        cosine = -1.0f;
    return crt::ACosF(cosine - 1.0f < 0.0f ? cosine : 1.0f);
}

// @func 0x00748D08 AngleFromCosine
float AngleFromCosine(float cosine)
{
    return SafeACos(Clamp(cosine, -1.0f, 1.0f));
}

// @func 0x00767A04 CompareFloats (copy 1)
// @func 0x0077D7C4 CompareFloats (copy 2)
int CompareFloats(const void* a, const void* b)
{
    const float fa = *static_cast<const float*>(a);
    const float fb = *static_cast<const float*>(b);
    if (fa > fb)
        return 1;
    return fa < fb ? -1 : 0;
}

// @func 0x00734E4C Clamp
// The lower bound wins if the bounds are inverted.
float Clamp(float value, float minValue, float maxValue)
{
    if (maxValue - value < 0.0f)
        value = maxValue;
    if (value - minValue < 0.0f)
        value = minValue;
    return value;
}

namespace {
// Both functions reduce the angle to an octant (units of pi/4) and evaluate
// a minimax polynomial on the fractional part. Odd octants are mirrored.
constexpr float kOctantsPerRadian = 1.2732395f;  // 4 / pi

float SinPoly(float t)  // sin(t * pi/4) for t in [0, 1]
{
    const float t2 = t * t;
    return (((-3.5950437e-05f * t2 + 0.0024900010f) * t2 - 0.080745436f) * t2 + 0.78539819f) * t;
}

float CosPoly(float t)  // cos(t * pi/4) for t in [0, 1]
{
    const float t2 = t * t;
    return ((-0.00031872783f * t2 + 0.015849683f) * t2 - 0.30842417f) * t2 + 0.99999994f;
}
}

// @func 0x007344E0 FastCos
float FastCos(float radians)
{
    const float a = std::fabs(radians) * kOctantsPerRadian;
    const uint32_t octant = (uint32_t)(int32_t)a;
    float t = a - (float)(int64_t)a;
    if (octant & 1)
        t = 1.0f - t;
    float r = ((octant + 1) & 2) ? SinPoly(t) : CosPoly(t);
    if (((octant + 2) & 7) > 3)
        r = -r;
    return r;
}

// @func 0x007345F0 FastSin
float FastSin(float radians)
{
    if (radians < 0.0f)
        return -FastSin(-radians);
    const float a = radians * kOctantsPerRadian;
    const uint32_t octant = (uint32_t)(int32_t)a;
    float t = a - (float)(int64_t)a;
    if (octant & 1)
        t = 1.0f - t;
    float r = ((octant + 1) & 2) ? CosPoly(t) : SinPoly(t);
    if ((octant & 7) > 3)
        r = -r;
    return r;
}

// @func 0x00734BA0 Vec3Copy
// @func 0x00734D90 Vec3Copy (copy 2)
// @func 0x0074D080 Vec3Copy (copy 3)
// @func 0x0074DBEC Vec3Copy (copy 4)
void Vec3Copy(Vec3& out, const Vec3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
}

// @func 0x00734E94 Vec3Add
void Vec3Add(Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = a.x + b.x;
    out.y = a.y + b.y;
    out.z = a.z + b.z;
}

// @func 0x00734A8C Vec3Sub
void Vec3Sub(Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = a.x - b.x;
    out.y = a.y - b.y;
    out.z = a.z - b.z;
}

// @func 0x00734E6C Vec3Scale
void Vec3Scale(Vec3& out, const Vec3& v, float s)
{
    out.x = v.x * s;
    out.y = v.y * s;
    out.z = v.z * s;
}

// @func 0x007359E4 Vec3ScaleInPlace
// @func 0x00755E08 Vec3ScaleInPlace (copy 2)
void Vec3ScaleInPlace(float s, Vec3& v)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
}

// @func 0x00734DF8 Vec3DivideInPlace
void Vec3DivideInPlace(float s, Vec3& v)
{
    v.x /= s;
    v.y /= s;
    v.z /= s;
}

// @func 0x00734B10 Vec3Dot
float Vec3Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// @func 0x00734BBC Vec3LengthSq
float Vec3LengthSq(const Vec3& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// @func 0x00734DAC Vec3Length
float Vec3Length(const Vec3& v)
{
    return std::sqrt(Vec3LengthSq(v));
}

// @func 0x00734AC8 Vec3Normalize
void Vec3Normalize(Vec3& v)
{
    const float inv = 1.0f / Vec3Length(v);
    v.x *= inv;
    v.y *= inv;
    v.z *= inv;
}

// @func 0x0074C600 Vec3AddInPlace
void Vec3AddInPlace(Vec3& a, const Vec3& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

// @func 0x00742C74 Vec3SubInPlace
void Vec3SubInPlace(Vec3& a, const Vec3& b)
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
}

// @func 0x00735CDC Vec3Mul
void Vec3Mul(Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = a.x * b.x;
    out.y = a.y * b.y;
    out.z = a.z * b.z;
}

// @func 0x0074B5C0 Vec3Divide
void Vec3Divide(float s, Vec3& out, const Vec3& v)
{
    const float inv = 1.0f / s;
    out.x = v.x * inv;
    out.y = v.y * inv;
    out.z = v.z * inv;
}

// @func 0x0074B6B0 Vec3Negate
void Vec3Negate(Vec3& out, const Vec3& v)
{
    out.x = -v.x;
    out.y = -v.y;
    out.z = -v.z;
}

// @func 0x00748AFC Vec3Cross
// @func 0x00755DAC Vec3Cross (copy 2)
void Vec3Cross(Vec3& out, const Vec3& a, const Vec3& b)
{
    const float x = a.y * b.z - a.z * b.y;
    const float y = a.z * b.x - a.x * b.z;
    const float z = a.x * b.y - a.y * b.x;
    out.x = x;
    out.y = y;
    out.z = z;
}

// @func 0x00735BA0 Vec3Lerp
void Vec3Lerp(float t, Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = a.z + (b.z - a.z) * t;
}

// @func 0x00748A98 Vec3NearlyEqual
bool Vec3NearlyEqual(float epsilon, const Vec3& a, const Vec3& b)
{
    return std::fabs(a.x - b.x) < epsilon && std::fabs(a.y - b.y) < epsilon &&
           std::fabs(a.z - b.z) < epsilon;
}

// @func 0x00755D88 Vec2Add
void Vec2Add(float out[2], const float a[2], const float b[2])
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
}

// @func 0x00742CA8 Vec2AddInPlace
void Vec2AddInPlace(float a[2], const float b[2])
{
    a[0] += b[0];
    a[1] += b[1];
}

// @func 0x0074C7F8 Vec2Scale
void Vec2Scale(float s, float out[2], const float v[2])
{
    out[0] = v[0] * s;
    out[1] = v[1] * s;
}

// @func 0x00755DEC Vec2ScaleInPlace
void Vec2ScaleInPlace(float s, float v[2])
{
    v[0] *= s;
    v[1] *= s;
}

// @func 0x0077AECC Vec2LengthSq
float Vec2LengthSq(const float v[2])
{
    return v[0] * v[0] + v[1] * v[1];
}

// @func 0x00735960 Vec4FromVec3
void Vec4FromVec3(float w, Vec4& out, const Vec3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
    out.w = w;
}

// @func 0x00734CC8 Matrix34Copy
// @func 0x00748BB8 Matrix34Copy (copy 2)
// @func 0x0076FFFC Matrix34Copy (copy 3)
void Matrix34Copy(Matrix34& out, const Matrix34& m)
{
    Vec3Copy(out.xAxis, m.xAxis);
    Vec3Copy(out.yAxis, m.yAxis);
    Vec3Copy(out.zAxis, m.zAxis);
    Vec3Copy(out.translation, m.translation);
}

// @func 0x00735980 Matrix34FromAxes
void Matrix34FromAxes(Matrix34& out, const Vec3& xAxis, const Vec3& yAxis,
                      const Vec3& zAxis, const Vec3& translation)
{
    Vec3Copy(out.xAxis, xAxis);
    Vec3Copy(out.yAxis, yAxis);
    Vec3Copy(out.zAxis, zAxis);
    Vec3Copy(out.translation, translation);
}

namespace {
constexpr float kInv65535 = 1.0f / 65535.0f;

uint32_t RotateLeft(uint32_t v, int bits)
{
    return (v << bits) | (v >> (32 - bits));
}

// One generator step: mixes two rotations of the seed with a constant.
uint32_t RandomStep(uint32_t seed)
{
    return (RotateLeft(seed, 4) + RotateLeft(seed, 16)) ^ 0x87654321u;
}
}

// @func 0x00734B40 RandomUnit
float RandomUnit(RandomState& state)
{
    const int64_t previous = state.value;
    const uint32_t mixed = RandomStep(state.seed);
    state.value = mixed & 0xFFFF;
    state.seed = RotateLeft(mixed, 16);
    return (float)previous * kInv65535;
}

// @func 0x007342D8 RandomRange
float RandomRange(float minValue, float maxValue, RandomState& state)
{
    return (maxValue - minValue) * RandomUnit(state) + minValue;
}

// @func 0x00735470 RandomDirection2D
float* RandomDirection2D(float minValue, float maxValue, RandomState& state, float out[2])
{
    // Two generator steps are taken; the first sample is the stored value,
    // the second is the intermediate value of this call.
    const int64_t first = state.value;
    const uint32_t step1 = RandomStep(state.seed);
    const uint32_t step2 = (RotateLeft(step1, 20) + step1) ^ 0x87654321u;
    state.value = step2 & 0xFFFF;
    state.seed = RotateLeft(step2, 16);

    const float range = maxValue - minValue;
    const float x = range * ((float)first * kInv65535) + minValue;
    const float y = range * ((float)(step1 & 0xFFFF) * kInv65535) + minValue;
    const float inv = 1.0f / std::sqrt(x * x + y * y);
    out[0] = x * inv;
    out[1] = y * inv;
    return out;
}

} // namespace ape
