// openTIT - The level's active game-mode object (game world +0x1281F4).
//
// Minigames such as the bartender game install themselves here; the HUD and
// the sequencer talk to whichever one is active. Only the members used by
// the gameplay code reconstructed so far are modelled.
#pragma once

#include <cstdint>

namespace sly {

class WorldObject;

namespace gameplay {

// Bits of Minigame::resetRequests, consumed by the minigame's update.
// NOTE: names are guesses from the script commands that set them.
enum MinigameRequest : uint32_t {
    kMinigameRequest_Score = 0x2,
    kMinigameRequest_Timer = 0x4,
    kMinigameRequest_All   = 0xF,
};

class Minigame {
public:
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
    uint32_t resetRequests = 0;        // +0x1A8 MinigameRequest bits
};

} // namespace gameplay
} // namespace sly
