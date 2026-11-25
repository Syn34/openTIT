// openTIT - <FacePlayer> component (type 0x16, vtable 0x00B1C730, 0x110 bytes):
// turns a part of the entity (a skeleton bone, a sub-object or the entity
// itself) about its up axis so that it faces the local player, at a limited
// speed, and drives "turning" states of the entity's anim script.
#pragma once

#include "sly/entity/Entity.h"

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; struct ApeSkeletonBone; }

namespace sly {
namespace components {

struct FacePlayerComponentDesc;

class FacePlayerComponent : public entity::EntityComponent {
public:
    // Anim script states set while turning. NOTE: names unknown.
    static constexpr uint32_t kState_TurnBegin      = 0x882FF4C0u;
    static constexpr uint32_t kState_TurnBeginLeft  = 0x51CB2CC7u;
    static constexpr uint32_t kState_TurnBeginRight = 0xE7F0CD3Cu;
    static constexpr uint32_t kState_TurnEnd        = 0x246E625Eu;
    static constexpr uint32_t kState_TurnEndLeft    = 0xA7BD9B65u;
    static constexpr uint32_t kState_TurnEndRight   = 0x504C325Eu;

    enum Turn : uint32_t { kTurn_None = 0, kTurn_Left = 1, kTurn_Right = 2 };

    // 0x0025C090.
    explicit FacePlayerComponent(const FacePlayerComponentDesc* desc);
    // 0x0028F2B0 / 0x0028F3E4.
    ~FacePlayerComponent() override;
    // Slot 7 (0x0028F588): turns towards the player and places the part.
    void PostUpdate(float dt) override;
    // Slot 11 (0x00290190): switches the component; switching it on takes the
    // part's current facing.
    void SetOption(bool value) override;

    // 0x00290084: facing taken from the part as it is now.
    void ResetFacing();
    // 0x002901E0: writes `output` into the bone or sub-object.
    void ApplyToPart();
    // World callback 0x002908EC (descriptor 0x00F353E0) running ApplyToPart.
    static void ApplyCallback(void* unused, FacePlayerComponent* component);

    ape::Matrix34 rest;                  // +0x20 part transform at creation (world space)
    ape::Matrix34 previous;              // +0x60 last output
    ape::Matrix34 output;                // +0xA0 part transform facing the player
    ape::Vec3 facing;                    // +0xE0 current facing (output z axis)
    // +0xF0: the parent bone of `bone` when the part is a bone, else the
    // named sub-object (null: the entity itself).
    union {
        ape::world::ApeSkeletonBone* parentBone;
        ape::world::ApeObj* partObject = nullptr;
    };
    ape::world::ApeSkeletonBone* bone = nullptr;   // +0xF4
    bool isBone = false;                 // +0xF8 bit 31
    uint32_t turn = kTurn_None;          // +0xFC Turn
    float rotSpeed = 0.0f;               // +0x100 radians per second (0: snap)
    bool lockY = false;                  // +0x104 bit 31 turn in the horizontal plane
    bool snapToAxes = false;             // +0x104 bit 30 desc flag1D. NOTE: name assumed; faces
                                         // along one of the rest axes only
};

} // namespace components
} // namespace sly
