// openTIT - Plane helpers (0x0055EA6C-0x0055EB58).
//
// Planes are packed as Vec4 (normal.x, normal.y, normal.z, d) for the plane
// dot(normal, p) == d.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"
#include "ape/math/ApeMatrixOps.h"

namespace ape {

using Plane = Vec4;

// Transforms a plane in place by m (row vectors): the plane is multiplied by
// the inverse of m's transpose. Returns `plane`.
Plane& PlaneTransform(Plane& plane, const Matrix44& m);

// Point where three planes meet. Returns false (leaving `out` untouched)
// when the normals are linearly dependent.
bool PlaneIntersect3(Vec3& out, const Plane& a, const Plane& b, const Plane& c);

// Copies `m` with each of the three axis rows scaled to unit length (all
// four lanes of an axis row are scaled; the translation row is copied).
void Matrix34NormalizeAxes(Matrix34& out, const Matrix34& m);
// In-place version; returns `m`.
Matrix34& Matrix34NormalizeAxes(Matrix34& m);

// True when all eight corners of `box` are on the positive side of all six
// planes (dot(normal, p) + w >= 0).
bool FrustumContainsAabb(const Plane planes[6], const Aabb& box);
// True when the plane dot(normal, p) == 0 passes through the box spanning
// [-extents - center, extents - center] (the plane's w is not used).
bool PlaneIntersectsBox(const Plane& plane, const Vec3& center, const Vec3& extents);

// --- camera matrices (0x0056A0D0-0x0056A578) --------------------------------------
// All of these write a 4x4 matrix for row vectors (clip = v * M) with a
// right-handed view space looking down -Z and a [0, 1] depth range. Entries
// not listed in a function's comment are left untouched where noted.

// View matrix from eye position, target and up vector. Only the 3x4 part is
// written: the fourth column (rows[i].w) keeps its previous contents.
void MatrixLookAt(Matrix44& out, const Vec3& eye, const Vec3& target, const Vec3& up);
// Perspective projection from a vertical field of view (radians).
void MatrixPerspectiveFov(float fovY, float aspect, float zNear, float zFar, Matrix44& out);
// Perspective projection from the view volume size at the near plane.
void MatrixPerspective(float width, float height, float zNear, float zFar, Matrix44& out);
// Off-centre orthographic projection.
void MatrixOrtho(float left, float right, float bottom, float top, float zNear, float zFar, Matrix44& out);
// Rotation of `radians` about a unit axis (fast sine/cosine approximations),
// zero translation. The fourth lanes are left untouched.
void MatrixAxisAngle(float radians, Matrix34& out, const Vec3& axis);
// Replaces the near plane of a perspective projection with the view-space
// plane `clipPlane` (oblique frustum clipping, e.g. for mirrors and water).
void MatrixSetObliqueNearPlane(Matrix44& projection, const Plane& clipPlane);

} // namespace ape
