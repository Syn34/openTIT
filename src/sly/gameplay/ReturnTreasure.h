// openTIT - Return-treasure microgame (0xB0 bytes, owned by the game world
// at +0x12847C; constructed by the game update).
//
// When a thief grabs one of the player's treasures, the treasure is
// attached to the carrier and the microgame runs: an optional countdown on
// the "Timer" HUD element, the "Treasure"/carrier variables of its anim
// script (0x9F90E54F) bound to the two world objects. Getting the treasure
// back completes it (the job manager collects the treasure); losing the
// carrier or running out of time loses it ("$STR_TREASURE_LOST").
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace entity { class Entity; }
namespace hud { class HudObject; }
namespace gameplay {

class ReturnTreasureMicrogame {
public:
    ReturnTreasureMicrogame();

    // Ends the anim script (state "0x00010CBB", dropped when finished).
    void Shutdown();
    // Back to the idle state; unbinds the script's object variables.
    void Reset();
    // Starts the microgame for `treasure` carried by `carrier` (0x00424FB8).
    void Start(float timeLimit, entity::Entity* carrier, entity::Entity* treasure, uint32_t carrierBone,
               const ape::Vec3& offset);
    // Ends a running microgame; `lost` plays the loss (the treasure is
    // destroyed and "$STR_TREASURE_LOST" shown).
    void Stop(bool lost);
    // The treasure came back: the job manager collects it.
    void Complete();
    // An entity left the world: losing the carrier loses the treasure.
    void OnEntityRemoved(entity::Entity* entity);
    // Pauses (hiding the countdown) or resumes the microgame.
    void SetPaused(bool paused);
    // Countdown HUD element slots 3 (Refresh) and 5 (SetActive).
    void RefreshHud();
    void SetHudActive(bool active);
    // Places the treasure on the carrier (0x0042685C).
    void UpdatePlacement();
    // Per-frame update (0x00426380).
    void Update(float dt);

    bool IsActive() const { return active; }

    ape::Matrix34 field_00{};                      // +0x00 cleared by Reset
    ape::Vec4 startRotation{0.0f, 0.0f, 0.0f, 1.0f}; // +0x40
    ape::Vec4 targetRotation{0.0f, 0.0f, 0.0f, 1.0f}; // +0x50
    ape::Vec3 startPosition{};                     // +0x60 treasure position when grabbed
    ape::Vec3 targetOffset{};                      // +0x70
    uint32_t field_80 = 0;                         // +0x80
    entity::Entity* carrier = nullptr;             // +0x84 (referenced)
    entity::Entity* treasure = nullptr;            // +0x88 (referenced)
    float timeLimit = 0.0f;                        // +0x8C 0: no countdown
    float timeLeft = 0.0f;                         // +0x90
    float field_94 = 0.0f;                         // +0x94
    hud::HudObject* timerHud = nullptr;            // +0x98 the "Timer" element while counting down
    ape::ApeAnimScriptInstance* script = nullptr;  // +0x9C
    uint32_t field_A0 = 0;                         // +0xA0
    bool active = false;                           // +0xA4 bit 31
    bool paused = false;                           // +0xA4 bit 30
};

} // namespace gameplay
} // namespace sly
