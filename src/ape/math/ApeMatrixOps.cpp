// openTIT - Ape engine matrix and quaternion routines (0x005532E4-0x00555718).
#include "ape/math/ApeMatrixOps.h"

#include <cmath>
#include <cstdint>

namespace ape {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.28318531f;
constexpr float kHalfPi = 1.57079633f;

float Dot3(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Length3(const Vec3& v)
{
    return std::sqrt(Dot3(v, v));
}

void SetXYZ(Vec3& v, float x, float y, float z)
{
    v.x = x;
    v.y = y;
    v.z = z;
}

// Parabolic sine approximation for x in [-pi, pi] with one refinement step
// (max error about 0.001).
float ParabolicSin(float x)
{
    const float kLinear = 1.27323954f;      // 4 / pi
    const float kQuadratic = -0.40528473f;  // -4 / pi^2
    const float kRefine = 0.225f;
    const float y = x * kLinear + x * kQuadratic * std::fabs(x);
    return kRefine * (y * std::fabs(y) - y) + y;
}

// Wraps the angle into [-pi, pi] and evaluates sine and cosine.
void FastSinCos(float radians, float& s, float& c)
{
    const float offset = radians < 0.0f ? -kPi : kPi;
    float x = radians + offset;
    x = x - (float)(int64_t)(x / kTwoPi) * kTwoPi - offset;
    float xc = x + kHalfPi;
    if (xc > kPi)
        xc -= kTwoPi;
    s = ParabolicSin(x);
    c = ParabolicSin(xc);
}

void SetIdentityAxes(Matrix34& out)
{
    SetXYZ(out.xAxis, 1.0f, 0.0f, 0.0f);
    SetXYZ(out.yAxis, 0.0f, 1.0f, 0.0f);
    SetXYZ(out.zAxis, 0.0f, 0.0f, 1.0f);
}

const Vec3& Axis(const Matrix34& m, int i)
{
    return i == 0 ? m.xAxis : (i == 1 ? m.yAxis : m.zAxis);
}

Vec3& Axis(Matrix34& m, int i)
{
    return i == 0 ? m.xAxis : (i == 1 ? m.yAxis : m.zAxis);
}

const float* Row(const Matrix44& m, int i)
{
    return &m.rows[i].x;
}

bool Close(float a, float b, float tolerance)
{
    return std::fabs(a - b) < tolerance;
}

bool CloseXYZ(const Vec3& a, const Vec3& b, float tolerance)
{
    return Close(a.x, b.x, tolerance) && Close(a.y, b.y, tolerance) && Close(a.z, b.z, tolerance);
}

} // namespace

// -- Matrix34 ------------------------------------------------------------------

// @func 0x005532E4 Matrix34InvTransformPoint
void Matrix34InvTransformPoint(Vec3& out, const Vec3& p, const Matrix34& m)
{
    Vec3 d;
    SetXYZ(d, p.x - m.translation.x, p.y - m.translation.y, p.z - m.translation.z);
    SetXYZ(out, Dot3(d, m.xAxis), Dot3(d, m.yAxis), Dot3(d, m.zAxis));
}

// @func 0x00553368 Matrix34InvTransformPointInPlace
void Matrix34InvTransformPointInPlace(Vec3& p, const Matrix34& m)
{
    Matrix34InvTransformPoint(p, Vec3(p), m);
}

// @func 0x005533EC Matrix34InvRotateVector
void Matrix34InvRotateVector(Vec3& out, const Vec3& v, const Matrix34& m)
{
    SetXYZ(out, Dot3(v, m.xAxis), Dot3(v, m.yAxis), Dot3(v, m.zAxis));
}

// @func 0x00553468 Matrix34InvRotateVectorInPlace
void Matrix34InvRotateVectorInPlace(Vec3& v, const Matrix34& m)
{
    Matrix34InvRotateVector(v, Vec3(v), m);
}

// @func 0x00553A90 Matrix34FromQuat
void Matrix34FromQuat(Matrix34& out, const Quat& q, const Vec3& translation)
{
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    SetXYZ(out.xAxis, 1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + w * z), 2.0f * (x * z - w * y));
    SetXYZ(out.yAxis, 2.0f * (x * y - w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + w * x));
    SetXYZ(out.zAxis, 2.0f * (x * z + w * y), 2.0f * (y * z - w * x), 1.0f - 2.0f * (x * x + y * y));
    SetXYZ(out.translation, translation.x, translation.y, translation.z);
}

