// openTIT - Small actor classes of 0x001AA000-0x001B2000 whose own code is a
// handful of overrides (their destructors live in the helper range
// 0x0075xxxx). Names are descriptive; each gives its console vtable.
#pragma once

#include "sly/entity/ActorEntities.h"

namespace sly {
namespace entity {

// Base version of an actor's message handler (0x000E5E44, entity module).
void ActorHandleMessageBase(ActorEntity* actor, const EntityMessage& msg);

// A group of member actors that react together (vtable 0x00B15310;
// destructor 0x00751314).
class ActorGroup : public ActorEntity {
public:
    // @func 0x00751314 ActorGroup::~ActorGroup
    // @func 0x0075140C ActorGroup::~ActorGroup (deleting)
    // @func 0x0075184C ActorGroup::~ActorGroup (thunk, +0x17C)
    // @func 0x00751638 ActorGroup::~ActorGroup (deleting thunk, +0x17C)
    // @func 0x00751750 ActorGroup::~ActorGroup (thunk, +0x180)
    // @func 0x00751520 ActorGroup::~ActorGroup (deleting thunk, +0x180)
    ~ActorGroup() override = default;
    // @func 0x001AA5E0 ActorGroup::Release
    bool Release(bool kill) override { return ActorEntity::Release(kill); }
    // A hurt member plays its second slot state and enters state 1.
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    // Slot 11: the group and every member back to their first state.
    void ResetState() override;

    struct Member {
        Entity* entity;
        void* record;        // NOTE: its word +0x14 takes the group's counter
    };
    ape::ApeTArray<Member> members;      // 0x1A8
    uint32_t counter = 0;                // 0x1C4
    bool anyState = false;               // 0x1D0 members react in any state
};

// Counts hits from players (vtable 0x00B15550; destructor 0x00751AA4).
class HitCounterActor : public ActorEntity {
public:
    // @func 0x00751AA4 HitCounterActor::~HitCounterActor
    // @func 0x00751B5C HitCounterActor::~HitCounterActor (deleting)
    // @func 0x00751E90 HitCounterActor::~HitCounterActor (thunk, +0x17C)
    // @func 0x00751D00 HitCounterActor::~HitCounterActor (deleting thunk, +0x17C)
    // @func 0x00751DD4 HitCounterActor::~HitCounterActor (thunk, +0x180)
    // @func 0x00751C2C HitCounterActor::~HitCounterActor (deleting thunk, +0x180)
    ~HitCounterActor() override = default;
    // @func 0x001ABAF8 HitCounterActor::Release
    bool Release(bool kill) override { return ActorEntity::Release(kill); }
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    void OnHitCategory4(EntityHitRecord& hit) override;      // slot 87
    // Slot 88: takes no damage; counts the player hits instead.
    bool Damage(float amount, Entity* source, uint32_t type) override;

    uint32_t hits = 0;                   // 0x1C4
    uint32_t key = 0;                    // 0x1D0 message argument that counts as a hit
    uint32_t counterFlags = 0;           // 0x1D4 bit 30 shields projectiles of damage bit 5, bit 29 shielded one
};

// Waits for a named entity to be killed or picked up (vtable 0x00B15778;
// destructor 0x00751FD4).
class WatchActor : public ActorEntity {
public:
    // @func 0x00751FD4 WatchActor::~WatchActor
    // @func 0x0075208C WatchActor::~WatchActor (deleting)
    // @func 0x007523C0 WatchActor::~WatchActor (thunk, +0x17C)
    // @func 0x00752230 WatchActor::~WatchActor (deleting thunk, +0x17C)
    // @func 0x00752304 WatchActor::~WatchActor (thunk, +0x180)
    // @func 0x0075215C WatchActor::~WatchActor (deleting thunk, +0x180)
    ~WatchActor() override = default;
    // @func 0x001ACA78 WatchActor::Release
    bool Release(bool kill) override { return ActorEntity::Release(kill); }
    void HandleMessage(const EntityMessage& msg) override;   // slot 43

