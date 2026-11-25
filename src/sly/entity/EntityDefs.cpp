// openTIT - Entity definitions: registry, spawning, deferred events.
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeBounds.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"

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

// 0x01013B84 (12-byte entries, peak count kept by the array).
std::vector<DeferredEntityEvent> g_deferred;

} // namespace

void RegisterEntityDef(EntityDef* def)
{
    DefMap().Add(def->nameHash, def);
}

void UnregisterEntityDef(EntityDef* def)
{
    DefMap().Remove(def->nameHash);
}

// @func 0x000C2BB0 EntityDef::~EntityDef
// @func 0x000C2DA0 EntityDef::~EntityDef (copy 2)
// @func 0x000C2C9C EntityDef::~EntityDef (deleting)
EntityDef::~EntityDef()
{
    delete[] field_80;
    field_80 = nullptr;
    PropertySystemForgetDef(this);
    UnregisterEntityDef(this);
    delete attachments;
    delete componentDescs;
}

// @func 0x000CB934 EntityDefBaseBounds
void EntityDefBaseBounds(const EntityDef* def, ape::Aabb& out)
{
    const ape::world::ApeObj* object = def->templateObject;
    if (!object) {
        static const ape::Vec3 kOrigin = { 0.0f, 0.0f, 0.0f, 0.0f };
        ape::AabbFromPoint(out, kOrigin);
        return;
    }
    out = object->GetDef()->GetBounds();
    ape::AabbScale(out, object->GetScale());
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
const char* EntityDef::InstanceName(const SpawnParams& params)
{
    if (params.name[0] != '\0')
        return params.name;
    if (params.sourceObject)
        return params.sourceObject->GetName();
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
Entity* SpawnFromDef(EntityDef* def, SpawnParams& params)
{
    return def ? def->Spawn(params) : nullptr;
}

// @func 0x000D6D28 SpawnEntity
// Completes the parameters from the definition: its fields, its option bit
// and, when the caller supplied none, its attachment records.
Entity* SpawnEntity(uint32_t defName, SpawnParams& params)
{
    EntityDef* def = FindEntityDef(defName);
    if (!def)
        return nullptr;
    params.def = def;
    params.defField24 = def->field_24;
    params.defField28 = def->field_28;
    params.flag20000000 = (def->options & 0x04) != 0;

    if (def->attachments && def->attachments->Count() != 0 &&
        (!params.attachments || params.attachments->Count() == 0)) {
        for (const SpawnAttachmentDesc& desc : *def->attachments) {
            if (!params.attachments)
                params.attachments = new ape::ApeTArray<SpawnAttachmentDesc>(4);
            params.attachments->PushBack(desc);
        }
    }
    return def->Spawn(params);
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
