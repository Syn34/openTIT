// openTIT - Scripted actors: the entity base of most interactive world
// objects (console vtable 0x00B12898, destructor 0x007476B0 in the entity
// helper range) and the actor classes of 0x001A0000-0x00200000 whose
// teardown is ported. Class names are descriptive (the original names are
// not known); the console vtable is given for each.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/Tween.h"

namespace ape::world { class ApeObj; }
namespace ape::setting { class Setting; }
namespace sly::camera { class CameraMode; }
namespace sly::objects { struct ActorInstanceDesc; struct InteractivePropBInstanceDesc; struct ObjectActorInstanceDesc; struct SafeInstanceDesc; }
namespace sly::hud { class HudObject; }
namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace entity {
class GameCamera;

// The fields an actor definition part shares with its description (the
// 0xA0-byte records of ActorDef::Part / ActorPartDesc around the word at
// +0x20). NOTE: names follow use where known; the attribute names of the
// rest are not in the executable.
struct ActorPartFields {
    ape::Vec3 offset{};             // +0x00 attribute 0x63662D61
    ape::Vec3 field_10{};           // +0x10 attribute 0x1B89D5F3
    uint32_t field_24 = 0;          // +0x24 hash 0x75849CFE
    ape::Vec3 field_30{};           // +0x30 (with partFlags bit 28)
    // +0x40 with partFlags bit 28: attributes 0x46206B53, 0xE3E14C49,
    // 0x471281EF, 0x9538DE99, 0x9533AB5A, 0x040C1D75, 0x69FE7274.
    float field_40[7] = {1.0f, 0.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f};
    // +0x60 attributes 0x29F45D46, 0xA1FDE21D, 0xE2154ADB, 0x8933D0E0,
    // 0x0023FEF4 (default: a global set at start-up), 0xB89310FD.
    float field_60[6] = {-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 1.0f};
    uint32_t field_78 = 0;          // +0x78 hash 0x725B9AF2
    uint32_t partFlags = 0;         // +0x7C bits 28-31 (ActorDef::PartFlags)
    ape::Vec3 aimOffset{};          // +0x80 attribute 0xCC0D6864
    uint32_t navMode = 0;           // +0x90 hash 0x5B1E4A49 (0 = the definition's mode)
};

// One part of an actor definition as the loader describes it (0xA0 bytes):
// the part's fields with the name of the Havok body template where the
// part keeps its body.
struct ActorPartDesc : ActorPartFields {
    const char* bodyName = nullptr; // +0x20 "colObjName": rigid body of the level's physics data
    // A part is described by a body name or an offset.
    bool IsDescribed() const;
};

// Description of actor definitions (ScriptedEntityDefDesc + 0x64 bytes).
struct ActorDefDesc : ScriptedEntityDefDesc {
    ActorDefDesc() = default;
    // The defaults every actor description starts from: scripted and actor
    // class bits, the loader's name, 100 health, static nav mode, unit damage
    // scales (a collision threshold of 0.3, no collision maximum), every
    // interaction kind needed and definition flag 26 (the console
    // initialiser 0x00745D88, called with constant arguments by its users).
    explicit ActorDefDesc(const EntityDefDesc& loader);
    // Scripted attributes, then the actor's: damage scales, flags,
    // targeting flags and the parts (0x000C85A8).
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
    ape::ApeTArray<ActorPartDesc>* parts = nullptr;   // 0x7C
    uint8_t interactionNeeds[6] = {};       // 0x80
    float field_88 = -1.0f;                 // 0x88 attribute 0x43B894DB
    uint32_t navMode = 0;                   // 0x8C
    ape::Vec3 navObstacleSize{};            // 0x90
    float damage[15] = {};                  // 0xA0 ActorDef::impactScale .. interactionPriority
    uint32_t defFlags = 0;                  // 0xDC bits 22-31
};

// Actor definition base (vtable 0x00B12838). Owns a table of 0xA0-byte part
// records, each holding a Havok rigid body at +0x20.
class ActorDef : public ScriptedEntityDef {
public:
    struct Part : ActorPartFields {
        void* havokObject = nullptr;    // +0x20 Havok rigid body
    };
    // Copies the description and builds a Havok body for every part
    // (0x000C79C0, thunk 0x000E27FC).
    explicit ActorDef(const ActorDefDesc& desc);
    // Slot 7: a PlainActor for the placement (0x000E4314).
    Entity* Spawn(EntityPlacement* placement) override;
    enum PartFlags : uint32_t {
        // The part's body is held at its bone by a constraint (0x000E2AB0).
        kPartFlag_BoneConstraint = 0x10000000,
        // The body is keyframed to follow its part while the actor is active.
        kPartFlag_Keyframed      = 0x80000000,
    };
    // Parts and definitions with this nav mode block navigation.
    static constexpr uint32_t kNavMode_Obstacle = 0xA970EC1Fu;
    static constexpr uint32_t kNavMode_None = 0x0024A738u;   // "None"
    // Definition flags (+0x12C).
    enum Flags : uint32_t {
        kDefFlag_FollowObject   = 1u << 22,  // NOTE: actors always follow their object
        kDefFlag_CarryOnly      = 1u << 23,  // kind 2 only while a carry slot is free
        kDefFlag_FullNavMask    = 1u << 24,  // obstacles block every agent (mask 0xFFFF)
        kDefFlag_Field26        = 1u << 26,  // new actors refuse interactions (ActorEntity::kActorFlag_NoInteraction)
        kDefFlag_KeepListeners  = 1u << 27,  // NOTE: deleted bodies keep the actor as contact listener
    };
    ~ActorDef() override;

protected:
    // NOTE: for subclasses whose console constructors are not ported yet.
    ActorDef() = default;

public:
    // Slot 6: the nav mode is one of the four modes that make the actor's
    // parts static obstacles (0x9260C26E "Static", 0x41EB0AA3, 0x29846DDA and
    // 0x4438A28F "Obstacle"). NOTE: two of the names are unknown.
    uint32_t DefSlot6() const override;
    ape::ApeTArray<Part>* parts = nullptr;   // 0xC0 allocated on demand
    // Answers to InteractionNeeds by interaction kind. NOTE: the number of
    // kinds is not known (at most 12: the nav mode follows at 0xD0).
    uint8_t interactionNeeds[6] = {};        // 0xC4
    float field_CC = -1.0f;                  // 0xCC ActorDefDesc::field_88
    uint32_t navMode = 0;                    // 0xD0
    // Size of the nav obstacle box (zero: shaped like each part's body).
    ape::Vec3 navObstacleSize{};             // 0xE0
    // Damage taken per hit category (0 = immune).
    float impactScale = 0.0f;                // 0xF0 category 1
    float field_F4 = 0.0f;                   // 0xF4
    float blastScale = 0.0f;                 // 0xF8 category 2
    float field_FC = 0.0f;                   // 0xFC
    float collisionScale = 0.0f;             // 0x100 category 0, per unit of speed
    float collisionThreshold = 0.0f;         // 0x104 speed below which nothing happens
    float collisionMax = 0.0f;               // 0x108 (negative: the scripted default field_8C)
    float meleeScale = 0.0f;                 // 0x10C category 3
    float field_110 = 0.0f;                  // 0x110
    float projectileScale = 0.0f;            // 0x114 category 4
    float attackerScale[4] = {};             // 0x118 by the attacker's flag bits 22-23
    float interactionPriority = 0.0f;        // 0x128 scales the priority of kinds 0 and 2
    uint32_t defFlags = 0;                   // 0x12C Flags
};

// Definition of SoundActor and base of its subclasses' definitions
// (vtable 0x00B15970; constructor 0x001AD7F0).
class SoundActorDef : public ActorDef {
public:
    explicit SoundActorDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~SoundActorDef() override;
    // Per slot: a sound to play when it starts (non-zero name) and the
    // slot's default duration (negative: none).
    struct SlotSound {
        uint32_t name = 0;
        float duration = -1.0f;
    };
    SlotSound slotSounds[7];        // 0x130
    ape::Vec3 soundOffset{};        // 0x170 where the sounds play, local to the actor
    uint32_t field_180 = 0;         // 0x180 hash 0x3A15BFAB
    uint32_t field_184 = 0;         // 0x184 hash 0xC03F596F
    float durationScale = 1.0f;     // 0x188 applied with SoundActor::kSeq_Scaled
    uint8_t soundFlags = 0;         // 0x18C bit 7: attribute 0xBD75D672

protected:
    // NOTE: for subclasses whose console constructors are not ported yet.
    SoundActorDef() = default;
};

// Definition of InteractivePropA (vtable 0x00B17DB0, 0x280 bytes).
class InteractivePropADef : public ActorDef {
public:
    ~InteractivePropADef() override;
    ape::Vec3 aimOffset{};              // 0x160 (12 bytes used) aim point, local to the prop's frame
    uint8_t interactionFlags[6] = {};   // 0x170 per interaction kind (InteractivePropC)
    uint32_t bufferSize = 0;        // 0x254
    void* buffer = nullptr;         // 0x258 owned
    float field_278[2] = {};        // 0x278 placement default (radians)
};

// The definition InteractivePropA actually uses (vtable 0x00B17B88).
// NOTE: only the flag its overrides read is modelled.
class InteractivePropAUseDef : public ActorDef {
public:
    explicit InteractivePropAUseDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    ~InteractivePropAUseDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7: an InteractivePropA
    uint32_t sharedUse = 0;         // 0x130 non-zero: several users at once
};

// Safe definition (vtable 0x00B17FF0, 0x1E0 bytes): a combination safe with
// three dials the player turns in turn, each with its own control.
class SafeDef : public ActorDef {
public:
    explicit SafeDef(const struct SafeDefDesc& desc);
    ~SafeDef() override;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7: a Safe (0x001E60E0)
    // Object names (attribute hashes in SafeDefDesc). NOTE: entry k names the
    // object of dial 2 - k.
    uint32_t dialNames[3] = {};         // 0x130 the dials (rotated about +Z)
    uint32_t highlightNames[3] = {};    // 0x13C shown while the dial is turned
    uint32_t solvedNames[3] = {};       // 0x148 shown once the dial is solved
    uint32_t openObjectName = 0;        // 0x154 NOTE: only ever hidden
    float tolerance = 0.0f;             // 0x158 degrees around 0 that solve a dial
    ape::Vec3 field_160{};              // 0x160
    float field_170 = 55.0f;            // 0x170 NOTE: unused by the safe's own code
    float field_174 = 0.5f;             // 0x174 NOTE: unused by the safe's own code
    float timeLimit = 0.0f;             // 0x178 seconds to crack the safe
    float openDelay = 0.0f;             // 0x17C seconds of the opening state before the reward
    float dialSmoothing[3] = {};        // 0x180 share of the remaining turn each dial makes per frame
    uint32_t soundBank = 0;             // 0x18C
    uint32_t tickSounds[3] = {};        // 0x190 every 5 degrees of turn, per dial
    uint32_t solvedSounds[3] = {};      // 0x19C per dial
    uint32_t failSound = 0;             // 0x1A8 the time ran out
    uint32_t openSound = 0;             // 0x1AC
    // Placement defaults.
    uint32_t rewardDefName = 0;         // 0x1B0 definition spawned when the safe opens
    float rewardOffset[3] = {};         // 0x1C0 where, in the safe object's frame
    uint32_t openScript = 0;            // 0x1D0 anim script of the instance
    uint32_t timerHudName = 0;          // 0x1D4 HUD element showing the time left
    uint32_t promptHudName = 0;         // 0x1D8 HUD element with the stick prompt
    float turnSpeed = 4.0f;             // 0x1DC degrees per unit of input
};

// Actor base (0x00B12898 + 0x00B12A44/0x00B12A6C at +0x17C/+0x180).
class ActorEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Builds the actor and a copy of every definition part's rigid body,
    // placed at its bone, locator or offset (0x000E38F0, thunk 0x000E40B4).
    explicit ActorEntity(const objects::ActorInstanceDesc& desc);
    ActorEntity() = default;
    // Derived classes inline it.
    // @func 0x007476B0 ActorEntity::~ActorEntity
    // @func 0x00747764 ActorEntity::~ActorEntity (deleting)
    // @func 0x00747A88 ActorEntity::~ActorEntity (thunk, +0x17C)
    // @func 0x00747900 ActorEntity::~ActorEntity (deleting thunk, +0x17C)
    // @func 0x007479D0 ActorEntity::~ActorEntity (thunk, +0x180)
    // @func 0x00747830 ActorEntity::~ActorEntity (deleting thunk, +0x180)
    ~ActorEntity() override { delete parts; }
    // Replaces the low half (layer and sub-system bits) of every part
    // body's collision filter word (0x000E9728).
    void SetCollisionFilterLow(uint32_t low);
    // Collision layer of the first part that has a body (false without a
    // physics world or a body) (0x000E96A0).
    bool FirstBodyCollisionLayer(uint32_t& layer) const;
    // Slot 5: simulated part bodies pose the bones and locators they stand
    // for; then the physics state timers.
    bool PostUpdate(float dt) override;
    // Slot 4: while active, keyframed part bodies are driven to their parts.
    bool Update(float dt) override;
    // Slot 13: root motion moves only the world object (the actor's
    // transform follows it through the object).
    void ApplyRootMotion(float dt) override;
    bool Release(bool kill) override;                                  // slot 10
    void SetActive(bool active) override;
    ape::Vec3 GetCenter() override;                                    // slot 20
    // Slot 23: hides (on) or shows the actor's bodies, all of them or only
    // part `part`; nav obstacles are rebuilt around it.
    void SetFlag80(bool on, int part = -1) override;
    // Slot 55: hidden bodies become fixed, shown ones get their motion back.
    void ApplyVisibility(bool visible) override;
    void SetTransform(const ape::Matrix34& m) override;                // slot 21
    // Slot 37: a part body stands for its locator object, or for the
    // actor's object at the part's bone.
    ape::world::ApeObj* ObjectForCollidable(const void* collidable, int32_t* bone) override;
    uint32_t GetRenderId() override;                                   // slot 25
    void SetRenderId(uint32_t id) override;                            // slot 26
    float GetGravityFactor() override;                                 // slot 27
    void Detached(float gravityFactor) override;                       // slot 28
    void HandleMessage(const EntityMessage& msg) override;             // slot 43
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;   // slot 76
    // Slots 94-96: nav obstacles of the parts with bodies (definition nav
    // mode kNavMode_Obstacle): rebuilt, removed, added.
    virtual void RefreshNavObstacles();
    virtual void RemoveNavObstacles();
    virtual void AddNavObstacles();
    // Physics states (+0x188 bits 1-31). NOTE: names from what SetPhysState
    // does with them; 2 is only compared against.
    enum ActorPhysState : int {
        kActorPhys_Enter   = 1,   // bodies into the world, interactive
        kActorPhys_InWorld = 2,
        kActorPhys_Remove  = 3,   // stored as kActorPhys_Removed
        kActorPhys_Dead    = 4,   // set when the actor is released
        kActorPhys_Removed = 5,
        kActorPhys_Gone    = 6,   // PostUpdate stops here
        kActorPhys_Active  = 7,   // like kActorPhys_Enter; 7 and up are interactive
    };
    int PhysState() const { return static_cast<int>(physWord >> 1); }
    // Slot 104.
    virtual void SetPhysState(int state, uint32_t arg = 0);
    enum ActorFlags : uint32_t {
        kActorFlag_NoInteraction = 0x40000000,
        kActorFlag_Dead          = 0x80000000,
    };

    // One physical part of the actor (0x30-byte records at +0x184).
    struct Part {
        ape::Vec3 offset{};                  // +0x00 in the part's frame
        ape::world::ApeObj* locator = nullptr; // +0x10 object the part follows, or null
        int32_t bone = -1;                   // +0x14 bone of the actor's object, -1 for none
        uint32_t field_18 = 0;
        void* body = nullptr;                // +0x1C Havok rigid body, or null
        void* navObstacle = nullptr;         // +0x20 nav obstacle handle
        uint32_t motionType = 0;             // +0x24 Havok motion type while visible
        uint8_t field_28[0x8] = {};
    };
    // World transform of part `index` placed from `frame` (the actor's
    // transform unless the part follows a bone or locator), or `frame`
    // itself when the actor has no parts.
    void PartTransform(ape::Matrix34& out, int index, const ape::Matrix34& frame) const;

    // Slot 71: an interactive actor is aimed at at its position; any other
    // at its first definition part's offset from its first part (or from
    // its object).
    ape::Vec3 GetAimPoint() override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    bool Damage(float amount, Entity* source, uint32_t type) override;
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory1(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;
    // Slot 82: some hit category and some attacker can hurt the actor.
    bool CanBeDamaged() const override;
    // Slot 89: damage scale of projectiles.
    float ProjectileDamageScale() const override;
    // Slots 98-103: the Havok listener callbacks the actor overrides (each
    // also has a this-adjusting thunk in the interface's vtable) and two
    // empty slots of its own. NOTE: the meanings of 99/103 are unknown.
    // Primary slot 98 / +0x17C slot 5.
    // @func 0x00747C18 ActorEntity::Interface17CSlot5
    // @func 0x00747C1C ActorEntity::Interface17CSlot5 (thunk, +0x17C)
    void Interface17CSlot5(const void* event) override { (void)event; }
    // @func 0x00747C20 ActorEntity::ActorSlot99
    virtual void ActorSlot99() {}
    // Primary slot 100 / +0x17C slot 7.
    // @func 0x00747C24 ActorEntity::Interface17CSlot7
    // @func 0x00747C28 ActorEntity::Interface17CSlot7 (thunk, +0x17C)
    void Interface17CSlot7() override {}
    // Primary slot 101 / +0x17C slot 6.
    // @func 0x00747C2C ActorEntity::Interface17CSlot6
    // @func 0x00747C30 ActorEntity::Interface17CSlot6 (thunk, +0x17C)
    void Interface17CSlot6() override {}
    // Primary slot 102 / +0x180 slot 6: stops listening to the contacts of a
    // body being deleted, unless the definition keeps its listeners (flag 27).
    void Interface180Slot6(void* body) override;
    // @func 0x00747CD0 ActorEntity::ActorSlot103
    virtual void ActorSlot103() {}
    // Slot 36: the body of a single-part actor, null otherwise. NOTE: see
    // NOTES on slot 36.
    ape::world::ApeObj* GetRenderObject() override;
    // Slots 41/42: the definition's nav mode is not "None" / is the
    // obstacle mode.
    int Slot41() override;
    int Slot42() override;

    // Filter consulted before projectile damage (+0x1A0); false ignores the hit.
    using HitFilter = bool (*)(ActorEntity* actor, EntityHitRecord& hit);

    ape::ApeTArray<Part>* parts = nullptr;        // 0x184 allocated on demand
    // 0x188 bits 1-31 physics state; bit 0: the actor follows its world
    // object (the transform is taken from it while active).
    uint32_t physWord = 0;
    int32_t field_18C = 0;                        // 0x18C negative: keyframed bodies are left alone
    Entity* field_190 = nullptr;                  // 0x190 NOTE: refused as an interaction partner
    // Seconds the actor may stay in the world once it entered (negative:
    // forever) and the time spent there (reset on entering).
    float lifetime = -1.0f;                       // 0x194
    float stateTime = 0.0f;                       // 0x198
    uint32_t collisionGroup = 0;                  // 0x19C
    HitFilter projectileFilter = nullptr;         // 0x1A0
    uint32_t actorFlags = 0;                      // 0x1A4 bit 30: no interactions, bit 31: health ran out

private:
    const ActorDef* Def() const { return static_cast<const ActorDef*>(def); }
    // World transform of `part` from `frame`.
    void PlacePart(ape::Matrix34& out, const Part& part, const ape::Matrix34& frame) const;
    // Obstacle of one part; whether a part blocks navigation at all.
    void AddPartNavObstacle(size_t index);
    bool PartBlocksNavigation(size_t index) const;
    void ConstrainPartToBone(int index, const ape::Vec3& velocity);
    // Applies `amount` through slot 88 and marks the hit fatal if it killed.
    void ApplyHitDamage(EntityHitRecord& hit, float amount, uint32_t type);
};

// Interactive prop that refuses player interaction while its definition says
// so (vtable 0x00B17BB8, 0x1B0 bytes; slot 76 0x001DC3A8).
class InteractivePropA : public ActorEntity {
public:
    using ActorEntity::ActorEntity;
    ~InteractivePropA() override;
    bool Update(float dt) override;
    // A player may not use the prop while one user holds it and the player
    // is busy (player block +0x10DC positive, unless the driver is idle).
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;   // slot 76
    // Stops a playing scripted animation of the user.
    bool InteractionBegin(Entity* user) override;              // slot 62
};

// Definition of SearchlightActor (vtable 0x00B18230; stand-in).
class SearchlightDef : public ActorDef {
public:
    explicit SearchlightDef(const ActorDefDesc& desc) : ActorDef(desc) {}
    // @func 0x00757308 SearchlightDef::~SearchlightDef
    // @func 0x00757338 SearchlightDef::~SearchlightDef (deleting)
    ~SearchlightDef() override = default;
    Entity* Spawn(EntityPlacement* placement) override;   // slot 7: a SearchlightActor
    uint8_t interactionNeeds[6] = {};   // 0x12E per interaction kind
    ape::Vec3 cameraOffset{};           // 0x140 camera position relative to the light
    // 0x150 rotation (quaternion) from the Euler angles of attribute
    // 0x91B5F872. NOTE: read by the spotlight camera constructor.
    ape::Vec4 field_150{0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t field_160[4] = {};         // 0x160 hashes 0xC8168A44, 0x942EEC95, 0x7D68F822, 0x4C4DB3D0
    float field_170 = 1.0f;             // 0x170 attribute 0x35F1E76E
};

// Player-steered searchlight (vtable 0x00B18260, 0x1D0 bytes); the spotlight
// camera mode rides on it.
class SearchlightActor : public ActorEntity {
public:
    // Looks up the four objects the definition names; the light object
    // starts hidden (0x001E9C90).
    explicit SearchlightActor(const objects::ActorInstanceDesc& desc);
    ~SearchlightActor() override;
    // Slot 71: the actor's own copy of the actor aim point.
    ape::Vec3 GetAimPoint() override;
    bool Release(bool kill) override;
    // Offers interaction kind 4 (bit value 0x10) to everyone.
    uint32_t InteractionOffer(int kind, Entity* other) override;
    // Per-kind byte of the definition (+0x12E).
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    // 0x001EB0BC / 0x001EB0DC
    ape::Vec3 CameraOffset() const;
    ape::Vec4 Field150() const;
    // The user takes the light: the spotlight camera rides on it and the user
    // is hidden while the light model shows.
    bool InteractionBegin(Entity* user) override;     // slot 62
    // Gives the light back: the camera leaves at once, the user shows again,
    // the light object hides and the other objects are opaque (slot 63).
    void InteractionCancel() override;
    // Slot 4: while in use the light model fades in over 0.7 s (the other
    // objects out), and a user who asks to leave from the light's camera
    // ends the use.
    bool Update(float dt) override;

    const SearchlightDef* Def() const { return static_cast<const SearchlightDef*>(def); }

    camera::CameraMode* cameraMode = nullptr;   // 0x1A8 spotlight camera mode
    Entity* user = nullptr;                     // 0x1AC
    ape::world::ApeObj* object_1B0 = nullptr;   // 0x1B0 SearchlightDef::field_160[0]
    ape::world::ApeObj* object_1B4 = nullptr;   // 0x1B4 field_160[1]
    ape::world::ApeObj* lightObject = nullptr;  // 0x1B8 field_160[2]
    ape::world::ApeObj* object_1BC = nullptr;   // 0x1BC field_160[3]
    float useTime = 0.0f;                       // 0x1C0 seconds in use
};

// The actor ActorDef itself spawns (vtable 0x00B12A90 + 0x00B12C3C/0x00B12C64;
// built by ActorDef's Spawn 0x000E4314): adds nothing to the base.
class PlainActor : public ActorEntity {
public:
    using ActorEntity::ActorEntity;
    ~PlainActor() override;
};

// Interactive prop driven by an anim script variable (vtable 0x00B18908,
// 0x240 bytes; slot 76 0x001F6808).
class InteractivePropB : public ActorEntity {
public:
    // Placement values; spawns the attached entity at once when the
    // placement asks for it (0x001F54E8).
    explicit InteractivePropB(const objects::InteractivePropBInstanceDesc& desc);
    ~InteractivePropB() override;
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;
    bool Release(bool kill) override;                 // slot 10
    // Takes over the interacting entity's transform and starts state 1.
    bool InteractionBegin(Entity* user) override;                 // slot 62
    // NOTE: both only look the interaction component up.
    void InteractionCancel() override;                // slot 63
    void InteractionEnd(bool completed) override;     // slot 64
    // Showing spawns the attached entity (state 0); hiding detaches and
    // kills it (state 3).
    void ApplyVisibility(bool visible) override;      // slot 55
    // Slot 76: in state 0 with the attached entity, while the time (+0x1FC)
    // runs or the script's int 0xDD61613C is set.
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;
    // Spawns the entity of definition `attachedDef` and attaches it (0x001F50E4).
    void SpawnAttached();

    ape::Matrix34 userTransform{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};   // 0x1B0 (identity)
    Entity* attached = nullptr;               // 0x1F0
    uint32_t field_1F4 = 0;                   // 0x1F4
    float field_1F8 = 0.0f;                   // 0x1F8 placement value
    float useTime = 0.0f;                     // 0x1FC NOTE: name guessed
    float field_200 = 0.0f;                   // 0x200 placement value
    uint32_t field_204 = 0;                   // 0x204 placement value (hash)
    ape::Vec3 field_210{};                    // 0x210 placement offset
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};   // 0x220 placement rotation
    uint32_t attachedDef = 0;                 // 0x230 definition name of the attached entity
};

// Interactive prop with two phantoms (vtable 0x00B17DF8; destructor
// 0x00756758). NOTE: its definition is the class at
// 0x00B17DB0, which ActorEntities.h calls InteractivePropADef.
class InteractivePropC : public ActorEntity {
public:
    // @func 0x00756758 InteractivePropC::~InteractivePropC
    // @func 0x00756810 InteractivePropC::~InteractivePropC (deleting)
    // @func 0x00756B44 InteractivePropC::~InteractivePropC (thunk, +0x17C)
    // @func 0x007569B4 InteractivePropC::~InteractivePropC (deleting thunk, +0x17C)
    // @func 0x00756A88 InteractivePropC::~InteractivePropC (thunk, +0x180)
    // @func 0x007568E0 InteractivePropC::~InteractivePropC (deleting thunk, +0x180)
    ~InteractivePropC() override = default;
    bool Release(bool kill) override;                 // slot 10
    void InteractionEnd(bool completed) override;     // slot 64: cancels
    // Offers interaction kind 0 to everyone.
    uint32_t InteractionOffer(int kind, Entity* other) override;
    uint32_t InteractionNeeds(int kind, Entity* other) override;
    // Player interaction is refused while a single user holds the prop and
    // the asking character does not carry its key object.
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;
    // Slot 71: the definition's aim offset in the frame of `frameObject`
    // (or of the prop).
    ape::Vec3 GetAimPoint() override;

    ape::world::ApeObj* frameObject = nullptr; // 0x1AC
    Entity* keyObject = nullptr;              // 0x1B0
    uint8_t propFlags = 0;                    // 0x1B4 bit 7: anyone may interact
    void* phantomA = nullptr;                 // 0x270
    void* phantomB = nullptr;                 // 0x274
};

// Actor with seven per-instance sounds (vtable 0x00B159B8); base of the
// three classes below.
class SoundActor : public ActorEntity {
public:
    ~SoundActor() override;
    // The anim script is paused while the actor is inactive.
    void SetActive(bool active) override;

    bool Release(bool kill) override;                 // slot 10 (0x001AE6AC)
    void OnSuspend() override;                        // slot 60 (0x001AE768)
    // Runs the slot timer of the current state until one keeps waiting
    // (slot 4, 0x001AEA50).
    bool Update(float dt) override;

    // Slot 105: plays sound slot `index` and switches to its state. Durations
    // are scaled while the player runs a script animation (driver state 7).
    virtual void PlaySlot(int index, bool force);
    // Slots 106-112: timers of the slot sequence. Unless the sequence is held
    // (flag bit 29) each moves on to its next slot once `elapsed` reaches
    // `duration` (a negative duration: at once) and returns false; true
    // keeps waiting. Slot 110 never moves on with a negative duration.
    virtual bool AdvanceTo1(float elapsed, float duration);    // slot 106
    virtual bool AdvanceTo2(float elapsed, float duration);    // slot 107
    virtual bool AdvanceTo3(float elapsed, float duration);    // slot 108
    virtual bool AdvanceTo4(float elapsed, float duration);    // slot 109 (forced)
    virtual bool AdvanceTo5(float elapsed, float duration);    // slot 110
    virtual bool AdvanceTo6(float elapsed, float duration);    // slot 111
    virtual bool AdvanceWrap(float elapsed, float duration);   // slot 112 (back to 1)
    // Switches the anim script to the state of slot `index`, running it at
    // once when `immediate` (slot 113, 0x001AF40C).
    virtual void SetSlotState(int index, bool immediate);
    // Slot 114: resumes the sequence at the saved slot and time (0x001AF56C).
    virtual void ResumeSequence();

    // Plays the slot whose name is `name` (0x001AE9C8).
    void PlaySlotByName(uint32_t name);
    // Starts the sequence at `slot`, skipping whole slots `time` covers;
    // the rest becomes the state time (0x001AF470).
    void PlayFrom(float time, int slot, bool force);
    // Duration of a slot, scaled by the definition when flag bit 30 is set.
    float SlotDuration(int index) const;

    // Sequence flags (+0x228).
    enum SequenceFlags : uint32_t {
        kSeq_ResumePending = 1u << 31, // resume the saved sequence on the next update
        kSeq_Held = 1u << 29,          // the timers do not advance
        kSeq_Scaled = 1u << 30,        // durations scaled by the definition (+0x188)
    };

    struct SoundSlot {
        audio::SoundName* sound = nullptr;
        audio::VoiceId voice = 0;                   // +0x04
        ape::ApeAnimScriptState* state = nullptr;   // +0x08
        float duration = 0.0f;                      // +0x0C
    };
    SoundSlot sounds[7];                      // 0x1A8
    struct SlotTimer {                        // NOTE: only the duration is known
        uint8_t field_00[0x14];
        float duration;                       // +0x14 takes the slot duration
    };
    SlotTimer* slotTimer = nullptr;           // 0x218
    int32_t currentSlot = -1;                 // 0x21C
    float savedTime = 0.0f;                   // 0x220
    int32_t savedSlot = 0;                    // 0x224
    uint32_t sequenceFlags = 0;               // 0x228 SequenceFlags
};

// vtable 0x00B16B48 (0x260 bytes).
class SoundActorB16B48 : public SoundActor {
public:
    ~SoundActorB16B48() override;
    // Slot 65: only while layer 0 is in state 1.
    bool Slot65() override;
    // Slot 105: slot 1 also clears the lit flag and switches the light on.
    void PlaySlot(int index, bool force) override;
    bool AdvanceTo2(float elapsed, float duration) override;   // only while lit
    bool AdvanceTo3(float elapsed, float duration) override;
    bool AdvanceTo4(float elapsed, float duration) override;
    bool AdvanceWrap(float elapsed, float duration) override;
    // Shows or hides the light objects and, when the definition says so,
    // puts the light's bodies into the world at their parts (or takes them
    // out). NOTE: name from the flag it pairs with; the lit flag itself is
    // not changed here.
    void SetLight(bool on);

    ape::ApeTArray<ape::world::ApeObj*> lightObjects;   // 0x22C
    ape::ApeTArray<void*> lightBodies;        // 0x240 Havok rigid bodies
    uint8_t lightFlags = 0;                   // 0x25C bit 7 lit, bit 5 held
};

// Definition of SoundActorB16B48. NOTE: only the options its timers read.
class SoundActorB16B48Def : public SoundActorDef {
public:
    // The sound actor fields, then the two lists and the options (0x001C1D4C).
    explicit SoundActorB16B48Def(const struct SoundActorB16B48DefDesc& desc);
    ~SoundActorB16B48Def() override;
    ape::ApeTArray<uint32_t> lightNames; // 0x190 names of the light objects
    ape::ApeTArray<int32_t> bodyParts; // 0x1A4 part index of each light body
    float field_1B8 = -1.0f;           // 0x1B8 placement default (attribute 0x2DF33CE3)
    bool restartWhenWaiting = false;   // 0x1BC attribute 0x45703A61
    bool unlightOnSlot3 = true;        // 0x1BD attribute 0xB10A3CDE
    bool unlightOnSlot4 = false;       // 0x1BE attribute 0x89D243B8
    bool lightHasBodies = true;        // 0x1BF attribute 0x428E4540
};

// Sound actor with a two-way switch (vtable 0x00B16678; destructor
// 0x0075420C).
class SoundActorB16678 : public SoundActor {
public:
    // @func 0x0075420C SoundActorB16678::~SoundActorB16678
    // @func 0x0075424C SoundActorB16678::~SoundActorB16678 (deleting)
    // Slot 75: the switch always takes part in interactions.
    // @func 0x00754428 SoundActorB16678::InteractionNeeds
    uint32_t InteractionNeeds(int kind, Entity* other) override { (void)kind; (void)other; return 1; }
    // @func 0x007543E0 SoundActorB16678::~SoundActorB16678 (thunk, +0x17C)
    // @func 0x00754328 SoundActorB16678::~SoundActorB16678 (deleting thunk, +0x17C)
    // @func 0x00754398 SoundActorB16678::~SoundActorB16678 (thunk, +0x180)
    // @func 0x007542B8 SoundActorB16678::~SoundActorB16678 (deleting thunk, +0x180)
    ~SoundActorB16678() override = default;
    // The class carries its own copy of SoundActor::Release (0x001BF794).
    bool Release(bool kill) override;
    bool AdvanceTo2(float elapsed, float duration) override;   // only while armed
    bool AdvanceTo5(float elapsed, float duration) override;
    // Slots 1 and 4 use the states of the switch position.
    void SetSlotState(int index, bool immediate) override;
    // Also runs a pending refresh and counts the delay down.
    bool Update(float dt) override;                   // slot 4
    // Flips the switch (or, with a non-negative reset time, turns it on and
    // starts the time) and reports it for the player that flipped it.
    void Refresh();
    // Messages 0x4DA1B5C3 / 0x59676D2A unlock / lock the switch.
    void HandleMessage(const EntityMessage& msg) override;    // slot 43
    // Collisions above the threshold, and melee and projectile hits from
    // players, flip the switch (on the next update).
    void OnHitCategory0(EntityHitRecord& hit) override;       // slot 83
    void OnHitCategory1(EntityHitRecord& hit) override;       // slot 84
    void OnHitCategory4(EntityHitRecord& hit) override;       // slot 87
    // A hit is taken and the switch may flip now.
    bool AcceptsFlip(EntityHitRecord& hit);
    // Queues a flip credited to `player`.
    void QueueFlip(int player) { switchFlags |= 0x40; flipPlayer = player; }

    int32_t position = 0;          // 0x22C 0 or 1 (indexes the state tables)
    int32_t flipPlayer = 0;        // 0x230 player slot credited with the flip
    float resetTime = 0.0f;        // 0x234 non-negative: flips only turn on, for this long
    float delay = 0.0f;            // 0x238 slot 5 waits while positive
    // Bit 3 used, bit 4 NOTE: unknown (skips the checks), bit 5 locked,
    // bit 6 flip pending, bit 7 locks after one flip.
    uint8_t switchFlags = 0;       // 0x23C
};

// vtable 0x00B168E0 (destructor 0x0075458C).
class SoundActorB168E0 : public SoundActor {
public:
    // @func 0x0075458C SoundActorB168E0::~SoundActorB168E0
    // @func 0x007545CC SoundActorB168E0::~SoundActorB168E0 (deleting)
    // @func 0x00754760 SoundActorB168E0::~SoundActorB168E0 (thunk, +0x17C)
    // @func 0x007546A8 SoundActorB168E0::~SoundActorB168E0 (deleting thunk, +0x17C)
    // @func 0x00754718 SoundActorB168E0::~SoundActorB168E0 (thunk, +0x180)
    // @func 0x00754638 SoundActorB168E0::~SoundActorB168E0 (deleting thunk, +0x180)
    ~SoundActorB168E0() override = default;
    bool AdvanceTo2(float elapsed, float duration) override;   // only while armed
    bool AdvanceTo3(float elapsed, float duration) override;
    bool AdvanceTo5(float elapsed, float duration) override;   // not while armed

    uint8_t flags240 = 0;          // 0x240 bit 3 armed, bit 7 reached slot 3
};

// Sound actor with six phantom-tracked entities (vtable 0x00B16DB0;
// destructor 0x00754B74).
class SoundActorB16DB0 : public SoundActor {
public:
    // @func 0x00754B74 SoundActorB16DB0::~SoundActorB16DB0
    // @func 0x00754BB4 SoundActorB16DB0::~SoundActorB16DB0 (deleting)
    // @func 0x00754D48 SoundActorB16DB0::~SoundActorB16DB0 (thunk, +0x17C)
    // @func 0x00754C90 SoundActorB16DB0::~SoundActorB16DB0 (deleting thunk, +0x17C)
    // @func 0x00754D00 SoundActorB16DB0::~SoundActorB16DB0 (thunk, +0x180)
    // @func 0x00754C20 SoundActorB16DB0::~SoundActorB16DB0 (deleting thunk, +0x180)
    ~SoundActorB16DB0() override = default;
    bool Release(bool kill) override;
    // Both wait while something occupies the actor, depending on flag bit 29.
    bool AdvanceTo2(float elapsed, float duration) override;
    bool AdvanceTo5(float elapsed, float duration) override;
    // Slot 5: the tracked triggers report while the actor is in state 4
    // past the definition's delay, or (flag bit 26) in state 6.
    bool PostUpdate(float dt) override;
    bool IsOccupied() const;
    static constexpr uint32_t kOccupancy_Bit26 = 1u << 26;
    static constexpr uint32_t kOccupancy_Bit29 = 1u << 29;

    struct Tracked {
        void* phantom = nullptr;   // +0x00 Havok phantom
        Entity* entity = nullptr;  // +0x04
        uint8_t pad[0x0C];
    };
    Entity* attached = nullptr;    // 0x22C
    uint32_t field_234 = 0;        // 0x234
    float holdTime = 0.0f;         // 0x240
    uint32_t occupancyFlags = 0;   // 0x244 bit 29 (see the timers). NOTE: meaning unknown
    Tracked tracked[6];            // 0x248
};

// Definition of SoundActorB16DB0. NOTE: only the flag its timer reads.
class SoundActorB16DB0Def : public SoundActorDef {
public:
    // One tracked trigger volume (0x50 bytes; "Trigger" children).
    struct Volume {
        uint32_t field_00 = 0;                      // +0x00 hash 0x75849CFE
        ape::Vec3 field_10{};                       // +0x10 attribute 0x2B7B3EB4
        ape::Vec3 field_20{};                       // +0x20 attribute 0x313AB0C1
        ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f}; // +0x30 from attribute 0x313AB844 (degrees)
        uint32_t field_40 = 0;                      // +0x40 hash 0x632DA119
        bool field_44 = true;                       // +0x44 attribute 0x8141EC8A
        bool field_45 = false;                      // +0x45 attribute 0xF8E455AC
    };
    static constexpr int kVolumeCount = 6;
    // The sound actor fields, then its own (0x001C463C).
    explicit SoundActorB16DB0Def(const struct SoundActorB16DB0DefDesc& desc);
    ~SoundActorB16DB0Def() override;
    ape::Vec3 angles{};            // 0x190 attribute 0x3C6AE50B (degrees in the file)
    float field_1A0 = 0.0f;        // 0x1A0 attribute 0x7D873487 (not negative)
    float triggerDelay = -1.0f;    // 0x1A4 time in state 4 before the triggers report (0x7BC2B446)
    float field_1A8 = 0.0f;        // 0x1A8 attribute 0x1B75F0DA (not negative)
    // Placement defaults (SoundActorB16DB0Placement +0x110..+0x11C).
    uint32_t field_1AC = 0;        // 0x1AC hash 0x2DF2E2B5
    uint32_t field_1B0 = 0;        // 0x1B0 hash 0x53FFAFC2
    float field_1B4 = -1.0f;       // 0x1B4 attribute 0x667AB4F5
    float field_1B8 = -1.0f;       // 0x1B8 attribute 0x7A392528
    // 0x1BC bits 30-22 from nine attributes (bit 28: close even when
    // occupied; bit 27: keep the default volumes; bit 22: placement
    // default); bits 16-21: 5 or 3 from the attributes 0xCD7DD617 /
    // 0xBC50985B.
    uint32_t options = 0;
    Volume volumes[kVolumeCount];  // 0x1C0
};

