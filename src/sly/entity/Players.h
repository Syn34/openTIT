// openTIT - Player helpers used by the character and alter ego code.
#pragma once

namespace sly {
namespace entity {

class Entity;

// The other player's entity in two-player levels (characters keep their
// player index at +0x304); null in single-player levels.
Entity* OtherPlayerEntity(int playerIndex);

} // namespace entity
} // namespace sly
