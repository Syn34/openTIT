// openTIT - The physical entity base and its definition base.
//
// Projectiles and other free-moving objects derive from a physical entity
// (console vtable 0x00B12CD8 + 0x00B12E78/0x00B12EA0, code 0x000E9BA0-
// 0x000EE000) that owns a Havok rigid body, optionally a second body on the
// trigger layer and two Havok actions, and can carry another entity spawned
// from its definition. Its definition (vtable 0x00B12C88, destructor
// 0x000E9820) owns the collision shape and the damage scales.
//
// The constructor (0x000E9DD8) and the definition's loader/spawn
// (0x000E99B0, 0x000EAC88) need the entity constructor family of the entity
// module and are not ported; the per-frame update (0x000EC5B8) neither.
#pragma once

#include "sly/entity/Entity.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/entity/EntityInterfaces.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace components { class OffscreenMarkerComponent; }
namespace entity {

class PhysicalEntityDef : public EntityDef {
public:
    ~PhysicalEntityDef() override;

    // Fall acceleration of the entity's body: the physics world's gravity
    // scaled by the definition, nothing without a collision shape.
    float FallAcceleration() const;

    // Definition flags (+0x174).
    enum Flags : uint32_t {
        kDefFlag_NoKind2Interaction = 1u << 9,
        kDefFlag_DieOnHit           = 1u << 10,   // NOTE: name from use (SweepTo)
        kDefFlag_FaceMotion         = 1u << 22,   // NOTE: name from use (SweepTo)
        kDefFlag_NoInteraction      = 1u << 15,
    };

    float gravityScale = 0.0f;               // 0xC0
    float rollRate = 0.0f;                   // 0x100 radians per second about the forward axis (flag 22)
    float launchValue = 0.0f;                // 0x12C handed to the anim script on launch. NOTE: meaning unknown
    // Damage taken per hit category (0 = immune). NOTE: filled by the
    // definition parser (not ported).
    float impactScale = 0.0f;                // 0xCC category 1
    float blastScale = 0.0f;                 // 0xD0 category 2
    float meleeScale = 0.0f;                 // 0xD4 category 3
    float projectileScale = 0.0f;            // 0xD8 category 4
    uint32_t impactSoundName = 0;            // 0x13C
    uint32_t carriedDefName = 0;             // 0x144 definition of the carried entity, 0 for none
    uint32_t defFlags = 0;                   // 0x174 Flags
    void* collisionShape = nullptr;          // 0x190
};

class PhysicalEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // Physics states (+0x190). Asking for kPhys_Enter puts the body into the
    // world and lands in kPhys_InWorld; kPhys_Removed takes the bodies out.
    // NOTE: 3-5 are only recorded here; their names follow the callers
    // (4 is entered when the entity runs out of health).
    enum PhysState : int32_t {
        kPhys_None    = 0,
        kPhys_Enter   = 1,
        kPhys_InWorld = 2,
        kPhys_Dead    = 4,
        kPhys_Removed = 6,
    };

    ~PhysicalEntity() override;
    // Minimal version of 0x000EC5B8 (not ported).
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;                                   // slot 10
    void SetTransform(const ape::Matrix34& m) override;                 // slot 21
    void SetVelocity(const ape::Vec3& velocity) override;              // slot 34
    ape::Vec3 GetVelocity() override;                                   // slot 35
    void HandleMessage(const EntityMessage& msg) override;              // slot 43
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;            // slot 76
    void OnHitCategory1(EntityHitRecord& hit) override;                 // slot 84
    void OnHitCategory2(EntityHitRecord& hit) override;                 // slot 85
    void OnHitCategory3(EntityHitRecord& hit) override;                 // slot 86
    void OnHitCategory4(EntityHitRecord& hit) override;                 // slot 87
    bool Damage(float amount, Entity* source, uint32_t type) override;  // slot 88
    float ProjectileDamageScale() const override;                       // slot 89

