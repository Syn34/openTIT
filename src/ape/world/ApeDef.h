// openTIT - World object definitions (ApeDef) and the per-type definition lists.
//
// A definition holds what every placed copy of an object shares: bounds, name,
// animation hash and type specific data (meshes, light parameters, camera
// fov, ...). Definitions are created by the level loader from IFF chunks,
// pooled per type (ApeTListAllocator<T>) and registered by name hash in a
// sorted map so instances can find them (FindDef() in ApeObj.h).
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTFreeList.h"
#include "ape/world/ApeBase.h"

#include <cstdint>
#include <new>

namespace ape::iff {
struct IffReader;
}

namespace ape::world {

// Named attachment points of a definition (definition offset 0x6C), shared
// by reference count between copies of the definition. See ApeLocators.cpp.
struct alignas(16) ApeLocator {
    Matrix34 transform;     // 0x00 relative to the object (or to the bone)
    uint32_t nameHash;      // 0x40
    int16_t bone;           // 0x44 bone of the definition's skeleton, -1 for none
    uint8_t pad[10];
};

struct ApeLocatorSet {
    int16_t refCount;
    ApeLocator* locators;   // allocated with a 16-byte header holding the capacity
    uint16_t count;

    // Set with room for `capacity` locators (identity, no bone) and one
    // reference; null when out of memory.
    static ApeLocatorSet* Create(uint32_t capacity);
    ApeLocator* Find(uint32_t nameHash);
};
// Drops one reference and frees the set (and its locators) when unused.
void ReleaseLocatorSet(ApeLocatorSet* set);

// Base of all definitions (vtable 0x00B246B0), 0x70 bytes on the console.
class ApeDef : public ApeBase {
public:
    explicit ApeDef(uint32_t type);
    ~ApeDef() override;

    // Definitions are reference counted; the last Release() destroys them
    // through the renderer's deferred destruction queue.
    void Release() override = 0;

    // @func 0x007A9A64 ApeDef::GetName
    virtual const char* GetName() const { return m_name; }
    // Copies the shared definition fields (bounds, name, hashes, locators).
    virtual void CopyFrom(const ApeDef& other);
    // NOTE: slots 5 and 6 of the console vtable are pure in the base and
    // implemented by every definition type; their purpose is not known for
    // all of them (the emitter definition uses slot 5 for a distance check
    // before spawning, see ApeEmitterDef).
    virtual int VirtualSlot5(void* arg) { return 0; }
    virtual void VirtualSlot6(int arg) {}

    void SetName(const char* name, bool updateHash);

    // Puts the shared fields back to their defaults (inlined into every
    // definition constructor on the console).
    void ResetBase();

    const Aabb& GetBounds() const { return m_bounds; }
    const Sphere& GetSphere() const { return m_sphere; }
    uint32_t GetNameHash() const { return m_nameHash; }
    uint32_t GetAnimHash() const { return m_animHash; }

    Aabb m_bounds;                      // 0x10
    Sphere m_sphere;                    // 0x30 local bounding sphere
    char m_name[32];                    // 0x40
    uint32_t m_nameHash = 0;            // 0x60
    uint32_t m_animHash = 0;            // 0x64
    uint32_t field_68 = 0;              // NOTE: written by CopyFrom only
    ApeLocatorSet* m_locators = nullptr; // 0x6C
};

// Callback run by the loaders around the parsing of a definition's chunks
// (phase 0 before, phase 1 after).
using DefLoadCallback = void (*)(ApeDef* def, void* arg, int phase);

// Name hash -> definition map of one definition type (kObjType_GeomDef ..
// kObjType_SplineDef).
ApeTSortedMap<ApeDef*>& GetDefMap(uint32_t defType);

// Per-type list of definitions: the element pool, the optional load callback
// and the name map (0x00F8F12C.. on the console, one static block per type).
// T must provide `static constexpr uint32_t kDefType`.
template <class T>
class ApeTDefList {
public:
    static ApeTSortedMap<ApeDef*>& Map() { return GetDefMap(T::kDefType); }

    static void SetLoadCallback(DefLoadCallback callback, void* arg)
    {
        State().callback = callback;
        State().callbackArg = arg;
    }

    static void RunLoadCallback(T* def, int phase)
    {
        if (State().callback)
            State().callback(def, State().callbackArg, phase);
    }

    // Constructs a definition in pooled storage.
    static T* Create()
    {
        void* storage = ApeTListAllocator<T>::GetAllocator().Alloc();
        return storage ? new (storage) T() : nullptr;
    }

    // Adds a loaded definition to the name map.
    static void Register(T* def)
    {
        Map().Add(def->m_nameHash, def);
        def->SetRegistered(true);
    }

    // Removes a definition from the name map (if it is the registered one),
    // destroys it and returns its storage to the pool.
    // @func 0x0059BD24 ApeTDefList<ApeGeomDef>::Destroy
    // @func 0x0059C5FC ApeTDefList<ApeLightDef>::Destroy
    // @func 0x0059D1A8 ApeTDefList<ApeCameraDef>::Destroy
    // @func 0x0059E7FC ApeTDefList<ApePointDef>::Destroy
    // @func 0x0059FE4C ApeTDefList<ApeSplineDef>::Destroy
    static void Destroy(T* def)
    {
        if (def->IsRegistered()) {
            if (*Map().Find(def->m_nameHash) == def)
                Map().Remove(def->m_nameHash);
            def->SetRegistered(false);
        }
        def->~T();
        ApeTListAllocator<T>::GetAllocator().Free(def);
    }
    static void DestroyErased(void* def) { Destroy(static_cast<T*>(def)); }

    static T* Find(uint32_t nameHash) { return static_cast<T*>(*Map().Find(nameHash)); }

private:
    struct StateBlock {
        DefLoadCallback callback = nullptr;
        void* callbackArg = nullptr;
    };
    static StateBlock& State()
    {
        static StateBlock s_state;
        return s_state;
    }
};

// Release() of definitions goes through the renderer's deferred destruction
// queue (0x005082F4) so that render data still in flight is not freed. Until
// the renderer installs its queue the object is destroyed immediately.
using DeferredDestroyFn = void (*)(void* object, void (*destroy)(void*));
extern DeferredDestroyFn g_deferredDestroy;
void DeferDestroy(void* object, void (*destroy)(void*));

// Generic handler for container chunks: parses the nested chunks with no
// object (0x0059AF58, 0x0059B25C).
int ParseNestedChunks(iff::IffReader* reader, void* arg);

} // namespace ape::world
