// openTIT - Entity definitions, placements and instance descriptions.
//
// Every gameplay object is spawned in three steps:
//   * the definition loader (0x000CA038) reads an entity XML element into an
//     EntityDefDesc and the per-class factory builds an EntityDef from it
//     (console vtable 0x00B12230). Definitions are registered by name in the
//     definition map (0x01013B04);
//   * the placement loader (0x000CC920) reads one placed instance into an
//     EntityPlacement ("settings", console vtable 0x00B12260) of the
//     definition's class; code that spawns at run time fills one on the stack;
//   * the definition's Spawn (slot 7) has the placement copy itself into an
//     EntityInstanceDesc (placement slot 2) and constructs the entity from it.
//
// The objects module (triggers, ropes, chains, ...) and the entity module use
// these same classes.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::world { class ApeDef; class ApeObj; }
namespace ape::xml { class XmlElement; }

namespace sly {
namespace entity {

class Entity;
class EntityPlacement;
struct ComponentDesc;

// Attachment record a definition hands to every placement made from it and
// the placement to its instance (0xC0 bytes on the console; elements
// 0xCCEB9AF9, nested). NOTE: copies share the nested list, as on the console.
struct EntityAttachmentDesc;
using EntityAttachmentList = ape::ApeTArray<EntityAttachmentDesc>;
struct EntityAttachmentDesc {
    // Flags (+0xB0).
    enum Flags : uint32_t {
        kAttach_Flag36E7D729 = 0x80000000u,   // attribute 0x36E7D729
        kAttach_FlagD1A6825E = 0x10000000u,   // attribute 0xD1A6825E
        kAttach_FlagEAE49DB2 = 0x01000000u,   // attribute 0xEAE49DB2
    };
    char name[0x40];                  // 0x00 "name": entity definition / object attached
    char nameOverride[0x40];          // 0x40 "nameOverride": instance name to give it
    ape::Vec3 offset;                 // 0x80 attribute 0xEF4DDD13
    ape::Vec4 rotation;               // 0x90 quaternion (attribute 0xC7400937, Euler degrees)
    uint32_t part;                    // 0xA0 attribute 0x20C00073 (hash): attach part
    uint32_t field_A4;                // 0xA4
    uint32_t field_A8;                // 0xA8 attribute 0xF91B96E2 (hash)
    uint32_t field_AC;                // 0xAC
    uint32_t flags;                   // 0xB0 Flags
    EntityAttachmentList* children;   // 0xB4 attachments of the attached entity
};
// Reads the attachment records under `elem` into `list` (created on the
// first), recursing into each record's own attachment records (0x000C9AB8).
void ParseEntityAttachments(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, EntityAttachmentList*& list);

// Class bits of a definition (EntityDefDesc::classBits, EntityDef::classBits).
// Each concrete definition class sets its own combination; a "from" parent is
// recognised by them.
enum DefClassBits : uint32_t {
    kDefClass_Trigger      = 0x00400000,  // trigger family (never in the property registry)
    kDefClass_Sound        = 0x01000000,  // ambient sound / sound zone triggers
    kDefClass_08000000     = 0x08000000,  // NOTE: set by damage/death/filtered/... triggers
};

// ---------------------------------------------------------------------------
// Definition description: the block a definition is built from (0x4C bytes on
// the console, extended by every class).
// ---------------------------------------------------------------------------
struct EntityDefDesc {
    uint32_t classBits = 0;        // 0x00 DefClassBits
    uint32_t typeBits = 0;         // 0x04 class-specific type bit
    // 0x08 registration flags (bit 1: keep out of the property registry).
    uint32_t registryFlags = 0;
    const char* name = "";         // 0x0C definition name
    const char* geomName = "";     // 0x10 geometry definition of the class (hashed)
    const char* templateName = ""; // 0x14 placed object the definition is modelled on (hashed)
    // 0x18..0x1F option flags; flag i becomes bit 31-i of EntityDef::flags.
    bool flags[8] = {};
    uint32_t field_20 = 0;         // 0x20 bit 0 becomes EntityDef::flags bit 23
    float field_24 = -1.0f;        // 0x24 placement default
    float field_28 = -1.0f;        // 0x28 placement default
    bool field_2C = false;         // 0x2C
    // 0x30 type hash under which the property registry files the definition
    // (one of the seven at 0x00AE9490). NOTE: role assumed from 0x00490360.
    uint32_t propertyType = 0;
    uint32_t field_34 = 0;         // 0x34
    uint32_t havokSkeleton = 0;    // 0x38 Havok skeleton the geometry's bones are mapped to
    uint32_t field_3C = 0;         // 0x3C
    // 0x40 names of the component templates the definition's entities get
    // (the loader's list; may be null or empty).
    ape::ApeTArray<uint32_t>* const* componentNames = nullptr;
    uint32_t field_44 = 0;         // 0x44
    // 0x48 bounding radius forced on the template object (when positive and
    // the object's definition has a radius).
    float boundingRadius = -1.0f;

