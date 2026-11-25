// openTIT - Inventory descriptions.
//
// Inventories are described in two places:
// * the inventory group file: <InventoryGroup ...> elements (tag 0x084DA662,
//   parsed by 0x00266C3C), each a list of slots. A slot is either a single
//   entity (tag 0x8C04DF57: entity def name plus display name overrides) or a
//   loot table (<loot> tag 0x70ED7ED3 with up to ten weighted entries).
//   Groups and slots can copy another group with "cloneFrom".
// * the <Inventory> component of an entity (type 0x12): a list of items (tag
//   0x01698F69), each referring to a group by "inventoryGroupDefName" and
//   carrying where the item hangs on the owner. Items are kept sorted by
//   priority.
//
// Most attribute names are not among the executable's strings; their hashes
// are kept with a NOTE.
#pragma once

#include "sly/entity/Entity.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

using entity::ComponentDesc;

constexpr uint32_t kCompDesc_Inventory = 0x12;

// One slot of an inventory group (vtable 0x00B1BE78; abstract, slot 3 is pure).
// The per-entry accessors take the entry index of slots with several
// entries (loot tables); -1 means "no entry" and gives the slot's own values.
class InventorySlotDesc {
public:
    InventorySlotDesc() = default;
    virtual ~InventorySlotDesc();                                               // slots 0/1
    // Slot 2: true for loot tables (several weighted entries).
    // NOTE: name assumed from the only override.
    virtual bool IsLootTable() const;
    // Slot 3: entity definition the entry spawns.
    virtual uint32_t GetEntityDefName(int entry) const = 0;
    // Slot 4: attach bone/part of an entry (falls back to the slot's).
    virtual uint32_t GetAttachName(int entry) const;
    // Slots 5/6: placement of an entry (falls back to the slot's).
    virtual ape::Vec3 GetOffset(int entry) const;
    virtual ape::Vec4 GetRotation(int entry) const;
    // Slot 7: name the spawned entity takes while in a pocket (null: keep).
    virtual const char* GetInPocketNameOverride() const;

    // 0x04..0x0C NOTE: not touched by the parser.
    ape::Vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};    // 0x10 from Euler degrees (attribute 0x693D7F03)
    ape::Vec3 offset = {0.0f, 0.0f, 0.0f, 0.0f};      // 0x20 attribute 0xE5BD03C7
    uint32_t attachName = 0;     // 0x30 attribute 0x8A47BB3C. NOTE: name unknown, role assumed (bone/part)
    int32_t countMin = 1;        // 0x34 attribute 0xD458E73E (negative values ignored)
    int32_t countMax = 1;        // 0x38 attribute 0xD458E650 (kept only when >= countMin)
    bool flag3C = false;         // 0x3C attribute 0xC8F7AAD1. NOTE: name unknown
};

// A slot holding one entity (vtable 0x00B1BEA0, 0xD0 bytes).
class InventoryEntitySlotDesc : public InventorySlotDesc {
public:
    ~InventoryEntitySlotDesc() override;
    uint32_t GetEntityDefName(int entry) const override;
    const char* GetInPocketNameOverride() const override;

    char entityNameOverride[0x40] = {};          // 0x3D
    uint32_t entityDefName = 0;                  // 0x80 "EntityDefName"
    char entityInPocketNameOverride[0x40] = {};  // 0x84
};

// One weighted entry of a loot table (0x30 bytes).
struct InventoryLootEntry {
    ape::Vec4 rotation;          // 0x00 identity when not given
    ape::Vec3 offset;            // 0x10 zero when not given
    uint32_t attachName;         // 0x20 0 = use the slot's
    float weight;                // 0x24 "weight"
    uint32_t entityDefName;      // 0x28 "EntityDefName"
};

// A slot filled from a loot table (vtable 0x00B1BEC8, 0x60 bytes).
class InventoryLootSlotDesc : public InventorySlotDesc {
public:
    InventoryLootSlotDesc() : entries(kMaxEntries) {}
    ~InventoryLootSlotDesc() override;

