// openTIT - Scripted actors (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"
#include "sly/entity/PhysicalEntity.h"
#include "sly/tuning/TargetRangeTuning.h"
#include "sly/objects/EntityPlacements.h"
#include "ape/world/ApeBase.h"
#include "sly/entity/PhysicsHooks.h"
#include <cmath>

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
#include "sly/entity/GameCamera.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/entity/ActorDefs.h"
#include "sly/entity/Character.h"
#include "sly/entity/ActorVariants.h"
#include "sly/entity/EntityHits.h"

#include <cstring>

namespace sly {
namespace entity {

// @func 0x000C79C0 ActorDef::ActorDef
// @func 0x000E27FC ActorDef::ActorDef (thunk)
// NOTE: the Havok side of each part (rigid body construction info from the
// named body of the level's physics data, the new hkpRigidBody, its name
// and collision properties) is built by ActorPartCreateBody.
ActorDef::ActorDef(const ActorDefDesc& desc)
    : ScriptedEntityDef(desc), field_CC(desc.field_88), navMode(desc.navMode),
      navObstacleSize(desc.navObstacleSize)
{
    float* const damage[15] = {&impactScale, &field_F4, &blastScale, &field_FC, &collisionScale,
                               &collisionThreshold, &collisionMax, &meleeScale, &field_110, &projectileScale,
                               &attackerScale[0], &attackerScale[1], &attackerScale[2], &attackerScale[3],
                               &interactionPriority};
    for (int i = 0; i < 15; ++i)
        *damage[i] = desc.damage[i];
    defFlags = desc.defFlags & 0xFFC00000u;
    for (int i = 0; i < 6; ++i)
        interactionNeeds[i] = desc.interactionNeeds[i];
    if (collisionMax < 0.0f)
        collisionMax = desc.field_4C;

    if (!desc.parts)
        return;
    for (const ActorPartDesc& partDesc : *desc.parts) {
        void* body = ActorPartCreateBody(partDesc, navMode);
        if (!body)
            continue;
        Part part;
        static_cast<ActorPartFields&>(part) = partDesc;
        part.havokObject = body;
        if (!parts)
            parts = new ape::ApeTArray<Part>();
        parts->Add(part);
    }
}

// @func 0x000E281C ActorDef::~ActorDef
// @func 0x000E29EC ActorDef::~ActorDef (deleting)
// @func 0x000E2A34 ActorDef::~ActorDef (forwarding copy, called by derived destructors)
// Releases the Havok objects of the part table, then the definition base.
ActorDef::~ActorDef()
{
    if (parts) {
        for (Part& part : *parts) {
            if (part.havokObject)
                camera::HavokRemoveReference(part.havokObject);
        }
        delete parts;
    }
}

// @func 0x001AD158 SoundActorDef::~SoundActorDef
// @func 0x001AD188 SoundActorDef::~SoundActorDef (deleting)
SoundActorDef::~SoundActorDef() = default;

// @func 0x001DC8B8 InteractivePropADef::~InteractivePropADef
// @func 0x001DC910 InteractivePropADef::~InteractivePropADef (deleting)
InteractivePropADef::~InteractivePropADef()
{
    if (buffer)
        ape::FreeIfNotNull(buffer);
}

// @func 0x001E52E8 SafeDef::~SafeDef
// @func 0x001E5318 SafeDef::~SafeDef (deleting)
SafeDef::~SafeDef() = default;

// @func 0x001DC04C InteractivePropA::~InteractivePropA
// @func 0x001DC104 InteractivePropA::~InteractivePropA (deleting)
// @func 0x001DC1D4 InteractivePropA::~InteractivePropA (deleting thunk, +0x180)
// @func 0x001DC1FC InteractivePropA::~InteractivePropA (deleting thunk, +0x17C)
// @func 0x001DC22C InteractivePropA::~InteractivePropA (thunk, +0x180)
// @func 0x001DC2E8 InteractivePropA::~InteractivePropA (thunk, +0x17C)
InteractivePropA::~InteractivePropA() = default;

// @func 0x001DBA58 InteractivePropAUseDef::Spawn
Entity* InteractivePropAUseDef::Spawn(EntityPlacement* placement)
{
    objects::ActorInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* prop = new InteractivePropA(desc);
    prop->OnSpawn();
    return prop;
}

// @func 0x001DC590 InteractivePropA::Update
bool InteractivePropA::Update(float dt)
{
    // NOTE: the console looks the interaction component up and discards it.
    (void)EntityInteraction(this);
    return ActorEntity::Update(dt);
}

// @func 0x001EA650 SearchlightActor::~SearchlightActor
// @func 0x001EA708 SearchlightActor::~SearchlightActor (deleting)
// @func 0x001EA7D8 SearchlightActor::~SearchlightActor (deleting thunk, +0x180)
// @func 0x001EA800 SearchlightActor::~SearchlightActor (deleting thunk, +0x17C)
// @func 0x001EA830 SearchlightActor::~SearchlightActor (thunk, +0x180)
// @func 0x001EA8EC SearchlightActor::~SearchlightActor (thunk, +0x17C)
SearchlightActor::~SearchlightActor() = default;

// @func 0x001E9C90 SearchlightActor::SearchlightActor
SearchlightActor::SearchlightActor(const objects::ActorInstanceDesc& desc) : ActorEntity(desc)
{
    const SearchlightDef* lightDef = Def();
    object_1B0 = EntityFindObject(this, lightDef->field_160[0]);
    object_1B4 = EntityFindObject(this, lightDef->field_160[1]);
    lightObject = EntityFindObject(this, lightDef->field_160[2]);
    object_1BC = EntityFindObject(this, lightDef->field_160[3]);
    if (lightObject)
        lightObject->SetVisible(false, false);
}

// @func 0x001E9D98 SearchlightDef::Spawn
Entity* SearchlightDef::Spawn(EntityPlacement* placement)
{
    objects::ActorInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* light = new SearchlightActor(desc);
    light->OnSpawn();
    return light;
}

// @func 0x001EB078 SearchlightActor::GetAimPoint
ape::Vec3 SearchlightActor::GetAimPoint() { return ActorEntity::GetAimPoint(); }

// @func 0x001EA9A8 SearchlightActor::Release
bool SearchlightActor::Release(bool kill)
{
    return ActorEntity::Release(kill);
}

// @func 0x001EB0B4 SearchlightActor::InteractionOffer
uint32_t SearchlightActor::InteractionOffer(int /*kind*/, Entity* /*other*/)
{
    return 0x10;
}

// @func 0x001EB09C SearchlightActor::InteractionNeeds
uint32_t SearchlightActor::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->interactionNeeds[kind];
}

// @func 0x001EB0BC SearchlightActor::CameraOffset
ape::Vec3 SearchlightActor::CameraOffset() const
{
    const ape::Vec3& offset = Def()->cameraOffset;
    return { offset.x, offset.y, offset.z, 0.0f };
}

// @func 0x001EB0DC SearchlightActor::Field150
ape::Vec4 SearchlightActor::Field150() const
{
    return Def()->field_150;
}

// @func 0x001F62DC InteractivePropB::~InteractivePropB
// @func 0x001F6394 InteractivePropB::~InteractivePropB (deleting)
// @func 0x001F6464 InteractivePropB::~InteractivePropB (deleting thunk, +0x180)
// @func 0x001F648C InteractivePropB::~InteractivePropB (deleting thunk, +0x17C)
// @func 0x001F64BC InteractivePropB::~InteractivePropB (thunk, +0x180)
// @func 0x001F6578 InteractivePropB::~InteractivePropB (thunk, +0x17C)
InteractivePropB::~InteractivePropB() = default;

// @func 0x001AE468 SoundActor::~SoundActor
// @func 0x001AE574 SoundActor::~SoundActor (deleting)
// @func 0x001AE5BC SoundActor::~SoundActor (deleting thunk, +0x180)
// @func 0x001AE608 SoundActor::~SoundActor (deleting thunk, +0x17C)
// @func 0x001AE654 SoundActor::~SoundActor (thunk, +0x180)
// @func 0x001AE67C SoundActor::~SoundActor (thunk, +0x17C)
SoundActor::~SoundActor()
{
    for (SoundSlot& slot : sounds)
        audio::SoundNameRelease(slot.sound);
}

// @func 0x001AE980 SoundActor::SetActive
void SoundActor::SetActive(bool active)
{
    if (animScript)
        SetAnimScriptPaused(animScript, !active);
    ActorEntity::SetActive(active);
}

namespace {
// Slot names (0x00AD139C). NOTE: the strings are not known.
constexpr uint32_t kSlotNames[7] = { 0xB9B6CE5Du, 0xCE7C29A3u, 0x8FA5922Du, 0xDCDF15C3u,
                                     0x8B3C3A33u, 0x047DC27Cu, 0x3C8207B4u };
} // namespace

// @func 0x001AE6AC SoundActor::Release
bool SoundActor::Release(bool kill)
{
    if (kill) {
        slotTimer = nullptr;
        for (SoundSlot& slot : sounds) {
            audio::StopVoice(slot.voice, true);
            slot.voice = 0;
            slot.state = nullptr;
        }
    }
    return ActorEntity::Release(kill);
}

// @func 0x001AE768 SoundActor::OnSuspend
void SoundActor::OnSuspend()
{
    ScriptedEntity::OnSuspend();
    for (SoundSlot& slot : sounds) {
        audio::StopVoice(slot.voice, true);
        slot.voice = 0;
    }
}

// @func 0x001AED20 SoundActor::PlaySlot
void SoundActor::PlaySlot(int index, bool force)
{
    const auto* d = static_cast<const SoundActorDef*>(def);
    if (!d)
        return;
    sequenceFlags &= ~kSeq_Scaled;
    if (Entity* player = PlayerEntity(0)) {
        for (EntityComponent* driver = player->GetComponent(kComp_17); driver;
             driver = player->GetComponent(kComp_17, driver)) {
            if (AnimDriverState(driver) == 7) {
                sequenceFlags |= kSeq_Scaled;
                break;
            }
        }
    }
    if (currentSlot != -1)
        audio::StopVoice(sounds[currentSlot].voice, true);
    const float scale = (sequenceFlags & kSeq_Scaled) ? d->durationScale : 1.0f;
    SoundSlot& slot = sounds[index];
    const float duration = slot.duration * scale;
    if (force || duration > 0.0f) {
        if (d->slotSounds[index].name != 0) {
            const ape::Vec3& o = d->soundOffset;
            const ape::Matrix34& m = transform;
            const ape::Vec3 at{ m.translation.x + m.xAxis.x * o.x + m.yAxis.x * o.y + m.zAxis.x * o.z,
                                m.translation.y + m.xAxis.y * o.x + m.yAxis.y * o.y + m.zAxis.y * o.z,
                                m.translation.z + m.xAxis.z * o.x + m.yAxis.z * o.y + m.zAxis.z * o.z, 0.0f };
            slot.voice = PlayEntitySound(slot.sound, &at, nullptr);
        }
        if (animScript) {
            if (slotTimer)
                slotTimer->duration = duration;
            SetSlotState(index, force);
        }
    }
    EntitySetState(this, 0, index);
    currentSlot = index;
}

// @func 0x001AEA50 SoundActor::Update
bool SoundActor::Update(float dt)
{
    if (!ActorEntity::Update(dt))
        return false;
    if (!def)
        return true;
    if (sequenceFlags & kSeq_ResumePending) {
        ResumeSequence();
        sequenceFlags &= ~kSeq_ResumePending;
    }
    // The timer of the state in effect decides whether to move on; a move
    // runs the next state's timer at once (at most 10000 steps per frame).
    using Timer = bool (SoundActor::*)(float, float);
    static const Timer kTimers[7] = { &SoundActor::AdvanceTo1, &SoundActor::AdvanceTo2, &SoundActor::AdvanceTo3,
                                      &SoundActor::AdvanceTo4, &SoundActor::AdvanceTo5, &SoundActor::AdvanceTo6,
                                      &SoundActor::AdvanceWrap };
    bool waiting = false;
    for (int step = 0; step < 10000 && !waiting; ++step) {
        const int state = EntityGetState(this, 0);
        if (state < 0 || state > 6)
            break;   // NOTE: the console spins through the remaining steps doing nothing
        const float elapsed = EntityStateTime(this, 0);
        waiting = (this->*kTimers[state])(elapsed, SlotDuration(state));
    }
    return true;
}

// @func 0x001AE9C8 SoundActor::PlaySlotByName
void SoundActor::PlaySlotByName(uint32_t name)
{
    for (int i = 0; i < 7; ++i) {
        if (kSlotNames[i] == name) {
            PlaySlot(i, false);
            return;
        }
    }
}

float SoundActor::SlotDuration(int index) const
{
    const float scale = (sequenceFlags & kSeq_Scaled) ? static_cast<const SoundActorDef*>(def)->durationScale : 1.0f;
    return sounds[index].duration * scale;
}

// @func 0x001AF470 SoundActor::PlayFrom
void SoundActor::PlayFrom(float time, int slot, bool force)
{
    if (!def)
        return;
    float remaining = time;
    if (time > 0.0f) {
        for (;;) {
            const float duration = SlotDuration(slot);
            remaining = time;
            if (time < duration)
                break;
            if (duration > 0.0f) {
                time -= duration;
            } else {
                // Slot 4 has no length of its own: the sequence rests there.
                remaining = 0.0f;
                if (slot == 4)
                    break;
            }
            if (++slot == 7)
                slot = 1;
        }
    }
    PlaySlot(slot, force);
    EntitySetStateTime(this, 0, remaining);
}

// @func 0x001AF56C SoundActor::ResumeSequence
void SoundActor::ResumeSequence()
{
    PlayFrom(savedTime, savedSlot, true);
}

namespace {
// Shared body of the slot timers.
bool AdvanceAfter(SoundActor* actor, float elapsed, float duration, int next, bool force)
{
    if (actor->sequenceFlags & SoundActor::kSeq_Held)
        return true;
    if (duration < 0.0f || duration <= elapsed) {
        actor->PlaySlot(next, force);
        return false;
    }
    return true;
}
} // namespace

// @func 0x001AF018 SoundActor::AdvanceTo1
bool SoundActor::AdvanceTo1(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 1, false); }
// @func 0x001AF0A8 SoundActor::AdvanceTo2
bool SoundActor::AdvanceTo2(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 2, false); }
// @func 0x001AF138 SoundActor::AdvanceTo3
bool SoundActor::AdvanceTo3(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 3, false); }
// @func 0x001AF1C8 SoundActor::AdvanceTo4
bool SoundActor::AdvanceTo4(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 4, true); }
// @func 0x001AF2F0 SoundActor::AdvanceTo6
bool SoundActor::AdvanceTo6(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 6, false); }
// @func 0x001AF380 SoundActor::AdvanceWrap
bool SoundActor::AdvanceWrap(float elapsed, float duration) { return AdvanceAfter(this, elapsed, duration, 1, false); }

