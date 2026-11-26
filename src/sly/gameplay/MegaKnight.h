// openTIT - Mega knight boss (entity vtable 0x00B1DAB0, 0x440 bytes) and its
// twelve phases (0x00303D38-0x0031AF50).
//
// The boss is an actor whose layer 0 state is the index of the running phase.
// Each phase is a small state machine (vtable base 0x00B1DCA8) with an anim
// script; its layer 0 states are named by a per-phase table and entering a
// state switches the script to the state of the same name. When a phase
// reports it is done the boss picks the next one from the behaviour lists of
// its definition (MegaKnightBehavior.h).
//
// Only part of the boss is ported: the phase framework, the small phase
// methods, the phase switch and the helpers they use. The big per-frame
// functions (boss Update 0x0030F798, the phases' updates, the behaviour
// runner 0x0030E1E0) are TODO.
#pragma once

#include "sly/entity/ActorEntities.h"
#include "sly/game/Tween.h"
#include "sly/gameplay/MegaKnightBehavior.h"
#include "sly/gameplay/StateMachine.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }
namespace ape::world { class ApeObj; }
namespace sly { class Spawner; }

namespace sly {
namespace gameplay {

class MegaKnight;

// Phase ids (the boss's layer 0 state; names from the table 0x00AD9928).
enum MegaKnightPhaseId : int32_t {
    kMKPhase_Idle = 0,
    kMKPhase_Laser = 1,
    kMKPhase_LaserQuick = 2,
    kMKPhase_GroundSmash = 3,
    kMKPhase_MonitorPlatform = 4,
    kMKPhase_ShootPlatform = 5,
    kMKPhase_BlockShot = 6,
    kMKPhase_RopeAttached = 7,
    kMKPhase_ShoulderHit = 8,
    kMKPhase_Struggle = 9,
    kMKPhase_TargetHit = 10,
    kMKPhase_TowerShake = 11,
    kMKPhase_Count = 12,
    kMKPhase_None = 0xFF,
};

// Two objects shown or hidden together (0x0C bytes; boss +0x32C and +0x338).
struct MegaKnightObjectPair {
    entity::Entity* objects[2] = {};   // +0x00 / +0x04
    bool shown[2] = {};                // +0x08 / +0x09

    // 0x0030C018: shows or hides both.
    void SetShown(bool on);
    // 0x0030C14C: hiding hides both; showing shows only the first (`first`)
    // or the second object.
    void Show(bool on, bool first);

private:
    void Apply(int index, bool on);
};

// The platform object of the fight (0x70 bytes, boss +0x368): a geometry
// object with two anim scripts.
struct MegaKnightPlatform {
    // 0x00303D38: takes (or instantiates) geometry 0x6F3D51EA and starts the
    // scripts.
    explicit MegaKnightPlatform(MegaKnight* owner);
    ~MegaKnightPlatform();
    // 0x00308F5C: stops the scripts and switches the boss's platform script
    // state; `full` also shows the object and resets the motion values.
    void Reset(bool full);

