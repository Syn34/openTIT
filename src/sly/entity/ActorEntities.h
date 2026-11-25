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
namespace sly::camera { class CameraMode; }

namespace sly {
namespace entity {

// Actor definition base (vtable 0x00B12838, destructor 0x000E281C in the
// entity module; stand-in). Owns a table of 0xA0-byte part records, each
// holding a Havok object reference at +0x20.
class ActorDef : public EntityDef {
public:
    struct Part {
        uint8_t field_00[0x20];
        void* havokObject;          // +0x20
        uint8_t field_24[0x7C];
    };
    ~ActorDef() override;
    ape::ApeTArray<Part>* parts = nullptr;   // 0xC0 allocated on demand
    // Answers to InteractionNeeds by interaction kind. NOTE: the number of
    // kinds is not known (the table may run up to 0xF0).
    uint8_t interactionNeeds[16] = {};       // 0xC4
    // Damage taken per hit category (0 = immune). NOTE: filled by the
    // definition parser (not ported).
    float impactScale = 0.0f;                // 0xF0 category 1
    float blastScale = 0.0f;                 // 0xF8 category 2
    float collisionScale = 0.0f;             // 0x100 category 0, per unit of speed
    float collisionThreshold = 0.0f;         // 0x104 speed below which nothing happens
    float collisionMax = 0.0f;               // 0x108
    float meleeScale = 0.0f;                 // 0x10C category 3
    float projectileScale = 0.0f;            // 0x114 category 4
    float attackerScale[4] = {};             // 0x118 by the attacker's flag bits 22-23
};

// Definition of SoundActor and base of its subclasses' definitions
// (vtable 0x00B15970; constructor 0x001AD7F0).
class SoundActorDef : public ActorDef {
public:
    ~SoundActorDef() override;
    float durationScale = 1.0f;     // 0x188 applied with SoundActor::kSeq_Scaled
};

// Definition of InteractivePropA (vtable 0x00B17DB0, 0x280 bytes).
class InteractivePropADef : public ActorDef {
public:
    ~InteractivePropADef() override;
    uint8_t interactionFlags[6] = {};   // 0x170 per interaction kind (InteractivePropC)
    uint32_t bufferSize = 0;        // 0x254
    void* buffer = nullptr;         // 0x258 owned
};

// The definition InteractivePropA actually uses (vtable 0x00B17B88).
// NOTE: only the flag its overrides read is modelled.
class InteractivePropAUseDef : public ActorDef {
public:
    uint32_t sharedUse = 0;         // 0x130 non-zero: several users at once
};

// Safe definition (vtable 0x00B17FF0, 0x1E0 bytes; the safe entity code
// around 0x001E5398 plays "SafeA_*" animations).
class SafeDef : public ActorDef {
public:
    ~SafeDef() override;
};

// Actor base (0x00B12898 + 0x00B12A44/0x00B12A6C at +0x17C/+0x180).
class ActorEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Derived classes inline it.
    // @func 0x007476B0 ActorEntity::~ActorEntity
    // @func 0x00747764 ActorEntity::~ActorEntity (deleting)
    // @func 0x00747A88 ActorEntity::~ActorEntity (thunk, +0x17C)
    // @func 0x00747900 ActorEntity::~ActorEntity (deleting thunk, +0x17C)
    // @func 0x007479D0 ActorEntity::~ActorEntity (thunk, +0x180)
    // @func 0x00747830 ActorEntity::~ActorEntity (deleting thunk, +0x180)
    ~ActorEntity() override { delete field_184; }
    // Minimal versions of 0x000E4720 / 0x000E4544 / 0x000E7730 (entity module).
    bool Update(float dt) override { return ScriptedEntity::Update(dt); }
    bool PostUpdate(float dt) override { return ScriptedEntity::PostUpdate(dt); }
    bool Release(bool kill) override { return ScriptedEntity::Release(kill); }
    void SetActive(bool active) override;

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
    // Slots 98-101 and 103 (empty in the base; the actor classes override
    // some of them). NOTE: meanings unknown.
    // @func 0x00747C18 ActorEntity::ActorSlot98
    virtual void ActorSlot98() {}
    // @func 0x00747C20 ActorEntity::ActorSlot99
    virtual void ActorSlot99() {}
    // @func 0x00747C24 ActorEntity::ActorSlot100
    virtual void ActorSlot100() {}
    // @func 0x00747C2C ActorEntity::ActorSlot101
    virtual void ActorSlot101() {}
    // @func 0x00747CD0 ActorEntity::ActorSlot103
    virtual void ActorSlot103() {}

    // Filter consulted before projectile damage (+0x1A0); false ignores the hit.
    using HitFilter = bool (*)(ActorEntity* actor, EntityHitRecord& hit);

