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
#include "sly/game/Upgrades.h"
#include "ape/world/ApeBase.h"

namespace sly {
namespace objects {

// One script variable binding of a scripted placement (0x4C-byte records,
// read by 0x0049EF40 from the placement's binding elements): the value the
// entity's anim script variable starts with.
using ScriptPropertyOverride = TextField;

// Instance description of scripted entities (base description + 0x38
// bytes); the classes' own parts follow from +0xC8.
struct ScriptedInstanceDesc : EntityInstanceDesc {
    using EntityInstanceDesc::EntityInstanceDesc;
    ape::ApeTArray<ScriptPropertyOverride> overridesA;   // 0x90
    ape::ApeTArray<ScriptPropertyOverride> overridesB;   // 0xA4
    float field_B8 = 0.0f;              // 0xB8
    uint32_t field_BC = 0;              // 0xBC
    uint32_t field_C0 = 0;              // 0xC0
    bool field_C4 = false;              // 0xC4
};

// Placement of scripted entities (vtable 0x00B12868). NOTE: the class names
// follow the definitions whose parsers build them; the two base names are
// guesses.
class ScriptedEntityPlacement : public EntityPlacement {
public:
    // 0x000CC188 (thunk 0x000CC4D4): the base placement, then the scripted
    // defaults from the definition (`defaultField_DC` without one).
    ScriptedEntityPlacement(float defaultField_DC, const char* placementName, EntityDef* ownerDef,
                            ape::world::ApeObj* source, const ape::Matrix34& placementTransform,
                            const ape::Vec3& scale, bool bit31, bool bit30, uint32_t field74 = 0);
    // The placement the loaders build: identity transform, unit scale.
    ScriptedEntityPlacement(const char* placementName, EntityDef* ownerDef)
        : ScriptedEntityPlacement(100.0f, placementName, ownerDef, nullptr, ape::world::kWorldIdentity,
                                  ape::world::kWorldUnitScale, true, false)
    {
    }
    ~ScriptedEntityPlacement() override;
    // Reads the scripted attributes and the two binding lists (0x000CBE10).
    void Parse(ape::xml::XmlElement* elem);
    // Slot 2 (0x000DFBFC): the base fields, the four scalars and both
    // override lists (appended).
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::ApeTArray<ScriptPropertyOverride> overridesA{16, nullptr, true};   // +0xB4 (child element 0xC3B709C0)
    ape::ApeTArray<ScriptPropertyOverride> overridesB{16, nullptr, true};   // +0xC8 (child element 0xB921AB51)
    float field_DC = 100.0f;            // +0xDC attribute 0xA7D74568
    uint32_t field_E0 = 0;              // +0xE0 attribute 0xF89FDD94 (hash)
    uint32_t field_E4 = 0;              // +0xE4 attribute 0x7973EFA5 (hash)
    bool field_E8 = false;              // +0xE8 attribute 0x2243757F
};

// Actor placements share the scripted copy (0x000E44C0 forwards to it).
class ActorPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~ActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;
    uint32_t field_EC = 0;              // +0xEC
};

// The scripted descriptions of the classes below. Field names keep the
// description offsets; each placement field sits 0x20 bytes further on.

// Physical entity: two vectors and two words (also the rigid-body classes'
// common block).
struct PhysicalSettings {
    ape::Vec3 field_00{};               // placement +0xF0, description +0xD0
    ape::Vec3 field_10{};               // +0x100 / +0xE0
    uint32_t field_20 = 0;              // +0x110 / +0xF0
};

class PhysicalEntityPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~PhysicalEntityPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x000EB080
    // The scripted attributes, then the two vectors (the second given in
    // degrees) (0x000CBF88).
    void Parse(ape::xml::XmlElement* elem);
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114 = 0;             // +0x114
};
struct PhysicalInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    PhysicalSettings settings;          // 0xD0
    uint32_t field_F4 = 0;              // 0xF4
};

