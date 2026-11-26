// openTIT - The game world object (0x1286B0 bytes on PS3, global pointer at
// 0x00F65CA0). Only the members the game-shell code touches are modelled;
// their console offsets are kept in comments.
#pragma once

#include "ape/memory/ApeTArray.h"

namespace ape { class ApeLevelDef; }

#include <cstdint>

namespace sly {

class WorldObject;   // world entities, reconstructed by the world module
class SpawnManager;
namespace goals { class Goal; }
namespace gameplay { class Minigame; }
namespace entity { class Entity; }

// Small clock block the game points at from +0x0C; the first word is used
// as the timestamp of game events. NOTE: probably a frame or tick counter.
struct GameClock {
    uint32_t now = 0;
};

// A deferred call queued on one of the world callback lists: the game
// calls function(owner, context) when it runs the list.
// NOTE: on the console 'function' is a function descriptor; the callers
// pass static functions of their modules.
struct WorldCallback {
    void* function = nullptr;
    void* context = nullptr;
    entity::Entity* owner = nullptr;
};

class SlyGame {
public:
    // Thirteen callback lists (+0x128480, 0x14-byte arrays). Lists 5..11
    // only live for one frame: they are emptied at the end of every render
    // frame.
    static constexpr int kCallbackListCount = 13;
    static constexpr int kFirstFrameList = 5;
    static constexpr int kFrameListCount = 7;

    // Resets the per-frame lists filled during rendering.
    void ResetFrameLists();
    // Queues function(owner, context) on callback list 'list'.
    void AddCallback(int list, void* function, void* context, entity::Entity* owner);
    // Removes every entry of list 'list' with this context and owner.
    // NOTE: the console passes the function too but does not compare it.
    void RemoveCallback(int list, void* context, entity::Entity* owner);
    // Drops every callback owned by 'owner' from all the lists (entity
    // teardown).
    void ForgetEntity(entity::Entity* owner);
    // Calls every callback of list 'list' (in the order queued); with
    // 'clear' the list is emptied afterwards.
    void RunCallbacks(int list, float dt, bool clear);

    // -- the level's game mode (+0x1281F4) ------------------------------------
    // Stops and destroys the running minigame, then installs 'next' (may be
    // null). The new minigame is not started.
    void SetMinigame(gameplay::Minigame* next);
    // Stops and destroys the running minigame.
    void EndMinigame();
    // Shows or hides the HUD through the running minigame (if any).
    void SetMinigameHudVisible(bool visible);
    // Player meter of the HUD. NOTE: the console passes the player index;
    // the HUD keeps a single player meter, so it is unused.
    void SnapPlayerMeter(uint32_t player);
    void SetPlayerMeterState(uint32_t player, bool on);
    // Snaps the HUD meter that follows 'owner' to its current value.
    void SnapMeterOf(const entity::Entity* owner);
    // Requests a reload of the running level at the next update (state bit
    // 0x40); 'flag' goes to state bit 0x20. NOTE: the flag's meaning is not
    // known (the job manager and the level script command pass false).
    void RequestLevelRestart(bool flag);
    // Name of the running level ("" before a level is loaded).
    const char* CurrentLevelName() const;

    // Timestamp stamped into every game event.
    uint32_t EventTime() const { return clock ? clock->now : 0; }

