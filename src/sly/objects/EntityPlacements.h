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
#include "sly/entity/PropEntities.h"
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
    // 0x000DFDF8: the base description (no own placement defaults), type
    // bits 0 and 1, and the scripted defaults from the (scripted) definition.
    ScriptedInstanceDesc(uint32_t layerCount, uint32_t stateDataSize, const char* instanceName, EntityDef* ownerDef,
                         ape::world::ApeObj* source, const void* field28, const ape::Matrix34& initialTransform,
                         const ape::Vec3& initialScale, bool bit16, bool bit15, bool bit14, bool startVisible,
                         bool bit12, bool bit9, bool bit8, bool notListed, uint32_t field80, uint8_t player,
                         uint32_t field8C);
    ScriptedInstanceDesc(const char* instanceName, EntityDef* ownerDef);
    ape::ApeTArray<ScriptPropertyOverride> overridesA{16, nullptr, true};   // 0x90
    ape::ApeTArray<ScriptPropertyOverride> overridesB{16, nullptr, true};   // 0xA4
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
    ape::Vec3 field_00{};               // placement +0xF0, description +0xD0 (attribute 0xADDC2CDD)
    ape::Vec3 rotation{};               // +0x100 / +0xE0 radians (attribute 0x0948D097, in degrees)
    float field_20 = 0.0f;              // +0x110 / +0xF0
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
    uint32_t field_114 = 0;             // +0x114
    float field_118 = -1.0f;            // +0x118 attribute 0x0023FEF4
    float field_11C = 3.4028235e38f;    // +0x11C attribute 0xCEDD379A
    uint32_t field_120[2] = {};         // +0x120, +0x124
};
struct AlterEgoInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    PhysicalSettings settings;          // 0xD0
    uint32_t field_F4 = 0;              // 0xF4
    float field_F8 = -1.0f;             // 0xF8
    float field_FC = 3.4028235e38f;     // 0xFC
    uint32_t field_100[2] = {};         // 0x100, 0x104
};

// Placement of TargetableA's definition (vtable 0x00B14B80).
class TargetableAPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x0017F318
    // The physical attributes and the target's own (0x0017EF48).
    void Parse(ape::xml::XmlElement* elem);
    PhysicalSettings settings{{}, {}, 1.0f};   // +0xF0 (field_20: attribute 0xCEDD379A)
    float field_114 = 0.0f;             // +0x114 attribute 0x04B70827
    float field_118 = 0.5f;             // +0x118 attribute 0x001FE6E0
    float field_11C = 0.5f;             // +0x11C attribute 0x94E315E2
    float field_120 = 0.0f;             // +0x120 attribute 0x7D4D7117
    float field_124 = 0.0f;             // +0x124 attribute 0x53CAD6F8
    uint32_t field_128 = 0;             // +0x128 attribute 0x6C3F4814 (hash)
    float field_12C = 0.0f;             // +0x12C attribute 0xD1A77009
    bool field_130 = false;             // +0x130 attribute 0x07248538
    uint32_t field_134 = 0;             // +0x134 attribute 0x7DFA87B7 (hash)
    uint32_t field_138 = 0;             // +0x138 attribute 0xD49CCAAE (hash)
    bool field_13C = false;             // +0x13C attribute 0xB8A0F299
    uint32_t field_140 = 0;             // +0x140
    uint32_t field_144 = 0;             // +0x144
};
struct TargetableAInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
    PhysicalSettings settings;          // 0xD0
    float field_F4 = 0.0f;              // 0xF4
    float field_F8 = 0.0f;              // 0xF8
    float field_FC = 0.0f;              // 0xFC
    float field_100 = 0.0f;             // 0x100
    float field_104 = 0.0f;             // 0x104
    uint32_t field_108 = 0;             // 0x108
    float field_10C = 0.0f;             // 0x10C
    bool field_110 = false;             // 0x110
    uint32_t field_114 = 0;             // 0x114
    uint32_t field_118 = 0;             // 0x118
    bool field_11C = false;             // 0x11C
    uint32_t field_120 = 0;             // 0x120
};

class TargetableBPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~TargetableBPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x00191024
    // The physical attributes and the target's own (0x00190C28).
    void Parse(ape::xml::XmlElement* elem);
    PhysicalSettings settings{{}, {}, 1.0f};   // +0xF0 (field_20: attribute 0xCEDD379A)
    float field_114 = 10.0f;            // +0x114 attribute 0x04B70827
    float field_118 = 1.0f;             // +0x118 attribute 0x9FC855CA
    float field_11C = 40.0f;            // +0x11C attribute 0xB595B151
    bool field_120 = false;             // +0x120 attribute 0x7883C439
    uint32_t field_124 = 0;             // +0x124 attribute 0x4578545F (hash)
    uint32_t field_128 = 0;             // +0x128 attribute 0x00241C53 (hash)
    float field_12C = 0.0f;             // +0x12C attribute 0x5161B474
    float field_130 = 1.0f;             // +0x130 attribute 0x8FA16092
    float field_134 = 0.0f;             // +0x134 attribute 0x7ECB8527
    float field_138 = 0.0f;             // +0x138 attribute 0xDBAF7E49
    float field_13C = 0.0f;             // +0x13C attribute 0x00DE534E
    bool field_140 = true;              // +0x140 attribute 0x641217B3
    uint32_t field_144 = 0;             // +0x144
    uint32_t field_148 = 0;             // +0x148
};
struct TargetableBInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_C8 = 0;              // 0xC8
    PhysicalSettings settings;          // 0xD0
    float field_F4 = 0.0f;              // 0xF4
    float field_F8 = 0.0f;              // 0xF8
    float field_FC = 0.0f;              // 0xFC
    bool field_100 = false;             // 0x100
    uint32_t field_104 = 0;             // 0x104
    uint32_t field_108 = 0;             // 0x108
    float field_10C = 0.0f;             // 0x10C
    float field_110 = 0.0f;             // 0x110
    float field_114 = 0.0f;             // 0x114
    float field_118 = 0.0f;             // 0x118
    float field_11C = 0.0f;             // 0x11C
    bool field_120 = false;             // 0x120
    uint32_t field_124 = 0;             // 0x124
};

#define SLY_PLACEMENT(Name, Base)                 \
    class Name : public Base {                    \
    public:                                       \
        using Base::Base;                         \
        ~Name() override;                         \
    }

// Vtable 0x00B18B30. NOTE: its copy (slot 2) is not ported.
class TargetableCPlacement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~TargetableCPlacement() override;
    PhysicalSettings settings;          // +0xF0
    float field_114 = -1.0f;            // +0x114 attribute 0x0023FEF4
    float field_118 = 3.4028235e38f;    // +0x118 attribute 0xCEDD379A
    bool field_11C = false;             // +0x11C
};
SLY_PLACEMENT(TargetableCLinkedPlacement, ScriptedEntityPlacement); // 0x00B18F48
// Vtable 0x00B19178. NOTE: its copy (slot 2) is not ported.
class RigidEntityB19198Placement : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~RigidEntityB19198Placement() override;
    PhysicalSettings settings;          // +0xF0
    uint32_t field_114 = 0;             // +0x114
    // Attributes 0x926425E9, 0x7F6CE437, 0x918F1125.
    bool field_118[3] = {};             // +0x118..+0x11A
};

#undef SLY_PLACEMENT

// Actor placements and their descriptions (vtables in the comments).

