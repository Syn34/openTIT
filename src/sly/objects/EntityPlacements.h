// openTIT - The placement ("settings") classes of the scripted entity
// definitions: one per definition class, each built by its definition's
// parser. The scripted entity placement adds two arrays to the base
// placement; the others add nothing their destructors see. The destructors
// (0x00739C5C-0x00758F1C) and the constructors of the three placements built
// straight on the base are ported; the placement copies (slot 2, 0x000DFBFC
// and the classes' own) are not.
#pragma once

#include "sly/objects/ObjectsBase.h"
#include "ape/memory/ApeTArray.h"

namespace sly {
namespace objects {

// Placement of scripted entities (vtable 0x00B12868). NOTE: the class names
// follow the definitions whose parsers build them; the two base names are
// guesses.
class ScriptedEntityPlacement : public EntityPlacement {
public:
    using EntityPlacement::EntityPlacement;
    ~ScriptedEntityPlacement() override;

    // NOTE: element types unknown (script variable overrides of the placement).
    ape::ApeTArray<void*> field_B4;   // +0xB4
    ape::ApeTArray<void*> field_C8;   // +0xC8
};

#define SLY_PLACEMENT(Name, Base)                 \
    class Name : public Base {                    \
    public:                                       \
        using Base::Base;                         \
        ~Name() override;                         \
    }

SLY_PLACEMENT(ActorPlacement, ScriptedEntityPlacement);             // 0x00B12880 (ActorDef)
SLY_PLACEMENT(PhysicalEntityPlacement, ScriptedEntityPlacement);    // 0x00B12CB8
SLY_PLACEMENT(RigidPropPlacement, ScriptedEntityPlacement);         // 0x00B14390
SLY_PLACEMENT(AlterEgoPlacement, ScriptedEntityPlacement);          // 0x00B14948
SLY_PLACEMENT(TargetableBPlacement, ScriptedEntityPlacement);       // 0x00B14DC8
SLY_PLACEMENT(TargetableCPlacement, ScriptedEntityPlacement);       // 0x00B18B30
SLY_PLACEMENT(TargetableCLinkedPlacement, ScriptedEntityPlacement); // 0x00B18F48
SLY_PLACEMENT(RigidEntityB19198Placement, ScriptedEntityPlacement); // 0x00B19178
SLY_PLACEMENT(ActorGroupPlacement, ActorPlacement);                 // 0x00B152F8
SLY_PLACEMENT(HitCounterActorPlacement, ActorPlacement);            // 0x00B15538
SLY_PLACEMENT(SoundActorPlacement, ActorPlacement);                 // 0x00B159A0
SLY_PLACEMENT(SignalActorPlacement, ActorPlacement);                // 0x00B15C08
SLY_PLACEMENT(ActorB15E60Placement, ActorPlacement);                // 0x00B15E48
SLY_PLACEMENT(ScriptObjectActorPlacement, ActorPlacement);          // 0x00B16420
SLY_PLACEMENT(SoundActorB16678Placement, ActorPlacement);           // 0x00B16660
SLY_PLACEMENT(SoundActorB168E0Placement, ActorPlacement);           // 0x00B168C8
SLY_PLACEMENT(SoundActorB16B48Placement, ActorPlacement);           // 0x00B16B30
SLY_PLACEMENT(SoundActorB16DB0Placement, ActorPlacement);           // 0x00B16D98
SLY_PLACEMENT(SoundActorB17270Placement, ActorPlacement);           // 0x00B17258
SLY_PLACEMENT(SoundActorB17728Placement, ActorPlacement);           // 0x00B17710
SLY_PLACEMENT(ActorB17990Placement, ActorPlacement);                // 0x00B17978
SLY_PLACEMENT(InteractivePropAPlacement, ActorPlacement);           // 0x00B17DE0
SLY_PLACEMENT(SafePlacement, ActorPlacement);                       // 0x00B18020
SLY_PLACEMENT(TweenActorPlacement, ActorPlacement);                 // 0x00B186B0
SLY_PLACEMENT(InteractivePropBPlacement, ActorPlacement);           // 0x00B188F0

#undef SLY_PLACEMENT

// Placements built straight on the base placement. NOTE: the meanings of
// their own fields are not known; their parsers are not ported.

// Vtable 0x00B15180; type bit 0x800.
class ChainPlacement : public EntityPlacement {
public:
    ChainPlacement(const char* placementName, EntityDef* ownerDef, uint32_t field70,
                   const ape::Matrix34& placementTransform, const ape::Vec3& offset, bool bit31, bool bit30,
                   uint32_t field74, uint32_t valueB4);
    ~ChainPlacement() override;

    uint32_t field_B4 = 0;             // +0xB4
};

// Vtable 0x00B162B0; type bit 0x1000.
class RopePlacement : public EntityPlacement {
public:
    RopePlacement(const char* placementName, EntityDef* ownerDef, uint32_t field70,
                  const ape::Matrix34& placementTransform, const ape::Vec3& offset, bool bit31, bool bit30,
                  uint32_t field74, uint32_t valueE0, const ape::Vec3& pointC0, const ape::Vec3& pointD0,
                  uint32_t valueE4, bool flagE8);
    ~RopePlacement() override;

    ape::Vec3 field_C0{};              // +0xC0
    ape::Vec3 field_D0{};              // +0xD0
    uint32_t field_E0 = 0;             // +0xE0
    uint32_t field_E4 = 0;             // +0xE4
    bool field_E8 = false;             // +0xE8
};

// Vtable 0x00B193A8; sets bit 7 of the word at +0x0C.
class PlacementB193A8 : public EntityPlacement {
public:
    PlacementB193A8(const char* placementName, EntityDef* ownerDef, const ape::Matrix34& placementTransform,
                    bool bit31, uint32_t valueB4, bool flagB8);
    ~PlacementB193A8() override;

    uint32_t field_B4 = 0;             // +0xB4
    bool field_B8 = false;             // +0xB8
};

} // namespace objects
} // namespace sly
