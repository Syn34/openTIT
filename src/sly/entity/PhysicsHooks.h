// openTIT - Havok rigid body operations used by the physical entities of
// 0x000F0000-0x00200000 (props, projectiles, alter egos). The physics layer
// is not ported; PhysicsHooks.cpp holds TODO stubs. Each hook names the
// Havok call or member it stands for.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace entity {

// The body is in a physics world (hkpWorldObject::m_world, +0x08).
bool RigidBodyInWorld(void* body);
// Moves a body from `from` towards `to` through the world (0x004A41C0):
// `reached` gets the frame it ends up at (`from` moved to `to` when free).
// True when something blocked the way.
bool RigidBodySweep(void* body, const ape::Matrix34& from, const ape::Vec3& to, ape::Matrix34& reached);
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
// Whole collision filter word of a body (+0x2C: collision layer in bits
// 0-4, group in bits 16-31). Setting it re-filters a body that is in a
// world (0x00930B70).
uint32_t RigidBodyFilterInfo(void* body);
void RigidBodySetFilterInfo(void* body, uint32_t info);
// Gravity of the game physics world (game world +0x1281D4, float +0x54).
float PhysicsWorldGravity();
// Collision group of a body (filter word +0x2C bits 16-31).
uint32_t RigidBodyCollisionGroup(void* body);
// Adds an action to the game physics world (hkpWorld::addAction 0x0092E9D8).
void PhysicsAddAction(void* action);
// Motion type byte (+0xE8): 4 = keyframed, 5 = fixed.
constexpr uint32_t kMotionType_Keyframed = 4;
constexpr uint32_t kMotionType_Fixed = 5;
// hkpRigidBody::setMotionType (0x0091D470, activation on, queued).
void RigidBodySetMotionType(void* body, uint32_t type);
uint8_t RigidBodyMotionType(void* body);
// hkpKeyFrameUtility::applyHardKeyFrame (0x009DD41C): velocities that take
// the body to `position`/`rotation` in 1/invDt seconds.
void RigidBodyApplyHardKeyFrame(void* body, const ape::Vec3& position, const ape::Vec4& rotation, float invDt);
// hkpEntity::addContactListener (0x0091A82C).
void RigidBodyAddContactListener(void* body, void* listener);
// hkpEntity::removeContactListener (0x0091A228).
void RigidBodyRemoveContactListener(void* body, void* listener);
// Wakes the body (hkpEntity::activate 0x0091A4C8).
void RigidBodyActivate(void* body);
// Wakes the body (0x0091A4C8) and applies a linear impulse (motion slot 21).
void RigidBodyApplyImpulse(void* body, const ape::Vec3& impulse);
// Wakes the body and sets its linear velocity (motion slot 17).
void RigidBodySetLinearVelocity(void* body, const ape::Vec3& velocity);
// Linear velocity (+0x1B0) and centre of mass (+0x140) of the body's motion.
ape::Vec3 RigidBodyLinearVelocity(void* body);
ape::Vec3 RigidBodyCenterOfMass(void* body);
// Moves a body (hkpRigidBody::setTransform 0x0091EAB8; the rotation rows
// are converted with their w cleared).
void RigidBodySetTransform(void* body, const ape::Matrix34& transform);
// Wakes the body and, through its constraints, every body linked to it (0x003556AC).
void RigidBodyWakeConnected(void* body);
// Clears the halfword +0xA6 of a body when it enters the world.
// NOTE: the Havok field is not identified.
void RigidBodyClearHalfA6(void* body);
// Position of the body (its motion state's transform, +0x120).
ape::Vec3 RigidBodyPosition(void* body);
// Drops the game's two user properties of a body (0x0093FED8, keys
// 0xA734004D and 0x91C7CF5B) before it is released.
void RigidBodyClearUserProperties(void* body);
// Navigation obstacle shaped like a body's box or transformed shape, with
// the given bounds, debug name and filter mask (0x00011410); null when the
// shape does not qualify or the level has no navigation.
void* NavAddBodyObstacle(void* body, const ape::Aabb& bounds, const char* name, uint32_t mask);
// Local bounds of a collision shape (shape virtual getAabb at the identity
// transform, tolerance 0).
void HavokShapeLocalAabb(const void* shape, ape::Aabb& out);
// World transform of a body (its motion state's transform, +0xF0).
ape::Matrix34 RigidBodyTransform(void* body);
// Collision shape of a body (its collidable's shape, +0x10).
const void* RigidBodyShape(void* body);
// Shape type byte (+0x08) values the game tells apart.
// NOTE: names from the fields read for each type (+0x20 translation,
// +0x30 transform).
enum HavokShapeKind : uint8_t {
    kHavokShape_ConvexTranslate = 10,
    kHavokShape_ConvexTransform = 11,
    kHavokShape_Transform       = 14,
};
uint8_t HavokShapeType(const void* shape);
// Translation of a convex translate shape (+0x20) / transform of a
// transform shape (+0x30, through 0x007CF51C / 0x007CF188).
ape::Vec3 HavokShapeTranslation(const void* shape);
ape::Matrix34 HavokShapeTransform(const void* shape);
// Bounds of a shape placed at `transform` (shape virtual getAabb, tolerance 0).
void HavokShapeAabb(const void* shape, const ape::Matrix34& transform, ape::Aabb& out);
// Body that owns a collidable: for collidables of type 1 (byte +0x18) the
// owner sits at the signed offset in byte +0x10; null otherwise.
void* CollidableOwnerBody(const void* collidable);
// Contact point event of the contact listener (hkpContactPointEvent):
// its two bodies (+0x04/+0x08; the properties are completed first by
// 0x009155FC when +0x20 is not set) and disabling the contact (flag 8 in
// byte +0x0F of the contact properties at +0x18).
void* ContactEventBody(const void* event, int index);
void ContactEventDisable(const void* event);
// Game user properties of a body (hkpProperty list +0x7C/+0x80): the owner
// type ("OwnerType", 0xA734004D; "Entity" = 0x7A30D7E3) and the owner
// ("OwnerPtr", 0x91C7CF5B); 0 when absent.
uint32_t RigidBodyOwnerType(void* body);
void* RigidBodyOwner(void* body);
constexpr uint32_t kBodyOwner_Entity = 0x7A30D7E3u;
// hkReferencedObject::removeReference (0x007D4EA8).
void HavokRelease(void* object);

} // namespace entity
} // namespace sly
