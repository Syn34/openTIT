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

// Damage taken and push applied per hit category (0 = none) by the rigid
// body entities (block of 0x28 bytes in their definitions, at +0xC4 or
// +0xC8). NOTE: filled by the definition parsers (not ported).
struct PropDamageSettings {
    float impactScale = 0.0f;          // +0x00 category 1 damage
    float impactPush = 0.0f;           // +0x04 category 1 push
    float blastScale = 0.0f;           // +0x08 category 2 damage
    float blastPush = 0.0f;            // +0x0C category 2 push
    float collisionScale = 0.0f;       // +0x10 category 0, per unit of speed
    float collisionThreshold = 0.0f;   // +0x14 speed below which nothing happens
    float collisionMax = 0.0f;         // +0x18
    float environmentScale = 0.0f;     // +0x1C category 3 damage
    float environmentPush = 0.0f;      // +0x20 category 3 push
    float projectileScale = 0.0f;      // +0x24 category 4 damage

    // Some hit category can hurt.
    bool CanHurt() const
    {
        return collisionScale != 0.0f || impactScale != 0.0f || blastScale != 0.0f ||
               environmentScale != 0.0f || projectileScale != 0.0f;
    }
};

// Hit reactions shared by the rigid body entities: the hit is taken
// (ScriptedEntity::ReceiveHit), `body` (may be null) is pushed and `self`
// hurt by the category's settings.
namespace propdamage {
void Collision(ScriptedEntity& self, const PropDamageSettings& settings, EntityHitRecord& hit);
void Impact(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit);
// The reactions of Impact and Blast without the ReceiveHit check.
void ApplyImpact(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit);
void ApplyBlast(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit,
                bool positiveOnly);
// `positiveOnly`: hurt only when the computed damage is positive
// (otherwise whenever the scale is).
void Blast(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit,
           bool positiveOnly);
void Environment(ScriptedEntity& self, void* body, const PropDamageSettings& settings, EntityHitRecord& hit);
void Projectile(ScriptedEntity& self, const PropDamageSettings& settings, EntityHitRecord& hit, uint32_t type);
// Records whether a hit killed the entity in its flags.
void SetFatal(EntityHitRecord& hit, bool alive);
} // namespace propdamage

// Base of the scripted entities that own one Havok rigid body built from
// their definition's collision object (RigidProp, CompoundEntity,
// AlterEgoEntity, TargetableA/B). Each class has its own copy of the
// virtual overrides on the console; the shared parts are the helpers here.
class RigidBodyEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Moves a body to `transform`; when its position changed the bodies
    // linked to it wake up.
    static void MoveBody(void* body, const ape::Matrix34& transform);
    // Interaction kinds 0 and 4 of the props that can be picked up: kind 0
    // needs some damage reaction and refuses `refused`; other kinds fail.
    // NOTE: the console also multiplies the caller's score by 5 for kind 0.
    bool CanBePickedUp(Entity* other, int kind, const PropDamageSettings& settings, const Entity* refused);
    // Sets a body's linear velocity unless it is keyframed or fixed or out of the world.
    static void SetBodyVelocity(void* body, const ape::Vec3& velocity);
    // Slot 35: linear velocity of the body (zero without one).
    ape::Vec3 GetVelocity() override { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
    // Slot 71: centre of mass of the body.
    ape::Vec3 GetAimPoint() override { return Position(); }
    // Slot 82: some hit category can hurt the entity.
    bool CanBeDamaged() const override { return false; }
    // Slot 89: damage scale of projectiles.
    float ProjectileDamageScale() const override { return 0.0f; }

    void* rigidBody = nullptr;           // 0x184 Havok rigid body

protected:
    // Takes the body out of the world and drops it (and `action`, if any).
    void ReleaseBody(void* action);
    ape::Vec3 BodyVelocity() const;

    ape::Vec3 BodyMassCenter() const;
    // The hit reactions with this entity's body (see propdamage).
    void HitCollision(const PropDamageSettings& settings, EntityHitRecord& hit)
    { propdamage::Collision(*this, settings, hit); }
    void HitImpact(const PropDamageSettings& settings, EntityHitRecord& hit)
    { propdamage::Impact(*this, rigidBody, settings, hit); }
    void HitBlast(const PropDamageSettings& settings, EntityHitRecord& hit, bool positiveOnly)
    { propdamage::Blast(*this, rigidBody, settings, hit, positiveOnly); }
    void HitEnvironment(const PropDamageSettings& settings, EntityHitRecord& hit)
    { propdamage::Environment(*this, rigidBody, settings, hit); }
    void HitProjectile(const PropDamageSettings& settings, EntityHitRecord& hit, uint32_t type)
    { propdamage::Projectile(*this, settings, hit, type); }
};