// @func 0x001AF258 SoundActor::AdvanceTo5
bool SoundActor::AdvanceTo5(float elapsed, float duration)
{
    if (sequenceFlags & kSeq_Held)
        return true;
    if (duration < 0.0f || elapsed < duration)
        return true;
    PlaySlot(5, false);
    return false;
}

// @func 0x001D20D0 SoundActorB174C0::Release
bool SoundActorB174C0::Release(bool kill)
{
    return SoundActor::Release(kill);
}

// -- SoundActor subclasses --------------------------------------------------------------

namespace {
// Timer test shared by the subclasses: false (and the slot played) once the
// duration ran out; a negative duration runs out at once.
bool TimerExpired(float elapsed, float duration) { return duration < 0.0f || duration <= elapsed; }
// State names of the switch positions (0x00F64A18 / 0x00F64A20). NOTE:
// strings unknown; 0x00F64A10/14 next to them are "Off"/"On".
constexpr uint32_t kSwitchSlot4States[2] = { 0xDA129A8Fu, 0xD57C789Fu };
constexpr uint32_t kSwitchSlot1States[2] = { 0x0181FDFFu, 0x39DAE72Fu };
// Interaction kind mask offered by the switch per position.
constexpr uint32_t kSwitchInteraction[2] = { 4, 0 };
} // namespace

