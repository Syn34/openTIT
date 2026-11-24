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

// @func 0x0016A740 AlterEgoBentleyDef::~AlterEgoBentleyDef
// @func 0x0016A7A0 AlterEgoBentleyDef::~AlterEgoBentleyDef (deleting)
AlterEgoBentleyDef::~AlterEgoBentleyDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0017CC7C TargetableDefA::~TargetableDefA
// @func 0x0017CCDC TargetableDefA::~TargetableDefA (deleting)
TargetableDefA::~TargetableDefA()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0018EDB4 TargetableDefB::~TargetableDefB
// @func 0x0018EEA0 TargetableDefB::~TargetableDefB (deleting)
TargetableDefB::~TargetableDefB()
{
    while (entries.Count() != 0) {
        delete entries[entries.Count() - 1];
        entries.RemoveAtSwap(entries.Count() - 1);
    }
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001F8F8C TargetableDefC::~TargetableDefC
// @func 0x001F8FEC TargetableDefC::~TargetableDefC (deleting)
TargetableDefC::~TargetableDefC()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001B3AAC PushBlockDef::~PushBlockDef
// @func 0x001B3B34 PushBlockDef::~PushBlockDef (deleting)
PushBlockDef::~PushBlockDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001A7058 ChainDef::~ChainDef
// @func 0x001A70D4 ChainDef::~ChainDef (deleting)
ChainDef::~ChainDef()
{
    for (audio::SoundName* sound : sounds)
        audio::SoundNameRelease(sound);
}

// @func 0x0016EC4C CompoundEntity::~CompoundEntity
// @func 0x0016EE20 CompoundEntity::~CompoundEntity (deleting)
// @func 0x0016EE68 CompoundEntity::~CompoundEntity (deleting thunk, +0x180)
// @func 0x0016EEB4 CompoundEntity::~CompoundEntity (deleting thunk, +0x17C)
// @func 0x0016EF00 CompoundEntity::~CompoundEntity (thunk, +0x180)
// @func 0x0016EF28 CompoundEntity::~CompoundEntity (thunk, +0x17C)
CompoundEntity::~CompoundEntity()
{
    DequeueEntityTransformCallback(this);
    UnregisterEntity(kList_B, this);
    for (Part* part : parts)
        delete part;
}

} // namespace entity
} // namespace sly
