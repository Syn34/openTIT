// openTIT - "HoverVehicle" (0x0023E3E0-0x00244900): a vehicle that floats over
// the ground on one physics body. The player steers it with the stick or the
// d-pad (or a script queues input directions), can fire a turbo boost that
// drains the player's power meter, and can be kept near a point by a script
// "leash".
//
// Console classes: definition vtable 0x00B1B9B8 (0x410 bytes), entity vtable
// 0x00B1B9E8 (0x280 bytes) with the contact listener (+0x184, vtable
// 0x00B1BBA0) and entity listener (+0x188, 0x00B1BBC8) interfaces. The
// definition element is "HoverVehicle".
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/objects/Vehicle.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }
namespace ape::xml { class XmlElement; }

namespace sly {
namespace objects {

struct PhysicsBody;

// The hover-specific settings (definition +0x310..+0x401; the description
// the parser fills holds the same block 0x40 bytes lower). Angles are stored
// in radians. Attribute names in quotes are recovered, the others keep their
// hash.
struct HoverVehicleSettings {
    float mass = 10.0f;                 // 0x310 "Mass"
    float angularDamping = 0.05f;       // 0x314 "AngularDamping"
    float deadZone = 0.0f;              // 0x318 "deadZone" (stick), 0..1
    float dpadScaleY = 1.0f;            // 0x31C attribute 0x824119DC (d-pad up/down), 0..1
    float dpadScaleX = 1.0f;            // 0x320 attribute 0x2CA6B8AF (d-pad left/right), 0..1
    float stickScaleY = 1.0f;           // 0x324 attribute 0x92AC6682, 0..1
    float stickScaleX = 1.0f;           // 0x328 attribute 0x24EC3F49, 0..1
    float field_32C = 5.0f;             // 0x32C attribute 0x6C3A7CD2, >= 0

    // "Movement" (element 0x48D1D90E).
    uint32_t movementType = 2;          // 0x330 "Type": index into kMovementTypes (2 = none given)
    float minSpeed = 1.0f;              // 0x334 "MinSpeed" (>= 0)
    float maxSpeed = 2.0f;              // 0x338 "MaxSpeed" (>= MinSpeed); also the scripted-input speed
    float maxAccel = 5.0f;              // 0x33C "MaxAccel"
    float maxDecel = 5.0f;              // 0x340 "maxdecel" (>= 0.1)
    float field_344 = 1080.0f;          // 0x344 attribute 0x8E0994A0 (>= 0)
    float field_348 = 1080.0f;          // 0x348 attribute 0x332715B6 (>= field_344)
    float field_34C = 180.0f;           // 0x34C attribute 0xBD488CBD (0..180)
    float field_350 = 1.0f;             // 0x350 attribute 0x7694E8F2 (0..1)
    float maxRotAccel = 360.0f;         // 0x354 "MaxRotAccel" (>= 0)
    float maxRotDecel = 360.0f;         // 0x358 "MaxRotDecel" (>= 0)

    // "Roll" and "Pitch" elements (same attributes).
    struct Tilt {
        float maxAngle;                 // "maxAngle"
        float angleScale;               // "angleScale"
        float maxRate;                  // "MaxRate"
        float maxDecel;                 // "maxdecel"
        float maxAccel;                 // "MaxAccel"
    };
    Tilt roll{kPi, 1.0f, -kPi, -kPi, -kPi};    // 0x35C
    Tilt pitch{kPi, 1.0f, -kPi, -kPi, -kPi};   // 0x370

    // "TurboBoost".
    float turboMinSpeed = 1.0f;         // 0x384 "MinSpeed" (>= 0)
    float turboMaxSpeed = 2.0f;         // 0x388 "MaxSpeed" (>= MinSpeed)
    float turboMaxDecel = -1.0f;        // 0x38C "maxdecel"
    float turboMaxAccel = -1.0f;        // 0x390 "MaxAccel"
    float turboField394 = -1.0f;        // 0x394 attribute 0x7694E8F2 (0..1)
    float turboStartCost = 0.0f;        // 0x398 attribute 0xBEDAEF09: power taken when the boost starts
    float turboCostRate = 0.0f;         // 0x39C "PowerCostRate" (hash match): power per second
    float turboField3A0 = 15.0f;        // 0x3A0 attribute 0xA2F8435A
    float turboMinTime = 0.0f;          // 0x3A4 "MinTime": a boost lasts at least this long
    float turboMaxTime = -1.0f;         // 0x3A8 "MaxTime": and at most this long (<= 0: no limit)
    float turboMinPower = 0.0f;         // 0x3AC attribute 0x6F625C15: power needed to arm the boost
    uint32_t turboScript = 0;           // 0x3B0 "Script": effects script (states StartOn/StartOff/EndOn/EndOff)

