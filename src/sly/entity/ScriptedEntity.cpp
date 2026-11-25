// openTIT - Scripted entity base (vtable 0x00B126B8) and the deferred hit queue.
#include "sly/entity/ScriptedEntity.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/entity/EntityHooks.h"
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

bool ScriptedEntity::IsInvulnerable() const
{
    return (classBits & kClass_Invulnerable) != 0 || invulnerableTime > 0.0f;
}

void ScriptedEntity::SetBusyFlag(bool on)
{
    flags = on ? (flags | kEnt_08) : (flags & ~uint32_t(kEnt_08));
    if (on)
        SetSuspended(false);
    stateFlags = on ? (stateFlags | kState_Busy) : (stateFlags & ~uint32_t(kState_Busy));
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
            GameWorldPostEvent1B(this);
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

} // namespace entity
} // namespace sly