// Like SoundActorB16DB0 with the records at +0x22C (vtable 0x00B17008;
// destructor 0x00754E18).
class SoundActorB17008 : public SoundActor {
public:
    // @func 0x00754E18 SoundActorB17008::~SoundActorB17008
    // @func 0x00754E58 SoundActorB17008::~SoundActorB17008 (deleting)
    // @func 0x00754FEC SoundActorB17008::~SoundActorB17008 (thunk, +0x17C)
    // @func 0x00754F34 SoundActorB17008::~SoundActorB17008 (deleting thunk, +0x17C)
    // @func 0x00754FA4 SoundActorB17008::~SoundActorB17008 (thunk, +0x180)
    // @func 0x00754EC4 SoundActorB17008::~SoundActorB17008 (deleting thunk, +0x180)
    ~SoundActorB17008() override = default;
    bool Release(bool kill) override;
    bool AdvanceTo2(float elapsed, float duration) override;
    bool AdvanceTo5(float elapsed, float duration) override;
    // Slot 5: the tracked triggers report while the actor is in state 4.
    bool PostUpdate(float dt) override;

    struct Tracked {               // 0x10-byte records
        void* phantom = nullptr;   // +0x00 Havok phantom
        Entity* entity = nullptr;  // +0x04
        uint8_t pad[0x08];
    };
    Tracked tracked[6];            // 0x22C
};