// Rigid props and the Bentley alter ego hand one word to +0xC8.
class RigidPropPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~RigidPropPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x00168114
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114 = 0;             // +0x114
};
struct RigidPropInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
    PhysicalSettings settings;          // 0xD0
};

// Placement of the Bentley alter ego definition (vtable 0x00B145C0).
class CompoundEntityPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x0016CCD8
    uint32_t field_EC = 0;              // +0xEC
};
struct CompoundEntityInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
};

class AlterEgoPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~AlterEgoPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x00177FB4
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114[5] = {};         // +0x114..+0x124
};
struct AlterEgoInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    PhysicalSettings settings;          // 0xD0
    uint32_t field_F4[5] = {};          // 0xF4..0x104
};

// Placement of TargetableA's definition (vtable 0x00B14B80).
class TargetableAPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x0017F318
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114[4] = {};         // +0x114..+0x120 NOTE: +0x110 is settings.field_20
    uint32_t field_124[3] = {};         // +0x124..+0x12C
    uint8_t field_130 = 0;              // +0x130
    uint32_t field_134 = 0;             // +0x134
    uint32_t field_138 = 0;             // +0x138
    uint8_t field_13C = 0;              // +0x13C
    uint32_t field_140 = 0;             // +0x140
    uint32_t field_144 = 0;             // +0x144
};
struct TargetableAInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
    PhysicalSettings settings;          // 0xD0
    uint32_t field_F4[4] = {};          // 0xF4..0x100
    uint32_t field_104[3] = {};         // 0x104..0x10C
    uint8_t field_110 = 0;              // 0x110
    uint32_t field_114 = 0;             // 0x114
    uint32_t field_118 = 0;             // 0x118
    uint8_t field_11C = 0;              // 0x11C
    uint32_t field_120 = 0;             // 0x120
};

class TargetableBPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~TargetableBPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x00191024
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114[3] = {};         // +0x114..+0x11C
    uint8_t field_120 = 0;              // +0x120
    uint32_t field_124[6] = {};         // +0x124..+0x138
    uint32_t field_13C = 0;             // +0x13C
    uint8_t field_140 = 0;              // +0x140
    uint32_t field_144 = 0;             // +0x144
    uint32_t field_148 = 0;             // +0x148
};
struct TargetableBInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
    PhysicalSettings settings;          // 0xD0
    uint32_t field_F4[3] = {};          // 0xF4..0xFC
    uint8_t field_100 = 0;              // 0x100
    uint32_t field_104[6] = {};         // 0x104..0x118
    uint32_t field_11C = 0;             // 0x11C
    uint8_t field_120 = 0;              // 0x120
    uint32_t field_124 = 0;             // 0x124
};

#define SLY_PLACEMENT(Name, Base)                 \
    class Name : public Base {                    \
    public:                                       \
        using Base::Base;                         \
        ~Name() override;                         \
    }

SLY_PLACEMENT(TargetableCPlacement, ScriptedEntityPlacement);       // 0x00B18B30
SLY_PLACEMENT(TargetableCLinkedPlacement, ScriptedEntityPlacement); // 0x00B18F48
SLY_PLACEMENT(RigidEntityB19198Placement, ScriptedEntityPlacement); // 0x00B19178

#undef SLY_PLACEMENT

// Actor placements and their descriptions (vtables in the comments).

class ActorGroupPlacement : public ActorPlacement {   // 0x00B152F8
public:
    using ActorPlacement::ActorPlacement;
    ~ActorGroupPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001AA57C
    uint32_t field_F0[4] = {};          // +0xF0..+0xFC
    bool field_100 = false;             // +0x100
};
struct ActorGroupInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[4] = {};          // 0xD0..0xDC
    bool field_E0 = false;              // 0xE0
};

