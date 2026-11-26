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
// 0x00B15BD8, spawns SignalActor. NOTE: only the placement defaults.
class SignalActorDef : public ActorDef {
public:
    ~SignalActorDef() override;
    float field_15C = 18.849556f;       // 0x15C radians
    float field_160 = 0.08726646f;      // 0x160 radians
    float field_168 = 0.0f;             // 0x168
    uint32_t flags_174 = 0x00800000u;   // 0x174 bits 22/23: placement defaults
};
// 0x00B15E18, spawns ActorB15E60. NOTE: only the placement defaults.
class ActorB15E60Def : public ActorDef {
public:
    ~ActorB15E60Def() override;
    float field_13C[5] = {1.0f, 2.0f, 1.0f, 1.0f, 0.0f};   // 0x13C..0x14C
};
// 0x00B163F0, spawns ScriptObjectActor. NOTE: only the placement defaults.
class ScriptObjectActorDef : public ActorDef {
public:
    ~ScriptObjectActorDef() override;
    float field_130 = 0.0f;             // 0x130
    ape::Vec3 field_140{};              // 0x140
};
SLY_ACTOR_DEF(ActorB17990Def, ActorDef);        // 0x00B17948, spawns ActorB17990
SLY_ACTOR_DEF(TweenActorDef, ActorDef);         // 0x00B18680, spawns TweenActor
SLY_ACTOR_DEF(InteractivePropBDef, ActorDef);   // 0x00B188C0, spawns InteractivePropB

// Sound actor definitions (base SoundActorDef).
// 0x00B16630. NOTE: only the placement defaults.
class SoundActorB16678Def : public SoundActorDef {
public:
    ~SoundActorB16678Def() override;
    uint32_t field_190 = 0;             // 0x190
    float field_194 = -1.0f;            // 0x194
    bool field_198 = false;             // 0x198
    bool field_199 = false;             // 0x199
};
// 0x00B16898. NOTE: only the placement defaults.
class SoundActorB168E0Def : public SoundActorDef {
public:
    ~SoundActorB168E0Def() override;
    float field_198 = 0.0f;             // 0x198
    uint32_t flags_19C = 0x40000000u;   // 0x19C bits 30/31: placement defaults
};
SLY_ACTOR_DEF(SoundActorB17008Def, SoundActorDef);   // 0x00B16FD0
// 0x00B17228. NOTE: only the placement defaults.
class SoundActorB17270Def : public SoundActorDef {
public:
    ~SoundActorB17270Def() override;
    uint32_t field_1B4 = 0;             // 0x1B4
    float field_1B8[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f};   // 0x1B8..0x1D0
    int32_t field_1E0 = -1;             // 0x1E0
    bool field_1E4 = true;              // 0x1E4
    bool field_1E5 = true;              // 0x1E5
};
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
