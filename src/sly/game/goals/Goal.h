// openTIT - Job goals: the objectives of a job (mission).
//
// A job (sly/game JobManager, game state 0x00F64C20) owns an ordered list of
// goals. Every goal is a polymorphic object built from a <goal> element of
// the job XML (0x00322D88 picks the class from the element name, each class
// has a static Create(xml, desc) factory). Goals react to the game events the
// game world posts (GameEvent) through Update(), and move through the
// GoalStatus values below. On every status change a goal runs scripts or
// sequences (GoalScriptCursor lists) and toggles "hint" objects (GoalHint:
// entities, spawners, lasers... that are enabled while the goal is active).
//
// Console layout: the base object is 0xA0C bytes (vtable 0x00B1E1F8); its
// constructor 0x00769BD8 and destructor 0x00769480 live outside this
// module's address range, as do the one-line virtual defaults 0x00769Axx
// (implemented inline here, untagged). Subclass data starts at +0xA0C.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/game/GameEvent.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }
namespace ape { namespace xml { class XmlElement; } }

namespace sly {

class SlyGame;

namespace goals {

class Job;

// Goal::status (+0x12). Also mirrored in the top 4 bits of the job's per-goal
// save word (Job +0x20074 + saveSlot*4).
enum GoalStatus : uint8_t {
    kGoal_None     = 0,
    // NOTE: 1-3 are the "not yet available" states chosen from the goal
    // flags by Reset(); their exact meaning is not decoded.
    kGoal_Locked1  = 1,   // flags bit 2
    kGoal_Locked2  = 2,   // flags bit 1
    kGoal_Locked3  = 3,   // flags bit 0
    kGoal_Active   = 4,
    kGoal_Complete = 5,
    kGoal_Failed   = 6,
};

// Goal::flags (+0x14).
enum GoalFlags : uint32_t {
    kGoalFlag_Lock3          = 0x01,
    kGoalFlag_Lock2          = 0x02,
    kGoalFlag_Lock1          = 0x04,
    kGoalFlag_KeepComplete   = 0x10,   // Reset() leaves a completed goal alone
    kGoalFlag_StatusFrozen   = 0x80,   // slot 24; SetStatus() does nothing
    kGoalFlag_CountsProgress = 0x100,  // counted by the parent group's progress
};

// Script lists run on status changes (Goal::scripts[]).
enum GoalScriptSlot : int {
    kScripts_Active       = 0,   // status 4, run every update
    kScripts_Failed       = 1,   // status 6
    kScripts_Complete     = 2,   // status 5
    kScripts_CompleteIdle = 3,   // status 5, prepared by PrepareScripts()
    kScripts_ActiveIdle   = 4,   // status 4, prepared by PrepareScripts()
    kScriptSlotCount      = 5,
};

// One variable binding handed to a started anim script (0x4C bytes on the
// console, consumed by AnimScriptBindVariables 0x0049FCC0). Opaque here.
struct GoalScriptBinding {
    uint8_t data[0x4C];
};

// GoalScript::flags.
enum GoalScriptFlags : uint32_t {
    kScript_Done       = 1u << 26,   // finished; the cursor moved past it
    kScript_Prepared   = 1u << 27,   // its sequence stream was requested (0x002E9B08)
    kScript_Running    = 1u << 28,   // a sequence/instance is playing
    kScript_Started    = 1u << 29,
    kScript_Restart    = 1u << 30,   // Reset() makes a finished script run again
    kScript_IsSequence = 1u << 31,   // name is a gameplay sequence, not an anim script
};

// One script or sequence of a list (0x20 bytes on the console).
struct GoalScript {
    ape::ApeTArray<GoalScriptBinding> bindings;   // +0x00
    uint32_t name = 0;                            // +0x14 sequence / anim script name hash
    ape::ApeAnimScriptInstance* instance = nullptr; // +0x18 running anim script (not for sequences)
    uint32_t flags = 0;                           // +0x1C GoalScriptFlags
};

// Holder allocated per script slot (4 bytes on the console).
struct GoalScriptList {
    ape::ApeTArray<GoalScript*>* scripts = nullptr;

    uint32_t Count() const { return scripts ? scripts->Count() : 0; }
};

// Goal::scripts[] entry: the list and how far it has run.
struct GoalScriptCursor {
    GoalScriptList* list = nullptr;   // +0x00
    uint8_t next = 0;                 // +0x04 (bits 24-31) index of the script to run next
    bool finished = false;            // +0x04 (bit 23) every script has run
};

// What a hint switches on and off (GoalHint::kind).
enum GoalHintKind : int32_t {
    kHint_Entity       = 0,   // an entity with targeting (field_C0 bit 22): slot 76
    kHint_Interaction  = 1,   // interaction component of an entity
    kHint_Spawner      = 2,
    kHint_SpawnGroup   = 3,
    kHint_LaserGroup   = 4,   // NOTE: kinds 4-6 are the laser objects (sly/gameplay lasers)
    kHint_Laser        = 5,
    kHint_LaserField   = 6,
    kHint_Interactable = 7,   // entity with field_C8 bit 1 (0x001EB814)
};

// Forced value of a hint (top two bits of the byte at +0x08).
enum GoalHintForce : uint8_t {
    kHintForce_None = 0,
    kHintForce_On   = 1,
    kHintForce_Off  = 2,
};

// One hint object (0x14 bytes on the console, array at Goal +0x8E0).
struct GoalHint {
    uint32_t target = 0;          // +0x00 object name hash
    int32_t kind = 0;             // +0x04 GoalHintKind
    uint8_t force = kHintForce_None; // +0x08 bits 6-7
    bool invert = false;          // +0x09 enable when the goal is NOT active
    bool persistent = false;      // +0x0A keep the state once applied
    bool applied = false;         // +0x0B
    bool missing = false;         // +0x0C the target could not be found
    bool keepAlive = false;       // +0x0D passed to the deactivate calls
    void* object = nullptr;       // +0x10 resolved target

