// openTIT - Vehicle entities: the definition and entity base shared by the
// player vehicles (0x00226A28-0x00229CC4). The three vehicle kinds (vtables
// 0x00B1B740, 0x00B1B9E8, 0x00B1BC60) derive from this base and live at
// 0x00229CC4-0x0024EEDC; they are not ported yet.
#pragma once

#include "sly/entity/ScriptedEntity.h"

#include <cstdint>

namespace sly {
namespace objects {

// Definition of a vehicle (vtable 0x00B1B4C8). It is built from a 0x780-byte
// parse description (parser 0x00226FA8, constructor 0x00226A28, neither
// ported); only the fields the entity base reads are modelled, with their
// offsets.
class VehicleDef : public entity::ScriptedEntityDef {
public:
    ~VehicleDef() override;
    // Slot 2: the display text from the XML, else the definition's name.
    const char* GetDisplayName() const override;

    // Damage taken per hit category (see VehicleEntity::OnHitCategory*).
    float collisionScale = 0.0f;      // 0xC8 per unit of speed above the threshold
    float collisionMax = 0.0f;        // 0xCC
    float collisionThreshold = 0.0f;  // 0xD0
    float impactScale = 0.0f;         // 0xD4
    float blastScale = 0.0f;          // 0xD8
    float meleeScale = 0.0f;          // 0xDC
    float projectileScale = 0.0f;     // 0xE0 (also slot 89 of the entity)
    float meleeScaleSpecial = 0.0f;   // 0xE4 melee hits of reaction 0x04E8CCF7
    ape::Vec3 aimOffset{};            // 0x2C0 aim point in the vehicle's frame
    uint32_t destroyAnim = 0;         // 0x2D0 played when the vehicle is destroyed
    float hitInvulnerableTime = 0.0f; // 0x2D4 after a hit that did not destroy it
    uint32_t displayNameSize = 0;     // 0x2DC
    char* displayName = nullptr;      // 0x2E0 owned, displayNameSize bytes
    uint32_t team = 0;                // 0x2E4 bits 0-1 go to entity flag bits 22-23
    uint32_t defFlags = 0;            // 0x2E8 bit 31: entity class bit 5
};

// Vehicle entity base (vtable 0x00B1B500): a scripted entity with damage
// scaled per hit category, a short invulnerability after each hit and a
// destruction sequence driven by layer 0/1 states:
//   layer 0: 0 alive, 1 destroyed; layer 1 (while destroyed): 0 starting,
//   1 destruction animation playing, 3 done.
class VehicleEntity : public entity::ScriptedEntity {
public:
    enum VehicleFlags : uint32_t {
        kVehicle_ShowHelp = 0x20000000u,       // show the controls help text (NOTE: name from the help text state machine)
        kVehicle_FlashPending = 0x40000000u,   // play the hit reaction on the next update
        kVehicle_Enabled = 0x80000000u,        // input and physics run (NOTE: name from how the kinds use it)
    };

    // 0x00228AC0
    explicit VehicleEntity(VehicleDef* vehicleDef);
    ~VehicleEntity() override;

    bool Update(float dt) override;                                     // slot 4
    bool PostUpdate(float dt) override;                                 // slot 5
    bool Release(bool kill) override;                                   // slot 10
    ape::Vec3 GetLocatorPosition(const ape::Matrix34* locator) override; // slot 39
    uint32_t InteractionOffer(int kind, entity::Entity* other) override; // slot 74
    uint32_t InteractionNeeds(int kind, entity::Entity* other) override; // slot 75
    bool CanInteractWith(entity::Entity* other, int kind, float* priority = nullptr) override; // slot 76
    void OnHitCategory0(entity::EntityHitRecord& hit) override;          // slots 83-87
    void OnHitCategory1(entity::EntityHitRecord& hit) override;
    void OnHitCategory2(entity::EntityHitRecord& hit) override;
    void OnHitCategory3(entity::EntityHitRecord& hit) override;
    void OnHitCategory4(entity::EntityHitRecord& hit) override;
    bool Damage(float amount, entity::Entity* source, uint32_t type) override;   // slot 88
    float ProjectileDamageScale() const override;                       // slot 89
    // Slots 94-99: the physics body interface the vehicle kinds implement
    // (names from the kinds' overrides).
    // Slot 94: centre of mass; the local position of the world object here.
    virtual ape::Vec3 GetCenterOfMass();
    // Slot 95: angular velocity (zero here).
    virtual ape::Vec3 GetAngularVelocity();
    // Slot 96: sets the angular velocity (nothing here).
    virtual void SetAngularVelocity(const ape::Vec3& velocity);
    // Slot 97: applies an impulse at a world point (nothing here).
    virtual void ApplyPointImpulse(const ape::Vec3& impulse, const ape::Vec3& point);
    // Slot 98: mass of the body (zero here).
    virtual float GetMass();
    // Slot 99: sets or clears kVehicle_Enabled (the kinds also stop the
    // body when the state changes).
    virtual void SetEnabled(bool on);
    // Slot 100 (+0x190): the destruction starts.
    virtual void OnDestroyed();
    // Slot 101 (+0x194): reaction to a hit that did not destroy the vehicle.
    virtual void OnHitReaction();

    // Player-team vehicle: class bit 5 with team 1.
    bool IsPlayerTeamVehicle() const;

    const VehicleDef* Def() const { return static_cast<const VehicleDef*>(def); }

    float invulnerableTimer = 0.0f;   // 0x17C
    uint32_t vehicleFlags = 0;        // 0x180 VehicleFlags

protected:
    // The checks every hit category makes before any damage is applied.
    bool CanTakeHit() const;
    void ApplyHitDamage(entity::EntityHitRecord& hit, float amount, uint32_t type);
};

} // namespace objects
} // namespace sly
