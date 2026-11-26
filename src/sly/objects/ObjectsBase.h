// openTIT - The entity framework as the world object classes see it.
//
// The world objects in this directory (triggers, ropes, chains, push blocks,
// safes, sentry lights, ...) are entity classes built on the entity module's
// definition, placement and instance description classes (sly/entity/EntityDef.h)
// and on entity::Entity. This header brings them into the objects namespace
// and names the entity bits the objects test.
//
// Everything that reaches outside this directory goes through ObjectsHooks.h.
#pragma once

#include "sly/entity/Entity.h"
#include "sly/entity/EntityDef.h"

#include <cstdint>

namespace ape::xml { class XmlElement; }

namespace sly {
namespace objects {

using entity::Entity;
using entity::EntityAttachmentDesc;
using entity::EntityAttachmentList;
using entity::EntityDef;
using entity::EntityDefDesc;
using entity::EntityInstanceDesc;
using entity::EntityPlacement;
using entity::InheritDefDesc;
using entity::kDefClass_Trigger;
using entity::kDefClass_Sound;
using entity::kDefClass_08000000;

// Placement flags (EntityPlacement::flags).
constexpr uint32_t kPlace_Trigger = EntityPlacement::kPlace_Trigger;
constexpr uint32_t kPlace_ForceCreate = EntityPlacement::kPlace_ForceCreate;

// Entity type bits (Entity::HasTypeBit) the objects test.
enum ObjEntityTypeBits : uint32_t {
    kType_Scripted      = 1,    // +0xC0 0x02: a ScriptedEntity (health, IsDead slot 80)
    kType_Character     = 3,    // +0xC0 0x08: a Character
    kType_Pushable      = 49,   // +0xC4 0x20000: can be pushed (push block triggers)
};

// Slot 80 of scripted entities (ScriptedEntity::IsDead) for an entity that
// has kType_Scripted; false for any other.
bool EntityIsDead(Entity* entity);
// Player slot of an entity (top byte of the flags).
inline uint8_t EntityPlayerIndex(const Entity* entity) { return uint8_t(entity->flags >> 24); }
// Class bit 0: the entity is a player character.
inline bool EntityIsPlayer(const Entity* entity) { return (entity->classBits & 1u) != 0; }

} // namespace objects
} // namespace sly
