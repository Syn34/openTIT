// openTIT - Level AI behaviours: the controller classes the AI component
// builds (see AIComponent.h for the type hashes).
//
// Class names are descriptive guesses: "Critter" and "Player" are the type
// names; El Jefe, Ms Decibel, Bismarque and Paradox are named after the
// tuning variables and level objects their code uses; the rest after what
// their settings describe.
#pragma once

#include "sly/levels/ai/AIController.h"
#include "sly/levels/ai/AIHooks.h"

namespace ape::world { class ApeSplineInstance; }

namespace sly {
namespace levels {
namespace ai {

// Ambient critters (vtable 0x00B0E028, 0x1A0 bytes; settings 0x00F94784..).
class CritterAI : public AIController {
public:
    // States (AIController::state). NOTE: names from use.
    enum State {
        kState_Spawn = 0,
        kState_Idle = 1,
        kState_Move = 3,
        kState_Flee = 8,
        kState_Dead = 0x0D,
        kState_Hit = 0x13,
    };

    void Attach(Entity* entity) override;
    void Update(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    float FacingLockRadius() override;
    bool AllowsInteraction() override;
    const char* StateName(int s) override;
    void ExitState() override;
    void OnProxyPointReached() override;
    bool UsesNavigation() override;

    // Clip played while moving (0x0001F330).
    void SetMoveClip(uint32_t clipHash);
    // Keeps the critter on the ground. TODO(0x0001D548): the ground probe
    // (0x0001D1E0) has bad instruction data.
    void SnapToGround(bool fromSpawnHeight);
    // Point to run to, away from `away`. TODO(0x0001E420): bad instruction data.
    ape::Vec3 FleePoint(const ape::Vec3& away);

    float idleDelay = 0.0f;           // +0x138
    uint32_t field_13C = 0;           // +0x13C
    float quickTurnDelay = 0.0f;      // +0x140
    uint32_t field_144 = 0;           // +0x144
    float poiDelay = 0.0f;            // +0x148 time until the next point of interest
    ape::Vec3 poiPosition{};          // +0x150
    ape::Vec3 poiDirection{};         // +0x160
    ape::Vec3 boundsMin{};            // +0x170
    ape::Vec3 boundsMax{};            // +0x180
    uint32_t moveClip = 0;            // +0x190
    uint32_t idleClip = 0;            // +0x194
    Entity* attacker = nullptr;       // +0x198 sender of the last hit
    uint32_t hitArgument = 0;         // +0x19C
};

// Player buddies driven by the AI (vtable 0x00B0E1B8, 0x1B0 bytes; settings
// FollowDist / AttackDist / Murray / Bentley).
class PlayerAI : public AIController {
public:
    // Byte +0x1A0.
    enum BuddyFlags : uint8_t {
        kBuddy_Flag20 = 0x20,   // set by message 0x50A63D95
        kBuddy_Flag40 = 0x40,   // set by 0x00764FA9, cleared by 0x7D2BB408
    };

    void Attach(Entity* entity) override;
    void SetActive(bool active) override;
    void Update(float dt) override;
    void PreUpdate(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    bool IsSettled() override;
    const char* StateName(int s) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    void ChooseState() override;
    bool Think(float dt) override;
    void ExitState() override;
    void OnNavGoalChanged() override;

    // Follow mode (0 and 1 lock out hits) (0x00024048).
    void SetMode(int mode);
    // Pops the state stack and resumes the state below.
    void PopState();
    // Puts the buddy next to `point` facing it (0x00024670).
    void WarpTo(const ape::Vec3& point);
    // Re-reads the character and body components (0x0002550C).
    void RefreshComponents();

    uint32_t field_150 = 0;           // +0x150
    float field_154 = 20.0f;          // +0x154
    uint32_t field_158 = 0;           // +0x158 set to 1 by Update
    int32_t mode = 0;                 // +0x15C
    ape::Vec3 startPosition{};        // +0x160
    uint32_t field_170 = 0;           // +0x170
    uint32_t field_178 = 0;           // +0x178
    ape::Vec3 warpPoint{};            // +0x190
    uint8_t buddyFlags = 0;           // +0x1A0
};

// Boss that runs state patterns per level and spawns ghosts (vtable
// 0x00B0E2F0, 0x180 bytes).
class PatternBossAI : public AIController {
public:
    void Update(float dt) override;
    void PreUpdate(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    void ChooseState() override;
    bool Think(float dt) override;
    void ExitState() override;

    // Starts or stops the looping effect script (0x00027E98).
    void SetLoopScript(bool on);

    ape::ApeAnimScriptInstance* loopScript = nullptr;   // +0x164
};

// Racer (vtable 0x00B0E428, 0x170 bytes): spawned by a spawner that keeps
// a race track; the spawner ranks its racers by their progress along it.
class RacerAI : public AIController {
public:
    ~RacerAI() override;
    void Update(float dt) override;
    void PreUpdate(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    bool IsCharacterDead() override;
    void ChooseState() override;
    bool Think(float dt) override;
    void ExitState() override;
    // Slot 75: place in the race (0 = first). Implemented outside this range
    // (0x00738E00).
    virtual void SetRacePosition(float position) { (void)position; }

    float progress = 0.0f;            // +0x14C position along the race track
};

// Race state of a spawner (spawner +0x50, 0x3C bytes).
class RaceTracker {
public:
    // Re-ranks the racers of `spawner` by their progress along the track and
    // tells each its place (0x0002B618).
    void UpdateRanking(SpawnerRef* spawner);

