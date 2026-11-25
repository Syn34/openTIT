// openTIT - Physical entity stand-ins (see PhysicalEntity.h).
#include "sly/entity/PhysicalEntity.h"

#include "sly/camera/CameraHooks.h"

namespace sly {
namespace entity {

// Minimal version of 0x000E9820 (entity module): drops the collision shape.
PhysicalEntityDef::~PhysicalEntityDef()
{
    if (collisionShape)
        camera::HavokRemoveReference(collisionShape);
}

// Minimal version of 0x000EB0F8 (entity module): takes the rigid body out of
// the physics world and drops it, releases the sound.
PhysicalEntity::~PhysicalEntity()
{
    if (rigidBody)
        camera::DestroyHavokEntity(rigidBody);
    audio::SoundNameRelease(impactSound);
}

bool PhysicalEntity::PostUpdate(float dt)
{
    return ScriptedEntity::PostUpdate(dt);
}

bool PhysicalEntity::Release(bool kill)
{
    return ScriptedEntity::Release(kill);
}

// TODO(0x000E9BA0 PhysicalEntity::SetPhysState)
void PhysicalEntity::SetPhysState(int state, uint32_t arg)
{
    (void)arg;
    physState = state;
}

} // namespace entity
} // namespace sly