// vtable 0x00B17728 (destructor 0x00755374): the
// sequence only advances from slot 6 back to 1.
class SoundActorB17728 : public SoundActor {
public:
    // @func 0x00755374 SoundActorB17728::~SoundActorB17728
    // @func 0x007553B4 SoundActorB17728::~SoundActorB17728 (deleting)
    // @func 0x00755548 SoundActorB17728::~SoundActorB17728 (thunk, +0x17C)
    // @func 0x00755490 SoundActorB17728::~SoundActorB17728 (deleting thunk, +0x17C)
    // @func 0x00755500 SoundActorB17728::~SoundActorB17728 (thunk, +0x180)
    // @func 0x00755420 SoundActorB17728::~SoundActorB17728 (deleting thunk, +0x180)
    ~SoundActorB17728() override = default;
    bool Release(bool kill) override;
    // The first update looks the attached entity up by name.
    bool Update(float dt) override;                   // slot 4
    // @func 0x001D4B6C SoundActorB17728::AdvanceTo2
    bool AdvanceTo2(float, float) override { return true; }
    // @func 0x001D4B74 SoundActorB17728::AdvanceTo4
    bool AdvanceTo4(float, float) override { return true; }
    // @func 0x001D4B7C SoundActorB17728::AdvanceTo5
    bool AdvanceTo5(float, float) override { return true; }
    // @func 0x001D4B84 SoundActorB17728::AdvanceTo6
    bool AdvanceTo6(float, float) override { return true; }
    bool AdvanceWrap(float elapsed, float duration) override;
    // Slot 5: opens (slot 2) or closes (slot 6) when holdWrap changes, except
    // in state 5.
    bool PostUpdate(float dt) override;
    // Whether the actor stands open: its attached entity's +0x160 is set,
    // or a player is within range (and, when the definition asks for one,
    // is the right character) (0x001D48B8).
    void UpdateOpen(bool& open);