// @func 0x00553B4C Matrix34FromRotationTranslation
void Matrix34FromRotationTranslation(Matrix34& out, const Matrix34& rotation, const Vec3& translation)
{
    for (int i = 0; i < 3; ++i) {
        const Vec3& axis = Axis(rotation, i);
        SetXYZ(Axis(out, i), axis.x, axis.y, axis.z);
    }
    SetXYZ(out.translation, translation.x, translation.y, translation.z);
}

// @func 0x00553BC4 Matrix34FromScale
void Matrix34FromScale(Matrix34& out, const Vec3& scale)
{
    SetIdentityAxes(out);
    SetXYZ(out.translation, 0.0f, 0.0f, 0.0f);
    out.xAxis.x = scale.x;
    out.yAxis.y = scale.y;
    out.zAxis.z = scale.z;
}

// @func 0x00553C70 Matrix34FromTranslation
void Matrix34FromTranslation(Matrix34& out, const Vec3& translation)
{
    SetIdentityAxes(out);
    SetXYZ(out.translation, translation.x, translation.y, translation.z);
}

// @func 0x00553D18 Matrix34RotationX
void Matrix34RotationX(float radians, Matrix34& out)
{
    float s, c;
    FastSinCos(radians, s, c);
    SetXYZ(out.xAxis, 1.0f, 0.0f, 0.0f);
    SetXYZ(out.yAxis, 0.0f, c, s);
    SetXYZ(out.zAxis, 0.0f, -s, c);
    SetXYZ(out.translation, 0.0f, 0.0f, 0.0f);
}

// @func 0x00553E18 Matrix34RotationY
void Matrix34RotationY(float radians, Matrix34& out)
{
    float s, c;
    FastSinCos(radians, s, c);
    SetXYZ(out.xAxis, c, 0.0f, -s);
    SetXYZ(out.yAxis, 0.0f, 1.0f, 0.0f);
    SetXYZ(out.zAxis, s, 0.0f, c);
    SetXYZ(out.translation, 0.0f, 0.0f, 0.0f);
}

// @func 0x00553F18 Matrix34RotationZ
void Matrix34RotationZ(float radians, Matrix34& out)
{
    float s, c;
    FastSinCos(radians, s, c);
    SetXYZ(out.xAxis, c, s, 0.0f);
    SetXYZ(out.yAxis, -s, c, 0.0f);
    SetXYZ(out.zAxis, 0.0f, 0.0f, 1.0f);
    SetXYZ(out.translation, 0.0f, 0.0f, 0.0f);
}

namespace {
void MulAxes(Matrix34& out, const Matrix34& a, const Matrix34& b)
{
    for (int i = 0; i < 3; ++i) {
        const Vec3 row = Axis(a, i);   // copy: out may alias a
        SetXYZ(Axis(out, i),
               row.x * b.xAxis.x + row.y * b.yAxis.x + row.z * b.zAxis.x,
               row.x * b.xAxis.y + row.y * b.yAxis.y + row.z * b.zAxis.y,
               row.x * b.xAxis.z + row.y * b.yAxis.z + row.z * b.zAxis.z);
    }
}
} // namespace

// @func 0x00553FF0 Matrix34MulRotation
void Matrix34MulRotation(Matrix34& out, const Matrix34& a, const Matrix34& b)
{
    MulAxes(out, a, b);
    SetXYZ(out.translation, 0.0f, 0.0f, 0.0f);
}

// @func 0x00555370 Matrix34MulRotationKeepTranslation
void Matrix34MulRotationKeepTranslation(Matrix34& out, const Matrix34& a, const Matrix34& b)
{
    MulAxes(out, a, b);
}

// @func 0x005540F8 Matrix34ScaleColumns
void Matrix34ScaleColumns(Matrix34& m, const Vec3& s)
{
    Vec3* rows[4] = { &m.xAxis, &m.yAxis, &m.zAxis, &m.translation };
    for (Vec3* row : rows) {
        row->x *= s.x;
        row->y *= s.y;
        row->z *= s.z;
    }
}

// @func 0x00554394 Matrix34ScaleAxes
void Matrix34ScaleAxes(Matrix34& m, const Vec3& s)
{
    const float factors[3] = { s.x, s.y, s.z };
    for (int i = 0; i < 3; ++i) {
        Vec3& axis = Axis(m, i);
        axis.x *= factors[i];
        axis.y *= factors[i];
        axis.z *= factors[i];
    }
}

// @func 0x00554414 Matrix34ScaleRotation
void Matrix34ScaleRotation(float s, Matrix34& m)
{
    for (int i = 0; i < 3; ++i) {
        Vec3& axis = Axis(m, i);
        axis.x *= s;
        axis.y *= s;
        axis.z *= s;
    }
}