    ape::ApeTArray<void*>* field_184 = nullptr;   // 0x184 allocated on demand
    HitFilter projectileFilter = nullptr;         // 0x1A0
    uint32_t actorFlags = 0;                      // 0x1A4 bit 31: health ran out

private:
    const ActorDef* Def() const { return static_cast<const ActorDef*>(def); }
    // Applies `amount` through slot 88 and marks the hit fatal if it killed.
    void ApplyHitDamage(EntityHitRecord& hit, float amount, uint32_t type);
};

// Interactive prop that refuses player interaction while its definition says
// so (vtable 0x00B17BB8, 0x1B0 bytes; slot 76 0x001DC3A8).
class InteractivePropA : public ActorEntity {
public:
    ~InteractivePropA() override;
    bool Update(float dt) override;
    // A player may not use the prop while one user holds it and the player
    // is busy (player block +0x10DC positive, unless the driver is idle).
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    // Stops a playing scripted animation of the user.
    bool InteractionBegin(Entity* user) override;              // slot 62
};

// Definition of SearchlightActor (vtable 0x00B18230; stand-in).
class SearchlightDef : public ActorDef {
public:
    // @func 0x00757308 SearchlightDef::~SearchlightDef
    // @func 0x00757338 SearchlightDef::~SearchlightDef (deleting)
    ~SearchlightDef() override = default;
    uint8_t interactionNeeds[6] = {};   // 0x12E per interaction kind
    ape::Vec3 cameraOffset{};           // 0x140 camera position relative to the light
    ape::Vec4 field_150{};              // 0x150 NOTE: read by the spotlight camera constructor
};

// Player-steered searchlight (vtable 0x00B18260, 0x1D0 bytes); the spotlight
// camera mode rides on it.
class SearchlightActor : public ActorEntity {
public:
    ~SearchlightActor() override;
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

    const SearchlightDef* Def() const { return static_cast<const SearchlightDef*>(def); }

    camera::CameraMode* cameraMode = nullptr;   // 0x1A8 spotlight camera mode
    Entity* user = nullptr;                     // 0x1AC
    ape::world::ApeObj* lightObject = nullptr;  // 0x1B8
};

// Interactive prop driven by an anim script variable (vtable 0x00B18908,
// 0x240 bytes; slot 76 0x001F6808).
class InteractivePropB : public ActorEntity {
public:
    ~InteractivePropB() override;
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;
    bool Release(bool kill) override;                 // slot 10
    // Takes over the interacting entity's transform and starts state 1.
    bool InteractionBegin(Entity* user) override;                 // slot 62
    // NOTE: both only look the interaction component up.
    void InteractionCancel() override;                // slot 63
    void InteractionEnd(bool completed) override;     // slot 64

    ape::Matrix34 userTransform{};            // 0x1B0
    Entity* attached = nullptr;               // 0x1F0
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
    bool CanInteractWith(Entity* other, int kind) override;

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

    // Slot 105: plays sound slot `index` and switches to its state
    // (0x001AED20, not ported).
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
    void* field_218 = nullptr;                // 0x218 NOTE: its float +0x14 takes the slot duration
    int32_t currentSlot = -1;                 // 0x21C
    float savedTime = 0.0f;                   // 0x220
    int32_t savedSlot = 0;                    // 0x224
    uint32_t sequenceFlags = 0;               // 0x228 SequenceFlags
};

// vtable 0x00B16B48 (0x260 bytes).
class SoundActorB16B48 : public SoundActor {
public:
    ~SoundActorB16B48() override;
    // Slot 105: slot 1 also clears the lit flag and switches the light on.
    void PlaySlot(int index, bool force) override;
    bool AdvanceTo2(float elapsed, float duration) override;   // only while lit
    bool AdvanceTo3(float elapsed, float duration) override;
    bool AdvanceTo4(float elapsed, float duration) override;
    bool AdvanceWrap(float elapsed, float duration) override;
    // 0x001C3EF0 (not ported). NOTE: name from the flag it pairs with.
    void SetLight(bool on);

    ape::ApeTArray<void*> field_22C;          // 0x22C
    ape::ApeTArray<void*> field_240;          // 0x240
    uint8_t lightFlags = 0;                   // 0x25C bit 7 lit, bit 5 held
};

// Definition of SoundActorB16B48. NOTE: only the options its timers read.
class SoundActorB16B48Def : public SoundActorDef {
public:
    bool restartWhenWaiting = false;   // 0x1BC
    bool unlightOnSlot3 = false;       // 0x1BD
    bool unlightOnSlot4 = false;       // 0x1BE
};

// Sound actor with a two-way switch (vtable 0x00B16678; destructor
// 0x0075420C).
class SoundActorB16678 : public SoundActor {
public:
    // @func 0x0075420C SoundActorB16678::~SoundActorB16678
    // @func 0x0075424C SoundActorB16678::~SoundActorB16678 (deleting)
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
    // 0x001BF8E8 (not ported).
    void Refresh();

