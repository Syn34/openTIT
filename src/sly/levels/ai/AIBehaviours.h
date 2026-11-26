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
#include "ape/memory/ApeTArray.h"

#include <array>
#include <vector>

namespace ape::world { class ApeObj; class ApeSplineInstance; }
namespace sly::hud { class HudObject; }
namespace sly::entity { class GameCamera; }
namespace sly::camera { class CameraMode; class StrafeCameraMode; }

namespace sly {
namespace levels {
namespace ai {

// Ambient critters (vtable 0x00B0E028, 0x1A0 bytes; settings 0x00F94784..).
class CritterAI : public AIController {
public:
    ~CritterAI() override;
    float AgentRadius() override;
    float Slot16() override;
    int Slot17() override;
    uint32_t Slot18() override;
    bool IsEngaged() override;
    bool WritesEntityTransform() override;
    bool HasNoSpawnClip() override;
    float RootMotionScale() override;
    // States (AIController::state). NOTE: names from use.
    enum State {
        kState_Spawn = 0,
        kState_Idle = 1,
        kState_ScriptAnim = 2,    // plays a clip a script asked for
        kState_Move = 3,
        kState_GoTo = 4,          // walks to a point a script named
        kState_Approach = 5,      // NOTE: names of 5-7 from what they lead to
        kState_Wait = 6,
        kState_Return = 7,
        kState_Flee = 8,
        kState_FleeTurn = 9,
        kState_Charge = 0x0A,
        kState_Attack = 0x0B,
        kState_Notice = 0x0C,
        kState_Dead = 0x0D,
        kState_Gone = 0x0E,
        kState_Perch = 0x0F,      // NOTE: flies to a perch point (see 0x00020100)
        kState_Land = 0x10,
        kState_Perched = 0x11,
        kState_TakeOff = 0x12,
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
    // Slot 52: empty for critters.
    // @func 0x0001ECC4 CritterAI::Slot52
    void Slot52() override {}

    void TickEntity(float dt) override;
    void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value) override;
    void ChooseState() override;
    bool Think(float dt) override;
    void EnterState() override;
    void UpdateState(float dt) override;

    // `point` kept inside the area the critter may roam: the spawner's
    // alternative object box (shrunk by the wander margin), else the
    // bounding radius around the spawn point (0x000212B8).
    ape::Vec3 ClampToWanderArea(const ape::Vec3& point);
    // Flight clip for the way the critter is heading: up, banking left or
    // right, or the move clip (0x000210F8).
    void UpdateFlightClip();
    // Clip played while moving (0x0001F330).
    void SetMoveClip(uint32_t clipHash);
    // Keeps the critter on the ground (0x0001D548) with a probe between two
    // points (0x0001D1E0, TODO).
    void SnapToGround(bool fromSpawnHeight);
    bool GroundProbe(const ape::Vec3& top, const ape::Vec3& bottom, ape::Vec3& hit);
    // Point to run to, away from the threat `toThreat` points at (0x0001E420).
    ape::Vec3 FleePoint(const ape::Vec3& toThreat);
    // Quick turn while fleeing (part of Think) and a perch point on a rail
    // (TODO 0x00020100).
    void QuickTurn(const Entity& player);
    // Random point to wander to: inside the spawner's alternative object
    // box, else around the spawn point within the bounding radius; kept at
    // the current height unless the critter flies (0x0001E068).
    ape::Vec3 RandomWanderPoint();
    bool FindPerch();

    float idleDelay = 0.0f;           // +0x138
    uint32_t field_13C = 0;           // +0x13C
    float quickTurnDelay = 0.0f;      // +0x140
    float wanderMargin = 0.0f;        // +0x144 kept from the edge of the wander area
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
    ~PlayerAI() override;
    void TickEntity(float dt) override;
    bool Slot24() override;
    bool IsAlly() override;
    bool Slot36() override;
    int NavFilter() override;
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

