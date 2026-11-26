// openTIT - Small overrides of the vtable-only entity classes (see MiscEntities.h).
#include "sly/entity/MiscEntities.h"

namespace sly {
namespace entity {

// @func 0x00758FF0 RigidEntityB19198::~RigidEntityB19198
// @func 0x00759038 RigidEntityB19198::~RigidEntityB19198 (deleting)
// @func 0x007591EC RigidEntityB19198::~RigidEntityB19198 (thunk, +0x17C)
// @func 0x00759124 RigidEntityB19198::~RigidEntityB19198 (deleting thunk, +0x17C)
// @func 0x0075919C RigidEntityB19198::~RigidEntityB19198 (thunk, +0x180)
// @func 0x007590AC RigidEntityB19198::~RigidEntityB19198 (deleting thunk, +0x180)
RigidEntityB19198::~RigidEntityB19198() = default;

// @func 0x0075923C RigidEntityB19198::GetRenderObject
ape::world::ApeObj* RigidEntityB19198::GetRenderObject() { return static_cast<ape::world::ApeObj*>(rigidBody); }

// @func 0x00759244 RigidEntityB19198::Slot95
void RigidEntityB19198::Slot95() {}

// @func 0x00759250 RigidEntityB19198::Interface17CSlot6
// @func 0x00759254 RigidEntityB19198::Interface17CSlot6 (thunk, +0x17C)
void RigidEntityB19198::Interface17CSlot6() {}

// @func 0x00759248 RigidEntityB19198::Interface17CSlot7
// @func 0x0075924C RigidEntityB19198::Interface17CSlot7 (thunk, +0x17C)
void RigidEntityB19198::Interface17CSlot7() {}

// @func 0x00759258 RigidEntityB19198::Interface180Slot6
// @func 0x00759288 RigidEntityB19198::Interface180Slot6 (thunk, +0x180)
void RigidEntityB19198::Interface180Slot6(void* body) { StopListeningTo(body); }

// @func 0x00759438 ScriptedObjectB193C8::~ScriptedObjectB193C8
// @func 0x00759468 ScriptedObjectB193C8::~ScriptedObjectB193C8 (deleting)
ScriptedObjectB193C8::~ScriptedObjectB193C8() = default;

// @func 0x00751048 EntityB15028::~EntityB15028
// @func 0x007510B0 EntityB15028::~EntityB15028 (deleting)
EntityB15028::~EntityB15028() = default;

} // namespace entity
} // namespace sly
