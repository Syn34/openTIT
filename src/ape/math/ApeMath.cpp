// openTIT - Ape engine scalar/vector math helpers.
#include "ape/math/ApeMath.h"

#include <cmath>

namespace ape {

RandomState g_gameRandom = { 0, 0 };

// @func 0x00734DCC Max
float Max(float a, float b)
{
    return (a - b < 0.0f) ? b : a;
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
// @func 0x00734D90 Vec3Copy (identical second copy)
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

// @func 0x00735960 Vec4FromVec3
void Vec4FromVec3(float w, Vec4& out, const Vec3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
    out.w = w;
}

// @func 0x00734CC8 Matrix34Copy
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
