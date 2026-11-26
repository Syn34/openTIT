// openTIT - Scripted entity base (vtable 0x00B126B8) and the deferred hit queue.
#include "sly/entity/ScriptedEntity.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/EntityHooks.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/objects/EntityPlacements.h"
#include "sly/levels/ai/AITuning.h"

#include <algorithm>
#include <cmath>

#include <vector>

namespace sly {
namespace entity {

namespace {

constexpr uint32_t kState_Destroy = 0x879F2BDAu;   // "Destroy"
constexpr uint32_t kState_Despawn = 0x879D1B7Au;   // "Despawn"
// NOTE: names of the suspend/resume states are unknown.
constexpr uint32_t kState_Suspend = 0xE69B8987u;
constexpr uint32_t kState_Resume = 0xD5E4206Fu;
constexpr uint32_t kVar_This = 0x0027469Eu;        // "this"

constexpr size_t kMaxDeferredHits = 0x200;

// Impact message (thrown objects and the like). NOTE: name unknown.
constexpr uint32_t kMsg_Impact = 0xB939F916u;
constexpr uint32_t kReaction_Impact = 0x80F43146u;   // NOTE: name unknown
constexpr uint16_t kDamageBit_Impact = 0x1000;

constexpr uint32_t kEvent_Damaged = 0x61F07A50u;     // NOTE: names unknown
constexpr uint32_t kMsgKey_Damage = 0xBD27942Du;
constexpr uint32_t kMsg_Damaged = 0x69988172u;
constexpr uint32_t kDamage_Sleep = 0x04B536B7u;      // "Sleep"

constexpr uint32_t kState_Spawn = 0x04B6FB5Bu;     // "Spawn"
constexpr uint32_t kState_Damaged = 0x77C17F6Fu;   // NOTE: names unknown
constexpr uint32_t kState_StageBase = 0x806E6EA1u; // + stage number
constexpr uint32_t kState_Defeated = 0xB1BC9747u;
constexpr uint32_t kEvent_Killed = 0x03DE9A54u;
constexpr uint32_t kMsgKey_Killed = 0xABB250AEu;
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
constexpr uint32_t kMsgKey_Defeated = 0x6A817B9Du;
constexpr uint32_t kMsg_Defeated = 0x35A7B402u;

// Player slot of a living player character for statistics events, else 0xFF.
uint8_t StatPlayerOf(const Entity* entity)
{
    if (entity && entity->IsAlive() && (entity->classBits & 1))
        return uint8_t(entity->flags >> 24);
    return 0xFF;
}

constexpr uint64_t kClass_Invulnerable = 0x20000u | 0x40000u;   // class bits 17 / 18

struct DeferredHit {
    EntityHitRecord hit;               // +0x00
    int category = 0;                  // +0x70
    ScriptedEntity* target = nullptr;  // +0x74
};

std::vector<DeferredHit>& DeferredHits()
{
    static std::vector<DeferredHit> s_hits;   // 0x01022100, count 0x01022080
    return s_hits;
}

} // namespace

void QueueDeferredHit(ScriptedEntity* target, const EntityHitRecord& hit)
{
    std::vector<DeferredHit>& hits = DeferredHits();
    if (hits.size() >= kMaxDeferredHits)
        return;
    hits.push_back({ hit, hit.Category(), target });
}

void DropDeferredHits(const ScriptedEntity* target)
{
    std::vector<DeferredHit>& hits = DeferredHits();
    for (size_t i = 0; i < hits.size();) {
        if (hits[i].target == target) {
            hits[i] = hits.back();
            hits.pop_back();
        } else {
            ++i;
        }
    }
}

// @func 0x000CF214 FlushDeferredHits
void FlushDeferredHits()
{
    for (DeferredHit& entry : DeferredHits()) {
        ScriptedEntity* target = entry.target;
        if (!target->IsAlive())
            continue;
        entry.hit.flags &= ~EntityHitRecord::kHit_Deferrable;
        switch (entry.category) {
        case 0: target->OnHitCategory0(entry.hit); break;
        case 1: target->OnHitCategory1(entry.hit); break;
        case 2: target->OnHitCategory2(entry.hit); break;
        case 3: target->OnHitCategory3(entry.hit); break;
        case 4: target->OnHitCategory4(entry.hit); break;
        default: break;
        }
    }
    DeferredHits().clear();
}

// @func 0x000C2E8C ScriptedEntityDef::ScriptedEntityDef
// @func 0x000C3078 ScriptedEntityDef::ScriptedEntityDef (thunk)
ScriptedEntityDef::ScriptedEntityDef(const ScriptedEntityDefDesc& desc)
    : EntityDef(desc), field_8C(desc.field_4C), lifetimeScriptName(desc.lifetimeScript), field_98(desc.field_54),
      invulnerableTime(desc.invulnerableTime), field_B4(desc.field_70), field_B8(desc.field_74),
      damageFlags(uint16_t(desc.damageFlags & 0xFFF0u)), hitMask(desc.hitMask)
{
    for (int i = 0; i < 5; ++i)
        stageHealth[i] = desc.stageHealth[i];
    // A missing script, or one without commands, only earns a warning
    // (stripped from the release build).
    lifetimeScript = ape::ApeAnimScriptLibrary::FindScriptAnywhere(desc.lifetimeScript, 0, true);
}

// @func 0x000E0BCC ScriptedEntity::ReceiveHit
bool ScriptedEntity::ReceiveHit(EntityHitRecord& hit)
{
    if ((hit.flags & EntityHitRecord::kHit_Deferrable) && PhysicsStepInProgress()) {
        QueueDeferredHit(this, hit);
        return false;
    }
    const uint16_t mask = def ? static_cast<const ScriptedEntityDef*>(def)->hitMask : 0;
    return (mask & hit.damageBits) == mask;
}

// @func 0x000E02F8 ScriptedEntity::Release
// Leaving the world also drops its queued hits and sends its anim script to
// "Despawn" (unless it is already being destroyed); the script lets go of
// the entity and is removed once it finishes.
bool ScriptedEntity::Release(bool kill)
{
    if (kill) {
        DropDeferredHits(this);
        if (ape::ApeAnimScriptInstance* script = animScript) {
            const ape::ApeAnimScriptState* state = script->GetState();
            if (!state || state->GetNameHash() != kState_Destroy) {
                script->SetActive(true);
                script->SetState(kState_Despawn);
            }
            ape::ApeAnimScriptVar* self = script->FindVar(kVar_This);
            if (self && self->GetType() == ape::AnimScriptVarType::Object)
                static_cast<ape::ApeAnimScriptObjectVar*>(self)->SetField24(0);
            // NOTE: flag 0x200 of the instance is read as "remove when finished".
            script->SetRemoveWhenFinished(true);
            animScript = nullptr;
        }
        field_16C = nullptr;
    }
    return Entity::Release(kill);
}

// @func 0x000E1060 ScriptedEntity::HandleMessage
// An impact message becomes a category 1 hit of "AI.ImpactDamage" pointing
// away from the sender (backwards along the facing without one); the hit
// keeps its damage bits only when the entity accepts them.
void ScriptedEntity::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Impact) {
        Entity* sender = MessageSenderEntity(msg);
        const ape::Vec3& position = Position();
        ape::Vec3 direction = { -transform.zAxis.x, -transform.zAxis.y, -transform.zAxis.z, 0.0f };
        if (sender && sender != this) {
            const ape::Vec3& from = sender->Position();
            const float dx = position.x - from.x, dy = position.y - from.y, dz = position.z - from.z;
            const float inv = 1.0f / std::sqrt(dx * dx + dy * dy + dz * dz);
            direction = { dx * inv, dy * inv, dz * inv, 0.0f };
        }
        EntityHitRecord hit;
        hit.source = sender;
        hit.reaction = kReaction_Impact;
        hit.damageBits = kDamageBit_Impact;
        hit.flags = uint16_t((1u << 13) | EntityHitRecord::kHit_FromPlayer);
        hit.vec10 = position;
        hit.data.impact.direction = direction;
        hit.data.impact.direction2 = direction;
        hit.data.impact.damage = levels::ai::g_aiImpactDamage;
        if (!ReceiveHit(hit))
            hit.damageBits = 0;
        OnHitCategory1(hit);
    }
    Entity::HandleMessage(msg);
}