    // Element 0x94174CE7.
    float cosMinAngle = 0.0f;           // 0x3B4 cos("minAngle")
    float cosMaxAngle = 1.0f;           // 0x3B8 cos("maxAngle")
    float minAngleScale = 0.0f;         // 0x3BC "minAngleScale" (hash match)
    float maxAngleScale = 1.0f;         // 0x3C0 "maxAngleScale" (hash match)

    // "AltMovement" (hash match).
    float altMaxAccel[2] = {65.0f, 65.0f};    // 0x3C4 "MaxAccel"
    float altMaxDecel = 15.0f;          // 0x3CC "maxdecel"
    float altMinSpeed = 10.0f;          // 0x3D0 "MinSpeed"
    float altMaxSpeed = 25.0f;          // 0x3D4 "MaxSpeed"
    float altMaxSpeedTurbo = 45.0f;     // 0x3D8 "MaxSpeedTurbo"
    float altMaxAccelTurbo[2] = {50.0f, 50.0f};   // 0x3DC "MaxAccelTurbo"
    float altField3E4 = 200.0f;         // 0x3E4 attribute 0xF7740920
    float altField3E8 = 8.0f;           // 0x3E8 attribute 0x7F1D654B (1..20)

    ape::Vec3 field_3F0{};              // 0x3F0 attribute 0x0F05FBBF
    bool rejectCollisions = false;      // 0x400 "REJECTCOLLISIONS" (hash match): ignore contacts with the world
    bool flightEnabled = true;          // 0x401 attribute 0x88CBF571: run the flight model

    static constexpr float kPi = 3.14159274f;
    // Hashes of the "Type" values of the Movement element (0x00AD3C0C).
    static const uint32_t kMovementTypes[2];

    // Reads the hover attributes and child elements of the definition element
    // (the tail of the parser 0x0023E590).
    void Parse(ape::xml::XmlElement* element);
};

// Definition (vtable 0x00B1B9B8).
class HoverVehicleDef : public VehicleDef {
public:
    ~HoverVehicleDef() override;
    entity::Entity* Spawn(entity::EntityPlacement* placement) override;   // slot 7

    // Clones the template body named bodyName from the level's physics world
    // the first time (0x00240AC0).
    void ResolveBodyTemplate();

    char bodyName[0x20] = {};           // 0x2E9 "colDefName"
    PhysicsBody* bodyTemplate = nullptr;   // 0x30C
    HoverVehicleSettings settings;      // 0x310
};

class HoverVehicle : public VehicleEntity, public entity::EntityInterface17C, public entity::EntityInterface180 {
public:
    // Script states of the turbo effects script.
    static constexpr uint32_t kTurboState_StartOn = 0xB9B6CDA1u;
    static constexpr uint32_t kTurboState_StartOff = 0x7D22E5CDu;
    static constexpr uint32_t kTurboState_EndOn = 0x03F0D3DAu;
    static constexpr uint32_t kTurboState_EndOff = 0x7A29A6B4u;

    // 0x002404F0
    HoverVehicle(HoverVehicleDef* def, const entity::EntityPlacement& placement);
    ~HoverVehicle() override;

    bool Update(float dt) override;                       // slot 4
    bool PostUpdate(float dt) override;                   // slot 5
    bool Release(bool kill) override;                     // slot 10
    void ResetState() override;                           // slot 11
    void SetTransform(const ape::Matrix34& m) override;   // slot 21
    void SetVelocity(const ape::Vec3& velocity) override; // slot 34
    ape::Vec3 GetVelocity() override;                     // slot 35
    // Slot 36: the physics body. NOTE: the slot is declared as the render
    // object in the entity base; vehicles return their body through it.
    ape::world::ApeObj* GetRenderObject() override;
    void SetActive(bool active) override;                 // slot 44
    void Slot67() override;                               // slot 67
    void* Slot70(uint32_t nameHash) override;             // slot 70: FindBody
    ape::Vec3 GetUpVector() override;                     // slot 72
    ape::Vec3 GetCenterOfMass() override;                 // slot 94
    ape::Vec3 GetAngularVelocity() override;              // slot 95
    void SetAngularVelocity(const ape::Vec3& velocity) override;   // slot 96
    void ApplyPointImpulse(const ape::Vec3& impulse, const ape::Vec3& point) override;   // slot 97
    float GetMass() override;                             // slot 98
    void SetEnabled(bool on) override;                    // slot 99
    void OnDestroyed() override;                          // slot 100
    // Slot 102 and contact listener slot 0: a contact point of the body.
    virtual void OnContact(const void* event);
    void OnStep(const void* event) override { OnContact(event); }
    // Slots 103-106: nothing (NOTE: meanings unknown; no caller seen); slot
    // 107 stops listening to a body's contacts.
    // @func 0x0075DAAC HoverVehicle::Slot103
    virtual void Slot103() {}
    // @func 0x0075DAB4 HoverVehicle::Slot104
    virtual void Slot104() {}
    // @func 0x0075DAB8 HoverVehicle::Slot105
    virtual void Slot105() {}
    // @func 0x0075DAC0 HoverVehicle::Slot106
    virtual void Slot106() {}
    virtual void StopContactListening(PhysicsBody* body);

