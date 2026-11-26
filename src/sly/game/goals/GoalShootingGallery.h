// openTIT - "ShootingGallery" goal (vtable 0x00B1EFD0, 0xB00 bytes): runs
// spawner groups through a command list (enable a group with spawn limits,
// pause) and pops up score texts over targets that report a score.
#pragma once

#include "sly/game/goals/Goal.h"

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

namespace sly {
class Spawner;
class SpawnGroup;
namespace goals {

class GoalShootingGallery : public Goal {
public:
    // One target slot of a group (12 bytes): the spawner whose target is up,
    // or the delay before the slot spawns again.
    struct Slot {
        Spawner* spawner = nullptr;   // +0x00
        float delay = 0.0f;           // +0x04
        bool justFreed = false;       // +0x08 its target went away this update
    };

    // A spawner group driven by the gallery (0x9C bytes).
    struct Group {
        enum State : int32_t { kState_Idle = 0, kState_Running = 1, kState_Stopping = 2 };

        char name[0x20] = {};               // +0x00 "Name" text
        uint32_t nameHash = 0;              // +0x20 "Name" (the spawn group)
        int32_t spawned = 0;                // +0x24 targets spawned since the group started
        uint32_t commandName = 0;           // +0x28 copied from the enabling command
        int32_t minActive = 0;              // +0x2C
        int32_t maxActive = 0;              // +0x30
        int32_t maxSpawn = 0;               // +0x34 0 = unlimited (see SpawnInto)
        float delayBetweenMin = 0.0f;       // +0x38
        float delayBetweenMax = 0.0f;       // +0x3C
        int32_t state = kState_Idle;        // +0x40 State
        SpawnGroup* spawnGroup = nullptr;   // +0x44
        ape::ApeTArray<Slot*> slots;        // +0x48 slots in use
        ape::ApeTArray<Slot*> freeSlots;    // +0x5C
        ape::ApeTArray<Spawner*> busy;      // +0x70 spawners with a target up
        ape::ApeTArray<Spawner*> available; // +0x84
        float stateTime = 0.0f;             // +0x98

        Group();
        ~Group();
        // 0x0033C03C: forgets the spawn group and frees every slot.
        void Clear();
        // 0x0033C130: spawns a target into a free slot from a random spawner.
        void SpawnInto(Slot* slot);
        // 0x0033C2C0
        void SetState(int newState);
        // 0x0033C5B4: binds the spawn group and starts in the stopping state.
        void Start();
        // 0x0033C7A0
        void Update(float dt);
        // 0x0033CB98: a target of `spawner` went away; the spawner becomes
        // available again and its slot waits for a new delay.
        void OnTargetGone(Spawner* spawner, bool flag);
        int ActiveCount() const;
        Slot* FirstReadySlot() const;
    };

    // One entry of <Commands> (0x24 bytes).
    struct Command {
        enum Type : int32_t { kEnableGroup = 1, kPause = 2 };
        int32_t type = 0;                   // +0x00
        uint32_t group = 0;                 // +0x04 "Name"
        int32_t minActive = 0;              // +0x08 "minActive"
        int32_t maxActive = 0;              // +0x0C "maxActive"
        int32_t maxSpawn = 0;               // +0x10 NOTE: hash matches "MaxSpawn"
        float delayBetweenMin = 0.0f;       // +0x14 "delayBetweenMin"
        float delayBetweenMax = 0.0f;       // +0x18 "delayBetweenMax"
        float time = 0.0f;                  // +0x1C "time" (Pause)
        bool enable = false;                // +0x20 "Enable" (EnableGroup)

        // 0x0033BEA4: the group parameters of an <EnableGroup>.
        void ParseGroupParams(ape::xml::XmlElement* xml);
    };

    // A score source (0x20 bytes): targets of this entity group pop up their
    // score at an offset.
    struct ScoreSource {
        uint32_t group = 0;                 // +0x00 "Name"
        ape::Vec3 popupOffset{};            // +0x10 NOTE: hash matches "PopupOffset"
    };

    // A rising score text (0x90 bytes). Only the fields the update moves
    // are named; the rest is filled by 0x0033E2C0 (not ported).
    struct ScorePopup {
        uint8_t data_00[0x44] = {};
        float height = 0.0f;                // +0x44
        uint8_t data_48[0x08] = {};
        float lifetime = 0.0f;              // +0x50
        float remaining = 0.0f;             // +0x54
        float speed = 0.0f;                 // +0x58
        float acceleration = 0.0f;          // +0x5C
        uint8_t data_60[0x07] = {};
        uint8_t alpha = 0;                  // +0x67
        uint8_t data_68[0x28] = {};
    };

    explicit GoalShootingGallery(const GoalDesc* desc);
    ~GoalShootingGallery() override;
    // 0x0033DC0C
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    // 0x0033D4E4
    void Parse(ape::xml::XmlElement* xml);
    int TypeId() const override;
    void HandleMessage(const GoalMessage& msg) override;
    uint8_t Update(int player, const GameEvent& e) override;
    void OnLevelLoaded() override;
    void SetStatus(uint8_t newStatus, bool force) override;

    // 0x0033DE30: moves the score popups and drops the expired ones.
    void UpdatePopups(float dt);
    void ClearPopups();
    // 0x0033E2C0: a target scored `score` points.
    void AddScorePopup(void* entity, int score, const ape::Vec3& offset);
    // 0x0033B9F8
    void DrawPopup(const ScorePopup& popup);

    ape::ApeTArray<Group*> groups;          // +0xA0C
    ape::ApeTArray<Command*> commands;      // +0xA20
    uint32_t commandIndex = 0;              // +0xA34
    float pauseTimer = 0.0f;                // +0xA38
    // Popup motion: XML 0x1E3F7010, 0xB6BFACEE, 0xF00CE6B3, 0x2AF63B94.
    float popup_A3C = 1.0f;                 // +0xA3C
    float popup_A40 = 1.0f;                 // +0xA40
    float popup_A44 = 0.0f;                 // +0xA44
    float popup_A48 = 0.0f;                 // +0xA48
    char positiveText[0x40] = {};           // +0xA4C GUI object for positive scores (XML 0xB8B3BE3A)
    char negativeText[0x40] = {};           // +0xA8C GUI object for negative scores (XML 0xB8B3B576)
    ape::xml::Color8 positiveColor{};       // +0xACC XML 0x0800836B
    ape::xml::Color8 negativeColor{};       // +0xAD0 XML 0x08007AA7
    ape::ApeTArray<ScorePopup*> popups;     // +0xAD4
    ape::ApeTArray<ScoreSource> scoreSources;   // +0xAE8
    bool popupsEnabled = false;             // +0xAFC XML 0xBA4D5487
};

} // namespace goals
} // namespace sly
