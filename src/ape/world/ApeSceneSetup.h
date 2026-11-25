// openTIT - Scene set-up: the passes FinishSceneLoad runs over the objects of
// a freshly loaded scene (world transforms, region masks, the light grid,
// the global lights, dynamic objects) and their counterparts used while the
// game runs.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world {

class ApeObj;
class ApeScene;

// Global light colours taken from the level's ambient and directional
// light instances (0x010C5AD0 ambient, 0x010C5AE0 directional colour,
// 0x010C5AF0 direction; on the console these live in the engine
// configuration block).
struct ApeSceneLighting {
    Vec3 ambient = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec3 directionalColor = {1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 directionalDir = {1.0f, 0.0f, 0.0f, 0.0f};
};
extern ApeSceneLighting g_sceneLighting;

// Bounds reset by every scene load (0x01194530); NOTE: its readers are not
// ported.
extern Aabb g_sceneObjectBounds;

// Brings world transforms up to date. With `allInstances` every dirty root
// geometry, light and emitter instance is updated; otherwise the queued
// dirty roots are processed and the queue is emptied.
void UpdateWorldTransforms(bool allInstances);
// Updates every root instance's world transform and the region masks of
// the renderable ones (end of a scene load).
void UpdateAllInstanceWorlds();
// Fits the light grid around the level's local lights.
void ComputeLightGrid();
// Takes the global ambient and directional light from the light instances.
void UpdateSceneLighting();
// Empties g_sceneObjectBounds.
void ResetSceneObjectBounds();
// Puts dynamic geometry/emitter instances (and with `recurse` their
// children) into the dynamic object list.
void AddToDynamicObjects(ApeObj* obj, bool recurse);
// AddToDynamicObjects for every emitter and geometry instance.
void CollectDynamicObjects();

// Per-frame procedural animation update (phase as for agl::EndFrame).
void UpdateProcAnims(int phase);
// Procedural animation manager and light list of the level (0x01194EC4,
// 0x01194ED0).
void CreateProcAnimManager();
void DestroyProcAnimManager();
void ReleaseLightList();
// Light link refresh (0x00583518 for all lights, 0x005832FC per object).
void UpdateLightLinks();
void UpdateObjectLightLink(ApeObj* obj);

// Passes of the scene load not ported yet (see ApeSceneSetup.cpp).
void SetupSceneRenderNodes(ApeScene* scene);
void BuildInstanceBatches();
void SetupLightList();
void HideEmptyGeometry();
void SetupObjectLighting();
void SetupSceneViews();
void FinishSceneBounds(ApeScene* scene);
void ReleaseInstanceBatches();

} // namespace ape::world
