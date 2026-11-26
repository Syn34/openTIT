// openTIT - Game helpers for world objects.
#include "sly/game/ObjectHelpers.h"

#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "sly/game/SlyGame.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"

namespace sly {

namespace {

// NOTE: the world layer's opaque AglMesh is the renderer's topology (mesh
// list at +0x30, part count at +0x34 on the console).
agl::AglTopology* LodTopology(const ape::world::ApeGeomLod& lod)
{
    return reinterpret_cast<agl::AglTopology*>(lod.mesh);
}

ape::world::ApeGeomDef* GeomDefOf(ape::world::ApeObj* object)
{
    return static_cast<ape::world::ApeGeomDef*>(object->GetDef());
}

template <class Fn>
void ForEachMaterial(ape::world::ApeGeomDef* def, Fn&& fn)
{
    for (int lod = 0; lod < def->m_lodCount; ++lod) {
        agl::AglTopology* topology = LodTopology(def->m_lods[lod]);
        for (uint16_t i = 0; i < topology->meshCount; ++i) {
            agl::AglMaterial* material = topology->meshes[i].material;
            if (material && fn(material))
                return;
        }
    }
}

// Visitor of SetHierarchyLod.
// @func 0x004B00EC SetLodVisitor
bool SetLodVisitor(ape::world::ApeObj* object, void* arg)
{
    if (object)
        static_cast<ape::world::ApeRenderObj*>(object)->SetLodField(*static_cast<uint32_t*>(arg));
    return true;
}

struct NameSearch {
    uint32_t nameHash;
    ape::world::ApeObj* found;
};

// Visitor of FindInHierarchy: stops at the first match.
// @func 0x004AFCB4 FindByNameVisitor
// @func 0x00099FE0 FindByNameVisitor (copy 2)
// @func 0x0009A074 FindByNameVisitor (copy 3)
// @func 0x002F5D6C FindByNameVisitor (copy 4)
bool FindByNameVisitor(ape::world::ApeObj* object, void* arg)
{
    auto* search = static_cast<NameSearch*>(arg);
    const uint32_t hash = object->GetNameHash();
    if (hash == search->nameHash || (hash == 0 && ape::HashNameNoCase(object->GetName()) == search->nameHash)) {
        search->found = object;
        return false;
    }
    return true;
}

} // namespace

// @func 0x004A59B4 FindGeomMaterial
agl::AglMaterial* FindGeomMaterial(ape::world::ApeObj* geom, uint32_t materialHash)
{
    agl::AglMaterial* found = nullptr;
    ForEachMaterial(GeomDefOf(geom), [&](agl::AglMaterial* material) {
        if (material->nameHash != materialHash)
            return false;
        found = material;
        return true;
    });
    return found;
}

// @func 0x004A5A48 FindMaterialInHierarchy
agl::AglMaterial* FindMaterialInHierarchy(ape::world::ApeObj* object, uint32_t materialHash)
{
    if (!object)
        return nullptr;
    if (agl::AglMaterial* material = FindGeomMaterial(object, materialHash))
        return material;
    for (ape::world::ApeObj* child = object->GetFirstChild(); child; child = child->GetNextSibling()) {
        if (child->GetType() != ape::world::kObjType_GeomInstance)
            continue;
        if (agl::AglMaterial* material = FindMaterialInHierarchy(child, materialHash))
            return material;
    }
    return nullptr;
}

// @func 0x004A5B80 SetGeomDepthAlways
void SetGeomDepthAlways(ape::world::ApeObj* geom)
{
    if (!geom || !geom->GetDef())
        return;
    ForEachMaterial(GeomDefOf(geom), [](agl::AglMaterial* material) {
        material->depthFunc = agl::gpu::CompareFunc::Always;
        return false;
    });
}

// @func 0x004A5C20 SetHierarchyLod
void SetHierarchyLod(ape::world::ApeObj* object, uint32_t lod)
{
    if (!object)
        return;
    static_cast<ape::world::ApeRenderObj*>(object)->SetLodField(lod);
    object->ForEachInHierarchy(SetLodVisitor, &lod, false);
}

// @func 0x004A5AE8 FindInHierarchy
ape::world::ApeObj* FindInHierarchy(ape::world::ApeObj* object, uint32_t nameHash)
{
    if (!object || nameHash == 0)
        return nullptr;
    if (object->GetNameHash() == nameHash)
        return object;
    NameSearch search{ nameHash, nullptr };
    object->ForEachInHierarchy(FindByNameVisitor, &search, false);
    return search.found;
}

// @func 0x004A5854 WorldToScreenPoint
ape::Vec3 WorldToScreenPoint(const ape::Vec3& position)
{
    float screen[4];
    g_game->mainView->WorldToScreen(screen, position);
    return ape::Vec3{screen[0], screen[1], 0.0f, 0.0f};
}

// @func 0x004A58C0 ScreenToWorldPoint
ape::Vec3 ScreenToWorldPoint(const float screen[2], float distance)
{
    agl::AglRenderTarget* view = g_game->mainView;
    // The point under the screen position on a near plane one unit away...
    const float savedNear = view->zNear;
    view->SetNear(1.0f);
    float onPlane[3];
    view->ClipToCamera(onPlane, ape::Vec4{screen[0], screen[1], 0.0f, 0.0f});
    view->SetNear(savedNear);
    // ...gives the direction from the camera; scale it to `distance`.
    const ape::Vec3& eye = view->camera.translation;
    return ape::Vec3{eye.x + (onPlane[0] - eye.x) * distance, eye.y + (onPlane[1] - eye.y) * distance,
                     eye.z + (onPlane[2] - eye.z) * distance, 0.0f};
}

} // namespace sly