// @func 0x00737790 ScriptedEntity::IsInvulnerable
bool ScriptedEntity::IsInvulnerable() const
{
    return (classBits & kClass_Invulnerable) != 0 || invulnerableTime > 0.0f;
}

// @func 0x007377D8 ScriptedEntity::SetBusyFlag
void ScriptedEntity::SetBusyFlag(bool on)
{
    Entity::SetBusyFlag(on);
    stateFlags = on ? (stateFlags | kState_Busy) : (stateFlags & ~uint32_t(kState_Busy));
}

// @func 0x007376F0 ScriptedEntity::AddHealth
void ScriptedEntity::AddHealth(float amount)
{
    const float raised = health + amount;
    health = std::min(raised, maxHealth) > 0.0f ? std::min(raised, maxHealth) : 0.0f;
}

// @func 0x000E1738 ScriptedEntity::Damage
// Health drops by the damage (or, for definitions counting hits, by one stage
// per hit) and the entity is invulnerable for the definition's time. Damage
// stages advance as health falls below their thresholds; reaching zero health
// (or the last stage) records the killer and marks the entity killed.
bool ScriptedEntity::Damage(float amount, Entity* source, uint32_t type)
{
    if (IsInvulnerable())
        return true;
    const ScriptedEntityDef* scriptDef = static_cast<const ScriptedEntityDef*>(def);
    const uint16_t defFlags = scriptDef ? scriptDef->damageFlags : 0;
    const uint32_t stageCount = (defFlags >> 4) & 0xF;

    if (health > 0.0f && amount > 0.0f) {
        if (listener && source != this &&
            (!source || !source->listener || ListenerIsAlly(source->listener)))
            EntitySendGameEvent(source, this, kEvent_Damaged, type, true);
        if (defFlags & 0x0800)
            EntityPostScriptMessage(this, kMsgKey_Damage, kMsg_Damaged, nameHash);
        if (type == kDamage_Sleep)
            GameWorldPostEntityEvent(0, 0x1B, this);
        invulnerableTime = scriptDef ? scriptDef->invulnerableTime : 0.0f;

        if (defFlags & 0x4000) {
            // Each hit costs one stage of the health bar.
            const uint32_t hits = (((stateFlags & kState_StageMask) >> kState_StageShift) + 1) & 0xF;
            stateFlags = (stateFlags & ~uint32_t(kState_StageMask)) | (hits << kState_StageShift);
            health = (1.0f - float(hits) / float(stageCount + 1)) * maxHealth;
            amount = 0.0f;
            if (stageCount <= hits) {
                if (!(stateFlags & kState_Immortal))
                    amount = health;
                stateFlags = (stateFlags & ~uint32_t(kState_StageMask)) | (stageCount << kState_StageShift);
            }
        }
        stateFlags |= kState_Damaged;
        if ((defFlags & 0x1000) && !(flags & kEnt_08)) {
            stateFlags &= ~uint32_t(kState_Busy);
            flags |= kEnt_08;
            if (flags & kEnt_Suspended) {
                flags &= ~uint32_t(kEnt_Suspended);
                OnResume();
            }
        }
        if (field_16C)
            DamageRecordSetType(field_16C, type);

        const float remaining = health - amount;
        health = std::min(remaining, maxHealth) > 0.0f ? std::min(remaining, maxHealth) : 0.0f;
        if (health == 0.0f) {
            if (!(stateFlags & kState_Immortal)) {
                lastAttacker = source;
                killer = source;
                stateFlags |= kState_Killed;
                SetBusyFlag(true);
            } else {
                health = 1.0f;
            }
        }

        if (!(defFlags & 0x4000) && scriptDef) {
            for (uint32_t i = 0; i < stageCount && i < 5; ++i) {
                if (health <= scriptDef->stageHealth[i])
                    stateFlags = (stateFlags & ~uint32_t(kState_StageMask)) | ((i + 1) << kState_StageShift);
            }
        }
        const uint32_t stage = (stateFlags & kState_StageMask) >> kState_StageShift;
        if (stage == stageCount && stage != ((stateFlags >> kState_DeathStageShift) & 0xF)) {
            lastAttacker = source;
            killer = source;
            stateFlags |= kState_Killed;
        }
    }
    return health > 0.0f;
}