    Entity* attached = nullptr;    // 0x22C counted
    uint32_t attachedName = 0;     // 0x230
    float rangeSq = 0.0f;          // 0x234 squared player distance (<= 0: open without one)
    uint8_t lookupFlags = 0;       // 0x238 bit 7: looked up
    bool holdWrap = false;         // 0x239 open
};

// Gadget turret (vtable 0x00B17270, 0x360 bytes; entity type bit 57).
// NOTE: identified from the turret hooks of the level scripts.
class SoundActorB17270 : public SoundActor {
public:
    ~SoundActorB17270() override;
    // Offers interaction kind 2 (bit value 4).
    // @func 0x001CEC84 SoundActorB17270::InteractionOffer
    uint32_t InteractionOffer(int kind, Entity* other) override { (void)kind; (void)other; return 4; }
    // The sequence only moves while layer 1 is idle and the actor is engaged
    // (+0x356); slot 2 never advances on its own.
    // @func 0x001CF700 SoundActorB17270::AdvanceTo2
    bool AdvanceTo2(float, float) override { return true; }
    bool AdvanceTo3(float elapsed, float duration) override;
    bool AdvanceTo4(float elapsed, float duration) override;
    bool AdvanceTo5(float elapsed, float duration) override;
    bool AdvanceTo6(float elapsed, float duration) override;
    bool AdvanceWrap(float elapsed, float duration) override;
    // Slots 2 and 6 have alternative states while engaged.
    void SetSlotState(int index, bool immediate) override;
    // Also disengages.
    void ResumeSequence() override;
    // Also lets go of the linked entity.
    bool Release(bool kill) override;                          // slot 10
    // Only interaction kind 0. NOTE: the console also scales the caller's score by 0.3.
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;   // slot 76
    // Projectiles from others strike the engaged actor (definition impact scale > 0).
    void OnHitCategory4(EntityHitRecord& hit) override;       // slot 87
    // Layer 1 in state 2 plays the struck state 0x803E1667 (unless
    // invulnerable); otherwise layer 1 goes to state 2 and layer 2 to 0.
    void OnStruck();
    // Wanted aim direction towards `point` from the aim origin.
    void AimAt(const ape::Vec3& point);
    // The current aim has reached the wanted direction.
    bool IsAimed() const;
    // Shots past the first wait the weapon's interval each (+0x1F4).
    void ResetAim();
    // Slot 21: the aim frame follows; the aim resets to its z axis (kept
    // level with turretFlags bit 7).
    void SetTransform(const ape::Matrix34& m) override;
    // Adds a target (counted reference).
    void AddTarget(Entity* target);
    // Lets go of every target and keeps only `target` (if any).
    void SetOnlyTarget(Entity* target);