    EntityDefDesc() = default;
    // The inlined initialiser every per-class factory starts with: class bits of
    // the new class, everything else at its default, the name from the loader.
    EntityDefDesc(uint32_t bits, const EntityDefDesc& loader) : classBits(bits), name(loader.name) {}
};

// Copies the fields a definition inherits from the loader's description
// (0x000C387C): the flag bytes and the shared scalar fields.
void InheritDefDesc(const EntityDefDesc& from, EntityDefDesc& to);

// ---------------------------------------------------------------------------
// Definition base (console 0x8C bytes, vtable 0x00B12230).
// ---------------------------------------------------------------------------
class EntityDef {
public:
    // Flag word (+0x20): desc.flags[i] is bit 31-i, desc.field_20 bit 23.
    enum Flags : uint32_t {
        kDefFlag_Desc0      = 1u << 31,
        kDefFlag_Desc1      = 1u << 30,   // copied into instance descriptions (flag 14)
        kDefFlag_Desc2      = 1u << 29,
        kDefFlag_Desc3      = 1u << 28,
        kDefFlag_SleepsAway = 1u << 27,   // sleeps unless a player is near (update-enable volumes)
        kDefFlag_Desc5      = 1u << 26,   // copied into placements (bits84 bit 29)
        kDefFlag_Desc6      = 1u << 25,
        kDefFlag_Desc7      = 1u << 24,
        kDefFlag_Field20    = 1u << 23,
    };

    // 0x000C26D8: copies the description, resolves the geometry / template
    // object, builds the component list from the named templates, the bone
    // map of the Havok skeleton, and registers the definition by name.
    explicit EntityDef(const EntityDefDesc& desc);
    virtual ~EntityDef();

    // Slot 2: name shown for the definition.
    // @func 0x007435C0 EntityDef::GetDisplayName
    virtual const char* GetDisplayName() const { return name; }
    // Slot 3 (0x000CB934): local bounds of the definition's entities: the
    // template object's geometry box scaled by the object's scale, a point
    // at the origin without one.
    virtual void GetBounds(ape::Aabb& out) const;
    // Slots 4-6. NOTE: meanings unknown; scripted definitions return their
    // words at +0xB4/+0xB8 from slots 4/5.
    // @func 0x00746F4C EntityDef::DefSlot4
    virtual uint32_t DefSlot4() const { return 0; }
    // @func 0x00746F54 EntityDef::DefSlot5
    virtual uint32_t DefSlot5() const { return 0; }
    // @func 0x007372F4 EntityDef::DefSlot6
    virtual uint32_t DefSlot6() const { return 0; }
    // Slot 7: builds an entity for a placement (pure on the console; null by
    // default here for the stand-in classes that do not spawn yet).
    virtual Entity* Spawn(EntityPlacement* placement) { (void)placement; return nullptr; }
    // Slot 8 (0x000CBA88): name for a new instance: the placement's name,
    // else its source object's name, else "<def name>_instNNNNN".
    virtual const char* InstanceName(const EntityPlacement* placement);

    const char* MakeInstanceName();
    // Component description of `type` after `after` (or from the start).
    ComponentDesc* FindComponentDesc(uint32_t type, ComponentDesc* after = nullptr) const;
    bool IsClass(uint32_t bits) const { return (classBits & bits) == bits; }
    bool HasFlag(uint32_t flag) const { return (flags & flag) != 0; }