// Definition of RigidProp (vtable 0x00B14360, 0x120 bytes).
class RigidPropDef : public ScriptedEntityDef {
public:
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~RigidPropDef() override;
    PropDamageSettings damage;         // 0xC4
    uint8_t targetingFlags[6] = {};    // 0x104 ("targetingFlags", one per interaction kind)
    void* collisionShape = nullptr;    // 0x10C
};

// A physics prop (vtable 0x00B143A8 + 0x00B14540/0x00B14568, 0x1E0 bytes;
// constructor 0x001676F8 builds its rigid body from the definition's shape).
class RigidProp : public RigidBodyEntity {
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
    // Slot 12: also turns the pivot of the world object 0x45A6A51A by `spin`.
    // NOTE: the object's name is unknown.
    void UpdateTransform(float dt) override;
    // Slot 21: the body follows the new transform.
    void SetTransform(const ape::Matrix34& m) override;
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;
    ape::Vec3 GetAimPoint() override;
    bool CanBeDamaged() const override;
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    bool Damage(float amount, Entity* source, uint32_t type) override;   // slot 88
    float ProjectileDamageScale() const override;
    // Slot 76: only in the dynamic or destroyed states.
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    // Slot 99: moves the body in or out of the physics world (0x00167570).
    virtual void SetPropState(int32_t state, uint32_t arg);

    void* bodyAction = nullptr;          // 0x188 NOTE: added to the world with the body
    int32_t propState = kProp_None;      // 0x18C PropState
    uint32_t field_190 = 0;              // 0x190 cleared when the body enters the world
    uint32_t field_194 = 0;              // 0x194
    Entity* interactor = nullptr;        // 0x198 NOTE: refused as interaction partner of kind 0
    uint32_t collisionGroup = 0;         // 0x19C
    ape::Vec4 spin{ 0.0f, 0.0f, 0.0f, 1.0f };   // 0x1B0 quaternion applied to the 0x45A6A51A object's pivot
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
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~AlterEgoDef() override;
    float field_C4 = 0.0f;             // 0xC4 NOTE: positive values refuse interactions of kind 4
    PropDamageSettings damage;         // 0xC8
    uint8_t targetingFlags[6] = {};    // 0xF4
    void* collisionShape = nullptr;    // 0xFC
    // Bit 29: dies landing hard on static geometry; bit 30: dies touching a
    // player-side entity (type bits 3 or 74); bit 31: cannot be picked up.
    uint32_t deathFlags = 0;           // 0x100
};

// Entity of AlterEgoDef (vtable 0x00B14968 + 0x00B14B00/0x00B14B28, 0x1C0 bytes;
// constructor 0x001774D8). Registered in entity list B while alive.
// NOTE: named after the "AlterEgoBit"/"AlterEgoMegabit" names its methods use.
class AlterEgoEntity : public RigidBodyEntity {
public:
    ~AlterEgoEntity() override;
    // Sends an event to both anim scripts (0x0017A178).
    void SendScriptEvent(uint32_t event);
    bool Release(bool kill) override;                 // slot 10
    void SetFlag80(bool on) override;                 // slot 23
    // Slot 99: 1 puts the body into the world, 4 takes it out (0x0017734C).
    virtual void SetAlterState(int32_t state);
    // Slots 27/28: gravity factor of the body while the physics world runs.
    float GetGravityFactor() override;
    void Detached(float gravityFactor) override;
    // Slot 21: the body follows the new transform while in the world.
    void SetTransform(const ape::Matrix34& m) override;
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;                 // slot 35
    ape::Vec3 GetAimPoint() override;                 // slot 71
    // Physics impacts can kill outright (see AlterEgoDef::deathFlags).
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    float ProjectileDamageScale() const override;     // slot 89
    // Slot 76: only in alter state 1; kind 4 needs a body with mass.
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const AlterEgoDef* Def() const { return static_cast<const AlterEgoDef*>(def); }