class ActorGroupPlacement : public ActorPlacement {   // 0x00B152F8
public:
    using ActorPlacement::ActorPlacement;
    ~ActorGroupPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001AA57C
    // Attributes 0x4E14723F, 0x6A96C7DB, 0xEAEDFCE1, 0x0A924DA2.
    float field_F0[4] = {};             // +0xF0..+0xFC
    bool field_100 = false;             // +0x100 attribute 0x317F6BB4
};
struct ActorGroupInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[4] = {};             // 0xD0..0xDC
    bool field_E0 = false;              // 0xE0
};

class HitCounterActorPlacement : public ActorPlacement {   // 0x00B15538
public:
    using ActorPlacement::ActorPlacement;
    ~HitCounterActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001ABA70
    // Attributes 0x8722570B, 0x1E6104E5, 0x08B6A993, 0x14EE18CD, 0x4E14723F.
    float field_F0[5] = {};             // +0xF0..+0x100
    // +0x104 bits 30/31 are handed on (attributes 0x994327AD / 0x609A61B6).
    uint32_t flags_104 = 0;
};
struct HitCounterActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[5] = {};             // 0xD0..0xE0
    uint32_t flags_E4 = 0;              // 0xE4
};

// Sound actors: seven sound names and a word.
class SoundActorPlacement : public ActorPlacement {   // 0x00B159A0
public:
    using ActorPlacement::ActorPlacement;
    ~SoundActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001AE3B4
    // The scripted attributes and the sound actor's own (inlined in every
    // sound actor placement loader).
    void Parse(ape::xml::XmlElement* elem);
    // One value per sound slot (-FLT_MAX: unset). NOTE: role assumed to be
    // the slot's time; attributes 0xD5F932F0, 0xEABE8E36, 0x800ACF3A,
    // 0xF9217A56, 0x7BA17740, 0xA9262FCF, 0xA88C3581.
    float slotTimes[7] = {-3.4028235e38f, -3.4028235e38f, -3.4028235e38f, -3.4028235e38f, -3.4028235e38f,
                          -3.4028235e38f, -3.4028235e38f};   // +0xF0..+0x108
    float field_10C = 0.0f;             // +0x10C attribute 0x7B2F95A0
};
struct SoundActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float slotTimes[7] = {};            // 0xD0..0xE8
    float field_EC = 0.0f;              // 0xEC
};

class SignalActorPlacement : public ActorPlacement {   // 0x00B15C08
public:
    using ActorPlacement::ActorPlacement;
    ~SignalActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001B0654
    // Attributes 0x26903F9C, 0xC3017FF2 (degrees) and 0xAA8C2237.
    float field_F0[3] = {};             // +0xF0..+0xF8
    // +0xFC bits 30/31 are handed on (attributes 0x3AF62E9E / 0x4D97C801).
    uint32_t flags_FC = 0;
};
struct SignalActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[3] = {};             // 0xD0..0xD8
    uint32_t flags_DC = 0;              // 0xDC
};

class ActorB15E60Placement : public ActorPlacement {   // 0x00B15E48
public:
    using ActorPlacement::ActorPlacement;
    ~ActorB15E60Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001B29B8
    // Attributes 0xCEA8E637, 0x06CD2139, 0x0DE78502, 0x0B0BD5E6, 0x285C1380.
    float field_F0[5] = {};             // +0xF0..+0x100
};
struct ActorB15E60InstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[5] = {};             // 0xD0..0xE0
};

// ScriptObjectActor uses the scripted copy directly.
class ScriptObjectActorPlacement : public ActorPlacement {   // 0x00B16420
public:
    using ActorPlacement::ActorPlacement;
    ~ScriptObjectActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001BDFE8
    float field_F0[3] = {};             // +0xF0..+0xF8 attribute 0x7DDB2524
    float field_100 = 0.0f;             // +0x100 attribute 0x7DDB32EA
    float field_104 = -1.0f;            // +0x104 attribute 0xE14281CC (not handed on)
    // Attributes 0x7D0D427B, 0xE019565D, 0x846B9DA2.
    bool field_108[3] = {};             // +0x108..+0x10A
};
struct ScriptObjectActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[3] = {};             // 0xD0..0xD8
    float field_E0 = 0.0f;              // 0xE0
    bool field_E8[3] = {};              // 0xE8..0xEA
};

