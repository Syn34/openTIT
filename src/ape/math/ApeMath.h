// openTIT - Ape engine scalar/vector math helpers.
//
// These are the small inline helpers the PS3 compiler emitted out of line
// (vector add/sub/scale, normalise, clamps, fast trig, a cheap PRNG).
// Vectors are stored as 4 floats (x, y, z, pad) to match the 16-byte
// aligned layout used throughout the engine's data structures.
#pragma once

#include <cstdint>

namespace ape {

struct alignas(16) Vec3 {
    float x, y, z, pad;
};

struct alignas(16) Vec4 {
    float x, y, z, w;
};

// 3x4 affine transform stored as four 16-byte rows: three axes and a translation.
struct alignas(16) Matrix34 {
    Vec3 xAxis, yAxis, zAxis, translation;
};

// -- scalars ---------------------------------------------------------------
float Max(float a, float b);
float Clamp(float value, float minValue, float maxValue);

// Fast polynomial approximations used by gameplay code (accurate to ~1e-6).
float FastCos(float radians);
float FastSin(float radians);

// -- Vec3 ------------------------------------------------------------------
void  Vec3Copy(Vec3& out, const Vec3& v);
void  Vec3Add(Vec3& out, const Vec3& a, const Vec3& b);
void  Vec3Sub(Vec3& out, const Vec3& a, const Vec3& b);
void  Vec3Scale(Vec3& out, const Vec3& v, float s);
void  Vec3ScaleInPlace(float s, Vec3& v);
void  Vec3DivideInPlace(float s, Vec3& v);
float Vec3Dot(const Vec3& a, const Vec3& b);
float Vec3LengthSq(const Vec3& v);
float Vec3Length(const Vec3& v);
// Normalises in place. The caller guarantees a non-zero vector.
void  Vec3Normalize(Vec3& v);

// A sphere/plane style packed vector: xyz from `v`, w = s.
void  Vec4FromVec3(float w, Vec4& out, const Vec3& v);

// -- Matrix34 --------------------------------------------------------------
// Copies rotation and translation, leaving the padding lanes untouched.
void  Matrix34Copy(Matrix34& out, const Matrix34& m);
void  Matrix34FromAxes(Matrix34& out, const Vec3& xAxis, const Vec3& yAxis,
                       const Vec3& zAxis, const Vec3& translation);

// -- random ----------------------------------------------------------------
// Small rotate/xor generator. Each call returns the value produced by the
// previous step scaled to [0, 1] and advances the state.
struct RandomState {
    int64_t value;   // last 16-bit output
    uint32_t seed;
};
float RandomUnit(RandomState& state);
// Random 2D direction built from two samples in [minValue, maxValue].
float* RandomDirection2D(float minValue, float maxValue, RandomState& state, float out[2]);

extern RandomState g_gameRandom;  // 0x01164888

} // namespace ape