// @func 0x001C4280 SoundActorB16B48::PlaySlot
void SoundActorB16B48::PlaySlot(int index, bool force)
{
    SoundActor::PlaySlot(index, force);
    if (index == 1) {
        lightFlags &= ~0x80;
        SetLight(true);
    }
}

// @func 0x001C3EF0 SoundActorB16B48::SetLight
void SoundActorB16B48::SetLight(bool on)
{
    const auto* d = static_cast<const SoundActorB16B48Def*>(def);
    if (!d)
        return;
    if (d->lightHasBodies) {
        for (size_t i = 0; i < lightBodies.Count(); ++i) {
            void* body = lightBodies[i];
            if (!body)
                continue;
            if (!on) {
                if (RigidBodyInWorld(body))
                    RigidBodyRemoveFromWorld(body);
            } else if (!RigidBodyInWorld(body)) {
                RigidBodyAddToWorld(body, ~0u);
                ape::Matrix34 placed;
                PartTransform(placed, d->bodyParts[i], ape::world::kWorldIdentity);
                RigidBodySetTransform(body, placed);
            }
        }
    }
    for (size_t i = 0; i < lightObjects.Count(); ++i) {
        if (lightObjects[i])
            lightObjects[i]->SetVisible(on, true);
    }
}

// @func 0x001C42E8 SoundActorB16B48::AdvanceTo2
bool SoundActorB16B48::AdvanceTo2(float elapsed, float duration)
{
    if (!(lightFlags & 0x80))
        return true;
    return SoundActor::AdvanceTo2(elapsed, duration);
}

// @func 0x001C4390 SoundActorB16B48::AdvanceTo3
// NOTE: the console passes the definition as a fourth argument; it is taken
// from the actor here.
bool SoundActorB16B48::AdvanceTo3(float elapsed, float duration)
{
    const auto* d = static_cast<const SoundActorB16B48Def*>(def);
    if ((sequenceFlags & kSeq_Held) || !TimerExpired(elapsed, duration)) {
        if (d->restartWhenWaiting && !(lightFlags & 0x20))
            PlaySlot(1, true);
        return true;
    }
    PlaySlot(3, false);
    if (d->unlightOnSlot3 && (lightFlags & 0x80))
        SetLight(false);
    return false;
}

// @func 0x001C44A8 SoundActorB16B48::AdvanceTo4
bool SoundActorB16B48::AdvanceTo4(float elapsed, float duration)
{
    const auto* d = static_cast<const SoundActorB16B48Def*>(def);
    if ((sequenceFlags & kSeq_Held) || !TimerExpired(elapsed, duration))
        return true;
    PlaySlot(4, true);
    if (d->unlightOnSlot4 && (lightFlags & 0x80))
        SetLight(false);
    return false;
}

// @func 0x001C4578 SoundActorB16B48::AdvanceWrap
bool SoundActorB16B48::AdvanceWrap(float elapsed, float duration)
{
    if ((sequenceFlags & kSeq_Held) || !TimerExpired(elapsed, duration))
        return true;
    PlaySlot(1, false);
    if (lightFlags & 0x80) {
        lightFlags &= ~0x80;
        SetLight(true);
    }
    return false;
}

// @func 0x001BF794 SoundActorB16678::Release
bool SoundActorB16678::Release(bool kill)
{
    return SoundActor::Release(kill);
}

// @func 0x001BFE38 SoundActorB16678::AdvanceTo2
bool SoundActorB16678::AdvanceTo2(float elapsed, float duration)
{
    if (!(switchFlags & 0x08))
        return true;
    return SoundActor::AdvanceTo2(elapsed, duration);
}

// @func 0x001BFEE0 SoundActorB16678::AdvanceTo5
// Reaching slot 5 flips the switch: the interaction it offers follows the
// position and the switch disarms.
bool SoundActorB16678::AdvanceTo5(float elapsed, float duration)
{
    if (delay > 0.0f || (sequenceFlags & kSeq_Held) || duration < 0.0f || elapsed < duration)
        return true;
    PlaySlot(5, false);
    if (resetTime >= 0.0f)
        position = 0;
    if (EntityComponent* interaction = EntityInteraction(this))
        InteractionSetOffer(interaction, kSwitchInteraction[position == 1 ? 1 : 0]);
    switchFlags &= ~0x08;
    return false;
}

// @func 0x001C0010 SoundActorB16678::SetSlotState
void SoundActorB16678::SetSlotState(int index, bool immediate)
{
    if (index == 1 || index == 4) {
        const uint32_t state = (index == 1 ? kSwitchSlot1States : kSwitchSlot4States)[position];
        if (immediate)
            animScript->SetStateAndUpdate(state);
        else
            animScript->SetState(state);
        return;
    }
    SoundActor::SetSlotState(index, immediate);
}

// @func 0x001BFA50 SoundActorB16678::Update
bool SoundActorB16678::Update(float dt)
{
    if (!SoundActor::Update(dt))
        return false;
    if (switchFlags & 0x40) {
        switchFlags &= ~0x40;
        Refresh();
    }
    if (delay > 0.0f)
        delay -= dt;
    return true;
}

// @func 0x001BF8E8 SoundActorB16678::Refresh
void SoundActorB16678::Refresh()
{
    constexpr uint32_t kEvent_SwitchOn = 0x0B;    // NOTE: kinds of 0x0046528C / 0x00465308
    constexpr uint32_t kEvent_SwitchOff = 0x0C;
    if (switchFlags & 0x20)
        return;
    if (resetTime >= 0.0f) {
        position = 1;
        delay = resetTime;
        PlaySlot(1, false);
    } else if (position == 0 || position == 1) {
        position = 1 - position;
    } else {
        return;
    }
    if (switchFlags & 0x80)
        switchFlags |= 0x20;
    GameWorldPostEntityEvent(uint8_t(flipPlayer), position == 1 ? kEvent_SwitchOn : kEvent_SwitchOff, this);
    switchFlags |= 0x08;
}

// @func 0x001BF850 SoundActorB16678::HandleMessage
void SoundActorB16678::HandleMessage(const EntityMessage& msg)
{
    constexpr uint32_t kMsg_Unlock = 0x4DA1B5C3u;   // NOTE: names unknown
    constexpr uint32_t kMsg_Lock = 0x59676D2Au;
    if (msg.id == kMsg_Unlock)
        switchFlags &= ~0x20;
    else if (msg.id == kMsg_Lock)
        switchFlags |= 0x20;
    ActorEntity::HandleMessage(msg);
}

