// openTIT - The rope entity (vtable 0x00B162C8, 0x1A0 bytes; spawned by
// EntityDefRope).
//
// A chain of segments, each with a world object, simulated by the rope
// solver (Entity.Rope.UpdateMode selects how). The solver itself
// (0x001BC5C8, 0x001BBC40) and the constructor are not ported.
#pragma once

#include "sly/entity/Entity.h"

namespace sly {
namespace entity {

class RopeEntity : public Entity {
public:
    // One segment (0x60 bytes).
    struct Segment {
        uint8_t field_00[0x10];
        ape::Vec3 axis{};                          // +0x10 direction of the segment
        uint8_t field_20[0x20];
        ape::Vec3 push{};                          // +0x40 sideways push gathered this frame
        ape::world::ApeObj* object = nullptr;      // +0x50 segment geometry (counted)
        uint8_t field_54[0xC];
    };
    // ropeFlags (+0x18A).
    enum RopeFlags : uint16_t {
        kRope_Pending      = 0x0400,   // set by RequestUpdate, cleared by PostUpdate
        kRope_SharedObject = 0x0800,   // the render object is not detached on destruction
    };
    // User tag the rope puts on its render object (data: the rope).
    static constexpr uint32_t kObjectTag = 0x02235C33u;

    RopeEntity() = default;
    ~RopeEntity() override;
    // Slot 5: runs the solver step (update modes 1-3).
    bool PostUpdate(float dt) override;
    // Slot 10: the plain entity release (the class carries its own copy).
    bool Release(bool kill) override;
    // Slot 12: takes the solver's result over (update mode 3).
    void UpdateTransform(float dt = 0.0f) override;
    // Slot 22: the segments follow the entity's visibility.
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;

    // Segment objects' slot 13 (0x001BD2B0).
    void SetSegmentsSlot13(uint8_t value, bool recurse);
    // Pushes the segment at `along` (0..1 of the rope) by the part of
    // `impulse` across it (0x001BD370). NOTE: the console takes a record
    // holding the impulse at +0x10 and the position along the rope at +0x24.
    void PushSegment(const ape::Vec3& impulse, float along);
    // @func 0x001BD440 RopeEntity::RequestUpdate
    void RequestUpdate() { ropeFlags |= kRope_Pending; }
    void SetRopeObjectVisible(bool visible);
    bool IsRopeObjectVisible() const;
    void SetRenderObjectVisible(bool visible);
    Segment& SegmentAt(int index);

    // Solver step and result (0x001BC5C8 / 0x001BBC40, not ported).
    void Simulate();
    void ApplySolver();

    ape::world::ApeObj* renderObject = nullptr;   // 0x180 counted
    Segment* segments = nullptr;                  // 0x184 owned array
    uint16_t segmentCount = 0;                    // 0x188
    uint16_t ropeFlags = 0;                       // 0x18A RopeFlags
    ape::world::ApeObj* solverObject = nullptr;   // 0x190 counted
    ape::world::ApeObj* ropeObject = nullptr;     // 0x194 counted
};

// Game world callback queued by the rope: its slot 12 later in the frame
// (0x001BD614).
void RopeQueuedUpdateTransform(void* owner, Entity* rope);

} // namespace entity
} // namespace sly
