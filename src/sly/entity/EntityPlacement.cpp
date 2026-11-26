// openTIT - The placement ("settings") base and the instance description.
#include "sly/entity/EntityDef.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"

namespace sly {
namespace entity {

namespace {

// Appends every record of `from` to `*to` (created on the first one).
void AppendAttachments(const EntityAttachmentList& from, EntityAttachmentList*& to)
{
    for (const EntityAttachmentDesc& record : from) {
        if (!to)
            to = new EntityAttachmentList();
        to->Add(record);
    }
}

} // namespace

// The placement base constructor (inlined into every placement constructor,
// e.g. 0x000CC188, TriggerPlacement 0x00746148, and into the run-time spawns):
// the definition's defaults and a copy of its attachment records.
EntityPlacement::EntityPlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                                 const ape::Matrix34& placementTransform, const ape::Vec3& scale,
                                 bool bit31, bool bit30, uint32_t field74)
    : transform(placementTransform), field_60(scale), sourceObject(source), field_74(field74), def(ownerDef)
{
    if (def) {
        field_7C = def->field_24;
        field_80 = def->field_28;
    }
    bits84 = 0x1FE00000u | (bit31 ? 0x80000000u : 0u) | (bit30 ? 0x40000000u : 0u);
    if (def && def->HasFlag(EntityDef::kDefFlag_Desc5))
        bits84 |= 0x20000000u;
    if (placementName)
        ape::StrCopyBounded(name, placementName, sizeof(name));
    if (def && def->attachments)
        AppendAttachments(*def->attachments, attachments);
}

EntityPlacement::EntityPlacement(const char* placementName, EntityDef* ownerDef)
    : EntityPlacement(placementName, ownerDef, nullptr, ape::world::kWorldIdentity, ape::world::kWorldUnitScale, true,
                      false, 0)
{
}

// @func 0x00746F5C EntityPlacement::~EntityPlacement
// @func 0x00746FCC EntityPlacement::~EntityPlacement (deleting)
EntityPlacement::~EntityPlacement()
{
    delete attachments;
}

void EntityPlacement::AdoptDef(EntityDef* newDef)
{
    def = newDef;
    field_7C = newDef->field_24;
    field_80 = newDef->field_28;
    bits84 = (bits84 & ~0x20000000u) | (newDef->HasFlag(EntityDef::kDefFlag_Desc5) ? 0x20000000u : 0u);
    if (newDef->attachments && newDef->attachments->Count() != 0 && (!attachments || attachments->Count() == 0))
        AppendAttachments(*newDef->attachments, attachments);
}

// @func 0x000CDB28 EntityPlacement::FillInstanceDesc
void EntityPlacement::FillInstanceDesc(EntityInstanceDesc& desc) const
{
    desc.transform.xAxis = {transform.xAxis.x, transform.xAxis.y, transform.xAxis.z, desc.transform.xAxis.pad};
    desc.transform.yAxis = {transform.yAxis.x, transform.yAxis.y, transform.yAxis.z, desc.transform.yAxis.pad};
    desc.transform.zAxis = {transform.zAxis.x, transform.zAxis.y, transform.zAxis.z, desc.transform.zAxis.pad};
    desc.transform.translation = {transform.translation.x, transform.translation.y, transform.translation.z,
                                  desc.transform.translation.pad};
    desc.scale = {field_60.x, field_60.y, field_60.z, desc.scale.pad};
    desc.sourceObject = sourceObject;
    desc.field_80 = field_74;
    desc.def = def;
    // Placement bits 29-31 become description flags 11-13.
    constexpr uint32_t kMirrored = EntityInstanceDesc::kDesc_PlacementBit29 | EntityInstanceDesc::kDesc_PlacementBit30 |
                                   EntityInstanceDesc::kDesc_PlacementBit31;
    desc.flags = (desc.flags & ~kMirrored) | ((bits84 >> 18) & kMirrored);
    desc.player = uint8_t(bits84 >> 21);
    desc.field_84 = field_7C;
    desc.field_88 = field_80;
    if (field_88 != 0)
        desc.renderId = field_88;
    // The placement's attachment records replace the description's.
    if (attachments && attachments->Count() != 0) {
        if (desc.attachments)
            desc.attachments->Clear();
        AppendAttachments(*attachments, desc.attachments);
    }
}

// @func 0x000CDD28 EntityInstanceDesc::EntityInstanceDesc
// @func 0x000CDFB0 EntityInstanceDesc::EntityInstanceDesc (thunk)
EntityInstanceDesc::EntityInstanceDesc(float field84, float field88, uint32_t layerCount, uint32_t dataSize,
                                       const char* instanceName, EntityDef* ownerDef, ape::world::ApeObj* source,
                                       const void* field28, const ape::Matrix34& initialTransform,
                                       const ape::Vec3& initialScale, bool bit16, bool bit15, bool bit14,
                                       bool startVisible, bool bit12, bool bit9, bool bit8, bool notListed,
                                       uint32_t field80, uint8_t playerSlot, uint32_t field8C)
    : stateDataSize(dataSize), player(playerSlot), name(instanceName), def(ownerDef), sourceObject(source),
      field_28(field28), field_80(field80), field_84(ownerDef ? ownerDef->field_24 : field84),
      field_88(ownerDef ? ownerDef->field_28 : field88), renderId(field8C)
{
    if (ownerDef)
        bit14 = ownerDef->HasFlag(EntityDef::kDefFlag_Desc1);
    flags = (layerCount << kDesc_LayerShift) | (bit16 ? kDesc_Bit16 : 0u) | (bit15 ? kDesc_Bit15 : 0u) |
            (bit14 ? kDesc_Bit14 : 0u) | (startVisible ? kDesc_PlacementBit31 : 0u) |
            (bit12 ? kDesc_PlacementBit30 : 0u) | (bit9 ? kDesc_Bit9 : 0u) | (bit8 ? kDesc_Bit8 : 0u) |
            (notListed ? kDesc_NotListed : 0u);
    transform.xAxis = {initialTransform.xAxis.x, initialTransform.xAxis.y, initialTransform.xAxis.z, 0.0f};
    transform.yAxis = {initialTransform.yAxis.x, initialTransform.yAxis.y, initialTransform.yAxis.z, 0.0f};
    transform.zAxis = {initialTransform.zAxis.x, initialTransform.zAxis.y, initialTransform.zAxis.z, 0.0f};
    transform.translation = {initialTransform.translation.x, initialTransform.translation.y,
                             initialTransform.translation.z, 1.0f};
    scale = {initialScale.x, initialScale.y, initialScale.z, 0.0f};
    if (ownerDef && ownerDef->attachments && ownerDef->attachments->Count() != 0)
        AppendAttachments(*ownerDef->attachments, attachments);
}

// NOTE: the console's triggers pass field 0x28 a global of the entity
// module (0x00B12000); its role is unknown, the port passes null.
EntityInstanceDesc::EntityInstanceDesc(const char* instanceName, EntityDef* ownerDef)
    : EntityInstanceDesc(-1.0f, -1.0f, 1, 0, instanceName, ownerDef, nullptr, nullptr, ape::world::kWorldIdentity,
                         ape::world::kWorldUnitScale, false, false, true, false, false, true, false, false, 0, 0xFF,
                         0)
{
}

EntityInstanceDesc::~EntityInstanceDesc()
{
    delete attachments;
}

} // namespace entity
} // namespace sly
