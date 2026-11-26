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
    explicit ActorGroupDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~ActorGroupDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    float field_130[4] = {};            // 0x130
    bool field_140 = false;             // 0x140
};
// 0x00B15508, spawns HitCounterActor. NOTE: as above.
class HitCounterActorDef : public ActorDef {
public:
    explicit HitCounterActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~HitCounterActorDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    float field_130[5] = {};            // 0x130
    int32_t field_144 = -1;             // 0x144 zero-based index (-1: none)
    uint32_t flags_148 = 0x80000000u;   // 0x148 bits 30/31: placement defaults
};
// 0x00B15748, spawns WatchActor. NOTE: the fields are placement defaults.
class WatchActorDef : public ActorDef {
public:
    explicit WatchActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~WatchActorDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    uint32_t field_130 = 0;             // 0x130 hash
    uint32_t field_134 = 0;             // 0x134 hash
    float field_138 = 0.0f;             // 0x138
    uint32_t flags_13C = 0;             // 0x13C bits 30/31
};
// 0x00B15BD8, spawns SignalActor (0x180 bytes). NOTE: the fields are
// placement defaults; names unknown.
class SignalActorDef : public ActorDef {
public:
    // Copies the class fields of the description (0x001AF598).
    explicit SignalActorDef(const struct SignalActorDefDesc& desc);
    ~SignalActorDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    ape::Vec3 direction{0.0f, 0.0f, 1.0f, 0.0f};   // 0x130 unit vector
    float field_140[7] = {1.0f, 6.0f, 3.5f, 2.0f, 0.5f, 6.0f, 3.5f};   // 0x140..0x158
    float field_15C = 18.849556f;       // 0x15C radians
    float field_160 = 0.08726646f;      // 0x160 radians
    float field_164 = 0.1f;             // 0x164
    float field_168 = 0.0f;             // 0x168
    float field_16C = 0.0f;             // 0x16C
    float field_170 = 0.0f;             // 0x170
    // 0x174 top byte: zero-based index (0xFF none); bits 20-23 placement
    // default flags.
    uint32_t flags_174 = 0xFF900000u;
};
// 0x00B15E18, spawns ActorB15E60. NOTE: only the placement defaults.
class ActorB15E60Def : public ActorDef {
public:
    explicit ActorB15E60Def(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~ActorB15E60Def() override;
    uint32_t field_130[3] = {};         // 0x130 hashes
    float field_13C[5] = {1.0f, 2.0f, 1.0f, 1.0f, 0.0f};   // 0x13C..0x14C
};
// 0x00B163F0, spawns ScriptObjectActor. NOTE: only the placement defaults.
class ScriptObjectActorDef : public ActorDef {
public:
    explicit ScriptObjectActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~ScriptObjectActorDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    float field_130 = 0.0f;             // 0x130 attribute 0x7DDB32EA
    ape::Vec3 field_140{};              // 0x140 attribute 0x7DDB2524
};
SLY_ACTOR_DEF(ActorB17990Def, ActorDef);        // 0x00B17948, spawns ActorB17990
// 0x00B18680, spawns TweenActor (0x1B0 bytes). NOTE: field names unknown;
// several are placement defaults.
class TweenActorDef : public ActorDef {
public:
    explicit TweenActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~TweenActorDef() override;
    uint32_t field_130 = 0;             // 0x130 index of the "type" value
    uint32_t field_134 = 0;             // 0x134 hash 0x161851F3
    ape::Vec3 field_140{};              // 0x140 attribute 0xB8B33A30
    uint32_t field_150 = 0;             // 0x150 hash 0xEFE3F2BE
    uint32_t field_154 = 0;             // 0x154 hash 0x80F96352
    ape::Vec3 field_160{};              // 0x160 attribute 0x43DAD405
    ape::Vec3 field_170{};              // 0x170 attribute 0xE7590BDA
    ape::Vec3 field_180{};              // 0x180 attribute 0x02528C1C
    ape::Vec3 field_190{};              // 0x190 attribute 0x0252939F (angles; degrees in the file)
    uint32_t field_1A0 = 0;             // 0x1A0 hash 0x6350BDB4
    uint32_t field_1A4 = 0;             // 0x1A4 hash 0x04623C7C
    float field_1A8 = 0.0f;             // 0x1A8 attribute 0x81B5B597
};
// 0x00B188C0, spawns InteractivePropB (0x170 bytes). NOTE: field names unknown.
class InteractivePropBDef : public ActorDef {
public:
    explicit InteractivePropBDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~InteractivePropBDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7
    uint32_t field_130 = 0;                         // 0x130 hash 0x2899600F
    ape::Vec3 field_140{};                          // 0x140 attribute 0xEF4DDD13
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};     // 0x150 from attribute 0xC7400937 (degrees)
    float field_160[3] = {};                        // 0x160 attributes 0xDD61F47C, 0x3CC6000D, 0x6AA47C5A
    uint32_t field_16C = 0;                         // 0x16C hash 0x6AA4B8C7
};

