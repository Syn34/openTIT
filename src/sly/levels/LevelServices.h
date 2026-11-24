// openTIT - Services used by the third batch of level script code.
//
// Same idea as GameServices.h / ScriptServices.h: each free function stands
// for a console function or field in a system outside this module (named in
// its comment), and LevelServices.cpp holds TODO stubs returning the
// "missing" value until those systems are ported.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace ape { class ApeRefCounted; class ApeAnimScriptObjectVar; class ApeAnimScriptInstance; namespace xml { struct Color8; } }

namespace sly {

class WorldObject;
struct GameEvent;
struct Laser;
struct LaserField;
struct LaserGroup;

namespace levels {

// Identity transform (the console copies the global 0x01164B50).
inline ape::Matrix34 IdentityTransform()
{
    return ape::Matrix34{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                         {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
}

// ---------------------------------------------------------------------------
// Lasers (laser manager 0x00F657FC: groups of 0x70 bytes in a sorted map at
// +0x44, lasers of 0x150 bytes found through each group's map +0x4C, fields
// of 0x170 bytes in the array +0x2C).

LaserGroup* FindLaserGroup(uint32_t nameHash);
// Searches every group for a laser of that name.
Laser* FindLaser(uint32_t nameHash);
// Field whose name (0x0039FEBC) matches.
LaserField* FindLaserField(uint32_t nameHash);
// 0x0039BEFC / 0x00399758(group, enable, 0, 0).
void LaserGroupReset(LaserGroup* group);
void LaserGroupSetEnabled(LaserGroup* group, bool enable);
// 0x0039BEDC / 0x003992AC(laser, enable, 0, 0).
void LaserReset(Laser* laser);
void LaserSetEnabled(Laser* laser, bool enable);
// 0x0039D32C(field, enable, 1, `immediate`). NOTE: last flag's meaning is a guess.
void LaserFieldSetEnabled(LaserField* field, bool enable, bool immediate);

// ---------------------------------------------------------------------------
// Interaction components (EntityInteraction 0x000D951C)

void* EntityInteractionComponent(WorldObject* entity);
// Component id (+0x08) of an interaction component.
uint32_t InteractionComponentId(const void* interaction);
// 0x002B72DC: offers the interaction (to `player`, may be null).
void InteractionActivate(void* interaction, WorldObject* player);
// 0x002B719C: withdraws it.
void InteractionDeactivate(void* interaction);
// Bit 28 of the component word +0x7C.
void InteractionSetCancelAllowed(void* interaction, bool allowed);
// 0x0012D0F0: the player character starts the interaction with that id.
void CharacterBeginInteraction(WorldObject* character, uint32_t interactionId);

// ---------------------------------------------------------------------------
// World objects

// Descendant of `object` named `nameHash` (ApeObj::ForEachInHierarchy
// 0x0057F5D8 with the name matcher 0x00F2FAF8), or null.
ape::ApeRefCounted* ObjectFindDescendant(ape::ApeRefCounted* object, uint32_t nameHash);
// True for geometry instances (object kind 8, bits 10-15 of +0x04).
bool ObjectIsGeometryInstance(const ape::ApeRefCounted* object);
// World position of an object (+0x40).
ape::Vec3 ObjectWorldPosition(const ape::ApeRefCounted* object);
// Animation controller an object variable carries (+0x24, set by the
// entity code), or null.
void* ObjectVarAnimController(const ape::ApeAnimScriptObjectVar* var);

// 0x002ACAB8: the binocucom shows `material` on a geometry instance.
void BinocucomSetObjectMaterial(void* binocucom, ape::ApeRefCounted* geometry, uint32_t material);

// ---------------------------------------------------------------------------
// Animation

// 0x0034CC34: playing animation of layer `layer`, or null.
void* AnimControllerFindLayer(void* controller, int32_t layer);
// 0x00353748 length / 0x00354150 set time of a playing animation.
float AnimLength(const void* anim);
void AnimSetTime(void* anim, float time);
// Character animation edits: 0x00298EE4 begin, 0x00276A20 set time
// (normalized or seconds, layer), 0x00299674 end.
void CharacterBeginAnimEdit(void* character);
void CharacterSetAnimTime(void* character, float time, bool normalized, int32_t layer);
void CharacterEndAnimEdit(void* character);
// Entity::SetAnimElapsed (0x000DC550).
void EntitySetAnimElapsed(WorldObject* entity, float time, bool normalized, int32_t anim);

// ---------------------------------------------------------------------------
// Camera

struct SequenceShakeParams;
// 0x002F2140 on the camera manager 0x00F64C38.
void CameraAddShake(const SequenceShakeParams& params);

// ---------------------------------------------------------------------------
// Characters

// Component 0x25 of a character (EntityComponent25 0x000D95C4): special move
// set-up. Direction +0xF0, angle +0x100, enable byte +0x104.
void* CharacterSpecialComponent(WorldObject* character);
void SpecialSetDirection(void* special, const ape::Vec3& direction);
void SpecialSetAngle(void* special, float angle);
void SpecialSetEnabled(void* special, bool enabled);
// Only while the character is in its scripted animation state (bit 7 set and
// bit 6 clear of byte +0x2D4, class bit 0x8000 clear, component 0x2D
// present): switches state layer 0 to 6 (virtual slot 7) and stores the flag
// in bit 31 of the state word +0x14.
void CharacterSetSkipAnim(WorldObject* character, bool skip);

// ---------------------------------------------------------------------------
// Level objects

// Sentry lights: entity type bit 0x08000000 of +0xC4; 0x001D218C switches the
// light, bit 26 of +0x3CC locks it.
bool EntityIsSentryLight(const WorldObject* entity);
void SentryLightSetEnabled(WorldObject* light, bool enable);
void SentryLightSetLocked(WorldObject* light, bool locked);

// 0x002EF648 on the effect manager 0x00F64C38: kills the effects attached to
// an entity.
void FxKillEntityEffects(WorldObject* entity);

// ---------------------------------------------------------------------------
// Physics and components

// Rigid body of the level's physics world (game world +0x1281D4) by name
// (0x0034654C), whether it is in the Havok world (+0x08), and the Havok
// world add/remove calls (0x0092F2A8 / 0x0092FFC8).
void* PhysicsFindRigidBody(const char* name);
bool RigidBodyIsInWorld(const void* body);
void PhysicsAddRigidBody(void* body);
void PhysicsRemoveRigidBody(void* body);
// Entity virtual slot 23 (+0x5C): collision of shape `index` off/on.
void EntitySetCollisionDisabled(WorldObject* entity, bool disabled, int32_t index);

// Entity::FindComponentById (0x000D92B0), the component's enabled byte
// (+0x10) and its virtual slot 11 (enable/disable).
void* EntityFindComponentById(WorldObject* entity, uint32_t id);
bool ComponentIsEnabled(const void* component);
void ComponentSetEnabled(void* component, bool enabled);
// Entity virtual slot 19 (+0x4C): debug name.
const char* EntityDebugName(WorldObject* entity);

// ---------------------------------------------------------------------------
// Entity state

// Collection values of a character's definition (definition +0x5B8..+0x5C0
// ints, +0x5D0 and +0x5E0 vectors). NOTE: meaning of the values unknown.
struct CharacterCollectInfo {
    int32_t values[3] = {};
    ape::Vec3 vectors[2] = {};
};
CharacterCollectInfo CharacterDefCollectInfo(const WorldObject* character);

// Class flag 0x400000 (+0xC0): entities with their own enable switch
// (virtual slot 76, +0x130).
constexpr uint32_t kEntityClass_Enableable = 0x400000;
void EntitySetEnabledState(WorldObject* entity, bool enabled);
// Entity::SetVisibilityDeferred (0x000DD02C).
void EntitySetVisibilityDeferred(WorldObject* entity, bool visible, bool deferred);
// Attached children (Entity +0x154 records, child at +0xC0).
int EntityChildCount(const WorldObject* entity);
WorldObject* EntityChildAt(const WorldObject* entity, int index);
// Script state: virtual slots 33 (+0x84, get) and 32 (+0x80, set).
uint32_t EntityScriptState(WorldObject* entity);
void EntitySetScriptState(WorldObject* entity, uint32_t state, bool flag);
// Gadget objects: type bit 0x80000 of +0xC4; 0x001AE9C8 sets their state.
bool EntityIsGadgetObject(const WorldObject* entity);
void GadgetObjectSetState(WorldObject* entity, uint32_t state);
// "No target" switch: bit 30 of +0x1A4 (class flag 4), bit 7 of +0x1B0
// (class flag 0x20) and bit 15 of +0x248 (type bit 0x20000 of +0xC4),
// whichever apply to the entity.
void EntitySetNoTarget(WorldObject* entity, bool noTarget);
// Entity virtual slot 22 (+0x58).
void EntitySetVisible(WorldObject* entity, bool visible, bool immediate, bool children, bool components);

// ---------------------------------------------------------------------------
// Colours and fades

// Full-screen colour filter fades of the game world (0x1C-byte records at
// +0x128334, count at +0x1283A4).
struct ScreenFilterFade {
    uint8_t blendMode = 0;          // bits 28-31 of the first halfword
    uint8_t type = 0;               // bits 24-27
    bool disabled = false;          // bit 23
    ape::xml::Color8 colorStart{};  // +0x02
    ape::xml::Color8 colorEnd{};    // +0x06
    float intensityStart = 0.0f;    // +0x0C
    float intensityEnd = 0.0f;      // +0x10
    float duration = 0.0f;          // +0x14
    float elapsed = 0.0f;           // +0x18
};
// Takes the next free record (the count grows).
int GameWorldAllocScreenFilter();
ScreenFilterFade& GameWorldScreenFilter(int index);

// Entity virtual slot 29 (+0x74): opacity (alpha, flag, 1, 0).
void EntitySetAlpha(WorldObject* entity, uint8_t alpha, bool flag);
// Lights (object kind 9): intensity is stored as its square root (+0x178);
// a change refreshes the light (0x005913E0 unless its type +0x170 is 2) and
// its bounds (virtual slot 9). Colour: floats at +0x160.
bool ObjectIsLight(const ape::ApeRefCounted* object);
void LightSetIntensity(ape::ApeRefCounted* light, float intensity);
void LightSetColor(ape::ApeRefCounted* light, const ape::xml::Color8& color);
// Geometry instances: opacity (virtual slot 13), material by name
// (0x004A59B4) and tint colour (+0x13C).
void GeometrySetAlpha(ape::ApeRefCounted* geometry, uint8_t alpha, bool flag);
void* GeometryFindMaterial(ape::ApeRefCounted* geometry, uint32_t materialName);
void GeometrySetTint(ape::ApeRefCounted* geometry, const ape::xml::Color8& color);
// Live material with that name in the AglMaterial free list (0x00F8F114).
void* FindLiveMaterial(uint32_t nameHash);
// Material colour (floats at +0x40).
void MaterialSetColor(void* material, const ape::xml::Color8& color);
// GUI node virtual slot 14 (+0x38).
void GuiNodeSetColor(void* node, const ape::xml::Color8& color);
// Character override colour: bit 5 of +0x3C8 switches it on, colour +0x3C9.
void CharacterSetOverrideColor(WorldObject* character, bool active, const ape::xml::Color8& color);

// ---------------------------------------------------------------------------
// Gadgets, turrets, binocucom

// Bit 29 of the gadget object word +0x228. NOTE: meaning unknown.
void GadgetObjectSetFlag(WorldObject* entity, bool value);
// Gadget turrets: type bit 0x2000000 of +0xC4; 0x001CC174 adds a target,
// 0x001CC214 removes one.
bool EntityIsGadgetTurret(const WorldObject* entity);
void TurretAddTarget(WorldObject* turret, WorldObject* target);
void TurretRemoveTarget(WorldObject* turret, WorldObject* target);
// Class flag 1 (+0xC0): entities a turret can aim at.
constexpr uint32_t kEntityClass_Targetable = 0x01;
// Player index meaning "no player" in script data.
constexpr int32_t kNoPlayerIndex = 9;
// 0x002A6044: the binocucom view is up.
bool BinocucomIsVisible(void* binocucom);

// Animation controller of a character (+0x24), the controller recorded for an
// object variable (ApeAnimScriptInstance::GetObjectVarField24, 0x004CF2BC),
// and the speed of a playing animation (0x003541B0).
void* CharacterAnimController(void* character);
void* ScriptObjectAnimController(ape::ApeAnimScriptInstance* instance, uint32_t varName);
float AnimSpeed(const void* anim);

// ---------------------------------------------------------------------------
// Queries

// Class bits of an entity (64-bit word +0xB8).
uint64_t EntityClassBits(const WorldObject* entity);
// True if one of the entity's effect components (type 0x17) has effect type
// `type` (bits 25-29 of its word +0x18).
bool EntityHasEffectOfType(const WorldObject* entity, int type);
// Jobs (game state 0x00F64C20): the current job (+0x28, name at +0x1D4),
// a job by name (0x002E1220) and its state index 0..2 (0x002D98A4).
bool HasCurrentJob();
uint32_t CurrentJobName();
void* GameStateFindJob(uint32_t jobName);
int JobState(void* job);

// Respawn point of a character (+0x240, four rows) and its respawn state
// bytes (+0x280..+0x283).
struct CharacterRespawnInfo {
    ape::Matrix34 point;
    uint8_t states[4] = {};
};
CharacterRespawnInfo CharacterRespawn(const WorldObject* character);
// Safes: the game state's current safe (0x002DDD24) and its state index
// (0x002DA990).
void* GameStateCurrentSafe();
int SafeState(void* safe);

// ---------------------------------------------------------------------------
// HUD

// Health meters: a player's meter (0x0036960C, text), an entity's meter
// shown with a meter name (0x00369910) or hidden (0x00369A80).
void HudSetPlayerHealthMeter(int player, const char* text);
void HudShowHealthMeter(WorldObject* entity, uint32_t meter);
void HudHideHealthMeter(WorldObject* entity);
// 0x00367530: text of a HUD object (index of the text field).
void HudSetText(void* hud, const char* text, int32_t index);
// The game state's selected job (+0x28), or null.
void* CurrentJob();

// ---------------------------------------------------------------------------
// Movers, meters, killing

// Input movers: class flag 1 of +0xC8; 0x001DB978 sets the input points
// (value, flag), virtual slot 11 resets.
bool EntityIsInputMover(const WorldObject* entity);
void InputMoverSetPoints(WorldObject* mover, float points, bool flag);
void InputMoverReset(WorldObject* mover);
// The decibel meter entity of the MS level and its update (0x0005BB08).
constexpr uint32_t kDecibelMeterEntity = 0xCD4F65FCu;  // NOTE: name unknown
bool EntityHasListener(const WorldObject* entity);   // listener +0xF4
void DecibelMeterTrigger(WorldObject* meter);
// Health of a killable entity (+0x158) and damage (virtual slot 88, no
// source, type 0).
float EntityHealth(const WorldObject* entity);
void EntityApplyDamage(WorldObject* entity, float amount);
// Entity virtual slot 10.
void EntityRelease(WorldObject* entity, bool kill);

// Override colour of an entity, written to every colour slot its classes
// have: characters (bit 6 of +0x3C8, colour +0x3C9), class flags 0x800
// (+0x1B1), 0x200 (+0x2B1), 0x400 (+0x2F9), 0x100 (+0x699) - these only
// enable while health (+0x15C) is positive and clear their blend timer when
// disabled - and class2 flag 0x2000 (bit 7 of +0x568).
void EntityApplyOverrideColor(WorldObject* entity, bool enabled, const ape::xml::Color8& color);

// 0x002A8F18: the binocucom scans a geometry instance.
void BinocucomScanObject(void* binocucom, ape::ApeRefCounted* geometry);
// Playing animations: speed (0x003541E0), loop flag (bit 27 of +0x0C) and
// a jump to the end (0x003540BC with the global clock 0x00F64C9C).
void AnimSetSpeed(void* anim, float speed);
bool AnimIsLooping(const void* anim);
void AnimJumpToEnd(void* anim);
// Top byte of the entity flags (+0xAC): the player slot of player entities.
uint8_t EntityPlayerByte(const WorldObject* entity);

// 0x00482D14 on the game world: posts a statistics event for a player.
void GameWorldPostEvent(uint8_t player, const GameEvent& event);

// Listener (+0xF4) virtual slot 41 (+0xA4): enter `state` with a clip and
// a target name.
void EntityListenerSetState(WorldObject* entity, uint32_t state, uint32_t clip, uint32_t target, float value);
// Entity::GetComponent (0x000D1288) by type; chain components are type 0x0F.
constexpr uint32_t kComponent_Chain = 0x0F;
void* EntityFindComponentOfType(WorldObject* entity, uint32_t type);
// Chain component parameters (+0x20 bits 31/30 and 22-29, floats +0x24..+0x30,
// vectors +0x40/+0x50). NOTE: meaning of the unnamed values unknown.
struct ChainComponentParams {
    bool flag31 = false;
    bool flag30 = false;
    uint8_t iterations = 0;
    float spring = 0.0f;
    float value28 = 0.0f;
    float value2C = 0.0f;
    float angleMax = 0.0f;
    ape::Vec3 gravity{};
    ape::Vec3 vector50{};
};
ChainComponentParams& ChainParams(void* chainComponent);
// World transform of a point instance and the player respawn setter
// (0x00488298 on the game world).
ape::Matrix34 PointInstanceTransform(const WorldObject* point);
void GameWorldSetPlayerSpawn(int player, const ape::Matrix34& transform);

// 0x00254814: pauses an animation layer of a character; Entity::SetAnimPaused
// (0x000DC314) for other entities.
void CharacterPauseAnimLayer(void* character, uint32_t layer);
void EntitySetAnimPaused(WorldObject* entity, uint32_t layer, bool paused);
// Definitions: an object's definition (+0xD0), a level definition by name
// (sorted map of 0x00F8F12C), the object kind of a definition (bits 10-15 of
// +0x04), ApeObj virtual slot 11 (SetDef), and the refresh of the entity's
// render components (type 0x10, 0x00287318) after a swap.
void* ObjectDef(const ape::ApeRefCounted* object);
void* FindLevelDef(uint32_t nameHash);
int DefKind(const void* def);
void ObjectSetDef(ape::ApeRefCounted* object, void* def);
void EntityRefreshRenderComponents(WorldObject* entity);
// Entity virtual slot 18 (+0x48): swaps the weapon component; turrets then
// reset their aim (0x001CAA88).
void EntitySwapWeapon(WorldObject* entity, uint32_t component, float speed, float interval1,
                      float interval2, float angle);
void TurretResetAim(WorldObject* turret);

// Entity definitions by name (FindEntityDef 0x000C9FC8) and their name (+0x34).
void* FindEntityDefByName(uint32_t nameHash);
uint32_t EntityDefNameOf(const void* def);
// Second class word (+0xC8) of an entity.
constexpr uint32_t kEntityClass2_Vehicle800 = 0x800;    // NOTE: vehicle class with swap 0x00238270
constexpr uint32_t kEntityClass2_Vehicle2000 = 0x2000;  // NOTE: vehicle class with swap 0x0024B1E8
bool EntityHasClass2Flag(const WorldObject* entity, uint32_t mask);
// Definition swaps: characters 0x00124D58, vehicles 0x00238270 / 0x0024B1E8.
void CharacterSwapDef(WorldObject* character, void* def, bool deferred);
void VehicleSwapDef800(WorldObject* vehicle, void* def, bool deferred);
void VehicleSwapDef2000(WorldObject* vehicle, void* def, bool deferred);
// After an immediate swap: state layer 0 to state 0 (virtual slot 7) and
// 0x00261328(component 0x00, 1).
void CharacterResetAfterSwap(WorldObject* character);

// Camera instance of the loaded level by name (sorted map of 0x00F8F134).
WorldObject* FindCameraInstance(uint32_t nameHash);

// ---------------------------------------------------------------------------
// Attaching entities

// Attach record handed to Entity::AttachChild (0x000D15C0; the console
// record is EntityChildLink, 0xF0 bytes).
struct EntityAttachDesc {
    WorldObject* child = nullptr;   // +0xC0
    ape::Vec3 offset{};             // +0x80
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};  // +0x90
    uint32_t parentPart = 0;        // +0xA0
    uint32_t childPart = 0;         // +0xA8
    bool forwardAnim = false;       // +0xB0 bit 31
    bool restoreState = false;      // +0xB0 bit 25
    bool flag28 = false;            // +0xB0 bit 28. NOTE: meaning unknown
    bool flag27 = false;            // +0xB0 bit 27. NOTE: meaning unknown
    float gravity = 0.0f;           // +0xE0
};
void EntityAttach(WorldObject* parent, const EntityAttachDesc& desc);
// Entity flag 0x200 (+0xAC): the entity forwards its animation to children.
bool EntityForwardsAnimation(const WorldObject* entity);
// Entity virtual slot 12.
void EntityUpdateTransform(WorldObject* entity);
// Carry components (EntityCarry 0x000D9544): whether it can take an entity
// (0x002B91EC) and picking it up at once (0x002B9454 with a zero offset).
void* EntityCarryComponent(WorldObject* entity);
bool CarryCanTake(void* carry, WorldObject* entity);
void CarryTake(void* carry, WorldObject* entity);

} // namespace levels
} // namespace sly
