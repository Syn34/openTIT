// openTIT - Calls from the world object classes into systems outside this directory.
//
// TODO stubs: each returns the "nothing there" value until the named console
// system is ported and can be called directly.
#include "sly/objects/ObjectsHooks.h"

#include "sly/game/AudioSystem.h"
#include "sly/game/SlyGame.h"
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
void VolumeInstallListener(TriggerVolume* volume, Entity* trigger)
{
    (void)volume;
    (void)trigger;
}
void VolumeRemoveListener(TriggerVolume* volume, Entity* trigger)
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
void VolumeSetTransform(TriggerVolume* volume, const ape::Matrix34& m) { (void)volume; (void)m; }
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
void EntityTrackTrigger(Entity* entity, Entity* trigger)
{
    (void)entity;
    (void)trigger;
}
// TODO(0x0012E8B0)
void EntityUntrackTrigger(Entity* entity, Entity* trigger)
{
    (void)entity;
    (void)trigger;
}

// --- game events -------------------------------------------------------------
// TODO(0x0046502C/0x00465124/0x004650A8 trigger events, 0x00482D14 post)
void PostTriggerEvent(TriggerEventKind kind, Entity* trigger, uint8_t playerIndex)
{
    (void)kind;
    (void)trigger;
    (void)playerIndex;
}
int GetPlayerCount() { return g_game ? g_game->localPlayerCount : 1; }

// --- characters, jobs, world lookups ----------------------------------------
// TODO(0x00129894)
bool IsCharacterDisguised(const Entity* character)
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
Entity* GetPlayerEntity() { return nullptr; }

SoundZone* FindSoundZone(uint32_t nameHash) { return audio::FindSoundZone(nameHash); }
MixState* FindMixState(uint32_t nameHash) { return audio::FindMixState(nameHash); }
float SoundZoneFadeDistance(const SoundZone* zone) { return zone->fadeDistance; }
float SoundZoneRadius(const SoundZone* zone) { return zone->radius; }
float SoundZoneFadeCurve(const SoundZone* zone) { return zone->fadeCurve; }
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

// TODO(0x000DCA20) entity script instances live in the entity module.
void* EntityStartScript(Entity* entity, void* script)
{
    (void)entity;
    (void)script;
    return nullptr;
}

void ScriptInstanceSetPaused(void* instance, bool paused)
{
    (void)instance;
    (void)paused;
}

Entity* ScriptGetEntityVar(void* instance, uint32_t varName) { (void)instance; (void)varName; return nullptr; }
void ScriptSetEntityVar(void* instance, uint32_t varName, Entity* entity) { (void)instance; (void)varName; (void)entity; }
void ScriptSetState(void* instance, uint32_t stateName) { (void)instance; (void)stateName; }
void ScriptSetTriggerVar(void* instance, Entity* trigger) { (void)instance; (void)trigger; }
int VolumeShapeType(const TriggerVolume* volume) { (void)volume; return -1; }
ape::Matrix34 VolumeWorldTransform(const TriggerVolume* volume)
{
    (void)volume;
    return ape::Matrix34{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
}
// TODO(0x004ACD68) wind fields: the wind module only has the system part so far.
WindSystem* WindFieldCreate(const ape::Matrix34& transform, const EntityDef* def) { (void)transform; (void)def; return nullptr; }
void WindFieldDestroy(WindSystem* field) { (void)field; }
void WindFieldSetTransform(WindSystem* field, const ape::Matrix34& transform) { (void)field; (void)transform; }
void WindFieldUpdate(WindSystem* field, float dt) { (void)field; (void)dt; }
bool WindFieldAddEntity(WindSystem* field, Entity* entity) { (void)field; (void)entity; return false; }
void WindFieldRemoveEntity(WindSystem* field, Entity* entity, bool immediate) { (void)field; (void)entity; (void)immediate; }
bool WindFieldHasEntity(const WindSystem* field, const Entity* entity) { (void)field; (void)entity; return false; }
void WindFieldSetEnabled(WindSystem* field, bool enabled) { (void)field; (void)enabled; }
uint16_t WindTriggerCapacity() { return 0x20; }

void EntitySetVisibilityDeferred(Entity* entity, bool visible, bool deferred) { (void)entity; (void)visible; (void)deferred; }
float EntityHealth(const Entity* entity) { (void)entity; return 0.0f; }

int JobState(const void* job) { (void)job; return 0; }
bool GameStateHasActiveJob() { return false; }
bool GameStateJobsBlocked() { return false; }
bool GameStateCanStartJob(Entity* entity, void* job) { (void)entity; (void)job; return false; }
void GameStateStartJob(Entity* entity, void* job, uint32_t startGoal) { (void)entity; (void)job; (void)startGoal; }
uint8_t CharacterAbilityState(const Entity* character, int ability) { (void)character; (void)ability; return 0; }
void ShowJobPrompt(Entity* entity, const char* text, const char* buttons) { (void)entity; (void)text; (void)buttons; }

bool TextDisplayHasSeen(uint32_t messageId) { (void)messageId; return false; }

uint32_t EntityClass2(const Entity* entity) { (void)entity; return 0; }

void CameraRemoveMode(void* mode, float blendTime) { (void)mode; (void)blendTime; }
void CameraModeDelete(void* mode) { (void)mode; }

// TODO(0x0021B2FC) entity virtual slot 86 (hit reaction) not ported.
void EntityTakeHit(Entity* target, EntityHit& hit) { (void)target; (void)hit; }

} // namespace objects
} // namespace sly
