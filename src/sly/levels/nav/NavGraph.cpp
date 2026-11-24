// openTIT - Level navigation: nav graph loading, nav mesh queries and dynamic obstacles.
#include "sly/levels/nav/NavGraph.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"

namespace sly {
namespace levels {

namespace {
NavPowerInterface g_nullNavPower;
NavPowerInterface* g_navPower = &g_nullNavPower;

const ape::Matrix34 kIdentity = {
    {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
};
} // namespace

NavPowerInterface& NavPower()
{
    return *g_navPower;
}

void SetNavPower(NavPowerInterface* navPower)
{
    g_navPower = navPower ? navPower : &g_nullNavPower;
}

// @func 0x000106EC LoadNavGraph
void LoadNavGraph(NavManager& nav, const char* levelName)
{
    const char* path = ape::TempFormat("Levels/%s/%s", levelName, "NavGraph.bfx_res");
    if (nav.graphData || !ape::file::FileExists(path))
        return;
    ape::file::File* file = ape::file::OpenFile(path, "rb");
    if (!file)
        return;
    const size_t size = static_cast<size_t>(file->GetSize());
    nav.graphData = ape::MemAlloc(size, ape::kMemClass_Default, 0x10);
    nav.graphSize = size;
    file->Read(nav.graphData, size);
    file->Close();
    // The graph is baked in world space.
    NavPower().AddNavGraph(nav.graphData, size, kIdentity);
}

// @func 0x0001087C SnapToNavMesh
ape::Vec3 SnapToNavMesh(const NavManager& nav, const ape::Vec3& position)
{
    ape::Vec3 result = position;
    if (nav.graphData) {
        ape::Vec3 hit = position;
        if (NavPower().FindNearestPoint(position, &hit))
            result = hit;
    }
    return result;
}

// @func 0x00010980 IsNearNavMesh
bool IsNearNavMesh(const NavManager& nav, const ape::Vec3& position, float radius, float minDy,
                   float maxDy, ape::Vec3* out)
{
    const ape::Vec3 point = SnapToNavMesh(nav, position);
    const float dy = point.y - position.y;
    if (dy < minDy || dy > maxDy)
        return false;
    if (out)
        *out = point;
    const float dx = point.x - position.x;
    const float dz = point.z - position.z;
    return dz * dz + dx * dx < radius * radius;
}

// @func 0x000110EC AddNavObstacle
NavObstacleHandle AddNavObstacle(NavManager& nav, const NavObstacleDesc& desc)
{
    if (!nav.graphData)
        return nullptr;
    if (nav.obstacleCount >= NavManager::kMaxObstacles) {
        ape::TempFormat("Obstacle list overflow, max is (%d)", NavManager::kMaxObstacles);
        return nullptr;
    }
    NavObstacleHandle handle = NavPower().CreateObstacle(desc);
    nav.obstacles[nav.obstacleCount++] = handle;
    return handle;
}

// @func 0x000112D8 RemoveNavObstacle
void RemoveNavObstacle(NavManager& nav, NavObstacleHandle handle)
{
    for (int i = 0; i < nav.obstacleCount; ++i) {
        if (nav.obstacles[i] == handle) {
            NavPower().DestroyObstacle(nav.obstacles[i]);
            // Swap-remove; the moved entry is not re-examined.
            nav.obstacles[i] = nav.obstacles[--nav.obstacleCount];
            nav.obstacles[nav.obstacleCount] = nullptr;
        }
    }
    // NOTE: the console's handles are reference counted; it also drops the
    // caller's reference here. Raw handles need no such step.
}

} // namespace levels
} // namespace sly
