// openTIT - The level's active game-mode object (game world +0x1281F4).
//
// Minigames such as the bartender game install themselves here; the HUD and
// the sequencer talk to whichever one is active. Only the members used by
// the gameplay code reconstructed so far are modelled.
#pragma once

#include "sly/gameplay/StateMachine.h"

#include <cstdint>

namespace sly {

class WorldObject;
class SlyGame;
namespace goals { struct GoalMarkerList; }

namespace gameplay {

// Bits of Minigame::resetRequests, consumed by the minigame's update.
// NOTE: names are guesses from the script commands that set them.
enum MinigameRequest : uint32_t {
    kMinigameRequest_Score = 0x2,
    kMinigameRequest_Timer = 0x4,
    kMinigameRequest_All   = 0xF,
};

// Base of every minigame (vtable 0x00B21AA0; constructor 0x0047C5D8 builds
// a state machine of four layers with 0x100 bytes of state data each).
// Slots 10-21 are the minigame interface; their names are guesses where the
// callers are not ported.
class Minigame : public StateMachine {
public:
    Minigame() : StateMachine(4, 0x100, 0) {}
    // Slot 9: messages are accepted and ignored by default.
    // @func 0x0077D8F8 Minigame::OnMessage
    bool OnMessage(const Message& message) override
    {
        (void)message;
        return true;
    }
    // @func 0x0077D8F4 Minigame::Slot10
    virtual void Slot10() {}
    // @func 0x0077D900 Minigame::Slot11
    virtual bool Slot11() { return true; }
    // @func 0x0077D908 Minigame::Slot12
    virtual void Slot12() {}
    // Slot 13: adds the minigame's markers to a goal's marker list.
    // @func 0x0077D90C Minigame::CollectMarkers
    virtual void CollectMarkers(goals::GoalMarkerList* list) { (void)list; }
    // @func 0x0077D910 Minigame::Slot14
    virtual bool Slot14() { return true; }
    // @func 0x0077D918 Minigame::Slot16
    virtual bool Slot16() { return true; }
    // @func 0x0077D920 Minigame::Slot17
    virtual bool Slot17() { return true; }
    // @func 0x0077D928 Minigame::Slot18
    virtual void Slot18() {}
    // Slot 19: the game the minigame was built for (+0x30).
    // @func 0x0077D92C Minigame::GetGame
    virtual SlyGame* GetGame() const { return game; }
    // Slots 20/21 are pure on the console. Slot 20 starts the minigame over.
    virtual void Reset() {}
    virtual void Slot21() {}

    SlyGame* game = nullptr;           // +0x30
    uint8_t field_36 = 0;              // +0x36 (see PenguinMinigame slots 29/30)

    // NOTE: the members below belong to the bartender minigame (0x003A4000..);
    // they stay here until that class is ported.
    // 0x003A4010
    void ClearRequests();
    // 0x003A4000 / 0x003AAAE0 / 0x003AAAF0
    void RequestFullReset();
    void RequestScoreReset();
    void RequestTimerReset();
    // 0x003AAB08: replays the intro animation of the minigame's GUI.
    void PlayIntro();
    // 0x003AAB54: wakes the player entity of the minigame up.
    void StartPlayer();

    // field_1C bit 6: the minigame accepts sequencer control.
    bool AcceptsScriptControl() const { return (flags1C & 0x40u) != 0; }

    uint32_t flags1C = 0;              // +0x1C
    void* introGui = nullptr;          // +0xF4
    WorldObject* player = nullptr;     // +0xF8
    // NOTE: cleared when the Bentley alter ego dies while flags1C bit 5 is set.
    uint8_t field_105 = 0;             // +0x105
    uint32_t resetRequests = 0;        // +0x1A8 MinigameRequest bits
};

} // namespace gameplay
} // namespace sly
