// openTIT - Ape engine matrix and quaternion routines (0x005532E4-0x00555718).
//
// NOTE: these live in the memory module's address range of the executable
// but are plain math; see NOTES.md (they belong next to ape/math/ApeMath).
//
// Conventions (same as ApeMath): row vectors, Matrix34 = three axis rows plus a
// translation row, each row 16 bytes with an unused 4th lane that these
// routines never write. Quaternions are (x, y, z, w).
#pragma once

#include "ape/math/ApeMath.h"

namespace ape {

struct alignas(16) Matrix44 {
    Vec4 rows[4];
};

using Quat = Vec4;

// -- Matrix34 -------------------------------------------------------------------
// Transform into the matrix's local space, assuming orthonormal axes:
// out = ((p - translation) . xAxis, ... . yAxis, ... . zAxis).
void Matrix34InvTransformPoint(Vec3& out, const Vec3& p, const Matrix34& m);
void Matrix34InvTransformPointInPlace(Vec3& p, const Matrix34& m);
// Same without the translation.
void Matrix34InvRotateVector(Vec3& out, const Vec3& v, const Matrix34& m);
void Matrix34InvRotateVectorInPlace(Vec3& v, const Matrix34& m);

void Matrix34FromQuat(Matrix34& out, const Quat& q, const Vec3& translation);
void Matrix34FromRotationTranslation(Matrix34& out, const Matrix34& rotation, const Vec3& translation);
// Identity rotation scaled per axis, zero translation.
void Matrix34FromScale(Matrix34& out, const Vec3& scale);
// Identity rotation with the given translation.
void Matrix34FromTranslation(Matrix34& out, const Vec3& translation);

// Rotations about one axis built with the engine's fast parabolic sine.
void Matrix34RotationX(float radians, Matrix34& out);
void Matrix34RotationY(float radians, Matrix34& out);
void Matrix34RotationZ(float radians, Matrix34& out);

// Rotation parts only: out.axes = a.axes * b.axes. The first variant clears
// out's translation, the second leaves it untouched. 'out' must not alias b.
void Matrix34MulRotation(Matrix34& out, const Matrix34& a, const Matrix34& b);
void Matrix34MulRotationKeepTranslation(Matrix34& out, const Matrix34& a, const Matrix34& b);

// Multiplies every row (translation included) component-wise by s.
void Matrix34ScaleColumns(Matrix34& m, const Vec3& s);
// Scales axis i of the rotation part by s[i].
void Matrix34ScaleAxes(Matrix34& m, const Vec3& s);
// Scales the three rotation rows by s.
void Matrix34ScaleRotation(float s, Matrix34& m);
// Lengths of the three axes.
Vec3& Matrix34GetScale(const Matrix34& m, Vec3& out);
// True when any axis length differs from 1 by at least 'tolerance'.
bool Matrix34HasScale(float tolerance, const Matrix34& m);
// Element-wise comparison (strictly closer than 'tolerance').
bool Matrix34Equal(float tolerance, const Matrix34& a, const Matrix34& b);
bool Matrix34RotationEqual(float tolerance, const Matrix34& a, const Matrix34& b);
// Splits m into a matrix with unit axes (same translation) and the axis scales.
void Matrix34Decompose(const Matrix34& m, Matrix34& rotation, Vec3& scale);

// -- Matrix44 -------------------------------------------------------------------
void Vec4MulMatrix44(Vec4& out, const Vec4& v, const Matrix44& m);
void Vec4MulMatrix44InPlace(Vec4& v, const Matrix44& m);
// (p, 1) * m.
Vec4& Matrix44TransformPoint(Vec4& out, const Vec3& p, const Matrix44& m);
// out = a * b. 'out' must not alias 'b'.
Matrix44& Matrix44Mul(Matrix44& out, const Matrix44& a, const Matrix44& b);
float Matrix44Determinant(const Matrix44& m);
// Stores the inverse of m in 'self' and also returns it. m must be invertible.
Matrix44 Matrix44SetInverse(Matrix44& self, const Matrix44& m);

// -- quaternions ---------------------------------------------------------------
// Rotation part of m (assumed orthonormal) as a unit quaternion.
void QuatFromMatrix34(Quat& out, const Matrix34& m);
// Hamilton product a * b.
void QuatMul(Quat& out, const Quat& a, const Quat& b);
void QuatMulInPlace(Quat& a, const Quat& b);

} // namespace ape
