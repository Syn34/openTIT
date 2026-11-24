// openTIT - Character abilities (moves and upgrades).
//
// The game knows 107 abilities (walk, jump, glide, the combat upgrades, ...).
// The static table (0x00AC9A18, stride 0x1C) gives each one its name hash,
// option flags, a range value and the two HUD prompts; a second table
// (0x00ACA980) gives the control bits an ability unlocks. Every character
// keeps one state byte per ability (+0x290).
#pragma once

#include <cstdint>

namespace sly {
namespace entity {

constexpr int kAbilityCount = 107;

struct AbilityInfo {
    const char* name;
    uint32_t nameHash;
    uint32_t flags;          // NOTE: option bits, roles not decoded
    float range;             // NOTE: 3.5 for nearly all; 999999 for two
    const char* prompt;      // HUD prompt (string table keys)
    const char* altPrompt;   // prompt for the alternate costume/character
    uint32_t field_18;       // NOTE: 0x01000000 on the basic locomotion moves
    uint32_t controlMask;    // 0x00ACA980 control bits enabled by the ability
};

const AbilityInfo& GetAbility(int index);

// Index of the ability with that name hash, -1 if none (0 is never found).
int FindAbility(uint32_t nameHash);
uint32_t AbilityHash(int index);

// Per-character state byte of an ability.
enum AbilityState : uint8_t {
    kAbility_Blocked   = 0x40,   // temporarily disabled by script
    kAbility_Available = 0x80,   // learned / unlocked
};

struct AbilitySet {
    uint8_t state[kAbilityCount] = {};   // character +0x290

    bool IsUsable(int index) const
    {
        return (state[index] & kAbility_Available) && !(state[index] & kAbility_Blocked);
    }
};

// Control bits of all usable abilities. `moveState` is the character's
// current state of layer 0 (its virtual slot 8): state 1 removes bit 16 and
// state 7 bit 17.
uint32_t AbilityControlMask(const AbilitySet& set, int moveState);

} // namespace entity
} // namespace sly