    // 0x00321324: finds the target object if not done yet; `report` enables
    // the (compiled out) warning for a missing target.
    void Resolve(bool report);
    // 0x003218B0: enables or disables the target for `owner` (may be null).
    void Apply(class Goal* owner, bool enable);
};

// HUD progress counter of a goal (slot 13).
struct GoalProgress {
    uint32_t name = 0;     // text id
    int32_t done = 0;
    int32_t total = 0;
};

// The entity the HUD arrow points at (slot 16).
struct GoalTarget {
    uint32_t name = 0;
    bool flag = false;     // NOTE: copied from Goal +0x9FE
};

// One waypoint marker (0x80 bytes on the console; built by the goals, drawn
// by the HUD). Only the entity is decoded.
struct GoalMarker {
    uint32_t field_00 = 0;
    uint32_t field_04 = 0;
    void* entity = nullptr;   // +0x08
    uint8_t rest[0x74] = {};
};

// List the markers are collected into (0x0075EBFC appends).
struct GoalMarkerList {
    ape::ApeTArray<GoalMarker> markers;
};

// One entry of the cycling text list (Goal +0x9F0, 8 bytes): a text and how
// long it stays up.
struct GoalCycleEntry {
    char* text = nullptr;
    float duration = 0.0f;
};

// A message sent to a goal through slot 2 (0x00321BE4): an id and two words.
struct GoalMessage {
    uint32_t sender = 0;   // +0x00
    uint32_t field_04 = 0;
    uint32_t id = 0;       // +0x08 message name hash (table 0x00F65BF8)
    uint32_t arg = 0;      // +0x0C
};

// Variable-length byte block copied from the parse description (four of
// them at +0x9B8.. on the console: data, size, extra word).
struct GoalBlob {
    uint8_t* data = nullptr;
    uint32_t size = 0;
    uint32_t extra = 0;
};

class Goal {
public:
    // Untagged stand-in for the console constructor 0x00769BD8 (out of
    // range): copies the parse description. Subclasses only use the job and
    // game pointers.
    explicit Goal(const struct GoalDesc* desc);
    virtual ~Goal();

    // slot 2 (+0x08)
    virtual void HandleMessage(const GoalMessage& msg);
    // slot 3: per class type id (0x0076Bxxx, out of range).
    virtual int TypeId() const { return 0; }
    // slot 4 (+0x10): processes one game event for `player`; returns the status.
    virtual uint8_t Update(int player, const GameEvent& e);
    // slot 5 (+0x14)
    virtual void OnLevelLoaded() {}
    // slot 6 (+0x18): resets the goal, `fromSave` takes the status from the
    // job's save word; `status` -1 picks it from the flags.
    virtual void Reset(bool fromSave, int status);
    // slot 7 (+0x1C)
    virtual void ResetHints(bool stopHintScript);
    // slot 8 (+0x20)
    virtual void PrepareScripts();
    // slot 9 (+0x24)
    virtual void ReapplyHints(Goal* owner);
    // slot 10 (+0x28)
    virtual void SetFlags(uint32_t value) { flags = value; }
    // slots 11/12 (+0x2C/+0x30): "???" for a hidden goal.
    virtual const char* GetName() { return status == kGoal_Locked2 ? "???" : name; }
    virtual const char* GetDescription() { return status == kGoal_Locked2 ? "???" : description; }
    // slot 13 (+0x34): progress counter shown by the HUD; false if none.
    virtual bool GetProgress(struct GoalProgress* out) { (void)out; return false; }
    // slot 14 (+0x38)
    virtual bool IsComplete() { return status == kGoal_Complete; }
    // slot 15 (+0x3C): adds the goal's waypoint markers (minigame goals let
    // the minigame do it).
    virtual void CollectMarkers(struct GoalMarkerList* list) { (void)list; }
    // slot 16 (+0x40): the entity the HUD points at; false if none.
    virtual bool GetTarget(struct GoalTarget* out);
    // slot 17 (+0x44)
    virtual uint32_t Field8A0() { return field_8A0; }
    // slot 18 (+0x48)
    virtual void CollectAllMarkers(struct GoalMarkerList* list) { CollectMarkers(list); }
    // slot 19 (+0x4C)
    virtual uint32_t Field8F4(int i) { return field_8F4[i]; }
    // slot 20 (+0x50)
    virtual uint32_t Field960() const { return field_960; }
    // slot 21 (+0x54): text of the current cycle entry and its fade value.
    virtual const char* CurrentCycleText(float* fade);
    // slot 22 (+0x58)
    virtual void ResetCycle();
    // slot 23 (+0x5C)
    virtual bool Field96C() const { return field_96C; }
    // slot 24 (+0x60)
    virtual void FreezeStatus(bool frozen)
    {
        flags = frozen ? (flags | kGoalFlag_StatusFrozen) : (flags & ~kGoalFlag_StatusFrozen);
    }
    // slot 25 (+0x64): true when the goal's own scripts suppress the
    // following goal's start scripts.
    virtual bool BlocksNextScripts() const { return false; }
    // slot 26 (+0x68)
    virtual void SetStatus(uint8_t newStatus, bool force);

