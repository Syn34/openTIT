// openTIT - <Receptacle> component (type 0x1C, vtable 0x00B1CE78, 0x48 bytes):
// slots on the owner that carried entities can be put into (a treasure on a
// pedestal, an object in a holder). Each slot names a bone or an object of the
// owner and an offset; an entity put in glides there over a short time and is
// then attached to the owner. Only the definitions listed by the description
// are accepted (any entity when the list is empty).
#pragma once

#include "sly/components/ComponentDescs.h"
#include "sly/entity/Entity.h"

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { namespace world { class ApeObj; } }

namespace sly {
namespace components {

// One slot at run time (0x90 bytes: the description's slot with the bone /
// object resolved and the state of the entity it holds).
struct ReceptacleSlotState {
    uint32_t name = 0;                          // +0x00 bone of the owner the slot sits on
    uint32_t part = 0;                          // +0x04 object of the owner used without such a bone
    int32_t bone = -1;                          // +0x08 resolved bone index (-1: none)
    ape::world::ApeObj* object = nullptr;       // +0x0C resolved object (without a bone)
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f}; // +0x10 slot orientation
    ape::Vec3 offset{};                         // +0x20 slot offset
    entity::Entity* held = nullptr;             // +0x30 entity in the slot
    uint8_t field_34 = 0;                       // +0x34 NOTE: meaning unknown
    ape::Vec3 startPosition{};                  // +0x40 where the held entity started gliding
    ape::Vec4 startRotation{0.0f, 0.0f, 0.0f, 1.0f};   // +0x50
    float glideTime = 0.0f;                     // +0x60 length of the glide
    float glideTimer = 0.0f;                    // +0x64
    ape::Vec3 field_70{};                       // +0x70 NOTE: vector handed to Insert
    float savedMass = 0.0f;                     // +0x80 mass of a rigid body entity while held
};

class ReceptacleComponent : public entity::EntityComponent {
public:
    // 0x0025843C
    explicit ReceptacleComponent(const ReceptacleComponentDesc* desc);
    ~ReceptacleComponent() override;
    // Slot 6: glides the entities put in to their slots.
    void Update(float dt) override;

    // The entity may be put in: its definition is accepted and a slot is free.
    bool CanAccept(const entity::Entity* entity) const;
    // Index of the first free slot, -1 when all are taken.
    int FreeSlot() const;
    // World position of the next free slot (zero when the receptacle is full).
    ape::Vec3 NextSlotPosition() const;
    // The entity is held by one of the first `heldCount` slots.
    bool Holds(const entity::Entity* entity) const;
    // Puts an entity in (0x002B9454, TODO).
    bool Insert(entity::Entity* entity, const ape::Vec3& target, float glideTime, bool immediate);
    // Takes an entity out again (0x002B99C0, TODO).
    bool Remove(entity::Entity* entity);

    ape::ApeTArray<ReceptacleSlotState> slots;   // 0x18
    ape::ApeTArray<uint32_t> acceptedDefs;       // 0x2C definition name hashes
    int32_t heldCount = 0;                       // 0x40
    bool lockEntities = false;                   // 0x44 bit 7: held entities get class bit 16

private:
    bool AcceptsDef(const entity::Entity* entity) const;
};

} // namespace components
} // namespace sly