namespace {
// Player slot of a player entity: the top byte of its flags.
int EntityPlayerSlot(const Entity* entity) { return int8_t(entity->flags >> 24); }
} // namespace

bool SoundActorB16678::AcceptsFlip(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit) || (switchFlags & 0x20))
        return false;
    if (switchFlags & 0x10)
        return true;
    if (switchFlags & 0x08)
        return false;
    return resetTime <= 0.0f || EntityGetState(this, 0) == 1;
}

// @func 0x001BFC10 SoundActorB16678::OnHitCategory0
void SoundActorB16678::OnHitCategory0(EntityHitRecord& hit)
{
    if (!AcceptsFlip(hit))
        return;
    const auto* d = static_cast<const ActorDef*>(def);
    if (d->collisionScale > 0.0f && d->collisionThreshold > 0.0f &&
        d->collisionThreshold < hit.data.collision.speed)
        QueueFlip(0);
}

// @func 0x001BFB00 SoundActorB16678::OnHitCategory1
void SoundActorB16678::OnHitCategory1(EntityHitRecord& hit)
{
    if (!AcceptsFlip(hit))
        return;
    if (static_cast<const ActorDef*>(def)->impactScale > 0.0f && (hit.source->classBits & 1))
        QueueFlip(EntityPlayerSlot(hit.source));
}

// @func 0x001BFD28 SoundActorB16678::OnHitCategory4
void SoundActorB16678::OnHitCategory4(EntityHitRecord& hit)
{
    if (!AcceptsFlip(hit))
        return;
    if ((hit.source->classBits & 1) && static_cast<const ActorDef*>(def)->projectileScale > 0.0f)
        QueueFlip(EntityPlayerSlot(hit.source));
}

// @func 0x001C1B50 SoundActorB168E0::AdvanceTo2
bool SoundActorB168E0::AdvanceTo2(float elapsed, float duration)
{
    if (!(flags240 & 0x08))
        return true;
    return SoundActor::AdvanceTo2(elapsed, duration);
}

// @func 0x001C1BF8 SoundActorB168E0::AdvanceTo3
bool SoundActorB168E0::AdvanceTo3(float elapsed, float duration)
{
    if ((sequenceFlags & kSeq_Held) || (duration >= 0.0f && elapsed < duration))
        return true;
    PlaySlot(3, false);
    flags240 |= 0x80;
    return false;
}

// @func 0x001C1CA0 SoundActorB168E0::AdvanceTo5
bool SoundActorB168E0::AdvanceTo5(float elapsed, float duration)
{
    if (flags240 & 0x08)
        return true;
    return SoundActor::AdvanceTo5(elapsed, duration);
}

namespace {
// Drops the tracked entities and their phantoms.
template <class Record>
void ReleaseTracked(Record (&tracked)[6])
{
    for (Record& record : tracked) {
        if (!record.entity)
            continue;
        if (record.entity->IsAlive())
            record.entity->Release(true);
        record.entity = nullptr;
        camera::DestroyPhantom(record.phantom);
        record.phantom = nullptr;
    }
}
} // namespace

bool SoundActorB16DB0::IsOccupied() const
{
    return attached || field_234 || holdTime > 0.0f;
}

// @func 0x001C7B38 SoundActorB16DB0::PostUpdate
bool SoundActorB16DB0::PostUpdate(float dt)
{
    if (!ActorEntity::PostUpdate(dt))
        return false;
    const int state = EntityGetState(this, 0);
    const auto* d = static_cast<const SoundActorB16DB0Def*>(def);
    const bool enabled = (state == 4 && d->triggerDelay < EntityStateTime(this, 0)) ||
                         ((occupancyFlags & kOccupancy_Bit26) && state == 6);
    for (Tracked& record : tracked) {
        if (record.entity)
            TriggerSetEnabled(record.entity, enabled);
    }
    return true;
}

// @func 0x001C70C8 SoundActorB16DB0::Release
bool SoundActorB16DB0::Release(bool kill)
{
    if (kill) {
        ReleaseTracked(tracked);
        if (attached) {
            attached->Release(false);
            attached = nullptr;
        }
    }
    return SoundActor::Release(kill);
}

// @func 0x001C7D60 SoundActorB16DB0::AdvanceTo2
bool SoundActorB16DB0::AdvanceTo2(float elapsed, float duration)
{
    if (IsOccupied() && !(occupancyFlags & kOccupancy_Bit29))
        return true;
    return SoundActor::AdvanceTo2(elapsed, duration);
}

// @func 0x001C7E38 SoundActorB16DB0::AdvanceTo5
bool SoundActorB16DB0::AdvanceTo5(float elapsed, float duration)
{
    // NOTE: the flag works the other way round here: an occupant holds
    // slot 5 only while bit 29 is set (and the definition does not say otherwise).
    const auto* d = static_cast<const SoundActorB16DB0Def*>(def);
    if (IsOccupied() && (occupancyFlags & kOccupancy_Bit29) && !(d->options & (1u << 28)))
        return true;
    return SoundActor::AdvanceTo5(elapsed, duration);
}

// @func 0x001C9B3C SoundActorB17008::PostUpdate
bool SoundActorB17008::PostUpdate(float dt)
{
    if (!ActorEntity::PostUpdate(dt))
        return false;
    const bool enabled = EntityGetState(this, 0) == 4;
    for (Tracked& record : tracked) {
        if (record.entity)
            TriggerSetEnabled(record.entity, enabled);
    }
    return true;
}

// @func 0x001C98C4 SoundActorB17008::Release
bool SoundActorB17008::Release(bool kill)
{
    if (kill)
        ReleaseTracked(tracked);
    return SoundActor::Release(kill);
}

// @func 0x001C9D10 SoundActorB17008::AdvanceTo2
bool SoundActorB17008::AdvanceTo2(float elapsed, float duration) { return SoundActor::AdvanceTo2(elapsed, duration); }

// @func 0x001C9DA0 SoundActorB17008::AdvanceTo5
bool SoundActorB17008::AdvanceTo5(float elapsed, float duration) { return SoundActor::AdvanceTo5(elapsed, duration); }

// @func 0x001D46E8 SoundActorB17728::Release
bool SoundActorB17728::Release(bool kill)
{
    if (kill && attached) {
        attached->Release(false);
        attached = nullptr;
    }
    return SoundActor::Release(kill);
}

// @func 0x001D47E4 SoundActorB17728::Update
bool SoundActorB17728::Update(float dt)
{
    if (!SoundActor::Update(dt))
        return false;
    if (!def)
        return true;
    if (!(lookupFlags & 0x80)) {
        // Only entities of type bit 22 can be attached.
        Entity* found = FindEntity(attachedName);
        attached = (found && (found->typeBits[0] & 0x400000)) ? found : nullptr;
        if (attached)
            attached->AddRef();
        lookupFlags |= 0x80;
    }
    return true;
}