    // The body when `nameHash` is the hash of the definition's body name
    // (the console's slot 70 takes the hash).
    PhysicsBody* FindBody(uint32_t nameHash) const;

    // Queues an input direction from a script (0x00244810); the queued
    // directions replace the pad for the next flight update.
    void QueueScriptedInput(const ape::Vec3& direction);
    // Script leash (0x00244900 / 0x002448C0): while enabled the vehicle is
    // steered back toward `point` once it strays further than the radius
    // (squared) for `duration` seconds.
    void SetLeash(float radiusSquared, float duration, const ape::Vec3& point);
    void SetLeashEnabled(bool enabled);

    const HoverVehicleDef* HoverDef() const { return static_cast<const HoverVehicleDef*>(def); }

    PhysicsBody* body = nullptr;                // 0x18C
    ape::Vec3 upVector{};                       // 0x190 (slot 72) NOTE: written by the flight model
    uint32_t skinValue = 0;                     // 0x1A0 from the spawn parameters, handed to the skin
    ape::ApeAnimScriptInstance* field_1A8 = nullptr;   // 0x1A8 NOTE: released (orphaned) with the entity
    uint32_t field_1AC = 0;                     // 0x1AC
    uint32_t field_1B0 = 0;                     // 0x1B0
    ape::Vec3 moveDirection{};                  // 0x1C0 desired direction on the ground plane
    float throttle = 0.0f;                      // 0x1D0
    ape::Vec3 cachedVelocity{};                 // 0x1E0 linear velocity at the last update
    ape::Vec3 cachedAngularVelocity{};          // 0x1F0
    float field_200[4] = {};                    // 0x200 NOTE: cleared with the velocities
    ape::ApeTArray<ape::Vec3> scriptedInputs{16};   // 0x210
    ape::ApeAnimScriptInstance* turboScript = nullptr;   // 0x224
    uint32_t turboScriptState = 0;              // 0x228
    float turboActive = 0.0f;                   // 0x22C 1 while boosting
    float turboTime = 0.0f;                     // 0x230 how long the boost has run
    bool turboArmed = true;                     // 0x234 bit 31: the boost may start
    float turboBlend = 0.0f;                    // 0x238 1 while boosting, eases back to 0
    ape::Vec3 leashPoint{};                     // 0x240
    float leashRadiusSquared = 0.0f;            // 0x250
    float leashDuration = 0.0f;                 // 0x254
    float leashTimer = 0.0f;                    // 0x258
    bool leashEnabled = false;                  // 0x25C
    entity::EntityComponent* skin = nullptr;    // 0x260
    entity::EntityComponent* controller = nullptr;      // 0x264 the one driving the vehicle
    entity::EntityComponent* padController = nullptr;   // 0x268 "TmpControl"
    entity::EntityComponent* extController = nullptr;   // 0x26C "TmpExtControl"
    uint32_t scriptState = 0;                   // 0x270 state of the entity's own script
    bool explodePending = false;                // 0x274 bit 7: a contact destroyed the vehicle

private:
    void CreateControllers();                   // 0x0023FFD8
    void StartTurboScript();                    // 0x00240288
    // Pad stick/d-pad reading of the flight model (0x002411B0).
    void ReadSteerInput(float out[3]) const;
    // Desired direction for this frame (0x00241500). TODO: only the
    // scripted-input path is ported (the pad path has bad instruction data).
    float UpdateMoveDirection();
    void UpdateFlight(float dt);                // TODO(0x002418B0)
    void UpdateTurbo(float dt);                 // 0x002428E0
    void UpdateSkinTarget();                    // 0x00243728
    void SyncFromBody();                        // 0x0024356C
    void CheckExplode();                        // 0x00242DF8
    void SetTurboScriptState(uint32_t state);
};

} // namespace objects
} // namespace sly
