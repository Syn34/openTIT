// openTIT - Goal classes (one per <Goal...> element of the job XML).
//
// The element names are recovered from their hashes (the XML is compiled);
// the classes are named after them. Console vtables are given per class.
#pragma once

#include "sly/game/goals/Goal.h"

#include "ape/math/ApeMath.h"

#include <memory>
#include <utility>
#include <vector>

namespace sly { namespace gameplay { class Minigame; } }

namespace sly {
namespace goals {

// "GoalWait" (vtable 0x00B1F1B0): completes after "Time" seconds.
class GoalWait : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalWait() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float remaining = 0.0f;   // +0xA0C
    float duration = 0.0f;    // +0xA10 "Time"
};

// "GoalTimer" (vtable 0x00B1F0C0): a countdown shown on the HUD; completes
// when it runs out.
class GoalTimer : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalTimer() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float remaining = 0.0f;     // +0xA0C
    float duration = 0.0f;      // +0xA10 "limit"
    bool startPaused = false;   // +0xA14 (XML 0x418A9FF0)
    bool paused = false;        // +0xA15
    bool showHud = true;        // +0xA16 (XML 0xDAC2D715)
    float warnTime = 10.0f;     // +0xA18 (XML 0xD4A5405E) the HUD flashes every second below it
};

// "GoalInput" (vtable 0x00B1F138): completes when a player presses a button
// or pushes a stick in a direction.
class GoalInput : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalInput() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;
    // 0x00340EA0: the stick of `pad` is pushed past the threshold.
    bool StickPushed(const void* pad) const;

    int32_t player = 0;        // +0xA0C (XML 0x368328E5, 0-8)
    uint32_t button = 0;       // +0xA10 "Button" action name
    int32_t direction = -1;    // +0xA14 index in the direction table, -1 for none
    float threshold = 1.0f;    // +0xA18 (XML 0x0B3E876F, clamped to 0..1)
};

// "GoalCostume" (vtable 0x00B1E7B0): the player wears a costume.
class GoalCostume : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalCostume() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    int32_t ability = 0;       // +0xA0C ability index of the costume, 0 = any
};

// "GoalBinocucomPower" (vtable 0x00B1E648): the binocucom power level is
// inside [Min, max] when event 6 arrives.
class GoalBinocucomPower : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalBinocucomPower() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;

    float minLevel = 0.0f;     // +0xA0C "Min"
    float maxLevel = 0.0f;     // +0xA10 "max"
    bool failInRange = false;  // +0xA14 (XML 0x6FE56119) fail instead of completing
};

// One entity the binocucom must keep in view (0x18 bytes on the console).
struct BinocucomRecordTarget {
    uint32_t name = 0;          // +0x00
    void* entity = nullptr;     // +0x04 resolved
    float field_08 = 0.0f;      // +0x08 (XML 0x03B880B3)
    float field_0C = 0.0f;      // +0x0C (XML 0x8E727ACC)
    float field_10 = 0.0f;      // +0x10 (XML 0x8E7279DE)
    bool visible = false;       // +0x14 NOTE: set by the binocucom when the target is in view
};

// "GoalBinocucomRecord" (vtable 0x00B1E6C0): record with the binocucom for
// a while; fails when the recording quality (event 8) drops too low.
class GoalBinocucomRecord : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalBinocucomRecord() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float failLevel = 0.0f;     // +0xA0C (XML 0x56EF5AEB)
    float remaining = 0.0f;     // +0xA10
    float duration = 0.0f;      // +0xA14 (XML 0x19E32604)
    int32_t player = 0;         // +0xA18 player of the last update
    ape::ApeTArray<BinocucomRecordTarget> targets;   // +0xA1C
};

// One attachment that must exist (0x20 bytes on the console).
struct EntityChildLink {
    ape::Vec3 offset;            // +0x00 (XML 0x4A4D4D4D)
    uint32_t parentName = 0;     // +0x10 "Name"
    void* parent = nullptr;      // +0x14 resolved
    uint32_t childName = 0;      // +0x18 (XML 0x373DE2C2)
    // NOTE: option bits from XML 0x5D28E3FC, 0x9446A097, 0xD9159FE7,
    // 0x09DA53D5, 0x04656B91, 0x310F51B2 (bits 30..25); unused by the goal.
    uint32_t options = 0;        // +0x1C
};

// "GoalEntityChild" (vtable 0x00B1E828): every listed entity has the given
// child attached.
class GoalEntityChild : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~GoalEntityChild() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    ape::ApeTArray<EntityChildLink> links;   // +0xA0C
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