// Sound actor definitions (base SoundActorDef).
// 0x00B16630. NOTE: only the placement defaults.
class SoundActorB16678Def : public SoundActorDef {
public:
    // The sound actor fields, then its own (0x001BE8CC).
    explicit SoundActorB16678Def(const struct SoundActorB16678DefDesc& desc);
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7 (0x001BEFE8)
    ~SoundActorB16678Def() override;
    uint32_t field_190 = 0;             // 0x190 starting switch position: 0 "OFF", 1 "ON" (attribute 0xC03F596F)
    float field_194 = -1.0f;            // 0x194 attribute 0xA6710C18
    bool field_198 = false;             // 0x198 attribute 0xD28465E5
    bool field_199 = false;             // 0x199 attribute 0xF0169515
};
// 0x00B16898. NOTE: only the placement defaults.
class SoundActorB168E0Def : public SoundActorDef {
public:
    // The sound actor fields, then its own (0x001C011C).
    explicit SoundActorB168E0Def(const struct SoundActorB168E0DefDesc& desc);
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7 (0x001C0A10)
    ~SoundActorB168E0Def() override;
    int32_t field_190 = -1;             // 0x190 zero-based index (attribute 0xCBC57F7D, one-based)
    uint32_t field_194 = 0;             // 0x194 hash 0x2DF2E2B5
    float field_198 = 0.0f;             // 0x198 attribute 0x08C08CA8
    // 0x19C bits 31 (0xD28465E5) and 30 (0xD02B978A): placement defaults.
    uint32_t flags_19C = 0x40000000u;
};
// 0x00B16FD0: six trigger volumes ("Trigger" children of the definition).
class SoundActorB17008Def : public SoundActorDef {
public:
    // One trigger volume (0x60 bytes). NOTE: field meanings unknown; the
    // attribute hashes are given.
    struct Volume {
        uint32_t field_00 = 0;                      // +0x00 hash 0x75849CFE
        ape::Vec3 field_10{};                       // +0x10 attribute 0x2B7B3EB4
        ape::Vec3 field_20{};                       // +0x20 attribute 0x313AB0C1
        ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f}; // +0x30 from attribute 0x313AB844 (degrees)
        bool field_40 = true;                       // +0x40 attribute 0x8141EC8A
        bool field_41 = false;                      // +0x41 attribute 0xF8E455AC
        uint32_t field_44 = 0;                      // +0x44 NOTE: the parser reads "WindType" and drops it
        float field_48 = 0.0f;                      // +0x48 attribute 0x8FA5922D
        float field_4C = 0.0f;                      // +0x4C attribute 0x3C8207B4
        float field_50 = 0.0f;                      // +0x50 attribute 0x7C8F6049
        float field_54 = 0.0f;                      // +0x54 attribute 0x8C014439
    };
    static constexpr int kVolumeCount = 6;
    // The sound actor fields, then the volumes (0x001C7F28).
    explicit SoundActorB17008Def(const struct SoundActorB17008DefDesc& desc);
    ~SoundActorB17008Def() override;
    int32_t field_190 = 0x16;                       // 0x190 NOTE: always 22
    Volume volumes[kVolumeCount];                   // 0x1A0
};
// 0x00B17228: the gadget turret's definition.
class SoundActorB17270Def : public SoundActorDef {
public:
    // The sound actor fields, then its own (0x001C9E34).
    explicit SoundActorB17270Def(const struct SoundActorB17270DefDesc& desc);
    ~SoundActorB17270Def() override;
    uint32_t aimPartName = 0;           // 0x190 hash 0x4156E113: bone (or object) the turret aims from
    uint32_t field_194 = 0;             // 0x194 hash 0x7997D9A2 (an object)
    uint32_t field_198 = 0;             // 0x198 hash 0xC72945F3 (an object)
    ape::Vec3 aimOffset{};              // 0x1A0 attribute 0xCC0D6864
    uint32_t field_1B0 = 0;             // 0x1B0 hash 0x7242FC8F
    // Placement defaults (SoundActorB17270Placement +0x110..+0x135).
    uint32_t field_1B4 = 0;             // 0x1B4 hash 0x53FFAFC2
    float field_1B8[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f};   // 0x1B8..0x1D0
    float field_1D4 = 0.34906584f;      // 0x1D4 attribute 0x2D3AC4D0 (radians; degrees in the file)
    float field_1D8 = 0.08726646f;      // 0x1D8 attribute 0x2C30B0AF (radians; degrees in the file)
    uint32_t mode = 0;                  // 0x1DC index of hash 0x04F3FC3D in {0x13234CFF, 0x8FA5D8C3}
    int32_t field_1E0 = -1;             // 0x1E0
    bool field_1E4 = true;              // 0x1E4
    bool field_1E5 = true;              // 0x1E5
    bool field_1E6 = true;              // 0x1E6 attribute 0xAD4866C3
    bool field_1E7 = false;             // 0x1E7 attribute 0xC4F817C0
};
// 0x00B17490: the sentry light definition ("SentryLight" element).
class SoundActorB174C0Def : public SoundActorDef {
public:
    // The sound actor fields, then its own (0x001CFE00).
    explicit SoundActorB174C0Def(const struct SoundActorB174C0DefDesc& desc);
    ~SoundActorB174C0Def() override;
    uint32_t field_190 = 0;             // 0x190 hash 0x503616D9
    uint32_t field_194 = 0;             // 0x194 hash 0x5B85769D
};
// 0x00B176E0
class SoundActorB17728Def : public SoundActorDef {
public:
    // The sound actor fields, then its own (0x001D3A24).
    explicit SoundActorB17728Def(const struct SoundActorB17728DefDesc& desc);
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7 (0x001D40A8)
    ~SoundActorB17728Def() override;
    uint32_t field_190 = 0;             // 0x190 placement default (hash 0x53FFAFC2)
    float field_194 = 0.0f;             // 0x194 placement default (attribute 0x667AB4F5)
    // 0x198 hash 0xCD839910: character (CharacterDef::characterName) a near player must be to
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
    explicit ObjectActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
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
