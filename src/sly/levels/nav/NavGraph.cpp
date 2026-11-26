// openTIT - Level navigation: nav graph loading, nav mesh queries and dynamic obstacles.
#include "sly/levels/nav/NavGraph.h"

#include "sly/entity/Entity.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/levels/ai/AIHooks.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"
#include "ape/setting/ApeSetting.h"
#include "ape/world/ApeInstances.h"
#include "sly/game/SAISettings.h"

namespace sly {
namespace levels {

namespace {
NavPowerInterface g_nullNavPower;
NavPowerInterface* g_navPower = &g_nullNavPower;
NavManager* g_navManager = nullptr;

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

// @func 0x00011410 AddBodyNavObstacle
NavObstacleHandle AddBodyNavObstacle(NavManager& nav, void* body, const char* name, uint32_t mask)
{
    using namespace entity;
    NavObstacleDesc desc;
    desc.transform = RigidBodyTransform(body);
    desc.name = name;
    desc.mask = mask;

    // Shapes that carry their own offset are placed by it; their box is
    // assumed to be centred on that frame.
    const void* shape = RigidBodyShape(body);
    bool placedByShape = false;
    if (shape) {
        switch (HavokShapeType(shape)) {
        case kHavokShape_ConvexTranslate: {
            const ape::Vec3 offset = HavokShapeTranslation(shape);
            const ape::Matrix34& m = desc.transform;
            desc.transform.translation.x += m.xAxis.x * offset.x + m.yAxis.x * offset.y + m.zAxis.x * offset.z;
            desc.transform.translation.y += m.xAxis.y * offset.x + m.yAxis.y * offset.y + m.zAxis.y * offset.z;
            desc.transform.translation.z += m.xAxis.z * offset.x + m.yAxis.z * offset.y + m.zAxis.z * offset.z;
            placedByShape = true;
            break;
        }
        case kHavokShape_ConvexTransform:
        case kHavokShape_Transform: {
            const ape::Matrix34 local = HavokShapeTransform(shape);
            const ape::Matrix34 bodyTransform = desc.transform;
            ape::Matrix34MulRotation(desc.transform, local, bodyTransform);
            const ape::Vec3& t = local.translation;
            desc.transform.translation.x = bodyTransform.translation.x + t.x * bodyTransform.xAxis.x +
                                           t.y * bodyTransform.yAxis.x + t.z * bodyTransform.zAxis.x;
            desc.transform.translation.y = bodyTransform.translation.y + t.x * bodyTransform.xAxis.y +
                                           t.y * bodyTransform.yAxis.y + t.z * bodyTransform.zAxis.y;
            desc.transform.translation.z = bodyTransform.translation.z + t.x * bodyTransform.xAxis.z +
                                           t.y * bodyTransform.yAxis.z + t.z * bodyTransform.zAxis.z;
            placedByShape = true;
            break;
        }
        default:
            break;
        }
    }

    // The box spans the shape's bounds in its own frame.
    ape::Aabb local;
    HavokShapeLocalAabb(shape, local);
    desc.extents = {local.max.x - local.min.x, local.max.y - local.min.y, local.max.z - local.min.z, 0.0f};
    if (!placedByShape) {
        // Plain shapes may be off centre: the box goes where the shape's
        // world bounds are centred.
        ape::Aabb world;
        HavokShapeAabb(shape, desc.transform, world);
        desc.transform.translation.x = (world.min.x + world.max.x) * 0.5f;
        desc.transform.translation.y = (world.min.y + world.max.y) * 0.5f;
        desc.transform.translation.z = (world.min.z + world.max.z) * 0.5f;
    }
    return AddNavObstacle(nav, desc);
}

NavManager* GetNavManager() { return g_navManager; }
void SetNavManager(NavManager* nav) { g_navManager = nav; }

NavManager* CreateNavManager()
{
    NavManager* nav = new NavManager();
    g_navManager = nav;
    nav->arena = ape::MemAlloc(NavManager::kArenaSize, ape::kMemClass_Default, 0x10);
    NavPower().StartWorld(nav->arena, NavManager::kArenaSize);
    return nav;
}

void DestroyNavManager()
{
    NavManager* nav = g_navManager;
    if (!nav)
        return;
    g_navManager = nullptr;
    for (NavObstacleHandle& obstacle : nav->obstacles) {
        if (obstacle) {
            NavPower().DestroyObstacle(obstacle);
            obstacle = nullptr;
        }
    }
    NavPower().StopWorld();
    ape::MemFree(nav->arena);
    for (NavJumpLink& link : nav->jumpLinks) {
        if (link.link) {
            NavPower().ReleaseJumpLink(link.link);
            link.link = nullptr;
        }
    }
    delete nav;
}

// @func 0x00010238 ResolveJumpLinkObjects
void ResolveJumpLinkObjects(NavManager& nav)
{
    using ai::AIProp;
    static const AIProp kObjectProps[6] = {AIProp::JumpLink_Object1, AIProp::JumpLink_Object2,
                                           AIProp::JumpLink_Object3, AIProp::JumpLink_Object4,
                                           AIProp::JumpLink_Object5, AIProp::JumpLink_Object6};
    for (int i = 0; i < nav.jumpLinkCount; ++i) {
        NavJumpLink& link = nav.jumpLinks[i];
        link.objectCount = 0;
        if (link.type != NavJumpLink::kPlatformLink)
            continue;
        const ai::PropertyBlock* props = link.settingName ? ai::FindSettingObject(link.settingName) : nullptr;
        if (!props)
            continue;
        for (int j = 0; j < 6; ++j) {
            const char* name = ai::PropString(props, kObjectProps[j]);
            const uint32_t hash = name ? ape::HashNameNoCase(name) : 0;
            if (hash == 0)
                continue;
            link.objects[j] = entity::FindEntity(hash);
            if (link.objects[j])
                ++link.objectCount;
        }
    }
}

// @func 0x00010B18 AssociateNavJumpLink
void AssociateNavJumpLink(ape::setting::Setting* setting, const char* splineName, bool reload)
{
    NavManager* nav = GetNavManager();
    if (reload || !nav)
        return;
    namespace params = sai_params;
    constexpr uint32_t kLinkType_Strip = 0x9AEE76F2u;     // "StripLink"
    constexpr uint32_t kLinkType_Platform = 0x936F8EEDu;  // "PlatformLink"

    ape::world::ApeSplineInstance* spline = ai::FindLevelSplineInstance(ape::HashNameNoCase(splineName));
    if (!spline) {
        ape::TempFormat("Fail to find the spline (%s) to create the Nav jump link!", splineName);
        return;
    }
    const uint32_t linkType = setting->Value(params::LinkType).u;
    ape::world::ApeSplineInstance* edge2 = nullptr;
    if (linkType == kLinkType_Strip) {
        // The second edge is the spline's child; a parented spline is
        // itself the second edge of another strip.
        if (spline->m_parent || !spline->m_firstChild)
            return;
        edge2 = ai::FindLevelSplineInstance(spline->m_firstChild->GetNameHash());
        if (!edge2) {
            ape::TempFormat("Fail to find the spline2 (%s) to create the Nav jump link strip!",
                            spline->m_firstChild->GetName());
            return;
        }
    }
    if (nav->jumpLinkCount >= NavManager::kMaxJumpLinks) {
        ape::TempFormat("Too many jump links, max is (%d)", NavManager::kMaxJumpLinks);
        return;
    }
    NavJumpLink& link = nav->jumpLinks[nav->jumpLinkCount++];
    link.type = linkType == kLinkType_Platform ? NavJumpLink::kPlatformLink : NavJumpLink::kLink;
    link.settingName = setting->nameHash;
    link.platformJumpDistXZ = setting->Value(params::PlatformJumpDistXZ).f;
    link.platformJumpHeight = setting->Value(params::PlatformJumpHeight).f;
    link.platformFallHeight = setting->Value(params::PlatformFallHeight).f;
    link.walk = setting->Value(params::Walk).b;

    spline->m_root->UpdateWorld(false, false);
    ape::Vec3 points[4];
    spline->EvaluatePosition(0.0f, points[0], false);
    spline->EvaluatePosition(1.0f, points[1], false);
    if (edge2) {
        edge2->EvaluatePosition(0.0f, points[2], false);
        edge2->EvaluatePosition(1.0f, points[3], false);
        link.link = NavPower().CreateJumpLink(points, 4);
    } else {
        // NOTE: the console hands NavPower a parameter block for point links
        // whose contents were not recovered.
        link.link = NavPower().CreateJumpLink(points, 2);
    }
}

// @func 0x000104C8 UpdatePlayerNavTracker
void UpdatePlayerNavTracker(PlayerNavTracker& tracker, const ape::Vec3* playerPosition, float dt)
{
    if (!playerPosition)
        return;
    ape::Vec3 point;
    bool onMesh = false;
    if (NavPower().FindNearestPoint(*playerPosition, &point)) {
        const float dy = point.y - playerPosition->y;
        if (dy > -1.0f && dy < 0.5f) {
            const float dx = point.x - playerPosition->x;
            const float dz = point.z - playerPosition->z;
            onMesh = dz * dz + dx * dx + 0.0f < 1.0f;
        }
    }
    if (tracker.onMesh != onMesh) {
        if (tracker.switchTimer > 1.0f)
            tracker.onMesh = onMesh;
        else
            tracker.switchTimer += dt;
    }
    if (onMesh && tracker.onMesh) {
        tracker.switchTimer = 0.0f;
        tracker.meshPoint = point;
    }
}

void ResetNavManager(NavManager& nav)
{
    for (int i = 0; i < nav.jumpLinkCount; ++i) {
        NavPower().ReleaseJumpLink(nav.jumpLinks[i].link);
        nav.jumpLinks[i].objectCount = 0;
    }
    for (int i = 0; i < nav.obstacleCount; ++i)
        NavPower().DestroyObstacle(nav.obstacles[i]);
    if (nav.graphData) {
        NavPower().UnloadGraph();
        ape::MemFree(nav.graphData);
        nav.graphData = nullptr;
    }
}

} // namespace levels
} // namespace sly