// @func 0x001D48B8 SoundActorB17728::UpdateOpen
void SoundActorB17728::UpdateOpen(bool& open)
{
    const auto* d = static_cast<const SoundActorB17728Def*>(def);
    if (!d)
        return;
    if (attached) {
        // The attached entity is a trigger (occupant count at +0x160).
        open = TriggerOccupantCount(attached) != 0;
        return;
    }
    open = rangeSq <= 0.0f;
    for (int i = 0; i < PlayerCount(); ++i) {
        Entity* player = PlayerEntity(i);
        if (!player)
            continue;
        const ape::Vec3& p = player->Position();
        const ape::Vec3& at = Position();
        const float dx = p.x - at.x, dy = p.y - at.y, dz = p.z - at.z;
        if (dz * dz + dx * dx + dy * dy > rangeSq)
            continue;
        open = true;
        if (!d->requiredCharacter)
            return;
        const EntityDef* playerDef = player->def;
        if (!playerDef || !(playerDef->classBits & 0x08))
            return;
        const auto* character = static_cast<const CharacterDef*>(playerDef);
        open = character->characterName == d->requiredCharacter || character->field_3D4 != 0;
        return;
    }
}

// @func 0x001D4A28 SoundActorB17728::PostUpdate
bool SoundActorB17728::PostUpdate(float dt)
{
    if (!ActorEntity::PostUpdate(dt))
        return false;
    if (!def)
        return true;
    const int state = EntityGetState(this, 0);
    if (state == 5)
        return true;
    const bool wasOpen = holdWrap && state != 1;
    UpdateOpen(holdWrap);
    const bool opened = !wasOpen && holdWrap;
    const bool closed = wasOpen && !holdWrap;
    if (opened || closed)
        PlaySlot(opened ? 2 : 6, false);
    return true;
}

// @func 0x001D4B90 SoundActorB17728::AdvanceWrap
bool SoundActorB17728::AdvanceWrap(float elapsed, float duration)
{
    if (holdWrap || (sequenceFlags & kSeq_Held) || (duration >= 0.0f && elapsed < duration))
        return true;
    PlaySlot(1, false);
    return false;
}

// -- SoundActorB17270 ---------------------------------------------------------------------

namespace {
// Alternative slot states of SoundActorB17270. NOTE: names unknown; the
// plain ones are the slot names.
constexpr uint32_t kB17270_Slot2Engaged = 0xDBA69E75u;
constexpr uint32_t kB17270_Slot6Alt = 0x8C7121FCu;
constexpr float kScriptStep = 1.0f / 60.0f;

bool B17270Waiting(SoundActorB17270* actor, float elapsed, float duration, bool coolDown)
{
    return EntityGetState(actor, 1) != 0 || !actor->engaged || (coolDown && actor->coolDown > 0.0f) ||
           (actor->sequenceFlags & SoundActor::kSeq_Held) || (duration >= 0.0f && elapsed < duration);
}
} // namespace

// @func 0x001CF710 SoundActorB17270::AdvanceTo3
bool SoundActorB17270::AdvanceTo3(float elapsed, float duration)
{
    if (B17270Waiting(this, elapsed, duration, true))
        return true;
    PlaySlot(3, false);
    return false;
}

// @func 0x001CF810 SoundActorB17270::AdvanceTo4
bool SoundActorB17270::AdvanceTo4(float elapsed, float duration)
{
    if (B17270Waiting(this, elapsed, duration, false))
        return true;
    PlaySlot(4, true);
    coolDown = field_2FC;
    flags357 |= 0x80;
    return false;
}

// @func 0x001CF910 SoundActorB17270::AdvanceTo5
// A negative duration keeps it in slot 4.
bool SoundActorB17270::AdvanceTo5(float elapsed, float duration)
{
    if (EntityGetState(this, 1) != 0 || !engaged || (sequenceFlags & kSeq_Held) || duration < 0.0f ||
        elapsed < duration)
        return true;
    PlaySlot(5, false);
    flags357 &= ~0x80;
    return false;
}

// @func 0x001CFA10 SoundActorB17270::AdvanceTo6
// Slot 6 is passed through straight to slot 3.
bool SoundActorB17270::AdvanceTo6(float elapsed, float duration)
{
    if (B17270Waiting(this, elapsed, duration, true))
        return true;
    PlaySlot(6, false);
    PlaySlot(3, false);
    return false;
}

// @func 0x001CFB38 SoundActorB17270::AdvanceWrap
// Back to slot 1 only once disengaged.
bool SoundActorB17270::AdvanceWrap(float elapsed, float duration)
{
    if (EntityGetState(this, 1) != 0 || field_355 || engaged || (sequenceFlags & kSeq_Held) ||
        (duration >= 0.0f && elapsed < duration))
        return true;
    PlaySlot(1, false);
    return false;
}

// @func 0x001CFC30 SoundActorB17270::SetSlotState
void SoundActorB17270::SetSlotState(int index, bool immediate)
{
    if (index == 2 || index == 6) {
        if (!animScript)
            return;
        uint32_t state;
        if (index == 2)
            state = (field_355 && engaged) ? kB17270_Slot2Engaged : kSlotNames[2];
        else
            state = field_355 ? kB17270_Slot6Alt : kSlotNames[6];
        if (animScript->SetState(state) && immediate)
            animScript->Update(kScriptStep);
        return;
    }
    SoundActor::SetSlotState(index, immediate);
}

// @func 0x001CC338 SoundActorB17270::Release
bool SoundActorB17270::Release(bool kill)
{
    if (kill && linked) {
        linked->Release(false);
        linked = nullptr;
    }
    return SoundActor::Release(kill);
}

// @func 0x001CE8B0 SoundActorB17270::CanInteractWith
bool SoundActorB17270::CanInteractWith(Entity* other, int kind, float* priority)
{
    return kind == 0 && Entity::CanInteractWith(other, kind, priority);
}

// @func 0x001CEC8C SoundActorB17270::OnStruck
void SoundActorB17270::OnStruck()
{
    constexpr uint32_t kState_Struck = 0x803E1667u;   // NOTE: name unknown
    if (EntityGetState(this, 1) == 2) {
        if (!IsInvulnerable() && animScript)
            animScript->SetStateAndUpdate(kState_Struck);
    } else {
        EntitySetState(this, 1, 2);
        EntitySetState(this, 2, 0);
    }
}

// @func 0x001CED90 SoundActorB17270::OnHitCategory4
void SoundActorB17270::OnHitCategory4(EntityHitRecord& hit)
{
    const auto* d = static_cast<const ActorDef*>(def);
    if (!d || !ReceiveHit(hit) || hit.source == this)
        return;
    if (d->impactScale > 0.0f && engaged)
        OnStruck();
}

// @func 0x001CC440 SoundActorB17270::SetTransform
void SoundActorB17270::SetTransform(const ape::Matrix34& m)
{
    ActorEntity::SetTransform(m);
    baseTransform = m;
    aimFrame = baseTransform;
    ape::Vec3 aim = { aimFrame.zAxis.x, aimFrame.zAxis.y, aimFrame.zAxis.z, 0.0f };
    if (turretFlags & 0x80)
        aim.y = 0.0f;
    const float lengthSq = aim.z * aim.z + aim.x * aim.x + aim.y * aim.y;
    if (lengthSq >= 1e-05f) {
        const float inv = 1.0f / std::sqrt(lengthSq);
        aim = { aim.x * inv, aim.y * inv, aim.z * inv, 0.0f };
    }
    wantedAim = { aim.x, aim.y, aim.z, wantedAim.pad };
    currentAim = { aim.x, aim.y, aim.z, currentAim.pad };
    restAim = { aim.x, aim.y, aim.z, restAim.pad };
}

// @func 0x001CF620 SoundActorB17270::AimAt
void SoundActorB17270::AimAt(const ape::Vec3& point)
{
    const float dx = point.x - aimFrame.translation.x;
    const float dy = point.y - aimFrame.translation.y;
    const float dz = point.z - aimFrame.translation.z;
    const float inv = 1.0f / std::sqrt(dz * dz + dx * dx + dy * dy);
    wantedAim = { dx * inv, dy * inv, dz * inv, wantedAim.pad };
}

