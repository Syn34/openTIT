// openTIT - Entity classes built from a definition's collision object.
//
// The definitions here all parse a "colObjName" and "targetingFlags"
// attribute and keep a reference to the collision shape they build from it;
// their entities create a rigid body from that shape. Class names are
// descriptive (the original names are not known).
#pragma once

#include "sly/entity/GameEntityHooks.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/AudioSystem.h"
#include "ape/memory/ApeTArray.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity {

// Releases a Havok shape reference held by a definition (0x007D4EA8).
void ReleaseCollisionShape(void* shape);

// Definition of RigidProp (vtable 0x00B14360, 0x120 bytes).
class RigidPropDef : public ScriptedEntityDef {
public:
    ~RigidPropDef() override;
    // Damage taken and push applied per hit category (0 = none). NOTE: filled
    // by the definition parser (0x001668B4, not ported).
    float impactScale = 0.0f;          // 0xC4 category 1 damage
    float impactPush = 0.0f;           // 0xC8 category 1 push
    float blastScale = 0.0f;           // 0xCC category 2 damage
    float blastPush = 0.0f;            // 0xD0 category 2 push
    float collisionScale = 0.0f;       // 0xD4 category 0, per unit of speed
    float collisionThreshold = 0.0f;   // 0xD8 speed below which nothing happens
    float collisionMax = 0.0f;         // 0xDC
    float environmentScale = 0.0f;     // 0xE0 category 3 damage
    float environmentPush = 0.0f;      // 0xE4 category 3 push
    float projectileScale = 0.0f;      // 0xE8 category 4 damage
    uint8_t targetingFlags[6] = {};    // 0x104 ("targetingFlags", one per interaction kind)
    void* collisionShape = nullptr;    // 0x10C
};

// A physics prop (vtable 0x00B143A8 + 0x00B14540/0x00B14568, 0x1E0 bytes;
// constructor 0x001676F8 builds its rigid body from the definition's shape).
class RigidProp : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Prop states (+0x18C, slot 99). NOTE: names from the transitions.
    enum PropState : int32_t {
        kProp_None = 0,
        kProp_Dynamic = 1,      // the body is in the physics world
        kProp_Removed = 4,      // states 2-4: the body left the world
        kProp_Destroyed = 7,    // health ran out
    };

    ~RigidProp() override;
    bool Release(bool kill) override;                         // slot 10
    void SetFlag80(bool on) override;                         // slot 23
    void HandleMessage(const EntityMessage& msg) override;    // slot 43
    // Slot 35: linear velocity of the body (zero without one).
    virtual ape::Vec3 GetVelocity();
    // Slot 71: centre of mass of the body, the position without one.
    virtual ape::Vec3 GetMassCenter();
    // Slot 82: some hit category can hurt the prop.
    virtual bool CanBeDamaged() const;
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    bool Damage(float amount, Entity* source, uint32_t type) override;   // slot 88
    // Slot 89: damage scale of projectiles.
    virtual float ProjectileDamageScale() const;
    // Slot 99: moves the body in or out of the physics world (0x00167570).
    virtual void SetPropState(int32_t state, uint32_t arg);

    void* rigidBody = nullptr;           // 0x184 Havok rigid body
    void* bodyAction = nullptr;          // 0x188 NOTE: added to the world with the body
    int32_t propState = kProp_None;      // 0x18C PropState
    uint32_t field_190 = 0;              // 0x190 cleared when the body enters the world
    uint32_t field_194 = 0;              // 0x194
    Entity* interactor = nullptr;        // 0x198 NOTE: refused as interaction partner of kind 0
    uint32_t collisionGroup = 0;         // 0x19C
    uint8_t field_1C0 = 0;               // 0x1C0 cleared when the body leaves the world
    uint8_t field_1C1 = 0;               // 0x1C1
    uint32_t field_1C4 = 0;              // 0x1C4
    // Both answer with the definition's targeting flag for the kind.
    uint32_t InteractionOffer(int kind, Entity* other) override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const RigidPropDef* Def() const { return static_cast<const RigidPropDef*>(def); }
};

// Definition of AlterEgoEntity (vtable 0x00B14918).
class AlterEgoDef : public ScriptedEntityDef {
public:
    ~AlterEgoDef() override;
    uint8_t targetingFlags[6] = {};    // 0xF4
    void* collisionShape = nullptr;    // 0xFC
};

// Entity of AlterEgoDef (vtable 0x00B14968 + 0x00B14B00/0x00B14B28, 0x1C0 bytes;
// constructor 0x001774D8). Registered in entity list B while alive.
// NOTE: named after the "AlterEgoBit"/"AlterEgoMegabit" names its methods use.
class AlterEgoEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    ~AlterEgoEntity() override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const AlterEgoDef* Def() const { return static_cast<const AlterEgoDef*>(def); }
};

// Definition of the Bentley alter ego entity (vtable 0x00B14590, 0x110
// bytes; spawns 0x0016BF00, whose code uses "AlterEgo_Bentley").
class AlterEgoBentleyDef : public ScriptedEntityDef {
public:
    ~AlterEgoBentleyDef() override;
    void* collisionShape = nullptr;    // 0xF4
};

