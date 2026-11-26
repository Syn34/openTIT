// openTIT - Definitions of the actor variants and a few other entity classes
// that add nothing the ported code reads (0x00750EAC-0x00759358).
//
// Only their destructors (and ObjectActorDef's Spawn) are console functions
// of their own; each class names its console vtable and the entity class its
// Spawn builds. Names follow the entity classes in ActorVariants.h /
// ActorEntities.h.
#pragma once

#include "sly/entity/ActorEntities.h"
#include "sly/entity/PropEntities.h"

namespace sly {
namespace entity {

#define SLY_ACTOR_DEF(Name, Base)       \
    class Name : public Base {          \
    public:                             \
        ~Name() override;               \
    }

// Plain actor definitions (base ActorDef).
// 0x00B152C8, spawns ActorGroup. NOTE: the fields are the defaults of its
// placements (the definition parser is not ported).
class ActorGroupDef : public ActorDef {
public:
    ~ActorGroupDef() override;
    float field_130[4] = {};            // 0x130
    bool field_140 = false;             // 0x140
};
// 0x00B15508, spawns HitCounterActor. NOTE: as above.
class HitCounterActorDef : public ActorDef {
public:
    ~HitCounterActorDef() override;
    float field_130[5] = {};            // 0x130
    uint32_t field_144 = 0;             // 0x144
    uint32_t flags_148 = 0x80000000u;   // 0x148 bits 30/31: placement defaults
};
SLY_ACTOR_DEF(WatchActorDef, ActorDef);         // 0x00B15748, spawns WatchActor
SLY_ACTOR_DEF(SignalActorDef, ActorDef);        // 0x00B15BD8, spawns SignalActor
SLY_ACTOR_DEF(ActorB15E60Def, ActorDef);        // 0x00B15E18, spawns ActorB15E60
SLY_ACTOR_DEF(ScriptObjectActorDef, ActorDef);  // 0x00B163F0, spawns ScriptObjectActor
SLY_ACTOR_DEF(ActorB17990Def, ActorDef);        // 0x00B17948, spawns ActorB17990
SLY_ACTOR_DEF(TweenActorDef, ActorDef);         // 0x00B18680, spawns TweenActor
SLY_ACTOR_DEF(InteractivePropBDef, ActorDef);   // 0x00B188C0, spawns InteractivePropB

// Sound actor definitions (base SoundActorDef).
SLY_ACTOR_DEF(SoundActorB16678Def, SoundActorDef);   // 0x00B16630
SLY_ACTOR_DEF(SoundActorB168E0Def, SoundActorDef);   // 0x00B16898
SLY_ACTOR_DEF(SoundActorB17008Def, SoundActorDef);   // 0x00B16FD0
SLY_ACTOR_DEF(SoundActorB17270Def, SoundActorDef);   // 0x00B17228
SLY_ACTOR_DEF(SoundActorB174C0Def, SoundActorDef);   // 0x00B17490
// 0x00B176E0
class SoundActorB17728Def : public SoundActorDef {
public:
    ~SoundActorB17728Def() override;
    // 0x198 character (CharacterDef::characterName) a near player must be to
    // open the actor; 0 for anyone.
    uint32_t requiredCharacter = 0;
};

// Plain entity definitions.
SLY_ACTOR_DEF(EntityDefRope, EntityDef);        // 0x00B16280 ("EntityDefRope"), spawns 0x00B162C8
SLY_ACTOR_DEF(EntityDefB19378, EntityDef);      // 0x00B19378, spawns 0x00B193C8

// Definition of TargetableCLinked (vtable 0x00B18F18).
SLY_ACTOR_DEF(TargetableCLinkedDef, TargetableDefC);

#undef SLY_ACTOR_DEF

// Definition of ObjectActor (vtable 0x00B18458): it spawns nothing itself.
class ObjectActorDef : public ActorDef {
public:
    ~ObjectActorDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
};

// Definition with one array of 12-byte records (vtable 0x00B14FD8; spawn
// 0x001A51E8). NOTE: the entity class and the records are not identified.
class EntityDefB14FD8 : public EntityDef {
public:
    struct Record {
        uint32_t field_00 = 0;
        uint32_t field_04 = 0;
        uint32_t field_08 = 0;
    };
    ~EntityDefB14FD8() override;
    ape::ApeTArray<Record> records;   // 0x8C
};

} // namespace entity
} // namespace sly
