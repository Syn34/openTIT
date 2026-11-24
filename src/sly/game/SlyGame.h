// openTIT - The game world object (0x1286B0 bytes on PS3, global pointer at
// 0x00F65CA0). Only the members the game-shell code touches are modelled;
// their console offsets are kept in comments.
#pragma once

#include <cstdint>

namespace sly {

class WorldObject;   // world entities, reconstructed by the world module
class SpawnManager;

// Small clock block the game points at from +0x0C; the first word is used
// as the timestamp of game events. NOTE: probably a frame or tick counter.
struct GameClock {
    uint32_t now = 0;
};

// Per-frame list cleared at the end of every render frame (+0x1284EC,
// stride 0x14, seven of them).
struct FrameList {
    void* items = nullptr;
    uint16_t capacity = 0;
    uint16_t count = 0;
    uint32_t field_08 = 0;
    uint32_t field_0C = 0;
    uint32_t field_10 = 0;
};

class SlyGame {
public:
    static constexpr int kFrameListCount = 7;

    // Resets the per-frame lists filled during rendering.
    void ResetFrameLists();

    // Timestamp stamped into every game event.
    uint32_t EventTime() const { return clock ? clock->now : 0; }

    GameClock* clock = nullptr;                 // +0x0C
    uint8_t localPlayerCount = 1;               // +0x1D
    uint8_t stateFlags = 0;                     // +0x2C NOTE: bit 5/6 toggled by GUI code
    WorldObject* playerObject = nullptr;        // +0x1078 the controlled player character
    SpawnManager* spawnManager = nullptr;       // NOTE: owner of the spawner groups; slot unknown
    FrameList frameLists[kFrameListCount];      // +0x1284EC
};

// The single game world (0x00F65CA0).
extern SlyGame* g_game;

} // namespace sly
