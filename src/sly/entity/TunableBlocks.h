// openTIT - The tunable blocks of a character definition besides the camera
// and damage settings (Character.h): movement, shoulder camera, disguise and
// one more. Each loads its values from the tuning variables and reloads them
// when the tuning changes (constructed by the character definition, 0x0001BFE0
// / 0x0011A608 / 0x0011E2B8).
#pragma once

#include "sly/camera/CameraModes.h"
#include "sly/entity/Character.h"

namespace sly {
namespace entity {

// Movement settings (vtable 0x00B0DB54, Entity.Character.Move.* and the
// carry tuning; 0x1E0 bytes).
class MoveSettingsBlock : public TunableBlock {
public:
    ~MoveSettingsBlock() override;
    void LoadDefaults() override;
    bool Reload() override;

    bool groundAlignToSurface = false;      // +0x04
    bool groundTurnInPlace = false;         // +0x05
    float groundIdleTime = 0.0f;           // +0x08
    float groundJoyMinTurn = 0.0f;         // +0x0C
    float groundJoyMinWalk = 0.0f;         // +0x10
    float groundJoyMinRun = 0.0f;          // +0x14
    float groundJoyMinSprint = 0.0f;       // +0x18
    float groundSpeedmaxDefault = 0.0f;    // +0x1C
    float groundSpeedmaxSprint = 0.0f;     // +0x20
    float groundAnglemaxDefault = 0.0f;    // +0x24
    float groundTurnMaxDefault = 0.0f;     // +0x28
    float groundTurnMaxRun = 0.0f;         // +0x2C
    float groundTurnMaxSprint = 0.0f;      // +0x30
    float groundTurnAccelTime = 0.0f;      // +0x34
    float groundWalkAnimSpeed = 0.0f;      // +0x38
    float groundRunAnimSpeed = 0.0f;       // +0x3C
    float groundSprintAnimSpeed = 0.0f;    // +0x40
    ape::Vec3 groundTakedownOffset{}; // +0x50
    float groundStrafeAnalogDeg = 0.0f;    // +0x60
    float groundCombatSpeedmax = 0.0f;     // +0x64
    float groundCombatAnimSpeed = 0.0f;    // +0x68
    float groundSneakSpeedmax = 0.0f;      // +0x6C
    float groundSneakAnimSpeed = 0.0f;     // +0x70
    float groundCrawlTurnInterp = 0.0f;    // +0x74
    float groundCrawlSpeedmaxDefault = 0.0f; // +0x78
    float groundCrawlAnglemaxDefault = 0.0f; // +0x7C
    float groundCrawlTurnMaxDefault = 0.0f; // +0x80
    uint32_t carryAttachBone = 0;       // +0x84
    ape::Vec3 carryAttachOffset{};   // +0x90
    ape::Vec3 carryAttachRot{};      // +0xA0
    ape::Vec3 carryThrowOffset{};    // +0xB0
    float carryThrowDist = 0.0f;           // +0xC0
    float carryThrowVelocity = 0.0f;       // +0xC4
    ape::Vec3 carryDamageVelocity{}; // +0xD0
    float groundAimSpeedmaxRun = 0.0f;     // +0xE0
    float groundSkidTimeMax = 0.0f;        // +0xE4
    float groundSkidSpeedMin = 0.0f;       // +0xE8
    float groundSkidFrictionScalar = 0.0f; // +0xEC
    float groundLeanFactor = 0.0f;         // +0xF0
    float groundLeanAngleMax = 0.0f;       // +0xF4
    float jumpPowerMaxDefault = 0.0f;      // +0xF8
    float doublejumpTimeMin = 0.0f;        // +0xFC
    float doublejumpTimeMax = 0.0f;        // +0x100
    float doublejumpPowerMax = 0.0f;       // +0x104
    float airJumpTimeMax = 0.0f;           // +0x108
    float airSpeedMaxDefault = 0.0f;       // +0x10C
    float airSpeedMaxDoublejump = 0.0f;    // +0x110
    float airAccDefault = 0.0f;            // +0x114
    float airAccDoublejump = 0.0f;         // +0x118
    float airJumpStrafeTime = 0.0f;        // +0x11C
    float airAngleMaxDefault = 0.0f;       // +0x120
    float airTurnMaxDefault = 0.0f;        // +0x124
    float airVertInterp = 0.0f;            // +0x128
    float climbJoyMin = 0.0f;              // +0x12C
    float climbRailSpeedMaxWalk = 0.0f;    // +0x130
    float climbRailSpeedMaxRun = 0.0f;     // +0x134
    float climbRailSpeedMaxSprint = 0.0f;  // +0x138
    float climbRailAnimSpeedWalk = 0.0f;   // +0x13C
    float climbRailAnimSpeedRun = 0.0f;    // +0x140
    float climbRailAnimSpeedSprint = 0.0f; // +0x144
    ape::Vec3 climbPoleOffset{};     // +0x150
    float climbPoleSpeedMax = 0.0f;        // +0x160
    float climbPoleAnimSpeed = 0.0f;       // +0x164
    ape::Vec3 climbZiplineOffset{};  // +0x170
    ape::Vec3 climbHookSwingOffset{}; // +0x180
    float climbSidleSpeedMax = 0.0f;       // +0x190
    float climbSidleSpeedHiMin = 0.0f;     // +0x194
    float climbSidleAnimSpeedLo = 0.0f;    // +0x198
    float climbSidleAnimSpeedHi = 0.0f;    // +0x19C
    float climbJumpPowerMaxRail = 0.0f;    // +0x1A0
    float climbJumpPowerMaxPoint = 0.0f;   // +0x1A4
    float climbJumpPowerMaxPole = 0.0f;    // +0x1A8
    float climbJumpPowerMaxChain = 0.0f;   // +0x1AC
    float climbJumpPowerMaxRope = 0.0f;    // +0x1B0
    float climbJumpPowerMaxZipline = 0.0f; // +0x1B4
    float climbJumpPowerMaxSwing = 0.0f;   // +0x1B8
    float climbJumpPowerMaxHang = 0.0f;    // +0x1BC
    float climbJumpPowerMaxGrind = 0.0f;   // +0x1C0
    float climbJumpPowerMaxSpring = 0.0f;  // +0x1C4
    float climbJumpPowerMaxSidle = 0.0f;   // +0x1C8
    float climbJumpPowerMaxTrack = 0.0f;   // +0x1CC
    float climbJumpPowerMaxVehicle = 0.0f; // +0x1D0
    float climbJumpPowerMaxStealth = 0.0f; // +0x1D4
    float superSpirePowerMaxDefault = 0.0f; // +0x1D8
    float aimDodgeCooldownTime = 0.0f;     // +0x1DC
};

// Over-the-shoulder camera settings (vtable 0x00B0DB84; the settings sit at +0x10).
class ShoulderSettingsBlock : public TunableBlock {
public:
    ~ShoulderSettingsBlock() override;
    void LoadDefaults() override;
    bool Reload() override;

    camera::ShoulderSettings settings;   // +0x10
};

// Settings block of vtable 0x00B0DB9C: only the disguise expiration rate
// (+0xC8) comes from the tuning. NOTE: the rest of the block is not decoded.
class DisguiseSettingsBlock : public TunableBlock {
public:
    ~DisguiseSettingsBlock() override;
    void LoadDefaults() override;
    bool Reload() override;

    float disguiseExpirationRate = 0.0f;   // +0xC8
};

// Settings block of vtable 0x00B14120 (loader 0x00119D8C, not ported).
class SettingsBlockB14120 : public TunableBlock {
public:
    ~SettingsBlockB14120() override;
    void LoadDefaults() override;
    bool Reload() override;
};

} // namespace entity
} // namespace sly