    float shotDelay = 0.0f;                   // 0x1F4 NOTE: set by ResetAim
    ape::Matrix34 aimFrame{};                 // 0x230 z axis = rest aim, translation = aim origin
    ape::Matrix34 baseTransform{};            // 0x270 last transform set
    ape::Vec3 restAim{};                      // 0x2B0
    ape::Vec3 wantedAim{};                    // 0x2C0 unit direction
    ape::Vec3 currentAim{};                   // 0x2D0 unit direction
    EntityComponent* weapon = nullptr;        // 0x2E0 skin component; its +0x4D0 is the shot interval
    Entity* linked = nullptr;                 // 0x2F0 released on kill
    float field_2FC = 0.0f;                   // 0x2FC copied into the cool-down by slot 4
    float coolDown = 0.0f;                    // 0x314 slots 3 and 6 wait while positive
    int32_t shotCount = 0;                    // 0x30C
    uint8_t turretFlags = 0;                  // 0x310 bit 7: aims level
    uint32_t field_328 = 0;                   // 0x328 cleared by SetOnlyTarget
    ape::ApeTArray<Entity*> targets;          // 0x32C counted references
    bool field_355 = false;                   // 0x355 NOTE: selects the alternative states
    bool engaged = false;                     // 0x356
    uint8_t flags357 = 0;                     // 0x357 bit 7 set by slot 4, cleared by slot 5
};

