// openTIT - Game helpers for world objects (0x004A5854-0x004A5C20,
// 0x004AFCB4, 0x004B00EC): material lookup on geometry, render LOD and
// name search over an object hierarchy.
#pragma once

#include <cstdint>

namespace ape::world { class ApeObj; }
namespace agl { class AglMaterial; }

namespace sly {

// Material with name hash 'materialHash' used by any LOD of a geometry
// instance (searched in LOD then part order), or null.
agl::AglMaterial* FindGeomMaterial(ape::world::ApeObj* geom, uint32_t materialHash);
// Same, also searching the geometry instances below 'object' depth first.
agl::AglMaterial* FindMaterialInHierarchy(ape::world::ApeObj* object, uint32_t materialHash);
// Every material of the object's geometry passes the depth test (depth
// function "always"), e.g. for objects drawn on top of the scene.
void SetGeomDepthAlways(ape::world::ApeObj* geom);
// Render LOD of a render object and all of its descendants.
void SetHierarchyLod(ape::world::ApeObj* object, uint32_t lod);
// The object itself or its first descendant named 'nameHash' (objects
// without a cached hash are compared by their hashed name), or null.
ape::world::ApeObj* FindInHierarchy(ape::world::ApeObj* object, uint32_t nameHash);

} // namespace sly
