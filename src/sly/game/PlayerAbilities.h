// openTIT - Purchasable player abilities (upgrades).
//
// Each profile record keeps two bits per ability at +0xFB0: bit 0 = owned
// (enabled), bit 1 = blocked (temporarily disabled by a script). An ability
// is in effect when it is owned and not blocked. The abilities are listed in
// a fixed table (console 0x00AE8128).
#pragma once

#include <cstdint>

namespace sly {
namespace game {

enum PlayerAbility : int {
    kAbility_LessWaterDamage = 0,
    kAbility_CoinIncrease = 1,
    kAbility_BinocucomBottleVisible = 2,
    kAbility_IncreaseAutoCollectDist = 3,
    kAbility_IncreaseDefense = 4,
    kAbilityCount = 5,
};

struct AbilityInfo {
    const char* name;      // +0x00
    uint32_t nameHash;     // +0x04 HashNameNoCase(name)
    uint32_t field_08;     // NOTE: zero for every entry
    uint32_t field_0C;
};
extern const AbilityInfo g_abilities[kAbilityCount];

// Index of the ability named 'nameHash', -1 if unknown (or 0).
int FindAbility(uint32_t nameHash);

// The profile a player block refers to (byte +0x1070 of the player block);
// the members operate on that profile's record.
struct PlayerProfileRef {
    uint8_t profile = 0;

    // NOTE: profile record +0x70; meaning unknown.
    uint32_t GetField70() const;
    void SetAbilityOwned(uint32_t ability, bool owned);
    void SetAbilityBlocked(uint32_t ability, bool blocked);
    // Owned and not blocked.
    bool HasAbility(uint32_t ability) const;
};

} // namespace game
} // namespace sly
