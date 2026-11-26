// openTIT - Small actor classes (see ActorVariants.h).
#include "sly/entity/ActorVariants.h"

#include <cstring>

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/entity/ActorDefs.h"
#include "sly/objects/EntityPlacements.h"

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


// -- ActorGroup --------------------------------------------------------------------------

namespace {
// The script variable a group shares with its members (a float variable
// whose word is cleared).
constexpr uint32_t kVar_Group = 0xBBBC16FFu;
// Event listener group every actor group joins. NOTE: name unknown.
constexpr uint32_t kListenerGroup_ActorGroups = 0xBD27942Du;

// A float variable of the script, or null.
ape::ApeAnimScriptVar* FindFloatVar(ape::ApeAnimScriptInstance* script, uint32_t name)
{
    ape::ApeAnimScriptVar* var = script->FindVar(name);
    return var && var->GetType() == ape::AnimScriptVarType::Float ? var : nullptr;
}

ape::ApeAnimScriptVar* TakeGroupVar(ape::ApeAnimScriptInstance* script, uint32_t name = kVar_Group)
{
    ape::ApeAnimScriptVar* var = FindFloatVar(script, name);
    if (var)
        GroupVarSetValue(var, 0.0f);
    return var;
}
} // namespace

// @func 0x001A990C ActorGroup::ActorGroup
ActorGroup::ActorGroup(const objects::ActorGroupInstanceDesc& desc)
    : ActorEntity(desc), anyState(desc.field_E0)
{
    for (int i = 0; i < 4; ++i)
        stateTimes[i] = desc.field_D0[i];
    if (!children)
        children = new ape::ApeTArray<EntityChildLink>();
    if (children->Count() != 0)
        members.SetCapacity(children->Count());
    for (const EntityChildLink& link : *children) {
        Entity* member = link.child;
        if (!member)
            continue;
        EntitySetState(member, 0, 0);
        ape::ApeAnimScriptVar* var = nullptr;
        if (ape::ApeAnimScriptInstance* script = ScriptOf(member)) {
            var = TakeGroupVar(script);
            script->SetStateAndUpdate(kState_Slot1);
        }
        members.PushBack(Member{member, var});
    }
    if (animScript)
        groupVar = TakeGroupVar(animScript);
    EntityJoinListenerGroup(this, kListenerGroup_ActorGroups, true);
    EntitySetState(this, 0, 0);
}

// @func 0x001A9C80 ActorGroupDef::Spawn
Entity* ActorGroupDef::Spawn(EntityPlacement* placement)
{
    objects::ActorGroupInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* group = new ActorGroup(desc);
    group->OnSpawn();
    return group;
}

// @func 0x001AA610 ActorGroup::PostUpdate
bool ActorGroup::PostUpdate(float dt)
{
    if (!ActorEntity::PostUpdate(dt))
        return false;
    constexpr uint32_t kState_Member2 = 0x8B3C3A33u;
    constexpr uint32_t kState_Member3 = 0x3C8207B4u;
    constexpr uint32_t kState_GroupDone = 0x00B9CB91u;
    constexpr uint32_t kState_GroupReset = 0xC38793CAu;
    constexpr uint32_t kEvent_GroupDone = 0x19;
    const int state = EntityGetState(this, 0);
    if (state == 0) {
        uint32_t waiting = 0;
        for (Member& member : members) {
            Entity* entity = member.entity;
            const int memberState = EntityGetState(entity, 0);
            const float time = EntityStateTime(entity, 0);
            ape::ApeAnimScriptInstance* script = nullptr;
            if (memberState == 1 && time >= stateTimes[1]) {
                if ((script = ScriptOf(entity)) != nullptr) {
                    if (member.record)
                        GroupVarSetValue(member.record, stateTimes[2]);
                    script->SetStateAndUpdate(kState_Member2);
                }
                EntitySetState(entity, 0, 2);
            } else if (memberState == 2) {
                if (time >= stateTimes[2]) {
                    if ((script = ScriptOf(entity)) != nullptr) {
                        if (member.record)
                            GroupVarSetValue(member.record, stateTimes[3]);
                        script->SetStateAndUpdate(kState_Member3);
                    }
                    EntitySetState(entity, 0, 3);
                } else {
                    ++waiting;
                }
            } else if (memberState == 3 && time >= stateTimes[3]) {
                if ((script = ScriptOf(entity)) != nullptr)
                    script->SetStateAndUpdate(kState_Slot1);
                EntitySetState(entity, 0, 0);
            }
        }
        const uint32_t needed = field_1D4 < 0 ? members.Count() : uint32_t(field_1D4);
        if (needed <= waiting) {
            if (groupVar)
                GroupVarSetValue(groupVar, stateTimes[0]);
            if (animScript)
                animScript->SetStateAndUpdate(kState_GroupDone);
            EntitySetState(this, 0, 1);
            // NOTE: posted for player 0.
            GameWorldPostEntityEvent(0, kEvent_GroupDone, this);
        }
    } else if (state == 1 && EntityStateTime(this, 0) >= stateTimes[0]) {
        if (animScript)
            animScript->SetStateAndUpdate(kState_GroupReset);
        EntitySetState(this, 0, 2);
    }
    return true;
}

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
                    GroupVarSetValue(member.record, stateTimes[1]);
                script->SetStateAndUpdate(kState_Slot2);
            }
            EntitySetState(member.entity, 0, 1);
            break;
        }
    }
    ActorEntity::HandleMessage(msg);
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

