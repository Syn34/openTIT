// openTIT - Character helpers of 0x00120000-0x00140000 that reach into the
// game layer (see Character.h).
#include "sly/entity/Character.h"

#include "sly/entity/Awards.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/SlyGame.h"
#include "sly/levels/ai/AIController.h"
#include "sly/objects/CharacterMoveTuning.h"

#include <cstring>

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kTrophy_Takedowns = 0x93D26E8Cu;
} // namespace

// @func 0x0013D530 Character::CountStealthTakedown
// The count is kept pending for the player's profile and capped so that the
// saved counter plus the pending count stays below 255. NOTE: the console
// indexes the progress record by the player's profile index; the port has
// one record.
void Character::CountStealthTakedown()
{
    const levels::ai::AIManager* manager = levels::ai::GetAIManager();
    if (manager && manager->alert)
        return;
    const GamePlayer& player = g_game->players[PlayerSlot()];
    uint8_t& pending = g_game->pendingCounts[player.index & 7];
    const uint32_t saved = g_game->progress.field_144;
    if (saved + pending >= 0xFF)
        return;
    ++pending;
    if (int(saved + pending) >= int(objects::g_characterMoveGroundTakedownAwardNumTakedowns))
        AwardTrophyByName(kTrophy_Takedowns);
}

// @func 0x001293C0 Character::IdleStateValue
uint32_t Character::IdleStateValue(bool requireFlag)
{
    if (EntityGetState(this, 0) != 0 || EntityGetState(this, 1) != 0)
        return 0;
    const uint8_t* data = EntityStateData(this, 0);
    if (!data)
        return 0;
    uint32_t words[2];
    std::memcpy(words, data, sizeof words);
    if (requireFlag && !(words[0] & 0x20000u))
        return 0;
    return words[1];
}

} // namespace entity
} // namespace sly