// @func 0x001CF690 SoundActorB17270::IsAimed
bool SoundActorB17270::IsAimed() const
{
    return std::fabs(currentAim.x - wantedAim.x) < 1e-05f && std::fabs(currentAim.y - wantedAim.y) < 1e-05f &&
           std::fabs(currentAim.z - wantedAim.z) < 1e-05f;
}

// @func 0x001CAA88 SoundActorB17270::ResetAim
void SoundActorB17270::ResetAim()
{
    const float interval = weapon ? SkinShotInterval(weapon) : 0.0f;
    if (shotCount > 1 && interval > 0.0f)
        shotDelay = float(shotCount - 1) * interval;
}

// @func 0x001CC174 SoundActorB17270::AddTarget
void SoundActorB17270::AddTarget(Entity* target)
{
    target->AddRef();
    targets.PushBack(target);
}

// @func 0x001CC214 SoundActorB17270::SetOnlyTarget
void SoundActorB17270::SetOnlyTarget(Entity* target)
{
    for (size_t i = 0; i < targets.Count(); ++i)
        targets[i]->Release(false);
    targets.Clear();
    if (target) {
        target->AddRef();
        targets.PushBack(target);
        field_328 = 0;
    }
}

// @func 0x001CFDB0 SoundActorB17270::ResumeSequence
void SoundActorB17270::ResumeSequence()
{
    PlayFrom(savedTime, savedSlot, true);
    field_355 = false;
    engaged = false;
    flags357 &= ~0x80;
}

// @func 0x001AF40C SoundActor::SetSlotState
void SoundActor::SetSlotState(int index, bool immediate)
{
    ape::ApeAnimScriptState* state = sounds[index].state;
    if (immediate)
        animScript->SetStateAndUpdate(state);
    else
        animScript->SetState(state);
}

// @func 0x001C33C4 SoundActorB16B48::~SoundActorB16B48
// @func 0x001C345C SoundActorB16B48::~SoundActorB16B48 (deleting)
// @func 0x001C3510 SoundActorB16B48::~SoundActorB16B48 (deleting thunk, +0x180)
// @func 0x001C35C8 SoundActorB16B48::~SoundActorB16B48 (deleting thunk, +0x17C)
// @func 0x001C3688 SoundActorB16B48::~SoundActorB16B48 (thunk, +0x180)
// @func 0x001C3724 SoundActorB16B48::~SoundActorB16B48 (thunk, +0x17C)
SoundActorB16B48::~SoundActorB16B48() = default;

// @func 0x001CBE10 SoundActorB17270::~SoundActorB17270
// @func 0x001CBE90 SoundActorB17270::~SoundActorB17270 (deleting)
// @func 0x001CBF2C SoundActorB17270::~SoundActorB17270 (deleting thunk, +0x180)
// @func 0x001CBFCC SoundActorB17270::~SoundActorB17270 (deleting thunk, +0x17C)
// @func 0x001CC06C SoundActorB17270::~SoundActorB17270 (thunk, +0x180)
// @func 0x001CC0F0 SoundActorB17270::~SoundActorB17270 (thunk, +0x17C)
SoundActorB17270::~SoundActorB17270()
{
    targets.Clear();
}

// @func 0x001D1D94 SoundActorB174C0::~SoundActorB174C0
// @func 0x001D1E0C SoundActorB174C0::~SoundActorB174C0 (deleting)
// @func 0x001D1EA0 SoundActorB174C0::~SoundActorB174C0 (deleting thunk, +0x180)
// @func 0x001D1F38 SoundActorB174C0::~SoundActorB174C0 (deleting thunk, +0x17C)
// @func 0x001D1FD8 SoundActorB174C0::~SoundActorB174C0 (thunk, +0x180)
// @func 0x001D2054 SoundActorB174C0::~SoundActorB174C0 (thunk, +0x17C)
SoundActorB174C0::~SoundActorB174C0() = default;

// @func 0x001EB5E0 ObjectActor::~ObjectActor
// @func 0x001EB6E4 ObjectActor::~ObjectActor (deleting)
// @func 0x001EB72C ObjectActor::~ObjectActor (deleting thunk, +0x180)
// @func 0x001EB778 ObjectActor::~ObjectActor (deleting thunk, +0x17C)
// @func 0x001EB7C4 ObjectActor::~ObjectActor (thunk, +0x180)
// @func 0x001EB7EC ObjectActor::~ObjectActor (thunk, +0x17C)
ObjectActor::~ObjectActor()
{
    if (object_1F4)
        object_1F4->Release();
}

// @func 0x001ED5F4 TweenActor::~TweenActor
// @func 0x001ED72C TweenActor::~TweenActor (deleting)
// @func 0x001ED774 TweenActor::~TweenActor (deleting thunk, +0x180)
// @func 0x001ED7C0 TweenActor::~TweenActor (deleting thunk, +0x17C)
// @func 0x001ED80C TweenActor::~TweenActor (thunk, +0x180)
// @func 0x001ED834 TweenActor::~TweenActor (thunk, +0x17C)
TweenActor::~TweenActor() = default;

// @func 0x001EEE40 TweenActor::Update
bool TweenActor::Update(float dt)
{
    return ActorEntity::Update(dt);
}

// @func 0x001EEE78 TweenActor::PostUpdate
bool TweenActor::PostUpdate(float dt)
{
    return ActorEntity::PostUpdate(dt);
}

// @func 0x001F4088 TweenActor::UpdateTransform
void TweenActor::UpdateTransform(float /*dt*/) {}

// @func 0x001F509C TweenActor::ApplyVisibility
void TweenActor::ApplyVisibility(bool visible)
{
    if (!visible && field_358)
        Reset(true);
}

namespace {
// Script message posted when an interaction with the tween actor begins.
// NOTE: names unknown.
constexpr uint32_t kMsgKey_TweenBegin = 0x55B67D7Cu;
constexpr uint32_t kMsg_TweenBegin = 0x57C66C13u;
constexpr uint32_t kMsg_Damaged = 0x69988172u;
} // namespace

// @func 0x001EE3E4 TweenActor::InteractionBegin
bool TweenActor::InteractionBegin(Entity* /*user*/)
{
    EntitySetState(this, 0, 1);
    EntityPostScriptMessage(this, kMsgKey_TweenBegin, kMsg_TweenBegin, 0);
    EntityComponent* interaction = EntityInteraction(this);
    Entity* user = InteractionUser(interaction);
    if (user)
        TakeUser(user);
    field_1FC = 0;
    tweenFlags &= ~(1u << 21);
    if (user)
        field_3C8 = PlayerBlockValue10D8(user->PlayerSlot() & 0xFF);
    return true;
}

// TODO(0x001EDD84 TweenActor::TakeUser)
// @func 0x001EDD84 TweenActor::TakeUser
void TweenActor::TakeUser(Entity* user)
{
    if (field_358) {
        if (!user) {
            EntityComponent* character = field_358->GetCharacterComponent();
            if (character && CharacterCompHasController(character)) {
                CharacterCompSetPoseHook(character, nullptr, nullptr);
                for (int32_t& bone : userBones)
                    bone = 0;
            }
            field_D0 = savedFieldD0;
            savedFieldD0 = 0.0f;
        }
        ReleaseUserProp();
        field_358->Release(false);
    }
    field_358 = (user && user->HasTypeBit(3)) ? user : nullptr;
    if (!field_358)
        return;
    savedFieldD0 = field_D0;
    field_D0 = 1000000.0f;
    ++field_358->interactionCount;
    EntityComponent* character = field_358->GetCharacterComponent();
    if (character && CharacterCompHasController(character)) {
        static constexpr uint32_t kBones[3] = {0xB3521F35u, 0xB3521F36u, 0xB3521F37u};
        for (int i = 0; i < 3; ++i)
            userBones[i] = field_358->FindBone(kBones[i]);
        CharacterCompSetPoseHook(character, &TweenActor::UserPoseHook, this);
    }
}

