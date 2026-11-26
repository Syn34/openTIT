// openTIT - <Receptacle> component (see ReceptacleComponent.h).
#include "sly/components/ReceptacleComponent.h"

#include "sly/entity/EntityDef.h"

namespace sly {
namespace components {

using entity::Entity;

// @func 0x0025843C ReceptacleComponent::ReceptacleComponent
ReceptacleComponent::ReceptacleComponent(const ReceptacleComponentDesc* desc)
    : EntityComponent(desc), acceptedDefs(desc->acceptedDefs.Count())
{
    lockEntities = desc->lockEntities;
    for (const ReceptacleSlot& source : desc->slots) {
        ReceptacleSlotState slot;
        slot.name = source.name;
        slot.part = source.part;
        slot.bone = source.field08;
        slot.rotation = {source.rot.x, source.rot.y, source.rot.z, 0.0f};
        slot.offset = source.pos;
        // NOTE: the console copies the whole record, including the run-time
        // part, which the parser leaves at its defaults (nothing held, no
        // glide running).
        slot.startPosition = source.vec40;
        slot.startRotation = source.quat50;
        slot.field_70 = source.vec70;
        // A slot sits on the bone named `name`, else on the object `part`.
        if (slot.name != 0) {
            slot.bone = owner->FindBone(slot.name);
            if (slot.bone == -1)
                slot.object = entity::EntityFindObject(owner, slot.part);
        }
        slots.PushBack(slot);
    }
    for (uint32_t def : desc->acceptedDefs)
        acceptedDefs.PushBack(def);
}

// @func 0x002B8798 ReceptacleComponent::~ReceptacleComponent
// @func 0x002B88F0 ReceptacleComponent::~ReceptacleComponent (deleting)
ReceptacleComponent::~ReceptacleComponent() = default;

// TODO(0x002B8A80 ReceptacleComponent::Update): eases each held entity
// from its start transform to its slot (cosine ease over glideTime), then
// attaches it to the owner and sets both scripts to state 0x268D69AD. The
// vector part could not be decoded yet.
void ReceptacleComponent::Update(float dt)
{
    for (int32_t i = 0; i < heldCount; ++i) {
        ReceptacleSlotState& slot = slots[i];
        if (slot.glideTimer >= slot.glideTime)
            continue;
        float timer = slot.glideTimer + dt;
        if (timer > slot.glideTime)
            timer = slot.glideTime;
        if (timer < 0.0f)
            timer = 0.0f;
        slot.glideTimer = timer;
    }
}

bool ReceptacleComponent::AcceptsDef(const Entity* entity) const
{
    if (acceptedDefs.Count() == 0)
        return true;
    const uint32_t defName = entity->def ? entity->def->nameHash : 0;
    for (uint32_t accepted : acceptedDefs)
        if (accepted == defName)
            return true;
    return false;
}

// @func 0x002B91EC ReceptacleComponent::CanAccept
bool ReceptacleComponent::CanAccept(const Entity* entity) const
{
    if (!entity || !AcceptsDef(entity))
        return false;
    return heldCount < int32_t(slots.Count());
}

// @func 0x002B92A0 ReceptacleComponent::FreeSlot
int ReceptacleComponent::FreeSlot() const
{
    for (int i = 0; i < int(slots.Count()); ++i)
        if (!slots[i].held)
            return i;
    return -1;
}

// @func 0x002B92F8 ReceptacleComponent::NextSlotPosition
ape::Vec3 ReceptacleComponent::NextSlotPosition() const
{
    if (heldCount >= int32_t(slots.Count()))
        return {0.0f, 0.0f, 0.0f, 0.0f};
    // NOTE: like the console this indexes with -1 when no slot is free
    // although fewer entities are held than there are slots.
    const ReceptacleSlotState& slot = slots[size_t(FreeSlot())];
    // NOTE: the slot's rotation is handed over as a quaternion with an
    // undefined w (the console leaves a register there); 0 is used.
    const ape::Matrix34 at = owner->AttachTransform(slot.bone, slot.object, slot.rotation, slot.offset);
    return at.translation;
}

// @func 0x002B940C ReceptacleComponent::Holds
bool ReceptacleComponent::Holds(const Entity* entity) const
{
    for (int32_t i = 0; i < heldCount; ++i)
        if (slots[i].held == entity)
            return true;
    return false;
}

// TODO(0x002B9454 ReceptacleComponent::Insert): takes the first free slot,
// records the entity's start transform and either starts the glide or
// attaches it at once; needs the rigid body mass helpers (0x001FB178/
// 0x001FB1C8) and the body property calls (0x007EE08C).
bool ReceptacleComponent::Insert(Entity* entity, const ape::Vec3& target, float glideTime, bool immediate)
{
    (void)entity; (void)target; (void)glideTime; (void)immediate;
    return false;
}

// TODO(0x002B99C0 ReceptacleComponent::Remove): frees the entity's slot,
// sets both scripts to state 0x3E5C25CD, restores the saved mass and
// detaches it from the owner.
bool ReceptacleComponent::Remove(Entity* entity)
{
    (void)entity;
    return false;
}

} // namespace components
} // namespace sly