enum class MinigameKind;

// Base of the goals that run a minigame (vtable 0x00B1E368): the minigame is
// built and installed in the game world's minigame slot when the goal becomes
// active; its win/lose events (0x0D/0x0E with the minigame as argument)
// complete or fail the goal on the next update.
class MinigameGoal : public Goal {
public:
    int TypeId() const override;
    using Goal::Goal;
    ~MinigameGoal() override;
    uint8_t Update(int player, const GameEvent& e) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;
    // slot 27 (+0x6C): builds the minigame and installs it in the game world.
    virtual void InstallMinigame() = 0;

    enum ResultBits : uint32_t {
        kResult_Won  = 0x80000000u,
        kResult_Lost = 0x40000000u,
    };
    gameplay::Minigame* minigame = nullptr;   // +0xA0C the minigame this goal started
    uint32_t result = 0;                      // +0xA10 ResultBits
};

// Configuration words of a minigame, read from an attribute table (the
// consumer is the minigame's constructor; the meaning of most words is not
// decoded, the table keeps the XML attribute hash of each).
struct MinigameConfig {
    static constexpr int kMaxWords = 24;
    uint32_t words[kMaxWords] = {};
};

// Base of the penguin minigame goals (vtable 0x00B1E4D0).
class PenguinGameGoal : public MinigameGoal {
public:
    using MinigameGoal::MinigameGoal;
    ~PenguinGameGoal() override;
    // slot 28 (+0x70): builds the minigame without installing it (the
    // penguin montage runs several of them); none by default.
    virtual gameplay::Minigame* BuildMinigame();
    // slot 29 (+0x74): NOTE: empty in every penguin goal; purpose unknown.
    virtual void PenguinHook();

    MinigameConfig config;   // +0xA14
};

// "GoalBentleyVsPenelope" (vtable 0x00B1E458): the Bentley vs Penelope fight.
class GoalBentleyVsPenelope : public MinigameGoal {
public:
    int TypeId() const override;
    using MinigameGoal::MinigameGoal;
    ~GoalBentleyVsPenelope() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    void InstallMinigame() override;
};

#define SLY_PENGUIN_GOAL(Name)                                                   \
    class Name : public PenguinGameGoal {                                        \
    public:                                                                      \
        using PenguinGameGoal::PenguinGameGoal;                                  \
        ~Name() override;                                                        \
        static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);    \
        int TypeId() const override;                                             \
        void InstallMinigame() override;                                         \
        gameplay::Minigame* BuildMinigame() override;                            \
    }

SLY_PENGUIN_GOAL(GoalPenguinBalanceGame);    // vtable 0x00B1E550
SLY_PENGUIN_GOAL(GoalPenguinDodgeGame);      // vtable 0x00B1EBE8
SLY_PENGUIN_GOAL(GoalPenguinPunchGame);      // vtable 0x00B1EC68
SLY_PENGUIN_GOAL(GoalPenguinSlingshotGame);  // vtable 0x00B1ECE8
SLY_PENGUIN_GOAL(GoalPenguinSumoGame);       // vtable 0x00B1ED68
// Element hash 0x618B173D (name not recovered); builds the whack-a-penguin
// minigame.
SLY_PENGUIN_GOAL(GoalPenguinWhackGame);      // vtable 0x00B1EE60

#undef SLY_PENGUIN_GOAL

// "GoalFencing" (vtable 0x00B1E918).
class GoalFencing : public MinigameGoal {
public:
    int TypeId() const override;
    using MinigameGoal::MinigameGoal;
    ~GoalFencing() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    void InstallMinigame() override;

    int32_t field_A14 = 0;       // +0xA14 (XML 0x04B8CADE)
    uint32_t field_A18 = 0;      // +0xA18 (XML 0x922D3BD1)
    float field_A1C = 4.0f;      // +0xA1C (XML 0x39FB22A9)
};

// "GoalFishingGame" (vtable 0x00B1E990).
class GoalFishingGame : public MinigameGoal {
public:
    int TypeId() const override;
    using MinigameGoal::MinigameGoal;
    ~GoalFishingGame() override;
    void InstallMinigame() override;

    // NOTE: the 0x150-byte configuration built by 0x00330C88/0x00330958
    // (not ported); kept opaque.
    uint8_t config[0x150] = {};   // +0xA20
};