// Definitions with a collision object and targeting flags whose entities are
// not identified yet (vtables 0x00B14B50 / 0x00B14D98 / 0x00B18B00).
class TargetableDefA : public ScriptedEntityDef {
public:
    ~TargetableDefA() override;
    void* collisionShape = nullptr;    // 0x120
};

class TargetableDefB : public ScriptedEntityDef {
public:
    uint8_t targetingFlags[6] = {};    // 0x11D
    // Entries of 0x3C bytes owned by the definition. NOTE: contents not decoded.
    struct Entry {
        uint8_t data[0x3C];
    };
    ~TargetableDefB() override;
    void* collisionShape = nullptr;    // 0x124
    ape::ApeTArray<Entry*> entries;    // 0x128
};

class TargetableDefC : public ScriptedEntityDef {
public:
    ~TargetableDefC() override;
    uint8_t targetingFlags[6] = {};    // 0x120
    void* collisionShape = nullptr;    // 0x128
};

// Push block definition (vtable 0x00B16058, 0x100 bytes; spawns 0x001B4A10).
class PushBlockDef : public ScriptedEntityDef {
public:
    ~PushBlockDef() override;
    ape::ApeTArray<void*> field_C0;    // 0xC0
    void* collisionShape = nullptr;    // 0xE4
};

// Chain definition (vtable 0x00B15150, 0xC0 bytes; derives from the plain
// definition base; spawns 0x001A7180, which builds "Spline_Chain_%s").
class ChainDef : public EntityDef {
public:
    ~ChainDef() override;
    audio::SoundName* sounds[5] = {};  // 0xAC
};

// A physics chain built from ChainDef (vtable 0x00B151A0, 0x780 bytes;
// constructor 0x001A7180 builds the "Spline_Chain_%s" spline and one world
// object per link).
class ChainEntity : public Entity {
public:
    static constexpr int kMaxLinks = 15;
    struct Link {
        ape::world::ApeObj* object = nullptr;   // +0x00 counted reference
        uint8_t field_04[0x5C] = {};
    };
    ~ChainEntity() override;
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;

    Link links[kMaxLinks];                       // 0x1B0
    uint16_t linkCount = 0;                      // 0x760
    ape::world::ApeObj* spline = nullptr;        // 0x768 NOTE: the chain's spline instance
    ape::world::ApeObj* field_76C = nullptr;     // 0x76C
};

// Common base of the entities of TargetableDefC (vtables 0x00B18B48,
// 0x00B18D30 and 0x00B18F60 share its methods; their destructors are not in
// this range).
class TargetableCBase : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const TargetableDefC* Def() const { return static_cast<const TargetableDefC*>(def); }
};

class TargetableA;

// Entity of TargetableDefB (vtable 0x00B14DE0 + 0x00B14F88/0x00B14FB0, 0x2C0
// bytes; constructor 0x00190158). Instances link into parent/child stacks and
// can be held by a TargetableA; the links only matter while the active
// minigame has its bit 30 set.
class TargetableB : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    ~TargetableB() override;
    uint32_t InteractionOffer(int kind, Entity* other) override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    // Empty override of the +0x17C interface's slot 5.
    void Interface17CSlot5() override;
    const TargetableDefB* Def() const { return static_cast<const TargetableDefB*>(def); }
    // Takes `child` off this entity's child list and settles it (0x00191944).
    void DetachChild(TargetableB* child);

    ape::ApeTArray<TargetableB*> children;   // 0x254
    TargetableB* parent = nullptr;           // 0x268
    uint32_t field_270 = 0;                  // 0x270
    TargetableA* holder = nullptr;           // 0x298
    bool freezeWhenOrphaned = false;         // 0x2A0 NOTE: stops updating when its parent goes away
};

// Entity of TargetableDefA (vtable 0x00B14B98 + 0x00B14D48/0x00B14D70,
// 0x6C0 bytes; constructor 0x0017DD90): the minigame figure that can hold a
// TargetableB. Six world objects (0x00F6498C..0x00F649A0) are shared by all
// instances.
class TargetableA : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    static constexpr int kSharedObjects = 6;
    // Shared world objects, counted by every instance; cleared when the last
    // instance goes.
    static ape::world::ApeObj* s_sharedObjects[kSharedObjects];

    ~TargetableA() override;

    TargetableB* held = nullptr;             // 0x520
    void* animScript524 = nullptr;           // 0x524
    ape::world::ApeObj* object_530 = nullptr; // 0x530 counted
    ape::ApeTArray<void*> field_678;         // 0x678
    HkArray<void*> field_68C;                // 0x68C
};

// Scripted entity owning eleven sub-objects and two Havok pointer arrays
// (vtable 0x00B14730 + 0x00B148CC/0x00B148F4). Registered in entity list B
// and on the world's transform callback list while alive.
// NOTE: named from its members; its role is not known yet.
class CompoundEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Sub-objects deleted with the entity (virtual deleting destructor).
    class Part {
    public:
        virtual ~Part() = default;
    };

    ~CompoundEntity() override;
    // Empty overrides of the +0x17C interface's slots 5 and 6.
    void Interface17CSlot5() override;
    void Interface17CSlot6() override;

    HkArray<void*> field_1FC;      // 0x1FC
    HkArray<void*> field_208;      // 0x208
    Part* parts[11] = {};          // 0x214
};

} // namespace entity
} // namespace sly