    ape::world::ApeObj* object = nullptr;            // +0x00
    MegaKnight* boss = nullptr;                      // +0x04
    ape::Vec3 field_10{};                            // +0x10
    ape::Vec3 position{};                            // +0x20 (starts at console global 0x01164B30)
    ape::Vec3 field_30{};                            // +0x30
    float field_40 = 0.0f;                           // +0x40
    float height = 1.0f;                             // +0x44 height of the object's world box
    float field_48 = 0.0f;                           // +0x48
    float field_4C = 0.0f;                           // +0x4C
    ape::Vec3 up{0.0f, 1.0f, 0.0f, 0.0f};            // +0x50
    ape::ApeAnimScriptInstance* scriptB = nullptr;   // +0x60 script 0x048D2280
    ape::ApeAnimScriptInstance* scriptA = nullptr;   // +0x64 script 0x454F68A7
    bool field_68 = false;                           // +0x68
    bool field_69 = false;                           // +0x69
};

// A stamp that landed (0x90 bytes): the stamp entity and its shadow /
// effect entity. NOTE: only the two entity slots are decoded.
struct MegaKnightStampRecord {
    uint8_t data_00[0x60] = {};
    entity::Entity* entity = nullptr;      // +0x60
    entity::Entity* effect = nullptr;      // +0x64
    uint8_t data_68[0x28] = {};
};

// A stamp waiting to come down (0x70 bytes).
struct MegaKnightPendingStamp {
    struct MegaKnightStampManager* owner = nullptr;   // +0x00
    uint32_t kind = 0;                     // +0x04
    ape::Matrix34 transform{};             // +0x10 where it comes down
    float timer = 0.0f;                    // +0x50
    float delay = 1.0f;                    // +0x54 (1 when not positive)
    float duration = 1.0f;                 // +0x58 (1 when not positive)
    float field_5C = 0.0f;                 // +0x5C
    uint32_t field_60 = 0;                 // +0x60
    bool flag = false;                     // +0x64
};

// The stamp manager (0xC0 bytes, boss +0x294): the knight's feet coming down
// (stamp sets from the definition) and the stamps on the ground.
struct MegaKnightStampManager {
    // 0x00304D94
    explicit MegaKnightStampManager(MegaKnight* owner);
    ~MegaKnightStampManager();
    // 0x0030B4AC: releases the landed stamps' entities and forgets every
    // stamp.
    void Clear();
    // Clear() after resetting the counters (inline at its users).
    void Reset();
    // 0x0030BE48: queues a stamp.
    void QueueStamp(float delay, float duration, uint32_t kind, const ape::Matrix34& transform,
                    uint32_t field60, bool flag);
    // Idle phase test: nothing falling, at most `count` stamps landed, none
    // queued.
    bool Settled(uint32_t count) const;
    void ClearPending();

    MegaKnight* boss = nullptr;                          // +0x00
    ape::Vec3 field_10{};                                // +0x10
    ape::Vec3 field_20{};                                // +0x20
    int32_t field_30 = 0;                                // +0x30
    ape::ApeTArray<MegaKnightStampRecord*> landed;       // +0x34
    ape::ApeTArray<MegaKnightPendingStamp*> pending;     // +0x48
    int32_t falling = 0;                                 // +0x5C
    int32_t field_60 = 0;                                // +0x60
    ape::Vec3 field_70{};                                // +0x70
    ape::Vec3 field_80{};                                // +0x80
    uint32_t field_90[5] = {};                           // +0x90
    float leadDist = 0.0f;                               // +0xA4 "Missile.LeadDist"
    entity::EntityDef* stampDef = nullptr;               // +0xB0 definition 0xD4A3F67A
    entity::EntityDef* stampDefB = nullptr;              // +0xB4 definition 0xB267EAFB
    entity::EntityDef* effectDef = nullptr;              // +0xB8 definition 0x8F7C1D39
};

// Tracking block at +0x1B0 (built by 0x00768298, outside this module): the
// player's position history and debounced player state flags.
struct MegaKnightTracking {
    ape::Vec3 anchor{};                 // +0x1B0 copied to the boss's focus point by idle phases
    ape::Vec3 field_1C0{};              // +0x1C0
    ape::Vec3 field_1D0{};              // +0x1D0
    int32_t bone = 0;                   // +0x1E0 boss bone 0x0021C5E0
    void* camera = nullptr;             // +0x1E4 the game camera
    int32_t field_1E8 = -1;             // +0x1E8
    uint8_t history[0x24] = {};         // +0x1EC ring of 16-byte records (capacity 16)
    ape::Vec3 field_210{};              // +0x210
    DelayedState state220;              // +0x220 (0.75 s)
    DelayedState fightActive;           // +0x22C (0.5 s) NOTE: name from its uses
    DelayedState state238;              // +0x238 (0.5 s)
    DelayedState state244;              // +0x244 (1 s)
    DelayedState state250;              // +0x250
    DelayedState onArm[3];              // +0x25C the player is on arm 0..2. NOTE: name assumed
    DelayedState ropesHeld;             // +0x280 NOTE: cleared when the ropes are reset
};

// The mega knight boss entity.
class MegaKnight : public entity::ActorEntity {
public:
    ~MegaKnight() override;
    bool Release(bool kill) override;                       // slot 10
    // The boss takes no hits through the hit categories.
    void OnHitCategory0(entity::EntityHitRecord& hit) override;
    void OnHitCategory1(entity::EntityHitRecord& hit) override;
    void OnHitCategory2(entity::EntityHitRecord& hit) override;
    void OnHitCategory3(entity::EntityHitRecord& hit) override;
    void OnHitCategory4(entity::EntityHitRecord& hit) override;
    // Primary slot 98 / +0x17C slot 5: contacts are ignored.
    void Interface17CSlot5(const void* event) override;

