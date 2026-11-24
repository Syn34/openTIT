// openTIT - Level AI: the controller that drives one AI character, and the
// level's AI manager.
//
// Every AI driven entity gets a controller object (console vtable
// 0x00B0DC70, about 0x138 bytes) that installs itself as the entity's
// listener (+0xF4). The controller keeps its own copy of the entity's
// transform, runs a small state machine (state stack of four), steers the
// entity along the nav mesh or a proxy point patrol path and holds the
// entity it is currently interested in ("target", usually the player).
// Boss and creature behaviours derive from it (vtables 0x00B0E028 ..
// 0x00B0EDA0).
//
// The manager (console pointer 0x00F557F0) lists the live controllers,
// remembers where the player was last seen and owns the priority target
// list.
//
// NOTE: neither class name is known. Virtual slots are given with their
// number on the console vtable; slots whose default does nothing or returns
// a constant (0x007383F8..0x007387A0, outside this range) are written inline.
#pragma once

#include "ape/math/ApeMath.h"
#include "sly/entity/Entity.h"
#include "sly/levels/ai/AIPriorityTargets.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace levels {
struct AIProxyPoint;

namespace ai {

class PropertyBlock;
using entity::Entity;
using entity::EntityComponent;

// Navigation agent of a controller (NavPower; opaque here).
struct NavAgent;

class AIController : public entity::EntityListener {
public:
    // Controller flags (+0x130).
    enum Flags0 : uint8_t {
        kF0_Bit0          = 0x01,  // reported by slot 22
        kF0_PatrolReverse = 0x02,  // walking the patrol path backwards
        kF0_Leashed       = 0x04,  // stays within leashRadius of homePosition
        kF0_NavRetry      = 0x08,  // nav request failed for lack of a nav graph; retry
        kF0_NavMoving     = 0x10,  // following a nav path
        kF0_Messaged      = 0x20,  // set/cleared by messages 0x7B5D8742/0xFB303547
        kF0_Bit6          = 0x40,  // reported (inverted) by slot 23
        kF0_NoApply       = 0x80,  // do not push the transform to the entity
    };
    // Controller flags (+0x131).
    enum Flags1 : uint8_t {
        kF1_BodyEnabled   = 0x01,  // physics body wanted on
        kF1_Bit1          = 0x02,
        kF1_ForceRethink  = 0x04,  // choose a new state every frame
        kF1_Bit3          = 0x08,
        kF1_HitLocked     = 0x10,  // ignore hits until the state changes
        kF1_FreeFacing    = 0x20,  // facing may point up or down
        kF1_FlatFacing    = 0x40,  // facing is always horizontal
        kF1_Hit           = 0x80,  // hit since the last state change
    };
    // Controller flags (+0x132).
    enum Flags2 : uint8_t {
        kF2_SightDirty    = 0x40,  // line of sight to the target needs a new test
        kF2_TargetVisible = 0x80,
    };

    AIController() = default;
    ~AIController() override;