// @func 0x000E0520 ScriptedEntity::Update
// The first update after spawning starts the script's "Spawn" state (with the
// placement's script values when asked for).
bool ScriptedEntity::Update(float dt)
{
    if (stateFlags & kState_SpawnPending) {
        if (animScript) {
            if ((stateFlags & kState_UsePlacement) && object)
                ApplyPlacementToScript(animScript, object);
            animScript->SetStateAndUpdate(kState_Spawn);
        }
        stateFlags &= ~uint32_t(kState_SpawnPending);
    }
    return Entity::Update(dt);
}

// @func 0x000E0620 ScriptedEntity::OnDefeated
void ScriptedEntity::OnDefeated()
{
    if (stateFlags & kState_Defeated)
        return;
    stateFlags |= kState_Defeated;
    if (animScript)
        animScript->SetStateAndUpdate(::sly::entity::kState_Defeated);
    GameWorldPostEntityEvent(StatPlayerOf(lastAttacker), 0x16, this);
    EntityPostScriptMessage(this, kMsgKey_Defeated, kMsg_Defeated, nameHash);
}

// @func 0x000E0734 ScriptedEntity::PostUpdate
// Runs the reactions queued by Damage(): the damage / stage script states,
// the end of the busy state once the script is done and, for a killed
// entity, the "Destroy" state with its events and messages.
bool ScriptedEntity::PostUpdate(float dt)
{
    const ScriptedEntityDef* scriptDef = static_cast<const ScriptedEntityDef*>(def);
    const uint16_t defFlags = scriptDef ? scriptDef->damageFlags : 0;
    invulnerableTime -= dt;

    if (stateFlags & kState_Damaged) {
        if (animScript) {
            animScript->SetStateAndUpdate(::sly::entity::kState_Damaged);
            if (animScript) {
                const uint32_t stage = (stateFlags & kState_StageMask) >> kState_StageShift;
                const uint32_t shown = (stateFlags >> kState_DeathStageShift) & 0xF;
                if (stage == shown) {
                    if (shown == ((defFlags >> 4) & 0xF))
                        OnDefeated();
                } else {
                    stateFlags = (stateFlags & ~(0xFu << kState_DeathStageShift)) | (stage << kState_DeathStageShift);
                    animScript->SetStateAndUpdate(kState_StageBase + stage);
                }
            }
        }
        stateFlags &= ~uint32_t(kState_Damaged);
    }

    if ((defFlags & 0x1000) && !(stateFlags & kState_Busy) && (flags & kEnt_08) && animScript &&
        animScript->IsFinished(false))
        SetBusyFlag(false);

    if (stateFlags & kState_Killed) {
        stateFlags &= ~uint32_t(kState_SpawnPending);
        if (animScript)
            animScript->SetStateAndUpdate(kState_Destroy);
        Entity* killerEntity = lastAttacker && lastAttacker->IsAlive() ? lastAttacker : nullptr;
        if (field_DC)
            SpawnerEntityKilled(field_DC, this);
        EntitySendGameEvent(killerEntity, this, kEvent_Killed, 0, true);
        EntityPostScriptMessage(this, kMsgKey_Killed, kMsg_Killed, nameHash);
        ape::world::ApeObj* render = GetRenderObject();
        // NOTE: object type 5 is not named in the world module.
        const bool skipStat = (render && static_cast<uint32_t>(render->GetType()) == 5) || (typeBits[0] & 0x10);
        if (!skipStat)
            GameWorldPostEntityEvent(StatPlayerOf(killerEntity), 0x01, this);
        stateFlags &= ~uint32_t(kState_Killed);
        const bool alive = IsAlive();
        lastAttacker = nullptr;
        if (!alive)
            return false;
        OnDefeated();
    }
    return Entity::PostUpdate(dt);
}

