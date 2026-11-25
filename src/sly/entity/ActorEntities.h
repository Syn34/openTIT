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
    uint32_t bufferSize = 0;        // 0x254
    void* buffer = nullptr;         // 0x258 owned
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
    // The console's out-of-line destructor is 0x007476B0 (not in this range);
    // derived classes inline it.
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
    virtual bool CanBeDamaged() const;
    // Slot 89: damage scale of projectiles.
    virtual float ProjectileDamageScale() const;

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
};

// Definition of SearchlightActor (vtable 0x00B18230; destructor outside this
// range, stand-in).
class SearchlightDef : public ActorDef {
public:
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

    const SearchlightDef* Def() const { return static_cast<const SearchlightDef*>(def); }
};

// Interactive prop driven by an anim script variable (vtable 0x00B18908,
// 0x240 bytes; slot 76 0x001F6808).
class InteractivePropB : public ActorEntity {
public:
    ~InteractivePropB() override;
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;
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
    ape::ApeTArray<void*> field_22C;          // 0x22C
    ape::ApeTArray<void*> field_240;          // 0x240
};

// vtable 0x00B17270 (0x360 bytes).
class SoundActorB17270 : public SoundActor {
public:
    ~SoundActorB17270() override;
    // Offers interaction kind 2 (bit value 4).
    // @func 0x001CEC84 SoundActorB17270::InteractionOffer
    uint32_t InteractionOffer(int kind, Entity* other) override { (void)kind; (void)other; return 4; }
    ape::ApeTArray<void*> field_32C;          // 0x32C emptied before it is freed
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

    uint32_t field_358 = 0;                   // 0x358 NOTE: non-zero while a reset is pending
    FloatTween tween_220;                     // 0x220
    ape::ApeTArray<void*> field_280;          // 0x280
    IntTween tween_36C;                       // 0x36C
    ape::ApeTArray<void*> field_3A0;          // 0x3A0
};

} // namespace entity
} // namespace sly
