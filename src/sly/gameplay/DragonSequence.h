// openTIT - Dragon boss attack sequences (0x002F7D60-0x002FE634).
//
// The dragon boss definition (vtable 0x00B1D5C8) loads "phases" from XML;
// a phase is a list of collections, a collection a set of threads that run
// in parallel, and a thread an ordered list of commands (wait, play an
// attack on a body part, switch a part's state, enable another thread,
// start the next collection). Commands drive "actions" on the dragon's body
// parts (the three heads, the root and the laser).
//
// The dragon entity itself (vtable 0x00B1D7C0, 0x470 bytes) is not ported;
// DragonBoss models the members the sequencer uses.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }
namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace gameplay {

class DragonBoss;
struct DragonPhase;
struct DragonThread;

// One animated body part of the dragon (0x48 bytes; constructor 0x00767C20
// outside this range).
struct DragonBodyPart {
    uint32_t field_24 = 0;                          // +0x24 NOTE: bone/part id handed to effects
    uint32_t field_28 = 0;                          // +0x28
    ape::ApeAnimScriptInstance* script = nullptr;   // +0x34
    DragonBoss* dragon = nullptr;                   // +0x38
    bool available = false;                         // +0x44 not used by a thread right now
    bool alive = true;                              // +0x45
    bool busy = false;                              // +0x46
    bool field_47 = false;                          // +0x47

    // 0x002F9424: sets the part's script state and forwards it to the
    // head's attached models.
    void SetState(uint32_t state);
    // 0x002F9540: same, then advances the script by a tenth of a second.
    void SetStateAndStep(uint32_t state);
};

// The dragon entity members used here.
class DragonBoss {
public:
    static constexpr int kHeadCount = 3;

    void* worldObject = nullptr;              // +0xA0 the entity's world object
    int32_t mode = 0;                         // +0x1B0 (1: the head models get states too)
    DragonPhase* currentPhase = nullptr;      // +0x1BC
    DragonBodyPart* randomPartA = nullptr;    // +0x1C4 part picked for random threads 0x0416F651
    DragonBodyPart* randomPartB = nullptr;    // +0x1C8 part picked for random threads 0x0416F652
    bool field_1F0 = false;                   // +0x1F0
    DragonBodyPart root;                      // +0x21C
    DragonBodyPart laser;                     // +0x264
    DragonBodyPart* heads[kHeadCount] = {};   // +0x384
    // Per head: two attached models that receive the head's states
    // (+0x394 + i*8, +0x398 + i*8; the first only in mode 1).
    void* headModelA[kHeadCount] = {};
    void* headModelB[kHeadCount] = {};

    int HeadIndex(const DragonBodyPart* part) const;
};

// ---------------------------------------------------------------------------
// Actions

// Base of the actions a command runs on a body part (vtable 0x00B1D600).
class DragonAction {
public:
    DragonAction(DragonBoss* dragon, DragonBodyPart* part) : dragon(dragon), part(part) {}
    virtual ~DragonAction();
    // slot 2: starts the part's animation at `speed`.
    virtual void Start();
    // slot 3: releases the part; with `reset` it goes back to idle.
    virtual void Stop(bool reset);
    // slot 4
    virtual void Update(float dt);
    // slot 5: a projectile reported `event` (no-op by default).
    // @func 0x007684D0 DragonAction::OnProjectileEvent
    virtual void OnProjectileEvent(void* projectile, uint32_t event) { (void)projectile; (void)event; }
    // slot 6
    virtual bool IsDone();

    DragonBoss* dragon = nullptr;   // +0x04
    DragonBodyPart* part = nullptr; // +0x08
    float speed = 1.0f;             // +0x0C
};

// Plays one script state on the part (vtable 0x00B1D678, 0x14 bytes).
class DragonStateAction : public DragonAction {
public:
    DragonStateAction(DragonBoss* dragon, DragonBodyPart* part, uint32_t state)
        : DragonAction(dragon, part), state(state) {}
    // @func 0x007681C4 DragonStateAction::~DragonStateAction
    // @func 0x007684D4 DragonStateAction::~DragonStateAction (deleting)
    ~DragonStateAction() override = default;
    void Start() override;
    void Update(float dt) override;

    uint32_t state = 0;   // +0x10
};

// Flame / wind attack (vtable 0x00B1D628, 0x30 bytes).
class DragonFlameAction : public DragonAction {
public:
    DragonFlameAction(DragonBoss* dragon, DragonBodyPart* part, int32_t kind, float a, float b)
        : DragonAction(dragon, part), kind(kind), field_1C(a), field_20(b) {}
    ~DragonFlameAction() override;
    void Start() override;
    void Update(float dt) override;
    bool IsDone() override;
    // slot 7: stops and drops the effect script.
    virtual void Release();
    // 0x002FA3F0: starts the flame effect script.
    void StartEffect();

    int32_t phase = 0;        // +0x10
    bool field_14 = false;    // +0x14
    int32_t kind = 0;         // +0x18 action type (8 start, 9 flame, 10 end)
    float field_1C = 0.0f;    // +0x1C
    float field_20 = 0.0f;    // +0x20
    float field_24 = 0.0f;    // +0x24
    uint32_t field_28 = 0;    // +0x28
    ape::ApeAnimScriptInstance* effect = nullptr;   // +0x2C
};

// Homing fireballs (vtable 0x00B1D650, 0x50 bytes).
class DragonHomingAction : public DragonAction {
public:
    enum Phase : int32_t { kPhase_Idle = 0, kPhase_Firing = 1, kPhase_Done = 2 };

    DragonHomingAction(DragonBoss* dragon, DragonBodyPart* part, uint32_t target);
    ~DragonHomingAction() override;
    void Start() override;
    void Update(float dt) override;
    void OnProjectileEvent(void* projectile, uint32_t event) override;
    bool IsDone() override;
    // 0x002FC600: a projectile hit (1) or missed (2).
    void OnProjectileResult(int result);

    uint32_t target = 0;                    // +0x10 attack state / target
    int32_t phase = kPhase_Idle;            // +0x14
    ape::ApeTArray<void*> projectiles;      // +0x18
    void* field_40 = nullptr;               // +0x40
};

// ---------------------------------------------------------------------------
// Commands

// Base of the thread commands (vtable 0x00B1D6A0).
class DragonCommand {
public:
    DragonCommand(DragonThread* thread, const char* name);
    // @func 0x00768518 DragonCommand::~DragonCommand
    // @func 0x00768528 DragonCommand::~DragonCommand (deleting)
    virtual ~DragonCommand() = default;
    // slot 2
    virtual void Parse(ape::xml::XmlElement* xml);
    // slot 3: `rearm` lets a fire-once command run again.
    virtual void Reset(bool rearm);
    // slot 4: returns true when the command is complete right away.
    virtual bool Start(DragonBodyPart* part);
    // slot 5
    virtual void Update(float dt) = 0;
    // slot 6
    // @func 0x0076856C DragonCommand::OnProjectileEvent
    virtual void OnProjectileEvent(void* projectile, uint32_t event) { (void)projectile; (void)event; }
    // slot 7
    virtual bool IsDone() = 0;
    // slot 8: kind of command (0 for the base).
    // @func 0x00768570 DragonCommand::Kind
    virtual int Kind() const { return 0; }

    DragonThread* thread = nullptr;   // +0x04
    char name[0x20] = {};             // +0x08
    bool fireOnce = false;            // +0x28
    bool fired = false;               // +0x29
};

// "Wait" (vtable 0x00B1D6D0): waits "Time" seconds.
class DragonWaitCommand : public DragonCommand {
public:
    using DragonCommand::DragonCommand;
    // @func 0x007681D4 DragonWaitCommand::~DragonWaitCommand
    // @func 0x00768580 DragonWaitCommand::~DragonWaitCommand (deleting)
    ~DragonWaitCommand() override = default;
    // @func 0x00768578 DragonWaitCommand::Kind
    int Kind() const override { return 1; }
    void Parse(ape::xml::XmlElement* xml) override;
    void Reset(bool rearm) override;
    bool Start(DragonBodyPart* part) override;
    void Update(float dt) override;
    bool IsDone() override;

    float duration = 0.0f;    // +0x2C
    float remaining = 0.0f;   // +0x30
};

// "StartNextCollection" (vtable 0x00B1D700).
class DragonNextCollectionCommand : public DragonCommand {
public:
    using DragonCommand::DragonCommand;
    // @func 0x007681E4 DragonNextCollectionCommand::~DragonNextCollectionCommand
    // @func 0x007685CC DragonNextCollectionCommand::~DragonNextCollectionCommand (deleting)
    ~DragonNextCollectionCommand() override = default;
    // @func 0x007685C4 DragonNextCollectionCommand::Kind
    int Kind() const override { return 2; }
    void Parse(ape::xml::XmlElement* xml) override;
    void Reset(bool rearm) override;
    bool Start(DragonBodyPart* part) override;
    void Update(float dt) override;
    bool IsDone() override;
};

// Prime/UnPrime/Rumble/Fireball*/Flame* (vtable 0x00B1D730): runs an action.
class DragonActionCommand : public DragonCommand {
public:
    enum ActionType : int32_t {
        kAction_Prime = 1, kAction_UnPrime = 2, kAction_Rumble = 3,
        kAction_FireballA = 4, kAction_FireballB = 5, kAction_FireballC = 6,
        kAction_FireballHoming = 7, kAction_FlameStart = 8, kAction_Flame = 9,
        kAction_FlameEnd = 10,
    };

    DragonActionCommand(DragonThread* thread, const char* name, ActionType type)
        : DragonCommand(thread, name), type(type) {}
    ~DragonActionCommand() override;
    // @func 0x00768610 DragonActionCommand::Kind
    int Kind() const override { return 3; }
    void Parse(ape::xml::XmlElement* xml) override;
    void Reset(bool rearm) override;
    bool Start(DragonBodyPart* part) override;
    void Update(float dt) override;
    void OnProjectileEvent(void* projectile, uint32_t event) override;
    bool IsDone() override;

    ActionType type = kAction_Prime;   // +0x2C
    DragonAction* action = nullptr;    // +0x30
    bool done = false;                 // +0x34
    bool field_35 = false;             // +0x35
    float flameA = 0.0f;               // +0x38 (XML 0x771571A7)
    float flameB = 0.0f;               // +0x3C (XML 0x5AC9E44E)
    uint32_t homingState = 0;          // +0x40 (XML 0x9178C4CB)
};

// "Wind" / "Laser" / "SpawnBabyDragons" (vtable 0x00B1D760): plays a state
// on a body part.
class DragonPartStateCommand : public DragonCommand {
public:
    using DragonCommand::DragonCommand;
    // @func 0x007681F4 DragonPartStateCommand::~DragonPartStateCommand
    // @func 0x00768620 DragonPartStateCommand::~DragonPartStateCommand (deleting)
    ~DragonPartStateCommand() override = default;
    // @func 0x00768618 DragonPartStateCommand::Kind
    int Kind() const override { return 4; }
    void Parse(ape::xml::XmlElement* xml) override;
    void Reset(bool rearm) override;
    bool Start(DragonBodyPart* part) override;
    void Update(float dt) override;
    bool IsDone() override;

    uint32_t state = 0;                // +0x2C (XML 0x9178C4CB)
    DragonBodyPart* target = nullptr;  // +0x30
};

// "EnableThread" (vtable 0x00B1D790).
class DragonEnableThreadCommand : public DragonCommand {
public:
    using DragonCommand::DragonCommand;
    // @func 0x00768204 DragonEnableThreadCommand::~DragonEnableThreadCommand
    // @func 0x0076866C DragonEnableThreadCommand::~DragonEnableThreadCommand (deleting)
    ~DragonEnableThreadCommand() override = default;
    // @func 0x00768664 DragonEnableThreadCommand::Kind
    int Kind() const override { return 5; }
    void Parse(ape::xml::XmlElement* xml) override;
    void Reset(bool rearm) override;
    bool Start(DragonBodyPart* part) override;
    void Update(float dt) override;
    bool IsDone() override;

    uint32_t threadName = 0;   // +0x2C "Name"
};

// ---------------------------------------------------------------------------
// Threads, collections, phases

struct DragonCollection;

// 0x58 bytes on the console.
struct DragonThread {
    DragonCollection* collection = nullptr;   // +0x00
    char name[0x20] = {};                     // +0x04
    uint32_t nameHash = 0;                    // +0x24
    uint32_t bodyPart = 0;                    // +0x28 (XML 0x9C939FA0)
    bool randomPart = false;                  // +0x2C (XML 0x98D03360)
    DragonBodyPart* part = nullptr;           // +0x30
    bool enabled = true;                      // +0x34
    bool startEnabled = true;                 // +0x35 (XML 0xCADB1721)
    uint32_t commandIndex = 0;                // +0x38
    DragonCommand* current = nullptr;         // +0x3C
    ape::ApeTArray<DragonCommand*> commands;  // +0x40
    bool finished = false;                    // +0x54

    // 0x002F7D60
    void Parse(ape::xml::XmlElement* xml);
    // 0x002FCD4C
    void Update(float dt);
};

// 0x3C bytes on the console.
struct DragonCollection {
    char name[0x20] = {};                     // +0x00
    DragonPhase* phase = nullptr;             // +0x20
    ape::ApeTArray<DragonThread*> threads;    // +0x24
    bool waitForScripts = false;              // +0x38
    bool done = false;                        // +0x39
    bool running = false;                     // +0x3A

    // 0x002F892C
    void Parse(ape::xml::XmlElement* xml);
    // 0x002FCC8C: every enabled thread's part script has finished.
    bool ScriptsFinished() const;
    // 0x002FD0AC
    void Update(float dt);
    // 0x002FD1B0
    void Begin();
};

// 0x50 bytes on the console.
struct DragonPhase {
    char name[0x20] = {};                       // +0x00
    int32_t field_20 = 0;                       // +0x20 (XML 0x0489115B)
    int32_t field_24 = 0;                       // +0x24 (XML 0xBE8976CD)
    uint32_t collectionIndex = 0;               // +0x2C
    DragonCollection* current = nullptr;        // +0x30
    ape::ApeTArray<DragonCollection*> collections;   // +0x34
    bool advance = false;                       // +0x48 set by StartNextCollection
    DragonBoss* dragon = nullptr;               // +0x4C

    // 0x002F8AFC
    void Parse(ape::xml::XmlElement* xml);
    // 0x002FD2A8
    void Update(float dt);
    // 0x002FD414
    void ForwardProjectileEvent(void* projectile, uint32_t event);
    // 0x002F6054: frees the collections, threads and commands.
    void Destroy();
    // 0x002F92C0: rewinds every collection and thread.
    void Reset(bool rearm);
};

// Hooks into the (unported) dragon entity / projectile code.
// Head model virtual slot 32 (+0x80): plays a state.
void DragonModelSetState(void* model, uint32_t state);
// Sets an object variable of a script instance (reference counted).
void DragonSetScriptObject(ape::ApeAnimScriptInstance* script, uint32_t var, void* object);

} // namespace gameplay
} // namespace sly
