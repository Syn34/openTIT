// openTIT - Inventory descriptions.
#include "sly/components/InventoryDescs.h"

#include "sly/objects/TriggerEnv.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace components {

namespace {

using ape::xml::XmlElement;

constexpr uint32_t kTag_InventoryGroup   = 0x084DA662u;   // NOTE: element name unknown
constexpr uint32_t kTag_EntitySlot       = 0x8C04DF57u;   // NOTE: element name unknown
constexpr uint32_t kTag_LootSlot         = 0x70ED7ED3u;   // NOTE: element name unknown
constexpr uint32_t kTag_Loot             = 0x0023BEA8u;   // "loot"
constexpr uint32_t kTag_InventoryItem    = 0x01698F69u;   // NOTE: element name unknown

constexpr uint32_t kAttr_GroupHash14     = 0x163F3E52u;   // NOTE: name unknown
constexpr uint32_t kAttr_GroupRangeAMin  = 0xF00D11C2u;   // NOTE: name unknown
constexpr uint32_t kAttr_GroupRangeAMax  = 0xF00D10D4u;   // NOTE: name unknown
constexpr uint32_t kAttr_GroupRangeBMin  = 0x115F0965u;   // NOTE: name unknown
constexpr uint32_t kAttr_GroupRangeBMax  = 0x115F0877u;   // NOTE: name unknown
constexpr uint32_t kAttr_GroupFlag28     = 0x8A511493u;   // NOTE: name unknown
constexpr uint32_t kAttr_EntityDefName   = 0x700F12ADu;   // "EntityDefName"
constexpr uint32_t kAttr_SlotRot         = 0x693D7F03u;   // NOTE: name unknown
constexpr uint32_t kAttr_SlotOffset      = 0xE5BD03C7u;   // NOTE: name unknown
constexpr uint32_t kAttr_SlotAttach      = 0x8A47BB3Cu;   // NOTE: name unknown
constexpr uint32_t kAttr_SlotCountMin    = 0xD458E73Eu;   // NOTE: name unknown
constexpr uint32_t kAttr_SlotCountMax    = 0xD458E650u;   // NOTE: name unknown
constexpr uint32_t kAttr_SlotFlag3C      = 0xC8F7AAD1u;   // NOTE: name unknown
constexpr uint32_t kAttr_LootChance      = 0xC8B6FC4Fu;   // NOTE: name unknown
constexpr uint32_t kAttr_Weight          = 0x98643678u;   // "weight"

constexpr uint32_t kAttr_InventoryGroupDefName = 0x429417EDu;   // "inventoryGroupDefName"
constexpr uint32_t kAttr_AttachedRot     = 0x53D63D93u;   // "AttachedRot" (hash match)
constexpr uint32_t kAttr_AttachedOffset  = 0x2AC4FB37u;   // "AttachedOffset" (hash match)
constexpr uint32_t kAttr_ItemHash20      = 0x016126A2u;   // NOTE: name unknown
constexpr uint32_t kAttr_ItemHash30      = 0x13163268u;   // NOTE: name unknown
constexpr uint32_t kAttr_ItemHash28      = 0x5F5A6881u;   // NOTE: name unknown
constexpr uint32_t kAttr_ItemHash2C      = 0xCDA698D9u;   // NOTE: name unknown
constexpr uint32_t kAttr_Priority        = 0xE92E69A4u;   // "priority"
constexpr uint32_t kAttr_ItemFlag38      = 0xFA32EEBFu;   // NOTE: name unknown
constexpr uint32_t kAttr_EnableInteract  = 0xB0A96CB9u;   // "EnableInteract" (hash match)

constexpr float kDegToRad = 0.017453292f;
const ape::Vec4 kIdentityRotation = {0.0f, 0.0f, 0.0f, 1.0f};   // 0x01164C90
const float kZero3[3] = {0.0f, 0.0f, 0.0f};                      // 0x01164B40

// Euler angles in degrees (missing = zero) as a quaternion.
ape::Vec4 ReadEulerRotation(XmlElement* elem, uint32_t attr)
{
    ape::Vec3 degrees = {0.0f, 0.0f, 0.0f, 0.0f};
    elem->GetVec3(attr, &degrees.x, kZero3);
    const ape::Vec3 radians = {degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f};
    return objects::QuaternionFromEuler(radians);
}

bool IsZero(const ape::Vec3& v) { return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f; }

bool IsIdentity(const ape::Vec4& q)
{
    return q.x == kIdentityRotation.x && q.y == kIdentityRotation.y && q.z == kIdentityRotation.z &&
           q.w == kIdentityRotation.w;
}

// Count ranges: a negative minimum and a maximum below the minimum are
// ignored (the previous values stay).
void ReadRangeMin(XmlElement* elem, uint32_t attr, int32_t& minValue, int32_t defaultValue)
{
    int32_t value = 0;
    elem->GetInt(attr, &value, defaultValue);
    if (value >= 0)
        minValue = value;
}

void ReadRangeMax(XmlElement* elem, uint32_t attr, int32_t minValue, int32_t& maxValue, int32_t defaultValue)
{
    int32_t value = 0;
    elem->GetInt(attr, &value, defaultValue);
    if (value >= minValue)
        maxValue = value;
}

// The attributes every slot kind shares; the slot then joins the global slot
// list and its group.
void ReadSlotCommon(XmlElement* elem, InventorySlotDesc* slot, InventoryGroupDesc* group)
{
    slot->rotation = ReadEulerRotation(elem, kAttr_SlotRot);
    elem->GetVec3(kAttr_SlotOffset, &slot->offset.x, &slot->offset.x);
    elem->GetHash(kAttr_SlotAttach, &slot->attachName, 0);
    ReadRangeMin(elem, kAttr_SlotCountMin, slot->countMin, 1);
    ReadRangeMax(elem, kAttr_SlotCountMax, slot->countMin, slot->countMax, slot->countMin);
    elem->GetBool(kAttr_SlotFlag3C, &slot->flag3C, slot->flag3C);
    InventorySlotDescs().PushBack(slot);
    group->slots.PushBack(slot);
}

InventorySlotDesc* ParseEntitySlot(XmlElement* elem)
{
    auto* slot = new InventoryEntitySlotDesc();
    elem->GetHash(kAttr_EntityDefName, &slot->entityDefName, 0);
    if (const char* name = elem->GetString("entityNameOverride"))
        ape::StrCopyBounded(slot->entityNameOverride, name, sizeof(slot->entityNameOverride));
    if (const char* name = elem->GetString("entityInPocketNameOverride"))
        ape::StrCopyBounded(slot->entityInPocketNameOverride, name, sizeof(slot->entityInPocketNameOverride));
    return slot;
}

InventorySlotDesc* ParseLootSlot(XmlElement* elem)
{
    auto* slot = new InventoryLootSlotDesc();
    elem->GetFloat(kAttr_LootChance, &slot->chance, 0.0f);
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != kTag_Loot || slot->entries.Count() >= InventoryLootSlotDesc::kMaxEntries)
            continue;
        InventoryLootEntry entry;
        entry.rotation = ReadEulerRotation(child, kAttr_SlotRot);
        entry.offset = {0.0f, 0.0f, 0.0f, 0.0f};
        child->GetVec3(kAttr_SlotOffset, &entry.offset.x, &entry.offset.x);
        child->GetHash(kAttr_SlotAttach, &entry.attachName, 0);
        child->GetFloat(kAttr_Weight, &entry.weight, 0.0f);
        child->GetHash(kAttr_EntityDefName, &entry.entityDefName, 0);
        slot->entries.PushBack(entry);
    }
    return slot;
}

} // namespace