    // +0x140 place next to the player the AI manager assigns (in the
    // player's frame).
    ape::Vec3 surroundOffset{};
    uint32_t field_150 = 0;           // +0x150
    // +0x154 targets are only handed over within this distance of
    // startPosition (none when not positive).
    float field_154 = 20.0f;
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
    ~PatternBossAI() override;
    void TickEntity(float dt) override;
    bool IsCharacterDead() override;
    int NavFilter() override;
    void Update(float dt) override;
    void PreUpdate(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    void ChooseState() override;
    bool Think(float dt) override;
    void ExitState() override;

    // Starts or stops the looping effect script (0x00027E98).
    void SetLoopScript(bool on);
    // Moves the pattern cursor on (wrapping) and enters the state of the
    // pattern step `step` that was due: "Shoot" 4, 0x040A726F 3, "Idle" 2,
    // 0xFB4F8FFE 7, 0xFB497E59 10, anything else "Idle" (0x000296BC).
    void RunPatternStep(uint32_t step);

    ape::ApeAnimScriptInstance* loopScript = nullptr;   // +0x164
    // The attack pattern: step names (hashes) run in turn. NOTE: filled by
    // code not ported yet.
    uint32_t patternCount = 0;                         // +0x168
    const uint32_t* pattern = nullptr;                 // +0x16C
    uint32_t patternCursor = 0;                        // +0x170
};

// Racer (vtable 0x00B0E428, 0x170 bytes): spawned by a spawner that keeps
// a race track; the spawner ranks its racers by their progress along it.
class RacerAI : public AIController {
public:
    void TickEntity(float dt) override;
    int NavFilter() override;
    void UpdateMovement(float dt) override;
    // racerFlags (+0x164).
    enum RacerFlags : uint32_t {
        kRacer_Bit29 = 1u << 29,
        kRacer_Bit30 = 1u << 30,
        kRacer_Racing = 1u << 31,   // the entity is a racer with a vehicle (+0x1FC)
    };

    ~RacerAI() override;
    void Attach(Entity* entity) override;
    void Update(float dt) override;
    void PreUpdate(float dt) override;
    void OnMessage(const entity::EntityMessage& msg) override;
    void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    bool IsCharacterDead() override;
    void ChooseState() override;
    bool Think(float dt) override;
    void ExitState() override;
    // Slot 75: place in the race (0 = first).
    virtual void SetRacePosition(float position);
    // The entity (type bit 9) moves the way it faces: its heading follows
    // the direction of its z axis (0x0002D8C8).
    void AlignMoveHeading();

    float progress = 0.0f;            // +0x14C position along the race track
    uint32_t field_138 = 0;           // +0x138
    float racePosition = 0.0f;        // +0x13C place in the race
    ape::Vec3 field_150{};            // +0x150
    hud::HudObject* hud = nullptr;    // +0x160 HUD element 0x2508CAD1
    uint32_t racerFlags = 0;          // +0x164
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
    // The target is within the longer of the ranged and far attack ranges
    // and, when the settings ask for it, in sight (0x00034444).
    bool TargetInAttackRange();
    void TickEntity(float dt) override;
    float AgentRadius() override;
    float Slot16() override;
    int Slot17() override;
    uint32_t Slot18() override;
    float TurnInPlaceAngle() override;
    bool CanBeSneakedUpOn() override;
    bool Slot27() override;
    bool Slot28() override;
    void Slot29() override;
    bool IsListedActive() override;
    bool HasNoSpawnClip() override;
    bool IsHeavy() override;
    int Slot46() override;
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
    // Byte +0x1D2.
    enum EnemyFlagsC : uint8_t {
        kEC_Bit5 = 0x20,        // set by slot 29. NOTE: meaning unknown
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

    void OnProxyPointReached() override;
    void OnNavGoalChanged() override;

    // Lets go of the target and any claim when engaged (0x0002EC04).
    void Disengage();
    // Enters combat against `target`; the first time also posts the alert
    // game event unless the settings say "CombatAlways" (0x0003094C).
    void Engage(Entity* target);
    // Looks for a better target (0x0002F900).
    void RefreshTarget();
    // Takes over the entity's animation (0x00030A70).
    void LockEntityAnimation();
    // Calls nearby enemies for help (0x00035D44).
    void CallForHelp();

    // Two patrolling enemies may play an interaction clip with each other:
    // `other` is close enough (between PatrolInteractionMinDist and MaxDist),
    // enough time has passed, the character has the clip and is in one of
    // the calm states (0x00039314).
    bool CanStartPatrolInteraction(AIController* other, uint32_t clip);
    // Turns to `other` and enters the interaction state (13), after a turn
    // in place when it is more than 45 degrees off (0x00039178).
    void StartPatrolInteraction(AIController* other, uint32_t clip);

