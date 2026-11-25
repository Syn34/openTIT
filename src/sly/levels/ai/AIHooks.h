// openTIT - Calls the level AI makes into systems outside this directory.
//
// Same convention as sly/levels/GameServices.h: each hook names the console
// function, virtual slot or field it stands for; the stubs in AIHooks.cpp
// return the "nothing there" value until the owning module provides it.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; class ApeRefCounted; }

namespace sly {
class WorldObject;
namespace entity { class Entity; class EntityComponent; struct EntityMessage; }
namespace hud { class HudObject; }

namespace levels {
struct AIProxyPoint;

namespace ai { class AIController; }

namespace ai {

using entity::Entity;
using entity::EntityComponent;

// The levels code refers to game entities through the opaque `WorldObject`
// (sly/game/WorldInterface.h); the AI works on entity::Entity. Both name the
// same console objects.
inline Entity* AsEntity(WorldObject* object) { return reinterpret_cast<Entity*>(object); }
inline WorldObject* AsWorldObject(Entity* entity) { return reinterpret_cast<WorldObject*>(entity); }

// -- level object properties ---------------------------------------------------
// A placed level object carries the values of the properties of its property
// class ("SAIProxyPt_", "SAINavJumpLink_", the AI settings objects, ...). On
// the console the block's value array (+0x0C) is indexed by the slot number of
// the property's attribute (attribute +0x40). The properties used by the AI
// are listed here under their registered names; the comment gives the
// property descriptor global.
class PropertyBlock;

enum class AIProp : uint16_t {
    // "SAIProxyPt_" (0x0043B800)
    ProxyPt_Anim,           // 0x010AAF7C string
    ProxyPt_Script,         // 0x010AAF94 string
    ProxyPt_PathType,       // 0x010AAFAC enum hash (Loop / PingPong / Random)
    ProxyPt_Filter,         // 0x010AAFC4 enum hash (Any / Heavy / Light / Special)
    ProxyPt_Reversible,     // 0x010AAFDC bool
    // "SAINavJumpLink_" (0x0043B450)
    JumpLink_LinkType,      // 0x010AADCC enum hash (PointLink / StripLink / PlatformLink)
    JumpLink_Object1,       // 0x010AAEEC .. 0x010AAF64 strings (Object1..Object6)
    JumpLink_Object2,
    JumpLink_Object3,
    JumpLink_Object4,
    JumpLink_Object5,
    JumpLink_Object6,
    // Critter settings (0x0001CE40)
    Critter_BoundingRadius,       // 0x00F94784 float
    Critter_ScaleMin,             // 0x00F9479C float
    Critter_ScaleMax,             // 0x00F947B4 float
    Critter_AlwaysUseBoundingRadius, // 0x00F947CC bool
    Critter_AvoidanceRadius,      // 0x00F947E4 float
    Critter_IsFlyer,              // 0x00F947FC bool
    Critter_UseFlightAnims,       // 0x00F94814 bool
    Critter_OrientYUp,            // 0x00F9482C bool
    Critter_UseNavigation,        // 0x00F94844 bool
    Critter_UseAvoidance,         // 0x00F9485C bool
    Critter_UsePhysics,           // 0x00F94874 bool
    Critter_SnapToGround,         // 0x00F9488C bool
    Critter_PerchOnRails,         // 0x00F948A4 bool
    Critter_IdleDelayMin,         // 0x00F948BC float
    Critter_IdleDelayMax,         // 0x00F948D4 float
    Critter_POIDelayMin,          // 0x00F948EC float
    Critter_POIDelayMax,          // 0x00F94904 float
    Critter_QuickTurnDelayMin,    // 0x00F9491C float
    Critter_QuickTurnDelayMax,    // 0x00F94934 float
    Critter_QuickTurnAngleMin,    // 0x00F9494C float
    Critter_QuickTurnAngleMax,    // 0x00F94964 float
    Critter_POITargetDist,        // 0x00F9497C float
    Critter_FleePlayerDist,       // 0x00F94994 float
    Critter_StopFleeingPlayerDist, // 0x00F949AC float
    Critter_ChargePlayerDist,     // 0x00F949C4 float
    Critter_AttackPlayerDist,     // 0x00F949DC float
    Critter_NoticePlayerDist,     // 0x00F949F4 float
    Critter_SpeedScale,           // 0x00F94A0C float
    Critter_AttackBone,           // 0x00F94A24 hash
    Critter_IdleAnimClip,         // 0x00F94A3C hash
    Critter_SpawnAnimClip,        // 0x00F94A54 hash
    Critter_FleeAnimClip,         // 0x00F94A6C hash
    Critter_SpawnPosIsDst,        // 0x00F94A84 bool
    Critter_OneHitKill,           // 0x00F94A9C bool
    // Player buddy settings (0x00023F3C)
    Buddy_FollowDist,             // 0x00F94AB4 float
    Buddy_AttackDist,             // 0x00F94ACC float
    Buddy_Murray,                 // 0x00F94AE4 bool
    Buddy_Bentley,                // 0x00F94AFC bool
    // Enemy settings (0x0002E4FC)
    Enemy_EntityDefName,           // 0x00F94B14 string
    Enemy_Awareness,               // 0x00F9509C bool
    Enemy_RespondToDistressCall,   // 0x00F950B4 bool
    Enemy_NoTarget,                // 0x00F94B2C bool
    Enemy_CombatAlways,            // 0x00F94B44 bool
    Enemy_Health,                  // 0x00F94B5C float
    Enemy_Heavy,                   // 0x00F94B74 bool
    Enemy_RatTrooper,              // 0x00F94B8C bool
    Enemy_Turret,                  // 0x00F94BA4 bool
    Enemy_TurretAttackMinTime,     // 0x00F94BBC float
    Enemy_TurretAttackMaxTime,     // 0x00F94BD4 float
    Enemy_Cover,                   // 0x00F94BEC bool
    Enemy_Patrol,                  // 0x00F94C04 bool
    Enemy_PatrolIdleMinTime,       // 0x00F94C1C float
    Enemy_PatrolIdleMaxTime,       // 0x00F94C34 float
    Enemy_PatrolInteractionMinTime, // 0x00F94C4C float
    Enemy_PatrolInteractionMinDist, // 0x00F94C64 float
    Enemy_PatrolInteractionMaxDist, // 0x00F94C7C float
    Enemy_IdleAnimClip,            // 0x00F94C94 hash
    Enemy_SpawnAnimClip,           // 0x00F94CAC hash
    Enemy_MovementAnim,            // 0x00F94CC4 hash
    Enemy_CombatAttackCloseAnim,   // 0x00F94CDC hash
    Enemy_CombatAttackMidAnim,     // 0x00F94CF4 hash
    Enemy_CombatAttackFarAnim,     // 0x00F94D0C hash
    Enemy_CombatAttackRangedAnim,  // 0x00F94D24 hash
    Enemy_CombatOtherAnim,         // 0x00F94D3C hash
    Enemy_CombatMovementAnim,      // 0x00F94D54 hash
    Enemy_Scale,                   // 0x00F94D6C float
    Enemy_OptimalTargetRangeMin,   // 0x00F94D84 float
    Enemy_OptimalTargetRangeMax,   // 0x00F94D9C float
    Enemy_CloseAttackRange,        // 0x00F94DB4 float
    Enemy_CloseAttackAngle,        // 0x00F94DCC float
    Enemy_CloseAttackHeightMin,    // 0x00F94DE4 float
    Enemy_CloseAttackHeightMax,    // 0x00F94DFC float
    Enemy_MidAttackRange,          // 0x00F94E14 float
    Enemy_MidAttackAngle,          // 0x00F94E2C float
    Enemy_MidAttackHeightMin,      // 0x00F94E44 float
    Enemy_MidAttackHeightMax,      // 0x00F94E5C float
    Enemy_FarAttackRange,          // 0x00F94E74 float
    Enemy_FarAttackAngle,          // 0x00F94E8C float
    Enemy_FarAttackHeightMin,      // 0x00F94EA4 float
    Enemy_FarAttackHeightMax,      // 0x00F94EBC float
    Enemy_RangedAttackRange,       // 0x00F94ED4 float
    Enemy_RangedAttackAngle,       // 0x00F94EEC float
    Enemy_RangedAttackHeightMin,   // 0x00F94F04 float
    Enemy_RangedAttackHeightMax,   // 0x00F94F1C float
    Enemy_RangedAttackChaseFrequency, // 0x00F94F34 float
    Enemy_CounterAttackRange,      // 0x00F94F4C float
    Enemy_SearchRange,             // 0x00F94F64 float
    Enemy_DodgeRate,               // 0x00F94F7C float
    Enemy_TauntRate,               // 0x00F94F94 float
    Enemy_StunTime,                // 0x00F94FAC float
    Enemy_SleepTime,               // 0x00F94FC4 float
    Enemy_ThrowDamage,             // 0x00F94FDC float
    Enemy_KnockbackDamage,         // 0x00F94FF4 float
    Enemy_TorsoIKAngleOffset,      // 0x00F9500C float
    Enemy_AttackControllerUseTimer, // 0x00F95024 bool
    Enemy_AttackRequiresLOS,       // 0x00F9503C bool
    Enemy_PlayerDamageOnly,        // 0x00F95054 bool
    Enemy_StealthKillAttachTest,   // 0x00F9506C bool
    Enemy_InitialAttackTime,       // 0x00F95084 float
    Enemy_LoseTargetTime,          // 0x00F950CC float
    Enemy_LoseTargetDist,          // 0x00F950E4 float
    Enemy_TooCloseCylinderRadius,  // 0x00F950FC float
    Enemy_TooCloseCylinderHeight,  // 0x00F95114 float
    Enemy_LightCylinderRadius,     // 0x00F9512C float
    Enemy_LightCylinderHeight,     // 0x00F95144 float
    Enemy_AudioAlertRadius,        // 0x00F9515C float
    Enemy_AudioAlertHeightMin,     // 0x00F95174 float
    Enemy_AudioAlertHeightMax,     // 0x00F9518C float
    Enemy_AudioAlertInvestigate,   // 0x00F951A4 bool
    Enemy_CallForHelpDist,         // 0x00F951BC float
    Enemy_CallForHelpHeightMin,    // 0x00F951D4 float
    Enemy_CallForHelpHeightMax,    // 0x00F951EC float
    Enemy_VisionDistance,          // 0x00F95204 float
    Enemy_VisionDistanceCombat,    // 0x00F9521C float
    Enemy_VisionFOVAngle,          // 0x00F95234 float
    Enemy_VisionHeightMin,         // 0x00F9524C float
    Enemy_VisionHeightMax,         // 0x00F95264 float
    Enemy_VisionHeightsRelativeToBase, // 0x00F9527C bool
    Enemy_VisionAttachBone,        // 0x00F95294 hash
    // Sentry settings (0x0003BE60)
    Sentry_SpawnAnimClip,          // 0x00F952AC hash
    Sentry_IdleAnimClip,           // 0x00F952C4 hash
    Sentry_FormationOffset,        // 0x00F952E0 vec3
    Sentry_Awareness,              // 0x00F95320 bool
    Sentry_AudioAlertRadius,       // 0x00F95338 float
    Sentry_AudioAlertWhileALertFail, // 0x00F95350 bool
    Sentry_AudioAlertHeightMin,    // 0x00F95368 float
    Sentry_AudioAlertHeightMax,    // 0x00F95380 float
    Sentry_VisionDistance,         // 0x00F95398 float
    Sentry_VisionFOVAngle,         // 0x00F953B0 float
    Sentry_VisionHeightMin,        // 0x00F953C8 float
    Sentry_VisionHeightMax,        // 0x00F953E0 float
    Sentry_VisionAttachBone,       // 0x00F953F8 hash
    Sentry_SneakAllowed,           // 0x00F95410 bool
    Sentry_DetectionCylinderRadius, // 0x00F95428 float
    Sentry_DetectionCylinderHeight, // 0x00F95440 float
    Sentry_DetectionCylinderYOffset, // 0x00F95458 float
    Sentry_AlwaysApplyGravity,     // 0x00F95470 bool
    Sentry_OrientToTerrain,        // 0x00F95488 bool
};

const char* PropString(const PropertyBlock* block, AIProp prop);
// 0x004F7128: adds a property to a setting class (see AISettingClasses.h).
void SettingClassAddProperty(class AISettingClass* settingClass, AIProp prop);
uint32_t PropHash(const PropertyBlock* block, AIProp prop);   // enum properties store the hash
bool PropBool(const PropertyBlock* block, AIProp prop);
float PropFloat(const PropertyBlock* block, AIProp prop);

// Hashes of the enum values used above.
constexpr uint32_t kPathType_PingPong = 0x152D006Au;
constexpr uint32_t kPathType_Random = 0x8FA5D8C3u;

// Level object holding the properties of a setting, by name (the scene's
// named setting map, 0x00F658D4 +0x20), or null.
const PropertyBlock* FindSettingObject(uint32_t nameHash);
// Character spawners (spawner slot 5) carry up to three entity definition
// names (+0x68..+0x70) with the AI setting object to use for each
// (+0x78..+0x80). False when `entity` was not made by a character spawner.
bool CharacterSpawnerSettings(const Entity* entity, uint32_t defNames[3],
                              const PropertyBlock* settings[3]);

// Spawner of an entity (entity +0xDC): bounds of the definition of its
// alternative spawn object (0x00451E08, object +0xD0 box), false without one;
// and its "IsSpawnField" setting (0x010AB0E8).
bool SpawnerAltObjectBounds(const Entity* entity, ape::Vec3& boxMin, ape::Vec3& boxMax);
bool SpawnerIsSpawnField(const Entity* entity);
bool HasSpawner(const Entity* entity);
// The spawner that created an entity (entity +0xDC), opaque here, and its
// list of live spawned objects (spawner +0x14/+0x18).
struct SpawnerRef;
SpawnerRef* EntitySpawner(const Entity* entity);
int SpawnerLiveCount(const SpawnerRef* spawner);
Entity* SpawnerLive(const SpawnerRef* spawner, int index);
void SpawnerSwapLive(SpawnerRef* spawner, int a, int b);
// Race state a spawner keeps for the racers it spawns (spawner +0x50), or
// null.
class RaceTracker;
RaceTracker* SpawnerRaceTracker(const SpawnerRef* spawner);
// 0x0048FA54: registers the entity in an event listener group.
void AddToListenerGroup(Entity* entity, uint32_t group, bool add);

// Sender of an entity message (EntityMessage +0x00, an entity pointer on
// the console).
Entity* MessageSender(const entity::EntityMessage& msg);

// -- entities --------------------------------------------------------------------
// Entity virtual slot 13 (+0x34): per-frame update of the entity itself.
void EntityTick(Entity* entity, float dt);
// Entity virtual slot 21 (+0x54): moves the entity to a new world transform.
void EntitySetTransform(Entity* entity, const ape::Matrix34& transform);
// Entity virtual slot 24 (+0x60): NOTE: toggled on while the AI holds a
// target, off when it lets go.
void EntitySetEngaged(Entity* entity, bool engaged);
// Entity virtual slot 32 (+0x80) with mode 0x1D6F9964: NOTE: resets the
// entity's animation mode; called on every state change.
void EntityResetAnimMode(Entity* entity);
// Entity virtual slot 52 (+0xD0): steps the root motion of the playing
// animation by `dt`, giving the delta transform.
void EntityRootMotion(Entity* entity, float dt, ape::Matrix34& delta);
// Scale of the entity's world object (object +0x90), (1,1,1) without one.
ape::Vec3 EntityObjectScale(const Entity* entity);
// Animation controller (+0xA4) flag word +0x28: bit 10 "free", bit 12
// "engaged" (only one of the two is set).
void AnimControllerSetEngaged(Entity* entity, bool engaged);
// Character entity: class bit 14 (+0xB8) and the AI hold on it (0x00129B58);
// its character component's +0x17C play rate is reset to 1.
void CharacterClearAIHold(Entity* character);

// Entity virtual slot 36 (+0x90): the Havok rigid body of the entity's
// physics component, or null.
void* EntityRigidBody(Entity* entity);
// Game world halfword +0x128214. NOTE: non-zero keeps critters from
// charging (a game mode flag; meaning unknown).
bool CrittersMayNotCharge();
// Spawner registered under a name (spawner map 0x00F65BE8 +0x04), or null.
void* FindSpawnerByName(uint32_t nameHash);
// -- physics body (entity virtual slot 16) ------------------------------------------
bool BodyIsEnabled(EntityComponent* body);                // 0x002697F8
void BodySetEnabled(EntityComponent* body, bool enabled); // 0x00260C14
void BodySync(EntityComponent* body);                     // 0x0026D9D4
ape::Vec3 BodyPosition(EntityComponent* body);            // +0x1E0
// Rotation of the body as a transform without translation (+0x200).
ape::Matrix34 BodyRotation(EntityComponent* body);
// Motion type bits 28-30 of +0x1A0 (1 = keyframed).
int BodyMotionType(EntityComponent* body);
// Virtual slot 2 (+0x08) returns the body's motion request; the AI fills in a
// linear velocity, clears the angular part and marks it pending (bit 25 of
// word +0xA8).
void BodyRequestVelocity(EntityComponent* body, const ape::Vec3& velocity);
// 0x0026D824: normal of the ground under the body.
ape::Vec3 BodyGroundNormal(EntityComponent* body);
// Hold request: zero velocity, identity orientation, factor +0x80 = 1 and
// flag bit 26 instead of bit 25 (word +0xA8).
void BodyRequestHold(EntityComponent* body);
// Same request with zero velocity and the given orientation (identity
// quaternion on the console).
void BodyRequestStop(EntityComponent* body);

// 0x000DAABC: world transform of the named bone of the entity's skeleton
// (the entity transform when the bone is missing).
ape::Matrix34 EntityBoneTransform(Entity* entity, uint32_t boneHash);
// 0x000DAEC8: switches the entity's anim script (slot 68) to a state.
void EntitySetScriptState(Entity* entity, uint32_t stateHash);
// Entity virtual slot 88 (+0x160): damage.
void EntityDamage(Entity* entity, float amount, Entity* source, int type);
// 0x00254040(component, 0) on component type 0x24. NOTE: purpose unknown;
// the buddy calls it when it leaves its idle and dead states.
void Component24Reset(EntityComponent* component);
// Physics body request (slot 2) flag word +0xA8: clears bit 31.
void BodyClearRequestBit31(EntityComponent* body);

// Entity type word +0xC8 bit 10. NOTE: marks a target enemies do not
// switch away from.
bool EntityHoldsAttention(const Entity* entity);
// Racer entities: word +0x1FC (NOTE: the vehicle or racing state; zero
// when the racer does not race).
bool RacerHasVehicle(const Entity* racer);
// Character component (+0x24 sub-object, +0x34/+0x38): animation event
// listener (the console installs callback 0x00065404 with the controller as
// its user data); null removes it.
void CharacterSetEventListener(EntityComponent* character, AIController* listener);
// Component 3 of an entity (0x000D93FC) and whether its state block
// (+0x88) is missing or has a non-zero field in bits 26-29 of +0x44.
// NOTE: meaning unknown; picks one of two dodge clips.
bool AttackerComponentFlag(Entity* attacker, bool* hasComponent);

// -- character component (entity virtual slot 15) ------------------------------------
// 0x002775A8: plays a clip (blend 1.0, 0.25 s). False if it is not available.
bool CharacterPlayClip(EntityComponent* character, uint32_t clipHash);
// 0x0029879C(component, 0, 1): true when the current clip is done.
bool CharacterClipFinished(EntityComponent* character);
// 0x00298B44: the character has a clip of that name.
bool CharacterHasClip(EntityComponent* character, uint32_t clipHash);
// Character entity +0x15C: health.
float CharacterHealth(const Entity* character);

// -- NavPower ------------------------------------------------------------------------
// Point handles (0x0062E164 valid, 0x0062E094 release) and agents
// (0x006C0D24 destroy).
bool NavPointIsValid(void* point);
void NavPointRelease(void* point);
struct NavAgent;
void NavAgentDestroy(NavAgent* agent);
// 0x006C0AA4 (with the nav query settings 0x011AD1D4): asks the agent to go
// to `goal`; 0x006C1B94: the goal it settled on (nearest reachable point).
void NavAgentRequestGoal(NavAgent* agent, const ape::Vec3& goal);
ape::Vec3 NavAgentGoal(NavAgent* agent);

// 0x004705F8: sends a game event from one entity to another.
void SendGameEvent(Entity* sender, Entity* receiver, uint32_t event, uint32_t data, bool immediate);
// 0x0048F90C: posts `event` to the members of listener group `group` within
// `range` of the sender.
void BroadcastInRange(Entity* sender, float range, uint32_t group, uint32_t event, uint32_t argument);

// Camera mode objects the bosses create: removed from the camera manager
// (game world +0x1281E0, 0x000FBB88 with a 1 s blend) and deleted.
class AICameraMode;
void ReleaseCameraMode(AICameraMode* mode);
// Game world byte +0x1285A5. NOTE: set while El Jefe's fight runs.
void GameWorldSetBossFlag(bool on);

// Knowledge system (0x00F6578C) and the AI object pool (0x00F557F8 +4):
// level start (0x00390D9C / 0x004B2398), level end (0x00390DC4 /
// 0x004B25C0) and reset (0x00390BAC / 0x004B0E70).
void KnowledgeLevelStart();
void KnowledgeLevelEnd();
void KnowledgeReset();
void AIObjectPoolLevelStart();
void AIObjectPoolLevelEnd();
void AIObjectPoolReset();

// -- HUD -----------------------------------------------------------------------------
// hud::SetContextByString (0x00369D14) / hud::SetContext (0x00369D00) with
// the entity as the context value (a 32-bit pointer on the console).
void HudSetContext(Entity* entity, const char* name);
void HudSetContext(Entity* entity, uint32_t nameHash);
// The HUD element's object (+0x18) has bit 15 of +0xA4 set. NOTE: the race
// HUD sets it while the racers must wait (countdown / results).
bool HudObjectPausesAI(const hud::HudObject* element);

// -- anim scripts ------------------------------------------------------------------
// The entity's world object (+0xA0) as an anim script object reference.
// NOTE: the engine's world objects and the anim script reference type are
// separate classes in this port.
ape::ApeRefCounted* EntityScriptObject(const Entity* entity);
// 0x004D0280: stops an instance and queues it for deletion.
void ReleaseAnimScript(ape::ApeAnimScriptInstance* instance);

// -- game ---------------------------------------------------------------------------
// The local player's character (game world +0x1078) if it is a player
// character (type bit 1 of +0xC0), else null.
Entity* PlayerCharacter();
// Game world +0x128168: a render target zoom applies (split screen); its
// zoom factor (0x005206F8).
bool RenderZoomActive();
float RenderZoom();
// 0x00F9BC28: the AI is told to leave targets alone (debug / cutscene).
bool AITargetingSuppressed();
// 0x002E2E78 on the job manager (0x00F64C20): hands a pending level
// request to the game world (game::JobManager::FlushPendingRequest); sent
// by AI message 0x9A986484.
void GameStateNotify();
// 0x00470660 with key/message 0x00F65C54: posts a script message.
void PostScriptMessage(Entity* sender, uint32_t key, uint32_t message, uint32_t argument);
uint32_t ProxyPointMessageKey();   // value of 0x00F65C54
// Key of a world object (+0xCC).
uint32_t WorldObjectKeyOf(const WorldObject* object);
// World object hierarchy: first child (+0x110) and next sibling (+0x10C).
WorldObject* WorldObjectFirstChild(const WorldObject* object);
WorldObject* WorldObjectNextSibling(const WorldObject* object);
// Proxy point record a point instance is tagged with (user tag
// kUserTag_AIProxyPoint at +0x114, record at +0x118), or null.
AIProxyPoint* ProxyPointOf(const WorldObject* object);
// Spawner that created an entity (entity +0xDC): the patrol proxy point
// index it hands to its characters (spawner slot 5 tells whether it is a
// character spawner; index at +0x74), or -1.
int ProxyPointIndexFromSpawner(const Entity* entity);
// Entity +0xD0: a render distance that grows with the render target zoom
// (geometry instances only: object +0x138 / zoom^2 when that is larger).
void EntityApplyRenderZoom(Entity* entity, float zoom);
// Height of the level's low point (0x0106BA84); objects below
// 20 - 0.5 * value are pulled back to their entity.
float LevelFloorDepth();

} // namespace ai
} // namespace levels
} // namespace sly
