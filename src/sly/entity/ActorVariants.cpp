// openTIT - Small actor classes (see ActorVariants.h).
#include "sly/entity/ActorVariants.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kMsg_Damaged = 0x69988172u;
constexpr uint32_t kMsg_Killed = 0xCCF83851u;
constexpr uint32_t kMsg_PickedUp = 0x35007D87u;
// Messages of the small actors. NOTE: names unknown.
constexpr uint32_t kMsg_CountHit = 0xC6678E85u;
constexpr uint32_t kMsg_Signal = 0x86B846D5u;
// State names shared with the sound actors' slots. NOTE: strings unknown.
constexpr uint32_t kState_Slot1 = 0xCE7C29A3u;
constexpr uint32_t kState_Slot2 = 0x8FA5922Du;
constexpr uint32_t kState_Signalled = 0xCA8F18D3u;
// Damage bit of the projectiles a shielded counter ignores.
constexpr uint16_t kDamageBit_Shielded = 0x20;

ape::ApeAnimScriptInstance* ScriptOf(Entity* entity)
{
    return static_cast<ape::ApeAnimScriptInstance*>(entity->GetAnimScript());
}
} // namespace

// TODO(0x000E5E44 ActorEntity::HandleMessage)
void ActorHandleMessageBase(ActorEntity* actor, const EntityMessage& msg) { actor->ScriptedEntity::HandleMessage(msg); }
// TODO(0x000E5EF0 ActorEntity::SetTransform): only the entity transform.
void ActorSetTransformBase(ActorEntity* actor, const ape::Matrix34& m) { actor->Entity::SetTransform(m); }

// -- ActorGroup --------------------------------------------------------------------------

// @func 0x001AAA70 ActorGroup::HandleMessage
void ActorGroup::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Damaged) {
        Entity* sender = MessageSenderEntity(msg);
        for (Member& member : members) {
            if (member.entity != sender)
                continue;
            // Unless the group reacts in any state, busy members are skipped.
            if (!anyState && EntityGetState(member.entity, 0) != 0)
                continue;
            if (ape::ApeAnimScriptInstance* script = ScriptOf(member.entity)) {
                if (member.record)
                    GroupRecordSetCounter(member.record, counter);
                script->SetStateAndUpdate(kState_Slot2);
            }
            EntitySetState(member.entity, 0, 1);
            break;
        }
    }
    ActorHandleMessageBase(this, msg);
}

// @func 0x001AABD8 ActorGroup::ResetState
void ActorGroup::ResetState()
{
    EntitySetState(this, 0, 0);
    for (Member& member : members) {
        if (ape::ApeAnimScriptInstance* script = ScriptOf(member.entity))
            script->SetStateAndUpdate(kState_Slot1);
        EntitySetState(member.entity, 0, 0);
    }
}

// -- HitCounterActor ---------------------------------------------------------------------

// @func 0x001AC060 HitCounterActor::HandleMessage
void HitCounterActor::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_CountHit && key != 0 && msg.argument == key)
        ++hits;
    ActorHandleMessageBase(this, msg);
}

// @func 0x001AC0D4 HitCounterActor::OnHitCategory4
void HitCounterActor::OnHitCategory4(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    if ((counterFlags & (1u << 30)) && (hit.damageBits & kDamageBit_Shielded))
        counterFlags |= 1u << 29;
    else
        ActorEntity::OnHitCategory4(hit);
}

// @func 0x001AC158 HitCounterActor::Damage
bool HitCounterActor::Damage(float amount, Entity* source, uint32_t type)
{
    (void)type;
    if (key != 0 || amount <= 0.0f || (flags & kEnt_Suspended) || !source || !(source->classBits & 1))
        return true;
    const bool alive = health > 0.0f;
    ++hits;
    return alive;
}

// -- WatchActor --------------------------------------------------------------------------

// @func 0x001AD044 WatchActor::HandleMessage
void WatchActor::HandleMessage(const EntityMessage& msg)
{
    if ((msg.id == kMsg_Killed || msg.id == kMsg_PickedUp) && watched && watched->IsAlive() &&
        watched->nameHash == msg.argument) {
        watched = nullptr;
        EntitySetState(this, 0, 1);
        EntitySetState(this, 1, 0);
    }
    ActorHandleMessageBase(this, msg);
}

// -- SignalActor -------------------------------------------------------------------------

// @func 0x001B17F4 SignalActor::HandleMessage
void SignalActor::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Signal) {
        if (animScript)
            animScript->SetStateAndUpdate(kState_Signalled);
        field_1D0 = 0;
        EntitySetState(this, 0, 2);
    }
    ActorHandleMessageBase(this, msg);
}

// -- ScriptObjectActor -------------------------------------------------------------------

namespace {
constexpr uint32_t kMsg_Release = 0xCBF1C4F3u;     // also the state it plays. NOTE: name unknown
constexpr uint32_t kVar_HeldObject = 0x7A30D7E3u;  // NOTE: name unknown
} // namespace

// @func 0x001BE750 ScriptObjectActor::HandleMessage
void ScriptObjectActor::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Release && animScript) {
        if (ape::ApeAnimScriptVar* var = animScript->FindVar(kVar_HeldObject)) {
            if (var->GetType() == ape::AnimScriptVarType::Object)
                static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(nullptr);
        }
        animScript->SetState(kMsg_Release);
    }
    ActorHandleMessageBase(this, msg);
}

// @func 0x001BE858 ScriptObjectActor::CanInteractWith
bool ScriptObjectActor::CanInteractWith(Entity* other, int kind)
{
    if ((mode >> 1) == 1 && kind == kInteract_Player)
        return ActorCanInteractWith(this, other, kind);
    return false;
}

// -- ActorB17990 ---------------------------------------------------------------------------

// @func 0x001D8560 ActorB17990::Release
bool ActorB17990::Release(bool kill)
{
    if (kill)
        SetField1D81F0(false);
    return ActorEntity::Release(kill);
}

// TODO(0x001D81F0 ActorB17990::SetField1D81F0)
void ActorB17990::SetField1D81F0(bool on) { (void)on; }

// @func 0x001DB978 ActorB17990::SetPoints
void ActorB17990::SetPoints(float value, bool scaled)
{
    if (scaled)
        value *= maxPoints;
    shownPoints = value;
    points = value;
    const float ratio = maxPoints > 0.0f ? value / maxPoints : 0.0f;
    if (meter)
        meter->ratio = ratio;
}

// @func 0x001DB9C8 ActorB17990::ResetState
void ActorB17990::ResetState()
{
    points = 0.0f;
    EntitySetState(this, 0, 1);
    InteractionSetOffer(EntityInteraction(this), 0);
    field_260 = 0;
}

} // namespace entity
} // namespace sly