    // -- virtual interface ------------------------------------------------------------
    // Slot 2: takes over `entity` and registers with the manager.
    virtual void Attach(Entity* entity);
    // Slot 3: lets go of the entity, the target and every manager list.
    virtual void Detach();
    // Slot 4: AI on/off.
    virtual void SetActive(bool active);
    // Slot 5: per-frame update.
    virtual void Update(float dt);
    // Slot 6: pushes the controller transform to the entity.
    virtual void ApplyTransform(float dt);
    // Slot 7: ticks the entity (unless an ally is in charge of it).
    virtual void TickEntity(float dt);
    // Slot 10: first step of Update. NOTE: some behaviours look up bones here.
    virtual void PreUpdate(float dt);
    // Slot 12.
    void OnMessage(const entity::EntityMessage& msg) override;
    // Slot 13: moves controller (optionally) and entity to `transform`.
    virtual void Teleport(const ape::Matrix34& transform, bool moveController);
    // Slot 14: where the target is, or the player's last known position.
    virtual ape::Vec3 GetTargetPosition();
    // Slot 20 (+0x50): NOTE: maximum yaw error (degrees) before the controller
    // turns on the spot; 0 disables it.
    virtual float TurnInPlaceAngle() { return 0.0f; }
    // Slot 19 (+0x4C): NOTE: within this distance of the goal the controller
    // keeps its facing instead of turning around.
    virtual float FacingLockRadius() { return 0.0f; }
    // Slot 21 (EntityListener).
    bool AllowsInteraction() override { return true; }
    // Slot 30 (+0x78): kept in the manager's "active" list.
    virtual bool IsListedActive() { return false; }
    // Slot 31 (+0x7C): ally of the player; allies do not own their target and
    // their character is listed separately.
    virtual bool IsAlly() { return false; }
    // Slot 32 (+0x80): the transform is written straight into the entity
    // instead of through EntitySetTransform.
    virtual bool WritesEntityTransform() { return false; }
    // Slot 33 (+0x84): facing may tilt (flyers).
    virtual bool CanPitch() { return false; }
    // Slot 44 (+0xB0): gravity applies to the body velocity being built
    // (only its y is filled in at that point). The base version
    // (0x00738690) applies it while not moving up.
    virtual bool ShouldApplyGravity(const ape::Vec3& velocity) { return velocity.y < 1e-05f; }
    // Slot 11 (+0x2C): NOTE: event from outside (0 and 1 are handled by the
    // standard enemy).
    virtual void OnEvent(int kind) { (void)kind; }
    // Slot 21 is AllowsInteraction (EntityListener). Slots 22/23/25 (+0x58,
    // +0x5C, +0x64): NOTE: state queries asked by other modules; the base
    // answers from flags0.
    virtual bool IsEngaged() { return (flags0 & kF0_Bit0) != 0; }
    virtual bool IsCalm() { return !(flags0 & kF0_Bit6); }
    virtual bool CanBeTakenDown() { return false; }
    // Slot 26 (+0x68): NOTE: the player may sneak up on this one.
    virtual bool CanBeSneakedUpOn() { return false; }
    // Slot 38 (+0x98): NOTE: true in the states where the enemy is helpless.
    virtual bool IsHelpless() { return false; }
    // Slot 39 (+0x9C): the controlled character has no health left.
    virtual bool IsCharacterDead();
    // Slot 40 (+0xA0): NOTE: false while the behaviour is still on its way
    // somewhere (meaning inferred from the buddy override).
    virtual bool IsSettled() { return true; }
    // Slot 41 (+0xA4): command from a script (hash, two arguments and a value).
    virtual void OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
    {
        (void)command; (void)arg1; (void)arg2; (void)value;
    }
    // Slot 42 (+0xA8): debug name of a state.
    virtual const char* StateName(int s);
    // Slot 43 (+0xAC): NOTE: the alarm went off; the base version (0x007385A0)
    // drops the target.
    virtual void OnAlarm() { DropTarget(); }
    // Slot 47 (+0xBC): attacked by `attacker` (standard enemies may dodge).
    virtual void OnAttacked(Entity* attacker) { (void)attacker; }
    // Slot 48 (+0xC0): chance to taunt.
    virtual void OnTauntChance() {}
    // Slot 49 (+0xC4): damage taken. The base version (0x007386E0) sets
    // kF1_Hit unless hits are locked out.
    virtual void OnDamaged(float amount, uint32_t kind, const struct AIDamageInfo* info)
    {
        (void)kind; (void)info;
        if (amount > 0.0f && !(flags1 & kF1_HitLocked))
            flags1 |= kF1_Hit;
    }
    // Slot 53 (+0xD4): picks the next state.
    virtual void ChooseState();
    // Slot 54 (+0xD8): behaviour specific thinking; true when it chose a state.
    virtual bool Think(float dt);
    // Slot 55 (+0xDC).
    virtual void SetState(int state);
    // Slots 56-58 (+0xE0..+0xE8).
    virtual void EnterState();
    virtual void UpdateState(float dt);
    virtual void ExitState();
    // Slot 59 (+0xEC): advances the nav path. TODO(0x00015040).
    virtual void UpdateNavigation();
    // Slots 60/61 (+0xF0/+0xF4): settings of the nav agent: search radius and
    // the proxy point / nav filter (1 heavy, 2 light; see "Filter").
    virtual float NavSearchRadius() { return 0.0f; }
    virtual int NavFilter() { return 0; }
    // Slot 62 (+0xF8): a nav request produced a new goal.
    virtual void OnNavGoalChanged() {}
    // Slot 64 (+0x100): the facing error got larger than TurnInPlaceAngle().
    virtual void OnTurnInPlace() {}
    // Slot 65 (+0x104): a nav path step was taken.
    virtual void OnNavStep() {}
    // Slot 66 (+0x108): arrived at the current proxy point.
    virtual void OnProxyPointReached();
    // Slot 67 (+0x10C): uses the nav mesh.
    virtual bool UsesNavigation() { return true; }
    // Slot 68 (+0x110): NOTE: scale of the root motion step.
    virtual float RootMotionScale() { return 1.0f; }
    // Slot 69 (+0x114): lets a behaviour change the root motion step (local
    // translation) before it is applied.
    virtual ape::Vec3 FilterRootMotion(const ape::Vec3& step) { return step; }
    // Slot 70 (+0x118): refreshes the direction/distance to the target.
    virtual void UpdateTargetInfo(float dt);
    // Slot 71 (+0x11C).
    virtual void SetTarget(Entity* target);
    // Slot 72 (+0x120): a state change may interrupt `state`.
    virtual bool IsStateInterruptible(int state);
    // Slot 73 (+0x124): turning, root motion and body velocity.
    virtual void UpdateMovement(float dt);
    // Slot 74 (+0x128): vertical speed added when gravity applies.
    virtual float GravitySpeed() { return 0.0f; }

