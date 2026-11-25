// openTIT - Scene set-up passes (see ApeSceneSetup.h).
#include "ape/world/ApeSceneSetup.h"

#include "ape/memory/ApeTFreeList.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeWorldHooks.h"
#include "ape/world/ApeWorldMath.h"

#include <cfloat>

namespace ape::world {

ApeSceneLighting g_sceneLighting;
Aabb g_sceneObjectBounds;

namespace {

template <class T, class Fn>
void ForEachInstance(Fn&& fn)
{
    ApeTListAllocator<T>::GetAllocator().ForEach(fn);
}

// Root objects get their world transform from their local one; children
// are updated through their root.
void UpdateRootWorld(ApeObj* obj)
{
    if (!obj->GetParent())
        obj->UpdateWorld(false, false);
}

template <class T>
void UpdateDirtyRoots()
{
    ForEachInstance<T>([](T* obj) {
        if (obj->IsDirty() && !obj->GetParent())
            obj->UpdateWorld(false, false);
    });
}

void SetEmptyBounds(Aabb& box)
{
    box.min = {FLT_MAX, FLT_MAX, FLT_MAX, 0.0f};
    box.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f};
}

} // namespace

// @func 0x00588FFC UpdateWorldTransforms
void UpdateWorldTransforms(bool allInstances)
{
    if (allInstances) {
        UpdateDirtyRoots<ApeGeomInstance>();
        UpdateDirtyRoots<ApeLightInstance>();
        UpdateDirtyRoots<ApeEmitterInstance>();
        return;
    }
    ApeTArray<ApeObj*>& roots = GetDirtyRootList();
    while (roots.Count() != 0) {
        ApeObj* root = roots[roots.Count() - 1];
        if (!root->GetParent())
            root->UpdateWorld(false, false);
        roots.RemoveAtSwap(roots.Count() - 1);
        root->m_flags &= uint16_t(~kObjFlag_InDirtyList);
    }
}

// @func 0x0058F118 UpdateAllInstanceWorlds
void UpdateAllInstanceWorlds()
{
    ForEachInstance<ApeLightInstance>([](ApeLightInstance* obj) {
        UpdateRootWorld(obj);
        obj->UpdateRegionMask(true);
    });
    ForEachInstance<ApeGeomInstance>([](ApeGeomInstance* obj) {
        UpdateRootWorld(obj);
        obj->UpdateRegionMask(true);
        // NOTE: geometry outside draw modes 3 and 4 loses its name hash
        // here, so it can no longer be looked up by name after the load.
        const uint8_t mode = obj->GetDrawMode();
        if (mode != 3 && mode != 4)
            obj->m_nameHash = 0;
    });
    ForEachInstance<ApeEmitterInstance>([](ApeEmitterInstance* obj) {
        UpdateRootWorld(obj);
        obj->UpdateRegionMask(true);
    });
    ForEachInstance<ApePointInstance>([](ApePointInstance* obj) { UpdateRootWorld(obj); });
    ForEachInstance<ApeCameraInstance>([](ApeCameraInstance* obj) { UpdateRootWorld(obj); });
    ForEachInstance<ApeSplineInstance>([](ApeSplineInstance* obj) { UpdateRootWorld(obj); });
}

// @func 0x0058FD50 ComputeLightGrid
void ComputeLightGrid()
{
    // Box around the local lights (just the origin when there are none).
    Aabb lights;
    bool first = true;
    AabbFromPoint(lights, kWorldZero);
    ForEachInstance<ApeLightInstance>([&](ApeLightInstance* light) {
        if (light->m_lightType != 2)
            return;
        if (first) {
            AabbFromPoint(lights, light->GetWorld().translation);
            first = false;
        } else {
            AabbAddPoint(lights, light->GetWorld().translation);
        }
    });

    // The grid covers at least 128 x 128 units (in x and z) around the
    // centre of the lights.
    constexpr float kMinHalfExtent = 64.0f;
    Vec3 center;
    AabbCenter(lights, center);
    ApeLightGrid& grid = g_lightGrid;
    grid.min = {Min(center.x - kMinHalfExtent, lights.min.x), Min(center.y, lights.min.y),
                Min(center.z - kMinHalfExtent, lights.min.z), 0.0f};
    grid.max = {Max(center.x + kMinHalfExtent, lights.max.x), Max(center.y, lights.max.y),
                Max(center.z + kMinHalfExtent, lights.max.z), 0.0f};

    constexpr float kInvCells = 1.0f / ApeLightGrid::kCells;
    grid.cellSize = {(grid.max.x - grid.min.x) * kInvCells, (grid.max.y - grid.min.y) * kInvCells,
                     (grid.max.z - grid.min.z) * kInvCells, 0.0f};
    auto inverse = [](float size) { return size > 0.0f ? 1.0f / size : 0.0f; };
    grid.invCellSize = {inverse(grid.cellSize.x), inverse(grid.cellSize.y), inverse(grid.cellSize.z), 0.0f};
}