class SoundActorB16678Placement : public SoundActorPlacement {   // 0x00B16660
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16678Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001BF6C0
    // The sound actor attributes and the switch's own (0x001BF2A0).
    void Parse(ape::xml::XmlElement* elem);
    uint32_t switchState = 0;           // +0x110 0 "off", 1 "on" (attribute 0xC03F596F)
    float field_114 = -1.0f;            // +0x114 attribute 0xA6710C18
    bool field_118[2] = {};             // +0x118, +0x119 attributes 0xD28465E5, 0xF0169515
};
struct SoundActorB16678InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t switchState = 0;           // 0xF0
    float field_F4 = -1.0f;             // 0xF4
    bool field_F8[2] = {};              // 0xF8, 0xF9
};

class SoundActorB168E0Placement : public SoundActorPlacement {   // 0x00B168C8
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB168E0Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C103C
    // The sound actor attributes and its own (0x001C0C64).
    void Parse(ape::xml::XmlElement* elem);
    float field_110 = 0.0f;             // +0x110 attribute 0x08C08CA8
    // +0x114 bits 30/31 are handed on (attributes 0xD02B978A / 0xD28465E5).
    uint32_t flags_114 = 0;
};
struct SoundActorB168E0InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    float field_F0 = 0.0f;              // 0xF0
    uint32_t flags_F4 = 0;              // 0xF4
};

class SoundActorB16B48Placement : public SoundActorPlacement {   // 0x00B16B30
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16B48Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C3308
    float field_110 = 0.0f;             // +0x110 attribute 0x2DF33CE3
};
struct SoundActorB16B48InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    float field_F0 = 0.0f;              // 0xF0
};

// One of the six tracked triggers of SoundActorB16DB0 (0x50 bytes; child
// elements 0xEB9EE198).
struct SoundActorB16DB0Record {
    uint32_t name = 0;                  // +0x00 attribute 0x75849CFE
    ape::Vec3 field_10{};               // +0x10 attribute 0x2B7B3EB4
    ape::Vec3 field_20{};               // +0x20 attribute 0x313AB0C1
    // +0x30 orientation (attribute 0x313AB844, Euler angles in degrees).
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t field_40 = 0;              // +0x40 attribute 0x632DA119
    bool field_44 = true;               // +0x44 attribute 0x8141EC8A
    bool field_45 = false;              // +0x45 attribute 0xF8E455AC
};
struct SoundActorB16DB0Records {
    SoundActorB16DB0Record records[6];
};
class SoundActorB16DB0Placement : public SoundActorPlacement {   // 0x00B16D98
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB16DB0Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001C6FB0
    // The sound actor attributes and the tracked triggers (0x001C6838).
    void Parse(ape::xml::XmlElement* elem);
    uint32_t field_110 = 0;             // +0x110 attribute 0x2DF2E2B5 (hash)
    uint32_t field_114 = 0;             // +0x114 attribute 0x53FFAFC2 (hash)
    float field_118 = -1.0f;            // +0x118 attribute 0x667AB4F5
    float field_11C = -1.0f;            // +0x11C attribute 0x7A392528
    SoundActorB16DB0Records records{};  // +0x120
    // +0x300 bits 30/31 are handed on: 31 set when records were given, 30
    // attribute 0xA8DB95B1.
    uint32_t flags_300 = 0;
};
struct SoundActorB16DB0InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0 = 0;              // 0xF0
    uint32_t field_F4 = 0;              // 0xF4
    float field_F8 = -1.0f;             // 0xF8
    float field_FC = -1.0f;             // 0xFC
    SoundActorB16DB0Records records{};  // 0x100
    uint32_t flags_2E0 = 0;             // 0x2E0
};

