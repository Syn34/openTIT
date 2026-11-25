// openTIT - Stand-ins for the entity framework that the world object classes build on.
//
// The world objects in this directory (triggers, ropes, chains, push blocks, safes,
// sentry lights, ...) are all entity classes. Their base classes live in the entity
// module (sly/entity, console code 0x000B0000-0x00180000), which is being ported in
// parallel. Until it can be shared, this header models the parts of those bases that
// the derived classes here read or call, under the same console layout notes:
//
//  * EntityDefDesc     - the temporary block an entity definition is built from
//                        (filled by the def loader 0x000CA038 and the per-class parsers).
//  * EntityDef         - the definition base (console vtable 0x00B12230).
//  * EntityPlacement   - the per-instance placement record ("settings", vtable
//                        0x00B12260) a level file gives for each placed entity.
//  * EntityInstanceDesc- the block an entity instance is constructed from
//                        (0x000CDD28 fills the base part, the placement adds its own).
//  * ObjEntity         - the entity instance base (console vtable 0x00B12550).
//
// Everything that reaches outside this directory goes through ObjectsHooks.h.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>
#include <vector>

namespace ape::xml { class XmlElement; }

namespace sly {
namespace objects {

class EntityDef;
class EntityPlacement;
class ObjEntity;

// Class bits of a definition (EntityDefDesc::classBits, EntityDef::classBits).
// Each concrete definition class sets its own combination; the trigger family
// shares kDefClass_Trigger so that a "from" parent can be recognised.
enum DefClassBits : uint32_t {
    kDefClass_Trigger      = 0x00400000,
    kDefClass_Sound        = 0x01000000,  // ambient sound / sound zone triggers
    kDefClass_08000000     = 0x08000000,  // NOTE: set by damage/death/filtered/... triggers
};

// Flags of a placement (EntityPlacement::flags, +0x04).
enum PlacementFlags : uint32_t {
    kPlace_Trigger        = 0x00400000,   // placement is a TriggerPlacement
    kPlace_ForceCreate    = 0x02000000,   // NOTE: overrides a placement's "skip" bit
};

// ---------------------------------------------------------------------------
// Definition description (console block of 0x4C bytes, extended per class).
// ---------------------------------------------------------------------------
struct EntityDefDesc {
    uint32_t classBits = 0;        // 0x00
    uint32_t typeBits = 0;         // 0x04 class-specific type bit (copied to EntityDef::typeBits)
    uint32_t field_08 = 0;         // 0x08
    const char* name = "";         // 0x0C definition name (copied into EntityDef::name)
    const char* geomName = "";     // 0x10 hashed into EntityDef::geomNameHash
    const char* field_14 = "";     // 0x14 hashed into EntityDef::field_30
    bool flags[8] = {};            // 0x18..0x1F packed into EntityDef::flags
    uint32_t field_20 = 0;         // 0x20
    float field_24 = -1.0f;        // 0x24
    float field_28 = -1.0f;        // 0x28
    bool field_2C = false;         // 0x2C
    uint32_t field_30 = 0;         // 0x30
    uint32_t field_34 = 0;         // 0x34
    uint32_t field_38 = 0;         // 0x38
    uint32_t field_3C = 0;         // 0x3C
    const void* tagList = nullptr; // 0x40
    uint32_t field_44 = 0;         // 0x44
    float field_48 = -1.0f;        // 0x48

    EntityDefDesc() = default;
    // The inlined initialiser every per-class factory starts with: class bits of
    // the new class, everything else at its default, the name from the loader.
    EntityDefDesc(uint32_t bits, const EntityDefDesc& loader) : classBits(bits), name(loader.name) {}
};

// Copies the fields a definition inherits from the loader's description
// (0x000C387C, entity module): the flag bytes and the shared scalar fields.
void InheritDefDesc(const EntityDefDesc& from, EntityDefDesc& to);

// ---------------------------------------------------------------------------
// Definition base (console 0x8C bytes, vtable 0x00B12230).
// ---------------------------------------------------------------------------
class EntityDef {
public:
    // 0x000C26D8 (entity module): copies the description and resolves names.
    explicit EntityDef(const EntityDefDesc& desc);
    // 0x000C2DA0 (entity module).
    virtual ~EntityDef();

    // Slot 7: creates an entity for a placement (null if the class does not spawn one).
    virtual ObjEntity* Create(EntityPlacement* placement) { (void)placement; return nullptr; }
    // Slot 8 (default 0x000CBA88): name for a new instance. Uses the placement's
    // name, or "<def name>_inst%05d" with a running number.
    virtual const char* GetInstanceName(EntityPlacement* placement);

    bool IsClass(uint32_t bits) const { return (classBits & bits) == bits; }

    EntityDef(const EntityDef&) = delete;
    EntityDef& operator=(const EntityDef&) = delete;

    uint32_t instanceCounter = 0;  // 0x0C
    uint32_t classBits = 0;        // 0x14
    uint32_t typeBits = 0;         // 0x18
    uint8_t options = 0;           // 0x20 bit 3: sleeps unless a player is near (update-enable volumes)
    uint32_t geomNameHash = 0;     // 0x2C
    uint32_t field_30 = 0;         // 0x30
    uint32_t nameHash = 0;         // 0x34
    char name[0x40] = {};          // 0x38
};

// ---------------------------------------------------------------------------
// Instance description (console block of 0x90 bytes, extended per class).
// ---------------------------------------------------------------------------
struct EntityInstanceDesc {
    uint32_t flags = 0;                 // 0x14
    const char* name = nullptr;         // 0x1C
    EntityDef* def = nullptr;           // 0x20
    ape::Matrix34 transform{};          // 0x30
    float field_84 = 1.0f;              // 0x84
    float field_88 = 1.0f;              // 0x88