    uint32_t field_00 = 0;
    ape::world::ApeSplineInstance* track = nullptr;   // +0x04
};

// Standard enemy (vtable 0x00B0E5B8, 0x1E0 bytes): patrol, cover, turret and
// combat settings.
class EnemyAI : public AIController {
public:
    // Byte +0x1D0.
    enum EnemyFlagsA : uint8_t {
        kEA_Bit1  = 0x02,   // movement held (no root motion)
        kEA_Bit3  = 0x08,   // kept when activated
        kEA_Taunt = 0x40,   // wants to taunt
        kEA_Bit7  = 0x80,   // set by event 0
    };
    // Byte +0x1D1.
    enum EnemyFlagsB : uint8_t {
        kEB_Bit1      = 0x02,   // in combat range checks
        kEB_FarSearch = 0x40,   // nav search radius 500 instead of 50
    };

    ~EnemyAI() override;
    void SetActive(bool active) override;
    void PreUpdate(float dt) override;
    void OnEvent(int kind) override;
    bool AllowsInteraction() override;
    bool IsEngaged() override;
    bool IsCalm() override;
    bool CanBeTakenDown() override;
    bool IsHelpless() override;
    bool IsCharacterDead() override;
    const char* StateName(int s) override;
    void OnAlarm() override;
    void OnAttacked(Entity* attacker) override;
    void OnTauntChance() override;
    float NavSearchRadius() override;
    int NavFilter() override;
    void OnTurnInPlace() override;
    ape::Vec3 FilterRootMotion(const ape::Vec3& step) override;
    void SetTarget(Entity* target) override;
    void UpdateMovement(float dt) override;
    float GravitySpeed() override;

    // Lets go of the target and any claim when engaged (0x0002EC04).
    void Disengage();
    // Looks for a better target (0x0002F900).
    void RefreshTarget();
    // Takes over the entity's animation (0x00030A70).
    void LockEntityAnimation();
    // Calls nearby enemies for help (0x00035D44).
    void CallForHelp();

    uint8_t field_180 = 0;            // +0x180
    uint8_t field_181 = 0;            // +0x181
    float attackTimer = 0.0f;         // +0x198 (InitialAttackTime on activation)
    ape::Vec3 field_19C{};            // +0x19C
    float field_1B0 = 0.0f;           // +0x1B0 NOTE: interactions are refused while positive
    uint32_t field_1B4 = 0;           // +0x1B4
    ape::Vec3 field_1BC{};            // +0x1BC
    uint32_t field_1C8 = 0;           // +0x1C8
    uint8_t enemyFlagsA = 0;          // +0x1D0
    uint8_t enemyFlagsB = 0;          // +0x1D1
    uint8_t enemyFlagsC = 0;          // +0x1D2
};

// Watcher with awareness, vision and detection cylinder settings (vtable
// 0x00B0E750, 0x150 bytes) and a variant of it (vtable 0x00B0EDA0, 0x1F0
// bytes; its code lies outside this range).
class SentryAI : public AIController {
public:
    void Attach(Entity* entity) override;
    void Update(float dt) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    float GravitySpeed() override;
    bool CanBeSneakedUpOn() override;
    bool IsSettled() override;
    bool ShouldApplyGravity(const ape::Vec3& velocity) override;
    void ExitState() override;

    Entity* alertReceiver = nullptr;  // +0x138 told when the sentry is hit
    uint8_t settled = 0;              // +0x13C
    uint8_t field_13D = 0;            // +0x13D
    uint8_t field_13E = 1;            // +0x13E
    uint32_t idleClip = 0;            // +0x140
    uint32_t field_144 = 0;           // +0x144
};
class SentryAIVariant : public SentryAI {
public:
};

// Bosses (vtables 0x00B0E888, 0x00B0E9C0, 0x00B0EAF8, 0x00B0EC30).
class BismarqueAI : public AIController {
public:
};
class ElJefeAI : public AIController {
public:
};
class MsDecibelAI : public AIController {
public:
    bool field_1870 = false;   // +0x1870
};
class ParadoxAI : public AIController {
public:
};

} // namespace ai
} // namespace levels
} // namespace sly