    Entity* watched = nullptr;           // 0x1AC
};

// Plays a state when told to (vtable 0x00B15C20; destructor 0x007526AC).
class SignalActor : public ActorEntity {
public:
    // @func 0x007526AC SignalActor::~SignalActor
    // @func 0x00752764 SignalActor::~SignalActor (deleting)
    // @func 0x00752A98 SignalActor::~SignalActor (thunk, +0x17C)
    // @func 0x00752908 SignalActor::~SignalActor (deleting thunk, +0x17C)
    // @func 0x007529DC SignalActor::~SignalActor (thunk, +0x180)
    // @func 0x00752834 SignalActor::~SignalActor (deleting thunk, +0x180)
    ~SignalActor() override = default;
    // @func 0x001B06CC SignalActor::Release
    bool Release(bool kill) override { return ActorEntity::Release(kill); }
    void HandleMessage(const EntityMessage& msg) override;   // slot 43

    uint32_t field_1D0 = 0;              // 0x1D0 cleared by the signal
};

// vtable 0x00B15E60 (destructor 0x00752CB0).
class ActorB15E60 : public ActorEntity {
public:
    // @func 0x00752CB0 ActorB15E60::~ActorB15E60
    // @func 0x00752D68 ActorB15E60::~ActorB15E60 (deleting)
    // @func 0x0075309C ActorB15E60::~ActorB15E60 (thunk, +0x17C)
    // @func 0x00752F0C ActorB15E60::~ActorB15E60 (deleting thunk, +0x17C)
    // @func 0x00752FE0 ActorB15E60::~ActorB15E60 (thunk, +0x180)
    // @func 0x00752E38 ActorB15E60::~ActorB15E60 (deleting thunk, +0x180)
    ~ActorB15E60() override = default;
    // @func 0x001B2A1C ActorB15E60::Release
    bool Release(bool kill) override { return ActorEntity::Release(kill); }
};

// Actor holding an object in an anim script variable (vtable 0x00B16438;
// destructor 0x00753C08).
class ScriptObjectActor : public ActorEntity {
public:
    // @func 0x00753C08 ScriptObjectActor::~ScriptObjectActor
    // @func 0x00753CC0 ScriptObjectActor::~ScriptObjectActor (deleting)
    // @func 0x00753FF4 ScriptObjectActor::~ScriptObjectActor (thunk, +0x17C)
    // @func 0x00753E64 ScriptObjectActor::~ScriptObjectActor (deleting thunk, +0x17C)
    // @func 0x00753F38 ScriptObjectActor::~ScriptObjectActor (thunk, +0x180)
    // @func 0x00753D90 ScriptObjectActor::~ScriptObjectActor (deleting thunk, +0x180)
    ~ScriptObjectActor() override = default;
    // Drops the script's held object and plays the release state.
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    // Only player interaction, and only in mode 1 (+0x188 >> 1).
    bool CanInteractWith(Entity* other, int kind) override;  // slot 76

    uint32_t mode = 0;                   // 0x188
};

// Input mover (vtable 0x00B17990, destructor 0x00756154; class flag 1 of
// +0xC8). NOTE: identified from the input mover hooks of the level scripts.
class ActorB17990 : public ActorEntity {
public:
    // @func 0x00756154 ActorB17990::~ActorB17990
    // @func 0x0075620C ActorB17990::~ActorB17990 (deleting)
    // @func 0x00756540 ActorB17990::~ActorB17990 (thunk, +0x17C)
    // @func 0x007563B0 ActorB17990::~ActorB17990 (deleting thunk, +0x17C)
    // @func 0x00756484 ActorB17990::~ActorB17990 (thunk, +0x180)
    // @func 0x007562DC ActorB17990::~ActorB17990 (deleting thunk, +0x180)
    ~ActorB17990() override = default;
    bool Release(bool kill) override;                 // slot 10
    // Slot 11: back to state 1, offering nothing.
    void ResetState() override;
    // 0x001D81F0 (not ported).
    void SetField1D81F0(bool on);
    // Sets the input points (a fraction of the maximum when `scaled`) and
    // shows the fill ratio on the meter.
    void SetPoints(float points, bool scaled);

    float maxPoints = 0.0f;              // 0x1F0
    struct Meter {                       // NOTE: only the ratio is known
        uint8_t field_00[0x14];
        float ratio;                     // +0x14
    };
    Meter* meter = nullptr;              // 0x240
    float points = 0.0f;                 // 0x248
    float shownPoints = 0.0f;            // 0x24C
    uint32_t field_260 = 0;              // 0x260
};

} // namespace entity
} // namespace sly