// @func 0x00554484 Matrix34GetScale
Vec3& Matrix34GetScale(const Matrix34& m, Vec3& out)
{
    SetXYZ(out, Length3(m.xAxis), Length3(m.yAxis), Length3(m.zAxis));
    return out;
}

// @func 0x005544FC Matrix34HasScale
bool Matrix34HasScale(float tolerance, const Matrix34& m)
{
    for (int i = 0; i < 3; ++i)
        if (std::fabs(Length3(Axis(m, i)) - 1.0f) >= tolerance)
            return true;
    return false;
}

// @func 0x005545B4 Matrix34Equal
bool Matrix34Equal(float tolerance, const Matrix34& a, const Matrix34& b)
{
    return Matrix34RotationEqual(tolerance, a, b) && CloseXYZ(a.translation, b.translation, tolerance);
}

// @func 0x00554730 Matrix34RotationEqual
bool Matrix34RotationEqual(float tolerance, const Matrix34& a, const Matrix34& b)
{
    return CloseXYZ(a.xAxis, b.xAxis, tolerance) && CloseXYZ(a.yAxis, b.yAxis, tolerance) &&
           CloseXYZ(a.zAxis, b.zAxis, tolerance);
}

// @func 0x00554858 Matrix34Decompose
void Matrix34Decompose(const Matrix34& m, Matrix34& rotation, Vec3& scale)
{
    Matrix34GetScale(m, scale);
    const float lengths[3] = { scale.x, scale.y, scale.z };
    for (int i = 0; i < 3; ++i) {
        const Vec3& axis = Axis(m, i);
        const float inv = 1.0f / lengths[i];
        SetXYZ(Axis(rotation, i), axis.x * inv, axis.y * inv, axis.z * inv);
    }
    SetXYZ(rotation.translation, m.translation.x, m.translation.y, m.translation.z);
}

// -- Matrix44 ------------------------------------------------------------------

