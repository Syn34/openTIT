// openTIT - Stubs for PhysicsHooks.h.
#include "sly/entity/PhysicsHooks.h"

#include "sly/levels/nav/NavGraph.h"

namespace sly {
namespace entity {

bool RigidBodyInWorld(void* body) { (void)body; return false; }
void RigidBodyRemoveFromWorld(void* body) { (void)body; }
void RigidBodyAddToWorld(void* body, uint32_t collisionGroup) { (void)body; (void)collisionGroup; }
void RigidBodySetFilterGroup(void* body, uint32_t collisionGroup) { (void)body; (void)collisionGroup; }
int RigidBodyConstraintCount(void* body) { (void)body; return 0; }
void* RigidBodyConstraint(void* body, int index) { (void)body; (void)index; return nullptr; }
uint32_t RigidBodyCollisionGroup(void* body) { (void)body; return 0; }
uint32_t RigidBodyFilterInfo(void* body) { (void)body; return 0; }
void RigidBodySetFilterInfo(void* body, uint32_t info) { (void)body; (void)info; }
float PhysicsWorldGravity() { return 0.0f; }
void PhysicsAddAction(void* action) { (void)action; }
uint8_t RigidBodyMotionType(void* body) { (void)body; return 0; }
void RigidBodyActivate(void* body) { (void)body; }
void RigidBodyApplyImpulse(void* body, const ape::Vec3& impulse) { (void)body; (void)impulse; }
void RigidBodySetLinearVelocity(void* body, const ape::Vec3& velocity) { (void)body; (void)velocity; }
ape::Vec3 RigidBodyLinearVelocity(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
ape::Vec3 RigidBodyCenterOfMass(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void RigidBodySetTransform(void* body, const ape::Matrix34& transform) { (void)body; (void)transform; }
void RigidBodyWakeConnected(void* body) { (void)body; }
void RigidBodyClearHalfA6(void* body) { (void)body; }
// The console passes the bounds along but does not use them (0x00011410).
void* NavAddBodyObstacle(void* body, const ape::Aabb& bounds, const char* name, uint32_t mask)
{
    (void)bounds;
    levels::NavManager* nav = levels::GetNavManager();
    if (!nav || !body)
        return nullptr;
    return levels::AddBodyNavObstacle(*nav, body, name, mask);
}
ape::Matrix34 RigidBodyTransform(void* body)
{
    (void)body;
    return { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
}
const void* RigidBodyShape(void* body) { (void)body; return nullptr; }
uint8_t HavokShapeType(const void* shape) { (void)shape; return 0; }
ape::Vec3 HavokShapeTranslation(const void* shape) { (void)shape; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
ape::Matrix34 HavokShapeTransform(const void* shape)
{
    (void)shape;
    return { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
}
void HavokShapeAabb(const void* shape, const ape::Matrix34& transform, ape::Aabb& out)
{
    (void)shape;
    out.min = transform.translation;
    out.max = transform.translation;
}
ape::Vec3 RigidBodyPosition(void* body) { (void)body; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
void RigidBodyClearUserProperties(void* body) { (void)body; }
void HavokShapeLocalAabb(const void* shape, ape::Aabb& out)
{
    (void)shape;
    out.min = { 0.0f, 0.0f, 0.0f, 0.0f };
    out.max = { 0.0f, 0.0f, 0.0f, 0.0f };
}
void HavokRelease(void* object) { (void)object; }

} // namespace entity
} // namespace sly
