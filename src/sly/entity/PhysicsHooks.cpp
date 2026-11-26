// openTIT - Stubs for PhysicsHooks.h.
#include "sly/entity/PhysicsHooks.h"

#include "sly/levels/nav/NavGraph.h"

namespace sly {
namespace entity {

// TODO(0x0091E100 hkpRigidBody for actor parts; Havok)
void* ActorPartCreateBody(const ActorPartDesc& part, uint32_t navMode)
{
    (void)part; (void)navMode;
    return nullptr;
}

const char* HavokBodyName(void*) { return nullptr; }

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
// TODO(0x004A41C0 body sweep): Havok linear cast not ported; the way is free.
bool RigidBodySweep(void* body, const ape::Matrix34& from, const ape::Vec3& to, ape::Matrix34& reached)
{
    (void)body;
    reached = from;
    reached.translation = {to.x, to.y, to.z, from.translation.pad};
    return false;
}
void PhysicsAddAction(void* action) { (void)action; }
uint8_t RigidBodyMotionType(void* body) { (void)body; return 0; }
void RigidBodyActivate(void* body) { (void)body; }
void RigidBodyAddContactListener(void* body, void* listener) { (void)body; (void)listener; }
void RigidBodyRemoveContactListener(void* body, void* listener) { (void)body; (void)listener; }
void RigidBodyApplyHardKeyFrame(void* body, const ape::Vec3& position, const ape::Vec4& rotation, float invDt)
{
    (void)body; (void)position; (void)rotation; (void)invDt;
}
void RigidBodySetMotionType(void* body, uint32_t type) { (void)body; (void)type; }
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
void* CollidableOwnerBody(const void* collidable) { (void)collidable; return nullptr; }
void* ContactEventBody(const void* event, int index) { (void)event; (void)index; return nullptr; }
void ContactEventDisable(const void* event) { (void)event; }
uint32_t RigidBodyOwnerType(void* body) { (void)body; return 0; }
void* RigidBodyOwner(void* body) { (void)body; return nullptr; }
void HavokRelease(void* object) { (void)object; }

} // namespace entity
} // namespace sly
