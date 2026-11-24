// openTIT - The level's active game-mode object (see Minigame.h).
#include "sly/gameplay/Minigame.h"

#include "sly/gameplay/GameplayWorld.h"

namespace sly {
namespace gameplay {

// @func 0x003A4010 Minigame::ClearRequests
void Minigame::ClearRequests()
{
    resetRequests = 0;
}

// @func 0x003A4000 Minigame::RequestFullReset
void Minigame::RequestFullReset()
{
    resetRequests |= kMinigameRequest_All;
}

// @func 0x003AAAE0 Minigame::RequestScoreReset
void Minigame::RequestScoreReset()
{
    resetRequests |= kMinigameRequest_Score;
}

// @func 0x003AAAF0 Minigame::RequestTimerReset
void Minigame::RequestTimerReset()
{
    resetRequests |= kMinigameRequest_Timer;
}

// @func 0x003AAB08 Minigame::PlayIntro
void Minigame::PlayIntro()
{
    GuiPlayAnimation(-1.0f, introGui, nullptr, true);
}

// @func 0x003AAB54 Minigame::StartPlayer
void Minigame::StartPlayer()
{
    if (player != nullptr)
        EntityActivate(player);
}

} // namespace gameplay
} // namespace sly