    // 0x00325B58: requests the sequence streams of a script slot.
    void PrepareScriptSlot(int slot);
    // 0x00325FCC: releases the sequence streams of a script slot.
    void ReleaseScriptSlot(int slot);
    // 0x003250A0: runs the scripts of a slot in order, one at a time.
    void RunScriptSlot(int slot);
    // 0x00324F58: starts one script of slot `slot`.
    void StartScript(GoalScript& script, int slot);
    // 0x00324B08: starts a sequence (returns null) or an anim script instance.
    ape::ApeAnimScriptInstance* LaunchScript(uint32_t name, uint32_t stateName, GoalScript& script,
                                             bool asSequence, int slot);
    // 0x00325F7C: the failure scripts are done (or there are none).
    bool FailScriptsDone() const;
    // 0x00325E70: index (0-8) of checkpoint `name`, -1 if none.
    int CheckpointIndex(uint32_t checkpointName) const;
    // 0x00325EBC: index of the checkpoint the entity belongs to, -1 if none.
    int CheckpointIndexOf(const void* entity) const;

    uint8_t GetStatus() const { return status; }
    bool IsActive() const { return status == kGoal_Active; }

    // -- console members ---------------------------------------------------
    SlyGame* game = nullptr;            // +0x08
    Job* job = nullptr;                 // +0x0C
    uint8_t repeatCount = 0;            // +0x10 times a group must complete (0 = once)
    uint8_t repeatProgress = 0;         // +0x11 completions so far
    uint8_t status = kGoal_None;        // +0x12 GoalStatus
    uint32_t flags = 0;                 // +0x14 GoalFlags
    uint32_t initialFlags = 0;          // +0x18
    char name[0x80] = {};               // +0x1C
    char description[0x800] = {};       // +0x9C
    bool showDescription = false;       // +0x99C NOTE: a group shows its active child's description
    uint32_t targetName = 0;            // +0x89C
    uint32_t field_8A0 = 0;             // +0x8A0
    uint32_t progressName = 0;          // +0x8A4 GoalProgress::name of a group
    int32_t progressBase = 0;           // +0x8A8 count added to a group's progress
    uint32_t hintScriptName = 0;        // +0x8AC anim script started while the goal is active
    ape::ApeAnimScriptInstance* hintScript = nullptr; // +0x8B0
    GoalScriptCursor scripts[kScriptSlotCount];       // +0x8B8
    ape::ApeTArray<GoalHint>* hints = nullptr;         // +0x8E0
    uint32_t field_8F4[27] = {};        // +0x8F4
    uint32_t field_960 = 0;             // +0x960
    bool field_96C = false;             // +0x96C
    bool completesParent = false;       // +0x8B6 a completed child completes its group
    uint32_t checkpoints[9] = {};       // +0x970
    int32_t saveSlot = 0;               // +0x994 index of the goal's job save word
    int32_t index = -1;                 // +0x998 position in the job's goal list
    GoalBlob blobs[4];                  // +0x9B8
    int32_t cycleCount = 0;             // +0x9E8
    int32_t cycleIndex = 0;             // +0x9EC
    GoalCycleEntry* cycle = nullptr;    // +0x9F0
    float cycleTimer = 0.0f;            // +0x9F4
    float cycleFade = 0.0f;             // +0x9F8
    bool countsForGroup = true;         // +0x9FC the parent group looks at this child's result
    bool failRestarts = false;          // +0x9FD a failed group resets instead of failing
    bool targetFlag = false;            // +0x9FE
    Goal* nextGoal = nullptr;           // +0xA04 chained goal (shares the start scripts)
    Goal* parentGoal = nullptr;         // +0xA08
};

// Parse description handed from 0x00322D88 to the factories (0x2A8 bytes on
// the console). Only the members the ported code reads are modelled.
struct GoalDesc {
    SlyGame* game = nullptr;
    Job* job = nullptr;
    uint8_t repeatCount = 0;
    uint32_t flags = 0;
    const char* name = nullptr;
    const char* description = nullptr;
};

} // namespace goals
} // namespace sly
