// openTIT - Stubs for VehiclePhysics.h (no physics layer yet). The operations
// that the entity module's physics hooks already cover forward there.
#include "sly/objects/VehiclePhysics.h"

#include "sly/entity/PhysicsHooks.h"

namespace sly {
namespace objects {

namespace {
void* Raw(const PhysicsBody* body) { return const_cast<PhysicsBody*>(body); }
} // namespace

PhysicsBody* FindWorldBody(const char* name) { (void)name; return nullptr; }
PhysicsBody* CloneBody(const PhysicsBody* source, const char* name) { (void)source; (void)name; return nullptr; }

void ReleaseBody(PhysicsBody* body)
{
    if (!body)
        return;
    RemoveBodyFromWorld(body);
    entity::HavokRelease(body);
}

void BodyRemoveReference(PhysicsBody* body)
{
    if (body)
        entity::HavokRelease(body);
}

void PhysicsRemoveReference(void* object)
{
    if (object)
        entity::HavokRelease(object);
}

void RemoveBodyFromWorld(PhysicsBody* body)
{
    if (body && BodyInWorld(body))
        entity::RigidBodyRemoveFromWorld(body);
}

bool BodyInWorld(const PhysicsBody* body) { return entity::RigidBodyInWorld(Raw(body)); }
const char* BodyName(const PhysicsBody* body) { (void)body; return ""; }
BodyMotion BodyMotionType(const PhysicsBody* body)
{
    switch (entity::RigidBodyMotionType(Raw(body))) {
    case entity::kMotionType_Keyframed: return BodyMotion::Keyframed;
    case entity::kMotionType_Fixed: return BodyMotion::Fixed;
    default: return BodyMotion::Dynamic;
    }
}

ape::Matrix34 BodyTransform(const PhysicsBody* body) { return entity::RigidBodyTransform(Raw(body)); }
ape::Vec3 BodyCenterOfMass(const PhysicsBody* body) { return entity::RigidBodyCenterOfMass(Raw(body)); }
ape::Vec3 BodyLinearVelocity(const PhysicsBody* body) { return entity::RigidBodyLinearVelocity(Raw(body)); }
ape::Vec3 BodyAngularVelocity(const PhysicsBody* body) { (void)body; return {0.0f, 0.0f, 0.0f, 0.0f}; }
float BodyMass(const PhysicsBody* body) { (void)body; return 0.0f; }

void BodySetLinearVelocity(PhysicsBody* body, const ape::Vec3& velocity)
{
    entity::RigidBodySetLinearVelocity(body, velocity);
}
void BodySetAngularVelocity(PhysicsBody* body, const ape::Vec3& velocity) { (void)body; (void)velocity; }
void BodyApplyPointImpulse(PhysicsBody* body, const ape::Vec3& impulse, const ape::Vec3& point)
{
    (void)body; (void)impulse; (void)point;
}
void BodySetTransform(PhysicsBody* body, const ape::Matrix34& transform)
{
    entity::RigidBodySetTransform(body, transform);
}

void BodySetOwner(PhysicsBody* body, void* owner) { (void)body; (void)owner; }
void BodyAddContactListener(PhysicsBody* body, void* listener) { entity::RigidBodyAddContactListener(body, listener); }
void BodyRemoveContactListener(PhysicsBody* body, void* listener)
{
    entity::RigidBodyRemoveContactListener(body, listener);
}
void BodyAddEntityListener(PhysicsBody* body, void* listener) { (void)body; (void)listener; }
void AddBodyToWorld(PhysicsBody* body)
{
    entity::RigidBodyClearHalfA6(body);
    entity::RigidBodyAddToWorld(body, ~0u);
}

uint32_t BodyCollisionGroup(const PhysicsBody* body) { return entity::RigidBodyCollisionGroup(Raw(body)); }

void* CreateSpherePhantom(float radius, uint32_t filterInfo) { (void)radius; (void)filterInfo; return nullptr; }
void ReleasePhantom(void* phantom) { PhysicsRemoveReference(phantom); }

bool ContactNormal(const void* event, const PhysicsBody* from, ape::Vec3& normal)
{
    (void)event; (void)from;
    normal = {0.0f, 0.0f, 0.0f, 0.0f};
    return false;
}

PhysicsBody* ContactBody(const void* event, int index)
{
    return static_cast<PhysicsBody*>(entity::ContactEventBody(event, index));
}
uint32_t BodyCollisionLayer(const PhysicsBody* body) { return entity::RigidBodyFilterInfo(Raw(body)) & 0x1Fu; }
void ContactDisable(const void* event) { entity::ContactEventDisable(event); }
void ContactScaleResponse(const void* event, PhysicsBody* body, const ape::Vec4& factor)
{
    (void)event; (void)body; (void)factor;
}

} // namespace objects
} // namespace sly