ape::ApeTSortedMap<InventoryGroupDesc*>& InventoryGroupDescs()
{
    static ape::ApeTSortedMap<InventoryGroupDesc*> s_groups(0x10);
    return s_groups;
}

ape::ApeTSortedMap<InventoryItemDesc*>& InventoryItemDescs()
{
    static ape::ApeTSortedMap<InventoryItemDesc*> s_items(0x10);
    return s_items;
}

ape::ApeTArray<InventorySlotDesc*>& InventorySlotDescs()
{
    static ape::ApeTArray<InventorySlotDesc*> s_slots(0x10);
    return s_slots;
}

// @func 0x00264D70 InventoryLootSlotDesc::GetEntityDefName
uint32_t InventoryLootSlotDesc::GetEntityDefName(int entry) const
{
    return entry != -1 ? entries[entry].entityDefName : 0;
}

// @func 0x00264DA4 InventoryLootSlotDesc::GetAttachName
uint32_t InventoryLootSlotDesc::GetAttachName(int entry) const
{
    const uint32_t name = entry != -1 ? entries[entry].attachName : 0;
    return name != 0 ? name : attachName;
}

// @func 0x00264DE8 InventoryLootSlotDesc::GetOffset
ape::Vec3 InventoryLootSlotDesc::GetOffset(int entry) const
{
    ape::Vec3 result = {0.0f, 0.0f, 0.0f, 0.0f};
    if (entry != -1)
        result = entries[entry].offset;
    // An entry without its own offset uses the slot's.
    if (IsZero(result))
        result = offset;
    return result;
}