    // Running phase (layer 0 state) and its object (null when invalid).
    int CurrentPhaseIndex() const;
    class MegaKnightPhase* CurrentPhase() const;
    // 0x0030D2B8: leaves the running phase and enters `phase` (unless the
    // phase is invalid or the "StateLocked" tuning switch is on).
    void SwitchPhase(uint32_t phase);
    // 0x0030932C / 0x0030912C helpers: forwards an event to the running phase.
    void PhaseEventA();
    // 0x0030C3F0 / 0x0030C2B4: marks an entity as a target on the player's
    // targeting gadget (with a marker offset) / takes the mark off.
    void AddMarkedTarget(entity::Entity* target, const ape::Vec3& offset);
    void RemoveMarkedTarget(entity::Entity* target);
    void ClearMarkedTargets();
    // 0x0030DBF0: switches the fight camera to the definition's camera
    // `name` over `ease` seconds (< 0: the camera's own time); only while the
    // fight is active unless `force`.
    void SetCamera(float ease, uint32_t name, bool force);
    // Plays a clip on the boss unless it is already playing (the blend is
    // skipped once after skipBlend was set).
    void PlayAnim(uint32_t clip, float speedScale, bool loop);
    // Slides the glow blend (+0x400) to `target` at 0.5 per second.
    void BlendGlowTo(float target);
    // Puts the three ropes back and forgets that they are held.
    void ResetRopes();
    // 0x0030E1E0: picks the next phase from the behaviour lists. TODO.
    uint32_t ChooseNextPhase();
    // 0x00304298: stops the arm marker spawners and releases the marker objects.
    void ClearArmMarkers();
    // 0x00311590: swaps the marker object of arm `arm`. TODO.
    void SetArmMarker(uint32_t arm, bool on);

