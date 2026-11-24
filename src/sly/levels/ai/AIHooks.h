// openTIT - Calls the level AI makes into systems outside this directory.
//
// Same convention as sly/levels/GameServices.h: each hook names the console
// function, virtual slot or field it stands for; the stubs in AIHooks.cpp
// return the "nothing there" value until the owning module provides it.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
class WorldObject;
namespace entity { class Entity; class EntityComponent; }

namespace levels {
struct AIProxyPoint;

namespace ai {

using entity::Entity;
using entity::EntityComponent;

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
    // AI settings objects (component 0x0D +0x1C; registered by 0x00067738)
    AI_UseNavigation,       // 0x00F94844 bool
    AI_UsePhysics,          // 0x00F94874 bool
};

const char* PropString(const PropertyBlock* block, AIProp prop);
uint32_t PropHash(const PropertyBlock* block, AIProp prop);   // enum properties store the hash
bool PropBool(const PropertyBlock* block, AIProp prop);
float PropFloat(const PropertyBlock* block, AIProp prop);

// Hashes of the enum values used above.
constexpr uint32_t kPathType_PingPong = 0x152D006Au;
constexpr uint32_t kPathType_Random = 0x8FA5D8C3u;

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
// Same request with zero velocity and the given orientation (identity
// quaternion on the console).
void BodyRequestStop(EntityComponent* body);

// -- character component (entity virtual slot 15) ------------------------------------
// 0x002775A8: plays a clip (blend 1.0, 0.25 s). False if it is not available.
bool CharacterPlayClip(EntityComponent* character, uint32_t clipHash);
// 0x0029879C(component, 0, 1): true when the current clip is done.
bool CharacterClipFinished(EntityComponent* character);
// Character entity +0x15C: health.
float CharacterHealth(const Entity* character);

// -- NavPower ------------------------------------------------------------------------
// Point handles (0x0062E164 valid, 0x0062E094 release) and agents
// (0x006C0D24 destroy).
bool NavPointIsValid(void* point);
void NavPointRelease(void* point);
struct NavAgent;
void NavAgentDestroy(NavAgent* agent);

// -- anim scripts ------------------------------------------------------------------
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
// 0x002E2E78 on the game state (0x00F64C20). NOTE: purpose unknown; sent by
// AI message 0x9A986484.
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
