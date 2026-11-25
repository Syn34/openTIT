// openTIT - Start-up of the world object module (pools, name maps, level
// chunk handlers of the definition and instance types, regions, quads,
// locators).
#pragma once

namespace ape::world {

// Sets up every world object type; called once at engine start.
bool InitWorld();

// Per-type parts of InitWorld.
void InitGeomDefs();
void InitLightDefs();
void InitEmitterDefs();
void InitCameraDefs();
void InitSplineDefs();
void InitPointDefs();
void InitGeomInstances();
void InitLightInstances();
void InitEmitterInstances();
void InitCameraInstances();
void InitPointInstances();
void InitSplineInstances();

} // namespace ape::world