    int32_t stateAfterTurn = 0;       // +0x144 state entered once a turn in place ends
    uint8_t field_180 = 0;            // +0x180
    uint8_t field_181 = 0;            // +0x181
    uint32_t proxyClip = 0;           // +0x184 clip named by the reached proxy point
    float attackTimer = 0.0f;         // +0x198 (InitialAttackTime on activation)
    ape::Vec3 field_19C{};            // +0x19C
    float field_1B0 = 0.0f;           // +0x1B0 NOTE: interactions are refused while positive
    // +0x1B4 NOTE: compared with PatrolInteractionMinTime (seconds since the
    // last patrol interaction).
    float patrolInteractionTime = 0.0f;
    uint32_t field_1B8 = 0;           // +0x1B8
    ape::Vec3 field_1BC{};            // +0x1BC
    float field_1A8 = 0.0f;           // +0x1A8 NOTE: slot 46 reports it being positive
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
    ~SentryAI() override;
    float Slot16() override;
    int Slot17() override;
    uint32_t Slot18() override;
    bool CanPitch() override;
    void Attach(Entity* entity) override;
    void Update(float dt) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    float GravitySpeed() override;
    bool CanBeSneakedUpOn() override;
    bool IsSettled() override;
    bool ShouldApplyGravity(const ape::Vec3& velocity) override;
    void ExitState() override;
    void OnProxyPointReached() override;
    void UpdateMovement(float dt) override;

    Entity* alertReceiver = nullptr;  // +0x138 told when the sentry is hit
    uint8_t settled = 0;              // +0x13C
    uint8_t field_13D = 0;            // +0x13D
    uint8_t field_13E = 1;            // +0x13E
    uint32_t idleClip = 0;            // +0x140
    uint32_t field_144 = 0;           // +0x144
};
// The variant is Grizz's ice cave sentry ("Entity.BismarqueIceCave.*"
// tuning): between actions it either plays an impatient animation facing its
// digging machine or looks around (away from the machine or at the player),
// and hits are only counted on its butt (a capsule along a bone).
class SentryAIVariant : public SentryAI {
public:
    ~SentryAIVariant() override;
    static constexpr int kDartCount = 3;

    void Attach(Entity* entity) override;
    void Detach() override;
    void Update(float dt) override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    bool IsSeeThroughLayer(int layer) override;

    float actionTimer = 0.0f;          // +0x148 time to the next idle action
    Entity* diggingMachine = nullptr;  // +0x154 ("DiggingMachineInst"; referenced)
    int32_t buttHits = 0;              // +0x158
    // +0x15C attached children released on detach. NOTE: assumed to be the
    // darts stuck in it ("DartDef").
    Entity* darts[kDartCount] = {};
};

// Bosses (vtables 0x00B0E888, 0x00B0E9C0, 0x00B0EAF8, 0x00B0EC30).
// Bismarque's attack patterns: per fight phase three lists of steps (the
// normal one and two alternatives picked by the controller's mode bits).
struct BismarqueStep {          // 0x2C bytes
    int32_t kind = 0;           // +0x00 0-12, see BismarqueAI::EnterStepState
    uint8_t field_04[0x28] = {};
};
struct BismarqueStepList {
    const BismarqueStep* steps = nullptr;   // +0x00
    uint32_t field_04 = 0;
    uint16_t count = 0;                     // +0x08
    uint16_t field_0A = 0;
};
struct BismarquePhase {         // 0x74 bytes
    BismarqueStepList normal;   // +0x00
    uint8_t field_0C[0x1C];
    BismarqueStepList modeB;    // +0x28 controller mode bit 31
    uint8_t field_34[0x08];
    BismarqueStepList modeC;    // +0x3C controller mode bit 30 (wins over bit 31)
    uint8_t field_48[0x2C];
};
struct BismarquePattern {       // NOTE: only the phase table is known
    uint8_t header[0x70];
    BismarquePhase phases[1];   // +0x70, one per fight phase
};

class BismarqueAI : public AIController {
public:
    // Empty lists and pool, no attack running, the counter attack and QTE
    // intro timers at their tuning maxima (0x000175FC).
    BismarqueAI();
    // Frees the boss's lists (their element types are not decoded).
    ~BismarqueAI() override;
    bool AllowsInteraction() override;
    bool IsBoss() override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    int NavFilter() override;
    ape::Vec3 GetTargetPosition() override;
    bool Think(float dt) override;
    float GravitySpeed() override;

