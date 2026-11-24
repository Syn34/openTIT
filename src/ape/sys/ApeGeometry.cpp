// openTIT - Plane helpers.
#include "ape/sys/ApeGeometry.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include <cmath>

namespace ape {

namespace {
struct Xyz {
    float x, y, z;
};

Xyz Cross(const Plane& a, const Plane& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
}

// @func 0x0055EA6C PlaneTransform
Plane& PlaneTransform(Plane& plane, const Matrix44& m)
{
    Matrix44 transposed;
    for (int r = 0; r < 4; ++r) {
        const float* row = &m.rows[r].x;
        (&transposed.rows[0].x)[r] = row[0];
        (&transposed.rows[1].x)[r] = row[1];
        (&transposed.rows[2].x)[r] = row[2];
        (&transposed.rows[3].x)[r] = row[3];
    }
    Matrix44 inverse;
    Matrix44SetInverse(inverse, transposed);
    const Plane copy = plane;
    Vec4MulMatrix44(plane, copy, inverse);
    return plane;
}

// @func 0x0055EB58 PlaneIntersect3
bool PlaneIntersect3(Vec3& out, const Plane& a, const Plane& b, const Plane& c)
{
    const Xyz bc = Cross(b, c);
    const float det = a.x * bc.x + a.y * bc.y + a.z * bc.z;
    if (det == 0.0f)
        return false;

    // Cramer's rule: p = (da (b x c) + db (c x a) + dc (a x b)) / det.
    const Xyz ca = Cross(c, a);
    const Xyz ab = Cross(a, b);
    const float inv = 1.0f / det;
    out.x = (ab.x * c.w + bc.x * a.w + ca.x * b.w) * inv;
    out.y = (ab.y * c.w + bc.y * a.w + ca.y * b.w) * inv;
    out.z = (ab.z * c.w + bc.z * a.w + ca.z * b.w) * inv;
    return true;
}

// --- axis normalisation ------------------------------------------------------------

// NOTE: the console uses the AltiVec reciprocal square root estimate refined
// by one Newton step; the exact value is used here.
// @func 0x0056D268 Matrix34NormalizeAxes
void Matrix34NormalizeAxes(Matrix34& out, const Matrix34& m)
{
    const Vec3* in[3] = { &m.xAxis, &m.yAxis, &m.zAxis };
    float scale[3];
    for (int i = 0; i < 3; ++i)
        scale[i] = 1.0f / std::sqrt(in[i]->x * in[i]->x + in[i]->y * in[i]->y + in[i]->z * in[i]->z);
    const Vec3 translation = m.translation;
    Vec3* dst[3] = { &out.xAxis, &out.yAxis, &out.zAxis };
    const Vec3 axes[3] = { m.xAxis, m.yAxis, m.zAxis };
    for (int i = 0; i < 3; ++i)
        *dst[i] = Vec3{ axes[i].x * scale[i], axes[i].y * scale[i], axes[i].z * scale[i], axes[i].pad * scale[i] };
    out.translation = translation;
}

// @func 0x0056D374 Matrix34NormalizeAxes
Matrix34& Matrix34NormalizeAxes(Matrix34& m)
{
    Matrix34NormalizeAxes(m, m);
    return m;
}

// --- culling -----------------------------------------------------------------------

// @func 0x00576E28 FrustumContainsAabb
bool FrustumContainsAabb(const Plane planes[6], const Aabb& box)
{
    for (int corner = 0; corner < 8; ++corner) {
        const float x = (corner & 1) ? box.min.x : box.max.x;
        const float y = (corner & 2) ? box.min.y : box.max.y;
        const float z = (corner & 4) ? box.min.z : box.max.z;
        for (int i = 0; i < 6; ++i) {
            const Plane& p = planes[i];
            if (p.x * x + p.y * y + p.z * z + p.w < 0.0f)
                return false;
        }
    }
    return true;
}

// @func 0x00577680 PlaneIntersectsBox
bool PlaneIntersectsBox(const Plane& plane, const Vec3& center, const Vec3& extents)
{
    const float n[3] = { plane.x, plane.y, plane.z };
    const float c[3] = { center.x, center.y, center.z };
    const float e[3] = { extents.x, extents.y, extents.z };
    float nearCorner[3], farCorner[3];
    for (int i = 0; i < 3; ++i) {
        // Corners of the box closest to / furthest along the normal.
        const float lo = n[i] > 0.0f ? -e[i] : e[i];
        nearCorner[i] = lo - c[i];
        farCorner[i] = -lo - c[i];
    }
    const float dNear = n[0] * nearCorner[0] + n[1] * nearCorner[1] + n[2] * nearCorner[2];
    const float dFar = n[0] * farCorner[0] + n[1] * farCorner[1] + n[2] * farCorner[2];
    return !(dNear > 0.0f || dFar < 0.0f);
}

// Separating axis test of a triangle against an axis aligned box: the nine
// edge x box-axis cross products, the three box faces and the triangle plane.
// @func 0x00577788 TriangleIntersectsBox
bool TriangleIntersectsBox(const Vec3& center, const Vec3& extents, const Vec3 triangle[3])
{
    float v[3][3];
    for (int i = 0; i < 3; ++i) {
        v[i][0] = triangle[i].x - center.x;
        v[i][1] = triangle[i].y - center.y;
        v[i][2] = triangle[i].z - center.z;
    }
    const float e[3] = {extents.x, extents.y, extents.z};
    float edges[3][3];
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k)
            edges[i][k] = v[(i + 1) % 3][k] - v[i][k];