// @func 0x001EE4D8 TweenActor::ReleaseCamera
void TweenActor::ReleaseCamera()
{
    if (camera && cameraMode) {
        const float blend = (tweenFlags & (1u << 20)) ? -1.0f : float(g_tgtRangeCamPopTime);
        if (camera->HasMode(cameraMode))
            camera->RemoveMode(blend, cameraMode);
        delete cameraMode;
    }
    cameraMode = nullptr;
    camera = nullptr;
}

// @func 0x001F4440 TweenActor::UserPoseHook
void TweenActor::UserPoseHook(void* actor)
{
    static_cast<TweenActor*>(actor)->PoseUser();
}

// @func 0x001EE0CC TweenActor::Release
bool TweenActor::Release(bool kill)
{
    if (kill) {
        TakeUser(nullptr);
        ReleaseEntities334();
        ReleaseOwned(field_32C);
        ReleaseEntities33C();
        if (field_368)
            field_368->Release(false);
        field_368 = nullptr;
        ReleaseEntity330();
        if (phantom)
            camera::DestroyPhantom(phantom);   // out of its world (when in one) and dropped
        ReleaseAllOwned();
    }
    return ActorEntity::Release(kill);
}

// @func 0x001ED85C TweenActor::ReleaseOwned
void TweenActor::ReleaseOwned(Entity* entity)
{
    if (!entity)
        return;
    // NOTE: the definition is read as a physical one whatever its class.
    const auto* entityDef = static_cast<const PhysicalEntityDef*>(entity->def);
    if ((entityDef->defFlags & (1u << 21)) && entity->object) {
        if (ape::world::ApeObj* parent = entity->object->m_parent)
            parent->DetachChild(entity->object);
    }
    entity->Release(false);
    if (entity->flags & kEnt_Alive)
        entity->Release(true);
    if (entity == field_32C)
        field_32C = nullptr;
}

// @func 0x001ED964 TweenActor::ReleaseOwnedPair
void TweenActor::ReleaseOwnedPair(const OwnedPair& pair)
{
    if (pair.second)
        pair.second->Release(false);
    ReleaseOwned(pair.first);
}

// @func 0x001ED9D4 TweenActor::ReleaseAllOwned
void TweenActor::ReleaseAllOwned()
{
    for (const OwnedPair& pair : field_280)
        ReleaseOwnedPair(pair);
    field_280.Clear();
}

// @func 0x001EDA8C TweenActor::ReleaseEntity330
void TweenActor::ReleaseEntity330()
{
    Entity* entity = field_330;
    if (!entity)
        return;
    if (entity->object) {
        if (ape::world::ApeObj* parent = entity->object->m_parent)
            parent->DetachChild(entity->object);
    }
    entity->SetVisible(false, true, true, true);
    entity->Release(false);
    entity->Release(true);
    field_330 = nullptr;
}

// @func 0x001EDB90 TweenActor::ReleaseEntities33C
void TweenActor::ReleaseEntities33C()
{
    for (Entity*& entity : field_33C) {
        if (entity) {
            entity->Release(true);
            entity = nullptr;
        }
    }
}

// @func 0x001EDC2C TweenActor::ReleaseEntities334
void TweenActor::ReleaseEntities334()
{
    for (Entity** slot : {&field_334, &field_338}) {
        if (*slot) {
            (*slot)->Release(true);
            *slot = nullptr;
        }
    }
    tweenFlags &= ~(1u << 28);
}

// @func 0x001EDCD4 TweenActor::ReleaseUserProp
void TweenActor::ReleaseUserProp()
{
    if (!field_358)
        return;
    if (Entity* prop = field_358->FindDescendant(0x366FD481u)) {
        prop->SetVisible(false, true, true, true);
        prop->Release(true);
    }
}

// @func 0x001EE940 TweenActor::InteractionCancel
void TweenActor::InteractionCancel() { Reset(false); }

// @func 0x001EE964 TweenActor::InteractionEnd
void TweenActor::InteractionEnd(bool completed) { Reset(completed); }

// @func 0x001EEEB0 TweenActor::HandleMessage
// Remembers that the entity using the actor was hurt.
void TweenActor::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_Damaged && field_358 && MessageSenderEntity(msg) == field_358)
        tweenFlags |= 0x20000;
}

// @func 0x001F3FEC TweenActor::SetVisible
void TweenActor::SetVisible(bool visible, bool immediate, bool includeChildren, bool components)
{
    Entity::SetVisible(visible, immediate, includeChildren, components);
    if (visible)
        return;
    tweenFlags &= ~(1u << 29);
    if (field_368)
        field_368->Release(false);
    field_368 = nullptr;
}

// @func 0x001F6634 InteractivePropB::Release
bool InteractivePropB::Release(bool kill)
{
    if (kill && attached) {
        attached->Release(false);
        if (attached->IsAlive())
            attached->Release(true);
    }
    return ActorEntity::Release(kill);
}

// @func 0x001F66E8 InteractivePropB::InteractionBegin
bool InteractivePropB::InteractionBegin(Entity* /*user*/)
{
    Entity* user = InteractionUser(EntityInteraction(this));
    // NOTE: the console does not check for a missing user.
    if (!user) {
        EntitySetState(this, 0, 1);
        return true;
    }
    userTransform.xAxis = { user->transform.xAxis.x, user->transform.xAxis.y, user->transform.xAxis.z, userTransform.xAxis.pad };
    userTransform.yAxis = { user->transform.yAxis.x, user->transform.yAxis.y, user->transform.yAxis.z, userTransform.yAxis.pad };
    userTransform.zAxis = { user->transform.zAxis.x, user->transform.zAxis.y, user->transform.zAxis.z, userTransform.zAxis.pad };
    userTransform.translation = { user->Position().x, user->Position().y, user->Position().z, userTransform.translation.pad };
    EntitySetState(this, 0, 1);
    return true;
}

// @func 0x001F67B4 InteractivePropB::InteractionCancel
void InteractivePropB::InteractionCancel() { EntityInteraction(this); }

// @func 0x001F67D8 InteractivePropB::InteractionEnd
void InteractivePropB::InteractionEnd(bool completed) { (void)completed; EntityInteraction(this); }

// @func 0x001DEBA4 InteractivePropC::Release
bool InteractivePropC::Release(bool kill)
{
    if (kill) {
        camera::DestroyPhantom(phantomA);
        camera::DestroyPhantom(phantomB);
    }
    return ActorEntity::Release(kill);
}

// @func 0x001F7168 InteractivePropB::ApplyVisibility
void InteractivePropB::ApplyVisibility(bool visible)
{
    if (visible && !attached) {
        SpawnAttached();
        EntitySetState(this, 0, 0);
    } else if (!visible && attached) {
        DetachChild(attached);
        attached->Release(false);
        attached->Release(true);
        attached = nullptr;
        EntitySetState(this, 0, 3);
    }
}

