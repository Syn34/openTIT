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
#include "sly/entity/EntityHits.h"
#include "sly/entity/EntityInterfaces.h"

namespace ape { class ApeAnimScriptInstance; class ApeAnimScriptState; }

namespace sly {
namespace entity {

// Scripted entity (vtable 0x00B126B8). NOTE: overrides Update, PostUpdate,
// Release, SetTimeScale, HandleMessage, Suspend/Resume and returns an anim
// script; adds three virtual slots. Its destructor has no body of its own.
class ScriptedEntity : public Entity {
public:
    ~ScriptedEntity() override = default;

    bool Release(bool kill) override;
    // Slot 32: also switches the entity's anim script to the state.
    void SetTimeScale(uint32_t value, bool children) override;
    void OnSuspend() override;
    void OnResume() override;

    // Slots 83-87: a hit of category 0..4 (pure on the console; the derived
    // classes react). NOTE: names follow the dispatch only.
    virtual void OnHitCategory0(struct EntityHitRecord& hit) { (void)hit; }
    virtual void OnHitCategory1(struct EntityHitRecord& hit) { (void)hit; }
    virtual void OnHitCategory2(struct EntityHitRecord& hit) { (void)hit; }
    virtual void OnHitCategory3(struct EntityHitRecord& hit) { (void)hit; }
    virtual void OnHitCategory4(struct EntityHitRecord& hit) { (void)hit; }

    // Takes a hit: during the physics step a deferrable hit is queued (and
    // false returned); otherwise true when the hit carries every damage bit
    // the definition asks for (0x000E0BCC).
    bool ReceiveHit(EntityHitRecord& hit);

    ape::ApeAnimScriptInstance* animScript = nullptr;   // 0x164 (returned by slot 68)
    void* field_16C = nullptr;                          // 0x16C NOTE: cleared on release
};

// Scripted entity definition (vtable 0x00B11D98, slot 7 Spawn is pure).
// Destructor with no body of its own (0x007437D0).
class ScriptedEntityDef : public EntityDef {
public:
    ~ScriptedEntityDef() override = default;
    uint32_t field_B4 = 0;   // 0xB4 (returned by slot 4)
    uint32_t field_B8 = 0;   // 0xB8 (returned by slot 5)
    uint16_t hitMask = 0;    // 0xBE damage bits a hit must carry to count
};

} // namespace entity
} // namespace sly