// @func 0x000E13D0 ScriptedEntity::SetScript
void ScriptedEntity::SetScript(uint32_t scriptHash)
{
    script = scriptHash ? ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptHash, 0, true) : nullptr;
}

// @func 0x000E0B64 ScriptedEntity::SetTimeScale
void ScriptedEntity::SetTimeScale(uint32_t value, bool children)
{
    if (animScript)
        animScript->SetStateAndUpdate(value);
    Entity::SetTimeScale(value, children);
}

// @func 0x000E1420 ScriptedEntity::OnSuspend
void ScriptedEntity::OnSuspend()
{
    Entity::OnSuspend();
    if (animScript) {
        animScript->SetStateAndUpdate(kState_Suspend);
        animScript->SetActive(false);
    }
    if (animController)
        AnimControllerSetRunning(animController, false);
}

// @func 0x000E158C ScriptedEntity::OnResume
void ScriptedEntity::OnResume()
{
    Entity::OnResume();
    if (animScript) {
        animScript->SetActive(true);
        animScript->SetState(kState_Resume);
    }
    if (animController)
        AnimControllerSetRunning(animController, true);
}

// -- construction ---------------------------------------------------------------

namespace {
constexpr uint32_t kVar_Transform = 0x001E8ECCu;    // NOTE: name unknown (matrix variable)
constexpr uint32_t kVar_DamageType = 0xB32DB729u;   // NOTE: name unknown (hash variable)
} // namespace

