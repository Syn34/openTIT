// openTIT - Calls from the world object classes into systems outside this directory.
//
// TODO stubs: each returns the "nothing there" value until the named console
// system is ported and can be called directly.
#include "sly/objects/ObjectsHooks.h"

#include "sly/game/AudioSystem.h"
#include "sly/game/SoundZones.h"

#include "ape/world/ApeBase.h"

namespace sly {
namespace objects {

// --- physics volumes -------------------------------------------------------
// TODO(0x007D48C4 hkReferencedObject::addReference)
void VolumeAddRef(TriggerVolume* volume) { (void)volume; }
// TODO(0x007D4EA8 hkReferencedObject::removeReference)
void VolumeRelease(TriggerVolume* volume) { (void)volume; }
bool VolumeIsInWorld(const TriggerVolume* volume) { (void)volume; return false; }
// TODO(0x00931D10 hkpWorld::addPhantom)
void VolumeAddToWorld(TriggerVolume* volume) { (void)volume; }
// TODO(0x00931F00 hkpWorld::removePhantom)
void VolumeRemoveFromWorld(TriggerVolume* volume) { (void)volume; }
// TODO(0x009315F0 hkpWorld::updateCollisionFilterOnPhantom)
void VolumeSetCollisionFilter(TriggerVolume* volume, uint32_t filterInfo)
{
    (void)volume;
    (void)filterInfo;
}
uint64_t VolumeGetProperty(const TriggerVolume* volume, uint32_t key, uint64_t fallback)
{
    (void)volume;
    (void)key;
    return fallback;
}
bool VolumeHasProperty(const TriggerVolume* volume, uint32_t key)
{
    (void)volume;
    (void)key;
    return false;
}
// TODO(0x0093FED8 hkpWorldObject::removeProperty)
void VolumeRemoveProperty(TriggerVolume* volume, uint32_t key)
{
    (void)volume;
    (void)key;
}
// TODO(0x0093FE00 hkpWorldObject::addProperty)
void VolumeAddProperty(TriggerVolume* volume, uint32_t key, uint64_t value)
{
    (void)volume;
    (void)key;
    (void)value;
}
// TODO(0x00928828/0x009288D8 hkpPhantom remove/addPhantomListener)
void VolumeInstallListener(TriggerVolume* volume, ObjEntity* trigger)
{
    (void)volume;
    (void)trigger;
}
void VolumeRemoveListener(TriggerVolume* volume, ObjEntity* trigger)
{
    (void)volume;
    (void)trigger;
}
ape::Aabb VolumeGetAabb(const TriggerVolume* volume)
{
    (void)volume;
    ape::Aabb box;
    ape::AabbFromPoint(box, ape::world::kWorldZero);
    return box;
}
// TODO(0x00924B90 hkpAabbPhantom::setAabb)
void VolumeSetAabb(TriggerVolume* volume, const ape::Aabb& box)
{
    (void)volume;
    (void)box;
}
int VolumeGetType(const TriggerVolume* volume) { (void)volume; return -1; }
VolumeShapeKind VolumeGetShapeKind(const TriggerVolume* volume)
{
    (void)volume;
    return kVolumeShape_None;
}
float VolumeGetSphereRadius(const TriggerVolume* volume) { (void)volume; return 0.0f; }
ape::Vec3 VolumeGetBoxHalfExtents(const TriggerVolume* volume)
{
    (void)volume;
    return ape::world::kWorldZero;
}
const char* VolumeGetName(const TriggerVolume* volume) { (void)volume; return ""; }
ape::Matrix34 VolumeGetTransform(const TriggerVolume* volume)
{
    (void)volume;
    return ape::world::kWorldIdentity;
}
int VolumeGetOverlaps(TriggerVolume* volume, const void** out, int capacity)
{
    (void)volume;
    (void)out;
    (void)capacity;
    return 0;
}
TriggerVolume* CollidableWorldObject(const void* collidable, int* type)
{
    (void)collidable;
    if (type)
        *type = 0;
    return nullptr;
}

// --- navigation ------------------------------------------------------------
// TODO(0x000110EC AddNavObstacle)
NavObstacleHandle NavAddObstacle(const ape::Matrix34& transform, const ape::Vec3& size,
                                 const char* name)
{
    (void)transform;
    (void)size;
    (void)name;
    return {};
}
// TODO(0x000112D8 RemoveNavObstacle)
void NavRemoveObstacle(NavObstacleHandle& handle) { handle.obstacle = nullptr; }
// TODO(0x0012E824)
void EntityTrackTrigger(ObjEntity* entity, ObjEntity* trigger)
{
    (void)entity;
    (void)trigger;
}
// TODO(0x0012E8B0)
void EntityUntrackTrigger(ObjEntity* entity, ObjEntity* trigger)
{
    (void)entity;
    (void)trigger;
}

// --- game events -------------------------------------------------------------
// TODO(0x0046502C/0x00465124/0x004650A8 trigger events, 0x00482D14 post)
void PostTriggerEvent(TriggerEventKind kind, ObjEntity* trigger, uint8_t playerIndex)
{
    (void)kind;
    (void)trigger;
    (void)playerIndex;
}
int GetPlayerCount() { return 1; }

// --- characters, jobs, world lookups ----------------------------------------
// TODO(0x00129894)
bool IsCharacterDisguised(const ObjEntity* character)
{
    (void)character;
    return false;
}
// TODO(0x007335C4 ApeTDefList<ApeSplineDef>::Find)
void* FindSplineDef(uint32_t nameHash)
{
    (void)nameHash;
    return nullptr;
}
// TODO(0x002E1220)
void* FindJob(uint32_t jobNameHash)
{
    (void)jobNameHash;
    return nullptr;
}

// --- audio -------------------------------------------------------------------
audio::SoundName* FindSoundName(uint32_t soundHash, uint32_t bankHash)
{
    return audio::CreateSoundName(soundHash, bankHash);
}

// TODO(0x0044C474)
uint32_t PlaySound3D(audio::SoundName* sound, const ape::Vec3& position, float volume,
                     uint32_t bus)
{
    (void)sound;
    (void)position;
    (void)volume;
    (void)bus;
    return 0;
}
// TODO(0x0044C758)
void SetVoice3D(uint32_t voice, const ape::Vec3& position, float volume)
{
    (void)voice;
    (void)position;
    (void)volume;
}
ape::Vec3 GetListenerPosition() { return ape::world::kWorldZero; }
ObjEntity* GetPlayerEntity() { return nullptr; }

SoundZone* FindSoundZone(uint32_t nameHash) { return audio::FindSoundZone(nameHash); }
MixState* FindMixState(uint32_t nameHash) { return audio::FindMixState(nameHash); }
float SoundZoneFadeDistance(const SoundZone* zone) { return zone->fadeDistance; }
float SoundZoneRadius(const SoundZone* zone) { return zone->radius; }
void* SoundZoneActiveTrigger(const SoundZone* zone) { return zone->activeTriggers; }
void SetSoundZoneActiveTrigger(SoundZone* zone, void* trigger) { zone->activeTriggers = trigger; }
void SoundZoneActivate(SoundZone* zone) { audio::ActivateSoundZone(zone); }
void SoundZoneDeactivate(SoundZone* zone) { audio::DeactivateSoundZone(zone); }
void SoundZoneSetWeight(SoundZone* zone, float weight) { audio::SetSoundZoneWeight(zone, weight); }
// TODO(0x0044D5BC)
void MixStatePush(MixState* mix, float fadeTime)
{
    (void)mix;
    (void)fadeTime;
}
// TODO(0x0044D614)
void MixStatePop(MixState* mix, float fadeTime)
{
    (void)mix;
    (void)fadeTime;
}

} // namespace objects
} // namespace sly