// @func 0x00553910 Vec4MulMatrix44
void Vec4MulMatrix44(Vec4& out, const Vec4& v, const Matrix44& m)
{
    const float in[4] = { v.x, v.y, v.z, v.w };
    float result[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (int r = 0; r < 4; ++r) {
        const float* row = Row(m, r);
        for (int c = 0; c < 4; ++c)
            result[c] += in[r] * row[c];
    }
    out.x = result[0];
    out.y = result[1];
    out.z = result[2];
    out.w = result[3];
}

// @func 0x005539E4 Vec4MulMatrix44InPlace
void Vec4MulMatrix44InPlace(Vec4& v, const Matrix44& m)
{
    const Vec4 copy = v;
    Vec4MulMatrix44(v, copy, m);
}

// @func 0x00553A2C Matrix44TransformPoint
Vec4& Matrix44TransformPoint(Vec4& out, const Vec3& p, const Matrix44& m)
{
    const Vec4 point = { p.x, p.y, p.z, 1.0f };
    Vec4MulMatrix44(out, point, m);
    return out;
}

// @func 0x0055499C Matrix44Mul
Matrix44& Matrix44Mul(Matrix44& out, const Matrix44& a, const Matrix44& b)
{
    for (int r = 0; r < 4; ++r)
        Vec4MulMatrix44(out.rows[r], a.rows[r], b);
    return out;
}

// @func 0x00554A2C Matrix44Determinant
float Matrix44Determinant(const Matrix44& m)
{
    const float* a = Row(m, 0);
    const float* b = Row(m, 1);
    const float* c = Row(m, 2);
    const float* d = Row(m, 3);
    // Laplace expansion on 2x2 minors of the bottom two rows.
    const float s0 = c[0] * d[1] - c[1] * d[0];
    const float s1 = c[0] * d[2] - c[2] * d[0];
    const float s2 = c[0] * d[3] - c[3] * d[0];
    const float s3 = c[1] * d[2] - c[2] * d[1];
    const float s4 = c[1] * d[3] - c[3] * d[1];
    const float s5 = c[2] * d[3] - c[3] * d[2];
    const float t0 = a[0] * b[1] - a[1] * b[0];
    const float t1 = a[0] * b[2] - a[2] * b[0];
    const float t2 = a[0] * b[3] - a[3] * b[0];
    const float t3 = a[1] * b[2] - a[2] * b[1];
    const float t4 = a[1] * b[3] - a[3] * b[1];
    const float t5 = a[2] * b[3] - a[3] * b[2];
    return t0 * s5 - t1 * s4 + t2 * s3 + t3 * s2 - t4 * s1 + t5 * s0;
}

// @func 0x007A430C Matrix44Copy
void Matrix44Copy(Matrix44& out, const Matrix44& m)
{
    for (int i = 0; i < 4; ++i)
        out.rows[i] = m.rows[i];
}

// @func 0x00554C28 Matrix44SetInverse
Matrix44 Matrix44SetInverse(Matrix44& self, const Matrix44& m)
{
    const float* a = Row(m, 0);
    const float* b = Row(m, 1);
    const float* c = Row(m, 2);
    const float* d = Row(m, 3);

    const float s0 = a[0] * b[1] - b[0] * a[1];
    const float s1 = a[0] * b[2] - b[0] * a[2];
    const float s2 = a[0] * b[3] - b[0] * a[3];
    const float s3 = a[1] * b[2] - b[1] * a[2];
    const float s4 = a[1] * b[3] - b[1] * a[3];
    const float s5 = a[2] * b[3] - b[2] * a[3];
    const float c5 = c[2] * d[3] - d[2] * c[3];
    const float c4 = c[1] * d[3] - d[1] * c[3];
    const float c3 = c[1] * d[2] - d[1] * c[2];
    const float c2 = c[0] * d[3] - d[0] * c[3];
    const float c1 = c[0] * d[2] - d[0] * c[2];
    const float c0 = c[0] * d[1] - d[0] * c[1];

    const float inv = 1.0f / Matrix44Determinant(m);

    Matrix44 r;
    r.rows[0] = { ( b[1] * c5 - b[2] * c4 + b[3] * c3) * inv,
                  (-a[1] * c5 + a[2] * c4 - a[3] * c3) * inv,
                  ( d[1] * s5 - d[2] * s4 + d[3] * s3) * inv,
                  (-c[1] * s5 + c[2] * s4 - c[3] * s3) * inv };
    r.rows[1] = { (-b[0] * c5 + b[2] * c2 - b[3] * c1) * inv,
                  ( a[0] * c5 - a[2] * c2 + a[3] * c1) * inv,
                  (-d[0] * s5 + d[2] * s2 - d[3] * s1) * inv,
                  ( c[0] * s5 - c[2] * s2 + c[3] * s1) * inv };
    r.rows[2] = { ( b[0] * c4 - b[1] * c2 + b[3] * c0) * inv,
                  (-a[0] * c4 + a[1] * c2 - a[3] * c0) * inv,
                  ( d[0] * s4 - d[1] * s2 + d[3] * s0) * inv,
                  (-c[0] * s4 + c[1] * s2 - c[3] * s0) * inv };
    r.rows[3] = { (-b[0] * c3 + b[1] * c1 - b[2] * c0) * inv,
                  ( a[0] * c3 - a[1] * c1 + a[2] * c0) * inv,
                  (-d[0] * s3 + d[1] * s1 - d[2] * s0) * inv,
                  ( c[0] * s3 - c[1] * s1 + c[2] * s0) * inv };
    Matrix44Copy(self, r);
    Matrix44 result;
    Matrix44Copy(result, r);
    return result;
}

// -- quaternions -----------------------------------------------------------------

// @func 0x00555478 QuatFromMatrix34
void QuatFromMatrix34(Quat& out, const Matrix34& m)
{
    const float m00 = m.xAxis.x, m01 = m.xAxis.y, m02 = m.xAxis.z;
    const float m10 = m.yAxis.x, m11 = m.yAxis.y, m12 = m.yAxis.z;
    const float m20 = m.zAxis.x, m21 = m.zAxis.y, m22 = m.zAxis.z;
    const float trace = m00 + m11 + m22;

    if (trace > 0.0f) {
        const float w = std::sqrt(trace + 1.0f) * 0.5f;
        const float s = 0.25f / w;
        out = { (m12 - m21) * s, (m20 - m02) * s, (m01 - m10) * s, w };
    } else if (m00 > m11 && m00 > m22) {
        const float x = std::sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
        const float s = 0.25f / x;
        out = { x, (m10 + m01) * s, (m20 + m02) * s, (m12 - m21) * s };
    } else if (m11 > m22) {
        const float y = std::sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
        const float s = 0.25f / y;
        out = { (m10 + m01) * s, y, (m21 + m12) * s, (m20 - m02) * s };
    } else {
        const float z = std::sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
        const float s = 0.25f / z;
        out = { (m20 + m02) * s, (m21 + m12) * s, z, (m01 - m10) * s };
    }
}

// @func 0x005556A4 QuatMul
void QuatMul(Quat& out, const Quat& a, const Quat& b)
{
    const Quat r = {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
        a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
    out = r;
}

// @func 0x00555718 QuatMulInPlace
void QuatMulInPlace(Quat& a, const Quat& b)
{
    QuatMul(a, a, b);
}

} // namespace ape
