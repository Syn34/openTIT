// openTIT - Stubs for PhysicsHooks.h.
#include "sly/entity/PhysicsHooks.h"

namespace sly {
namespace entity {

bool RigidBodyInWorld(void* body) { (void)body; return false; }
void RigidBodyRemoveFromWorld(void* body) { (void)body; }
void RigidBodyAddToWorld(void* body, uint32_t collisionGroup) { (void)body; (void)collisionGroup; }
void PhysicsAddAction(void* action) { (void)action; }
uint8_t RigidBodyMotionType(void* body) { (void)body; return 0; }
void RigidBodyApplyImpulse(void* body, const ape::Vec3& impulse) { (void)body; (void)impulse; }
ape::Vec3 RigidBodyLinearVelocity(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
ape::Vec3 RigidBodyCenterOfMass(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void RigidBodyClearUserProperties(void* body) { (void)body; }
void HavokRelease(void* object) { (void)object; }

} // namespace entity
} // namespace sly