    // Moves the two ground markers to their anchors (0x0004399C).
    void UpdateMarkers();
    // Shows or hides the two ground markers (0x00043A80).
    void SetMarkersVisible(bool first, bool second);
    // Weight of moving to `point` facing `direction`: lower distance and
    // turning count better with the default (negative) weights (0x00044318).
    float ScoreTargetPoint(const ape::Vec3& point, const ape::Vec3& direction) const;
    // Counter attack cool-down ran out and the player is close (0x00046638).
    bool CounterAttackReady(float dt);

    // Step list of the current phase for the current mode.
    const BismarqueStepList& CurrentSteps() const;
    // The current step (0x0003E040).
    const BismarqueStep& CurrentStep() const;
    // Moves to the next step; past the last one the attack ends (0x00042D5C).
    void AdvanceStep();
    // Enters the AI state of the current step's kind (0x0003E0CC).
    void EnterStepState();

    uint32_t modeFlags = 0;                // +0x204 bit 30 mode C, bit 31 mode B
    int32_t currentAttack = -1;            // +0x250 -1 between attacks
    int32_t stepIndex = 0;                 // +0x254
    const BismarquePattern* pattern = nullptr;   // +0x264
    int32_t phase = 0;                     // +0x2CC
    ape::world::ApeObj* markers[2] = {};   // +0x290
    ape::world::ApeObj* anchors[2] = {};   // +0x298
    float counterCooldown = 0.0f;          // +0x2EC
    // Lists whose element types are not decoded (records of the sizes the
    // constructor gives).
    template <size_t N> struct Record { uint8_t bytes[N]; };
    // +0x134 the buttons of the QTE game (0x70 bytes each). NOTE: only the
    // fields used by the ported code are named.
    struct QteKey {
        uint32_t field_00;
        float time;                           // +0x04 when the button is due
    };
    struct QteButton {
        ape::Vec3 position;                   // +0x00 where it starts moving
        uint8_t field_10[0x20];
        ape::Vec3 target;                     // +0x30 where it moves to
        const QteKey* key;                    // +0x40
        ape::world::ApeObj* object;           // +0x44 the button's object
        uint8_t field_48[8];
        ape::ApeAnimScriptInstance* script;   // +0x50
        float interpTime;                     // +0x54
        float delay;                          // +0x58
        uint8_t field_5C[8];
        bool done;                            // +0x64
        uint8_t field_65[0xB];
    };
    ape::ApeTArray<QteButton> qteButtons{0x10};          // +0x134
    // Sends the buttons not done yet back to the start line beside the
    // first anchor (their targets along the detect bar by their key time),
    // one after another (the last first, `skip` buttons fewer) (0x00043784).
    void RestartQteButtons(int skip);
    // In state 0x19 and with no cutscene queued: hides (or shows) the two
    // ground markers and switches the attack effect scripts to their hidden
    // (0x8C3F8D2E) or shown (0x28298107) state (0x00041A08).
    void SetFightVisualsHidden(bool hidden);
    ape::ApeTArray<Record<4>> list148{0x40, nullptr, true};   // +0x148
    ape::ApeTArray<Record<4>> list15C{0x40, nullptr, true};   // +0x15C
    ape::ApeTArray<Record<0x30>> list170{0x40, nullptr, true}; // +0x170
    // +0x184 the points Bismarque retreats to (each a world object and a
    // word not read here).
    struct RetreatPoint {
        ape::world::ApeObj* object;
        uint32_t field_04;
    };
    ape::ApeTArray<RetreatPoint> retreatPoints{8};      // +0x184
    // The retreat point farthest from the player and Bismarque together (on
    // the ground plane) that lies ahead of Bismarque; a random one when none
    // does; null without points (0x000447E8).
    RetreatPoint* PickRetreatPoint();
    ape::ApeTArray<Record<0x20>> list198{4};             // +0x198
    // +0x1AC pool of ten 16-byte records (begin/end/cursor, capacity +0x1BC).
    std::vector<std::array<uint8_t, 16>> pool1AC;
    // +0x1C4 ice barriers spawned this attack, waiting to be thrown.
    ape::ApeTArray<Entity*> pendingBarriers{0x40, nullptr, true};
    ape::ApeTArray<Record<4>> list1D8{0x40, nullptr, true};   // +0x1D8
    // +0x1EC ice barriers being thrown, each with its throw delay.
    struct ThrownBarrier {
        Entity* entity;
        float delay;
    };
    ape::ApeTArray<ThrownBarrier> thrownBarriers{0x40, nullptr, true};
    Entity* field_200 = nullptr;           // +0x200 an entity the attack spawned (killed by ClearAttack)
    // Kills what the current attack spawned (the barriers' scripts hear
    // 0x879F2BDA first) and forgets the attack (0x0003F260).
    void ClearAttack();
    // Moves the waiting barriers to the thrown list, every second one
    // delayed by Entity.Bismarque.IceBarrier.ThrowOffsetTimemax, and clears
    // the player character's +0x1AC reference (0x00040208).
    void ThrowBarriers();
    ape::Vec3 field_210{};                 // +0x210
    uint16_t field_220 = 0, field_222 = 0; // +0x220
    uint16_t field_224 = 0, field_226 = 0; // +0x224
    uint32_t field_230 = 0;                // +0x230
    ape::Vec3 field_240{0.0f, 0.0f, 1.0f, 0.0f};   // +0x240 a direction (the engine's default forward)
    float qteIntroDelay = 0.0f;            // +0x2C8 Entity.Bismarque.QTE.Game.IntroDelayTimeMax
    uint32_t field_308 = 0;                // +0x308 flag bits 29-31 cleared
};
class ElJefeAI : public AIController {
public:
    ~ElJefeAI() override;
    void SetActive(bool active) override;
    void TickEntity(float dt) override;
    bool IsBoss() override;
    // One fight stage (0x14 bytes from +0x19C).
    struct Stage {
        uint32_t field_00 = 0;
        AIProxyPoint* point = nullptr;   // +0x04 where El Jefe stands
        uint32_t field_08[3] = {};
    };
    static constexpr int kMaxStages = 0x34;   // NOTE: count assumed (stages run up to +0x600)
    static constexpr int kMaxSpawned = 0x80;  // NOTE: capacity assumed (array runs up to +0x91C)