    // Axis = edge x unit axis `a`; project the three vertices and the box.
    for (const auto& edge : edges) {
        for (int a = 0; a < 3; ++a) {
            const int b = (a + 1) % 3, c = (a + 2) % 3;
            // axis components: (edge x unit_a) has zero along a, edge[c] along b, -edge[b] along c
            float pmin = FLT_MAX, pmax = -FLT_MAX;
            for (const auto& vert : v) {
                const float p = edge[c] * vert[b] - edge[b] * vert[c];
                pmin = std::min(pmin, p);
                pmax = std::max(pmax, p);
            }
            const float radius = std::fabs(edge[c]) * e[b] + std::fabs(edge[b]) * e[c];
            if (pmin > radius || pmax < -radius)
                return false;
        }
    }

    // Box faces: the triangle's extent along each axis.
    for (int a = 0; a < 3; ++a) {
        const float lo = std::min(v[0][a], std::min(v[1][a], v[2][a]));
        const float hi = std::max(v[0][a], std::max(v[1][a], v[2][a]));
        if (lo > e[a] || hi < -e[a])
            return false;
    }

    // Triangle plane against the box.
    const Plane normal = {edges[0][1] * edges[1][2] - edges[0][2] * edges[1][1],
                          edges[0][2] * edges[1][0] - edges[0][0] * edges[1][2],
                          edges[0][0] * edges[1][1] - edges[0][1] * edges[1][0], 0.0f};
    return PlaneIntersectsBox(normal, {v[0][0], v[0][1], v[0][2], 0.0f}, extents);
}

// --- camera matrices -------------------------------------------------------------

namespace {
float* M(Matrix44& m) { return &m.rows[0].x; }

void Normalize3(float& x, float& y, float& z)
{
    const float inv = 1.0f / std::sqrt(x * x + y * y + z * z);
    x *= inv;
    y *= inv;
    z *= inv;
}

void ZeroAll(float* m)
{
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;
}

// Parabolic sine approximation for x in [-pi, pi] with one refinement step.
float FastParabolicSin(float x)
{
    float y = x * 1.2732395f + x * -0.40528473f * std::fabs(x);
    return (y * std::fabs(y) - y) * 0.225f + y;
}
}

// @func 0x0056A0D0 MatrixLookAt
void MatrixLookAt(Matrix44& out, const Vec3& eye, const Vec3& target, const Vec3& up)
{
    // Z points from the target back to the eye.
    float zx = eye.x - target.x, zy = eye.y - target.y, zz = eye.z - target.z;
    Normalize3(zx, zy, zz);
    float xx = up.y * zz - up.z * zy, xy = up.z * zx - up.x * zz, xz = up.x * zy - up.y * zx;
    Normalize3(xx, xy, xz);
    const float yx = zy * xz - zz * xy, yy = zz * xx - zx * xz, yz = zx * xy - zy * xx;

    float* m = M(out);
    m[0] = xx;  m[1] = yx;  m[2] = zx;
    m[4] = xy;  m[5] = yy;  m[6] = zy;
    m[8] = xz;  m[9] = yz;  m[10] = zz;
    m[12] = -(xx * eye.x + xy * eye.y + xz * eye.z);
    m[13] = -(yx * eye.x + yy * eye.y + yz * eye.z);
    m[14] = -(zx * eye.x + zy * eye.y + zz * eye.z);
}

// @func 0x0056A200 MatrixPerspectiveFov
void MatrixPerspectiveFov(float fovY, float aspect, float zNear, float zFar, Matrix44& out)
{
    const float f = 1.0f / std::tan(fovY * 0.5f);
    float* m = M(out);
    ZeroAll(m);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = zFar / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = zFar * zNear / (zNear - zFar);
}

// @func 0x0056A2D8 MatrixPerspective
void MatrixPerspective(float width, float height, float zNear, float zFar, Matrix44& out)
{
    float* m = M(out);
    ZeroAll(m);
    m[0] = zNear * 2.0f / width;
    m[5] = zNear * 2.0f / height;
    m[10] = zFar / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = zFar * zNear / (zNear - zFar);
}

// @func 0x0056A358 MatrixOrtho
void MatrixOrtho(float left, float right, float bottom, float top, float zNear, float zFar, Matrix44& out)
{
    float* m = M(out);
    ZeroAll(m);
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = 1.0f / (zNear - zFar);
    m[12] = (left + right) / (left - right);
    m[13] = (bottom + top) / (bottom - top);
    m[14] = zNear / (zNear - zFar);
    m[15] = 1.0f;
}

// @func 0x0056A410 MatrixAxisAngle
void MatrixAxisAngle(float radians, Matrix34& out, const Vec3& axis)
{
    // Wrap into [-pi, pi].
    const float pi = 3.14159265f, twoPi = 6.28318531f;
    const float bias = radians < 0.0f ? -pi : pi;
    float a = bias + radians;
    a = a - (float)(int64_t)(a / twoPi) * twoPi - bias;

    const float s = FastParabolicSin(a);
    float shifted = a + 1.57079633f;
    if (shifted > pi)
        shifted -= twoPi;
    const float c = FastParabolicSin(shifted);

    const float x = axis.x, y = axis.y, z = axis.z;
    const float tx = x - x * c, ty = y - y * c, tz = z - z * c;   // axis * (1 - cos)
    out.xAxis.x = x * tx + c;
    out.xAxis.y = x * ty + z * s;
    out.xAxis.z = x * tz - y * s;
    out.yAxis.x = y * tx - z * s;
    out.yAxis.y = y * ty + c;
    out.yAxis.z = y * tz + x * s;
    out.zAxis.x = z * tx + y * s;
    out.zAxis.y = z * ty - x * s;
    out.zAxis.z = z * tz + c;
    out.translation.x = out.translation.y = out.translation.z = 0.0f;
}

// @func 0x0056A578 MatrixSetObliqueNearPlane
void MatrixSetObliqueNearPlane(Matrix44& projection, const Plane& clipPlane)
{
    float* m = M(projection);
    auto sign = [](float v) { return (float)((v > 1e-5f ? 1 : 0) - (v < -1e-5f ? 1 : 0)); };
    // Clip-space corner opposite the plane, taken back to view space.
    const float qx = (sign(clipPlane.x) + m[8]) / m[0];
    const float qy = (sign(clipPlane.y) + m[9]) / m[5];
    const float qw = (1.0f + m[10]) / m[14];
    const float scale = 1.0f / (clipPlane.x * qx + clipPlane.y * qy - clipPlane.z + clipPlane.w * qw);
    m[2] = clipPlane.x * scale;
    m[6] = clipPlane.y * scale;
    m[10] = clipPlane.z * scale;
    m[14] = clipPlane.w * scale;
}

} // namespace ape
