// openTIT - Calls from the entity layer into systems outside 0x000B0000-0x00180000.
//
// The entity code reaches into the game object base class (0x00464xxx), the
// component factory, the animation controller, the culling manager, the
// camera and the geometry renderer. Those are not ported (or are being
// ported in parallel), so every such call goes through one of these free
// functions. EntityHooks.cpp holds TODO stubs that return what the console
// returns for a missing object; each names the function it stands for.
#pragma once

#include <vector>

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; struct AglMesh; }
namespace ape { class ApeAnimScriptInstance; class ApeRefCounted; }

namespace ape::xml { class XmlElement; }

namespace sly {
namespace entity {

class Entity;
class EntityComponent;
class EntityDef;
struct ComponentDesc;
struct EntityInstanceDesc;

// -- game object base class (vtable 0x00B219D0, 0x00464xxx) -------------------
// Base part of Entity::OnSpawn (0x00464B18).
void GameObjectSpawn(Entity* entity);
// Base part of Entity::Update / PostUpdate; false while the object is not
// active (0x00464BD4 / 0x00464CBC).
bool GameObjectUpdate(Entity* entity, float dt);
bool GameObjectPostUpdate(Entity* entity, float dt);
// The game object base's constructor (state machine with `layers` layers and
// `dataSize` bytes of state data each; StateMachine::StateMachine 0x0045CBE8).
void GameObjectInit(Entity* entity, uint32_t layers, uint32_t dataSize);
// 0x000D2F04: the entity's world object: the placement's source object or a
// new instance of the definition's geometry, its animation controller and
// skeleton, initial transform and scale.
void EntitySetUpObject(Entity* entity, const EntityInstanceDesc& desc);
// 0x000D1B10: applies the description's attachment records (attached child
// entities, locators, effects).
void EntityApplyAttachments(Entity* entity, const EntityInstanceDesc& desc);

// -- components ----------------------------------------------------------------
// Builds a component from its description (0x002635E8). Null on failure.
EntityComponent* CreateComponent(ComponentDesc* desc);
// Offset of the "centre" component (type 0x1F) from the entity origin (0x002BC1E0).
ape::Vec3 CenterComponentOffset(EntityComponent* component);

// -- scripts ----------------------------------------------------------------------
// The entity's world object (+0xA0) as an anim script object reference.
// NOTE: the port keeps world objects and script references separate.
ape::ApeRefCounted* EntityWorldObjectRef(Entity* entity);
// 0x000CE688: world transform of skeleton bone `boneHash` of the entity
// (the entity transform when 0 or missing), after updating the world object.
ape::Matrix34 EntityBoneWorldTransform(Entity* entity, uint32_t boneHash);

// -- release ---------------------------------------------------------------------
// 0x0046B6CC on the game world: drops the entity from its 13 per-frame event
// lists.
void GameWorldForgetEntity(Entity* entity);
// Watcher virtual slot 74 (+0x128): the watched entity left the world.
void WatcherEntityReleased(void* watcher, Entity* entity);
// Spawner (entity +0xDC) virtual slot 8 (+0x20): its spawn left the world.
void SpawnerEntityReleased(void* spawner, Entity* entity);
// Clears the entity from the two global target tables (0x01019BE0, count
// 0x01019BD0, and 0x0101DBF0, count 0x0101DBE0). NOTE: owners unknown.
void ClearEntityFromTargetTables(Entity* entity);

// -- render ids --------------------------------------------------------------
// Allocates a fresh render/visibility id (0x004A2560).
uint32_t NewRenderId();
// Pushes the cull distance of an object to the cull manager at 0x00F64C38 (0x002EF7B8).
void CullManagerUpdate(ape::world::ApeObj* object);

// -- camera ------------------------------------------------------------------
// World position of the active game camera (game world +0x128168, +0x360).
ape::Vec3 GameCameraPosition();
// Transform of the active game camera's view (game world +0x128168, +0x330).
ape::Matrix34 GameCameraTransform();

// -- game world (0x00F65CA0) -------------------------------------------------
// Byte +0x1D == 1: the level runs with one player only.
bool IsSinglePlayerMode();
// Byte +0x1D: number of players in the level.
int PlayerCount();
// Entity of player `index` (world +0x1078 + index * 0x20C80), may be null.
Entity* PlayerEntity(int index);

// -- animation controller (entity +0xA4) ---------------------------------------
// Animation layer playing `anim` on the controller (0x0034CC34), or null.
void* AnimFindPlaying(void* controller, uint32_t anim);
// Length of a playing animation (0x00353748) and its current time (0x00353780).
float AnimDuration(void* playing);
float AnimElapsed(void* playing);
void AnimSetElapsed(void* playing, float time);     // 0x00354150
void AnimSetWeight(void* playing, float weight);    // 0x003541E0
// NOTE: 0x00353CC0 takes one flag; read as "paused".
void AnimSetPaused(void* playing, bool paused);
// True when the playing animation has its "finished" bit (+0x0C bit 27) set.
bool AnimIsFinished(void* playing);
// Stops a playing animation (0x00353EB0); returns the console's result.
bool AnimStop(void* playing, bool blend);

// -- world objects -----------------------------------------------------------
// Visibility of a world object (virtual slot 10 of ApeObj, +0x28).
void WorldObjectSetVisible(ape::world::ApeObj* object, bool visible, bool recurse);
// Outline / highlight bit of a geometry instance (byte +0x142 bit 7) and the
// redraw notification that follows it (virtual slot 9, +0x24).
bool GeomGetHighlight(ape::world::ApeObj* geom);
void GeomSetHighlight(ape::world::ApeObj* geom, bool on);
// Byte +0xA8 of a geometry instance. NOTE: meaning unknown (LOD index?).
uint8_t GeomByteA8(ape::world::ApeObj* geom);

// Bit 5 of byte +0x1C of the object's render node bounds entry: set hides the
// object from the renderer's culling pass. NOTE: only touched when the node
// has its +0x88 block.
void RenderNodeSetHidden(ape::world::ApeObj* object, bool hidden);
// Global byte 0x01022014. NOTE: when set, visibility changes are forwarded to
// component 0x0D as well.
bool ForwardVisibilityToComponents();
// Bit 7 of byte +0x140 of component 0x20. NOTE: read as "keep hidden".
bool ComponentHidesRender(EntityComponent* component);

// Animation controller flag word +0x28 bit 14. NOTE: cleared while the
// entity is suspended.
void AnimControllerSetRunning(void* controller, bool running);
// -- damage ------------------------------------------------------------------------
// 0x004705F8: game event from one entity to another.
void EntitySendGameEvent(Entity* sender, Entity* receiver, uint32_t event, uint32_t data, bool immediate);
// 0x00470660: script message to the listeners of `key`.
void EntityPostScriptMessage(Entity* sender, uint32_t key, uint32_t message, uint32_t argument);
// GameEvent init (0x00464ED4 kind 0x01, 0x0046528C kind 0x0B, 0x00465308
// kind 0x0C, 0x0046582C kind 0x16, 0x00465534 kind 0x1B) with the entity as source, posted through 0x00482D14 on the game
// world for `player` (0xFF = none).
void GameWorldPostEntityEvent(uint8_t player, uint32_t kind, Entity* source);
// Spawner (entity +0xDC) virtual slot 9 (+0x24): its spawn was killed.
void SpawnerEntityKilled(void* spawner, Entity* entity);
// Placement record of the world object (map 0x01013B1C keyed by object
// +0xCC): when its flag bit 1 is set, 0x0049FCC0 hands its values (+0xB4) to
// the anim script.
void ApplyPlacementToScript(ape::ApeAnimScriptInstance* script, ape::world::ApeObj* object);
// Listener (+0xF4) slot 31 (+0x7C): the listener is an ally of the player.
bool ListenerIsAlly(class EntityListener* listener);
// Object at scripted entity +0x16C: its word +0x14 records the last damage
// type. NOTE: owner unknown.
void DamageRecordSetType(void* record, uint32_t type);

// Sender of a message (EntityMessage +0x00, a 32-bit entity pointer on the
// console that the port's message record cannot hold).
Entity* MessageSenderEntity(const struct EntityMessage& msg);
// Global 0x01032148: the physics step is running (hits are deferred).
bool PhysicsStepInProgress();
// Animation controllers are deleted through their virtual slot 1.
void AnimControllerDestroy(void* controller);
// 0x0048F298 (entity, true): removes the entity from its event listener group.
void EntityLeaveListenerGroup(Entity* entity);
// 0x0048FA54: adds the entity to event listener group `group`.
void EntityJoinListenerGroup(Entity* entity, uint32_t group, bool add);
// User data tag a released world object keeps (with the entity's render id).
constexpr uint32_t kUserTag_ReleasedEntity = 0x25431D90u;
void WorldObjectSetUserData(ape::world::ApeObj* object, uint32_t tag, uint32_t value);

// Global animation clip registry (sorted map 0x0106BBE8, 0x0034A398).
void* AnimClipRegistryFind(uint32_t nameHash);

// Message handlers outside the entity module: component 0x1D's reaction to
// message 0x2FE47F7F (0x00257F10), and the two calls on the object at +0xDC
// (virtual slots 6 and 7).
void Component1DOnMessage(EntityComponent* component);
void EntityObjectDCStart(void* object);
void EntityObjectDCStop(void* object);
// ApeAnimScriptInstance::SetStateAndUpdate by state name.
void ScriptInstanceSetState(void* instance, uint32_t stateName);

// Loaders of the entity file sections outside this module (see
// LoadEntityDocument): HUD definitions (0x0036196C), animation sets
// (0x00288728), section 0xE452FC60 (0x002AD4F4), combat tables
// (0x00274CDC), section 0x1F7B9E20 (0x002AF5F0), shooting tables
// (0x002C9D0C), spawn settings (0x000D5C00) and the entity definition
// loader (0x000CA038).
void LoadHudDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
void LoadAnimationDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
void LoadDefsE452FC60(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
void LoadCombatDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
void LoadDefs1F7B9E20(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
void LoadShootingDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
void LoadSpawnDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
EntityDef* LoadEntityDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

// 0x00490518: drops whatever the game property system keeps for a definition.
void PropertySystemForgetDef(EntityDef* def);
// 0x00490360: files a definition in the game property registry (under its
// name, and under `type` when that is one of the seven registry types),
// unless it is a trigger or registryFlags bit 1 is set.
void PropertySystemRegisterDef(EntityDef* def, uint32_t type);
// The registered definitions: list 0 all, lists 1-6 one property type each.
const std::vector<EntityDef*>& PropertyRegistry(int index);

// 0x000D8C18: queues a flash/fade draw of every geometry instance under
// `object` (the instance's parts drawn again with the renderer's two
// built-in overlay materials in `color`); with `additive` the instance is
// also flagged first (0x004A6DF0) and the draw uses mode 10 instead of 3.
// NOTE: the render-side callback is 0x005A1584; the part/material mapping
// needs the mesh and material layouts of the renderer.
void QueueObjectFlash(ape::world::ApeObj* object, const uint8_t color[4], bool additive);
// 0x00443CE0 with a 3D request (flags: position, velocity): moves a
// playing voice; false when the voice has stopped.
bool VoiceUpdate3D(uint32_t voice, const ape::Vec3& position, const ape::Vec3& velocity);
// Character component around an animation update: 0x00298EE4 before,
// 0x00299674 after.
void CharacterCompBeginAnim(EntityComponent* character);
void CharacterCompEndAnim(EntityComponent* character);
// Entity slot 52 (+0xD0, base 0x000DC730): root motion step of the playing
// animation as a transform (identity without one).
ape::Matrix34 EntityRootMotionDelta(Entity* entity, float dt);
// Root motion of a playing animation layer over `dt` (0x00352F70): the
// identity unless the clip has a root track.
void AnimLayerRootMotion(void* layer, float dt, ape::Matrix34& out);
// Tint record of a renderer mesh (mesh +0x5C, optional): colour bytes at
// +0x08 and amount at +0x0C.
void MeshSetTint(ape::world::AglMesh* mesh, const uint8_t color[4], float amount);

} // namespace entity
} // namespace sly