// Sentry light (vtable 0x00B174C0, 0x3D0 bytes; entity type bit 27 of
// +0xC4). NOTE: identified from the sentry light hooks of the level scripts.
class SoundActorB174C0 : public SoundActor {
public:
    ~SoundActorB174C0() override;
    // The class carries its own copy of SoundActor::Release (0x001D20D0).
    bool Release(bool kill) override;
    // Turns the patrol around: bits 30 and 31 both become the inverse of bit 31.
    // @func 0x001D3A00 SoundActorB174C0::ReversePatrol
    void ReversePatrol()
    {
        const bool wasReversed = (lightFlags & 0x80000000u) != 0;
        lightFlags = (lightFlags & 0x3FFFFFFFu) | (wasReversed ? 0u : 0xC0000000u);
    }

    ape::ApeTArray<void*> field_22C;          // 0x22C
    // Bit 26 locked, bit 27 on, bit 29 NOTE: unknown, bits 30-31 patrol direction.
    uint32_t lightFlags = 0;                  // 0x3CC
};

// Actor holding a world object reference (vtable 0x00B18488, 0x200 bytes).
// NOTE: this is the teleporter ("Sly_Teleporter" settings, "Teleporter"
// definition element).
class ObjectActor : public ActorEntity {
public:
    // Keeps the destination; a destination object in this level is looked
    // up and referenced (0x001EB478).
    explicit ObjectActor(const objects::ObjectActorInstanceDesc& desc);
    ~ObjectActor() override;
    // Slot 62: asks the game for the trip (game state layer 1 state 7) with
    // the destination; refused while the teleporter does not teleport.
    bool InteractionBegin(Entity* user) override;
    // Shows or hides the teleporter and switches its interaction (0x001EB814).
    void SetEnabled(bool enabled);

