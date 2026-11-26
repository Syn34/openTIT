// openTIT - The physics body abstraction the vehicle entities are written
// against.
//
// On the console every vehicle owns one or more Havok rigid bodies
// (hkpRigidBody) cloned from bodies of the level's physics world, plus Havok
// vehicle/constraint objects for the wheeled kinds. The vehicle code only
// needs a small set of operations on them; those are declared here on an
// opaque `PhysicsBody` so that the gameplay code never touches Havok
// internals. VehiclePhysics.cpp holds TODO stubs (safe defaults) until a
// physics layer exists; each declaration names the Havok call or member it
// stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace objects {

// A rigid body of the physics world (hkpRigidBody on the console).
struct PhysicsBody;

// How a body moves (hkpMotion::MotionType values the game tells apart).
enum class BodyMotion : uint8_t {
    Dynamic   = 0,   // any simulated motion type
    Keyframed = 4,
    Fixed     = 5,
};

// -- lookup and lifetime ----------------------------------------------------------
// Body of the level's physics world with that name (0x0034654C on the game
// physics world, game world +0x1281D4); null when there is none.
PhysicsBody* FindWorldBody(const char* name);
// A new body built from the construction info of `source` (hkpRigidBody::
// getCinfo 0x0091D208 + constructor 0x0091E100), named `name` (0x007CE950).
// The new body is not in a world.
PhysicsBody* CloneBody(const PhysicsBody* source, const char* name);
// Takes a body out of its world when it is in one (hkpWorld::removeEntity
// 0x0092FFC8) and drops the reference (0x007D4EA8).
void ReleaseBody(PhysicsBody* body);
// Drops a reference to a body without touching its world (0x007D4EA8).
void BodyRemoveReference(PhysicsBody* body);
// Drops a reference to any other physics object (shapes, phantoms).
void PhysicsRemoveReference(void* object);
// Takes a body out of its world (if it is in one) and keeps it.
void RemoveBodyFromWorld(PhysicsBody* body);
// The body is in a physics world (hkpWorldObject::m_world, +0x08).
bool BodyInWorld(const PhysicsBody* body);
// Name of a body (hkpWorldObject::m_name, +0x78).
const char* BodyName(const PhysicsBody* body);
// Motion type (+0xE8).
BodyMotion BodyMotionType(const PhysicsBody* body);
// The body moves under simulation (neither keyframed nor fixed).
inline bool BodyIsDynamic(const PhysicsBody* body)
{
    const BodyMotion m = BodyMotionType(body);
    return m != BodyMotion::Keyframed && m != BodyMotion::Fixed;
}

// -- state -----------------------------------------------------------------------
// World transform of the body (motion state transform, +0xF0).
ape::Matrix34 BodyTransform(const PhysicsBody* body);
// Centre of mass in world space (+0x140).
ape::Vec3 BodyCenterOfMass(const PhysicsBody* body);
// Linear (+0x1B0) and angular (+0x1C0) velocity of the motion.
ape::Vec3 BodyLinearVelocity(const PhysicsBody* body);
ape::Vec3 BodyAngularVelocity(const PhysicsBody* body);
// Mass of the body (hkpMotion::getMass 0x00922918: 1 / inverse mass, 0 for
// infinite mass).
float BodyMass(const PhysicsBody* body);

// -- control ---------------------------------------------------------------------
// Each of these wakes the body first (hkpEntity::activate 0x0091A4C8) and
// then calls the motion's virtual: setLinearVelocity (slot 17),
// setAngularVelocity (slot 18), applyPointImpulse (slot 22).
void BodySetLinearVelocity(PhysicsBody* body, const ape::Vec3& velocity);
void BodySetAngularVelocity(PhysicsBody* body, const ape::Vec3& velocity);
void BodyApplyPointImpulse(PhysicsBody* body, const ape::Vec3& impulse, const ape::Vec3& point);
// Moves the body (hkpRigidBody::setTransform 0x0091EAB8).
void BodySetTransform(PhysicsBody* body, const ape::Matrix34& transform);

// -- ownership and listeners -----------------------------------------------------
// Marks the body as belonging to an entity (the "OwnerType" = "Entity" and
// "OwnerPtr" user properties, 0x0093FE00).
void BodySetOwner(PhysicsBody* body, void* owner);
// hkpEntity::addContactListener (0x0091A82C) / removeContactListener
// (0x0091A228) / addEntityListener (0x0091A5F8); the listeners are the
// entity's +0x17C-style / +0x180-style interfaces.
void BodyAddContactListener(PhysicsBody* body, void* listener);
void BodyRemoveContactListener(PhysicsBody* body, void* listener);
void BodyAddEntityListener(PhysicsBody* body, void* listener);
// Puts the body into the game physics world, active (hkpWorld::addEntity
// 0x0092F2A8). NOTE: the console also clears the body's halfword +0xA6 and
// sets bit 0 of byte +0xCE first (Havok fields not identified).
void AddBodyToWorld(PhysicsBody* body);

// Collision group of a body (filter info +0x2C, bits 16-31).
uint32_t BodyCollisionGroup(const PhysicsBody* body);

// -- phantoms --------------------------------------------------------------------
// A sphere phantom (hkpSphereShape 0x0089FE70 inside an
// hkpSimpleShapePhantom 0x0092A718) with the collision filter word
// `filterInfo`, added to the game physics world (0x00931D10).
void* CreateSpherePhantom(float radius, uint32_t filterInfo);
// Takes a phantom out of its world (0x00931F00) and drops it.
void ReleasePhantom(void* phantom);

// -- contacts --------------------------------------------------------------------
// The contact point event handed to a contact listener (hkpContactPointEvent):
// the two bodies in contact (+0x04/+0x08) and the collision layer of a body
// (filter info +0x2C, bits 0-4).
PhysicsBody* ContactBody(const void* event, int index);
uint32_t BodyCollisionLayer(const PhysicsBody* body);
// Normal of the contact point, pointing away from `from` (the event's
// contact point +0x14, normal +0x10; negated when `from` is the event's
// second body). False when the event carries no contact point (+0x1C).
bool ContactNormal(const void* event, const PhysicsBody* from, ape::Vec3& normal);
// Turns the contact off (flag 8 in byte +0x0F of the contact properties).
void ContactDisable(const void* event);
// Scales how strongly the contact pushes `body` (hkpResponseModifier::
// setInvMassScalingForContact 0x00907A18): xyz scale the inverse inertia,
// w the inverse mass.
void ContactScaleResponse(const void* event, PhysicsBody* body, const ape::Vec4& factor);

} // namespace objects
} // namespace sly