    int32_t alterState = 0;              // 0x188
    uint32_t field_18C = 0;              // 0x18C cleared by states 1 and 3
    Entity* refusedPartner = nullptr;    // 0x194 NOTE: refused for interactions of kind 0
    uint32_t collisionGroup = 0;         // 0x198
    void* animScript1A0 = nullptr;       // 0x1A0
    void* animScript1A4 = nullptr;       // 0x1A4
};

// Definition of the Bentley alter ego entity (vtable 0x00B14590, 0x110
// bytes; spawns 0x0016BF00, whose code uses "AlterEgo_Bentley").
class AlterEgoBentleyDef : public ScriptedEntityDef {
public:
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~AlterEgoBentleyDef() override;
    void* collisionShape = nullptr;    // 0xF4
};

// Definitions with a collision object and targeting flags whose entities are
// not identified yet (vtables 0x00B14B50 / 0x00B14D98 / 0x00B18B00).
class TargetableDefA : public ScriptedEntityDef {
public:
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~TargetableDefA() override;
    PropDamageSettings damage;         // 0xC4
    uint8_t targetingFlags[6] = {};    // 0x119
    void* collisionShape = nullptr;    // 0x120
};

class TargetableDefB : public ScriptedEntityDef {
public:
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    PropDamageSettings damage;         // 0xC4
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
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~TargetableDefC() override;
    uint8_t field_C4[0x0C] = {};       // 0xC4
    ape::Vec3 aimOffset{};             // 0xD0 (12 bytes used) NOTE: name from GetAimPoint
    uint8_t field_DC[0x0C] = {};       // 0xDC
    PropDamageSettings damage;         // 0xE8
    uint8_t targetingFlags[6] = {};    // 0x120
    void* collisionShape = nullptr;    // 0x128
    // Bit 24: the body collides with layer 0 too; bit 28: dies landing hard on
    // static geometry; bits 29/30: dies touching a player-side entity other
    // than the one that threw it (29 only when the entity has class bit 0).
    uint32_t hitFlags = 0;             // 0x12C
};

// Push block definition (vtable 0x00B16058, 0x100 bytes; spawns 0x001B4A10).
class PushBlockDef : public ScriptedEntityDef {
public:
    // Slot 3: local bounds of the collision shape, the entity default
    // without one. NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    ~PushBlockDef() override;
    ape::ApeTArray<void*> field_C0;    // 0xC0
    void* collisionShape = nullptr;    // 0xE4
};