class HitCounterActorPlacement : public ActorPlacement {   // 0x00B15538
public:
    using ActorPlacement::ActorPlacement;
    ~HitCounterActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001ABA70
    uint32_t field_F0[5] = {};          // +0xF0..+0x100
    uint32_t flags_104 = 0;             // +0x104 bits 30/31 are handed on
};
struct HitCounterActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[5] = {};          // 0xD0..0xE0
    uint32_t flags_E4 = 0;              // 0xE4
};

// Sound actors: seven sound names and a word.
class SoundActorPlacement : public ActorPlacement {   // 0x00B159A0
public:
    using ActorPlacement::ActorPlacement;
    ~SoundActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001AE3B4
    uint32_t sounds[7] = {};            // +0xF0..+0x108
    uint32_t field_10C = 0;             // +0x10C
};
struct SoundActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t sounds[7] = {};            // 0xD0..0xE8
    uint32_t field_EC = 0;              // 0xEC
};

class SignalActorPlacement : public ActorPlacement {   // 0x00B15C08
public:
    using ActorPlacement::ActorPlacement;
    ~SignalActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001B0654
    uint32_t field_F0[3] = {};          // +0xF0..+0xF8
    uint32_t flags_FC = 0;              // +0xFC bits 30/31 are handed on
};
struct SignalActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[3] = {};          // 0xD0..0xD8
    uint32_t flags_DC = 0;              // 0xDC
};

class ActorB15E60Placement : public ActorPlacement {   // 0x00B15E48
public:
    using ActorPlacement::ActorPlacement;
    ~ActorB15E60Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001B29B8
    uint32_t field_F0[5] = {};          // +0xF0..+0x100
};
struct ActorB15E60InstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[5] = {};          // 0xD0..0xE0
};

// ScriptObjectActor uses the scripted copy directly.
class ScriptObjectActorPlacement : public ActorPlacement {   // 0x00B16420
public:
    using ActorPlacement::ActorPlacement;
    ~ScriptObjectActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001BDFE8
    uint32_t field_F0[3] = {};          // +0xF0..+0xF8
    uint32_t field_100 = 0;             // +0x100
    bool field_108[3] = {};             // +0x108..+0x10A
};
struct ScriptObjectActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[3] = {};          // 0xD0..0xD8
    uint32_t field_E0 = 0;              // 0xE0
    bool field_E8[3] = {};              // 0xE8..0xEA
};

class SoundActorB16678Placement : public SoundActorPlacement {   // 0x00B16660
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16678Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001BF6C0
    uint32_t field_110[2] = {};         // +0x110, +0x114
    bool field_118[2] = {};             // +0x118, +0x119
};
struct SoundActorB16678InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0[2] = {};          // 0xF0, 0xF4
    bool field_F8[2] = {};              // 0xF8, 0xF9
};

class SoundActorB168E0Placement : public SoundActorPlacement {   // 0x00B168C8
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB168E0Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C103C
    uint32_t field_110 = 0;             // +0x110
    uint32_t flags_114 = 0;             // +0x114 bits 30/31 are handed on
};
struct SoundActorB168E0InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0 = 0;              // 0xF0
    uint32_t flags_F4 = 0;              // 0xF4
};

class SoundActorB16B48Placement : public SoundActorPlacement {   // 0x00B16B30
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16B48Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C3308
    uint32_t field_110 = 0;             // +0x110
};
struct SoundActorB16B48InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0 = 0;              // 0xF0
};

// Six phantom-tracked records (0x1E0 bytes copied as a block).
struct SoundActorB16DB0Records {
    uint8_t data[0x1E0];
};
class SoundActorB16DB0Placement : public SoundActorPlacement {   // 0x00B16D98
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16DB0Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C6FB0
    uint32_t field_110[4] = {};         // +0x110..+0x11C
    SoundActorB16DB0Records records{};  // +0x120
    uint32_t flags_300 = 0;             // +0x300 bits 30/31 are handed on
};
struct SoundActorB16DB0InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0[4] = {};          // 0xF0..0xFC
    SoundActorB16DB0Records records{};  // 0x100
    uint32_t flags_2E0 = 0;             // 0x2E0
};