    // Slot 36: the rigid body (see NOTES on slot 36).
    ape::world::ApeObj* GetRenderObject() override;
    // Physics impacts are handled by the contact callbacks instead.
    void OnHitCategory0(EntityHitRecord& hit) override;                 // slot 83
    // Empty overrides of the +0x17C interface's slots 7/6 (primary slots 97/98).
    void Interface17CSlot7() override;
    void Interface17CSlot6() override;
    // Slot 94: aims at `target` (its aim point, the origin without one): the
    // aim frame (carriedTransform) moves there, with the carried entity.
    virtual void AimAt(Entity* target);
    // Slot 95: the entity was launched: works out the flight time to the aim
    // point and hands the aim frame, whether there is a target and the
    // definition's launch value to the anim script, which enters "Start".
    virtual void OnLaunched();
    // Seconds until the entity reaches the aim point: over the ground at the
    // current horizontal speed, else (straight up or down) along the fall
    // under the body's gravity (0x000EF3C8).
    void UpdateFlightTime();
    // Frame for moving from `from` to `to`: the body is swept through the
    // world when it is in one (definitions with flag 10 die when it hits
    // something); with `orient` and definition flag 22 the forward axis is
    // eased towards the direction of motion (Entity.Projectile.OrientDampening)
    // (0x000EB420).
    void SweepTo(const ape::Matrix34& from, const ape::Vec3& to, ape::Matrix34& out, bool orient);
    // Slot 101: takes the body's frame over: as it is, or (definition flag
    // 22) facing along the body's velocity and rolled by the definition's
    // roll rate.
    virtual void UpdateFromBody(float dt);
    // Slot 99: empty. NOTE: no caller known.
    virtual void Slot99();
    // Primary slot 100 / +0x180 slot 6: stops listening to the body's contacts.
    void Interface180Slot6(void* body) override;

    void SetPhysState(int state, uint32_t arg = 0);
    // The definition rules out every interaction (0x000EFFC8).
    bool InteractionDisabled() const;
    // Slot 96 / contact listener slot 0: a contact point of the main body or
    // the trigger body, while in the world; otherwise contacts are disabled.
    void OnStep(const void* event) override;
    // Main body contact (0x000F0E48, not ported) and trigger body contact:
    // two physical entities touching with their trigger bodies destroy each
    // other. False when the event is not about `ownBody`.
    bool HandleBodyContact(const void* event, void* ownBody);
    bool HandleTriggerContact(const void* event, void* ownBody);
    // Entity the object is aimed at (counted reference, shared with the
    // offscreen marker when there is one).
    void SetTarget(Entity* target);
    // Where the carried entity sits; it is moved there at once.
    void SetCarriedTransform(const ape::Matrix34& m);

    const PhysicalEntityDef* Def() const { return static_cast<const PhysicalEntityDef*>(def); }

    void* body = nullptr;                    // 0x184 Havok rigid body
    void* action = nullptr;                  // 0x188 Havok actions added with the body
    void* action2 = nullptr;                 // 0x18C
    int32_t physState = kPhys_None;          // 0x190 PhysState
    // NOTE: entities with type bit 13 keep the one that threw or fired them
    // here (the figures and targets compare it with themselves).
    Entity* owner = nullptr;                 // 0x1B0 counted reference
    Entity* target = nullptr;                // 0x1B4 counted reference
    Entity* carried = nullptr;               // 0x1B8 spawned from the definition
    // 0x1C0 aim frame: identity at the aim point (slot 94); the carried
    // entity (e.g. a marker) sits there.
    ape::Matrix34 carriedTransform{};
    ape::Vec3 launchPoint{};                 // 0x200 where the entity was launched from
    void* triggerBody = nullptr;             // 0x224 second body, on collision layer 7
    float flightTime = 0.0f;                 // 0x228 seconds to the aim point (UpdateFlightTime)
    audio::SoundName* impactSound = nullptr; // 0x230 NOTE: name from use
    components::OffscreenMarkerComponent* marker = nullptr;  // 0x26C first component of type 0x1E
};

// The physical entity PhysicalEntityDef itself spawns (vtable 0x00B12EC8 +
// 0x00B13068/0x00B13090; built by the definition's Spawn 0x000EAC88): adds
// nothing to the base.
class PlainPhysicalEntity : public PhysicalEntity {
public:
    ~PlainPhysicalEntity() override;
};

} // namespace entity
} // namespace sly