    // -- small accessors of the game update (0x00787830-0x00787D5C) ------------
    // Sets or clears bits of the world flags (+0x28; bit 1 HUD hidden, bit 2
    // input locked, see gameplay::WorldFlags).
    // @func 0x00787B80 SlyGame::SetWorldFlags
    void SetWorldFlags(uint32_t mask, bool on) { worldFlags = on ? (worldFlags | mask) : (worldFlags & ~mask); }
    // The sub-state word (+0x48) only counts while +0x2C bit 7 is set.
    // NOTE: names guessed from the update's use (values 0-3).
    // @func 0x00787914 SlyGame::SubState
    uint32_t SubState() const { return (stateFlags & 0x80) ? subState : 0; }
    // @func 0x00787938 SlyGame::SetSubState
    void SetSubState(uint32_t state)
    {
        if (stateFlags & 0x80)
            subState = state;
    }
    // +0x1282A0 bit 30 clear. NOTE: meaning unknown.
    // @func 0x00787830 SlyGame::Flag1282A0Clear
    bool Flag1282A0Clear() const { return (flags1282A0 & 0x40000000u) == 0; }
    // Game speed: the two scales (+0x128588, +0x128594), with the slow-motion
    // factor (+0x1282AC) while it is active (+0x1282BC bit 31).
    // @func 0x007877D0 SlyGame::TimeScale
    float TimeScale() const
    {
        float scale = timeScaleA * timeScaleB;
        if (slowMotionFlags & 0x80000000u)
            scale *= slowMotionScale;
        return scale;
    }
    // @func 0x00787D44 SlyGame::EndSlowMotion
    void EndSlowMotion() { slowMotionFlags &= 0x7FFFFFFFu; }
    // +0x1282D8 bit 31. NOTE: meaning unknown.
    // @func 0x00787D5C SlyGame::ClearFlag1282D8
    void ClearFlag1282D8() { flags1282D8 &= 0x7FFFFFFFu; }
    // Queues a blend request (+0x1283A8): three values, a word and the
    // `flag` bit; bit 30 marks the request pending.
    // NOTE: what is blended is not known.
    // @func 0x00787CCC SlyGame::RequestBlend
    void RequestBlend(float a, float b, float c, uint32_t word, bool flag)
    {
        blendRequest.values[0] = a;
        blendRequest.values[1] = b;
        blendRequest.values[2] = c;
        blendRequest.word = word;
        blendRequest.field_10 = 0;
        blendRequest.flags = (blendRequest.flags & 0x3FFFFFFFu) | 0x40000000u | (flag ? 0x80000000u : 0u);
    }

    GameClock* clock = nullptr;                 // +0x0C
    uint8_t localPlayerCount = 1;               // +0x1D
    uint32_t worldFlags = 0;                    // +0x28
    uint8_t stateFlags = 0;                     // +0x2C NOTE: bit 5/6 toggled by GUI code; bit 7 see SubState
    uint32_t subState = 0;                      // +0x48
    uint32_t jobStateBits = 0;                  // +0x1074 NOTE: bits 20-31 are per-job state, cleared with the goals
    WorldObject* playerObject = nullptr;        // +0x1078 the controlled player character
    ape::ApeTArray<goals::Goal*> goals;         // +0x10C0 the running job's goals (count +0x10C8)
    SpawnManager* spawnManager = nullptr;       // NOTE: owner of the spawner groups; slot unknown
    ape::ApeLevelDef* levelDef = nullptr;       // +0x1281C0 the running level
    gameplay::Minigame* minigame = nullptr;     // +0x1281F4 the level's game mode
    // NOTE: saved and cleared while a minigame runs, restored when it stops.
    uint8_t field_12828C = 0;                   // +0x12828C
    uint32_t flags1282A0 = 0;                   // +0x1282A0
    float slowMotionScale = 1.0f;               // +0x1282AC
    uint32_t slowMotionFlags = 0;               // +0x1282BC bit 31: slow motion active
    uint32_t flags1282D8 = 0;                   // +0x1282D8
    struct BlendRequest {
        float values[3] = {};
        uint32_t word = 0;
        uint32_t field_10 = 0;
        uint32_t flags = 0;                     // bit 31: flag, bit 30: pending
    } blendRequest;                             // +0x1283A8
    ape::ApeTArray<WorldCallback> callbackLists[kCallbackListCount]; // +0x128480
    float timeScaleA = 1.0f;                    // +0x128588
    float timeScaleB = 1.0f;                    // +0x128594
};

// The single game world (0x00F65CA0).
extern SlyGame* g_game;

// Requests a plain reload of the running level.
void RequestLevelRestart();
// Level of detail weight of height differences (EngineConfig +0x3C): set
// directly, or from the running level's "lodYInfluence" (1 without a level).
void SetLodYInfluence(float influence);
void ApplyLevelLodYInfluence(const SlyGame& game);

} // namespace sly