// @func 0x00264E98 InventoryLootSlotDesc::GetRotation
ape::Vec4 InventoryLootSlotDesc::GetRotation(int entry) const
{
    // NOTE: with no entry the console starts from an all-zero quaternion,
    // which is not the identity, so the result is zero rather than the slot's.
    ape::Vec4 result = {0.0f, 0.0f, 0.0f, 0.0f};
    if (entry != -1)
        result = entries[entry].rotation;
    if (IsIdentity(result))
        result = rotation;
    return result;
}

// @func 0x00264F58 InventoryLootSlotDesc::PickEntry
int InventoryLootSlotDesc::PickEntry() const
{
    if (ape::RandomUnit(ape::g_gameRandom) > chance)
        return -1;
    float total = 0.0f;
    for (const InventoryLootEntry& entry : entries)
        total += entry.weight;
    if (total <= 0.0f)
        return -1;
    float pick = total * ape::RandomUnit(ape::g_gameRandom);
    for (uint32_t i = 0; i < entries.Count(); ++i) {
        if (pick <= entries[i].weight)
            return int(i);
        pick -= entries[i].weight;
    }
    return -1;
}

// @func 0x002650D8 ParseInventoryGroupDesc
InventoryGroupDesc* ParseInventoryGroupDesc(XmlElement* root, XmlElement* elem)
{
    auto* group = new InventoryGroupDesc();
    const char* name = elem->GetString("name");
    if (const char* source = elem->GetString("cloneFrom")) {
        InventoryGroupDesc* original = *InventoryGroupDescs().Find(ape::HashNameNoCase(source));
        if (original && original != group) {
            for (InventorySlotDesc* slot : original->slots)
                group->slots.PushBack(slot);
            group->hash14 = original->hash14;
            group->rangeAMin = original->rangeAMin;
            group->rangeAMax = original->rangeAMax;
            group->rangeBMin = original->rangeBMin;
            group->rangeBMax = original->rangeBMax;
            group->flag28 = original->flag28;
        }
    }
    elem->GetHash(kAttr_GroupHash14, &group->hash14, group->hash14);
    ReadRangeMin(elem, kAttr_GroupRangeAMin, group->rangeAMin, group->rangeAMin);
    ReadRangeMax(elem, kAttr_GroupRangeAMax, group->rangeAMin, group->rangeAMax, group->rangeAMax);
    ReadRangeMin(elem, kAttr_GroupRangeBMin, group->rangeBMin, group->rangeBMin);
    ReadRangeMax(elem, kAttr_GroupRangeBMax, group->rangeBMin, group->rangeBMax, group->rangeBMax);
    elem->GetBool(kAttr_GroupFlag28, &group->flag28, group->flag28);

    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        InventorySlotDesc* slot = nullptr;
        if (child->GetNameHash() == kTag_EntitySlot)
            slot = ParseEntitySlot(child);
        else if (child->GetNameHash() == kTag_LootSlot)
            slot = ParseLootSlot(child);
        if (slot)
            ReadSlotCommon(child, slot, group);
        else
            (void)root->GetDocumentPath();   // compiled-out error report
    }
    if (name)
        InventoryGroupDescs().Add(ape::HashNameNoCase(name), group);
    return group;
}

