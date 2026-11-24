// openTIT - Definition locators: named attachment points loaded with a
// geometry definition ("numLocators" chunk and one record per locator).
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeSkeleton.h"

#include <cstring>

namespace ape::world {

ApeLocatorSet* ApeLocatorSet::Create(uint32_t capacity)
{
    ApeLocatorSet* set = static_cast<ApeLocatorSet*>(MemAlloc(sizeof(ApeLocatorSet)));
    if (!set)
        return nullptr;
    set->refCount = 1;
    set->locators = nullptr;
    set->count = 0;

    // The array carries its capacity in a 16-byte header.
    uint8_t* block = static_cast<uint8_t*>(MemAlloc(16 + capacity * sizeof(ApeLocator)));
    if (block) {
        *reinterpret_cast<uint32_t*>(block + 12) = capacity;
        set->locators = reinterpret_cast<ApeLocator*>(block + 16);
        for (uint32_t i = 0; i < capacity; ++i) {
            ApeLocator& locator = set->locators[i];
            Matrix34Copy(locator.transform, kWorldIdentity);
            locator.nameHash = 0;
            locator.bone = -1;
        }
    }
    return set;
}

void ReleaseLocatorSet(ApeLocatorSet* set)
{
    if (!set || --set->refCount >= 1)
        return;
    if (set->locators) {
        MemFree(reinterpret_cast<uint8_t*>(set->locators) - 16);
        set->locators = nullptr;
    }
    MemFree(set);
}

// @func 0x005848CC ApeLocatorSet::Find
ApeLocator* ApeLocatorSet::Find(uint32_t nameHash)
{
    for (uint32_t i = 0; i < count; ++i)
        if (locators[i].nameHash == nameHash)
            return &locators[i];
    return nullptr;
}

// Locator list header: replaces the definition's locator set with an empty
// one of the announced size.
// @func 0x005A9478 LoadLocatorListChunk
int LoadLocatorListChunk(iff::IffReader* reader, void* arg)
{
    ApeDef* def = static_cast<ApeDef*>(arg);
    uint8_t version = 0;
    uint16_t numLocators = 0;
    reader->ReadU8(&version);
    reader->ReadU16(&numLocators);

    ApeLocatorSet* set = ApeLocatorSet::Create(numLocators);
    if (!set || !set->locators) {
        ReleaseLocatorSet(set);
        return 0;
    }
    if (def->m_locators != set) {
        ApeLocatorSet* old = def->m_locators;
        def->m_locators = set;
        ++set->refCount;
        ReleaseLocatorSet(old);
    }
    ReleaseLocatorSet(set);
    return 1;
}

// One locator: orientation, position and optional bone of the geometry
// definition's skeleton (by hash).
// @func 0x005A96E8 LoadLocatorChunk
int LoadLocatorChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomDef* def = static_cast<ApeGeomDef*>(arg);
    uint8_t version = 0;
    uint32_t nameHash = 0, boneHash = 0;
    float quat[4] = {}, position[3] = {};
    char name[32] = {}, boneName[32] = {};
    reader->ReadU8(&version);
    reader->ReadU32(&nameHash);
    reader->ReadVec4(quat);
    reader->ReadVec3(position);
    reader->ReadU32(&boneHash);
    reader->ReadRaw32(name);
    reader->ReadRaw32(boneName);

    ApeLocatorSet* set = def->m_locators;
    ApeLocator* locator = &set->locators[set->count++];
    locator->nameHash = nameHash;
    const Quat q = {quat[0], quat[1], quat[2], quat[3]};
    Matrix34FromQuat(locator->transform, q, kWorldZero);
    locator->transform.translation = {position[0], position[1], position[2], locator->transform.translation.pad};

    if (boneHash != 0 && def->m_skeletonDef)
        locator->bone = int16_t(def->m_skeletonDef->FindBone(boneHash));
    return 1;
}

} // namespace ape::world