class SoundActorB17270Placement : public SoundActorPlacement {   // 0x00B17258
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB17270Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001CBD04
    // The sound actor attributes and the turret's own (0x001CB740).
    void Parse(ape::xml::XmlElement* elem);
    uint32_t field_110 = 0;             // +0x110 attribute 0x53FFAFC2 (hash)
    // +0x114..+0x12C attributes 0x09198A77, 0x58E70C1B, 0x0B109E97,
    // 0x0B10795B, 0x0B13D227 (those three not negative), 0x667AB4F5, 0xDBA26CDA.
    float field_114[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f};
    int32_t field_130 = -1;             // +0x130 attribute 0x9F5A4080
    bool field_134[2] = {true, true};   // +0x134, +0x135 attributes 0x3EFBF433, 0x4128CDCD
};
struct SoundActorB17270InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0 = 0;              // 0xF0
    float field_F4[7] = {};             // 0xF4..0x10C
    int32_t field_110 = -1;             // 0x110
    bool field_114[2] = {};             // 0x114, 0x115
};

class SoundActorB17728Placement : public SoundActorPlacement {   // 0x00B17710
public:
    using SoundActorPlacement::SoundActorPlacement;
    ~SoundActorB17728Placement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001D4620
    uint32_t field_110 = 0;             // +0x110 attribute 0x53FFAFC2 (hash)
    float field_114 = 0.0f;             // +0x114 attribute 0x667AB4F5
};
struct SoundActorB17728InstanceDesc : SoundActorInstanceDesc {
    using SoundActorInstanceDesc::SoundActorInstanceDesc;
    uint32_t field_F0 = 0;              // 0xF0
    float field_F4 = 0.0f;              // 0xF4
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
    float field_F0[2] = {};             // +0xF0, +0xF4 radians (attribute 0xBF3F0DB1, in degrees)
};
struct InteractivePropAInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[2] = {};             // 0xD0, 0xD4
};

class SafePlacement : public ActorPlacement {   // 0x00B18020
public:
    using ActorPlacement::ActorPlacement;
    ~SafePlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001E6F3C
    uint32_t field_F0 = 0;              // +0xF0 attribute 0x06556689 (hash)
    float field_100[3] = {};            // +0x100..+0x108 attribute 0xC88E4511
    uint32_t field_110 = 0;             // +0x110 attribute 0x675C8395 (hash)
};
struct SafeInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    uint32_t field_D0 = 0;              // 0xD0
    float field_E0[3] = {};             // 0xE0..0xE8
    uint32_t field_F0 = 0;              // 0xF0
};

class TweenActorPlacement : public ActorPlacement {   // 0x00B186B0
public:
    using ActorPlacement::ActorPlacement;
    ~TweenActorPlacement() override;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;   // 0x001ED4F0
    // The scripted attributes and the tween's own (0x001ED004).
    void Parse(ape::xml::XmlElement* elem);
    float field_F0[2] = {};             // +0xF0 attribute 0x64231FD8
    float field_F8[2] = {};             // +0xF8 attribute 0x64231EEA
    // +0x100..+0x120 attributes 0x161851F3, 0xEFE3F2BE, 0x80F96352, 0x93116591,
    // 0x3F7939FE, 0x04623C7C, 0xDEC8B85D, 0xF24070D0, 0x6350BDB4 (hashes).
    uint32_t names[9] = {};
    float field_124 = 0.0f;             // +0x124 attribute 0xC6BA9BDE
    float field_128 = 0.0f;             // +0x128 attribute 0xBE5301F3
    float field_12C = 0.0f;             // +0x12C attribute 0x48CDF4C0
    float field_130 = 0.0f;             // +0x130 attribute 0x81B5B597
    // +0x134..+0x13B attributes 0x6C2C79EC, 0x33DA09EF, 0xF9C0C1B5,
    // 0x1E85A989, 0xD6F80AE3, 0x850F9FE3, 0x4DE83B5A, 0x6BDE6A90.
    bool field_134[8] = {false, true, false, true, true, false, false, false};
};
struct TweenActorInstanceDesc : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    float field_D0[2] = {};             // 0xD0
    float field_D8[2] = {};             // 0xD8
    uint32_t names[9] = {};             // 0xE0..0x100
    float field_104 = 0.0f;             // 0x104
    float field_108 = 0.0f;             // 0x108
    float field_10C = 0.0f;             // 0x10C
    float field_110 = 0.0f;             // 0x110
    bool field_114[8] = {};             // 0x114..0x11B
};