    // -- helpers ----------------------------------------------------------------------
    // Turns the transform to face `direction` (horizontal part only, up axis
    // straight up); ignored for a vertical direction (0x00015F90).
    void FaceDirection(const ape::Vec3& direction);
    // Sets the home position; while following a nav path the nav agent's
    // reachable goal is used instead (0x00015944).
    void SetHomePosition(const ape::Vec3& position);
    // Home becomes the current proxy point (0x000159EC).
    void ReturnToProxyPoint();
    // Sets the facing direction (normalised; flattened unless pitching is allowed).
    void SetFacing(const ape::Vec3& direction);
    void NormalizeFacing();
    // Lets go of the target (dropping our reference unless we are an ally).
    void DropTarget();
    // True while within the leash of the home position.
    bool IsWithinLeash() const;
    void SetTransform(const ape::Matrix34& transform);
    void SetPosition(const ape::Vec3& position);
    // Plays a clip on the character component and remembers it.
    void PlayClip(uint32_t clipHash);
    // Starts an anim script on the entity: the one-shot form runs detached,
    // the state form replaces the controller's state script.
    void StartOneShotScript(uint32_t scriptHash);
    void StartStateScript(uint32_t scriptHash);
    // Creates the NavPower agent (needs a loaded nav graph).
    void CreateNavAgent();
    // Pushes a state on the state stack (at most four deep).
    void PushState(int state);
    // The physics body is keyframed (motion type 1).
    bool IsBodyKeyframed() const;
    // Steps along the patrol path of the current proxy point.
    void AdvancePatrol();
    // Re-tests the line of sight to the target if needed.
    bool IsTargetVisible();
    // Line of sight test between two points. TODO(0x000138B0): Ghidra reports
    // bad instruction data; not ported (reports "not visible").
    bool TestLineOfSight(const ape::Vec3& from, const ape::Vec3& to, Entity* other);

    Entity* GetEntity() const { return entity; }
    int GetState() const { return state; }

