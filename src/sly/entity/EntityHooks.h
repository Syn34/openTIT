// openTIT - Calls from the entity layer into systems outside 0x000B0000-0x00180000.
//
// The entity code reaches into the game object base class (0x00464xxx), the
// component factory, the animation controller, the culling manager, the
// camera and the geometry renderer. Those are not ported (or are being
// ported in parallel), so every such call goes through one of these free
// functions. EntityHooks.cpp holds TODO stubs that return what the console
// returns for a missing object; each names the function it stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity {

class Entity;
class EntityComponent;
class EntityDef;
struct ComponentDesc;

// -- game object base class (vtable 0x00B219D0, 0x00464xxx) -------------------
// Base part of Entity::OnSpawn (0x00464B18).
void GameObjectSpawn(Entity* entity);
// Base part of Entity::Update / PostUpdate; false while the object is not
// active (0x00464BD4 / 0x00464CBC).
bool GameObjectUpdate(Entity* entity, float dt);
bool GameObjectPostUpdate(Entity* entity, float dt);

// -- components ----------------------------------------------------------------
// Builds a component from its description (0x002635E8). Null on failure.
EntityComponent* CreateComponent(ComponentDesc* desc);
// Offset of the "centre" component (type 0x1F) from the entity origin (0x002BC1E0).
ape::Vec3 CenterComponentOffset(EntityComponent* component);

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

// -- game world (0x00F65CA0) -------------------------------------------------
// Byte +0x1D == 1: the level runs with one player only.
bool IsSinglePlayerMode();
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
// Global 0x01032148: the physics step is running (hits are deferred).
bool PhysicsStepInProgress();
// Animation controllers are deleted through their virtual slot 1.
void AnimControllerDestroy(void* controller);
// 0x0048F298 (entity, true): removes the entity from its event listener group.
void EntityLeaveListenerGroup(Entity* entity);
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

// 0x00490518: drops whatever the game property system keeps for a definition.
void PropertySystemForgetDef(EntityDef* def);

} // namespace entity
} // namespace sly
