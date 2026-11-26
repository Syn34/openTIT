// openTIT - The small slice of the world-object API the game shell needs.
//
// World objects (entities, characters, level instances) are reconstructed by
// the world module, which is being written in parallel. The game shell only
// talks to them through these free functions; WorldInterface.cpp holds
// TODO stubs with safe defaults until the real implementations land.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {

class WorldObject;

// Name of a level object (virtual slot 3 of the level-object classes).
const char* WorldObjectName(const WorldObject* object);
// Name hash used to register an object in its owner (object->field_CC).
uint32_t WorldObjectKey(const WorldObject* object);

// World position of a placed level instance (instance->field_80).
ape::Vec3 InstancePosition(const WorldObject* instance);
// World position of a character (character->field_90).
ape::Vec3 CharacterPosition(const WorldObject* character);

// Spawned objects: bit 18 of field_AC marks an object that is currently
// alive in the world, bit 0 one that must not be removed by group resets.
bool SpawnedIsAlive(const WorldObject* object);
bool SpawnedIsPersistent(const WorldObject* object);
// Virtual slot 22 of spawned characters: kill without effects.
// NOTE: the four flag arguments of the original are (0, 1, 1, 1).
void SpawnedKill(WorldObject* object);
// Virtual slot 10: return the object to its pool.
void SpawnedRelease(WorldObject* object);

// Parent object of a placed instance (ApeObj +0x104) and whether an object
// is a spawn group object (ApeBase type 0x0C).
WorldObject* WorldObjectParent(const WorldObject* object);
bool WorldObjectIsGroup(const WorldObject* object);

// Looks up a level object instance by name hash in the loaded level.
WorldObject* FindLevelInstance(uint32_t nameHash);

// True if the point lies inside the current camera view (the original tests
// it against two rows of the camera's view-projection matrix).
bool IsPointInCameraView(const ape::Vec3& point);

// Optional gate installed in the game world (+0x1281F4, virtual slot 16);
// spawning is suppressed while it returns false. NOTE: purpose unknown.
bool SpawnGateAllows();

} // namespace sly
