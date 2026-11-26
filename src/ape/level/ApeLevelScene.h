// openTIT - Level start-up helpers (camera settings, director scripts).
#pragma once

#include <cstdint>

namespace agl { class AglRenderTarget; }
namespace ape::world { class ApeScene; class ApeCameraInstance; }

namespace ape {

class ApeLevelDef;

// The world of the running level: its scene and two camera instances (the
// game keeps one; NOTE: the roles of the two cameras are not known, both
// start from the level's camera definition).
struct ApeLevelWorld {
    world::ApeScene* scene = nullptr;                 // +0x00
    world::ApeCameraInstance* cameraA = nullptr;      // +0x04
    world::ApeCameraInstance* cameraB = nullptr;      // +0x08

    ApeLevelWorld();
    // Creates the (empty) level scene.
    void CreateScene();
    // Loads a level definition into the scene: its included levels first
    // (recursively), its scene and director files, the cameras, then its
    // atmospheres and own objects. 'arg' is passed to the object set-up.
    void LoadLevel(const ApeLevelDef& level, void* arg);
    // Per-frame update of the world; with a view, camera B is applied to it.
    void Update(float dt, agl::AglRenderTarget* view);
    // Runs the scene's virtual slot 14 (render set-up on the console).
    void RenderScene();
    // Releases the cameras and destroys the scene.
    void Destroy();
};

void ApplyLevelCamera(const ApeLevelDef& level, agl::AglRenderTarget& target, bool force);
void LoadLevelDirectorFiles(const ApeLevelDef& level);
// Loads every scene file of the level into `scene`, each with its world
// offset. NOTE: the console passes the level's scene holder, whose first
// member is the scene.
void LoadLevelSceneFiles(world::ApeScene* scene, const ApeLevelDef& level);
// Ends the scene load of a level (FinishSceneLoad) and sets up its camera
// instances.
void FinishLevelScene(world::ApeScene* scene);

} // namespace ape
