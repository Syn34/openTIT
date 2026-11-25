// openTIT - Scripted actors (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
#include "sly/entity/GameCamera.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

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
        field_218 = nullptr;
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

// TODO(0x001AED20 SoundActor::PlaySlot)
void SoundActor::PlaySlot(int index, bool force)
{
    (void)force;
    currentSlot = index;
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

// TODO(0x001C3EF0 SoundActorB16B48::SetLight)
void SoundActorB16B48::SetLight(bool on) { lightFlags = on ? (lightFlags | 0x80) : (lightFlags & ~0x80); }

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
    if (field_234 >= 0.0f)
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
    field_32C.Clear();
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
void TweenActor::TakeUser(Entity* user) { field_358 = user; }

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

// @func 0x001E411C InteractivePropC::CanInteractWith
bool InteractivePropC::CanInteractWith(Entity* other, int kind)
{
    if (kind == kInteract_Player && !(propFlags & 0x80) && other && (other->typeBits[0] & 0x08)) {
        EntityComponent* interaction = EntityInteraction(this);
        if (interaction && InteractionUsers(interaction) < 2 && CharacterCarried(other) != keyObject)
            return false;
    }
    return ActorCanInteractWith(this, other, kind);
}

// -- InteractivePropA --------------------------------------------------------------------

// @func 0x001DC3A8 InteractivePropA::CanInteractWith
bool InteractivePropA::CanInteractWith(Entity* other, int kind)
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
    return ActorCanInteractWith(this, other, kind);
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
