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

namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; class ApeAnimScriptState; }

namespace sly {
namespace entity {

// Scripted entity (vtable 0x00B126B8). NOTE: overrides Update, PostUpdate,
// Release, SetTimeScale, HandleMessage, Suspend/Resume and returns an anim
// script; adds three virtual slots. Its destructor has no body of its own.
class ScriptedEntity : public Entity {
public:
    ~ScriptedEntity() override = default;

    bool Update(float dt) override;
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;
    void HandleMessage(const EntityMessage& msg) override;
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

    // Slot 81 (base 0x00737790): class bits 17/18 or a running
    // invulnerability timer.
    virtual bool IsInvulnerable() const;
    // Slot 88 (+0x160): takes `amount` damage of `type` from `source`; true
    // while the entity still has health (0x000E1738).
    virtual bool Damage(float amount, Entity* source, uint32_t type);

    // State flags (+0x178).
    enum StateFlags : uint32_t {
        kState_Defeated    = 0x00004000,  // bit 14: OnDefeated ran
        kState_Busy        = 0x00008000,  // bit 15 mirror of flags bit 3 (slot 24)
        kState_SpawnPending = 0x00020000, // bit 17: start "Spawn" on the next update
        kState_UsePlacement = 0x00080000, // bit 19: hand the placement values to the script
        kState_StageShift  = 20,          // bits 20-23: damage stage / hits taken
        kState_StageMask   = 0x00F00000,
        kState_DeathStageShift = 24,      // bits 24-27: stage at which the entity dies
        kState_Immortal    = 0x10000000,  // bit 28: health never reaches zero
        kState_Damaged     = 0x20000000,
        kState_Killed      = 0x40000000,
    };

    float maxHealth = 0.0f;                             // 0x158
    float health = 0.0f;                                // 0x15C
    float invulnerableTime = 0.0f;                      // 0x160
    // Selects the script the entity runs (any library); 0 clears it
    // (0x000E13D0).
    void SetScript(uint32_t scriptHash);

    ape::ApeAnimScriptInstance* animScript = nullptr;   // 0x164 (returned by slot 68)
    ape::ApeAnimScript* script = nullptr;               // 0x168
    void* field_16C = nullptr;                          // 0x16C NOTE: cleared on release
    Entity* lastAttacker = nullptr;                     // 0x170
    Entity* killer = nullptr;                           // 0x174
    uint32_t stateFlags = 0;                            // 0x178 StateFlags

    // Defeat reaction (once): "defeated" script state, statistics event 0x16
    // and a script message (0x000E0620).
    void OnDefeated();

private:
    // Slot 24 (base 0x007377D8): flags bit 3 (and the state mirror); turning
    // it on also wakes a suspended entity.
    void SetBusyFlag(bool on);
};

// Scripted entity definition (vtable 0x00B11D98, slot 7 Spawn is pure).
// Destructor with no body of its own (0x007437D0).
class ScriptedEntityDef : public EntityDef {
public:
    ~ScriptedEntityDef() override = default;
    uint32_t field_B4 = 0;   // 0xB4 (returned by slot 4)
    uint32_t field_B8 = 0;   // 0xB8 (returned by slot 5)
    // Damage settings. NOTE: filled by the definition parser (not ported).
    float stageHealth[5] = {};  // 0x9C health thresholds of the damage stages
    float invulnerableTime = 0.0f;  // 0xB0 after each hit
    uint16_t damageFlags = 0;   // 0xBC bits 4-7 stage count, 11 message, 12 busy on hit, 14 hit counting
    uint16_t hitMask = 0;    // 0xBE damage bits a hit must carry to count
};

} // namespace entity
} // namespace sly
