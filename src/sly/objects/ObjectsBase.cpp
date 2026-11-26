// openTIT - Stand-ins for the entity framework that the world object classes build on.
//
// None of these functions belong to this directory's address range; they are the
// smallest behaviour the derived classes need until the entity module can be used
// directly (see NOTES.md). They are deliberately untagged.
#include "sly/objects/ObjectsBase.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"

#include <cstdio>
#include <cstring>

namespace sly {
namespace objects {

// @func 0x000C387C InheritDefDesc
void InheritDefDesc(const EntityDefDesc& from, EntityDefDesc& to)
{
    for (int i = 0; i < 8; ++i)
        to.flags[i] = from.flags[i];
    to.field_20 = from.field_20;
    to.field_24 = from.field_24;
    to.field_28 = from.field_28;
    to.field_2C = from.field_2C;
    to.field_30 = from.field_30;
    to.field_34 = from.field_34;
    to.field_38 = from.field_38;
    to.field_3C = from.field_3C;
    to.tagList = from.tagList;
    to.field_48 = from.field_48;
}

// Stand-in for 0x000C26D8 (entity module): only the identity fields.
EntityDef::EntityDef(const EntityDefDesc& desc)
    : classBits(desc.classBits), typeBits(desc.typeBits), field_24(desc.field_24), field_28(desc.field_28)
{
    geomNameHash = ape::HashNameNoCase(desc.geomName);
    field_30 = ape::HashNameNoCase(desc.field_14);
    ape::StrNCopy(name, desc.name ? desc.name : "", sizeof(name) - 1);
    nameHash = ape::HashNameNoCase(name);
}

// Stand-in for 0x000C2DA0 (entity module).
EntityDef::~EntityDef() = default;

// Stand-in for 0x000CBA88 (entity module).
const char* EntityDef::GetInstanceName(EntityPlacement* placement)
{
    if (placement && placement->name[0] != '\0')
        return placement->name;
    ++instanceCounter;
    return ape::TempFormat("%s_inst%05d", name, (int)instanceCounter);
}

// Stand-in for 0x000CDD28 (entity module).
EntityInstanceDesc::EntityInstanceDesc(const char* instanceName, EntityDef* ownerDef)
    : name(instanceName), def(ownerDef), transform(ape::world::kWorldIdentity)
{
}

// The placement base constructor (entity module; inlined into every
// placement constructor, e.g. TriggerPlacement 0x00746148): identity
// transform, the definition's defaults and a copy of its attachment records.
EntityPlacement::EntityPlacement(const char* placementName, EntityDef* ownerDef, uint32_t field70,
                                 const ape::Matrix34& placementTransform, const ape::Vec3& offset,
                                 bool bit31, bool bit30, uint32_t field74)
    : transform(placementTransform), field_60(offset), field_70(field70), field_74(field74), def(ownerDef)
{
    if (def) {
        field_7C = def->field_24;
        field_80 = def->field_28;
    }
    bits84 = 0x1FE00000u | (bit31 ? 0x80000000u : 0u) | (bit30 ? 0x40000000u : 0u);
    if (def && (def->options & 0x02))
        bits84 |= 0x20000000u;
    if (placementName)
        ape::StrNCopy(name, placementName, sizeof(name) - 1);
    if (def && def->attachments) {
        for (const DefAttachment& attachment : *def->attachments) {
            if (!ownedArray)
                ownedArray = new ape::ApeTArray<DefAttachment>();
            ownedArray->Add(attachment);
        }
    }
}

EntityPlacement::EntityPlacement(const char* placementName, EntityDef* ownerDef)
    : EntityPlacement(placementName, ownerDef, 0, ape::world::kWorldIdentity, ape::Vec3{}, true, false, 0)
{
}

// Stand-in for 0x000CDB28 (entity module): the transform and name.
// Also inlined into every placement destructor on the console.
// @func 0x00746F5C EntityPlacement::~EntityPlacement
// @func 0x00746FCC EntityPlacement::~EntityPlacement (deleting)
EntityPlacement::~EntityPlacement()
{
    if (ownedArray) {
        ownedArray->Release();
        delete ownedArray;
    }
}

void EntityPlacement::FillInstanceDesc(EntityInstanceDesc& desc) const
{
    desc.transform = transform;
    desc.field_84 = field_7C;
    desc.field_88 = field_80;
}

ObjEntity::ObjEntity(const EntityInstanceDesc& desc)
    : transform(desc.transform), def(desc.def)
{
    if (desc.name)
        ape::StrNCopy(name, desc.name, sizeof(name) - 1);
    id = ape::HashNameNoCase(name);
    flags |= kEntityAlive;
}

ObjEntity::~ObjEntity() = default;

void ObjEntity::Release(bool immediate)
{
    (void)immediate;
    if (refCount > 0)
        --refCount;
}

void ObjEntity::SetState(int layer, int value)
{
    if (layer >= 0 && layer < 4)
        state[layer] = value;
}

int ObjEntity::GetState(int layer) const
{
    return (layer >= 0 && layer < 4) ? state[layer] : 0;
}

ape::Aabb ObjEntity::GetBounds(bool world) const
{
    ape::Aabb box;
    ape::AabbFromPoint(box, world ? Position() : ape::world::kWorldZero);
    return box;
}

} // namespace objects
} // namespace sly
