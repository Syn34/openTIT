// openTIT - Screen rectangle of a sphere (0x00557A90), used to scissor the
// draws of local lights.
#include "ape/math/ApeMatrixOps.h"

#include <algorithm>
#include <cmath>

namespace ape {

// @func 0x00557A90 SphereScreenRect
// For each screen axis the planes through the camera that touch the sphere
// give the extent: with the centre at (L, Lz) in that axis' plane (Lz the
// distance in front of the camera) and d = L^2 + Lz^2 - r^2, the tangent
// points lie at L*sqrt(d) -/+ r*Lz over r*L +/- Lz*sqrt(d); only tangent
// points in front of the camera narrow the default -1..1 range.
void SphereScreenRect(const Vec4& sphere, const Matrix34& view, const Matrix44& projection, float* left,
                      float* top, float* right, float* bottom)
{
    const Vec3 centre{view.xAxis.x * sphere.x + view.yAxis.x * sphere.y + view.zAxis.x * sphere.z + view.translation.x,
                      view.xAxis.y * sphere.x + view.yAxis.y * sphere.y + view.zAxis.y * sphere.z + view.translation.y,
                      view.xAxis.z * sphere.x + view.yAxis.z * sphere.y + view.zAxis.z * sphere.z + view.translation.z,
                      0.0f};
    const float radius = sphere.w;
    const float depth = -centre.z;
    const float scale[2] = {projection.rows[0].x, projection.rows[1].y};
    const float offset[2] = {centre.x, centre.y};
    float minimum[2] = {-1.0f, -1.0f};
    float maximum[2] = {1.0f, 1.0f};

    for (int axis = 0; axis < 2; ++axis) {
        const float l = offset[axis];
        const float lengthSq = l * l + depth * depth;
        const float d = lengthSq - radius * radius;
        if (d <= 0.0f)
            continue;   // the camera is inside the sphere's cone: full range
        const float root = std::sqrt(d);
        const float front = lengthSq * depth;
        // First tangent.
        const float numA = root * l - radius * depth;
        const float denA = radius * l + root * depth;
        if (front > -(radius * numA))
            minimum[axis] = std::max(scale[axis] * numA / denA, -1.0f);
        // Second tangent.
        const float numB = radius * depth + root * l;
        const float denB = root * depth - radius * l;
        if (front > radius * numB)
            maximum[axis] = std::min(scale[axis] * numB / denB, 1.0f);
    }
    *left = (minimum[0] + 1.0f) * 0.5f;
    *top = (maximum[1] - 1.0f) * -0.5f;
    *right = (maximum[0] + 1.0f) * 0.5f;
    *bottom = (minimum[1] - 1.0f) * -0.5f;
}

} // namespace ape