    // -- members (offsets on the console) ---------------------------------
    MegaKnightTracking tracking;                       // 0x1B0..0x28B
    void* cameraController = nullptr;                  // 0x290 (0xCC bytes, 0x00305828; not ported)
    MegaKnightStampManager* stampManager = nullptr;    // 0x294
    // State block (built by 0x00768478).
    bool field_298 = false;                            // 0x298
    bool secondHalf = false;                           // 0x299 NOTE: switches the clip and value sets
    uint32_t currentAnim = 0;                          // 0x29C
    uint32_t currentCamera = 0;                        // 0x2A0
    bool armIntact[3] = {true, true, true};            // 0x2A4 arms 0..2
    bool field_2A7 = false;                            // 0x2A7
    int32_t field_2A8 = 0;                             // 0x2A8
    int32_t hitCount = 0;                              // 0x2AC
    bool field_2B0 = false;                            // 0x2B0
    void* behavior = nullptr;                          // 0x2B4 running behaviour (MegaKnightBehavior)
    int32_t behaviorAction = 0;                        // 0x2B8
    bool skipBlend = false;                            // 0x2BC the next clip starts without a blend
    bool restartBehavior = false;                      // 0x2BD
    class MegaKnightPhase* phases[kMKPhase_Count] = {};   // 0x2C0
    // Arm marker spawners (second half of the fight at 0x2F0, first half at 0x2FC).
    Spawner* armSpawners[2][3] = {};                   // 0x2F0
    entity::Entity* ropes[3] = {};                     // 0x308
    entity::Entity* field_314[4] = {};                 // 0x314
    entity::Entity* armTargets[2] = {};                // 0x320 / 0x324 NOTE: marked while arms 0 / 2 stand
    entity::Entity* field_328 = nullptr;               // 0x328
    MegaKnightObjectPair pair32C;                      // 0x32C
    MegaKnightObjectPair pair338;                      // 0x338
    entity::Entity* heartTarget = nullptr;             // 0x344 NOTE: name from the marker offset it gets
    ape::ApeTArray<void*> field_350;                   // 0x350 (capacity 32)
    MegaKnightPlatform* platform = nullptr;            // 0x368
    ape::ApeTArray<entity::Entity*> spawned;           // 0x36C entities the boss spawned (capacity 64)
    entity::Entity* armObject = nullptr;               // 0x380
    ape::ApeTArray<entity::Entity*> retiredArmObjects; // 0x384 (capacity 8)
    ape::ApeTArray<entity::Entity*> markedTargets;     // 0x398
    ape::ApeAnimScriptInstance* script3AC = nullptr;   // 0x3AC
    ape::ApeAnimScriptInstance* scripts3B0[3] = {};    // 0x3B0
    int32_t field_3BC = -1;                            // 0x3BC
    int32_t field_3C0 = -1;                            // 0x3C0
    ApproachVec3 lookAt;                               // 0x3D0 (current at +0x3D0)
    ape::Vec3 focus{};                                 // 0x3E0 point the boss looks at
    FloatTween glow;                                   // 0x400 NOTE: name assumed (0..1 blend)
    int32_t targetArm = 0;                             // 0x424
    bool field_428 = false;                            // 0x428

    // Placeholder for the layer 0 state of the console's state machine base
    // (the entity port does not model it).
    int32_t phaseIndex = kMKPhase_Idle;
};

// Base of the phases (vtable 0x00B1DCA8).
class MegaKnightPhase : public StateMachine {
public:
    // Inline on the console: layer 0 starts in state 0; the phase loads its
    // anim script and the hashes of its state names.
    MegaKnightPhase(MegaKnight* owner, uint32_t scriptName, const char* const* stateNames, int stateCount);
    ~MegaKnightPhase() override;

    bool OnMessage(const Message& message) override;     // slot 9
    // Slot 10 (+0x28): the boss switched to this phase.
    virtual void OnEnter();
    // Slot 11 (+0x2C): the boss leaves this phase for `next`.
    virtual void OnExit(uint32_t next);
    // Slots 12-15: events the boss forwards (12: 0x0030932C, 13: an
    // interaction (0x0030912C), 14: message 0x39F6520D, 15: message
    // 0xD39C61BE). NOTE: the meanings are not decoded.
    virtual void OnEventA();
    virtual void OnEventB();
    virtual void OnEventC();
    virtual void OnEventD();
    // Slot 16 (+0x40): where the boss looks while the phase runs.
    virtual void UpdateFocus();
    // Slots 17/18: NOTE: meaning unknown (0x00768924 / 0x0076892C defaults).
    virtual bool Query17() const { return false; }
    virtual bool Query18() const { return true; }
    // Slot 19 (+0x4C): the phase is over (default 0x00768934: always).
    virtual bool IsDone() { return true; }
    // Slot 20: the phase id (default 0x0076893C: -1).
    virtual int PhaseId() const { return -1; }
    // Slot 21: name of a state (default 0x00768944: "Invalid").
    virtual const char* StateName(int state) const { (void)state; return "Invalid"; }
    // Slot 22 (+0x58): per-frame update; false requests another pass.
    virtual bool UpdatePhase(float dt) = 0;

