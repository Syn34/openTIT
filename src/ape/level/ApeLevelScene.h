// openTIT - Level start-up helpers (camera settings, director scripts).
#pragma once

namespace agl { class AglRenderTarget; }
namespace ape::world { class ApeScene; }

namespace ape {

class ApeLevelDef;

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
