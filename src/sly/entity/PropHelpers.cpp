// openTIT - Small helpers of the prop and target classes (PropEntities.h).
#include "sly/entity/PropEntities.h"

#include "sly/entity/Awards.h"
#include "sly/game/SlyGame.h"
#include "sly/gameplay/Minigame.h"

namespace sly {
namespace entity {

// @func 0x001882F8 MinigameFlagFigureHit
// A running minigame whose flags1C bit 8 is set hears that a figure was hit.
// NOTE: name from the caller (TargetableA slot 96).
void MinigameFlagFigureHit()
{
    gameplay::Minigame* minigame = g_game->minigame;
    if (minigame && (minigame->flags1C & 0x100u))
        minigame->field_135 = 1;
}

// @func 0x00175CF0 CompoundEntity::FinishMinigame
// NOTE: Damage clears the minigame's byte +0x105 when the Bentley's health
// runs out; the test here reads as given.
void CompoundEntity::FinishMinigame()
{
    if (propState == 16)
        return;
    propState = 16;
    if (gameplay::Minigame* minigame = g_game->minigame) {
        if (minigame->flags1C & 0x20u) {
            if (!minigame->field_105)
                AwardTrophyByName(0xB2308356u);
            minigame->field_105 = 0;
        }
    }
    g_game->progress.field_158 = 0;
}

} // namespace entity
} // namespace sly
