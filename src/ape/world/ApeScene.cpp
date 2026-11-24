// openTIT - Scenes: construction, destruction and creation.
#include "ape/world/ApeScene.h"

#include "agl/AglTexture.h"
#include "ape/sys/ApeIff.h"
#include "ape/sys/ApeStringPool.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRegions.h"

namespace ape::world {

namespace {
ApeScene* s_scenes[ApeScene::kMaxScenes] = {};   // 0x01193060
uint8_t s_sceneCount = 0;                         // 0x01193080
// NOTE: two load-state flags CreateScene sets (0x0119300E = 1,
// 0x010DDC90 = 0); their readers are not ported.
bool s_sceneLoadFlag = false;
bool s_renderLoadFlag = false;
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
    ApeScene* scene = new ApeLevelScene(createFlags);
    g_dirtyRootListLocked = true;
    scene->m_flags |= ApeScene::kFlag_Loading;
    s_sceneLoadFlag = true;
    s_renderLoadFlag = false;
    (void)s_sceneLoadFlag;
    (void)s_renderLoadFlag;
    agl::ClearTexturePaths();
    iff::BeginLoadedObjectList(&scene->m_loadedObjects);
    g_levelInstanceOffset = kWorldZero;
    return scene;
}

ApeScene* GetScene(int index)
{
    return (index >= 0 && index < ApeScene::kMaxScenes) ? s_scenes[index] : nullptr;
}

} // namespace ape::world
