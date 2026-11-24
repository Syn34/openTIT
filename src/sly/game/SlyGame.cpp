// openTIT - Game world object: the parts owned by the game shell.
#include "sly/game/SlyGame.h"

namespace sly {

SlyGame* g_game = nullptr;

// @func 0x00458B50 SlyGame::ResetFrameLists
void SlyGame::ResetFrameLists()
{
    for (FrameList& list : frameLists)
        list.count = 0;
}

} // namespace sly
