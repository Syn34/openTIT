// openTIT - Services used by the third batch of level script code.
//
// Same idea as GameServices.h / ScriptServices.h: each free function stands
// for a console function or field in a system outside this module (named in
// its comment), and LevelServices.cpp holds TODO stubs returning the
// "missing" value until those systems are ported.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeRefCounted; class ApeAnimScriptObjectVar; }

namespace sly {

class WorldObject;
struct Laser;
struct LaserField;
struct LaserGroup;

namespace levels {

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

} // namespace levels
} // namespace sly
