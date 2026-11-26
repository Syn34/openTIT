// openTIT - The laser field entity (vtable 0x00B12080 + 0x00B12204 at +0x17C;
// constructor 0x000B2E18, used by the "MineLaserField_Tutorial" set-up).
//
// Only the small virtual overrides in 0x00744358-0x007446A4 are ported:
// any accepted hit trips the field from its idle states (0 and 1) into
// state 2.
#pragma once

#include "sly/entity/EntityInterfaces.h"
#include "sly/entity/ScriptedEntity.h"

namespace sly {
namespace entity {

// Definition of the laser field (vtable 0x00B12048, 0x110 bytes; Spawn
// 0x000C0FE8 not ported).
class LaserFieldDef : public ScriptedEntityDef {
public:
    // Drops the Havok shape it holds.
    ~LaserFieldDef() override;

    void* collisionShape = nullptr;   // 0xF0 Havok shape (counted reference)
    bool playersTrip = false;         // 0x106 players touching the field trip it
};

class LaserFieldEntity : public ScriptedEntity, public EntityInterface17C {
public:
    ~LaserFieldEntity() override;
    // Slot 36: the field's Havok object (+0x1B0). NOTE: see NOTES on slot 36.
    ape::world::ApeObj* GetRenderObject() override;
    // Physics impacts are ignored; any other accepted hit trips the field.
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    // Contact callback (+0x17C slot 5, 0x000C21EC, thunk 0x000C23D4): the
    // contact is disabled; a player touching the field's body trips it from
    // its idle states when the definition says so.
    void Interface17CSlot5(const void* event) override;

    void* body = nullptr;          // 0x1B0 NOTE: cleared by the constructor, set later
    uint32_t field_1B4 = 0;        // 0x1B4 from the definition (+0xF0)
    uint32_t field_1B8 = 0;        // 0x1B8 from the definition (+0xFC)

private:
    void Trip(EntityHitRecord& hit);
};

} // namespace entity
} // namespace sly
