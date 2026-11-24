// openTIT - Calls out of the component code into systems that are not
// ported yet. Each is a TODO stub until its owner exists.
#pragma once

#include <cstdint>

namespace sly {
namespace components {

// Physics world of the running level (game world 0x00F65CA0 +0x1281D4).
void* LevelPhysicsWorld();

} // namespace components
} // namespace sly
