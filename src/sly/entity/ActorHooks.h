// openTIT - Calls out of the actor classes (sly/entity) into modules that are
// not ported yet. Each is a TODO stub in ActorHooks.cpp naming the console
// function.
#pragma once

#include <cstdint>

#include "ape/math/ApeMath.h"

namespace sly {
namespace entity {
class Entity;
class EntityComponent;

// Interaction component: ends the current use (0x002B7704): the owner is told
// (slot 63), a character user leaves its interaction state and is let go, the
// user is forgotten and nothing is offered any more.
void InteractionStop(EntityComponent* interaction);

// Puts the safe's per-frame callback (descriptor 0x00F33C18, 0x001E9C64)
// on the game world's callback list 3 (0x0046B5F8); GameWorldDequeueSafeCallback
// takes it off again.
void GameWorldQueueSafeCallback(Entity* safe);

} // namespace entity
} // namespace sly

namespace sly {
struct AppPlayer;
namespace entity {
// The motion sensor samples of a local player's pad over the last `window`
// seconds (0x0046426C on the player's sample history, app player +0x50,
// after the pad's slot 12 says it has a sensor). Fills up to `maxSamples`
// and returns how many; false without sensor data.
bool AppPlayerMotionSamples(AppPlayer& player, float window, ape::Vec3* out, int maxSamples, int& count);
} // namespace entity
} // namespace sly
