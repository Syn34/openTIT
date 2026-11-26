// openTIT - "SimpleWheeledVehicle" (0x00244980-0x0024EEDC): the arcade car
// the player drives in a few jobs. One physics body with a sphere/box
// compound shape, four visual wheels, a phantom sphere that probes ahead, an
// optional gun (the skin component), a turbo that drains the power meter, a
// horn, a costume swap to another vehicle definition, and a small state
// machine that shows the controls help text.
//
// Console classes: definition vtable 0x00B1BC00 (0x410 bytes), entity vtable
// 0x00B1BC60 (0x5A0 bytes) with the contact listener interface (+0x184,
// vtable 0x00B1BE04) and the help text state machine member (+0x570,
// vtable 0x00B1BC30). The definition element is "SimpleWheeledVehicle".
#pragma once

#include "sly/gameplay/StateMachine.h"
#include "sly/objects/Vehicle.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; class ApeAnimScriptState; }
namespace sly { namespace hud { class HudObject; } }

namespace sly {
namespace objects {

struct PhysicsBody;
class SimpleWheeledVehicle;

inline constexpr ape::Matrix34 kVehicleIdentity{
    {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};

// The vehicle settings (definition +0x310..+0x40C, copied from the parse
// description where the same block sits 0x40 bytes lower). Attribute names
// in quotes are recovered (some from hash pairs); the others keep their hash.
struct SimpleWheeledVehicleSettings {
    float mass = 10.0f;                 // 0x310 "Mass" (>= 0)
    float field_314[3] = {};            // 0x314 NOTE: not copied by the constructor
    ape::Vec3 colSize{};                // 0x320 "ColSize": box size of the body
    float field_330 = 0.2f;             // 0x330 attribute 0x08DE1355
    float field_334 = 0.1f;             // 0x334 attribute 0xFA3B9D65
    float wheelValues[4] = {};          // 0x338 attributes 0x3FE174A3, 0xBCA380E0, 0xF8352638, 0x0EC601EB
    uint32_t wheelNames[4] = {};        // 0x348 attributes 0x507A8FF1, 0xBF2DCF52, 0x1184692A, 0x1F5F1D39
    uint32_t wheelNames2[4] = {};       // 0x358 attributes 0x4ACDEE8F, 0x0F464474, 0x2A1BBECC, 0x19B27BD7
    uint32_t turboScript = 0;           // 0x368 "TurboScript"
    uint32_t hornScript = 0;            // 0x36C "HornScript"
    uint32_t gunScript = 0;             // 0x370 attribute 0xAEFB8B49 (states "On"/"End")
    float gravityScaler = 1.0f;         // 0x374 "GravityScaler"
    float maxSpeed = 6.7f;              // 0x378 "MaxSpeed"
    float field_37C = 0.3f;             // 0x37C attribute 0x34FDB56B
    float maxAcceleration = 10.0f;      // 0x380 "maxAcceleration"
    float turboMaxSpeed = 16.0f;        // 0x384 "TurboMaxSpeed"
    float field_388 = 0.5f;             // 0x388 attribute 0xB2858F69
    float turboMaxAcceleration = 20.0f; // 0x38C "TurboMaxAcceleration"
    bool turboEnabled = false;          // 0x390 "TurboEnabled"
    float handling[10] = {};            // 0x394 attributes 0xF477B0AC .. 0xE0C6D2DD
    float turboPowerThreshold = 0.0f;   // 0x3BC "TurboPowerThreshold"
    float field_3C0[5] = {};            // 0x3C0 attributes 0x834837A3 .. 0x0FCF303D
    float field_3D4[3] = {};            // 0x3D4 NOTE: not copied by the constructor
    ape::Vec3 field_3E0{};              // 0x3E0 attribute 0x118A5989
    float probeMinSpeed = -1.0f;        // 0x3F0 attribute 0xF62C2C29: probe ahead above this speed
    float probeMinSpeed2 = -1.0f;       // 0x3F4 attribute 0x45E5E8A8
    bool probeReversing = true;         // 0x3F8 attribute 0x1F25286D: probe when backing up too
    uint32_t hudElement = 0;            // 0x3FC attribute 0x6350BDB4: HUD element shown while driving
    float field_400 = 0.0f;             // 0x400 attribute 0xC4EC7385
    uint32_t disguiseDef = 0;           // 0x404 "DisguiseDef" (hash match): definition the costume button swaps to
    uint32_t field_408 = 0;             // 0x408 attribute 0xD5494862
    bool autoCollect = true;            // 0x40C "AutoCollect"
};

// Definition (vtable 0x00B1BC00).
class SimpleWheeledVehicleDef : public VehicleDef {
public:
    ~SimpleWheeledVehicleDef() override;
    entity::Entity* Spawn(entity::SpawnParams& params) override;   // slot 7

    // Clones the template body named bodyName from the level's physics world
    // the first time (0x00245DD0).
    void ResolveBodyTemplate();

    char bodyName[0x20] = {};           // 0x2E9 "colDefName"
    PhysicsBody* bodyTemplate = nullptr;   // 0x30C
    SimpleWheeledVehicleSettings settings; // 0x310
};

// The controls help text (member +0x570, vtable 0x00B1BC30): layer 0 walks
// 0 idle -> 1 wait -> 2 driving prompts -> 3 wait -> 4 shooting prompts, each
// prompt page shown a limited number of times per vehicle.
class VehicleHelpText : public gameplay::StateMachine {
public:
    enum State : uint8_t {
        kHelp_Idle = 0,
        kHelp_WaitDrive = 1,
        kHelp_Drive = 2,
        kHelp_WaitShoot = 3,
        kHelp_Shoot = 4,
    };
    VehicleHelpText() : StateMachine(1, 4, kHelp_Idle) {}
    bool OnMessage(const Message& message) override;   // slot 9

    SimpleWheeledVehicle* owner = nullptr;   // +0x1C
    uint8_t drivePagesShown = 0;             // +0x20
    uint8_t shootPagesShown = 0;             // +0x21

private:
    static void HidePrompts();
};

class SimpleWheeledVehicle : public VehicleEntity, public entity::EntityInterface17C {
public:
    // Driving modes (+0x4C1).
    enum DriveMode : uint8_t { kDrive_Idle = 0, kDrive_Forward = 1, kDrive_Backward = 2 };

    // 0x00247AB8
    SimpleWheeledVehicle(SimpleWheeledVehicleDef* def, const entity::SpawnParams& params);
    ~SimpleWheeledVehicle() override;

    bool Update(float dt) override;                       // slot 4
    bool Release(bool kill) override;                     // slot 10
    void ResetState() override;                           // slot 11
    void UpdateTransform(float dt = 0.0f) override;       // slot 12
    void UpdateFade() override;                           // slot 14
    ape::Vec3 GetVelocity() override;                     // slot 35
    // Slot 72: the box size of the body (the definition's ColSize).
    ape::Vec3 GetUpVector() override;
    void ApplyPointImpulse(const ape::Vec3& impulse, const ape::Vec3& point) override;   // slot 97
    float GetMass() override;                             // slot 98
    void SetEnabled(bool on) override;                    // slot 99
    // Slot 102 and contact listener slot 0: a contact point of the body.
    virtual void OnContact(const void* event);
    void OnStep(const void* event) override { OnContact(event); }

    // Who drives the vehicle: a script through the external controller
    // (0x00246D50) or the player's pad again (0x0024DDBC).
    void GiveControlToScript() { controller = extController; }
    void GiveControlToPlayer() { controller = padController; }

    const SimpleWheeledVehicleDef* WheeledDef() const { return static_cast<const SimpleWheeledVehicleDef*>(def); }

    // One visual wheel (+0x210, stride 0x90).
    struct Wheel {
        uint32_t field_00 = 0;
        float spin = 0.0f;              // +0x04 NOTE: cleared on reset
        uint32_t field_08[2] = {};
        ape::Matrix34 local = kVehicleIdentity;   // +0x10
        ape::Matrix34 world = kVehicleIdentity;   // +0x50
    };

    float spawnDelay = 0.0f;                    // 0x188 input is ignored while positive
    entity::EntityComponent* skin = nullptr;    // 0x18C the gun
    entity::EntityComponent* controller = nullptr;      // 0x190
    entity::EntityComponent* padController = nullptr;   // 0x194 "TmpControl"
    entity::EntityComponent* extController = nullptr;   // 0x198 "TmpExtControl"
    void* shape = nullptr;                      // 0x19C NOTE: the body's shape, released with the vehicle
    PhysicsBody* body = nullptr;                // 0x1A0
    float mass = 0.0f;                          // 0x1A4
    uint32_t field_1A8 = 0;                     // 0x1A8 from the spawn description (+0xC8)
    float field_1AC = 0.0f;                     // 0x1AC
    float field_1B0 = 0.0f;                     // 0x1B0
    float field_1B4[4] = {};                    // 0x1B4
    float field_1C4[4] = {};                    // 0x1C4
    ape::Vec3 field_1E0{};                      // 0x1E0 NOTE: reset to the world up axis
    float field_1F0 = 0.0f;                     // 0x1F0
    bool grounded[2] = {};                      // 0x1F4 NOTE: front/rear axle on the ground
    float field_1F8[2] = {};                    // 0x1F8
    float field_200 = 90.0f;                    // 0x200
    Wheel wheels[4];                            // 0x210
    ape::Matrix34 chassis = kVehicleIdentity;   // 0x450 transform at the last reset
    ape::Vec3 velocity{};                       // 0x490 velocity without a body in the world
    ape::Vec3 field_4A0{};                      // 0x4A0
    ape::Vec3 impulse{};                        // 0x4B0 impulses received this frame
    bool turbo = false;                         // 0x4C0
    DriveMode driveMode = kDrive_Idle;          // 0x4C1
    ape::Vec3 wallNormal{};                     // 0x4D0 sum of the steep contact normals
    ape::Vec3 field_4E0{};                      // 0x4E0
    entity::Entity* target = nullptr;           // 0x4F0 what the gun aims at
    hud::HudObject* hudElement = nullptr;      // 0x4F4
    float field_4F8 = 0.0f;                     // 0x4F8
    ape::Matrix34 field_500 = kVehicleIdentity;   // 0x500
    void* phantom = nullptr;                    // 0x540 probe sphere
    float phantomRadius = 0.0f;                 // 0x544
    ape::ApeAnimScriptInstance* turboScript = nullptr;   // 0x548
    uint32_t turboScriptState = 0;              // 0x54C
    ape::ApeAnimScriptInstance* hornScript = nullptr;    // 0x550
    ape::ApeAnimScriptInstance* gunScript = nullptr;     // 0x554
    ape::ApeAnimScriptState* gunOnState = nullptr;       // 0x558 "On"
    ape::ApeAnimScriptState* gunEndState = nullptr;      // 0x55C "End"
    uint32_t scriptState = 0;                   // 0x560 state of the entity's own script
    float fireTimer = 0.0f;                     // 0x564 time since the last reset (the gun waits 0.25 s)
    bool fadeActive = false;                    // 0x568 bit 7
    bool fadeVisible = true;                    // 0x568 bit 6
    uint8_t fadeColor[4] = {};                  // 0x569
    VehicleHelpText helpText;                   // 0x570

private:
    void CreateControllers();                   // 0x00246D5C
    void StartScripts();                        // 0x002464F0
    void CreateProbePhantom();                  // 0x002462D8
    void MarkWheelMaterials();                  // 0x00245F70
    void CreateBody();                          // TODO(0x00247068)
    void SyncFromBody();                        // 0x0024B064
    void UpdateGun(float dt);                   // 0x0024A2E0
    void UpdateTurboScript();                   // 0x00248958
    void UpdateScriptState();                   // 0x00248BB0
    // Script state of the current motion (driving, idle or airborne).
    uint32_t MotionState() const;
    // Swaps to another vehicle definition (the costume button).
    void ChangeDefinition(SimpleWheeledVehicleDef* newDef);   // TODO(0x0024B1E8)
    static void QueuedUpdateTransform(void* unused, void* context);   // 0x0024C784
    // Not ported yet (vector code; see the TODOs in the .cpp).
    void UpdateDriving(float dt);               // TODO(0x0024A4D0)
    void UpdateWheels(float dt);                // TODO(0x00249C18)
    void ProbeAhead(float dt);                  // TODO(0x00248E00)
    void UpdateSounds(float dt);                // TODO(0x00248470)
    void PlaceWheels();                         // TODO(0x0024C860)
};

} // namespace objects
} // namespace sly
