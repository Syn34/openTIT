// openTIT - <Anim> component (type 2, vtable 0x00B1C880).
#include "sly/components/AnimComponent.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"
#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentHooks.h"
#include "sly/entity/GameEntityHooks.h"

#include "ape/anim/ApeAnimClip.h"

namespace sly {
namespace components {

namespace {

// The clip named `clip` in `set` or the nearest parent that has one
// (0x00760C54, AnimSet::FindClip).
const ape::anim::AnimClipDef* FindClipInSet(const ape::anim::AnimSet* set, uint32_t clip)
{
    for (; set; set = set->parent) {
        if (set->clips) {
            if (const ape::anim::AnimClipDef* found = *set->clips->Find(clip))
                return found;
        }
    }
    return nullptr;
}

} // namespace

// @func 0x00260464 AnimComponent::AnimComponent
AnimComponent::AnimComponent(const AnimComponentDesc* desc)
    : EntityComponent(desc), animSetName(desc->animSet), animTree(FindAnimTree(desc->animTree)),
      setSlots(4)
{
    // One anim set slot for the set and each of its parents; the set's own
    // slot is the current one.
    ape::anim::AnimClipManager* clips = GameAnimClipManager();
    bool first = true;
    for (ape::anim::AnimSet* set = clips ? clips->FindSet(animSetName) : nullptr; set; set = set->parent) {
        // NOTE: the console passes the owner entity as the slot's user value;
        // the port's 32-bit field cannot hold a pointer.
        uint32_t handle = 0;
        clips->AcquireSlot(ape::anim::AnimSetSlotRequest{ set, 0 }, &handle);
        setSlots.PushBack(handle);
        if (first) {
            first = false;
            currentSlot = handle;
        }
    }
}

void AnimComponent::ReleaseSetSlots()
{
    ape::anim::AnimClipManager* clips = GameAnimClipManager();
    for (uint32_t handle : setSlots) {
        if (clips)
            clips->ReleaseSlot(handle);
    }
}

void AnimComponent::StopAllSounds()
{
    while (soundCount > 0) {
        audio::StopVoice(sounds[0].voice, true);
        --soundCount;
        sounds[0] = sounds[soundCount];
        sounds[soundCount] = AttachedSound{};
    }
}

// @func 0x00297900 AnimComponent::~AnimComponent
// @func 0x00297B54 AnimComponent::~AnimComponent (deleting)
AnimComponent::~AnimComponent()
{
    ReleaseSetSlots();
    StopAllSounds();
}

// @func 0x002977B4 AnimComponent::ResetState
void AnimComponent::ResetState()
{
    // NOTE: the handles stay in the array (the console does the same).
    ReleaseSetSlots();
    StopAllSounds();
}

// @func 0x00297E00 AnimComponent::Update
void AnimComponent::Update(float dt)
{
    if (dt >= 1e-5f || dt <= -1e-5f)
        UpdateSounds();
}

// @func 0x00297BA8 AnimComponent::UpdateSounds
void AnimComponent::UpdateSounds()
{
    for (uint32_t i = 0; i < soundCount; ++i) {
        audio::SoundParams params;
        if (entity::GetVoiceParams(sounds[i].voice, params))
            continue;
        audio::StopVoice(sounds[i].voice, true);
        --soundCount;
        sounds[i] = sounds[soundCount];
        sounds[soundCount] = AttachedSound{};
        --i;
    }
}

// @func 0x00297E38 AnimComponent::PostUpdate
void AnimComponent::PostUpdate(float) {}

// @func 0x002980AC AnimComponent::SetController
void AnimComponent::SetController(anim::AnimController* controller_)
{
    controller = controller_;
    if (!controller)
        return;
    if (!controller->layers || controller->layers->Count() == 0)
        AnimControllerBuildLayers(controller, animTree);
    if (!controller->layers)
        return;
    for (uint32_t i = 0; i < controller->layers->Count() && i < uint32_t(kMaxLayers); ++i)
        layers[i].anim = (*controller->layers)[i]->anim;
}

// @func 0x00297E3C AnimComponent::CurrentSetName
uint32_t AnimComponent::CurrentSetName() const
{
    ape::anim::AnimClipManager* clips = GameAnimClipManager();
    if (!clips || (currentSlot & 0xFFFF) == 0)
        return 0;
    const ape::anim::AnimSetSlot* slot = clips->slots.Get(currentSlot);
    return slot && slot->set ? slot->set->nameHash : 0;
}

// @func 0x00298B44 AnimComponent::HasClip
bool AnimComponent::HasClip(uint32_t clip) const
{
    ape::anim::AnimClipManager* clips = GameAnimClipManager();
    if (!clips || (currentSlot & 0xFFFF) == 0)
        return false;
    const ape::anim::AnimSetSlot* slot = clips->slots.Get(currentSlot);
    return slot && slot->set && FindClipInSet(slot->set, clip) != nullptr;
}

AnimComponent::LayerSlot* AnimComponent::FindSlot(uint32_t anim)
{
    for (LayerSlot& slot : layers) {
        if (slot.anim == anim)
            return &slot;
    }
    return nullptr;
}

const AnimComponent::LayerSlot* AnimComponent::FindSlot(uint32_t anim) const
{
    return const_cast<AnimComponent*>(this)->FindSlot(anim);
}

anim::AnimLayer* AnimComponent::Layer(uint32_t anim) const
{
    return controller ? controller->FindLayer(anim) : nullptr;
}

// @func 0x00298874 AnimComponent::IsLoopingLayer
bool AnimComponent::IsLoopingLayer(uint32_t anim) const
{
    const LayerSlot* slot = FindSlot(anim);
    if (!slot || !slot->clip)
        return false;
    return (slot->clip->flags & ape::anim::kClip_Loop) != 0;
}

// @func 0x00298D98 AnimComponent::RefreshLayerSlot
void AnimComponent::RefreshLayerSlot(uint32_t anim)
{
    LayerSlot* slot = FindSlot(anim);
    if (!slot)
        return;
    anim::AnimLayer* layer = Layer(anim);
    slot->done = layer ? layer->IsDone(false) : true;
    slot->time = layer ? layer->LocalTime() : 0.0f;
    slot->loops = layer ? layer->OverflowCount() : 0;
}

// @func 0x00298270 AnimComponent::PlayAdditive
bool AnimComponent::PlayAdditive(uint32_t clip, uint32_t anim, bool loop, uint32_t /*unused*/)
{
    auto* binding = controller ? static_cast<anim::HkaAnimationBinding*>(anim::FindAnimClip(clip)) : nullptr;
    if (!binding)
        return false;   // NOTE: the console warns with the owner's name
    anim::AnimLayer* layer = controller->FindLayer(anim);
    if (!layer)
        return true;    // NOTE: warns as well, but reports success
    binding->blendHint = anim::HkaAnimationBinding::kBlend_Additive;
    // NOTE: the caller's fourth argument lands in an unused register; no
    // bone weight list is applied.
    layer->AddSecondaryControl(binding, loop, 1.0f, 0.0f, 0);
    layer->SetSecondaryMasterWeight(field_17C);
    return true;
}

// @func 0x00276A20 AnimComponent::SetLayerTime
void AnimComponent::SetLayerTime(float time, bool normalized, uint32_t anim)
{
    anim::AnimLayer* layer = Layer(anim);
    if (!layer)
        return;
    if (normalized) {
        const float length = layer->Duration();
        time = length > 1e-5f ? length * time : 1.0f;
    }
    layer->SetLocalTime(time);
}

// @func 0x00276AD8 AnimComponent::LayerSpeed
float AnimComponent::LayerSpeed(uint32_t anim) const
{
    anim::AnimLayer* layer = Layer(anim);
    return layer ? layer->PlaybackSpeed() : 0.0f;
}

// @func 0x00276B40 AnimComponent::LayerDuration
float AnimComponent::LayerDuration(uint32_t anim) const
{
    anim::AnimLayer* layer = Layer(anim);
    return layer ? layer->Duration() : 0.0f;
}

// @func 0x00276E4C AnimComponent::SetLayerSpeed
void AnimComponent::SetLayerSpeed(float speed, uint32_t anim)
{
    if (anim::AnimLayer* layer = Layer(anim))
        layer->SetPlaybackSpeed(speed);
}

// @func 0x00298748 AnimComponent::StopSecondary
void AnimComponent::StopSecondary(uint32_t anim)
{
    if (anim::AnimLayer* layer = Layer(anim))
        layer->StopSecondary();
}

// @func 0x0029879C AnimComponent::IsDone
bool AnimComponent::IsDone(uint32_t anim, bool countLoops) const
{
    anim::AnimLayer* layer = Layer(anim);
    return layer ? layer->IsDone(countLoops) : true;
}

// @func 0x00298808 AnimComponent::IsSecondaryDone
bool AnimComponent::IsSecondaryDone(uint32_t anim, bool countLoops) const
{
    anim::AnimLayer* layer = Layer(anim);
    return layer ? layer->IsSecondaryDone(countLoops) : true;
}

// @func 0x002988EC AnimComponent::ResetLoops
void AnimComponent::ResetLoops(uint32_t anim)
{
    if (anim::AnimLayer* layer = Layer(anim))
        layer->SetOverflowCount(0);
}

// @func 0x00298948 AnimComponent::Progress
float AnimComponent::Progress(uint32_t anim) const
{
    anim::AnimLayer* layer = Layer(anim);
    return layer ? layer->LocalTime() / layer->Duration() : 0.0f;
}

// @func 0x002989D8 AnimComponent::LayerTime
float AnimComponent::LayerTime(bool normalized, uint32_t anim) const
{
    anim::AnimLayer* layer = Layer(anim);
    if (!layer)
        return 0.0f;
    const float time = layer->LocalTime();
    if (!normalized)
        return time;
    const float length = layer->Duration();
    return length > 1e-5f ? time / length : 1.0f;
}

} // namespace components
} // namespace sly