// "GoalQteGame" (vtable 0x00B1EF58).
class GoalQteGame : public MinigameGoal {
public:
    int TypeId() const override;
    using MinigameGoal::MinigameGoal;
    ~GoalQteGame() override;
    void InstallMinigame() override;

    // Configuration built by 0x0033A520/0x00339BB0 (not ported) and handed
    // to the minigame as a whole: two sorted maps of owned records and an
    // array. NOTE: record contents not decoded.
    struct Step { uint32_t words[3]; };
    std::vector<std::pair<uint32_t, std::unique_ptr<Step>>> steps;        // +0xA20
    std::vector<std::pair<uint32_t, std::unique_ptr<uint32_t>>> values;   // +0xA38
    ape::ApeTArray<uint32_t> field_A50;                                    // +0xA50
};

// "GoalPenguinMontageGame" (vtable 0x00B1EEE0): a run of penguin
// minigames in rounds; completing it without losing a round awards a
// trophy.
class GoalPenguinMontageGame : public MinigameGoal {
public:
    int TypeId() const override;
    static constexpr uint32_t kTrophy_NoLoss = 0xCBB04602;
    static constexpr int kRoundCount = 6;

    using MinigameGoal::MinigameGoal;
    ~GoalPenguinMontageGame() override;
    void SetStatus(uint8_t newStatus, bool force) override;
    void InstallMinigame() override;

    // File-static flag 0x0106B62C: a round was lost during this montage.
    // NOTE: set by the montage minigame (outside this module).
    static bool s_roundLost;

    // One round (0x3C bytes on the console). NOTE: built by 0x003396A8 /
    // 0x00339388 (not ported): the sub-goals of the round and their weights.
    struct Round {
        ape::ApeTArray<uint64_t> entries;   // +0x04 (8-byte records)
        uint8_t data[0x24] = {};
    };
    uint32_t field_A14 = 0;          // +0xA14 (XML 0xCF4A1DD4)
    Round rounds[kRoundCount];       // +0xA18
};

// "GoalBartenderGame" (vtable 0x00B1E3E0).
class GoalBartenderGame : public MinigameGoal {
public:
    int TypeId() const override;
    using MinigameGoal::MinigameGoal;
    ~GoalBartenderGame() override;
    void InstallMinigame() override;

    // NOTE: drink/customer parameters built by 0x003292C8/0x00328D24 (not
    // ported); kept opaque.
    uint8_t config[0x290] = {};   // +0xA20
    struct Entry { uint32_t a, b, c; };
    ape::ApeTArray<Entry> entries;   // +0xCB0
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// Group of child goals (vtable 0x00B1E278; "GoalSet" 0x00B1E2F0 and the
// Operation Mousetrap goal derive from it). The children form a list
// through Goal::nextGoal; the group completes or fails according to `mode`.
class GoalGroup : public Goal {
public:
    int TypeId() const override;
    bool BlocksNextScripts() const override;
    enum Mode : int32_t {
        kMode_All         = 1,   // every counted child complete
        kMode_NotAll      = 2,
        kMode_Any         = 3,   // one counted child complete
        kMode_None        = 4,
        kMode_AllProgress = 5,   // like kMode_All, reports each completion to the hint script
        kMode_Sequence    = 6,   // children are activated one after the other
    };
    enum GroupFlags : uint32_t {
        kGroup_StopOnNewCompletion = 0x20000000,   // sequence: stop the pass when a child completes
        kGroup_WaitActiveScripts   = 0x40000000,   // complete only once the active scripts ran
        kGroup_NotifyAdvance       = 0x80000000,   // tell the job manager when a child starts
    };

    using Goal::Goal;
    ~GoalGroup() override;

    // 0x00321CF4: appends a child goal.
    void AddChild(Goal* child);

    uint8_t Update(int player, const GameEvent& e) override;
    void OnLevelLoaded() override;
    void Reset(bool fromSave, int status) override;
    void ResetHints(bool stopHintScript) override;
    void PrepareScripts() override;
    void ReapplyHints(Goal* owner) override;
    void SetFlags(uint32_t value) override;
    const char* GetDescription() override;
    bool GetProgress(GoalProgress* out) override;
    bool IsComplete() override;
    void CollectMarkers(GoalMarkerList* list) override;
    bool GetTarget(GoalTarget* out) override;
    uint32_t Field8A0() override;
    void CollectAllMarkers(GoalMarkerList* list) override;
    uint32_t Field8F4(int i) override;
    const char* CurrentCycleText(float* fade) override;
    void ResetCycle() override;
    void FreezeStatus(bool frozen) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    // 0x00327A58: copies the `maxMarkers` markers of `found` nearest to the
    // player into `list` (all of them if there are few enough).
    void SelectNearestMarkers(GoalMarkerList* list, const GoalMarkerList& found) const;