    // 0x000CDFB0/0x000CDD28 (entity module) with the arguments every Create in this
    // directory passes: one reference, identity transform, unit scales.
    EntityInstanceDesc(const char* instanceName, EntityDef* ownerDef);
    virtual ~EntityInstanceDesc() = default;
};

// ---------------------------------------------------------------------------
// Placement ("settings") base (console vtable 0x00B12260).
// ---------------------------------------------------------------------------
class EntityPlacement {
public:
    EntityPlacement(const char* placementName, EntityDef* ownerDef);
    // Releases the optional array at 0x10 (see below).
    virtual ~EntityPlacement();
    // Slot 2 (base 0x000CDB28): copies the placement into an instance description.
    virtual void FillInstanceDesc(EntityInstanceDesc& desc) const;

    EntityPlacement(const EntityPlacement&) = delete;
    EntityPlacement& operator=(const EntityPlacement&) = delete;

    uint32_t flags = 0;                // 0x04 PlacementFlags
    uint32_t typeBits = 0;             // 0x08 class-specific type bit (see EntityDef::typeBits)
    // 0x10: array owned by the placement, freed with it. NOTE: element type
    // and the code that fills it are in the entity module (not modelled).
    ape::ApeArrayBase* ownedArray = nullptr;
    ape::Matrix34 transform{};         // 0x20
    EntityDef* def = nullptr;          // 0x78
    float field_7C = 0.0f;             // 0x7C (from the def, overridable)
    float field_80 = 0.0f;             // 0x80
    char name[0x28] = {};              // 0x8C
};

// ---------------------------------------------------------------------------
// Entity instance base (console vtable 0x00B12550).
// ---------------------------------------------------------------------------
// Entity flag bit 18 (+0xAC): registered and alive.
constexpr uint32_t kEntityAlive = 1u << 18;
// Second flag word (+0xC0).
enum ObjEntityFlags2 : uint32_t {
    kEntFlag2_Highlightable = 0x02,   // NOTE: checked before IsEventSuppressed
    kEntFlag2_Character     = 0x08,   // the entity is a character
};
// Third flag word (+0xC4).
constexpr uint32_t kEntFlag3_Pushable = 0x20000;   // can be pushed (push block triggers)

class ObjEntity {
public:
    explicit ObjEntity(const EntityInstanceDesc& desc);
    virtual ~ObjEntity();

    // Slot 2: called once after construction.
    virtual void OnCreated() {}
    // Slot 4: per-frame update.
    virtual void Update(float dt) { (void)dt; }
    // Slot 7/8: script state per layer (triggers keep their occupant count in layer 0).
    virtual void SetState(int layer, int value);
    virtual int GetState(int layer) const;
    // Slot 16: the physics/character component (component type 0), if any.
    virtual void* GetPhysicsComponent() { return nullptr; }
    // Slot 10: drops a reference (paired with AddRef below).
    virtual void Release(bool immediate);
    // Slot 19.
    virtual const char* GetName() const { return name; }
    // Slot 24: NOTE: called with "has occupants" by triggers.
    virtual void SetBusy(bool busy) { (void)busy; }
    // Slot 40: bounds, world or local.
    virtual ape::Aabb GetBounds(bool world) const;
    // Slot 43: script/game message (`id` is the message hash at +0x08).
    virtual void HandleMessage(uint32_t id, const void* data) { (void)id; (void)data; }
    // Slot 55.
    virtual void ApplyVisibility(bool visible) { (void)visible; }
    // Slot 57/58: told by a trigger volume that this entity entered / left it.
    virtual void OnEnteredTrigger(ObjEntity* trigger) { (void)trigger; }
    virtual void OnLeftTrigger(ObjEntity* trigger) { (void)trigger; }
    // Slot 68: the entity's anim script instance (none by default).
    virtual void* GetAnimScript() { return nullptr; }
    // Slot 80: NOTE: non-zero suppresses the "entered" game event for players.
    virtual int IsEventSuppressed() const { return 0; }

    const ape::Vec3& Position() const { return transform.translation; }
    void AddRef() { ++refCount; }
    bool IsAlive() const { return (flags & kEntityAlive) != 0; }
    bool IsPlayer() const { return (classBits & 1u) != 0; }

    ObjEntity(const ObjEntity&) = delete;
    ObjEntity& operator=(const ObjEntity&) = delete;

    ape::Matrix34 transform{};     // 0x60 (translation row = position, 0x90)
    uint32_t flags = 0;            // 0xAC
    int16_t refCount = 0;          // 0xB2
    uint64_t classBits = 0;        // 0xB8 (bit 0: player)
    uint32_t flags2 = 0;           // 0xC0 ObjEntityFlags2
    uint32_t flags3 = 0;           // 0xC4
    EntityDef* def = nullptr;      // 0xD8
    uint32_t id = 0;               // 0xA8 unique id (name hash)
    uint8_t playerIndex = 0;       // 0xAC most significant byte of the flag word
    int state[4] = {};
    char name[0x40] = {};
};

} // namespace objects
} // namespace sly
