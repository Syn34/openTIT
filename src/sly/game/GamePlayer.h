// openTIT - Per-player block of the game world (SlyGame +0x1070, nine blocks
// of 0x20C80 bytes on the console; constructor 0x0045C868).
//
// Each block holds the player's character, its spawn transform, the goals
// of the running job, a regenerating meter and the log of game events the
// player has seen (a 4096-entry ring, plus a 64-entry ring that buffers
// events while a queued cutscene plays).
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"
#include "sly/game/GameEvent.h"

#include <cstdint>

namespace sly {

namespace entity { class Entity; }
namespace goals { class Goal; }

// Ring buffer of game events: pushing onto a full ring drops the oldest
// entry (the read index is moved past the slot just written).
template <uint16_t N>
struct GameEventRing {
    static_assert((N & (N - 1)) == 0, "ring sizes are powers of two");

    GameEvent events[N];
    uint16_t write = 0;
    uint16_t read = 0;

    void Push(const GameEvent& e)
    {
        events[write] = e;
        write = uint16_t((write + 1) % N);
        if (write == read)
            read = uint16_t((read + 1) % N);
    }
    bool Empty() const { return write == read; }
    void Clear()
    {
        for (GameEvent& e : events)
            e = GameEvent();
        write = 0;
        read = 0;
    }
};

struct GamePlayer {
    // Player block flag bits (+0x03).
    enum : uint8_t {
        // The meter does not regenerate this frame (cleared by UpdateMeter).
        kFlag_MeterHold = 0x40,
        kFlag_80 = 0x80,
    };

    GamePlayer();

    // Power back to its tuning defaults, full (0x0045C81C).
    void ResetMeter();
    // Releases the character (its virtual Release) and forgets it.
    void ReleaseCharacter();
    // Empties both event logs (0x0045C7A8).
    void ClearEventLogs();
    // Recharges the power meter by dt (0x00465CB8): rate +0x68 (a character
    // with the "FastRecharge" upgrade uses the fast rate instead), capped at
    // the maximum +0x64. Skipped once while kFlag_MeterHold is set; the flag
    // is cleared either way.
    void UpdateMeter(float dt);
    // The running job's goal named `name` (Goal::nameHash), or null
    // (0x00465F30).
    goals::Goal* FindGoal(uint32_t name) const;

    uint8_t index = 0xFF;                   // +0x00 NOTE: game player slot
    uint8_t padIndex = 0xFF;                // +0x01 app player (pad) driving it
    uint8_t field_02 = 0xFF;                // +0x02
    uint8_t flags = 0;                      // +0x03
    uint32_t jobStateBits = 0;              // +0x04 bits 20-31 are per-job state, cleared with the goals
    entity::Entity* character = nullptr;    // +0x08 the controlled player character
    uint32_t field_0C = 0;                  // +0x0C
    ape::Matrix34 spawnTransform;           // +0x10 NOTE: where the player was placed last
    ape::ApeTArray<goals::Goal*> goals;     // +0x50 the running job's goals
    // The player's power ("Game.Player.Power.Max" / ".Recharge").
    float meterMax = 0.0f;                  // +0x64
    float meterRate = 0.0f;                 // +0x68
    float meter = 0.0f;                     // +0x6C
    GameEventRing<4096> eventLog;           // +0x70 (indices +0x20070/+0x20072)
    uint8_t field_20074[0x400] = {};        // +0x20074 NOTE: cleared with the log, not read here
    GameEventRing<64> deferredEvents;       // +0x20474 (indices +0x20C74/+0x20C76)
};


} // namespace sly