    int32_t mode = kMode_All;     // +0xA0C Mode
    Goal* firstChild = nullptr;   // +0xA10
    int32_t completedCount = 0;   // +0xA14
    uint32_t groupFlags = 0;      // +0xA18 GroupFlags
    uint32_t maxMarkers = 0;      // +0xA1C 0 = every child adds its own markers

private:
    void ActivateChild(Goal* child);
    void PlayChildCompleteState(int count);
};

// "GoalSet" (vtable 0x00B1E2F0): a group under another element name; it
// adds nothing to GoalGroup.
class GoalSet : public GoalGroup {
public:
    using GoalGroup::GoalGroup;
    ~GoalSet() override;
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// One entity the camera goal can point out (0x30 bytes on the console).
struct CameraEntityTarget {
    uint32_t name = 0;          // +0x00 "Name"
    void* entity = nullptr;     // +0x04 resolved (holds a reference)
    ape::Vec3 offset;           // +0x10 (XML 0x8ACA5893)
    float screenRadius = 0.75f; // +0x20 max distance from the screen centre
    float minSize = 5.0f;       // +0x24 (XML 0x8E727ACC) view width at the entity's depth
    float maxSize = 50.0f;      // +0x28
    bool binocucomOnly = false; // +0x2C (XML 0x1EE7AC93) only while the binocucom is up
};

// "CameraEntity" (vtable 0x00B1E738): while active, tells the hint script
// which listed entity the player is looking at. It never completes by itself.
class GoalCameraEntity : public Goal {
public:
    int TypeId() const override;
    static constexpr int kMaxTargets = 16;

    using Goal::Goal;
    ~GoalCameraEntity() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void SetStatus(uint8_t newStatus, bool force) override;
    // 0x0032E038: the target nearest to the view centre, or null.
    CameraEntityTarget* SelectTarget();

    float timer = 0.0f;                          // +0xA0C (XML 0x71F634BF)
    float cooldown = 15.0f;                      // +0xA10 (XML 0xDBDD1EA5)
    CameraEntityTarget targets[kMaxTargets];     // +0xA20
    int32_t targetCount = 0;                     // +0xD20
    CameraEntityTarget* current = nullptr;       // +0xD24
    bool binocucomMode = false;                  // +0xD28
};

// One interaction the goal watches (0x20 bytes on the console).
struct InteractionLink {
    ape::Vec3 offset;               // +0x00 (XML 0x4A4D4D4D)
    uint32_t name = 0;              // +0x10 entity "Name"
    void* interaction = nullptr;    // +0x14 the entity's interaction component
    bool showMarker = false;        // +0x18 bit 31 (XML 0x76E96977)
    uint32_t options = 0;           // +0x18 bits 25-30 NOTE: same option set as EntityChildLink
};

// Element hash 0xC766004C (name not recovered; vtable 0x00B1E8A0): every
// listed interaction is in the required state.
class GoalInteraction : public Goal {
public:
    int TypeId() const override;
    enum Check : uint32_t {
        kCheck_Available = 0,   // state < 2
        kCheck_State1    = 1,
        kCheck_Flag22    = 2,   // interaction field_7C bit 22
        kCheck_State3    = 3,
        kCheck_Touch     = 4,   // NOTE: calls 0x002B719C on each and counts them all
    };

    using Goal::Goal;
    ~GoalInteraction() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    uint32_t check = kCheck_Available;     // +0xA0C bits 26-31
    ape::ApeTArray<InteractionLink> links; // +0xA10
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// "GoalFollow" (vtable 0x00B1EA08): stay within range of an entity; fails
// after too long too far away, completes after long enough close by.
class GoalFollow : public Goal {
public:
    int TypeId() const override;
    enum FollowFlags : uint32_t {
        kFollow_TooFar     = 0x01000000,   // warning currently shown
        kFollow_ShowMarker = 0x80000000,   // (XML 0x76E96977)
    };