    // -- data -------------------------------------------------------------------------
    int32_t state = -1;                          // +0x04
    int32_t stateStack[4] = {};                  // +0x08
    int32_t stateDepth = 0;                      // +0x18
    int32_t field_1C = -1;                       // +0x1C
    uint32_t currentClip = 0;                    // +0x20
    float stateTime = 0.0f;                      // +0x24
    float stateRandom = 0.0f;                    // +0x28 random [0,1] picked on entering a state
    int32_t stateRepeat = 0;                     // +0x2C times the same state was re-entered
    Entity* entity = nullptr;                    // +0x30
    AIController* partner = nullptr;             // +0x34 NOTE: another controller this one follows
    Entity* character = nullptr;                 // +0x3C the entity if it is a character (type bit 3)
    EntityComponent* characterComp = nullptr;    // +0x40 entity slot 15
    EntityComponent* body = nullptr;             // +0x44 entity slot 16
    ape::ApeAnimScriptInstance* stateScript = nullptr;   // +0x48
    float turnRate = 0.0f;                       // +0x4C degrees per second, 0 = no turning
    ape::Vec3 lastPosition{};                    // +0x50
    ape::Vec3 homePosition{};                    // +0x60
    ape::Vec3 navGoal{};                         // +0x70
    ape::Matrix34 transform{};                   // +0x80
    ape::Vec3 facing{};                          // +0xC0
    float leashRadius = 1.0f;                    // +0xD0
    NavAgent* navAgent = nullptr;                // +0xD4
    void* navPoint = nullptr;                    // +0xD8 NavPower point handle (released in the destructor)
    AIProxyPoint* proxyPoint = nullptr;          // +0xE0 current patrol point
    Entity* target = nullptr;                    // +0x100
    ape::Vec3 targetDir{};                       // +0x110
    float targetDist = 0.0f;                     // +0x120
    // +0x124 slot elsewhere that points back at this controller (cleared on
    // disengage). NOTE: what owns the slot is not known.
    AIController** claimSlot = nullptr;
    int32_t bodyMotion = 0;                      // +0x12C motion type of the body when attached
    uint8_t flags0 = 0;                          // +0x130 Flags0
    uint8_t flags1 = 0;                          // +0x131 Flags1
    uint8_t flags2 = 0;                          // +0x132 Flags2
    // +0x134 AI settings object; only filled in by the behaviours that read
    // settings (from the AI component, see AIComponent::GetSettings()).
    const PropertyBlock* settings = nullptr;
};

// Damage description handed to OnDamaged.
struct AIDamageInfo {
    uint32_t field_00 = 0;
    Entity* source = nullptr;    // +0x04
    uint32_t field_08 = 0;
    uint16_t field_0C = 0;
    uint16_t flags = 0;          // +0x0E NOTE: bit 12 makes sentries react
};

// One remembered sighting (0x50 bytes on the console; eight per block).
struct AISighting {
    Entity* entity = nullptr;                    // +0x00
    uint32_t field_04 = 0;                       // +0x04
    uint32_t kind = 0;                           // +0x08
    int32_t bone = -1;                           // +0x0C bone of the entity it refers to
    uint8_t data[0x40] = {};                     // NOTE: rest not decoded
};

// The level's AI manager (console pointer 0x00F557F0).
class AIManager {
public:
    static constexpr int kMaxControllers = 256;
    static constexpr int kMaxAllies = 9;
    static constexpr int kMaxSightings = 16;

    AIManager();