    EntityDef(const EntityDef&) = delete;
    EntityDef& operator=(const EntityDef&) = delete;

protected:
    // NOTE: for the definition classes whose console constructors are not
    // ported yet (they leave every field at its default and register nothing).
    EntityDef() = default;

public:
    ape::world::ApeDef* geomDef = nullptr;          // 0x04 geometry definition named by the description
    // 0x08 placed object the definition is modelled on (its geometry gives
    // the default bounds); only looked up without a geometry definition.
    ape::world::ApeObj* templateObject = nullptr;
    int32_t instanceCounter = 0;                    // 0x0C
    ape::ApeTArray<ComponentDesc*>* componentDescs = nullptr;   // 0x10 (allocated on the first)
    uint32_t classBits = 0;                         // 0x14 DefClassBits
    uint32_t typeBits = 0;                          // 0x18 class-specific type bit
    uint32_t registryFlags = 0;                     // 0x1C see EntityDefDesc
    uint32_t flags = 0;                             // 0x20 Flags
    float field_24 = -1.0f;                         // 0x24 placement default (EntityPlacement::field_7C)
    float field_28 = -1.0f;                         // 0x28 placement default (EntityPlacement::field_80)
    uint32_t geomNameHash = 0;                      // 0x2C
    uint32_t templateNameHash = 0;                  // 0x30
    uint32_t nameHash = 0;                          // 0x34
    char name[0x40] = {};                           // 0x38
    uint32_t field_78 = 0;                          // 0x78 EntityDefDesc::field_34
    uint32_t havokSkeleton = 0;                     // 0x7C
    // 0x80 Havok bone index of each geometry bone (BuildHavokBoneMap; owned).
    uint16_t* boneMap = nullptr;
    uint32_t field_84 = 0;                          // 0x84 EntityDefDesc::field_3C
    EntityAttachmentList* attachments = nullptr;    // 0x88 records copied into every placement
};

void RegisterEntityDef(EntityDef* def);
void UnregisterEntityDef(EntityDef* def);
EntityDef* FindEntityDef(uint32_t nameHash);
// Slot 3 of the base, callable without the virtual dispatch.
void EntityDefBaseBounds(const EntityDef* def, ape::Aabb& out);

// ---------------------------------------------------------------------------
// Instance description (console block of 0x90 bytes, extended per class).
// ---------------------------------------------------------------------------
struct EntityInstanceDesc {
    // Flag word (+0x14): bits 17-31 the number of state layers, the rest
    // options the entity constructor turns into entity flags.
    enum Flags : uint32_t {
        kDesc_NotListed      = 1u << 6,    // the entity stays out of the global entity list
        kDesc_NoObjectSync   = 1u << 7,    // entity flag kEnt_NoObjectSync
        kDesc_Bit8           = 1u << 8,    // entity flags 14 and 15
        kDesc_Bit9           = 1u << 9,    // entity flag 16
        kDesc_PlacementBit29 = 1u << 11,   // placement bits84 bit 29: the entity starts hidden
        kDesc_PlacementBit30 = 1u << 12,   // bit 30: entity flag kEnt_80
        kDesc_PlacementBit31 = 1u << 13,   // bit 31: the entity starts visible
        kDesc_Bit14          = 1u << 14,   // the definition's kDefFlag_Desc1 when there is a definition
        kDesc_Bit15          = 1u << 15,
        kDesc_Bit16          = 1u << 16,
        kDesc_Sound          = 1u << 19,   // set by the sound actor placements
        kDesc_LayerShift     = 17,
    };

    // 0x000CDD28 (thunk 0x000CDFB0). Without a definition `field84` and
    // `field88` are used, otherwise the definition's placement defaults;
    // `bit14` likewise gives way to the definition's kDefFlag_Desc1.
    // `player` is the player slot (0xFF for none). The attachment records of
    // the definition are copied.
    EntityInstanceDesc(float field84, float field88, uint32_t layerCount, uint32_t stateDataSize,
                       const char* instanceName, EntityDef* ownerDef, ape::world::ApeObj* source, const void* field28,
                       const ape::Matrix34& initialTransform, const ape::Vec3& initialScale, bool bit16, bool bit15,
                       bool bit14, bool startVisible, bool bit12, bool bit9, bool bit8, bool notListed,
                       uint32_t field80, uint8_t player, uint32_t field8C);
    // The arguments every trigger Spawn passes: one state layer, no source
    // object, identity transform, unit scale, no player.
    EntityInstanceDesc(const char* instanceName, EntityDef* ownerDef);
    virtual ~EntityInstanceDesc();
    EntityInstanceDesc(const EntityInstanceDesc&) = delete;
    EntityInstanceDesc& operator=(const EntityInstanceDesc&) = delete;

    uint32_t StateLayers() const { return flags >> kDesc_LayerShift; }