    char destControllable[0x40] = {};         // 0x1A5
    uint32_t destObj = 0;                     // 0x1E8
    uint32_t destLevel = 0;                   // 0x1EC
    uint32_t destSpawnPoint = 0;              // 0x1F0
    ape::world::ApeObj* object_1F4 = nullptr; // 0x1F4 counted reference: the destination object
    bool noTeleport = false;                  // 0x1F8
    bool destControllableCurrentPlayer = false;   // 0x1F9
};

// Association callback of the "Sly_Teleporter" setting (0x001EB9CC): the
// teleporter is built for the object `objectName` once the level runs.
void AssociateTeleporter(ape::setting::Setting* setting, const char* objectName, bool reload);

// Actor with two tweens and two arrays (vtable 0x00B186C8). NOTE: it is the
// target range ("Entity.TgtRange.*" tuning: its camera leaves over
// CamPopTime).
class TweenActor : public ActorEntity {
public:
    ~TweenActor() override;
    bool Update(float dt) override;
    bool PostUpdate(float dt) override;
    // The actor places itself; the base transform update is skipped.
    void UpdateTransform(float dt) override;
    // Hiding the actor also triggers its reset while one is pending.
    void ApplyVisibility(bool visible) override;
    // 0x001EE680
    void Reset(bool completed);
    // Shows or hides the user's script object 0x03BA7BE9 (0x001EE5A4).
    void SetUserObjectVisible(bool visible);
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;   // slot 22
    bool InteractionBegin(Entity* user) override;                 // slot 62
    void InteractionCancel() override;                // slot 63: Reset(false)
    void InteractionEnd(bool completed) override;     // slot 64: Reset(completed)
    // Takes the interacting entity (a character; anything else counts as
    // none) and lets go of the previous one: the user is kept from being
    // culled, counted as interacting and posed by the actor through its
    // animation controller's pose hook (0x001EDD84).
    void TakeUser(Entity* user);
    // The pose hook (0x001F4440): poses three bones of the user.
    static void UserPoseHook(void* actor);
    // TODO(0x001F4098 TweenActor::PoseUser)
    void PoseUser() {}
    bool Release(bool kill) override;                 // slot 10

