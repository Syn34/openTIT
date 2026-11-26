// openTIT - TODO stubs for the world-object API used by the game shell.
// Each returns the value the original produces for a missing object.
#include "sly/game/WorldInterface.h"

namespace sly {

const char* WorldObjectName(const WorldObject* object) { (void)object; return ""; }
uint32_t WorldObjectKey(const WorldObject* object) { (void)object; return 0; }
// TODO: forward to ape::world::ApeObj (parent +0x104, type bits of +0x04).
WorldObject* WorldObjectParent(const WorldObject* object) { (void)object; return nullptr; }
bool WorldObjectIsGroup(const WorldObject* object) { (void)object; return false; }

ape::Vec3 InstancePosition(const WorldObject* instance) { (void)instance; return ape::Vec3{}; }
ape::Vec3 CharacterPosition(const WorldObject* character) { (void)character; return ape::Vec3{}; }

bool SpawnedIsAlive(const WorldObject* object) { (void)object; return false; }
bool SpawnedIsPersistent(const WorldObject* object) { (void)object; return false; }
void SpawnedKill(WorldObject* object) { (void)object; }
void SpawnedRelease(WorldObject* object) { (void)object; }

WorldObject* FindLevelInstance(uint32_t nameHash) { (void)nameHash; return nullptr; }

bool IsPointInCameraView(const ape::Vec3& point) { (void)point; return false; }
bool SpawnGateAllows() { return true; }

} // namespace sly
