// openTIT - Entity definitions: registry, spawning, deferred events.
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeBounds.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "ape/memory/ApeMem.h"
#include "sly/anim/AnimStream.h"

#include <vector>

namespace sly {
namespace entity {

namespace {

// Definitions by name hash (0x01013B04).
ape::ApeTSortedMap<EntityDef*>& DefMap()
{
    static ape::ApeTSortedMap<EntityDef*> s_defs(0, nullptr, nullptr);
    return s_defs;
}

// Placements by name hash (0x01013B1C).
ape::ApeTSortedMap<EntityPlacement*>& PlacementMap()
{
    static ape::ApeTSortedMap<EntityPlacement*> s_placements(0, nullptr, nullptr);
    return s_placements;
}

// 0x01013B84 (12-byte entries, peak count kept by the array).
std::vector<DeferredEntityEvent> g_deferred;

} // namespace

void RegisterEntityPlacement(uint32_t nameHash, EntityPlacement* placement)
{
    PlacementMap().Add(nameHash, placement);
}

EntityPlacement* FindEntityPlacement(uint32_t nameHash)
{
    return nameHash ? *PlacementMap().Find(nameHash) : nullptr;
}

void RegisterEntityDef(EntityDef* def)
{
    DefMap().Add(def->nameHash, def);
}

void UnregisterEntityDef(EntityDef* def)
{
    DefMap().Remove(def->nameHash);
}

// @func 0x000C387C InheritDefDesc
void InheritDefDesc(const EntityDefDesc& from, EntityDefDesc& to)
{
    for (int i = 0; i < 8; ++i)
        to.flags[i] = from.flags[i];
    to.field_20 = from.field_20;
    to.field_24 = from.field_24;
    to.field_28 = from.field_28;
    to.loadScope = from.loadScope;
    to.propertyType = from.propertyType;
    to.field_34 = from.field_34;
    to.havokSkeleton = from.havokSkeleton;
    to.field_3C = from.field_3C;
    to.componentNames = from.componentNames;
    to.boundingRadius = from.boundingRadius;
}

// @func 0x000C26D8 EntityDef::EntityDef
// @func 0x000C2B90 EntityDef::EntityDef (copy 2)
EntityDef::EntityDef(const EntityDefDesc& desc)
    : classBits(desc.classBits), typeBits(desc.typeBits), registryFlags(desc.registryFlags),
      field_24(desc.field_24), field_28(desc.field_28), field_78(desc.field_34),
      havokSkeleton(desc.havokSkeleton), field_84(desc.field_3C)
{
    for (int i = 0; i < 8; ++i)
        if (desc.flags[i])
            flags |= 1u << (31 - i);
    if (desc.field_20 & 1)
        flags |= kDefFlag_Field20;

    geomNameHash = ape::HashNameNoCase(desc.geomName);
    templateNameHash = ape::HashNameNoCase(desc.templateName);
    ape::StrNCopy(name, desc.name, sizeof(name) - 1);
    nameHash = ape::HashNameNoCase(name);

    // The geometry definition; without one the definition is modelled on a
    // placed object (a geometry instance or, failing that, a spline).
    if (geomNameHash != 0)
        geomDef = ape::world::FindDef(geomNameHash, 0);
    if (!geomDef && templateNameHash != 0) {
        templateObject = ape::world::FindInstance(templateNameHash, 0);
        if (!templateObject)
            templateObject = ape::world::ApeTInstanceList<ape::world::ApeSplineInstance>::Find(templateNameHash);
    }

    PropertySystemRegisterDef(this, desc.propertyType);

    // Component templates by name (unknown names are skipped).
    if (desc.componentNames && *desc.componentNames) {
        for (uint32_t templateName : **desc.componentNames) {
            ComponentDesc* component = FindComponentTemplate(templateName);
            if (!component)
                continue;
            if (!componentDescs)
                componentDescs = new ape::ApeTArray<ComponentDesc*>(4);
            componentDescs->PushBack(component);
        }
    }

    // Map the bones of the geometry's skeleton to the Havok skeleton.
    const ape::world::ApeSkeletonDef* skeleton = nullptr;
    if (geomDef && geomDef->GetType() == ape::world::kObjType_GeomDef)
        skeleton = static_cast<ape::world::ApeGeomDef*>(geomDef)->m_skeletonDef;
    if (!skeleton && templateObject && templateObject->GetType() == ape::world::kObjType_GeomInstance)
        skeleton = static_cast<ape::world::ApeGeomDef*>(templateObject->GetDef())->m_skeletonDef;
    if (skeleton)
        boneMap = anim::BuildHavokBoneMap(havokSkeleton, skeleton);

    RegisterEntityDef(this);

    if (desc.boundingRadius > 0.0f && templateObject && templateObject->GetDef()->GetSphere().w != 0.0f)
        templateObject->SetBoundingRadius(desc.boundingRadius);
}

// @func 0x000C2BB0 EntityDef::~EntityDef
// @func 0x000C2DA0 EntityDef::~EntityDef (copy 2)
// @func 0x000C2C9C EntityDef::~EntityDef (deleting)
EntityDef::~EntityDef()
{
    if (boneMap) {
        ape::MemFree(boneMap);
        boneMap = nullptr;
    }
    PropertySystemForgetDef(this);
    UnregisterEntityDef(this);
    delete attachments;
    delete componentDescs;
}

// @func 0x000CB934 EntityDef::GetBounds
void EntityDef::GetBounds(ape::Aabb& out) const
{
    const ape::world::ApeObj* object = templateObject;
    if (!object) {
        ape::AabbFromPoint(out, ape::world::kWorldZero);
        return;
    }
    out = object->GetDef()->GetBounds();
    ape::AabbScale(out, object->GetScale());
}

void EntityDefBaseBounds(const EntityDef* def, ape::Aabb& out)
{
    def->EntityDef::GetBounds(out);
}

// @func 0x000C9FC8 FindEntityDef
EntityDef* FindEntityDef(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    return *DefMap().Find(nameHash);
}

// @func 0x000CBA40 EntityDef::MakeInstanceName
const char* EntityDef::MakeInstanceName()
{
    return ape::TempFormat("%s_inst%05d", name, ++instanceCounter);
}

// @func 0x000CBA88 EntityDef::InstanceName
const char* EntityDef::InstanceName(const EntityPlacement* placement)
{
    if (placement->name[0] != '\0')
        return placement->name;
    if (placement->sourceObject)
        return placement->sourceObject->GetName();
    return MakeInstanceName();
}

// @func 0x000CB888 EntityDef::FindComponentDesc
ComponentDesc* EntityDef::FindComponentDesc(uint32_t type, ComponentDesc* after) const
{
    if (!componentDescs)
        return nullptr;
    size_t start = 0;
    if (after) {
        for (size_t i = 0; i < componentDescs->Count(); ++i) {
            if ((*componentDescs)[i] == after) {
                start = i + 1;
                break;
            }
        }
    }
    for (size_t i = start; i < componentDescs->Count(); ++i)
        if ((*componentDescs)[i]->type == type)
            return (*componentDescs)[i];
    return nullptr;
}

// @func 0x000D6CC0 SpawnFromDef
Entity* SpawnFromDef(EntityDef* def, EntityPlacement& placement)
{
    return def ? def->Spawn(&placement) : nullptr;
}

// @func 0x000D6D28 SpawnEntity
Entity* SpawnEntity(uint32_t defName, EntityPlacement& placement)
{
    EntityDef* def = FindEntityDef(defName);
    if (!def)
        return nullptr;
    placement.AdoptDef(def);
    return def->Spawn(&placement);
}

// @func 0x000D6ED0 QueueDeferredEvent
void QueueDeferredEvent(void* target, uint32_t event, uintptr_t data)
{
    g_deferred.push_back({ target, event, data });
}

uint32_t DeferredEventCount() { return uint32_t(g_deferred.size()); }
DeferredEntityEvent* DeferredEvents() { return g_deferred.data(); }
void ClearDeferredEvents() { g_deferred.clear(); }

} // namespace entity
} // namespace sly