    GameCamera* camera = nullptr;             // 0x1F0 game camera the mode was pushed on
    camera::CameraMode* cameraMode = nullptr; // 0x1F4
    Entity* field_358 = nullptr;              // 0x358 entity using the actor (non-null while a reset is pending)
    uint32_t field_1FC = 0;                   // 0x1FC cleared when an interaction begins
    Entity* field_368 = nullptr;              // 0x368 counted, dropped when hidden
    int32_t userBones[3] = {-1, -1, -1};      // 0x35C bones 0xB3521F35..37 of the user
    uint32_t field_3C8 = 0;                   // 0x3C8 player block value (+0x10D8) of the user
    float savedFieldD0 = 0.0f;                // 0x3CC Entity::field_D0 while a user is held
    void* phantom = nullptr;                  // 0x354 Havok phantom (released with the actor)
    uint32_t tweenFlags = 0;                  // 0x3D4 bit 17 user damaged, bit 21, bit 29
    // Entities the actor spawned and owns: a pair per record (the second is
    // released first).
    struct OwnedPair {
        Entity* first;
        Entity* second;
    };
    // Lets go of an owned entity: off its parent object when its definition
    // says so (flag 21), released, and killed while still alive (0x001ED85C).
    void ReleaseOwned(Entity* entity);
    void ReleaseOwnedPair(const OwnedPair& pair);     // 0x001ED964
    void ReleaseAllOwned();                           // 0x001ED9D4
    void ReleaseEntity330();                          // 0x001EDA8C
    void ReleaseEntities33C();                        // 0x001EDB90
    void ReleaseEntities334();                        // 0x001EDC2C
    // Hides and kills the user's descendant 0x366FD481 (0x001EDCD4).
    void ReleaseUserProp();
    // Takes the actor's camera mode off its game camera (at once with
    // tweenFlags bit 20, else over Entity.TgtRange.CamPopTime) and deletes
    // it (0x001EE4D8).
    void ReleaseCamera();

    FloatTween tween_220;                     // 0x220
    ape::ApeTArray<OwnedPair> field_280;      // 0x280
    Entity* field_32C = nullptr;              // 0x32C an owned entity (also listed in field_280)
    Entity* field_330 = nullptr;              // 0x330
    Entity* field_334 = nullptr;              // 0x334
    Entity* field_338 = nullptr;              // 0x338
    Entity* field_33C[2] = {};                // 0x33C
    IntTween tween_36C;                       // 0x36C
    ape::ApeTArray<void*> field_3A0;          // 0x3A0
};

// Safe (vtable 0x00B18038, 0x270 bytes; destructor 0x00756CD4; the
// "SafeA_*" animations): a combination safe. The user turns its three dials
// one after another, each with a control drawn at random (left stick, right
// stick, tilt), to 0 within the definition's tolerance before the time runs
// out. The safe then opens (state 2) and spawns its reward.
class Safe : public ActorEntity {
public:
    // Random dial positions, the parts and sounds of the definition, closed,
    // and the instance's anim script started (0x001E5CD8).
    explicit Safe(const objects::SafeInstanceDesc& desc);
    ~Safe() override;
    bool Release(bool kill) override;                 // slot 10
    // Leaves the safe camera and shows the user again.
    void InteractionCancel() override;                // slot 63
    // Offers interaction kind 1 again.
    void InteractionEnd(bool completed) override;     // slot 64

    // The controls a dial can be turned with.
    enum Control : int32_t {
        kControl_LeftStick  = 0,
        kControl_RightStick = 1,
        kControl_Tilt       = 2,
    };
    // Back to state 0, closed, with a new combination; the marks and the HUD
    // go away. `failed` (the time ran out) also plays the fail sound and
    // lets the user go (0x001E5398).
    void Reset(bool failed);
    // Picks the definition's dial, highlight and solved objects out of the
    // tree under `object` (0x001E57E8).
    void FindParts(ape::world::ApeObj* object);
    // New random dial positions, shown at once (0x001E70D0).
    void Scramble();
    // The HUD prompt of the current control (0x001E77C8).
    void UpdatePrompt();
    // Turns dial `index` to `angle` degrees about +Z, keeping its position.
    void PoseDial(int index, float angle);

    ape::ApeTArray<int32_t> controlBag{3, nullptr, true};   // 0x1A8 controls not drawn yet
    ape::world::ApeObj* dials[3] = {};        // 0x1BC
    ape::world::ApeObj* highlights[3] = {};   // 0x1C8 shown while the dial is turned
    ape::world::ApeObj* solvedMarks[3] = {};  // 0x1D4 shown once the dial is solved
    ape::world::ApeObj* openObject = nullptr; // 0x1E0
    float dialAngle[3] = {};                  // 0x1E4 shown angle (eases to dialTarget)
    float dialTarget[3] = {};                 // 0x1F0 angle the input turned the dial to
    float dialStart[3] = {};                  // 0x1FC angle the dial was scrambled to
    float rewardOffset[3] = {};               // 0x210 placement value
    uint32_t rewardDefName = 0;               // 0x220 placement value
    float timeLeft = 0.0f;                    // 0x224
    ape::ApeAnimScriptInstance* script = nullptr;   // 0x228 owned
    int32_t activeDial = -1;                  // 0x22C dial being turned, -1 for none
    int32_t control = kControl_LeftStick;     // 0x230 Control of the active dial
    bool promptPending = false;               // 0x234 the prompt text must be set
    bool promptShown = false;                 // 0x235
    camera::CameraMode* cameraMode = nullptr; // 0x238
    hud::HudObject* timerHud = nullptr;       // 0x23C SafeDef::timerHudName
    hud::HudObject* promptHud = nullptr;      // 0x240 SafeDef::promptHudName
    hud::HudObject* tiltHud = nullptr;        // 0x244 HUD element 0x569F957F
    audio::SoundName* failSound = nullptr;    // 0x248
    audio::SoundName* openSound = nullptr;    // 0x24C
    audio::SoundName* tickSounds[3] = {};     // 0x250
    audio::SoundName* solvedSounds[3] = {};   // 0x25C

private:
    const SafeDef* Def() const { return static_cast<const SafeDef*>(def); }
};

} // namespace entity
} // namespace sly