// @func 0x000E01C0 ScriptedEntity::ScriptedEntity
// @func 0x000E02D8 ScriptedEntity::ScriptedEntity (thunk)
ScriptedEntity::ScriptedEntity(const objects::ScriptedInstanceDesc& desc)
    : Entity(desc), maxHealth(desc.field_B8), health(desc.field_B8)
{
    stateFlags = kState_Busy;
    if (desc.field_C4)
        stateFlags |= 0x80000000u;
    if (desc.overridesA.Count() != 0)
        stateFlags |= kState_UsePlacement;
    if (desc.overridesB.Count() != 0)
        stateFlags |= 0x00040000u;   // NOTE: second binding list present
    const auto* scriptedDef = static_cast<const ScriptedEntityDef*>(desc.def);
    if (scriptedDef->damageFlags & 0x2000)
        stateFlags |= kState_Immortal;
    if (desc.field_C0 != 0)
        script = ape::ApeAnimScriptLibrary::FindScriptAnywhere(desc.field_C0, 0, true);
    StartLifetimeScript(desc.field_BC);
}

// @func 0x000E0000 ScriptedEntity::StartLifetimeScript
void ScriptedEntity::StartLifetimeScript(uint32_t scriptOverride)
{
    const auto* scriptedDef = static_cast<const ScriptedEntityDef*>(def);
    ape::ApeAnimScript* lifetime = scriptedDef->lifetimeScript;
    if (scriptOverride != 0 && scriptOverride != scriptedDef->lifetimeScriptName) {
        if (ape::ApeAnimScript* other = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptOverride, 0, true))
            lifetime = other;
    }
    if (!lifetime)
        return;
    animScript = StartScript(lifetime, 0, true);
    if (!animScript)
        return;
    // The entity releases the instance itself; "Spawn" starts on the first update.
    animScript->SetRemoveWhenFinished(false);
    stateFlags |= kState_SpawnPending;
    ape::ApeAnimScriptVar* matrix = animScript->FindVar(kVar_Transform);
    if (matrix && matrix->GetType() == ape::AnimScriptVarType::Matrix)
        static_cast<ape::ApeAnimScriptMatrixVar*>(matrix)->SetMatrix(transform);
    ape::ApeAnimScriptVar* damageType = animScript->FindVar(kVar_DamageType);
    if (damageType && damageType->GetType() == ape::AnimScriptVarType::Hash)
        field_16C = damageType;
}

// Damage type names of the "DamageTypes" child, in hit mask bit order (table
// 0x00AC66CC, shared with the Blast script command).
namespace {
constexpr uint32_t kDamageTypes[13] = {
    ape::AnimHash("SuperSpring"), 0xCB378DD0u, 0xB4AC34A8u, ape::AnimHash("PushBlock"), 0xF66BC199u,
    ape::AnimHash("CrackShot"), ape::AnimHash("Bomb"), ape::AnimHash("laser"), 0x001FE77Fu,
    ape::AnimHash("ReflectProj"), ape::AnimHash("RatchetWrench"), ape::AnimHash("ColeAmp"),
    ape::AnimHash("ThrowImpact"),
};
constexpr uint32_t kElem_DamageTypes = 0xB2892E4Au;

// Sets or clears bit `bit` of `word` from a boolean attribute (default: the
// bit's current value in `defaults`).
void ReadFlagBit(ape::xml::XmlElement* elem, uint32_t attr, uint16_t& word, uint16_t defaults, int bit)
{
    bool value = false;
    elem->GetBool(attr, &value, (defaults >> bit) & 1);
    word = uint16_t((word & ~(1u << bit)) | (uint32_t(value) << bit));
}
} // namespace