    // 0x003044B0: replaces the phase's anim script and binds the boss to it.
    void SetScript(uint32_t scriptName);
    // 0x00312274: switches the script to the state named `name`.
    void EnterScriptState(uint32_t name);

    int32_t enterCount = 0;                           // +0x1C
    MegaKnight* boss = nullptr;                       // +0x20
    float field_24 = 0.0f;                            // +0x24 cleared on entry
    ape::ApeAnimScriptInstance* script = nullptr;     // +0x28
    ape::ApeTArray<uint32_t> stateNames;              // +0x2C name hashes per state
};

// "Idle" (vtable 0x00B1DD18, 0x4C bytes).
class MKPhaseIdle : public MegaKnightPhase {
public:
    explicit MKPhaseIdle(MegaKnight* owner);   // 0x00304C14
    ~MKPhaseIdle() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_Idle; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    bool waitForStamps = false;   // +0x40
    uint32_t stampCount = 0;      // +0x44 stamps that must have landed
    float duration = 0.0f;        // +0x48 0: until the next behaviour
};

// "Laser" (vtable 0x00B1DD80, 0x80 bytes).
class MKPhaseLaser : public MegaKnightPhase {
public:
    explicit MKPhaseLaser(MegaKnight* owner);   // 0x00304A5C
    ~MKPhaseLaser() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventA() override;
    void UpdateFocus() override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_Laser; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    ape::Vec3 target{};           // +0x40
    float duration = 0.0f;        // +0x50 "Laser.DefaultDuration" when not set
    FloatTween sweep;             // +0x54
    bool interrupted = false;     // +0x78
};

// "LaserQuick" (vtable 0x00B1DDE8, 0x44 bytes).
class MKPhaseLaserQuick : public MegaKnightPhase {
public:
    explicit MKPhaseLaserQuick(MegaKnight* owner);
    ~MKPhaseLaserQuick() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventA() override;
    void UpdateFocus() override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_LaserQuick; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    bool interrupted = false;     // +0x40
};

// "BlockShot" (vtable 0x00B1DE50, 0x4C bytes).
class MKPhaseBlockShot : public MegaKnightPhase {
public:
    explicit MKPhaseBlockShot(MegaKnight* owner);
    ~MKPhaseBlockShot() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_BlockShot; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    uint32_t field_40 = 0;        // +0x40
    uint32_t swatAnim = 0;        // +0x44
    uint32_t field_48 = 0;        // +0x48
};

// "MonitorPlatform" (vtable 0x00B1DEB8, 0x5C bytes).
class MKPhaseMonitorPlatform : public MegaKnightPhase {
public:
    explicit MKPhaseMonitorPlatform(MegaKnight* owner);
    ~MKPhaseMonitorPlatform() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventA() override;
    void OnEventC() override;
    void UpdateFocus() override;
    bool Query17() const override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_MonitorPlatform; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    uint32_t anims[3] = {};       // +0x44 start, loop, end
    int32_t arm = -1;             // +0x54 arm the player stands on
    bool eventC = false;          // +0x58
};

// "ShootPlatform" (vtable 0x00B1DF20, 0x58 bytes).
class MKPhaseShootPlatform : public MegaKnightPhase {
public:
    explicit MKPhaseShootPlatform(MegaKnight* owner);
    ~MKPhaseShootPlatform() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventA() override;
    void OnEventC() override;
    void UpdateFocus() override;
    bool Query17() const override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_ShootPlatform; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    uint32_t anims[3] = {};       // +0x44 charge, shoot, end
    bool aborted = false;         // +0x54
};

// "RopeAttached" (vtable 0x00B1DF88, 0xC0 bytes).
class MKPhaseRopeAttached : public MegaKnightPhase {
public:
    explicit MKPhaseRopeAttached(MegaKnight* owner);   // 0x00304834
    ~MKPhaseRopeAttached() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventB() override;
    void UpdateFocus() override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_RopeAttached; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    int32_t rope = -1;            // +0x40 rope the player holds (0 or 2)
    void* spline = nullptr;       // +0x44 the laser's path
    ape::Vec3 laserPoint{};       // +0x50
    bool reversed = false;        // +0x60
    bool field_61 = false;        // +0x61
    bool eventB = false;          // +0x62
    float eventTime = 0.0f;       // +0x64 time in state of the last event B
    float field_68 = 0.0f;        // +0x68
    float field_6C = 0.0f;        // +0x6C
    float distance = 0.0f;        // +0x70
    int32_t field_74 = -1;        // +0x74
    uint8_t data_78[0x20] = {};   // +0x78
    float field_98 = 0.0f;        // +0x98
    float field_9C = 0.0f;        // +0x9C
    float field_A0 = 0.0f;        // +0xA0
    float field_A4 = 0.0f;        // +0xA4
    float field_A8 = 0.0f;        // +0xA8
    float field_AC = 0.0f;        // +0xAC
    bool field_B0 = false;        // +0xB0
    float field_B4 = 0.0f;        // +0xB4
};

// "GroundSmash" (vtable 0x00B1DFF0, 0x7C bytes).
class MKPhaseGroundSmash : public MegaKnightPhase {
public:
    explicit MKPhaseGroundSmash(MegaKnight* owner);   // 0x00304654
    ~MKPhaseGroundSmash() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_GroundSmash; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    ape::ApeTArray<char> pattern;   // +0x40 smash sequence ('S'/'F' per smash)
    int32_t patternIndex = 0;       // +0x54
    bool field_58 = false;          // +0x58
    bool field_59 = false;          // +0x59
    bool mirrored = false;          // +0x5A
    bool field_5B = false;          // +0x5B
    char patternText[0x20] = {};    // +0x5C
};

// "ShoulderHit" (vtable 0x00B1E058, 0x48 bytes).
class MKPhaseShoulderHit : public MegaKnightPhase {
public:
    explicit MKPhaseShoulderHit(MegaKnight* owner);
    ~MKPhaseShoulderHit() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    void OnEventD() override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_ShoulderHit; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    int32_t side = 0;             // +0x40 (0 or 2 like the ropes)
    uint32_t sequence = 0;        // +0x44 cut scene played
};

// "Struggle" (vtable 0x00B1E0C0, 0x44 bytes).
class MKPhaseStruggle : public MegaKnightPhase {
public:
    explicit MKPhaseStruggle(MegaKnight* owner);
    ~MKPhaseStruggle() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_Struggle; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    int32_t rope = 0;             // +0x40
};

// "TargetHit" (vtable 0x00B1E128, 0x44 bytes).
class MKPhaseTargetHit : public MegaKnightPhase {
public:
    explicit MKPhaseTargetHit(MegaKnight* owner);
    ~MKPhaseTargetHit() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_TargetHit; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    int32_t rope = 0;             // +0x40
};

// "TowerShake" (vtable 0x00B1E190, 0x60 bytes).
class MKPhaseTowerShake : public MegaKnightPhase {
public:
    explicit MKPhaseTowerShake(MegaKnight* owner);
    ~MKPhaseTowerShake() override;
    void OnEnter() override;
    void OnExit(uint32_t next) override;
    bool Query17() const override;
    bool IsDone() override;
    int PhaseId() const override { return kMKPhase_TowerShake; }
    const char* StateName(int state) const override;
    bool UpdatePhase(float dt) override;

    uint32_t field_40 = 0;        // +0x40
    uint32_t igcAnim = 0;         // +0x44 NOTE: played in the "IGC" state (assumed)
    uint32_t field_48 = 0;        // +0x48
    uint32_t startAnim = 0;       // +0x4C
    uint32_t field_50 = 0;        // +0x50
    uint32_t endAnim = 0;         // +0x54 NOTE: assumed from the state order
    bool skipStart = false;       // +0x5C enter straight in the "IGC" state
};

} // namespace gameplay
} // namespace sly
