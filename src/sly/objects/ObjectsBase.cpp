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

// Stand-in for 0x000C387C (entity module).
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
    : classBits(desc.classBits), typeBits(desc.typeBits)
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

// Stand-in for the placement base constructor (inlined into 0x00746148).
EntityPlacement::EntityPlacement(const char* placementName, EntityDef* ownerDef)
    : transform(ape::world::kWorldIdentity), def(ownerDef)
{
    if (placementName)
        ape::StrNCopy(name, placementName, sizeof(name) - 1);
}

// Stand-in for 0x000CDB28 (entity module): the transform and name.
// Stand-in for the placement base destructor (inlined into every placement
// destructor on the console).
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
