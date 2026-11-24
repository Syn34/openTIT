// openTIT - Entity classes built from a definition's collision object (see PropEntities.h).
#include "sly/entity/PropEntities.h"

#include "sly/camera/CameraHooks.h"

namespace sly {
namespace entity {

void ReleaseCollisionShape(void* shape)
{
    camera::HavokRemoveReference(shape);
}

// @func 0x001667DC RigidPropDef::~RigidPropDef
// @func 0x0016683C RigidPropDef::~RigidPropDef (deleting)
RigidPropDef::~RigidPropDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0016819C RigidProp::~RigidProp
// @func 0x001681E4 RigidProp::~RigidProp (deleting)
// @func 0x00168258 RigidProp::~RigidProp (deleting thunk, +0x180)
// @func 0x001682D0 RigidProp::~RigidProp (deleting thunk, +0x17C)
// @func 0x00168350 RigidProp::~RigidProp (thunk, +0x180)
// @func 0x001683A0 RigidProp::~RigidProp (thunk, +0x17C)
RigidProp::~RigidProp() = default;

// @func 0x001762F8 AlterEgoDef::~AlterEgoDef
// @func 0x00176358 AlterEgoDef::~AlterEgoDef (deleting)
AlterEgoDef::~AlterEgoDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x00178060 AlterEgoEntity::~AlterEgoEntity
// @func 0x001780E4 AlterEgoEntity::~AlterEgoEntity (deleting)
// @func 0x00178184 AlterEgoEntity::~AlterEgoEntity (deleting thunk, +0x180)
// @func 0x00178228 AlterEgoEntity::~AlterEgoEntity (deleting thunk, +0x17C)
// @func 0x001782D0 AlterEgoEntity::~AlterEgoEntity (thunk, +0x180)
// @func 0x00178358 AlterEgoEntity::~AlterEgoEntity (thunk, +0x17C)
AlterEgoEntity::~AlterEgoEntity()
{
    UnregisterEntity(kList_B, this);
}

} // namespace entity
} // namespace sly
