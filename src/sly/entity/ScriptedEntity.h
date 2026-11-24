// openTIT - Stand-ins for the scripted entity base and its definition base.
//
// Most gameplay entities in 0x00160000-0x00200000 (characters, props, alter
// egos, world objects) derive from a scripted entity (console vtable
// 0x00B126B8, constructor 0x000E02D8, code at 0x000E0xxx and 0x00737xxx in the
// entity module) and mix in the two listener interfaces; their definitions
// derive from the scripted definition base (vtable 0x00B11D98). Those bases
// are not ported yet; only what the derived classes need is modelled here.
#pragma once

#include "sly/entity/Entity.h"
#include "sly/entity/EntityInterfaces.h"

namespace sly {
namespace entity {

// Scripted entity (vtable 0x00B126B8). NOTE: overrides Update, PostUpdate,
// Release, SetTimeScale, HandleMessage, Suspend/Resume and returns an anim
// script; adds three virtual slots. Its destructor has no body of its own.
class ScriptedEntity : public Entity {
public:
    ~ScriptedEntity() override = default;

    void* animScript = nullptr;   // 0x164 ApeAnimScriptInstance (returned by slot 68)
};

// Scripted entity definition (vtable 0x00B11D98, slot 7 Spawn is pure).
// Destructor with no body of its own (0x007437D0).
class ScriptedEntityDef : public EntityDef {
public:
    ~ScriptedEntityDef() override = default;
    uint32_t field_B4 = 0;   // 0xB4 (returned by slot 4)
    uint32_t field_B8 = 0;   // 0xB8 (returned by slot 5)
};

} // namespace entity
} // namespace sly
