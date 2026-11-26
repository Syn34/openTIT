// openTIT - Calls out of the actor classes (sly/entity) into modules that are
// not ported yet. Each is a TODO stub in ActorHooks.cpp naming the console
// function.
#pragma once

#include <cstdint>

namespace sly {
namespace entity {
class Entity;
class EntityComponent;

// Interaction component: ends the current use (0x002B7704): the owner is told
// (slot 63), a character user leaves its interaction state and is let go, the
// user is forgotten and nothing is offered any more.
void InteractionStop(EntityComponent* interaction);

} // namespace entity
} // namespace sly
