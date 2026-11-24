// openTIT - Shared helpers of the game's anim script commands.
//
// Sly registers about 270 commands of its own next to the engine built-ins
// (see ape/anim/ApeAnimScript.h). They are created from the XML element of a
// script ("ActivateMixState", "CameraShake", "SetPos", ...) and act on game
// entities, the player, sounds, the HUD and so on.
//
// Most commands name the entity they act on in one of two ways:
//   * an object variable of the running instance ("RefObject", "Object",
//     ...): its world object carries the entity as user data tagged
//     "pEntity";
//   * the entity's own name ("Entity"), looked up among the live entities.
// ResolveEntity() implements that lookup, which the console inlines in every
// command.
#pragma once

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace sly {

class WorldObject;
namespace audio { struct MixState; }

namespace levels {

using ape::AnimHash;
using ape::ApeAnimScriptCommand;
using ape::ApeAnimScriptInstance;
using ape::ApeAnimScriptState;
using ape::ApeAnimScriptVar;
using ape::AnimScriptVarType;
using ape::xml::XmlElement;

// User-data tag of world objects that belong to an entity.
constexpr uint32_t kUserTag_Entity = 0x02235C33u;  // HashNameNoCase("pEntity")

// Debug switch that enables script warnings (console global 0x010AEC60).
extern bool g_scriptWarnings;

// Formats a script warning when warnings are enabled. `hash` is printed in
// hex as the only argument (the console builds the same strings).
void ScriptWarning(const char* format, uint32_t hash);

// Entity stored as user data of a world object (null if the object does not
// belong to an entity).
WorldObject* EntityFromObject(const ape::ApeRefCounted* object);

// Entity held by the object variable `varHash` of the instance, or null.
WorldObject* EntityFromVar(ApeAnimScriptInstance* instance, uint32_t varHash);

// The common operand lookup: entity of the object variable `objectVar`, else
// the live entity named `entityName`. Either hash may be 0.
WorldObject* ResolveEntity(ApeAnimScriptInstance* instance, uint32_t objectVar, uint32_t entityName);

// Entity named by `nameHash` for the event commands: a world object of the
// instance, the instance's entity variable, else a live entity (0x0009A190).
WorldObject* ResolveEventEntity(ApeAnimScriptInstance* instance, uint32_t nameHash);

// Value of the float variable `varHash`, or `fallback` when the variable is
// missing or not a float.
float ResolveFloat(ApeAnimScriptInstance* instance, uint32_t varHash, float fallback);
// Same for int variables.
int32_t ResolveInt(ApeAnimScriptInstance* instance, uint32_t varHash, int32_t fallback);

// Object variable lookup used by commands that take a world object operand:
// the instance's variable first, then a world object of that name.
ape::ApeRefCounted* ResolveWorldObject(ApeAnimScriptInstance* instance, uint32_t nameHash);

// Object held by the object variable `varHash` (no world fallback).
ape::ApeRefCounted* ObjectFromVar(ApeAnimScriptInstance* instance, uint32_t varHash);

// The object variable itself (null if missing or of another type).
ape::ApeAnimScriptObjectVar* FindObjectVar(ApeAnimScriptInstance* instance, uint32_t varHash);

// Commands that provide a debug element build an empty element; the release
// build ignores the command name it is given.
XmlElement* NewDebugElement(const char* commandName);

// Entity flag names used by the Get/SetEntityFlag commands.
struct EntityFlagName {
    uint32_t nameHash;
    uint32_t flag;
};
constexpr int kEntityFlagNameCount = 31;
extern const EntityFlagName kEntityFlagNames[kEntityFlagNameCount];
// Flag bit for a flag name, 0 if the name is unknown.
uint32_t LookupEntityFlag(uint32_t nameHash);

// Runs every command of `list` (shared by commands with nested bodies).
void RunCommands(const ape::AnimScriptCommandList& list, ApeAnimScriptInstance* instance,
                 ApeAnimScriptState* state);
void LinkCommands(const ape::AnimScriptCommandList& list);

} // namespace levels
} // namespace sly
