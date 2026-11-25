// openTIT - Havok rigid body operations used by the physical entities of
// 0x000F0000-0x00200000 (props, projectiles, alter egos). The physics layer
// is not ported; PhysicsHooks.cpp holds TODO stubs. Each hook names the
// Havok call or member it stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace entity {

// The body is in a physics world (hkpWorldObject::m_world, +0x08).
bool RigidBodyInWorld(void* body);
// Takes the body out of its world (hkpWorld::removeEntity 0x0092FFC8).
void RigidBodyRemoveFromWorld(void* body);
// Puts the body into the game physics world (hkpWorld::addEntity 0x0092F2A8)
// with the collision group in the top half of its filter word (+0x2C);
// ~0u leaves the filter word alone.
void RigidBodyAddToWorld(void* body, uint32_t collisionGroup);
// Collision group of a body that stays where it is (filter word +0x2C bits 16-31).
void RigidBodySetFilterGroup(void* body, uint32_t collisionGroup);
// Constraints attached to a body (hkpEntity::getNumConstraints 0x0091A2CC /
// getConstraint 0x0091A7D8).
int RigidBodyConstraintCount(void* body);
void* RigidBodyConstraint(void* body, int index);
// Collision group of a body (filter word +0x2C bits 16-31).
uint32_t RigidBodyCollisionGroup(void* body);
// Adds an action to the game physics world (hkpWorld::addAction 0x0092E9D8).
void PhysicsAddAction(void* action);
// Motion type byte (+0xE8): 4 = keyframed, 5 = fixed.
uint8_t RigidBodyMotionType(void* body);
// Wakes the body (0x0091A4C8) and applies a linear impulse (motion slot 21).
void RigidBodyApplyImpulse(void* body, const ape::Vec3& impulse);
// Wakes the body and sets its linear velocity (motion slot 17).
void RigidBodySetLinearVelocity(void* body, const ape::Vec3& velocity);
// Linear velocity (+0x1B0) and centre of mass (+0x140) of the body's motion.
ape::Vec3 RigidBodyLinearVelocity(void* body);
ape::Vec3 RigidBodyCenterOfMass(void* body);
// Drops the game's two user properties of a body (0x0093FED8, keys
// 0xA734004D and 0x91C7CF5B) before it is released.
void RigidBodyClearUserProperties(void* body);
// hkReferencedObject::removeReference (0x007D4EA8).
void HavokRelease(void* object);

} // namespace entity
} // namespace sly