// @func 0x00265D60 ParseInventoryItemDesc
InventoryItemDesc* ParseInventoryItemDesc(XmlElement* root, XmlElement* elem)
{
    (void)root;
    auto* item = new InventoryItemDesc();
    if (const char* source = elem->GetString("cloneFrom")) {
        InventoryItemDesc* original = *InventoryItemDescs().Find(ape::HashNameNoCase(source));
        if (original && original != item)
            *item = *original;
    }
    if (elem->GetString("inventoryGroupDefName")) {
        uint32_t groupName = 0;
        elem->GetHash(kAttr_InventoryGroupDefName, &groupName, 0);
        item->group = *InventoryGroupDescs().Find(groupName);
    }
    // The rotation is always rebuilt (a cloned one is replaced).
    item->attachedRot = ReadEulerRotation(elem, kAttr_AttachedRot);
    elem->GetVec3(kAttr_AttachedOffset, &item->attachedOffset.x, &item->attachedOffset.x);
    elem->GetHash(kAttr_ItemHash20, &item->hash20, item->hash20);
    elem->GetHash(kAttr_ItemHash30, &item->hash30, item->hash30);
    elem->GetHash(kAttr_ItemHash28, &item->hash28, item->hash28);
    elem->GetHash(kAttr_ItemHash2C, &item->hash2C, item->hash2C);
    elem->GetInt(kAttr_Priority, &item->priority, item->priority);
    elem->GetBool(kAttr_ItemFlag38, &item->flag38, item->flag38);
    if (const char* name = elem->GetString("name"))
        InventoryItemDescs().Add(ape::HashNameNoCase(name), item);
    return item;
}

// @func 0x00266570 InventoryComponentDesc::InsertItem
void InventoryComponentDesc::InsertItem(InventoryItemDesc* item)
{
    // Before the first item with a higher priority, else at the end.
    int position = -1;
    for (uint32_t i = 0; i < items.Count(); ++i) {
        if (item->priority < items[i]->priority) {
            position = int(i);
            break;
        }
    }
    items.PushBack(item);
    if (position < 0)
        return;
    for (int i = int(items.Count()) - 1; i > position; --i)
        items[i] = items[i - 1];
    items[position] = item;
}

// @func 0x002666C0 ParseInventoryComponentDesc
ComponentDesc* ParseInventoryComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    auto* desc = new InventoryComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != kTag_InventoryItem)
            continue;
        if (InventoryItemDesc* item = ParseInventoryItemDesc(root, child))
            desc->InsertItem(item);
    }
    elem->GetBool(kAttr_EnableInteract, &desc->enableInteract, desc->enableInteract);
    return desc;
}

// @func 0x00266C3C ParseInventoryGroupFile
bool ParseInventoryGroupFile(XmlElement* root, XmlElement* elem, bool result)
{
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() == kTag_InventoryGroup)
            ParseInventoryGroupDesc(root, child);
    }
    return result;
}

} // namespace components
} // namespace sly