// Push block (vtable 0x00B160A8; spawned by PushBlockDef, destructor
// 0x00753258 outside this range): a rigid body the player pushes around.
class PushBlock : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Slot 35: linear velocity of the body.
    ape::Vec3 GetVelocity() override;
    // Suspending freezes the body and keeps its velocity for the resume.
    void OnSuspend() override;                        // slot 60
    void OnResume() override;                         // slot 61
    // Kind 0 is refused while the block is locked (+0x248 bit 15), kind 4 always.
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    // Killing drops the nav obstacle and the body.
    bool Release(bool kill) override;                 // slot 10
    // Unless broken (state 4), goes back to state 0 with the lock request in
    // bits 30 (flag) and 31 (locked) of the state data. NOTE: the class is
    // inferred from the code address (scripts call it on "lockable" entities).
    void SetLocked(bool flag, bool locked);

    ape::Vec3 savedVelocity{};          // 0x230
    void* rigidBody = nullptr;          // 0x240
    int16_t blockFlags = 0;             // 0x248 bit 15: locked
    void* navObstacle = nullptr;        // 0x24C levels::NavObstacleHandle
    bool hasNavObstacle = false;        // 0x250
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
    bool Release(bool kill) override;                 // slot 10
    // Slot 27/28: gravity factor of the body (the parent's link's while attached).
    float GetGravityFactor() override;
    void Detached(float gravityFactor) override;
    // Slot 35: linear velocity of the body.
    // Slot 21: the body follows the new transform while in the world.
    void SetTransform(const ape::Matrix34& m) override;
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;
    // Slot 20: the definition's aim offset placed by the entity transform
    // (the entity default without one).
    ape::Vec3 GetCenter() override;
    // Slot 71: centre of mass of the body unless the definition moves the
    // aim point, then the entity centre.
    ape::Vec3 GetAimPoint() override;
    // Slot 82: some hit category can hurt the entity.
    virtual bool CanBeDamaged() const;
    // Physics impacts can kill outright (see TargetableDefC::hitFlags).
    void OnHitCategory0(EntityHitRecord& hit) override;
    // Impacts; with definition flag 29 those carrying damage bit 12 are ignored.
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    bool Damage(float amount, Entity* source, uint32_t type) override;   // slot 88
    // Slot 89: damage scale of projectiles.
    virtual float ProjectileDamageScale() const;
    // Slot 99: physics state (0x001F9F70, not ported).
    virtual void SetTargetState(int32_t state, uint32_t arg);
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const TargetableDefC* Def() const { return static_cast<const TargetableDefC*>(def); }

    void* rigidBody = nullptr;           // 0x1A0
    Entity* thrower = nullptr;           // 0x1AC NOTE: its touch does not kill
    uint16_t targetFlags = 0;            // 0x1B0 bits 8-9: NOTE unknown, any kills on impact; bit 10: destroyed
};

// Entity of TargetableDefC with a linked entity and a game world update
// callback (vtable 0x00B18F60; most of its own methods are at 0x00200000+).
class TargetableCLinked : public TargetableCBase {
public:
    // Also drops the update callback (list 3) and lets go of the linked entity.
    bool Release(bool kill) override;                 // slot 10

    Entity* linked = nullptr;            // 0x234
};

class TargetableA;

// Entity of TargetableDefB (vtable 0x00B14DE0 + 0x00B14F88/0x00B14FB0, 0x2C0
// bytes; constructor 0x00190158). Instances link into parent/child stacks and
// can be held by a TargetableA; the links only matter while the active
// minigame has its bit 30 set.
// Record stacked targets point at (+0x26C). NOTE: contents mostly unknown.
struct TargetStackInfo {
    uint8_t field_00[0x2C];
    float spinRate;          // +0x2C NOTE: advances TargetableB +0x290 per second
    uint8_t field_30[5];
    bool shieldsParent;      // +0x35 a child pointing here keeps its parent from harm
};

