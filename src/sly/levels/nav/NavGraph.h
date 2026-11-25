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
namespace entity { class Entity; }
namespace levels {

// Opaque NavPower obstacle handle.
using NavObstacleHandle = void*;

// Description of a dynamic obstacle. NOTE: the console builds it from a
// transform and box extents (0x0067C024 decomposes the transform); the exact
// NavPower parameters were not recovered.
struct NavObstacleDesc {
    ape::Matrix34 transform;
    ape::Vec3 extents;
    const char* name = nullptr;   // debug name (the owner's name)
    uint32_t mask = 0xFFFF;       // NOTE: obstacle filter mask; meaning of the bits unknown
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
    // Jump links (0x0060EABC releases one) and the loaded graph (0x0067738C
    // unloads it).
    virtual void ReleaseJumpLink(void* link) { (void)link; }
    virtual void UnloadGraph() {}
};

NavPowerInterface& NavPower();
// Installs the NavPower binding (null restores the inert default).
void SetNavPower(NavPowerInterface* navPower);

// Jump link created from a "SAINavJumpLink_" level object (0x38 bytes).
struct NavJumpLink {
    enum Type : int32_t { kPointLink = 0, kStripLink = 1, kPlatformLink = 2 };
    int32_t type = kPointLink;                    // +0x00
    void* link = nullptr;                         // +0x04 NavPower link object
    uint32_t settingName = 0;                     // +0x08 level object holding the link's properties
    class entity::Entity* objects[6] = {};        // +0x0C entities named by Object1..Object6
    int32_t objectCount = 0;                      // +0x24
};

// Per-level navigation state.
struct NavManager {
    static constexpr int kMaxObstacles = 0x200;
    static constexpr int kMaxJumpLinks = 0x200;

    void* graphData = nullptr;                    // +0x0000 loaded NavGraph.bfx_res image
    size_t graphSize = 0;
    int32_t jumpLinkCount = 0;                    // +0x001C
    int32_t obstacleCount = 0;                    // +0x0020
    NavJumpLink jumpLinks[kMaxJumpLinks];         // +0x0024
    NavObstacleHandle obstacles[kMaxObstacles] = {};  // +0x7024 (pointer to the array on the console)
};

// The level's nav manager (console pointer 0x00F557EC), may be null.
NavManager* GetNavManager();
void SetNavManager(NavManager* nav);

// Platform links: looks up the entities their Object1..Object6 properties
// name (0x00010238).
void ResolveJumpLinkObjects(NavManager& nav);
// Tracks whether the player stands on the nav mesh, with a one second
// hysteresis before the answer flips (NOTE: owner unknown; updated by the AI
// manager's tick).
struct PlayerNavTracker {
    ape::Vec3 meshPoint{};      // +0x0C last nav mesh point under the player
    bool onMesh = false;        // +0x14
    float switchTimer = 0.0f;   // +0x18
};
// One update (0x000104C8). NOTE: the console first calls 0x0067C8B8 on the
// tracker; its result is not used.
void UpdatePlayerNavTracker(PlayerNavTracker& tracker, const ape::Vec3* playerPosition, float dt);

// Level reset: drops every jump link and obstacle and unloads the graph
// (part of 0x0001A840).
void ResetNavManager(NavManager& nav);

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
// Obstacle shaped like a physics body: a box the size of the body's shape,
// placed with the body (and the shape's own offset, for shapes that wrap a
// child shape with a translation or a transform).
NavObstacleHandle AddBodyNavObstacle(NavManager& nav, void* body, const char* name, uint32_t mask);

} // namespace levels
} // namespace sly