class SoundActorB17270Placement : public SoundActorPlacement {   // 0x00B17258
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB17270Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001CBD04
    uint32_t field_110[9] = {};         // +0x110..+0x130
    bool field_134[2] = {};             // +0x134, +0x135
};
struct SoundActorB17270InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0[9] = {};          // 0xF0..0x110
    bool field_114[2] = {};             // 0x114, 0x115
};

class SoundActorB17728Placement : public SoundActorPlacement {   // 0x00B17710
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB17728Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001D4620
    uint32_t field_110[2] = {};         // +0x110, +0x114
};
struct SoundActorB17728InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0[2] = {};          // 0xF0, 0xF4
};

// Input mover: two vectors, four words and a 0x44-byte block.
struct ActorB17990Block {
    uint32_t words[0x11];
};
class ActorB17990Placement : public ActorPlacement {   // 0x00B17978
public:
    using ActorPlacement::ActorPlacement;
    ~ActorB17990Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001D80C4
    ape::Vec3 field_F0{};               // +0xF0
    ape::Vec3 field_100{};              // +0x100
    uint32_t field_110[4] = {};         // +0x110..+0x11C
    ActorB17990Block block{};           // +0x120
    uint32_t flags_164 = 0;             // +0x164 bits 30/31 are handed on
};
struct ActorB17990InstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    ape::Vec3 field_D0{};               // 0xD0
    ape::Vec3 field_E0{};               // 0xE0
    uint32_t field_F0[4] = {};          // 0xF0..0xFC
    ActorB17990Block block{};           // 0x100
    uint32_t flags_144 = 0;             // 0x144
};

class InteractivePropAPlacement : public ActorPlacement {   // 0x00B17DE0
public:
    using ActorPlacement::ActorPlacement;
    ~InteractivePropAPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001DEB58
    uint32_t field_F0[2] = {};          // +0xF0, +0xF4
};
struct InteractivePropAInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[2] = {};          // 0xD0, 0xD4
};

class SafePlacement : public ActorPlacement {   // 0x00B18020
public:
    using ActorPlacement::ActorPlacement;
    ~SafePlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001E6F3C
    uint32_t field_F0 = 0;              // +0xF0
    uint32_t field_100[3] = {};         // +0x100..+0x108
    uint32_t field_110 = 0;             // +0x110
};
struct SafeInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0 = 0;              // 0xD0
    uint32_t field_E0[3] = {};          // 0xE0..0xE8
    uint32_t field_F0 = 0;              // 0xF0
};

class TweenActorPlacement : public ActorPlacement {   // 0x00B186B0
public:
    using ActorPlacement::ActorPlacement;
    ~TweenActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001ED4F0
    uint32_t field_F0[0x11] = {};       // +0xF0..+0x130
    bool field_134[8] = {};             // +0x134..+0x13B
};
struct TweenActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0[0x11] = {};       // 0xD0..0x110
    bool field_114[8] = {};             // 0x114..0x11B
};

#define SLY_PLACEMENT(Name, Base)                 \
    class Name : public Base {                    \
    public:                                       \
        using Base::Base;                         \
        ~Name() override;                         \
    }

SLY_PLACEMENT(InteractivePropBPlacement, ActorPlacement);           // 0x00B188F0

#undef SLY_PLACEMENT

