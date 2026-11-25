// openTIT - Stubs for PhysicsHooks.h.
#include "sly/entity/PhysicsHooks.h"

namespace sly {
namespace entity {

bool RigidBodyInWorld(void* body) { (void)body; return false; }
void RigidBodyRemoveFromWorld(void* body) { (void)body; }
void RigidBodyAddToWorld(void* body, uint32_t collisionGroup) { (void)body; (void)collisionGroup; }
void RigidBodySetFilterGroup(void* body, uint32_t collisionGroup) { (void)body; (void)collisionGroup; }
int RigidBodyConstraintCount(void* body) { (void)body; return 0; }
void* RigidBodyConstraint(void* body, int index) { (void)body; (void)index; return nullptr; }
uint32_t RigidBodyCollisionGroup(void* body) { (void)body; return 0; }
void PhysicsAddAction(void* action) { (void)action; }
uint8_t RigidBodyMotionType(void* body) { (void)body; return 0; }
void RigidBodyApplyImpulse(void* body, const ape::Vec3& impulse) { (void)body; (void)impulse; }
void RigidBodySetLinearVelocity(void* body, const ape::Vec3& velocity) { (void)body; (void)velocity; }
ape::Vec3 RigidBodyLinearVelocity(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
ape::Vec3 RigidBodyCenterOfMass(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void RigidBodySetTransform(void* body, const ape::Matrix34& transform) { (void)body; (void)transform; }
void RigidBodyWakeConnected(void* body) { (void)body; }
void RigidBodyClearHalfA6(void* body) { (void)body; }
ape::Vec3 RigidBodyPosition(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void RigidBodyClearUserProperties(void* body) { (void)body; }
void HavokShapeLocalAabb(void* shape, ape::Aabb& out)
{
    (void)shape;
    out.min = { 0.0f, 0.0f, 0.0f, 0.0f };
    out.max = { 0.0f, 0.0f, 0.0f, 0.0f };
}
void HavokRelease(void* object) { (void)object; }

} // namespace entity
} // namespace sly