// @func 0x00590DA0 UpdateSceneLighting
void UpdateSceneLighting()
{
    ApeSceneLighting& lighting = g_sceneLighting;
    lighting = ApeSceneLighting();
    ForEachInstance<ApeLightInstance>([&](ApeLightInstance* light) {
        if (light->m_lightGroupMask == 0 || !light->IsVisible())
            return;
        const ApeLightDef* def = static_cast<const ApeLightDef*>(light->GetDef());
        // The stored intensity is the square root of the light's strength.
        const float strength = def->m_intensity * def->m_intensity;
        const Vec3 color = {def->m_color.x * strength, def->m_color.y * strength, def->m_color.z * strength, 0.0f};
        if (light->m_lightType == 0) {
            lighting.ambient = color;
        } else if (light->m_lightType == 1) {
            lighting.directionalColor = color;
            lighting.directionalDir = light->GetWorld().zAxis;
        }
    });
}

// @func 0x005900BC ResetSceneObjectBounds
void ResetSceneObjectBounds()
{
    // NOTE: the console first builds the box of all geometry instance
    // positions and then discards it; only the reset is visible.
    SetEmptyBounds(g_sceneObjectBounds);
}

// @func 0x00590FEC AddToDynamicObjects
void AddToDynamicObjects(ApeObj* obj, bool recurse)
{
    const uint32_t type = obj->GetType();
    if (type < kObjType_FirstInstance || type >= kObjType_EndRenderable)
        return;
    ApeRenderObj* renderObj = static_cast<ApeRenderObj*>(obj);
    if (renderObj->IsDynamic() && (type == kObjType_GeomInstance || type == kObjType_EmitterInstance)) {
        // NOTE: LOD field value 12 marks dynamic objects for the renderer.
        renderObj->SetLodField(12);
        GetDynamicObjectList().PushBack(obj);
        if (type == kObjType_EmitterInstance)
            static_cast<ApeEmitterDef*>(obj->GetDef())->m_isDynamic = 1;
    }
    if (recurse) {
        for (ApeObj* child = obj->GetFirstChild(); child; child = child->GetNextSibling())
            AddToDynamicObjects(child, recurse);
    }
}

// @func 0x00591178 CollectDynamicObjects
void CollectDynamicObjects()
{
    ForEachInstance<ApeEmitterInstance>([](ApeEmitterInstance* obj) { AddToDynamicObjects(obj, false); });
    ForEachInstance<ApeGeomInstance>([](ApeGeomInstance* obj) { AddToDynamicObjects(obj, false); });
}

// @func 0x0058F8C4 HideEmptyGeometry
void HideEmptyGeometry()
{
    const ApeWorldRenderHooks& hooks = g_worldRenderHooks;
    if (!hooks.getMeshPartCount)
        return;
    ForEachInstance<ApeGeomInstance>([&](ApeGeomInstance* obj) {
        const ApeGeomDef* def = static_cast<const ApeGeomDef*>(obj->GetDef());
        if (!def)
            return;
        AglMesh* mesh = def->m_lods[0].mesh;
        const uint16_t parts = hooks.getMeshPartCount(mesh);
        if (parts == 0) {
            obj->SetVisible(false, false);
            return;
        }
        // Visible single-LOD, single-part roots whose material has flag 0x20
        // cleared are hidden as well. NOTE: the meaning of the material flag
        // is not known.
        if (!obj->GetParent() && obj->IsVisible() && def->m_lodCount == 1 && parts == 1 &&
            hooks.getMeshPartMaterialFlags && !(hooks.getMeshPartMaterialFlags(mesh, 0) & 0x20))
            obj->SetVisible(false, false);
    });
}

// @func 0x0058FADC SetupObjectLighting
void SetupObjectLighting()
{
    UpdateLightLinks();
    ForEachInstance<ApeGeomInstance>([](ApeGeomInstance* obj) { UpdateObjectLightLink(obj); });
    ForEachInstance<ApeEmitterInstance>([](ApeEmitterInstance* obj) { UpdateObjectLightLink(obj); });
}

// TODO(0x00596A8C UpdateProcAnims) procedural animation of the render node
// table on the worker threads, then the per-object node refresh.
void UpdateProcAnims(int /*phase*/)
{
}

// TODO(0x0059541C ApeProceduralMatrixAnimMgr::Create)
void CreateProcAnimManager()
{
}

// TODO(0x00596770 ApeProceduralMatrixAnimMgr::Destroy)
void DestroyProcAnimManager()
{
}

// TODO(0x00591A30 SetupLightList) builds the active light list (0x01194ED0).
void SetupLightList()
{
}

// TODO(0x005965BC ReleaseLightList)
void ReleaseLightList()
{
}

// TODO(0x00583518 UpdateLightLinks) per-frame light link refresh.
void UpdateLightLinks()
{
}

// TODO(0x005832FC UpdateObjectLightLink)
void UpdateObjectLightLink(ApeObj* /*obj*/)
{
}

// TODO(0x0058EA70 SetupSceneRenderNodes) render node / name map set-up of
// the geometry instances.
void SetupSceneRenderNodes(ApeScene* /*scene*/)
{
}

// TODO(0x00595968 BuildInstanceBatches) merges geometry instances sharing
// a definition into instanced batches (table 0x0116BF00, count 0x0116E100).
void BuildInstanceBatches()
{
}

// TODO(0x005908A0 SetupSceneViews)
void SetupSceneViews()
{
}

// TODO(0x0058DBE8 FinishSceneBounds) bounds of the shadow casting render
// nodes (0x0118E0B0).
void FinishSceneBounds(ApeScene* /*scene*/)
{
}

// Releases the instance batches built by BuildInstanceBatches (inline part
// of DestroyScene on the console; the batch table is not ported yet).
void ReleaseInstanceBatches()
{
}

} // namespace ape::world
