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
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeWorldHooks.h"
#include "ape/core/ApeSystem.h"
#include "ape/core/ApeString.h"

#include <cmath>

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

namespace {

// Camera definition a level's cameras start from. NOTE: name unknown.
constexpr uint32_t kLevelCameraDef = 0xFCCCF4CCu;
constexpr float kDefaultCameraFov = 55.0f;

// Light distance settings of a level: its Light element, else the engine
// configuration (fade 300/50 when the configuration leaves them at zero).
void ApplyLevelLightDistances(const ApeLevelDef& level)
{
    using namespace world;
    if (level.hasLight) {
        g_defaultLightFadeDist = level.defaultFadeDist;
        g_defaultLightFadeRange = level.defaultFadeRange;
        g_lightNoRescanDist = level.noRescanDist;
        g_lightNoUpdateDist = level.noUpdateDist;
    } else {
        const EngineConfig& config = g_engineConfig;
        g_defaultLightFadeDist = std::fabs(config.field_088) < 1e-5f ? 300.0f : config.field_088;
        g_defaultLightFadeRange = std::fabs(config.field_08C) < 1e-5f ? 50.0f : config.field_08C;
        g_lightNoRescanDist = config.lightNoRescanDist;
        g_lightNoUpdateDist = config.lightNoUpdateDist;
    }
    g_lightNoRescanDistSq = g_lightNoRescanDist * g_lightNoRescanDist;
    g_lightNoUpdateDistSq = g_lightNoUpdateDist * g_lightNoUpdateDist;
}

// The level's cameras look down -z from (0, 0, 7) when the level has no
// camera definition.
Matrix34 DefaultCameraTransform()
{
    const Vec3 eye = { 0.0f, 0.0f, 7.0f, 0.0f };
    const Vec3 target = { 0.0f, 0.0f, 0.0f, 0.0f };
    const Vec3 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    Matrix34 m{};
    m.zAxis = { eye.x - target.x, eye.y - target.y, eye.z - target.z, 0.0f };
    Vec3Normalize(m.zAxis);
    Vec3Cross(m.xAxis, up, m.zAxis);
    Vec3Normalize(m.xAxis);
    Vec3Cross(m.yAxis, m.zAxis, m.xAxis);
    m.translation = eye;
    return m;
}

// TODO(0x004F05A8 LevelWorld::CreateLevelObjects) creates the level's
// objects (entities and their geometry) from its definition.
void CreateLevelObjects(ApeLevelWorld& world, const ApeLevelDef& level, void* arg, bool first, bool own)
{
    (void)world; (void)level; (void)arg; (void)first; (void)own;
}
// TODO(0x004F0000 LevelWorld::CloneGeomTemplates) clones the geometry
// definitions of the level's GeomTemplates with their texture overrides.
void CloneGeomTemplates(ApeLevelWorld& world, const ApeLevelDef& level)
{
    (void)world; (void)level;
}
// TODO(0x004F23C4 LevelWorld::SetupInstancePasses) gives geometry instances
// with a reflection tag their render pass (0x005AC7AC).
void SetupInstancePasses(ApeLevelWorld& world, const ApeLevelDef& level, void* arg)
{
    (void)world; (void)level; (void)arg;
}
// TODO(0x005A5864 ClearCameraInstanceList) and TODO(0x005AC64C
// ClearRenderPasses): the camera list (0x01193088) and the instance render
// pass table (0x01194ED8) of the world module.
void ClearWorldPassTables() {}

} // namespace

// @func 0x004EFEF4 ApeLevelWorld::ApeLevelWorld
ApeLevelWorld::ApeLevelWorld() = default;

// @func 0x004EFFC8 ApeLevelWorld::CreateScene
void ApeLevelWorld::CreateScene()
{
    scene = world::CreateScene(world::kSceneCreate_OwnsRegions);
}

// @func 0x004F2678 ApeLevelWorld::LoadLevel
void ApeLevelWorld::LoadLevel(const ApeLevelDef& level, void* arg)
{
    if (!scene)
        return;
    ApplyLevelLightDistances(level);
    for (uint32_t included : level.includedLevels) {
        if (const ApeLevelDef* sub = FindLevelDef(included))
            LoadLevel(*sub, arg);
    }
    for (const LevelSceneFile& file : level.sceneFiles) {
        world::g_levelInstanceOffset = file.worldOffset;
        world::LoadSceneFile(scene, file.name.c_str(), false);
    }
    for (const std::string& path : level.directorFiles)
        ApeAnimScriptLibrary::Get(0)->LoadFile(path.c_str());

    if (!cameraB)
        cameraB = world::CreateCameraInstance(nullptr);
    if (!cameraA)
        cameraA = world::CreateCameraInstance(nullptr);
    world::ApeCameraDef* def = world::ApeTDefList<world::ApeCameraDef>::Find(kLevelCameraDef);
    if (!def) {
        // Any camera definition will do.
        auto& pool = ApeTListAllocator<world::ApeCameraDef>::GetAllocator();
        def = static_cast<world::ApeCameraDef*>(pool.NodeData(pool.First()));
    }
    if (!def) {
        const Matrix34 transform = DefaultCameraTransform();
        for (world::ApeCameraInstance* camera : { cameraB, cameraA }) {
            camera->SetLocal(transform);
            camera->MarkDirty();
        }
        cameraB->m_fov = kDefaultCameraFov;
        cameraA->m_fov = kDefaultCameraFov;
    } else {
        cameraB->SetDef(def);
        cameraA->SetDef(def);
    }

    SetupInstancePasses(*this, level, arg);
    // Atmospheres are levels of their own; the first one (or the level
    // itself when it has none) is created as the primary one.
    for (size_t i = 0; i < level.atmospherics.size(); ++i) {
        if (const ApeLevelDef* atmosphere = FindLevelDef(HashNameNoCase(level.atmospherics[i].c_str())))
            CreateLevelObjects(*this, *atmosphere, arg, i == 0, false);
    }
    CreateLevelObjects(*this, level, arg, level.atmospherics.empty(), true);
    CloneGeomTemplates(*this, level);
}

// @func 0x004F303C ApeLevelWorld::Update
void ApeLevelWorld::Update(float dt, agl::AglRenderTarget* view)
{
    if (view && cameraB)
        cameraB->ApplyToView(*view);
    world::UpdateWorld(dt);
}

// @func 0x004F2FF4 ApeLevelWorld::RenderScene
void ApeLevelWorld::RenderScene()
{
    if (scene)
        scene->VirtualSlot14();
}

// @func 0x004EFF08 ApeLevelWorld::Destroy
void ApeLevelWorld::Destroy()
{
    ClearWorldPassTables();
    if (cameraB) {
        cameraB->Release();
        cameraB = nullptr;
    }
    if (cameraA) {
        cameraA->Release();
        cameraA = nullptr;
    }
    if (scene) {
        world::DestroyScene(scene);
        scene = nullptr;
    }
}

} // namespace ape
