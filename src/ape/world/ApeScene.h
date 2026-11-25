// openTIT - Scenes: the containers a level's scene files are loaded into.
//
// A scene owns the objects created while its files are parsed (the loaded
// object list), an object table, an optional string pool and, for the main
// scene, the level's region set. Up to eight scenes are registered in a
// global slot table.
//
// Only construction, destruction and creation are ported; the loading and
// query virtuals of the scene classes (console vtables 0x00B24728 and
// 0x00B24770, 0x00586440-0x0059AF54) are not.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape {
class ApeStringPool;
}

namespace ape::world {

enum ApeSceneCreateFlags : uint32_t {
    kSceneCreate_OwnsRegions = 0x01,   // creates (and later releases) the level region set
    kSceneCreate_02          = 0x02,   // NOTE: clears scene flag 0x20; meaning unknown
    kSceneCreate_04          = 0x04,   // NOTE: sets scene flag 0x10; meaning unknown
};

class ApeScene {
public:
    static constexpr int kMaxScenes = 8;

    explicit ApeScene(uint32_t createFlags);
    virtual ~ApeScene();

    ApeScene(const ApeScene&) = delete;
    ApeScene& operator=(const ApeScene&) = delete;

    bool OwnsRegions() const { return (m_flags & kFlag_OwnsRegions) != 0; }
    uint8_t GetSlot() const { return m_slot; }
    ApeTArray<void*>& LoadedObjects() { return m_loadedObjects; }

    enum Flags : uint8_t {
        kFlag_10          = 0x10,   // NOTE: from create flag 0x04
        kFlag_20          = 0x20,   // NOTE: set unless create flag 0x02
        kFlag_OwnsRegions = 0x40,
        kFlag_Loading     = 0x80,   // set by CreateScene
    };

protected:
    friend ApeScene* CreateScene(uint32_t createFlags);

    ApeTArray<void*> m_objects;         // 0x24 (capacity 0x4000)
    uint8_t m_slot = 0;                 // 0x38 1-based index in the scene table, 0 for none
    ApeStringPool* m_strings = nullptr; // 0x3C
    uint8_t m_flags = 0;                // 0x40
    uint8_t m_byte41 = 0xFF;            // 0x41 NOTE: meaning unknown
    ApeTArray<void*> m_loadedObjects;   // 0x44
};

// The level scene class (console vtable 0x00B24770). Its query virtuals are
// not ported.
class ApeLevelScene : public ApeScene {
public:
    using ApeScene::ApeScene;
    // Adds nothing to the scene destructor (the console has its own copy).
    // @func 0x007A9BDC ApeLevelScene::~ApeLevelScene
    // @func 0x007A9D48 ApeLevelScene::~ApeLevelScene (deleting)
    ~ApeLevelScene() override = default;
};

// Creates a level scene and prepares the global load state for parsing its
// files into it.
ApeScene* CreateScene(uint32_t createFlags);
// Registered scene in slot `index` (0..7), or null.
ApeScene* GetScene(int index);

} // namespace ape::world