    // Adds a controller (and its lists) when it attaches.
    void Register(AIController* controller);
    // Takes a controller out of every list (0x00012544).
    void Unregister(AIController* controller);
    // Forgets the sighting of `entity` (0x0001285C).
    void ForgetSighting(Entity* entity);
    // Forgets the sightings of `entity` of the given kind whose bone still
    // exists (0x0001BCF4).
    void ForgetSighting(Entity* entity, uint32_t kind);
    // Nearest listed, living enemy within `radius` of `position` (optionally
    // only engaged ones) (0x0001AE6C).
    Entity* FindNearestEnemy(float radius, const ape::Vec3& position, bool engagedOnly) const;
    // Drops priority targets that are no longer alive (0x0001A7C0).
    void PrunePriorityTargets();
    // Level start: links the proxy point patrol chains and sets the kill
    // height (0x000191B0).
    void OnLevelLoaded();
    void ClearSightings();       // 0x0001AC68
    // Level start / level end / reset of the AI systems (0x0001AA34,
    // 0x0001AB3C, 0x0001A840).
    void OnLevelStart();
    void OnLevelEnd();
    void Reset();
    // Looks up the objects of the platform jump links (0x00019210).
    void ResolveJumpLinks();
    // Shows or hides every non-ally AI entity (0x0001B2F4).
    void SetEntitiesVisible(bool visible);
    // Hands a script command to every controller in the active list
    // (0x0001B3BC).
    void BroadcastScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value);
    // Target for an enemy: the nearest priority target, else the nearest of
    // the player and the allies (0x0001AC80).
    Entity* PickTarget(const AIController* controller) const;

    int32_t controllerCount = 0;                       // +0x00
    int32_t activeCount = 0;                           // +0x04
    int32_t allyCount = 0;                             // +0x10
    int32_t sightingCount = 0;                         // +0x18
    float killHeight = 0.0f;                           // +0x20 below this the AI snaps back to its entity
    float targetingPause = 0.0f;                       // +0x28 no new targets while positive
    float timeSinceSeen = 0.0f;                        // +0x2C
    ape::Vec3 lastPlayerPosition{};                    // +0x30
    AIController* controllers[kMaxControllers] = {};   // +0x250
    AIController* active[kMaxControllers] = {};        // +0x650
    Entity* allies[kMaxAllies] = {};                   // +0xE50
    AIPriorityTargets priorityTargets;                 // +0x14 / +0xE74
    AISighting* sightings[kMaxSightings] = {};         // +0xE94 in use first, then free
    AISighting sightingPool[kMaxSightings];            // +0xEE0
private:
    void ResetSightings();
};

// The level's AI manager (null between levels).
AIManager* GetAIManager();
void SetAIManager(AIManager* manager);

// Noise the enemies may hear (AlertEnemies kinds).
enum AINoise : int {
    kNoise_Normal = 0,      // fixed 15 m
    kNoise_Footsteps = 1,   // AI.AudioAlertRangeFootsteps
    kNoise_Bottle = 2,      // AI.AudioAlertRangeBottle
};
// Tells the enemies within the noise's range of `source` (0x0001B4A0).
// NOTE: the console passes an unused first argument.
void AlertEnemies(Entity* source, int noise, bool loud);

// Per-player threat timers (NOTE: owner and name unknown; the character
// module and the game loop use them).
struct AIThreatState {
    // True (and restarted) when the chase voice-over may play (0x0001BE50).
    bool ConsumeChaseVO();
    // Restarts the recovery time after the player was attacked (0x0001BE8C).
    void RestartAttackRecovery();
    // Forwards to the listener (slots 9/8) (0x0001BEA0 / 0x0001BEE8).
    void NotifyListener();
    void NotifyListener(uint32_t value);

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void OnValue(uint32_t value) { (void)value; }   // slot 8
        virtual void OnNotify() {}                             // slot 9
    };
    Listener* listener = nullptr;    // +0x24
    float attackRecovery = 0.0f;     // +0x44
    float chaseVOTimer = 0.0f;       // +0x48
};

// Point on segment [a, b] closest to p (0x00013E40).
ape::Vec3 ClosestPointOnSegment(const ape::Vec3& a, const ape::Vec3& b, const ape::Vec3& p);
// Signed angle (radians) from `from` to `to` about the up axis (0x00016680).
float SignedAngle(const ape::Vec3& from, const ape::Vec3& to);

// Proxy point patrol chains: the head point lists its child points in a ring.
AIProxyPoint* ProxyPointNext(const AIProxyPoint* point);       // 0x000154AC
AIProxyPoint* ProxyPointRandom(const AIProxyPoint* point);     // 0x000155E0

} // namespace ai
} // namespace levels
} // namespace sly
