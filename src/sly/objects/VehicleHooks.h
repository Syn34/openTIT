// openTIT - Calls from the vehicle code into systems that are not ported yet.
// VehicleHooks.cpp holds TODO stubs with safe defaults; each declaration names
// the console function or member it stands for.
#pragma once

#include <cstdint>

namespace sly {
namespace entity { class Entity; class EntityComponent; }
namespace objects {

// Word +0x28 bit 12 of the entity's component of type 3. NOTE: the
// component class is not ported; vehicles ignore hits while it is set.
bool VehicleComponent03Blocks(entity::EntityComponent* component);
// Plays a clip through the character component (0x002775A8, blend 1.0 over
// 0.25 s), or through entity virtual slot 50 when there is none.
void VehiclePlayClip(entity::Entity* entity, uint32_t clipHash);

} // namespace objects
} // namespace sly
