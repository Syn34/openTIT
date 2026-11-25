// openTIT - Calls from the character code (0x0011F000-0x0012A000) into
// systems that are not ported yet: the character component (type 0x00,
// code 0x0024xxxx-0x0029xxxx), the game world's player and profile blocks,
// world object tinting and the director scripts. CharacterHooks.cpp holds
// TODO stubs with the console's "missing object" results; each hook names
// the function or member it stands for.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; }
namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; }

namespace sly {
namespace entity {

class Entity;
class EntityComponent;
struct SpawnParams;

// -- character component (Entity slot 15 / 16) ----------------------------------
// Model (geometry definition name) the component shows (0x00297E3C), its
// default model (+0x18) and switching to another one (0x00297EDC).
uint32_t CharacterCompModel(EntityComponent* component);
uint32_t CharacterCompDefaultModel(EntityComponent* component);
void CharacterCompSetModel(EntityComponent* component, uint32_t model);
// Puts the skeleton back into its bind pose before a model swap (0x00254814 with 0).
void CharacterCompResetPose(EntityComponent* component);
// Hands the entity's animation controller to the component (0x002980AC).
void CharacterCompSetAnimController(EntityComponent* component, void* controller);
// Physics component (type 0x00, Entity slot 16): collision group kept in the
// top half of its filter word +0x1AC, and its Havok rigid body (0x00252940).
uint32_t PhysicsCompCollisionGroup(EntityComponent* component);
void* PhysicsCompRigidBody(EntityComponent* component);
// Collision filter word of a rigid body (+0x2C): the group goes into bits
// 16-31 and the world re-filters the body (0x00930B70) when it is in one.
void RigidBodySetCollisionGroup(void* body, uint32_t group);
// Position the character aims from (+0x250) and its turn rate (+0x280).
ape::Vec3 CharacterCompAimPosition(EntityComponent* component);
float CharacterCompTurnRate(EntityComponent* component);
// Gravity factor of a rigid body's motion (an hkHalf at +0x1FE).
float RigidBodyGravityFactor(void* body);
void RigidBodySetGravityFactor(void* body, float factor);

// Resets of the physics component (slot 4, then 0x00261328 with 1) and
// enabling/disabling its body (0x00260C14).
void PhysicsCompReset(EntityComponent* component);
void PhysicsCompResetAfterSwap(EntityComponent* component);
void PhysicsCompSetDisabled(EntityComponent* component, bool disabled);
// Sends an animation event to the character component's controller (+0x24, 0x0035327C).
void CharacterCompSendAnimEvent(EntityComponent* component, uint32_t event);

// Surface axes of the physics component by its movement mode (+0x1A0 bits
// 28-30: ground, water, wall climbing through component 0x2C) (0x0026A42C /
// 0x0026D8F4).
ape::Vec3 PhysicsCompUpVector(EntityComponent* component);
ape::Vec3 PhysicsCompSideVector(EntityComponent* component);
// Animation map component (0x0E): clip registered under a name (0x00288974).
void* AnimMapFindClip(EntityComponent* animMap, uint32_t name);

// -- other components -----------------------------------------------------------
// Component 0x11: re-reads its setup after the animation controller changed (0x00293264).
void Component11Refresh(EntityComponent* component);
// Component 0x10 (weapon attachment): shows the alternate (costume) variant (0x00287318).
void Component10SetAlternate(EntityComponent* component, bool alternate);
// Component 0x10: hidden flag (byte +0x20 bit 7).
void Component10SetHidden(EntityComponent* component, bool hidden);
// Component 0x24: back to its start (0x002549F8 with 0).
void Component24Reset(EntityComponent* component);
// Component 0x15 resets its props (0x0028EE68).
void Component15Reset(EntityComponent* component);
// Component 0x15 (attached props): the world objects it owns (+0xB60, count +0xB80).
uint32_t Component15ObjectCount(EntityComponent* component);
ape::world::ApeObj* Component15Object(EntityComponent* component, uint32_t index);
// Component 0x03: bit 31 of word +0x28 (set while it holds the character). NOTE: meaning guessed.
bool Component03Busy(EntityComponent* component);

// -- world objects ----------------------------------------------------------------
// Tints every render part of a geometry instance (0x000D6F90): colour bytes
// at +0x08 and amount at +0x0C of each part's material block.
void ObjectSetTint(float amount, ape::world::ApeObj* object, const uint8_t color[4]);

// Instance of a geometry definition (0x005A05E8); `flag` NOTE: meaning unknown.
ape::world::ApeObj* CreateGeomInstance(void* geomDef, bool flag);
// Material colour of every render part of a geometry instance (floats +0x40..+0x4C).
void SetGeomMaterialColor(ape::world::ApeObj* object, const uint8_t color[4]);
// HUD colour of the character a definition describes (0x004883C0 with the
// definition's slot 4 value; opaque white when none).
void CharacterHudColor(const class EntityDef* def, uint8_t out[4]);
// Easing curve the tint blend runs through (0x004A8320 with 1 - 0x01039360).
float EaseTint(float t);
// Animation controller flag word +0x28 and its event sink (+0x3C callback, +0x40 owner).
void AnimControllerSetFlag(void* controller, uint16_t bit, bool on);
void AnimControllerSetEventSink(void* controller, Entity* owner);

// -- game world -------------------------------------------------------------------
// Profile of the local player block (game world +0x1070 + player * 0x20C80).
uint8_t PlayerProfileIndex(uint32_t player);
// Profile record (game world +0x50 + profile * 0xFD8) fields read here:
// costume flag bytes (+0x1010 + costume index, bit 1 = alternate costume),
// the selected weapon (+0x174), Murray's (+0x178) and Bentley's (+0x17C)
// and the fourth character's (+0x180) selections.
bool ProfileAlternateCostume(uint8_t profile, uint32_t costume);
uint32_t ProfileSelection(uint8_t profile, uint32_t offset);
// Unlocked-ability bits of a player in the running level (level object
// +0x128164, 0x3C-byte player records with the bits at +0x28); false when no
// level runs or the player has no record.
bool LevelPlayerHasAbility(uint32_t player, int ability);
// Ability names bought for a character (0x0049E43C).
uint32_t PurchasedAbilities(uint32_t character, uint32_t* out, uint32_t max);
// World flag lookup by name (0x0048FC68).
bool WorldFlag(uint32_t name);
// The game physics world exists (game world +0x1281D4, +0x04).
bool PhysicsWorldPresent();

// Entities ---------------------------------------------------------------------
struct EntityChildLink;
class EntityListener;
// Entity::AttachChild (0x000D15C0) with a filled link record.
void EntityAttachChild(Entity* parent, const EntityChildLink& link);
// Starts a director script on the entity (0x000DCA20) and clears an
// instance's orphan flag again (+0x88 bit 9).
ape::ApeAnimScriptInstance* EntityStartScript(Entity* entity, ape::ApeAnimScript* script, bool flag, bool attach);
void AttachAnimScript(ape::ApeAnimScriptInstance* instance);
// Player block (game world +0x1070 + player * 0x20C80): the disguise ended
// (+0x10DC cleared, flag 0x40 of +0x1070 set).
void PlayerBlockDisguiseEnded(uint32_t player);
// Time spent in the current state of a layer (the state machine's time array).
void EntitySetStateTime(Entity* entity, uint32_t layer, float time);
// Entity virtual slot 52 (+0xD0): motion of the root animation this frame.
void EntityRootMotionDelta(Entity* entity, ape::Matrix34& out, bool consume);
// Entity virtual slot 21 (+0x54): takes a new world transform.
void EntitySetWorldTransform(Entity* entity, const ape::Matrix34& transform);
// Entity virtual slot 7 (+0x1C): state of a layer.
void EntitySetState(Entity* entity, int layer, int state);
// Opacity of a child entity (0x000DF3CC) and pushing its fade buffer (0x000DF4CC).
void EntitySetFadeAlpha(Entity* entity, bool flag, uint8_t alpha, bool immediate);
void EntityApplyFadeBuffer(Entity* entity);
// Listener slot 37 (+0x94): the entity is heavy to carry; slot 51 (+0xCC):
// it was dropped, with the carrier's drop settings.
bool ListenerIsHeavy(EntityListener* listener);
void ListenerOnDropped(EntityListener* listener, const uint8_t* dropSettings);
// Per-part alpha buffer of a geometry instance (0x004A783C) and its LOD
// field (0x00583E54).
void ObjectApplyAlphaBuffer(ape::world::ApeObj* object, const void* buffer, uint8_t alpha);
void RenderObjectSetLodField(ape::world::ApeObj* object, int32_t value);
// Game world callback lists (0x0046B5F8): `fn(owner, entity)` runs later.
using WorldCallback = void (*)(void* owner, Entity* entity);
void GameWorldQueueCallback(int list, WorldCallback fn, void* owner, Entity* entity);

// Who is behind an indirect hit source: the thrower of a thrown object
// (+0x1AC), the owner of an owned one (+0x188) or the spawner (+0x190).
enum HitOwnerKind { kHitOwner_Thrower, kHitOwner_Owner, kHitOwner_Spawner };
Entity* HitSourceOwner(Entity* source, HitOwnerKind kind);
// A thrown object (type bit 5) whose throw mode (+0x1B0 bits 8-9) is 3 and
// whose thrower is `entity`.
bool ThrownBySelf(Entity* thrown, Entity* entity);
// The other body of a physics impact hit (hit +0x34, a Havok collidable
// whose owner is a rigid body), its motion type (+0xE8 == 4: keyframed) and mass.
void* HitOtherBody(const struct EntityHitRecord& hit);
bool RigidBodyIsKeyframed(void* body);
float RigidBodyMass(void* body);
// Statistics of a melee hit taken (0x000B2AE0).
void RecordMeleeHit(Entity* victim, const struct EntityHitRecord& hit);
// AI listener byte +0x131 bit 3: the character ignores damage.
bool ListenerIgnoresDamage(EntityListener* listener);
// Game world reactions to damage: a player (0x0047BC68) or a tracked NPC (0x0047C1F0).
void GameWorldPlayerDamaged(uint32_t player);
void GameWorldNpcDamaged(Entity* entity);

// What an attacking entity (type bit 1) last went for (+0x174).
Entity* AttackTargetOf(Entity* attacker);
// Scoring component (type 0x28): its value (+0x18) and paying it out (0x002B21D8).
float ScoringValue(EntityComponent* scoring);
void ScoringAward(EntityComponent* scoring, int points, bool flag);
// Score counter of a player's entity (+0x1D0 of the player 0 entity at game world +0x1078).
void AddPlayerScore(uint32_t player, int points);

// The active minigame forgets a figure of that model (0x003DE47C).
void MinigameFigureRemoved(uint32_t model);

// Word +0x14 of an actor group member's record takes the group counter.
void GroupRecordSetCounter(void* record, uint32_t counter);
// Interaction component: the entity interacting (+0x40) and the number of
// interactions in progress (+0x44).
Entity* InteractionUser(EntityComponent* interaction);
int32_t InteractionUsers(EntityComponent* interaction);
// What a character carries (Character +0x1A8).
Entity* CharacterCarried(Entity* character);
// Player block value +0x10D8 (game world +0x1070 + player * 0x20C80). NOTE: meaning unknown.
uint32_t PlayerBlockValue10D8(uint32_t player);
// Actor base version of CanInteractWith (0x000E7FF0, entity module).
bool ActorCanInteractWith(Entity* actor, Entity* other, int kind);
// Interaction component: the interaction kinds it offers (0x00266E6C).
void InteractionSetOffer(EntityComponent* interaction, uint32_t kinds);

// -- entities ---------------------------------------------------------------------
// Plays a director script on the entity (0x000DCD20 with 0, 1).
void EntityPlayDirectorScript(Entity* entity, uint32_t script);
// Attaches the entity a spawn produced with the attach record the spawn
// parameters carry (Entity::AttachChild 0x000D15C0).
void EntityAttachSpawned(Entity* parent, SpawnParams& params, Entity* child);
// Base entity definition bounds (EntityDef slot 3, 0x000CB934).
void EntityDefBaseBounds(const class EntityDef* def, ape::Aabb& out);

} // namespace entity
} // namespace sly