// Scripted entity placement of the definitions with type bit 0x20000
// (vtable 0x00B16088, 0x110 bytes; parsed for element 0x786E8593, built by
// 0x001B55C8, outside this range). NOTE: names and meanings unknown.
class PlacementB16088 : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~PlacementB16088() override;

    struct Entry {
        uint32_t words[2];
    };
    // Slot 2 (0x001B57A4): the scripted copy, then the entries (appended)
    // and the word.
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::ApeTArray<Entry> field_EC;    // +0xEC copied from the definition (+0xC0)
    uint32_t field_100 = 0;            // +0x100 from the definition (+0xE8)
};
struct InstanceDescB16088 : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    ape::ApeTArray<PlacementB16088::Entry> field_C8;   // 0xC8
    uint32_t field_DC = 0;             // 0xDC
};

// Placements built straight on the base placement. NOTE: the meanings of
// their own fields are not known; their parsers are not ported.

// One piece of a breakable chain definition (12 bytes). NOTE: three floats
// of unknown meaning.
struct BreakableChainPiece {
    float values[3];
};

// Stand-in for the breakable chain definition (not ported): only the piece
// list its placements copy (+0x8C).
class BreakableChainDef : public EntityDef {
public:
    using EntityDef::EntityDef;

    ape::ApeTArray<BreakableChainPiece> pieces;   // +0x8C
};

// Placement of a breakable chain (vtable 0x00B15008, 0xD0 bytes; type bit
// 0x400): its own copy of the definition's pieces.
class BreakableChainPlacement : public EntityPlacement {
public:
    BreakableChainPlacement(const char* placementName, BreakableChainDef* ownerDef, ape::world::ApeObj* source,
                            const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31,
                            bool bit30, uint32_t field74);
    ~BreakableChainPlacement() override;
    // Slot 2 (0x001A5FF4): the base copy and the pieces (appended).
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::ApeTArray<BreakableChainPiece> pieces{16, nullptr, true};   // +0xB4
};
struct BreakableChainInstanceDesc : EntityInstanceDesc {
    using EntityInstanceDesc::EntityInstanceDesc;
    ape::ApeTArray<BreakableChainPiece> pieces;   // 0x90
};

// Vtable 0x00B15180; type bit 0x800.
class ChainPlacement : public EntityPlacement {
public:
    ChainPlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                   const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31, bool bit30,
                   uint32_t field74, uint32_t valueB4);
    ~ChainPlacement() override;
    // Slot 2 (0x001A7D48): the base copy and the word.
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    uint32_t field_B4 = 0;             // +0xB4
};
struct ChainInstanceDesc : EntityInstanceDesc {
    using EntityInstanceDesc::EntityInstanceDesc;
    uint32_t field_90 = 0;             // 0x90
};

// Vtable 0x00B162B0; type bit 0x1000.
class RopePlacement : public EntityPlacement {
public:
    RopePlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                  const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31, bool bit30,
                  uint32_t field74, uint32_t valueE0, const ape::Vec3& pointC0, const ape::Vec3& pointD0,
                  uint32_t valueE4, bool flagE8);
    ~RopePlacement() override;
    // Slot 2 (0x001BB920): the base copy, the two points and words, and the
    // flag when it is set.
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::Vec3 field_C0{};              // +0xC0
    ape::Vec3 field_D0{};              // +0xD0
    uint32_t field_E0 = 0;             // +0xE0
    uint32_t field_E4 = 0;             // +0xE4
    bool field_E8 = false;             // +0xE8
};
struct RopeInstanceDesc : EntityInstanceDesc {
    using EntityInstanceDesc::EntityInstanceDesc;
    ape::Vec3 field_90{};              // 0x90
    ape::Vec3 field_A0{};              // 0xA0
    uint32_t field_B0 = 0;             // 0xB0
    uint32_t field_B4 = 0;             // 0xB4
    bool field_BC = false;             // 0xBC
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

namespace sly {
namespace objects {
// Placement loaders of the plain scripted / actor / physical entity elements
// (0x000CC728, 0x000CC510, and inline in 0x000CC920).
EntityPlacement* ParseScriptedEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
EntityPlacement* ParseActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                     EntityDef* def);
EntityPlacement* ParsePhysicalEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
} // namespace objects
} // namespace sly