// @func 0x001F6808 InteractivePropB::CanInteractWith
bool InteractivePropB::CanInteractWith(Entity* other, int kind, float* priority)
{
    constexpr uint32_t kVar_Usable = 0xDD61613Cu;   // NOTE: name unknown
    if (EntityGetState(this, 0) != 0 || !attached)
        return false;
    if (useTime <= 0.0f) {
        ape::ApeAnimScriptVar* var = animScript ? animScript->FindVar(kVar_Usable) : nullptr;
        if (!var || var->GetType() != ape::AnimScriptVarType::Int || var->GetInt() == 0)
            return false;
    }
    return ActorEntity::CanInteractWith(other, kind, priority);
}

// @func 0x001F54E8 InteractivePropB::InteractivePropB
InteractivePropB::InteractivePropB(const objects::InteractivePropBInstanceDesc& desc)
    : ActorEntity(desc), field_1F8(desc.field_100), useTime(desc.field_104), field_200(desc.field_108),
      field_204(desc.field_10C), field_210(desc.field_E0), rotation(desc.rotation), attachedDef(desc.field_D0)
{
    if (desc.field_110) {
        SpawnAttached();
        EntitySetState(this, 0, 0);
    }
    EntityJoinListenerGroup(this, 0x6927A078u, true);
}

// @func 0x001F5660 InteractivePropBDef::Spawn
Entity* InteractivePropBDef::Spawn(EntityPlacement* placement)
{
    objects::InteractivePropBInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* prop = new InteractivePropB(desc);
    prop->OnSpawn();
    return prop;
}

// TODO(0x001F50E4 InteractivePropB::SpawnAttached)
void InteractivePropB::SpawnAttached() {}

// @func 0x001E40E0 InteractivePropC::InteractionEnd
void InteractivePropC::InteractionEnd(bool completed)
{
    (void)completed;
    InteractionCancel();
}

// @func 0x001E4388 InteractivePropC::InteractionOffer
uint32_t InteractivePropC::InteractionOffer(int kind, Entity* other)
{
    (void)kind; (void)other;
    return 1;
}

// @func 0x001E4370 InteractivePropC::InteractionNeeds
uint32_t InteractivePropC::InteractionNeeds(int kind, Entity* other)
{
    (void)other;
    return static_cast<const InteractivePropADef*>(def)->interactionFlags[kind];
}

// @func 0x001E4230 InteractivePropC::GetAimPoint
ape::Vec3 InteractivePropC::GetAimPoint()
{
    const ape::Vec3& o = static_cast<const InteractivePropADef*>(def)->aimOffset;
    const ape::Matrix34& m = frameObject ? frameObject->GetWorld() : transform;
    return { m.translation.x + m.xAxis.x * o.x + m.yAxis.x * o.y + m.zAxis.x * o.z,
             m.translation.y + m.xAxis.y * o.x + m.yAxis.y * o.y + m.zAxis.y * o.z,
             m.translation.z + m.xAxis.z * o.x + m.yAxis.z * o.y + m.zAxis.z * o.z, 0.0f };
}

// @func 0x001E411C InteractivePropC::CanInteractWith
bool InteractivePropC::CanInteractWith(Entity* other, int kind, float* priority)
{
    if (kind == kInteract_Player && !(propFlags & 0x80) && other && (other->typeBits[0] & 0x08)) {
        EntityComponent* interaction = EntityInteraction(this);
        if (interaction && InteractionUsers(interaction) < 2 && CharacterCarried(other) != keyObject)
            return false;
    }
    return ActorEntity::CanInteractWith(other, kind, priority);
}

// -- InteractivePropA --------------------------------------------------------------------

// @func 0x001DC3A8 InteractivePropA::CanInteractWith
bool InteractivePropA::CanInteractWith(Entity* other, int kind, float* priority)
{
    if (kind == kInteract_Player && other && (other->classBits & 1)) {
        EntityComponent* interaction = EntityInteraction(this);
        const auto* d = static_cast<const InteractivePropAUseDef*>(def);
        if (interaction && InteractionUsers(interaction) < 2 && d->sharedUse == 0) {
            bool idleDriver = false;
            if (other->typeBits[0] & 0x08) {
                EntityComponent* driver = other->GetComponent(kComp_17);
                idleDriver = driver && AnimDriverState(driver) == 1;
            }
            if (PlayerBlockBusyTime(other->PlayerSlot() & 0xFF) <= 0.0f || idleDriver)
                return false;
        }
    }
    return ActorEntity::CanInteractWith(other, kind, priority);
}

// @func 0x001DC504 InteractivePropA::InteractionBegin
bool InteractivePropA::InteractionBegin(Entity* user)
{
    const auto* d = static_cast<const InteractivePropAUseDef*>(def);
    if (d->sharedUse == 0 && user && (user->typeBits[0] & 0x08)) {
        EntityComponent* driver = user->GetComponent(kComp_17);
        if (driver && AnimDriverState(driver) != 1)
            AnimDriverStop(driver, true);
    }
    return true;
}

// -- SearchlightActor ----------------------------------------------------------------------

// @func 0x001EAD18 SearchlightActor::InteractionBegin
bool SearchlightActor::InteractionBegin(Entity* newUser)
{
    user = newUser;
    if (!newUser)
        return true;
    GameCamera* camera = MainGameCamera();
    // NOTE: the console builds the mode with its full constructor
    // (0x00111FE0, not ported), which also takes the user and the light.
    auto* mode = new camera::SpotlightCameraMode(camera);
    mode->player = newUser;
    mode->spotlight = this;
    cameraMode = mode;
    if (camera)
        camera->PushMode(0.7f, mode, false);
    newUser->SetVisible(false, true, true, true);
    if (lightObject) {
        lightObject->SetVisible(true, false);
        if (lightObject->GetType() == ape::world::kObjType_GeomInstance)
            lightObject->VirtualSlot13(0, false);
    }
    return true;
}

// -- Safe ------------------------------------------------------------------------------------

// @func 0x001E6FA0 Safe::Release
bool Safe::Release(bool kill)
{
    if (kill) {
        GameWorldDequeueSafeCallback(this);
        audio::SoundNameRelease(sound248);
        audio::SoundNameRelease(sound24C);
        for (int i = 0; i < 3; ++i) {
            audio::SoundNameRelease(sounds250[i]);
            audio::SoundNameRelease(sounds25C[i]);
        }
        if (script228) {
            delete static_cast<ape::ApeAnimScriptInstance*>(script228);
            script228 = nullptr;
        }
    }
    return ActorEntity::Release(kill);
}

// @func 0x001E9050 Safe::InteractionCancel
void Safe::InteractionCancel()
{
    Entity* user = InteractionUser(EntityInteraction(this));
    GameCamera* camera = MainGameCamera();
    if (camera && cameraMode) {
        camera->RemoveMode(-1.0f, cameraMode);
        delete cameraMode;
        cameraMode = nullptr;
    }
    if (user)
        user->SetVisible(true, true, true, true);
    GameWorldSetHudHidden(false);
    GameWorldSetPlayerControl(true);
}

// @func 0x001E9164 Safe::InteractionEnd
void Safe::InteractionEnd(bool completed)
{
    (void)completed;
    EntityComponent* interaction = EntityInteraction(this);
    InteractionClearFlag20(interaction);
    InteractionSetOffer(interaction, 2);
}

// TODO(0x001EE680 TweenActor::Reset)
void TweenActor::Reset(bool flag)
{
    (void)flag;
}

// @func 0x001F7128 InteractivePropB::SetVisible
void InteractivePropB::SetVisible(bool visible, bool immediate, bool children, bool components)
{
    Entity::SetVisible(visible, immediate, children, components);
}

} // namespace entity
} // namespace sly
