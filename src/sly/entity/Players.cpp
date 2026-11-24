// openTIT - Player helpers (see Players.h).
#include "sly/entity/Players.h"

#include "sly/entity/EntityHooks.h"

namespace sly {
namespace entity {

// @func 0x0016B59C OtherPlayerEntity
// NOTE: the console takes the character and reads its player index (+0x304).
Entity* OtherPlayerEntity(int playerIndex)
{
    if (IsSinglePlayerMode())
        return nullptr;
    return PlayerEntity(1 - playerIndex);
}

} // namespace entity
} // namespace sly