// @func 0x000C3910 ScriptedEntityDefDesc::Parse
void ScriptedEntityDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                  const EntityDef* parent)
{
    (void)root;   // only used for the warning about a parent of another class (stripped)
    InheritDefDesc(loader, *this);

    // Defaults: the scripted parent's values, else 100 health, no scripts,
    // flags 9 and 15, no damage stages.
    ScriptedEntityDefDesc defaults;
    defaults.damageFlags = 0x8200;
    float thresholds[5];
    for (int i = 0; i < 5; ++i)
        thresholds[i] = float(((defaults.damageFlags >> 4) & 0xF) - i);
    if (parent && parent->IsClass(kDefClass_Scripted)) {
        const auto* scripted = static_cast<const ScriptedEntityDef*>(parent);
        defaults.field_4C = scripted->field_8C;
        defaults.lifetimeScript = scripted->lifetimeScriptName;
        defaults.field_54 = scripted->field_98;
        defaults.invulnerableTime = scripted->invulnerableTime;
        defaults.field_70 = scripted->field_B4;
        defaults.field_74 = scripted->field_B8;
        defaults.damageFlags = uint16_t((scripted->damageFlags & 0xFFF0u) | (defaults.damageFlags & 0xFu));
        defaults.hitMask = scripted->hitMask;
        for (int i = 0; i < 5; ++i)
            thresholds[i] = scripted->stageHealth[i];
    }

    elem->GetFloat(0xA7D74568u, &field_4C, defaults.field_4C);
    elem->GetHash(0xF89FDD94u, &lifetimeScript, defaults.lifetimeScript);
    elem->GetHash(0x7973EFA5u, &field_54, defaults.field_54);
    ReadFlagBit(elem, 0x2243757Fu, damageFlags, defaults.damageFlags, 15);
    int32_t stages = 0;
    elem->GetInt(0xB981F897u, &stages, (defaults.damageFlags >> 4) & 0xF);
    damageFlags = uint16_t((damageFlags & 0xFF0Fu) | ((stages & 0xF) << 4));
    elem->GetFloat(0xC7348451u, &invulnerableTime, defaults.invulnerableTime);
    elem->GetHash(0xC9796122u, &field_70, defaults.field_70);
    elem->GetHash(0xDB88854Eu, &field_74, defaults.field_74);
    ReadFlagBit(elem, 0xEB08E801u, damageFlags, defaults.damageFlags, 14);
    ReadFlagBit(elem, 0xC2F5D6F9u, damageFlags, defaults.damageFlags, 13);
    ReadFlagBit(elem, 0x93CCEEE2u, damageFlags, defaults.damageFlags, 12);
    ReadFlagBit(elem, 0xF95AF325u, damageFlags, defaults.damageFlags, 11);
    ReadFlagBit(elem, 0x9366A555u, damageFlags, defaults.damageFlags, 10);
    ReadFlagBit(elem, 0xFD95C66Au, damageFlags, defaults.damageFlags, 9);
    ReadFlagBit(elem, 0xC3BD9B44u, damageFlags, defaults.damageFlags, 8);
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(ape::TempFormat("damageStateThreshold%d", i + 1), &stageHealth[i], thresholds[i]);

    // The damage types a hit must carry: each attribute present sets or
    // clears its bit (the rest keep the value the description had).
    if (ape::xml::XmlElement* types = elem->FirstChildElement(kElem_DamageTypes)) {
        for (int i = 0; i < 13; ++i) {
            bool on = false;
            if (types->GetBool(kDamageTypes[i], &on, false) == ape::xml::kQuery_Ok) {
                if (on)
                    hitMask = uint16_t(hitMask | (1u << i));
                else
                    hitMask = uint16_t(hitMask & ~(1u << i));
            }
        }
    }
}

} // namespace entity
} // namespace sly