    // 0x0033162C (the factory's constructor).
    GoalFollow(const GoalDesc* desc, const float offset[3], uint32_t target, float minDistance,
               float maxDistance, float failTime, float completeTime, uint32_t options);
    ~GoalFollow() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    ape::Vec3 offset;               // +0xA10 marker offset
    uint32_t target = 0;            // +0xA20 entity to follow
    float minDistanceSq = 0.0f;     // +0xA24
    float maxDistanceSq = 0.0f;     // +0xA28
    float failTime = 0.0f;          // +0xA2C too far for longer than this fails (< 0: never)
    float completeTime = -1.0f;     // +0xA30 close for this long completes (<= 0: never)
    uint32_t followFlags = 0;       // +0xA34 FollowFlags (+ option bits 25-30)
    void* entity = nullptr;         // +0xA38
    float farTime = 0.0f;           // +0xA3C
    float nearTime = 0.0f;          // +0xA40
    float lastDistanceSq = -1.0f;   // +0xA44
};

// "GoalHealthMonitor" (vtable 0x00B1EA80; destructor outside this range):
// plays "Threshold<n>_Passed" on the hint script when an entity's health
// drops past each threshold. Never holds up its job.
class GoalHealthMonitor : public Goal {
public:
    int TypeId() const override;
    // 0x0033273C (the factory's constructor).
    GoalHealthMonitor(const GoalDesc* desc, const ape::ApeTArray<float>& thresholds,
                      uint32_t target, bool percent);
    ~GoalHealthMonitor() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    ape::ApeTArray<float> thresholds;   // +0xA0C highest first
    uint32_t target = 0;                // +0xA20
    bool percent = false;               // +0xA24 thresholds are fractions of the maximum
    void* entity = nullptr;             // +0xA28
    float lastHealth = 0.0f;            // +0xA2C
};

// Element hash 0x5F3BC8F0 (a vocabulary search matched "GoalLifoHop",
// probably a collision; vtable 0x00B1EAF8): shows a marker on an entity
// while active. Never holds up its job.
class GoalMarkEntity : public Goal {
public:
    int TypeId() const override;
    enum MarkFlags : uint32_t {
        kMark_HideWhileTargeted = 0x02000000,   // option bit 25
    };

    // 0x00332E28 (the factory's constructor).
    GoalMarkEntity(const GoalDesc* desc, uint32_t target, const float offset[3], uint32_t options,
                   uint32_t field_A10);
    ~GoalMarkEntity() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    uint32_t target = 0;        // +0xA0C
    uint32_t field_A10 = 0;     // +0xA10 (XML 0x6E6EDD6B)
    ape::Vec3 offset;           // +0xA20
    uint32_t markFlags = 0;     // +0xA30 option bits 25-31
    void* entity = nullptr;     // +0xA34
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// "OperationMousetrap" (vtable 0x00B1EB70): a goal group that also keeps
// two sets of entities targeted by the job owner's targeting gadget and
// completes once every entity of the second set is gone.
class GoalOperationMousetrap : public GoalGroup {
public:
    int TypeId() const override;
    enum Phase : int32_t {
        kPhase_None   = 0,   // nothing targeted
        kPhase_SetB   = 1,   // the second set is targeted
        kPhase_SetA   = 2,   // the first set is targeted
        kPhase_Helper = 3,   // the helper entity is targeted
    };
    // Messages that switch between the two sets.
    static constexpr uint32_t kMsg_TargetSetA = 0xF41445FE;
    static constexpr uint32_t kMsg_TargetSetB = 0x3281C093;

    // 0x00333918: a group with the given options and the two name sets.
    GoalOperationMousetrap(const GoalDesc* desc, const struct GoalGroupSetup& setup,
                           const ape::ApeTArray<uint32_t>& setA, const ape::ApeTArray<uint32_t>& setB);
    ~GoalOperationMousetrap() override;
    // 0x003344B8: reads the group options, the names "...1".."...3" (set A)
    // and "Gear1".."Gear3" (set B) and the child goals.
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    void HandleMessage(const GoalMessage& msg) override;
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    // 0x00333BC8: changes which entities are targeted.
    void SetPhase(int32_t phase, bool force);
    // 0x00334114: drops every target and entity reference.
    void Stop();
    // 0x00334954: spawns the helper entity if there is none.
    void SpawnHelper();
    // 0x00334B74: resolves the entity names and starts targeting.
    void Start();
    // 0x00334D88: per-update logic while active.
    void Tick();

