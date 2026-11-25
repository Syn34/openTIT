// openTIT - A list of entity references that drops dead entities.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace sly {
namespace entity {

class Entity;

// Base of the reference lists (vtable 0x00B11DC4): a "has entries" flag
// and two update hooks.
struct EntityRefListBase {
    // @func 0x007438B0 EntityRefListBase::~EntityRefListBase
    // @func 0x007438C0 EntityRefListBase::~EntityRefListBase (deleting)
    virtual ~EntityRefListBase() = default;
    // Slot 2: empty. NOTE: purpose unknown.
    // @func 0x00743904 EntityRefListBase::VirtualSlot2
    virtual void VirtualSlot2() {}
    // Slot 3: forgets that the list has entries.
    // @func 0x00743908 EntityRefListBase::MarkEmpty
    virtual void MarkEmpty() { nonEmpty = false; }
    // Slot 4: drops stale entries and the entity named `name`.
    // @func 0x00743918 EntityRefListBase::Purge
    virtual void Purge(uint32_t name) { (void)name; }

    bool nonEmpty = false;                     // 0x08 bit 31
};

// NOTE: owner class unknown (code at 0x000B4524/0x000B566C, between the
// loading screen tuning and the mine laser tutorial). Its slot 3
// (0x000B4414) is not ported.
struct EntityRefList : EntityRefListBase {
    // The console class is polymorphic (vtable 0x00B11DE0); the destructor
    // releases the entry storage.
    // @func 0x0074391C EntityRefList::~EntityRefList
    // @func 0x00743984 EntityRefList::~EntityRefList (deleting)
    ~EntityRefList() override = default;

    ape::ApeTArray<Entity*> entries;           // 0x0C
    void* field_E4 = nullptr;                  // 0xE4

    // Removes entities that are no longer alive and the
    // entity named `name` (null entries stay); updates nonEmpty.
    void Purge(uint32_t name) override;
    void Reset();
};

} // namespace entity
} // namespace sly