    // 0x00 attachment records of the placement (created on the first one).
    EntityAttachmentList* attachments = nullptr;
    uint32_t typeBits[3] = {};          // 0x04 entity type bits (Entity::typeBits)
    uint32_t stateDataSize = 0;         // 0x10 bytes of state data per layer
    uint32_t flags = 0;                 // 0x14 Flags
    uint8_t player = 0xFF;              // 0x18 player slot (top byte of the entity flags)
    const char* name = nullptr;         // 0x1C
    EntityDef* def = nullptr;           // 0x20
    ape::world::ApeObj* sourceObject = nullptr;   // 0x24 placement sourceObject
    const void* field_28 = nullptr;     // 0x28 NOTE: triggers pass a global of the entity module
    ape::Matrix34 transform{};          // 0x30
    ape::Vec3 scale{1.0f, 1.0f, 1.0f, 0.0f};   // 0x70 placement field_60
    uint32_t field_80 = 0;              // 0x80 placement field_74 (entity +0xDC)
    float field_84 = 1.0f;              // 0x84 (entity +0xD0 squared)
    float field_88 = 1.0f;              // 0x88
    uint32_t renderId = 0;              // 0x8C placement field_88 (kept when that is 0); 0 or -1: a fresh one
};

// ---------------------------------------------------------------------------
// Placement ("settings") base (console vtable 0x00B12260; 0xB4 bytes).
// ---------------------------------------------------------------------------
class EntityPlacement {
public:
    // Flags (+0x04).
    enum Flags : uint32_t {
        kPlace_Scripted     = 0x00000002,   // a ScriptedEntityPlacement
        kPlace_Actor        = 0x00000004,   // an ActorPlacement
        kPlace_Physical     = 0x00002000,   // a PhysicalEntityPlacement
        kPlace_Trigger      = 0x00400000,   // a TriggerPlacement
        kPlace_ForceCreate  = 0x02000000,   // NOTE: overrides a placement's "skip" bit
    };

    // The full base constructor (inlined into every placement constructor).
    // NOTE: the meanings of the two bits and `field74` are unknown; most
    // placements pass true and false.
    EntityPlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                    const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31,
                    bool bit30, uint32_t field74);
    EntityPlacement(const char* placementName, EntityDef* ownerDef);
    // A placement without definition or name (run-time spawns fill it in).
    EntityPlacement() : EntityPlacement(nullptr, nullptr) {}
    // Releases the attachment array (+0x10).
    virtual ~EntityPlacement();
    // Slot 2 (base 0x000CDB28): copies the placement into an instance description.
    virtual void FillInstanceDesc(EntityInstanceDesc& desc) const;

    // Makes `def` the placement's definition with its defaults, and copies
    // its attachment records unless the caller already supplied some
    // (inlined in SpawnEntity 0x000D6D28).
    void AdoptDef(EntityDef* def);

protected:
    // Member-wise, for the derived placements' copies. NOTE: the copy shares
    // `attachments` (as on the console).
    EntityPlacement(const EntityPlacement&) = default;
    EntityPlacement& operator=(const EntityPlacement&) = default;

public:
    uint32_t flags = 0;                // 0x04 Flags
    uint32_t typeBits = 0;             // 0x08 class-specific type bit (see EntityDef::typeBits)
    uint32_t field_0C = 0;             // 0x0C
    // 0x10: the definition's attachment records, copied by the constructor
    // (created on the first one) and freed with the placement.
    EntityAttachmentList* attachments = nullptr;
    ape::Matrix34 transform{};         // 0x20
    ape::Vec3 field_60{1.0f, 1.0f, 1.0f, 0.0f};   // 0x60 scale of the instance (EntityInstanceDesc::scale)
    ape::world::ApeObj* sourceObject = nullptr;   // 0x70 placed object the entity is built from, or null
    uint32_t field_74 = 0;             // 0x74
    EntityDef* def = nullptr;          // 0x78
    float field_7C = -1.0f;            // 0x7C (from the def, overridable)
    float field_80 = -1.0f;            // 0x80
    // 0x84 bit field: bits 31/30 from the constructor, bit 29 the
    // definition's kDefFlag_Desc5, bits 21-28 the player slot (0xFF: none).
    // NOTE: the meanings of bits 29-31 are unknown.
    uint32_t bits84 = 0;
    uint32_t field_88 = 0;             // 0x88
    char name[0x28] = {};              // 0x8C requested instance name (may be empty)
};

// Placements loaded from the level files, by name hash (0x01013B1C).
void RegisterEntityPlacement(uint32_t nameHash, EntityPlacement* placement);
EntityPlacement* FindEntityPlacement(uint32_t nameHash);

// Spawns through the definition's slot 7 (0x000D6CC0); null without one.
Entity* SpawnFromDef(EntityDef* def, EntityPlacement& placement);
// Completes the placement from the named definition and spawns (0x000D6D28).
Entity* SpawnEntity(uint32_t defName, EntityPlacement& placement);

} // namespace entity
} // namespace sly