// @func 0x001AACEC HitCounterActor::HitCounterActor
HitCounterActor::HitCounterActor(const objects::HitCounterActorInstanceDesc& desc) : ActorEntity(desc)
{
    for (int i = 0; i < 5; ++i)
        field_1A8[i] = desc.field_D0[i];
    counterFlags = (counterFlags & 0x1FFFFFFFu) | (desc.flags_E4 & 0xC0000000u);
    if (animScript) {
        var1BC = TakeGroupVar(animScript);
        var1C0 = TakeGroupVar(animScript, 0x3F5805D6u);
    }
    const auto* counterDef = static_cast<const HitCounterActorDef*>(def);
    const uint32_t index = uint32_t(counterDef->field_144);
    if (parts && index < parts->Count())
        key = uint32_t(reinterpret_cast<uintptr_t>((*parts)[index].body));   // NOTE: a 32-bit pointer on the console
    EntityJoinListenerGroup(this, 0x5924BAFAu, true);
    EntitySetState(this, 0, 0);
}

// @func 0x001AAF38 HitCounterActorDef::Spawn
Entity* HitCounterActorDef::Spawn(EntityPlacement* placement)
{
    objects::HitCounterActorInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* actor = new HitCounterActor(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001AC060 HitCounterActor::HandleMessage
void HitCounterActor::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_CountHit && key != 0 && msg.argument == key)
        ++hits;
    ActorEntity::HandleMessage(msg);
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

// @func 0x001AC1E8 WatchActor::WatchActor
WatchActor::WatchActor(const objects::ActorInstanceDesc& desc) : ActorEntity(desc)
{
    const auto* watchDef = static_cast<const WatchActorDef*>(def);
    watchedDef = FindEntityDef(watchDef->field_134);
    if (!watchedDef) {
        EntitySetState(this, 0, 0);
        return;
    }
    if (watchDef->flags_13C & 0x80000000u)
        EntityJoinListenerGroup(this, 0x600AE700u, true);
    EntityJoinListenerGroup(this, 0xABB250AEu, true);
    if (animScript)
        var1B0 = FindFloatVar(animScript, kVar_Group);
    watched = nullptr;
    EntitySetState(this, 0, 1);
    EntitySetState(this, 1, 0);
}

// @func 0x001AC38C WatchActorDef::Spawn
Entity* WatchActorDef::Spawn(EntityPlacement* placement)
{
    objects::ActorInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* actor = new WatchActor(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001AD044 WatchActor::HandleMessage
void WatchActor::HandleMessage(const EntityMessage& msg)
{
    if ((msg.id == kMsg_Killed || msg.id == kMsg_PickedUp) && watched && watched->IsAlive() &&
        watched->nameHash == msg.argument) {
        watched = nullptr;
        EntitySetState(this, 0, 1);
        EntitySetState(this, 1, 0);
    }
    ActorEntity::HandleMessage(msg);
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
    ActorEntity::HandleMessage(msg);
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
    ActorEntity::HandleMessage(msg);
}

// @func 0x001BE858 ScriptObjectActor::CanInteractWith
bool ScriptObjectActor::CanInteractWith(Entity* other, int kind, float* priority)
{
    if ((mode >> 1) == 1 && kind == kInteract_Player)
        return ActorEntity::CanInteractWith(other, kind, priority);
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
