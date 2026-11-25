// openTIT - Scripted actors (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "sly/entity/CharacterHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
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