#define SLY_PLACEMENT(Name, Base)                 \
    class Name : public Base {                    \
    public:                                       \
        using Base::Base;                         \
        ~Name() override;                         \
    }

// Vtable 0x00B188F0. NOTE: its copy (slot 2) is not ported.
class InteractivePropBPlacement : public ActorPlacement {
public:
    using ActorPlacement::ActorPlacement;
    ~InteractivePropBPlacement() override;
    // The scripted attributes and the prop's own (0x001F5E50).
    void Parse(ape::xml::XmlElement* elem);
    uint32_t field_F0 = 0;              // +0xF0 attribute 0x2899600F (hash)
    ape::Vec3 field_100{};              // +0x100 attribute 0xEF4DDD13
    // +0x110 orientation (attribute 0xC7400937, Euler angles in degrees).
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float field_120 = 0.0f;             // +0x120 attribute 0xDD61F47C
    float field_124 = 0.0f;             // +0x124 attribute 0x3CC6000D
    float field_128 = 0.0f;             // +0x128 attribute 0x6AA47C5A
    uint32_t field_12C = 0;             // +0x12C attribute 0x6AA4B8C7 (hash)
    bool field_130 = false;             // +0x130 attribute 0xEA3BBF97
};

#undef SLY_PLACEMENT

// Placement of the push blocks (definitions with type bit 0x20000; vtable
// 0x00B16088, 0x110 bytes; element 0x786E8593). NOTE: the meaning of the
// points is unknown.
class PlacementB16088 : public ScriptedEntityPlacement {
public:
    using ScriptedEntityPlacement::ScriptedEntityPlacement;
    ~PlacementB16088() override;
    // The scripted attributes, the word and up to four points (children
    // 0x65AF9A49, attribute 0x01968AD4) (0x001B5438).
    void Parse(ape::xml::XmlElement* elem);

    using Entry = entity::PushBlockDef::Point;
    // Slot 2 (0x001B57A4): the scripted copy, then the entries (appended)
    // and the word.
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::ApeTArray<Entry> field_EC{4, nullptr, true};   // +0xEC copied from the definition (+0xC0)
    float field_100 = 0.0f;            // +0x100 from the definition (+0xE8); attribute 0xE6DF8EF6
};
struct InstanceDescB16088 : ScriptedInstanceDesc {
    using ScriptedInstanceDesc::ScriptedInstanceDesc;
    ape::ApeTArray<PlacementB16088::Entry> field_C8;   // 0xC8
    float field_DC = 0.0f;             // 0xDC
};

