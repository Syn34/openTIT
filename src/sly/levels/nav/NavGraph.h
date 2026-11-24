// openTIT - Level navigation: nav graph loading, nav mesh queries and dynamic obstacles.
//
// The console uses the NavPower middleware (Babel Flux, "bfx") for AI path
// finding. Each level ships a baked graph "Levels/<level>/NavGraph.bfx_res"
// that is handed to NavPower; gameplay code then queries nearest nav mesh
// points and registers dynamic obstacles.
//
// NavPower is not part of this project. The game reaches it only through
// NavPowerInterface; the default implementation does nothing (no graph, no
// nav mesh hits), which is what the original does for a level without a
// nav graph.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstddef>
#include <cstdint>

namespace sly {
namespace levels {

// Opaque NavPower obstacle handle.
using NavObstacleHandle = void*;

// Description of a dynamic obstacle. NOTE: the console builds it from a
// transform and box extents (0x0067C024 decomposes the transform); the exact
// NavPower parameters were not recovered.
struct NavObstacleDesc {
    ape::Matrix34 transform;
    ape::Vec3 extents;
};

class NavPowerInterface {
public:
    virtual ~NavPowerInterface() = default;

    // Hands a loaded graph image to NavPower (0x00677C7C).
    virtual void AddNavGraph(const void* data, size_t size, const ape::Matrix34& transform)
    {
        (void)data; (void)size; (void)transform;
    }
    // Nearest nav mesh point to `position` (0x00630F94 query, 0x0062D25C
    // result, 0x0062E094/0x0062D08C release). False if there is none.
    virtual bool FindNearestPoint(const ape::Vec3& position, ape::Vec3* out)
    {
        (void)position; (void)out;
        return false;
    }
    // Obstacles (0x006382A8 create, 0x00637C3C compare, 0x00637894/0x00638360 destroy).
    virtual NavObstacleHandle CreateObstacle(const NavObstacleDesc& desc)
    {
        (void)desc;
        return nullptr;
    }
    virtual void DestroyObstacle(NavObstacleHandle handle) { (void)handle; }
};

NavPowerInterface& NavPower();
// Installs the NavPower binding (null restores the inert default).
void SetNavPower(NavPowerInterface* navPower);

// Per-level navigation state.
struct NavManager {
    static constexpr int kMaxObstacles = 0x200;

    void* graphData = nullptr;                    // +0x0000 loaded NavGraph.bfx_res image
    size_t graphSize = 0;
    int32_t obstacleCount = 0;                    // +0x0020
    NavObstacleHandle obstacles[kMaxObstacles] = {};  // +0x7024 (pointer to the array on the console)
};

// Loads "Levels/<levelName>/NavGraph.bfx_res" once and registers it.
void LoadNavGraph(NavManager& nav, const char* levelName);

// Nearest nav mesh point to `position`, or `position` itself when the level
// has no graph or nothing is found.
ape::Vec3 SnapToNavMesh(const NavManager& nav, const ape::Vec3& position);

// True when the nav mesh point nearest `position` lies within `radius`
// horizontally and within [minDy, maxDy] vertically. The point is stored in
// `out` (if given) when the height test passes.
bool IsNearNavMesh(const NavManager& nav, const ape::Vec3& position, float radius, float minDy,
                   float maxDy, ape::Vec3* out);

// Creates an obstacle and remembers it. Returns the handle (null when the
// level has no graph or the list is full).
NavObstacleHandle AddNavObstacle(NavManager& nav, const NavObstacleDesc& desc);
// Destroys a remembered obstacle.
void RemoveNavObstacle(NavManager& nav, NavObstacleHandle handle);

} // namespace levels
} // namespace sly