    void Detach() override;
    ape::Vec3 GetTargetPosition() override;
    void OnTauntChance() override;

    Stage stages[kMaxStages];           // +0x19C
    uint32_t field_41C = 0;             // +0x41C
    int32_t stageIndex = 0;             // +0x704
    uint8_t field_713 = 0;              // +0x713
    int32_t spawnedCount = 0;           // +0x718
    Entity* spawned[kMaxSpawned] = {};  // +0x71C entities the fight spawned
    AICameraMode* camera = nullptr;     // +0x91C
};
// Switches off the two trigger volumes of El Jefe's arena (0x000496CC).
void ElJefeDisableArenaTriggers();

class MsDecibelAI : public AIController {
public:
    ~MsDecibelAI() override;
    void TickEntity(float dt) override;
    bool IsEngaged() override;
    bool IsBoss() override;
    ape::Vec3 GetTargetPosition() override;
    float GravitySpeed() override;
    void OnTauntChance() override;
    void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value) override;
    // Slot 49: hits only count while she is open to them (state 5, or state
    // 12 after three seconds) and come from the right attack (flag bit 12,
    // category 1); outside phase 9 they hurt her character. The side she
    // dodges to next is drawn at random unless the attack says.
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;

    // Starts the fight (called by a level script command) (0x0005BB08).
    void Start();
    // The stretch of the fight spline between the point instances `from`
    // and `to` (their closest spline parameters; 0 and 1 without them)
    // (0x0005BA08, set by a level script command).
    void SetSplineStretch(uint32_t from, uint32_t to);
    // Switches the fight phase. TODO(0x00053A80): not ported.
    void SetPhase(int phase);
    // A player that comes within Entity.MsDecibel.Platform.CounterAttackDist
    // (on the ground plane) while Ms Decibel is not in states 5, 9, 11 or 12
    // triggers the counter attack (state 9), using up one of the counted
    // ones (0x000544B8).
    void CheckCounterAttack();

