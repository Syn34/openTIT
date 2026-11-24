// openTIT - Calls from the world object classes into systems outside this directory.
//
// Every function here stands for a console function or object member in another
// module (physics, navigation, audio, game events, ...). They are TODO stubs that
// return the "nothing there" value until those modules can be called directly;
// the comment on each names what it stands for.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly { namespace audio { struct SoundName; } class WindSystem; }

namespace sly {
namespace objects {

class ObjEntity;
class EntityDef;

// ---------------------------------------------------------------------------
// Physics trigger volumes (Havok hkpAabbPhantom / hkpShapePhantom on the console)
// ---------------------------------------------------------------------------
struct TriggerVolume;   // opaque: the level's phantom object

enum VolumeShapeKind : int32_t {
    kVolumeShape_Sphere = 0,
    kVolumeShape_Box    = 3,
    kVolumeShape_None   = 0x22,   // no usable shape (also the "not set up" value)
};

// Property keys stored on volumes (hkpProperty; the keys are HashNameNoCase values).
constexpr uint32_t kVolumeProp_ColFlags  = 0x08A94D07;  // "ColFlags": collision layer 0..5
constexpr uint32_t kVolumeProp_OwnerType = 0xA734004D;  // "OwnerType"
constexpr uint32_t kVolumeProp_OwnerPtr  = 0x91C7CF5B;  // "OwnerPtr": the owning entity
constexpr uint32_t kVolumeOwner_Trigger  = 0x7A30D7E3;  // OwnerType value for triggers

// hkReferencedObject::addReference / removeReference (0x007D48C4 / 0x007D4EA8).
void VolumeAddRef(TriggerVolume* volume);
void VolumeRelease(TriggerVolume* volume);
// Whether the volume is in the physics world (hkpWorldObject::m_world, +0x08).
bool VolumeIsInWorld(const TriggerVolume* volume);
// hkpWorld::addPhantom on the game's physics world (0x00931D10) / removePhantom (0x00931F00).
void VolumeAddToWorld(TriggerVolume* volume);
void VolumeRemoveFromWorld(TriggerVolume* volume);
// Collision filter word (+0x2C) and hkpWorld::updateCollisionFilterOnPhantom (0x009315F0).
void VolumeSetCollisionFilter(TriggerVolume* volume, uint32_t filterInfo);
// Property lookup (+0x7C list): value of `key`, or `fallback` when absent.
uint64_t VolumeGetProperty(const TriggerVolume* volume, uint32_t key, uint64_t fallback);
bool VolumeHasProperty(const TriggerVolume* volume, uint32_t key);
// hkpWorldObject::removeProperty (0x0093FED8) / addProperty (0x0093FE00).
void VolumeRemoveProperty(TriggerVolume* volume, uint32_t key);
void VolumeAddProperty(TriggerVolume* volume, uint32_t key, uint64_t value);
// Phantom listeners: drops every installed listener and installs the trigger's own
// (0x00928828 removePhantomListener, 0x009288D8 addPhantomListener).
void VolumeInstallListener(TriggerVolume* volume, ObjEntity* trigger);
void VolumeRemoveListener(TriggerVolume* volume, ObjEntity* trigger);
// World-space bounds of the volume (virtual +0x20 of the phantom).
ape::Aabb VolumeGetAabb(const TriggerVolume* volume);
// hkpAabbPhantom::setAabb (0x00924B90).
void VolumeSetAabb(TriggerVolume* volume, const ape::Aabb& box);
// Phantom type (virtual +0x1C): 0..2 are AABB-style phantoms, others shape phantoms.
int VolumeGetType(const TriggerVolume* volume);
// Shape of a shape phantom (+0x10): its kind (+0x08), sphere radius (+0x10) and box
// half extents (+0x20).
VolumeShapeKind VolumeGetShapeKind(const TriggerVolume* volume);
float VolumeGetSphereRadius(const TriggerVolume* volume);
ape::Vec3 VolumeGetBoxHalfExtents(const TriggerVolume* volume);
// Name of the volume (+0x78, low bit masked off).
const char* VolumeGetName(const TriggerVolume* volume);
// Transform of a shape phantom (its motion state, +0x18).
ape::Matrix34 VolumeGetTransform(const TriggerVolume* volume);
// Collidables currently overlapping the volume (virtual +0x4C fills an hkArray).
// Returns the number written to `out` (at most `capacity`).
int VolumeGetOverlaps(TriggerVolume* volume, const void** out, int capacity);
// World object of a collidable (hkpCollidable::getOwner) and its type (+0x18:
// 1 rigid body, 2 phantom).
TriggerVolume* CollidableWorldObject(const void* collidable, int* type);

// ---------------------------------------------------------------------------
// Navigation obstacles (0x000110EC AddNavObstacle / 0x000112D8 RemoveNavObstacle)
// ---------------------------------------------------------------------------
struct NavObstacleHandle { void* obstacle = nullptr; };
// Registers an obstacle box of `size` centred on `transform` (id 0xFFFF).
NavObstacleHandle NavAddObstacle(const ape::Matrix34& transform, const ape::Vec3& size,
                                 const char* name);
void NavRemoveObstacle(NavObstacleHandle& handle);
// Pending-trigger bookkeeping of entities with flags2 bit 3 (0x0012E824 add,
// 0x0012E8B0 remove): the entity remembers triggers it is queued in.
void EntityTrackTrigger(ObjEntity* entity, ObjEntity* trigger);
void EntityUntrackTrigger(ObjEntity* entity, ObjEntity* trigger);

// ---------------------------------------------------------------------------
// Game events (0x0046502C entered, 0x00465124 all-entered, 0x004650A8 left;
// queued through 0x00482D14 on the game world)
// ---------------------------------------------------------------------------
enum TriggerEventKind {
    kTriggerEvent_Entered,
    kTriggerEvent_AllPlayersEntered,
    kTriggerEvent_Left,
    kTriggerEvent_Alert,              // 0x00464F88: a player was spotted by an alert volume
};
void PostTriggerEvent(TriggerEventKind kind, ObjEntity* trigger, uint8_t playerIndex);
// Number of local players (game world byte +0x1D).
int GetPlayerCount();

// ---------------------------------------------------------------------------
// Characters, jobs, world lookups
// ---------------------------------------------------------------------------
// Whether a player character is currently disguised (0x00129894).
bool IsCharacterDisguised(const ObjEntity* character);
// Spline definition by name hash (ApeTDefList<ApeSplineDef>::Find on 0x00F8F158).
void* FindSplineDef(uint32_t nameHash);
// Job manager (0x00F64C20): the job named `jobNameHash` (0x002E1220), or null.
void* FindJob(uint32_t jobNameHash);

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------
// New reference to the sound `soundHash` of bank `bankHash` (0x00446984).
audio::SoundName* FindSoundName(uint32_t soundHash, uint32_t bankHash);
// Starts a positional sound (0x0044C474); returns the voice id (0 if none).
uint32_t PlaySound3D(audio::SoundName* sound, const ape::Vec3& position, float volume,
                     uint32_t bus);
// Moves / re-levels a playing positional voice (0x0044C758).
void SetVoice3D(uint32_t voice, const ape::Vec3& position, float volume);
// Listener position (game world block +0x128260).
ape::Vec3 GetListenerPosition();
// The local player entity (game world +0x1078).
ObjEntity* GetPlayerEntity();

// Sound zones and mix states (sly/game audio).
struct SoundZone;   // opaque
struct MixState;    // opaque
SoundZone* FindSoundZone(uint32_t nameHash);          // 0x0044C404
MixState* FindMixState(uint32_t nameHash);            // 0x0044C43C
float SoundZoneFadeDistance(const SoundZone* zone);   // zone +0x10
float SoundZoneRadius(const SoundZone* zone);         // zone +0x00
// Head of the zone's list of active triggers (+0x70), as an opaque node pointer.
void* SoundZoneActiveTrigger(const SoundZone* zone);
void SetSoundZoneActiveTrigger(SoundZone* zone, void* trigger);
void SoundZoneActivate(SoundZone* zone);              // 0x00441C68
void SoundZoneDeactivate(SoundZone* zone);            // 0x00441CB8
void SoundZoneSetWeight(SoundZone* zone, float weight); // 0x00441D18
void MixStatePush(MixState* mix, float fadeTime);     // 0x0044D5BC
void MixStatePop(MixState* mix, float fadeTime);      // 0x0044D614

// ---------------------------------------------------------------------------
// Anim scripts run by entities
// ---------------------------------------------------------------------------
// Starts `script` on the entity (0x000DCA20 with (0, 1)) and returns the
// instance; bit 9 of the instance word +0x88 pauses it.
void* EntityStartScript(ObjEntity* entity, void* script);
void ScriptInstanceSetPaused(void* instance, bool paused);

// Script variables of an entity script instance: the object held by an object
// variable (0x004A66D0) and its setter (0x004A65F0, the entity's world object).
ObjEntity* ScriptGetEntityVar(void* instance, uint32_t varName);
void ScriptSetEntityVar(void* instance, uint32_t varName, ObjEntity* entity);
// ApeAnimScriptInstance::SetStateAndUpdate by state name (0x004CF988).
void ScriptSetState(void* instance, uint32_t stateName);
// The trigger variable (type 15, name 0x70D68D3A) that scripts started by a
// trigger use to reach it.
void ScriptSetTriggerVar(void* instance, ObjEntity* trigger);

// Volume shape type (phantom virtual slot 7: 0 AABB, 1-2 shape phantoms) and
// world transform of shape phantoms (+0xA0).
int VolumeShapeType(const TriggerVolume* volume);
ape::Matrix34 VolumeWorldTransform(const TriggerVolume* volume);

// Wind fields owned by wind triggers (sly/game wind module, 0x004ACxxx):
// built from a transform and the trigger's definition (0x004ACD68), moved
// (0x004ACCD0), ticked (0x004AD7C8), entities added (0x004ADD68, returns
// false when refused) and removed (0x004AE160, `immediate`).
WindSystem* WindFieldCreate(const ape::Matrix34& transform, const EntityDef* def);
void WindFieldDestroy(WindSystem* field);
void WindFieldSetTransform(WindSystem* field, const ape::Matrix34& transform);
void WindFieldUpdate(WindSystem* field, float dt);
bool WindFieldAddEntity(WindSystem* field, ObjEntity* entity);
void WindFieldRemoveEntity(WindSystem* field, ObjEntity* entity, bool immediate);
bool WindFieldHasEntity(const WindSystem* field, const ObjEntity* entity);
void WindFieldSetEnabled(WindSystem* field, bool enabled);
// Occupant capacity of wind triggers (global 0x00F64A30).
uint16_t WindTriggerCapacity();

// Entity::SetVisibilityDeferred (0x000DD02C): visible/awake state, optionally
// applied later.
void EntitySetVisibilityDeferred(ObjEntity* entity, bool visible, bool deferred);
// Health of killable entities (+0x15C).
float EntityHealth(const ObjEntity* entity);

// Jobs (game state 0x00F64C20): a job's state index (0x002D98A4: 0
// unavailable, 1 available, 2 complete), whether a job is running (+0x28),
// whether the game state blocks new jobs (bit 29 of +0x10), whether the
// entity may start this job (0x002E4ED0), and the start itself (0x002E1E98
// with the start goal).
int JobState(const void* job);
bool GameStateHasActiveJob();
bool GameStateJobsBlocked();
bool GameStateCanStartJob(ObjEntity* entity, void* job);
void GameStateStartJob(ObjEntity* entity, void* job, uint32_t startGoal);
// Ability byte of a character (+0x290 + index): bit 7 = locked, bit 6 = shown anyway.
uint8_t CharacterAbilityState(const ObjEntity* character, int ability);
// Yes/no prompt of the game world (0x00489A60) whose answer starts the job
// (callback 0x00F39CA0).
void ShowJobPrompt(ObjEntity* entity, const char* text, const char* buttons);

// Whether the game state's text display reports the message as already
// seen (0x002D9994 on game state +0x28 with a text field of that id, kind 13).
bool TextDisplayHasSeen(uint32_t messageId);

} // namespace objects
} // namespace sly