class TargetableB : public RigidBodyEntity {
public:
    ~TargetableB() override;
    bool Release(bool kill) override;                 // slot 10
    // Slot 21: the body (or, without one, the AI controller) follows the new transform.
    void SetTransform(const ape::Matrix34& m) override;
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;                 // slot 35
    ape::Vec3 GetAimPoint() override;                 // slot 71
    bool CanBeDamaged() const override;               // slot 82
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    // Slot 88: shielded while a stacked child says so; on death the body is
    // dropped, the entity takes its object's local transform and enters state 8.
    bool Damage(float amount, Entity* source, uint32_t type) override;
    float ProjectileDamageScale() const override;     // slot 89
    // Slot 94: enters a state of layer 0 and records it.
    virtual void SetTargetState(int state);
    // Slot 95: the state of layer 0; slot 97 asks slot 95.
    virtual int GetTargetState();
    virtual int GetTargetState97();
    // Slot 96: called after a state change (0x001947B0, not ported).
    virtual void OnTargetStateChanged();
    // The body leaves the world with the flag and returns in state 2.
    void SetFlag80(bool on) override;                 // slot 23
    // Slots 25/26: the collision group of the body (kept at +0x19C).
    uint32_t GetRenderId() override;
    void SetRenderId(uint32_t id) override;
    // A kill order (message 0xD77F98D6 with argument 0x14FBC) destroys the entity.
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    // Slot 57: while stacking, a watcher whose script state 0xDC175839 has
    // the event named after this entity knocks it off its holder and
    // destroys it.
    bool AddWatcher(void* watcher) override;
    // @func 0x001946F0 TargetableB::RemoveWatcher
    void RemoveWatcher(void* watcher) override { Entity::RemoveWatcher(watcher); }
    void* bodyAction = nullptr;              // 0x188
    uint8_t stateHistory[4] = {};            // 0x1B8 indexed by historyIndex
    uint32_t historyIndex = 0;               // 0x1BC
    Entity* refusedPartner = nullptr;        // 0x198 NOTE: refused for interactions of kind 0
    uint32_t collisionGroup = 0;             // 0x19C
    // Slot 76: only in state 2 or from state 8 on.
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    uint32_t InteractionOffer(int kind, Entity* other) override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    // Empty override of the +0x17C interface's slot 5.
    void Interface17CSlot5() override;
    const TargetableDefB* Def() const { return static_cast<const TargetableDefB*>(def); }
    // Predicate the minigame uses: not a stackable target (type bit 9), or
    // neither held nor (when stacking) on a parent (0x0018E9FC).
    static bool IsFreeTarget(Entity* entity);
    // Descendant of `object` (or itself) whose definition's model name is
    // `model` (0x001918B0).
    static ape::world::ApeObj* FindObjectByModel(ape::world::ApeObj* object, uint32_t model);
    // Takes `child` off this entity's child list and settles it (0x00191944).
    void DetachChild(TargetableB* child);

    ape::ApeTArray<TargetableB*> children;   // 0x254
    TargetableB* parent = nullptr;           // 0x268
    const TargetStackInfo* stackInfo = nullptr;   // 0x26C
    uint32_t field_270 = 0;                  // 0x270
    TargetableA* holder = nullptr;           // 0x298
    bool stacking = false;                   // 0x29C
    bool freezeWhenOrphaned = false;         // 0x2A0 NOTE: stops updating when its parent goes away
};

// Entity of TargetableDefA (vtable 0x00B14B98 + 0x00B14D48/0x00B14D70,
// 0x6C0 bytes; constructor 0x0017DD90): the minigame figure that can hold a
// TargetableB. Six world objects (0x00F6498C..0x00F649A0) are shared by all
// instances.
class TargetableA : public RigidBodyEntity {
public:
    // Slot 57: a watcher with an anim script is sent to its "figure" state
    // (0x2DA34781 unless the figure looks for a model, else 0x047034DE)
    // while nothing is held; refused when such a state exists.
    bool AddWatcher(void* watcher) override;
    static constexpr int kSharedObjects = 6;
    // Shared world objects, counted by every instance; cleared when the last
    // instance goes.
    static ape::world::ApeObj* s_sharedObjects[kSharedObjects];