    int32_t phase = 0;         // +0x134
    int32_t counterAttacks = 0;   // +0x138 NOTE: counted down by CheckCounterAttack, never below 0
    // One glass block of the stage (0x14 bytes): the block and the arm that
    // raises it ("INST_GlassBlockA<row><column>" / "INST_GlassBlockArmA..").
    struct GlassBlock {
        Entity* arm = nullptr;      // +0x00
        Entity* block = nullptr;    // +0x04
        float timer = 0.0f;         // +0x08
        int32_t state = 0;          // +0x0C 3 raising, 4 down
        uint32_t field_10 = 0;      // +0x10
        // Finds the block of row `row + rowBase` and column `column` (both
        // counted from 0) and its arm, marks both busy and hides them; the
        // block starts down (0x00052080).
        void Bind(int row, int column, int rowBase);
    };
    static constexpr int kGlassRows = 5;
    static constexpr int kGlassColumns = 7;
    // +0x14C two sets of blocks (phases up to 1 use the first).
    GlassBlock glassBlocks[2][kGlassRows][kGlassColumns];
    // Shows every block of the current set with its arm raised (script
    // state "Up", arm animation at its end) and starts it raising
    // (0x00058848).
    void RaiseGlassBlocks();
    ape::world::ApeSplineInstance* spline = nullptr;   // +0x185C
    ape::world::ApeObj* stretchFrom = nullptr;          // +0x1860
    ape::world::ApeObj* stretchTo = nullptr;            // +0x1864
    float stretchStart = 0.0f;                          // +0x1868
    float stretchEnd = 1.0f;                            // +0x186C
    int32_t hitsTaken = 0;     // +0x181C
    bool dodgeSide = false;    // +0x1820
    bool started = false;      // +0x1870
};
class ParadoxAI : public AIController {
public:
    ~ParadoxAI() override;
    void TickEntity(float dt) override;
    bool IsEngaged() override;
    bool IsBoss() override;
    bool Think(float dt) override;
    bool IsSettled() override;
    void OnTauntChance() override;
    void OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info) override;
    float GravitySpeed() override;
    // Slot 41: command 0xCC65890A also switches to the camera of the
    // current phase; every command is kept with its value for the next
    // think (0x0005FA08).
    void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value) override;
    // TODO(0x0005E990 ParadoxAI::PlayCamera): pushes camera `name` with the
    // given interpolation.
    void PlayCamera(uint32_t name, float interp) { (void)name; (void)interp; }
    // Slot 3: leaves the listener groups and the boss state, takes the fight
    // camera off the player and its two modes off it, sends the player's
    // script state 0xBC1BA9E7 and deactivates the player, then the base.
    void Detach() override;
    // In states 0 and 1 a strafe camera keeps the player and Paradox in view
    // (created on the fight camera when missing, following the player's
    // height); in any other state it is removed (0x0005C848).
    void UpdateStrafeCamera();

    // Phase 1 begins: the five objects named by the arena entity's script
    // (object variables 0x11A825BA, 0x7D63B6B9, 0xE91F47B8, 0x54DAD8B7,
    // 0xC09669B6; null when missing) become the phase objects, and the
    // controller returns to state 0 (0x0005E470).
    void StartPhaseOne();

    int32_t phase = 0;            // +0x134
    int32_t phaseObjectCount = 0;               // +0x140
    ape::ApeRefCounted* phaseObjects[8] = {};   // +0x160 NOTE: capacity assumed
    entity::GameCamera* fightCamera = nullptr;          // +0x1A0 attached to the player during the fight
    camera::StrafeCameraMode* strafeMode = nullptr;    // +0x1A4
    camera::CameraMode* cameraMode = nullptr;          // +0x1A8 the phase camera (PlayCamera)
    Entity* arena = nullptr;      // +0x1AC NOTE: the entity whose script names the phase objects
    uint32_t pendingCommand = 0;  // +0x1B0
    float pendingValue = 0.0f;    // +0x1B4
    bool hasPendingCommand = false;   // +0x1B8
    uint8_t settled = 0;       // +0x201
};

} // namespace ai
} // namespace levels
} // namespace sly