    int32_t position = 0;          // 0x22C 0 or 1 (indexes the state tables)
    float field_234 = 0.0f;        // 0x234 NOTE: non-negative resets the position
    float delay = 0.0f;            // 0x238 slot 5 waits while positive
    uint8_t switchFlags = 0;       // 0x23C bit 3 armed, bit 6 refresh pending
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
    bool IsOccupied() const;
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
    uint32_t options = 0;          // 0x1BC bit 28: close even when occupied
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

    Entity* attached = nullptr;    // 0x22C counted
    uint32_t attachedName = 0;     // 0x230
    uint8_t lookupFlags = 0;       // 0x238 bit 7: looked up
    bool holdWrap = false;         // 0x239
};

// vtable 0x00B17270 (0x360 bytes).
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
    bool CanInteractWith(Entity* other, int kind) override;   // slot 76
    // Projectiles from others strike the engaged actor (definition impact scale > 0).
    void OnHitCategory4(EntityHitRecord& hit) override;       // slot 87
    // Layer 1 in state 2 plays the struck state 0x803E1667 (unless
    // invulnerable); otherwise layer 1 goes to state 2 and layer 2 to 0.
    void OnStruck();

    Entity* linked = nullptr;                 // 0x2F0 released on kill
    float field_2FC = 0.0f;                   // 0x2FC copied into the cool-down by slot 4
    float coolDown = 0.0f;                    // 0x314 slots 3 and 6 wait while positive
    ape::ApeTArray<void*> field_32C;          // 0x32C emptied before it is freed
    bool field_355 = false;                   // 0x355 NOTE: selects the alternative states
    bool engaged = false;                     // 0x356
    uint8_t flags357 = 0;                     // 0x357 bit 7 set by slot 4, cleared by slot 5
};

// vtable 0x00B174C0 (0x3D0 bytes).
class SoundActorB174C0 : public SoundActor {
public:
    ~SoundActorB174C0() override;
    // The class carries its own copy of SoundActor::Release (0x001D20D0).
    bool Release(bool kill) override;
    ape::ApeTArray<void*> field_22C;          // 0x22C
};

// Actor holding a world object reference (vtable 0x00B18488, 0x200 bytes).
class ObjectActor : public ActorEntity {
public:
    ~ObjectActor() override;
    ape::world::ApeObj* object_1F4 = nullptr; // 0x1F4 counted reference
};

// Actor with two tweens and two arrays (vtable 0x00B186C8).
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
    void Reset(bool flag);
    void HandleMessage(const EntityMessage& msg) override;   // slot 43
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;   // slot 22
    bool InteractionBegin(Entity* user) override;                 // slot 62
    void InteractionCancel() override;                // slot 63: Reset(false)
    void InteractionEnd(bool completed) override;     // slot 64: Reset(completed)
    // 0x001EDD84 (not ported): takes the interacting entity.
    void TakeUser(Entity* user);

    Entity* field_358 = nullptr;              // 0x358 entity using the actor (non-null while a reset is pending)
    uint32_t field_1FC = 0;                   // 0x1FC cleared when an interaction begins
    Entity* field_368 = nullptr;              // 0x368 counted, dropped when hidden
    uint32_t field_3C8 = 0;                   // 0x3C8 player block value (+0x10D8) of the user
    uint32_t tweenFlags = 0;                  // 0x3D4 bit 17 user damaged, bit 21, bit 29
    FloatTween tween_220;                     // 0x220
    ape::ApeTArray<void*> field_280;          // 0x280
    IntTween tween_36C;                       // 0x36C
    ape::ApeTArray<void*> field_3A0;          // 0x3A0
};

// Safe (vtable 0x00B18038; destructor 0x00756A1C outside this range; the
// "SafeA_*" animations): the player cracks it in a camera of its own.
class Safe : public ActorEntity {
public:
    bool Release(bool kill) override;                 // slot 10
    // Leaves the safe camera and shows the user again.
    void InteractionCancel() override;                // slot 63
    // Offers interaction kind 1 again.
    void InteractionEnd(bool completed) override;     // slot 64

    void* script228 = nullptr;                // 0x228 owned anim script instance
    camera::CameraMode* cameraMode = nullptr; // 0x238
    audio::SoundName* sound248 = nullptr;     // 0x248
    audio::SoundName* sound24C = nullptr;     // 0x24C
    audio::SoundName* sounds250[3] = {};      // 0x250
    audio::SoundName* sounds25C[3] = {};      // 0x25C
};

} // namespace entity
} // namespace sly
