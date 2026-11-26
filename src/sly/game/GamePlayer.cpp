// openTIT - Per-player block of the game world.
#include "sly/game/GamePlayer.h"

#include "ape/world/ApeBase.h"
#include "sly/entity/Character.h"
#include "sly/entity/Entity.h"
#include "sly/tuning/GameTuning.h"

namespace sly {

namespace {
constexpr uint32_t kCharacterTypeBit = 3;     // Entity +0xC0 bit 3: a Character
constexpr int kAbilityFastRecharge = 84;      // "FastRecharge"
} // namespace

// @func 0x0045C868 GamePlayer::GamePlayer
GamePlayer::GamePlayer()
{
    spawnTransform = ape::world::kWorldIdentity;
    ResetMeter();
    ClearEventLogs();
}

// @func 0x0045C81C GamePlayer::ResetMeter
void GamePlayer::ResetMeter()
{
    meterMax = g_gamePlayerPowerMax;
    meterRate = g_gamePlayerPowerRecharge;
    meter = meterMax;
    flags |= kFlag_MeterHold;
}

// @func 0x0045C7A8 GamePlayer::ClearEventLogs
void GamePlayer::ClearEventLogs()
{
    eventLog.Clear();
    for (uint8_t& b : field_20074)
        b = 0;
    deferredEvents.Clear();
    flags &= ~kFlag_80;
}

// @func 0x00465CB8 GamePlayer::UpdateMeter
void GamePlayer::UpdateMeter(float dt)
{
    if (!(flags & kFlag_MeterHold)) {
        float rate = meterRate;
        // A character with the "FastRecharge" upgrade (ability 84, unlocked
        // and not blocked) recharges at the fast rate.
        if (character && character->HasTypeBit(kCharacterTypeBit)) {
            const auto* ch = static_cast<const entity::Character*>(character);
            if (ch->abilities.IsUsable(kAbilityFastRecharge))
                rate = g_gamePlayerPowerRechargeFast;
        }
        const float refilled = meter + rate * dt;
        meter = refilled <= meterMax ? refilled : meterMax;
    }
    flags &= ~kFlag_MeterHold;
}

} // namespace sly