// Placement of the definitions with registry flag 0x40 (vtable 0x00B138E8,
// 0xD0 bytes; element 0xEBE54755): a binding list and a script name.
// NOTE: the entity class is not identified.
class PlacementB138E8 : public EntityPlacement {
public:
    // 0x000CBB40: the base placement, the script name from the (scripted)
    // definition's field_B4.
    PlacementB138E8(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                    const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31, bool bit30,
                    uint32_t field74 = 0);
    ~PlacementB138E8() override;
    // Slot 2 (0x000DE454): the base copy, the word and the bindings (appended).
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    ape::ApeTArray<ScriptPropertyOverride> bindings{16, nullptr, true};   // +0xB4 (child element 0xC3B709C0)
    uint32_t field_C8 = 0;             // +0xC8 attribute 0xF89FDD94 (hash)
};
struct InstanceDescB138E8 : EntityInstanceDesc {
    using EntityInstanceDesc::EntityInstanceDesc;
    ape::ApeTArray<ScriptPropertyOverride> bindings;   // 0x90
    uint32_t field_A4 = 0;             // 0xA4
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
// The placement loader (0x000CC920): builds the placement of a level file's
// entity element by its element name (with the "name" attribute and the
// definition named by "associatedDef" when it is of the right class), reads
// the common attributes and attachment records and registers it by name.
// Null for unnamed or already registered placements and unknown elements.
EntityPlacement* LoadEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);

// Placement loaders of the plain scripted / actor / physical entity elements
// (0x000CC728, 0x000CC510, and inline in 0x000CC920).
EntityPlacement* ParseScriptedEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
EntityPlacement* ParseActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                     EntityDef* def);
EntityPlacement* ParsePhysicalEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
// Placement loaders of the actor group / hit counter, chain and rope
// elements (0x001AA360, 0x001AB7A0, 0x001A7C70, 0x001BB830).
EntityPlacement* ParseActorGroupPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def);
EntityPlacement* ParseHitCounterActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                               const char* name, EntityDef* def);
EntityPlacement* ParseChainPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                     EntityDef* def);
EntityPlacement* ParseRopePlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                    EntityDef* def);
// Sound, signal and 0x00B15E48 actor placement loaders (0x001AE130,
// 0x001B0398, 0x001B2718).
EntityPlacement* ParseSoundActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def);
EntityPlacement* ParseSignalActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def);
EntityPlacement* ParseActorB15E60Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def);
// Script object actor and two sound actor placement loaders (0x001BDDC0,
// 0x001BF4E0, 0x001C0E70).
EntityPlacement* ParseScriptObjectActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                 const char* name, EntityDef* def);
EntityPlacement* ParseSoundActorB16678Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
EntityPlacement* ParseSoundActorB168E0Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
// Light, tracked-trigger and turret sound actor placement loaders
// (0x001C3040, 0x001C6D30, 0x001CBA90).
EntityPlacement* ParseSoundActorB16B48Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
EntityPlacement* ParseSoundActorB16DB0Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
EntityPlacement* ParseSoundActorB17270Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
// Door sound actor and interactive prop placement loaders (0x001D4328,
// 0x001DE9D8).
EntityPlacement* ParseSoundActorB17728Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
EntityPlacement* ParseInteractivePropAPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
// Safe placement loader (0x001E6D78).
EntityPlacement* ParseSafePlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                    EntityDef* def);
// Tween actor and InteractivePropB placement loaders (0x001ED358, 0x001F60D8).
EntityPlacement* ParseTweenActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def);
EntityPlacement* ParseInteractivePropBPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def);
// TargetableC and 0x00B19178 rigid entity placement loaders (0x001FAB38,
// 0x002048A0).
EntityPlacement* ParseTargetableCPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def);
EntityPlacement* ParseRigidEntityB19198Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                 const char* name, EntityDef* def);
// Breakable chain placement loader (0x001A5C90).
EntityPlacement* ParseBreakableChainPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
// Push block placement loader (0x001B55C8).
EntityPlacement* ParsePushBlockPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                         const char* name, EntityDef* def);
// Placement loaders of the rigid-body scripted classes (0x00167F58,
// 0x0016CBE0, 0x00177D78, 0x0017F1A0, 0x00190EA0).
EntityPlacement* ParseRigidPropPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                         EntityDef* def);
EntityPlacement* ParseCompoundEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def);
EntityPlacement* ParseAlterEgoPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                        EntityDef* def);
EntityPlacement* ParseTargetableAPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def);
EntityPlacement* ParseTargetableBPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def);
} // namespace objects
} // namespace sly