    ape::ApeTArray<uint32_t> namesA;   // +0xA20
    ape::ApeTArray<uint32_t> namesB;   // +0xA34
    ape::ApeTArray<void*> entitiesA;   // +0xA48
    ape::ApeTArray<void*> entitiesB;   // +0xA5C
    void* helper = nullptr;            // +0xA70
    int32_t phase = kPhase_None;       // +0xA74
    bool started = false;              // +0xA78
    bool wantSetA = true;              // +0xA79
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// "GoalScore" (vtable 0x00B1F048): counts events of one type ("scoreType")
// and compares the score with a target.
class GoalScore : public Goal {
public:
    int TypeId() const override;
    enum Compare : uint8_t {
        kCompare_Less = 0, kCompare_AtMost = 1, kCompare_Near = 2,
        kCompare_Greater = 3, kCompare_AtLeast = 4,
    };
    enum ScoreFlags : uint32_t {
        kScore_ReportStat    = 0x10000000,   // (XML 0x58FA06CA)
        kScore_HasMax        = 0x20000000,
        kScore_HasMin        = 0x40000000,
        kScore_NotifyAdvance = 0x80000000,   // (XML 0x512CF5D9)
    };
    // An entity that is marked while it can still score (0x20 bytes).
    struct Marked {
        uint8_t pad[0x10];
        void* entity = nullptr;   // +0x10
        uint8_t pad2[0x0C];
    };
    // A scoring object (0x30 bytes; <Target> children of the goal).
    struct Source {
        ape::Vec3 offset{};      // +0x00 (XML 0x4A4D4D4D)
        uint32_t name = 0;       // +0x10 object (or child) name
        uint32_t parent = 0;     // +0x14 parent entity, 0 for none
        uint32_t group = 0;      // +0x18 compared with event arguments
        uint32_t markerDef = 0;  // +0x1C NOTE: "markerDef"
        int32_t points = 1;      // +0x20 "Count", -32..32
        uint32_t flags = 0;      // +0x24 SourceFlags
        uint8_t pad3[0x08];
    };
    enum SourceFlags : uint32_t {
        kSource_CountMissing = 0x00800000,   // a missing entity counts as scored (XML 0x5D1967C0)
        kSource_Unmark       = 0x40000000,   // drop its marker when it scores
        kSource_Marked       = 0x80000000,   // shown with a marker
    };
    // Points per event group id (8 bytes).
    struct Value {
        uint32_t group = 0;
        int32_t points = 1;
    };

    using Goal::Goal;
    ~GoalScore() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void Reset(bool fromSave, int status) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    bool GetProgress(GoalProgress* out) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    ape::ApeTArray<Marked> marked;    // +0xA0C
    ape::ApeTArray<Source> sources;   // +0xA20
    ape::ApeTArray<Value> values;     // +0xA34
    uint32_t eventName = 0;           // +0xA48 "scoreType"
    uint32_t customData = 0;          // +0xA4C "customData"
    uint8_t compare = kCompare_AtLeast; // +0xA50
    float target = 0.0f;              // +0xA54
    float tolerance = 0.0f;           // +0xA58
    float minScore = 0.0f;            // +0xA5C
    float maxScore = 0.0f;            // +0xA60
    float score = 0.0f;               // +0xA64
    uint32_t scoreFlags = 0;          // +0xA68 ScoreFlags
};

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// "GoalBinocucomPhoto" (vtable 0x00B1E5D0): photograph a set of entities
// with the binocucom. Only the target bookkeeping is ported; the camera
// test (Update 0x0032BCD0), SetStatus (0x0032AC78), the factory (0x0032B358)
// and the marker builder (0x0032AAE8) are TODO.
class GoalBinocucomPhoto : public Goal {
public:
    int TypeId() const override;
    // One photo target (0x90 bytes on the console).
    struct Target {
        uint8_t pad[0x10];
        uint32_t name = 0;        // +0x10
        void* entity = nullptr;   // +0x14
        uint8_t pad2[0x18];
        bool pending = true;      // +0x30 still to be photographed
        uint8_t pad3[0x5F];
    };

    using Goal::Goal;
    ~GoalBinocucomPhoto() override;
    uint8_t Update(int player, const GameEvent& e) override;
    void Reset(bool fromSave, int status) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void CollectMarkers(GoalMarkerList* list) override;
    void CollectAllMarkers(GoalMarkerList* list) override;
    void SetStatus(uint8_t newStatus, bool force) override;
    void BuildMarker(const Target& target, GoalMarker* marker) const;

    ape::ApeTArray<Target> targets;   // +0xA0C

private:
    // Resolves every target; fails the goal if one is gone. False if it failed.
    bool ResolveTargets(bool pending);
};

} // namespace goals
} // namespace sly
