// openTIT - Entity classes built from a definition's collision object.
//
// The definitions here all parse a "colObjName" and "targetingFlags"
// attribute and keep a reference to the collision shape they build from it;
// their entities create a rigid body from that shape. Class names are
// descriptive (the original names are not known).
#pragma once

#include "sly/entity/ScriptedEntity.h"

namespace sly {
namespace entity {

// Releases a Havok shape reference held by a definition (0x007D4EA8).
void ReleaseCollisionShape(void* shape);

// Definition of RigidProp (vtable 0x00B14360, 0x120 bytes).
class RigidPropDef : public ScriptedEntityDef {
public:
    ~RigidPropDef() override;
    void* collisionShape = nullptr;    // 0x10C
};

// A physics prop (vtable 0x00B143A8 + 0x00B14540/0x00B14568, 0x1E0 bytes;
// constructor 0x001676F8 builds its rigid body from the definition's shape).
class RigidProp : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    ~RigidProp() override;
};

// Definition of AlterEgoEntity (vtable 0x00B14918).
class AlterEgoDef : public ScriptedEntityDef {
public:
    ~AlterEgoDef() override;
    void* collisionShape = nullptr;    // 0xFC
};

// Entity of AlterEgoDef (vtable 0x00B14968 + 0x00B14B00/0x00B14B28, 0x1C0 bytes;
// constructor 0x001774D8). Registered in entity list B while alive.
// NOTE: named after the "AlterEgoBit"/"AlterEgoMegabit" names its methods use.
class AlterEgoEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    ~AlterEgoEntity() override;
};

} // namespace entity
} // namespace sly