    ~TargetableA() override;
    bool Release(bool kill) override;                 // slot 10
    // Slot 21: the body follows the new transform.
    void SetTransform(const ape::Matrix34& m) override;
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;                 // slot 35
    ape::Vec3 GetAimPoint() override;                 // slot 71
    bool CanBeDamaged() const override;               // slot 82
    // Slot 88: shares the first player's health; nothing hurts in states 2
    // and 11 (destroyed) or from type-8 sources and what they fired.
    bool Damage(float amount, Entity* source, uint32_t type) override;
    // Slot 43: state changes by message; kills by the figure (or by what it
    // fired) raise the combo and pay out the victim's score.
    void HandleMessage(const EntityMessage& msg) override;
    void OnHitCategory0(EntityHitRecord& hit) override;
    // Impacts and environment hits count at most once per 0.25 s each.
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    float ProjectileDamageScale() const override;     // slot 89
    // Slot 94: enters a state of layer 0 and records it.
    virtual void SetTargetState(int state);
    // Slot 96: called after a state change (not ported).
    virtual void OnTargetStateChanged();
    // The body leaves the world with the flag and returns in state 3.
    void SetFlag80(bool on) override;                 // slot 23
    // Slot 25: the collision group of the body (kept at +0x19C).
    uint32_t GetRenderId() override;
    void* bodyAction = nullptr;              // 0x188
    uint8_t stateHistory[7] = {};            // 0x1B1 indexed by historyIndex
    uint32_t historyIndex = 0;               // 0x1B8
    Entity* refusedPartner = nullptr;        // 0x198 NOTE: refused for interactions of kind 0
    uint32_t collisionGroup = 0;             // 0x19C
    // Slot 95: the state of layer 0; slot 97 asks slot 95.
    virtual int GetTargetState();
    virtual int GetTargetState97();
    // Slots 104/105: empty.
    // @func 0x0018E0BC TargetableA::Slot104
    virtual void Slot104() {}
    // @func 0x0018E380 TargetableA::Slot105
    virtual void Slot105() {}
    // Slot 76: only in state 3 or from state 11 on.
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    // Both answer with the definition's targeting flag for the kind.
    uint32_t InteractionOffer(int kind, Entity* other) override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    const TargetableDefA* Def() const { return static_cast<const TargetableDefA*>(def); }

    // Child object of the figure's model whose model is `modelName` (+0x624),
    // or null (0x00185C9C).
    ape::world::ApeObj* FindModelPart() const;

    TargetableB* held = nullptr;             // 0x520
    ape::world::ApeObj* model = nullptr;     // 0x52C
    uint32_t modelName = 0;                  // 0x624
    void* animScript524 = nullptr;           // 0x524
    ape::world::ApeObj* object_530 = nullptr; // 0x530 counted
    ape::ApeTArray<void*> field_678;         // 0x678
    HkArray<void*> field_68C;                // 0x68C
    ape::Vec3 teleportTarget{};              // 0x630 sender position of the "teleport" message
    uint32_t combo = 0;                      // 0x670 kills in a row, cleared by damage
    int32_t comboScore = 0;                  // 0x674 NOTE: read on the player 0 entity
    bool field_6A4 = false;                  // 0x6A4 set and cleared by messages
    Entity* lastThrown = nullptr;            // 0x6B0 entity of type bit 13 this figure fired
    float impactCooldown = 0.0f;             // 0x64C
    float environmentCooldown = 0.0f;        // 0x650
    ScriptedEntity* healthSource = nullptr;  // 0x6A8 NOTE: health is mirrored from it in mode 1
    int32_t healthMode = 0;                  // 0x6AC
    // Slot 79: in mode 1 the health goes to the source entity and is copied back.
    void AddHealth(float amount) override;
};

// Definition of CompoundEntity. NOTE: the class is not identified; only its
// damage block (at +0xC4, as in RigidPropDef) is read.
class CompoundEntityDef : public ScriptedEntityDef {
public:
    PropDamageSettings damage;         // 0xC4
};

class AlterEgoBentleyState;

// The Bentley alter ego entity (built by AlterEgoBentleyDef's spawn
// 0x0016BF00, "AlterEgo_Bentley"): owns its eleven power-up states and two
// Havok pointer arrays
// (vtable 0x00B14730 + 0x00B148CC/0x00B148F4). Registered in entity list B
// and on the world's transform callback list while alive.
// NOTE: the class keeps the descriptive name it was given first.
class CompoundEntity : public RigidBodyEntity {
public:
    // Sub-objects deleted with the entity (virtual deleting destructor).
    // The power-up states (AlterEgoBentley.h) are the entity's parts.
    using Part = AlterEgoBentleyState;
    // Enters power-ups 0..`level` and records the level (0x00175B60).
    void SetPartLevel(uint32_t level);
    // Clears `object` from the first Havok pointer array (0x0016DCF0).
    void ForgetHavokObject(void* object);
    // Runs the scale pulse (+0x2B8 timer of +0x2BC seconds): the entity
    // swells and wobbles back to its size (0x00173208).
    void UpdateScalePulse(float dt);
    // Plays a power-up effect on the character component unless the entity
    // is dead with its death effect showing; remembers the effect (0x0016CDE8).
    void PlayPowerUpFx(uint32_t fx);