    bool IsLootTable() const override;
    uint32_t GetEntityDefName(int entry) const override;
    uint32_t GetAttachName(int entry) const override;
    ape::Vec3 GetOffset(int entry) const override;
    ape::Vec4 GetRotation(int entry) const override;
    // Rolls the table: -1 when nothing drops (chance failed or no weight),
    // else the index of the entry picked by weight.
    int PickEntry() const;

    static constexpr uint32_t kMaxEntries = 10;
    ape::ApeTArray<InventoryLootEntry> entries;   // 0x40
    float chance = 0.5f;         // 0x54 attribute 0xC8B6FC4F, probability that anything drops
};

// <InventoryGroup> (0x2C bytes).
struct InventoryGroupDesc {
    InventoryGroupDesc() : slots(4) {}

    ape::ApeTArray<InventorySlotDesc*> slots;     // 0x00
    uint32_t hash14 = 0;         // 0x14 attribute 0x163F3E52. NOTE: name unknown
    int32_t rangeAMin = 0;       // 0x18 attribute 0xF00D11C2 ("...Min", negative values ignored). NOTE: name unknown
    int32_t rangeAMax = 0;       // 0x1C attribute 0xF00D10D4 ("...Max", kept only when >= min)
    int32_t rangeBMin = 0;       // 0x20 attribute 0x115F0965 ("...Min"). NOTE: name unknown
    int32_t rangeBMax = 0;       // 0x24 attribute 0x115F0877 ("...Max")
    bool flag28 = false;         // 0x28 attribute 0x8A511493. NOTE: name unknown
};

// One <Inventory> item (0x40 bytes).
struct InventoryItemDesc {
    ape::Vec4 attachedRot = {0.0f, 0.0f, 0.0f, 1.0f};   // 0x00 from "AttachedRot" (Euler degrees)
    ape::Vec3 attachedOffset = {0.0f, 0.0f, 0.0f, 0.0f};  // 0x10 "AttachedOffset"
    uint32_t hash20 = 0;         // 0x20 attribute 0x016126A2. NOTE: name unknown
    InventoryGroupDesc* group = nullptr;   // 0x24 "inventoryGroupDefName"
    uint32_t hash28 = 0;         // 0x28 attribute 0x5F5A6881. NOTE: name unknown
    uint32_t hash2C = 0;         // 0x2C attribute 0xCDA698D9. NOTE: name unknown
    uint32_t hash30 = 0;         // 0x30 attribute 0x13163268. NOTE: name unknown
    int32_t priority = 0;        // 0x34 "priority"
    bool flag38 = false;         // 0x38 attribute 0xFA32EEBF. NOTE: name unknown
};

// <Inventory> (vtable 0x00B1BEF0, 0x2C bytes).
struct InventoryComponentDesc : ComponentDesc {
    ~InventoryComponentDesc() override;
    explicit InventoryComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Inventory), items(4) {}
    // Inserts keeping the items ordered by priority (stable for equal ones).
    void InsertItem(InventoryItemDesc* item);

    ape::ApeTArray<InventoryItemDesc*> items;     // 0x14
    // 0x28 bit 31: "EnableInteract" (NOTE: name from a hash match).
    bool enableInteract = true;
};

// Registries: groups and items by name hash, and every slot ever parsed.
ape::ApeTSortedMap<InventoryGroupDesc*>& InventoryGroupDescs();   // 0x0105BE18
ape::ApeTSortedMap<InventoryItemDesc*>& InventoryItemDescs();     // 0x0105BE30
ape::ApeTArray<InventorySlotDesc*>& InventorySlotDescs();         // 0x0105BE04

InventoryGroupDesc* ParseInventoryGroupDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
InventoryItemDesc* ParseInventoryItemDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
bool ParseInventoryGroupFile(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, bool result);
ComponentDesc* ParseInventoryComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

} // namespace components
} // namespace sly
