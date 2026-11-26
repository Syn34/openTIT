// openTIT - Renderer services used by the world object layer.
//
// Meshes, light linking and particle systems belong to the renderer (Agl),
// which is ported separately. The world code reaches them through these
// hooks; every hook may be null, in which case the world code skips the
// call (meshes are then never found, nothing is drawn).
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world {

struct AglMesh;          // renderer mesh (reference counted)
struct AglLightLink;     // per-object light selection state

struct ApeWorldRenderHooks {
    // Mesh lookup by name hash (0x004FED94).
    AglMesh* (*findMesh)(uint32_t nameHash) = nullptr;
    // Placeholder mesh used when a mesh is missing (0x00526300).
    AglMesh* (*getDefaultMesh)() = nullptr;
    // Gives a mesh the name of the definition that uses it (0x0050A14C).
    void (*setMeshOwnerName)(AglMesh* mesh, const char* name) = nullptr;
    // Releases a mesh reference (mesh virtual slot 2).
    void (*releaseMesh)(AglMesh* mesh) = nullptr;
    // Marks a mesh as shadow caster (0x0050A214 with arguments 1, 0, 0).
    void (*enableMeshShadows)(AglMesh* mesh) = nullptr;
    // Duplicates a mesh for a cloned definition (0x005095C0).
    AglMesh* (*cloneMesh)(AglMesh* mesh) = nullptr;
    // Queues a mesh for drawing with the given draw parameters (0x005099B8).
    void (*drawMesh)(AglMesh* mesh, void* drawParams) = nullptr;
    // Propagates a value to the mesh (0x0050A19C). NOTE: purpose unknown.
    void (*meshSlot6)(AglMesh* mesh, int value) = nullptr;
    // Number of drawable parts of a mesh and the flags byte of a part's
    // material (renderer mesh +0x34, material +0x64).
    uint16_t (*getMeshPartCount)(AglMesh* mesh) = nullptr;
    uint8_t (*getMeshPartMaterialFlags)(AglMesh* mesh, int part) = nullptr;
    // Creates / frees the light linking state of a render object
    // (0x00526494 / 0x0050CAE8).
    AglLightLink* (*createLightLink)() = nullptr;
    void (*freeLightLink)(AglLightLink* link) = nullptr;
};

extern ApeWorldRenderHooks g_worldRenderHooks;

// Position used for level of detail distances (the active render camera,
// 0x010DD8B0 on the console). The renderer keeps it up to date.
extern Vec3 g_lodCameraPosition;

// Engine configuration defaults read by the definition loaders
// (EngineConfig 0x010C5A58, 0x010C5AB0, 0x010C5AB4 on the console).
extern float g_defaultMinNearFadeAlpha;
extern float g_defaultLightFadeDist;
extern float g_defaultLightFadeRange;
// Light relinking distances of the running level (EngineConfig 0x010C5A98..
// 0x010C5AA4 on the console: distances and their squares), set when a level
// is loaded.
extern float g_lightNoRescanDist;
extern float g_lightNoUpdateDist;
extern float g_lightNoRescanDistSq;
extern float g_lightNoUpdateDistSq;

} // namespace ape::world
