// openTIT - Level start-up helpers: applying a level definition's camera
// settings and loading its director scripts.
#include "ape/level/ApeLevelScene.h"

#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeScene.h"
#include "ape/world/ApeSceneSetup.h"

namespace ape {

// Copies the level's camera settings (field of view, clip planes, clear
// colour) to the render target, when the level has a Camera element or when
// `force` is set.
// @func 0x004F0394 ApplyLevelCamera
void ApplyLevelCamera(const ApeLevelDef& level, agl::AglRenderTarget& target, bool force)
{
    if (!force && !level.hasCamera)
        return;
    target.SetFov(level.cameraFov);
    target.SetNear(level.cameraNearZ);
    target.SetFar(level.cameraFarZ);
    agl::AglRenderSurface* surface = target.surface;
    surface->clearColor[0] = level.cameraColor.r;
    surface->clearColor[1] = level.cameraColor.g;
    surface->clearColor[2] = level.cameraColor.b;
    surface->clearColor[3] = level.cameraColor.a;
}

// Loads every DirectorFile of the level into the default script library.
// @func 0x004F0448 LoadLevelDirectorFiles
void LoadLevelDirectorFiles(const ApeLevelDef& level)
{
    for (const std::string& path : level.directorFiles)
        ApeAnimScriptLibrary::Get(0)->LoadFile(path.c_str());
}

// @func 0x004F04D4 LoadLevelSceneFiles
void LoadLevelSceneFiles(world::ApeScene* scene, const ApeLevelDef& level)
{
    for (const LevelSceneFile& file : level.sceneFiles) {
        // Root instances of the file are moved by its offset.
        world::g_levelInstanceOffset = file.worldOffset;
        world::LoadSceneFile(scene, file.name.c_str(), false);
    }
}

// @func 0x004F2FC0 FinishLevelScene
void FinishLevelScene(world::ApeScene* scene)
{
    world::FinishSceneLoad(scene);
    world::SetupCameraInstances();
}

} // namespace ape
