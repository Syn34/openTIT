// openTIT - Scenes: construction, destruction and creation.
#include "ape/world/ApeScene.h"

#include "agl/AglDeferred.h"
#include "agl/AglRenderThread.h"
#include "agl/AglResources.h"
#include "agl/AglTexture.h"
#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeTFreeList.h"
#include "ape/sys/ApeIff.h"
#include "ape/sys/ApeStringPool.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRegions.h"
#include "ape/world/ApeSceneSetup.h"
#include "ape/world/ApeTags.h"

namespace ape::world {

namespace {
ApeScene* s_scenes[ApeScene::kMaxScenes] = {};   // 0x01193060
uint8_t s_sceneCount = 0;                         // 0x01193080
// Set while a scene is being created/loaded (0x0119300E). NOTE: its
// readers are not ported.
bool s_sceneLoading = false;
// Set while DestroyScene releases objects (0x0116BD05). NOTE: its readers
// are not ported.
bool s_sceneDestroying = false;

// Objects destroyed while a scene still owns them leave its object list
// (0x00588F44, run from the ApeBase destructor).
// @func 0x00588F44 RemoveDestroyedSceneObject
void RemoveDestroyedSceneObject(ApeBase* object)
{
    const uint8_t slot = object->GetSceneSlot();
    ApeScene* scene = slot ? s_scenes[slot - 1] : nullptr;
    if (!scene)
        return;
    ApeTArray<ApeSceneObject>& objects = scene->Objects();
    for (int i = int(objects.Count()) - 1; i >= 0; --i) {
        if (objects[size_t(i)].object == object) {
            objects.RemoveAtSwap(size_t(i));
            return;
        }
    }
}

// Release functions stored with the scene's objects. With `clearSlot` the
// object forgets its scene first, so that its destruction does not try to
// remove it from the scene again.
void ReleaseBaseObject(void* object, bool clearSlot)
{
    ApeBase* base = static_cast<ApeBase*>(object);
    if (clearSlot)
        base->SetSceneSlot(0);
    base->Release();
}
void ReleaseTexture(void* object, bool clearSlot)
{
    agl::AglTexture* texture = static_cast<agl::AglTexture*>(object);
    if (clearSlot)
        texture->sceneSlot = 0;
    texture->Release();
}
void ReleaseMaterial(void* object, bool clearSlot)
{
    agl::AglMaterial* material = static_cast<agl::AglMaterial*>(object);
    if (clearSlot)
        material->sceneSlot = 0;
    material->Release();
}

// Load callback installed in every object list while a scene file is
// parsed: phase 0 (object created) marks the object with the scene's slot,
// phase 1 (object loaded) also adds it to the scene's object list.
void OnSceneObjectLoaded(ApeScene* scene, void* object, uint8_t& slotField,
                         void (*release)(void*, bool), int phase)
{
    if (phase == 1)
        scene->Objects().PushBack(ApeSceneObject{object, release});
    if (phase == 0 || phase == 1)
        slotField = uint8_t(scene->GetSlot() & 0x0F);
}

// @func 0x00588E70 OnSceneObjectLoaded
void OnSceneBaseLoaded(ApeBase* object, void* scene, int phase)
{
    uint8_t slot = object->GetSceneSlot();
    OnSceneObjectLoaded(static_cast<ApeScene*>(scene), object, slot, &ReleaseBaseObject, phase);
    object->SetSceneSlot(slot);
}
void OnSceneDefLoaded(ApeDef* def, void* scene, int phase)
{
    OnSceneBaseLoaded(def, scene, phase);
}
void OnSceneInstanceLoaded(ApeObj* obj, void* scene, int phase)
{
    OnSceneBaseLoaded(obj, scene, phase);
}
void OnSceneTextureLoaded(agl::AglTexture* texture, void* scene, int phase)
{
    OnSceneObjectLoaded(static_cast<ApeScene*>(scene), texture, texture->sceneSlot, &ReleaseTexture, phase);
}
void OnSceneMaterialLoaded(agl::AglMaterial* material, void* scene, int phase)
{
    OnSceneObjectLoaded(static_cast<ApeScene*>(scene), material, material->sceneSlot, &ReleaseMaterial, phase);
}

// Installs (scene != null) or removes the load callbacks of every object
// list. NOTE: the console clears every list's callback except the spline
// definitions' afterwards; here all are cleared.
void SetSceneLoadCallbacks(ApeScene* scene)
{
    agl::SetTextureCreateHook(scene ? &OnSceneTextureLoaded : nullptr, scene);
    agl::SetMaterialCreateHook(scene ? &OnSceneMaterialLoaded : nullptr, scene);
    const DefLoadCallback defCallback = scene ? &OnSceneDefLoaded : nullptr;
    ApeTDefList<ApeLightDef>::SetLoadCallback(defCallback, scene);
    ApeTDefList<ApeGeomDef>::SetLoadCallback(defCallback, scene);
    ApeTDefList<ApeCameraDef>::SetLoadCallback(defCallback, scene);
    ApeTDefList<ApeEmitterDef>::SetLoadCallback(defCallback, scene);
    ApeTDefList<ApePointDef>::SetLoadCallback(defCallback, scene);
    ApeTDefList<ApeSplineDef>::SetLoadCallback(defCallback, scene);
    void (*instanceCallback)(ApeObj*, void*, int) = scene ? &OnSceneInstanceLoaded : nullptr;
    ApeTInstanceList<ApeEmitterInstance>::SetLoadCallback(instanceCallback, scene);
    ApeTInstanceList<ApeLightInstance>::SetLoadCallback(instanceCallback, scene);
    ApeTInstanceList<ApeGeomInstance>::SetLoadCallback(instanceCallback, scene);
    ApeTInstanceList<ApePointInstance>::SetLoadCallback(instanceCallback, scene);
    ApeTInstanceList<ApeSplineInstance>::SetLoadCallback(instanceCallback, scene);
    // Camera instances are not tracked by scenes.
}
} // namespace

// @func 0x00586440 ApeScene::ApeScene
ApeScene::ApeScene(uint32_t createFlags)
{
    m_objects.Init(0x4000, nullptr, false);
    for (int i = 0; i < kMaxScenes; ++i) {
        if (!s_scenes[i]) {
            s_scenes[i] = this;
            m_slot = uint8_t(i + 1);
            break;
        }
    }
    if (createFlags & kSceneCreate_04)
        m_flags |= kFlag_10;
    if (!(createFlags & kSceneCreate_02))
        m_flags |= kFlag_20;
    if (createFlags & kSceneCreate_OwnsRegions) {
        m_flags |= kFlag_OwnsRegions;
        if (!g_regionSet)
            g_regionSet = new ApeRegionSet();
    }
    ++s_sceneCount;
}

// @func 0x005865D8 ApeScene::~ApeScene
// @func 0x00586744 ApeScene::~ApeScene (deleting)
ApeScene::~ApeScene()
{
    delete m_strings;
    m_strings = nullptr;
    if (m_slot)
        s_scenes[m_slot - 1] = nullptr;
    --s_sceneCount;
    if (OwnsRegions() && g_regionSet) {
        // The global pointer is only cleared when this was the last reference.
        const bool last = g_regionSet->GetRefCount() == 1;
        g_regionSet->Release();
        if (last)
            g_regionSet = nullptr;
    }
}

// @func 0x005903F4 CreateScene
ApeScene* CreateScene(uint32_t createFlags)
{
    // NOTE: the console installs this hook in the world module's start-up
    // (0x00588828, not ported).
    g_sceneObjectDestroyedHook = &RemoveDestroyedSceneObject;

    ApeScene* scene = new ApeLevelScene(createFlags);
    g_dirtyRootListLocked = true;
    scene->m_flags |= ApeScene::kFlag_Loading;
    s_sceneLoading = true;
    // Resources released while loading are destroyed at once.
    agl::SetDeferredDestructionEnabled(false);
    agl::ClearTexturePaths();
    iff::BeginLoadedObjectList(&scene->m_loadedObjects);
    g_levelInstanceOffset = kWorldZero;
    return scene;
}

ApeScene* GetScene(int index)
{
    return (index >= 0 && index < ApeScene::kMaxScenes) ? s_scenes[index] : nullptr;
}

// @func 0x005904EC LoadSceneFile
bool LoadSceneFile(ApeScene* scene, const char* path, bool flag)
{
    StrCopyBounded(scene->m_name, path, sizeof(scene->m_name));

    // Textures are looked for next to the scene: the part of the path in
    // front of "levels/".
    char folder[256];
    StrCopyBounded(folder, path, sizeof(folder));
    StrLower(folder);
    if (char* levels = StrStr(folder, "levels/")) {
        *levels = '\0';
        agl::SetTextureDirectory(folder);
    }

    SetSceneLoadCallbacks(scene);
    iff::ClearLoadedObjectSlots();
    // A scene with its own regions loads them as a new group.
    if (scene->OwnsRegions() && g_regionSet)
        g_regionSet->groupBase = g_regionSet->count;

    bool opened = false;
    if (file::File* file = file::OpenFile(path, "rb")) {
        iff::IffReader reader;
        reader.Init();
        if (reader.Open(file, flag ? 1 : 0)) {
            reader.ParseChunks(nullptr);
            reader.Close();
        }
        file->Close();
        opened = true;
        reader.Destroy();
    }
    SetSceneLoadCallbacks(nullptr);
    return opened;
}

// @func 0x005964B4 FinishSceneLoad
void FinishSceneLoad(ApeScene* scene)
{
    s_sceneDestroying = false;
    scene->m_flags &= uint8_t(~ApeScene::kFlag_Loading);
    iff::BeginLoadedObjectList(nullptr);
    UpdateWorldTransforms(true);
    if (scene->m_flags & ApeScene::kFlag_20) {
        SetupSceneRenderNodes(scene);
        BuildInstanceBatches();
        if (!(scene->m_flags & ApeScene::kFlag_10)) {
            CreateProcAnimManager();
            SetupLightList();
        }
        HideEmptyGeometry();
        CollectDynamicObjects();
        UpdateAllInstanceWorlds();
        ComputeLightGrid();
        ResetSceneObjectBounds();
        SetupObjectLighting();
        UpdateSceneLighting();
    }
    SetupSceneViews();
    FinishSceneBounds(scene);
    s_sceneLoading = false;
    g_dirtyRootListLocked = false;
    agl::SetDeferredDestructionEnabled(true);
}

// TODO(0x0056B57C ResetDebugDrawBuffers) swaps the debug draw buffers twice.
static void ResetDebugDrawBuffers()
{
}

// @func 0x00596E3C DestroyScene
void DestroyScene(ApeScene* scene)
{
    // Nothing may still be drawing what is about to be freed.
    agl::RenderThreadWait();
    UpdateProcAnims(2);
    agl::EndFrame(2);
    agl::FlushRendering();

    s_sceneDestroying = true;
    DestroyProcAnimManager();
    ReleaseLightList();
    ReleaseInstanceBatches();
    ReleaseCachedTags();

    // Release the owned objects, newest first.
    ApeTArray<ApeSceneObject>& objects = scene->m_objects;
    while (objects.Count() != 0) {
        const ApeSceneObject entry = objects[objects.Count() - 1];
        objects.RemoveAtSwap(objects.Count() - 1);
        entry.release(entry.object, true);
    }
    agl::FlushRendering();

    // Data blocks the scene's chunks created.
    ApeTArray<void*>& loaded = scene->m_loadedObjects;
    while (loaded.Count() != 0) {
        // NOTE: every loaded object is assumed to derive from ApeLoadedData
        // (the console calls its virtual destructor).
        delete static_cast<iff::ApeLoadedData*>(loaded[loaded.Count() - 1]);
        loaded.RemoveAtSwap(loaded.Count() - 1);
    }

    delete scene;
    ResetDebugDrawBuffers();
    ApeFreeList::CompactAll();
    s_sceneDestroying = false;
    (void)s_sceneLoading;
    (void)s_sceneDestroying;
}

} // namespace ape::world