    ~CompoundEntity() override;
    bool Release(bool kill) override;                 // slot 10
    void SetFlag80(bool on) override;                 // slot 23
    void SetVelocity(const ape::Vec3& velocity) override;    // slot 34
    ape::Vec3 GetVelocity() override;                 // slot 35
    bool CanBeDamaged() const override;               // slot 82
    // Collisions and projectiles also record the direction they came from.
    void OnHitCategory0(EntityHitRecord& hit) override;
    // Impacts (without the ReceiveHit check) also mark hits from the
    // 0x659234F5 definition in the buff flags (bit 23).
    void OnHitCategory1(EntityHitRecord& hit) override;
    // Blasts, without the ReceiveHit check.
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    // Slot 88: the Bentleys of both players share the first player's health;
    // nothing hurts while a timed buff runs or from type-10 sources.
    bool Damage(float amount, Entity* source, uint32_t type) override;
    // Slot 43: a dying entity is forgotten by both Bentleys; its score
    // (times the part level) goes to player 0 when this Bentley killed it.
    void HandleMessage(const EntityMessage& msg) override;
    // Slot 69: the shared target (see s_target).
    // @func 0x0017574C CompoundEntity::GetTrackedTarget
    Entity* GetTrackedTarget() override { return s_target; }
    // @func 0x00175740 CompoundEntity::SetTarget
    static void SetTarget(Entity* target) { s_target = target; }
    // Nearest entity of definition 0x659234F5 the Bentleys go for (0x00F64988).
    static Entity* s_target;
    float ProjectileDamageScale() const override;     // slot 89
    // Heading (degrees, 0..360) from the entity to a hit position.
    float HeadingTo(const ape::Vec3& point) const;
    // Slot 99: physics state (0x0016BC0C, not ported).
    virtual void SetPropState(int32_t state, uint32_t arg);
    // Slot 100: sets the velocity that brings the body to `target` in `dt`.
    virtual void DriveTowards(const ape::Vec3& target, float dt);
    void* bodyAction = nullptr;              // 0x188
    int32_t propState = 0;                   // 0x18C 6: body active
    int32_t hitHeading = 0;                  // 0x2E0 degrees
    const PropDamageSettings& DamageSettings() const;
    // Empty overrides of the +0x17C interface's slots 5 and 6.
    void Interface17CSlot5() override;
    void Interface17CSlot6() override;

    HkArray<void*> touchingBodies; // 0x1FC NOTE: bodies of entities it is in contact with (guessed)
    HkArray<void*> field_208;      // 0x208
    Part* parts[11] = {};          // 0x214
    uint32_t buffFlags = 0;        // 0x290 bit 25: a timed buff is running
    uint32_t partLevel = 0;        // 0x29C also counts the timed buff's charges used
    uint32_t scriptLevel = 0;      // 0x2A8
    float speedScale = 1.0f;       // 0x2AC
    float buffRate = 0.0f;         // 0x2B0
    uint32_t lastFx = 0;           // 0x2B4 effect last played (0x03DE9A54 = the death effect)
    float pulseTimer = 0.0f;       // 0x2B8
    float pulseTime = 0.0f;        // 0x2BC
    float pulseBase = 1.0f;        // 0x2C0 eases toward 1
    float pulseScale = 1.0f;       // 0x2C4
    float pulseAmplitude = 0.0f;   // 0x2C8
    float pulseFrequency = 0.0f;   // 0x2CC
    uint32_t skinLevel = 0;        // 0x2D0
    uint32_t skinCount = 0;        // 0x2D8
    uint32_t killCount = 0;        // 0x2EC kills since the last damage
    int32_t score = 0;             // 0x2F0 NOTE: read on the player 0 entity
    int32_t playerIndex = 0;       // 0x304 player this Bentley belongs to
    // Slot 79: the second player's Bentley shares the first player's health.
    void AddHealth(float amount) override;
};

} // namespace entity
} // namespace sly
