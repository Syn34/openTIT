// openTIT - Purchasable player abilities (upgrades).
#include "sly/game/PlayerAbilities.h"

#include "sly/game/goals/JobManager.h"

namespace sly {
namespace game {

namespace {

constexpr uint32_t kOwnedBit = 1;
constexpr uint32_t kBlockedBit = 2;

uint32_t& AbilityWord(uint8_t profile, uint32_t ability)
{
    return ProfileProgress(profile).abilityBits[ability / 16];
}

uint32_t AbilityShift(uint32_t ability)
{
    return (ability & 15) * 2;
}

} // namespace

const AbilityInfo g_abilities[kAbilityCount] = {
    { "LessWaterDamage", 0x110313EDu, 0, 0 },
    { "CoinIncrease", 0x0030F3D3u, 0, 0 },
    { "BinocucomBottleVisible", 0x6A8602C5u, 0, 0 },
    { "IncreaseAutoCollectDist", 0x4F187A1Fu, 0, 0 },
    { "IncreaseDefense", 0x15627D9Eu, 0, 0 },
};

// @func 0x00465E10 FindAbility
int FindAbility(uint32_t nameHash)
{
    if (nameHash == 0)
        return -1;
    for (int i = 0; i < kAbilityCount; ++i)
        if (g_abilities[i].nameHash == nameHash)
            return i;
    return -1;
}

// @func 0x00465D5C PlayerProfileRef::GetField70
uint32_t PlayerProfileRef::GetField70() const
{
    return ProfileProgress(profile).field_70;
}

// @func 0x00465D88 PlayerProfileRef::SetAbilityOwned
void PlayerProfileRef::SetAbilityOwned(uint32_t ability, bool owned)
{
    uint32_t& word = AbilityWord(profile, ability);
    const uint32_t bit = kOwnedBit << AbilityShift(ability);
    word = owned ? (word | bit) : (word & ~bit);
}

// @func 0x00465EA0 PlayerProfileRef::SetAbilityBlocked
void PlayerProfileRef::SetAbilityBlocked(uint32_t ability, bool blocked)
{
    uint32_t& word = AbilityWord(profile, ability);
    const uint32_t bit = kBlockedBit << AbilityShift(ability);
    word = blocked ? (word | bit) : (word & ~bit);
}

// @func 0x00465E60 PlayerProfileRef::HasAbility
bool PlayerProfileRef::HasAbility(uint32_t ability) const
{
    const uint32_t bits = (AbilityWord(profile, ability) >> AbilityShift(ability)) & 3;
    return bits == kOwnedBit;
}

} // namespace game
} // namespace sly
