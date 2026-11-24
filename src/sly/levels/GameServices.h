// openTIT - Game systems used by the level and script code of this module.
//
// The level scripts and level objects reach into entities, characters, the
// player, the HUD, audio and the camera. Those systems live in other address
// ranges that are not ported yet (or are being written in parallel), so this
// module talks to them only through the free functions declared here.
// GameServices.cpp holds TODO stubs that return what the original returns
// for a missing object; each names the console function it stands for.
//
// Entities are the game's world objects (sly::WorldObject, opaque here).
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeRefCounted; }

namespace ape { namespace xml { class XmlElement; } class ApeAnimScriptInstance; class ApeAnimScriptVar; }

namespace sly {

class WorldObject;
struct TextField;
namespace audio { struct MixState; struct SoundName; }

namespace levels {

// ---------------------------------------------------------------------------
// Entities

// Live entity with that name hash (entity list 0x01013B34; an entity counts
// while bit 18 of its flags +0xAC is set; name hash at +0xA8).
WorldObject* FindEntity(uint32_t nameHash);
// Name hash of an entity (+0xA8).
uint32_t EntityName(const WorldObject* entity);
// Name hash of the entity's definition (definition +0x34).
uint32_t EntityDefName(const WorldObject* entity);
// True while the entity is alive in the world (bit 18 of +0xAC).
bool EntityIsAlive(const WorldObject* entity);
// True for character entities (bit 3 of the class flags +0xC0).
bool EntityIsCharacter(const WorldObject* entity);
// Class flag test on +0xC0 (bit meaning partly unknown; see callers).
bool EntityHasClassFlag(const WorldObject* entity, uint32_t mask);
// The entity's world object (+0xA0).
ape::ApeRefCounted* EntityWorldObject(const WorldObject* entity);
// Entity that owns a world object, searching the object (and, with
// `searchParents`, its parents, else its attach links) for the "pEntity"
// tag (0x000D6AA0).
WorldObject* EntityFromWorldObject(ape::ApeRefCounted* object, bool searchParents, bool searchLinks);
// Point instance of the loaded level with that name hash (instance list 0x00F8F154).
WorldObject* FindPointInstance(uint32_t nameHash);
// Attaches user data to a world object (+0x114 tag, +0x118 pointer).
void WorldObjectSetUserData(WorldObject* object, uint32_t tag, void* data);
// World object user data: tag (+0x114) and pointer (+0x118).
uint32_t WorldObjectUserTag(const ape::ApeRefCounted* object);
void* WorldObjectUserData(const ape::ApeRefCounted* object);

// Class flag bits (+0xC0) seen in the scripts.
constexpr uint32_t kEntityClass_Character = 0x08;
constexpr uint32_t kEntityClass_Killable = 0x02;  // NOTE: entities with health (virtual slot 80 = IsDead)
constexpr uint32_t kEntityClass_20 = 0x20;   // NOTE: meaning unknown

// Virtual slot 80 of killable entities. NOTE: read as "is dead".
bool EntityIsDead(WorldObject* entity);

// Character interface of an entity (virtual slot 15), null for plain entities.
void* EntityCharacter(WorldObject* entity);
// Animation layer weight of a character (0x00276E4C) / plain entity (0x000DBD7C,
// recursive over the entity's sub-entities).
void CharacterSetAnimLayerWeight(void* character, float weight, int32_t layer);
void EntitySetAnimLayerWeight(WorldObject* entity, float weight, int32_t layer);

// Attaching characters to entities.
// NOTE: tests bits 7 (set) and 6 (clear) of the character byte +0x2C0.
bool CharacterCanAttach(const WorldObject* character);
// 0x00121FA8: optional part name, offset and rotation quaternion.
void CharacterAttachTo(WorldObject* character, WorldObject* target, const uint32_t* part,
                       const ape::Vec3* offset, const ape::Vec4* rotation, bool ignoreRotation);
// Sets the 2-bit mode in bits 8-9 of the entity halfword +0x1B0. NOTE: meaning unknown.
void EntitySetAttachMode(WorldObject* entity, int mode);
// 0x0011F0E4 (value, flag). NOTE: called with (0, true) right after attaching.
void CharacterBlendMotion(WorldObject* character, float value, bool flag);

// ---------------------------------------------------------------------------
// The game world singleton (0x00F65CA0)

// The player's entity (world +0x1078), may be null.
WorldObject* PlayerEntity();
// The game camera entity (world +0x1281E0), may be null.
WorldObject* GameCamera();

// Camera entity calls.
void CameraSetEase(WorldObject* camera, float value);          // 0x000FA660
void CameraSetEaseRate(WorldObject* camera, float rate);       // camera +0x254
void CameraPlayAnim(WorldObject* camera, uint32_t anim, float speed, bool loop,
                    const void* owner, bool flag);              // 0x000FCE18
void CameraStopAnim(WorldObject* camera, bool removeAnimation); // 0x000FCFDC

// Player gadgets.
void* PlayerBinocucom(WorldObject* player);   // 0x000D94CC
void BinocucomClose(void* binocucom);         // 0x002AB8F0

// Entity this entity is attached to (+0x150), or null.
WorldObject* EntityAttachParent(const WorldObject* entity);
// Detaches `child` from `parent` (0x000D72C0).
void EntityDetach(WorldObject* parent, WorldObject* child);

// Game state object at 0x00F64C20: entity notification (0x002E2B34).
// NOTE: what the game state does with the name is unknown.
void GameStateNotifyEntity(uint32_t entityName);
// Clears the halfword +0x2C of the game object at 0x00F64C38. NOTE: meaning unknown.
void ClearGameCounter();

// Game world byte +0x12828D. NOTE: meaning unknown.
void SetWorldFlag12828D(bool value);
// Global byte 0x010AF1BC. NOTE: meaning unknown.
void SetGlobalSwitch010AF1BC(bool value);
// Darkening screen effect (0x0048C6EC on the game world).
void SetDarkScreen(bool enable, float distance, float ramp, float minValue);

// Option bytes of the game world (+0x2C, +0x2D).
uint8_t GameWorldFlags2C();
uint8_t GameWorldFlags2D();

// Script text messages.
// Parses a text element into `text`; false if the element is not a text (0x0049EC08).
bool TextFieldParse(ape::xml::XmlElement* element, TextField* text);
// True when the game state has a text display (game state +0x28).
bool TextDisplayAvailable();
void TextDisplayShow(const TextField& text);                                  // 0x002D9994
void TextFieldNotify(ape::ApeAnimScriptInstance* instance, const TextField& text); // 0x0049F080

// Trophies by award name (name map 0x00F64624, 0x000B27B4).
void AwardTrophyByName(uint32_t awardName);

// HUD objects (0x003568E0 lookup by name).
void* FindHudObject(uint32_t nameHash);
void HudPlayAnim(void* hud, int mode, uint32_t anim, bool loop);  // 0x0036C124
void HudSetAnimMode(void* hud, uint32_t mode);                    // 0x003676DC
void HudSetAnimRate(void* hud, float rate);                       // 0x00367758
// Script instance driving a HUD object (+0x14), may be null.
ape::ApeAnimScriptInstance* HudScriptInstance(void* hud);

// Collectables: id of the collectable named `nameHash` (sorted map at
// 0x00F65BE8+4), 0 if none; game state query 0x002E6B8C(id, 0).
uint32_t FindCollectableId(uint32_t nameHash);
int GameStateCollectableQuery(uint32_t id);
// Notification flag of a player (0x000AE35C on the player's record).
bool PlayerNotifyFlagIsSet(int playerIndex, uint32_t flag);

// Camera weight blend on the game world (0x00484488).
void CameraBlendWeight(float weight, float duration, float exponent);
// Stops the cut-scene sequence with that name (0x0042C00C).
void KillSequence(uint32_t name);
// Levels: name of the loaded level (world +0x128114), restart (0x004896F8)
// and change (0x004674C4).
uint32_t CurrentLevelName();
void RestartLevel();
void ChangeLevel(uint32_t levelName);

// Game events (payload: entity pointer or raw 32-bit value).
void SendGameEvent(WorldObject* sender, WorldObject* receiver, uint32_t event, uintptr_t data,
                   bool immediate);                                                     // 0x004705F8
void BroadcastGameEvent(WorldObject* sender, uint32_t group, uint32_t event, uintptr_t data); // 0x00470660
void BroadcastGameEventInRange(WorldObject* sender, uint32_t group, uint32_t event, uintptr_t data,
                               float range);                                            // 0x0048F90C
// Child entity by name (0x000DB290).
WorldObject* EntityFindChild(WorldObject* entity, uint32_t childName);
// Value of a game entity variable (script variable type 15, +0x14).
WorldObject* EntityVarValue(ape::ApeAnimScriptVar* var);

// Goals (goal manager at 0x00F64C20, 0x002E56F4).
void GoalSetComplete(uint32_t goal, bool complete, bool notify);

// Global system switched on by script command 0xC495712B (0x002E4A0C with
// the object at 0x00F64C20 and 1). NOTE: system unknown.
void GlobalToggleOn();

// ---------------------------------------------------------------------------
// Audio

// Mix state with that name hash (0x0044C43C), null if none.
const audio::MixState* FindMixState(uint32_t nameHash);
// New reference to the sound `soundHash` of bank `bankHash` (0x00446984).
audio::SoundName* FindSoundName(uint32_t soundHash, uint32_t bankHash);

} // namespace levels
} // namespace sly
