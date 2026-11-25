// openTIT - Small geometric queries used by the world layer.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

namespace ape::world {

bool SegmentIntersectsQuad(const Vec3 corners[4], const Vec3& start, const Vec3& end, float& outT);

// 8x8 grid over the level's x/z extent used to bucket lights
// (0x011944F0 min, 0x01194500 max, 0x01194510 cell size, 0x01194520
// inverse cell size). Built by ComputeLightGrid (ApeSceneSetup.h).
struct ApeLightGrid {
    static constexpr int kCells = 8;
    Vec3 min = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec3 max = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec3 cellSize = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec3 invCellSize = {0.0f, 0.0f, 0.0f, 0.0f};
};
extern ApeLightGrid g_lightGrid;

void ComputeLightGridCells(const Sphere& sphere, int& outMinX, int& outMaxX, int& outMinZ, int& outMaxZ);

} // namespace ape::world
