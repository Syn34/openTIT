// openTIT - Animation layers (0x0034F518-0x00354C18).
#include "sly/anim/AnimLayer.h"

#include "sly/anim/AnimController.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"

#include <algorithm>
#include <cstring>

namespace sly {
namespace anim {

ape::ApeTSortedMap<BoneWeightList*>& BoneWeightLists()
{
    static ape::ApeTSortedMap<BoneWeightList*> lists;
    return lists;
}

ape::ApeTSortedMap<void*>& ControlListeners()
{
    static ape::ApeTSortedMap<void*> listeners;
    return listeners;
}

uint32_t HashBoneName(const char* boneName)
{
    // Skeleton bone names may carry a "<rig>:" prefix that the weight lists leave out.
    const char* colon = std::strchr(boneName, ':');
    if (colon && std::strlen(colon) > 1)
        boneName = colon + 1;
    return ape::HashNameNoCase(boneName);
}

namespace {

// Bone names are stored with a flag in bit 0 of the pointer.
const char* BoneName(const HkaBone& bone)
{
    return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(bone.name) & ~uintptr_t(1));
}

// Index of the bone whose name hashes to `hash`, or -1.
int FindBone(const HkaSkeleton* skeleton, uint32_t hash)
{
    for (int i = 0; i < skeleton->boneCount; ++i) {
        if (HashBoneName(BoneName(skeleton->bones[i])) == hash)
            return i;
    }
    return -1;
}

uint8_t WeightByte(float weight)
{
    return uint8_t(int64_t(weight * 255.0f));
}

} // namespace

// Layers are built inline by AnimController::AddLayer (0x0035133C).
AnimLayer::AnimLayer(AnimController* owner_, uint32_t anim_) : owner(owner_), anim(anim_) {}

// @func 0x00353334 AnimLayer::~AnimLayer
// @func 0x0035357C AnimLayer::~AnimLayer (deleting)
AnimLayer::~AnimLayer()
{
    if (pose) {
        HkPoseDelete(pose);
        pose = nullptr;
    }
    if (skeleton)
        HkRemoveReference(skeleton);
}

// @func 0x00350178 AnimLayer::Bind
void AnimLayer::Bind(const HkaSkeleton* bones, bool force)
{
    if ((skeleton && !force) || !bones)
        return;
    if (pose) {
        HkPoseDelete(pose);
        pose = nullptr;
    }
    if (skeleton) {
        control = nullptr;
        HkRemoveReference(skeleton);
    }
    skeleton = HkNewAnimatedSkeleton(bones);
    skeleton->referencePoseWeightThreshold = 0.01f;
    pose = HkNewReferencePose(bones);
    if (owner)
        owner->BindSkeleton(bones, false);
}

// @func 0x003535C4 AnimLayer::AddSecondaryControl
void AnimLayer::AddSecondaryControl(HkaAnimationBinding* binding, bool loop, float speed, float time,
                                    uint32_t weightList)
{
    if (!binding || !skeleton)
        return;
    HkaAnimationControl* added = HkNewDefaultControl(binding, loop ? -1 : 1);
    secondary = added;
    added->localTime = time;
    added->playbackSpeed = speed;
    added->masterWeight = 1.0f;
    // The skeleton keeps the control alive from here on.
    HkSkeletonAddControl(skeleton, added);
    HkRemoveReference(added);
    if (weightList)
        SetBoneWeights(*BoneWeightLists().Find(weightList));
}

// @func 0x00350860 AnimLayer::AttachListener
void AnimLayer::AttachListener(uint32_t listenerName)
{
    if (!control)
        return;
    void* listener = nullptr;
    if (listenerName) {
        listener = *ControlListeners().Find(listenerName);
        if (!listener)
            return;
    }
    if (skeleton->controlCount > 0) {
        HkaAnimationControl* newest = skeleton->controls[skeleton->controlCount - 1];
        if (newest)
            HkControlAddListener(newest, listener);
    }
}

// @func 0x003540BC AnimLayer::AddControlListener
void AnimLayer::AddControlListener(void* listener)
{
    if (control)
        HkControlPushListener(control, listener);
}

// @func 0x00353748 AnimLayer::Duration
float AnimLayer::Duration() const
{
    return control ? control->Duration() : 0.0f;
}

// @func 0x00354088 AnimLayer::SecondaryDuration
float AnimLayer::SecondaryDuration() const
{
    return secondary ? secondary->Duration() : 0.0f;
}

// @func 0x00353780 AnimLayer::LocalTime
float AnimLayer::LocalTime() const
{
    return control ? control->localTime : 0.0f;
}

// @func 0x00354150 AnimLayer::SetLocalTime
void AnimLayer::SetLocalTime(float time)
{
    if (control)
        control->localTime = time;
}

// @func 0x003541B0 AnimLayer::PlaybackSpeed
float AnimLayer::PlaybackSpeed() const
{
    return control ? control->playbackSpeed : 0.0f;
}

// @func 0x003541E0 AnimLayer::SetPlaybackSpeed
void AnimLayer::SetPlaybackSpeed(float speed)
{
    if (!control)
        return;
    control->playbackSpeed = speed;
    flags &= ~(kFlag_Paused | kFlag_Reverse | kFlag_Forward);
    if (speed == 0.0f)
        flags |= kFlag_Paused;
    if (speed < 0.0f)
        flags |= kFlag_Reverse;
    if (speed > 0.0f)
        flags |= kFlag_Forward;
}

// @func 0x00354254 AnimLayer::SetSecondarySpeed
void AnimLayer::SetSecondarySpeed(float speed)
{
    if (secondary)
        secondary->playbackSpeed = speed;
}

// @func 0x00354184 AnimLayer::SetSecondaryMasterWeight
void AnimLayer::SetSecondaryMasterWeight(float w)
{
    if (secondary)
        secondary->masterWeight = w;
}

// @func 0x00353E88 AnimLayer::OverflowCount
uint32_t AnimLayer::OverflowCount() const
{
    return control ? control->overflowCount : 0;
}

// @func 0x00354198 AnimLayer::SetOverflowCount
void AnimLayer::SetOverflowCount(uint32_t count)
{
    if (control)
        control->overflowCount = count;
}

void AnimLayer::EaseOutNow(HkaAnimationControl* c)
{
    // Turn an ease-in into an ease-out from the same blend point.
    if (c->easeStatus < HkaAnimationControl::kEase_EasingOut)
        c->easeT = 1.0f - c->easeT;
    c->easeInvDuration = 3.4028e38f;   // NOTE: 0x7F7FFFEE on the console: practically instant
    c->easeStatus = c->easeT == 1.0f ? HkaAnimationControl::kEase_EasedOut : HkaAnimationControl::kEase_EasingOut;
}

// @func 0x00353CC0 AnimLayer::Stop
void AnimLayer::Stop(bool remove)
{
    if (!skeleton || !control)
        return;
    for (int i = 0; i < skeleton->controlCount; ++i) {
        HkaAnimationControl* c = skeleton->controls[i];
        if (c != control)
            continue;
        if (remove) {
            control = nullptr;
            HkSkeletonRemoveControl(skeleton, c);
        } else {
            EaseOutNow(c);
        }
        return;
    }
}

// @func 0x00353DC8 AnimLayer::StopSecondary
void AnimLayer::StopSecondary()
{
    if (!skeleton || !secondary)
        return;
    for (int i = 0; i < skeleton->controlCount; ++i) {
        HkaAnimationControl* c = skeleton->controls[i];
        if (c == secondary) {
            EaseOutNow(c);
            return;
        }
    }
}

bool AnimLayer::ControlDone(const HkaAnimationControl* c, bool countLoops) const
{
    if (c->easeStatus >= HkaAnimationControl::kEase_EasingOut)
        return true;
    // Both controls are measured against the main clip.
    const float end = Duration();
    const bool atEnd = !(flags & kFlag_Looping) && !(flags & kFlag_Paused) &&
                       ((c->localTime == end && (flags & kFlag_Forward)) ||
                        (c->localTime == 0.0f && (flags & kFlag_Reverse)));
    if (atEnd)
        return true;
    return countLoops && c->overflowCount >= 1;
}

// @func 0x00353EB0 AnimLayer::IsDone
bool AnimLayer::IsDone(bool countLoops) const
{
    return control ? ControlDone(control, countLoops) : true;
}

// @func 0x00353F98 AnimLayer::IsSecondaryDone
bool AnimLayer::IsSecondaryDone(bool countLoops) const
{
    return secondary ? ControlDone(secondary, countLoops) : true;
}

// @func 0x0034CD50 AnimLayer::HasActiveControls
bool AnimLayer::HasActiveControls() const
{
    if (!skeleton)
        return false;
    for (int i = 0; i < skeleton->controlCount; ++i) {
        if (skeleton->controls[i]->easeStatus < HkaAnimationControl::kEase_EasedOut)
            return true;
    }
    return false;
}

// @func 0x003537A8 AnimLayer::Update
void AnimLayer::Update(float dt, bool /*flag*/, bool noEvents)
{
    if (!skeleton || skeleton->controlCount < 1)
        return;
    HkSkeletonStepDeltaTime(skeleton, dt);

    // Latch the end of the main clip.
    if ((flags & kFlag_Finished) || !control) {
        flags &= ~kFlag_JustFinished;
    } else {
        const bool done = control->easeStatus >= HkaAnimationControl::kEase_EasingOut ||
                          (!(flags & kFlag_Looping) && control->localTime == control->Duration());
        flags &= ~(kFlag_JustFinished | kFlag_Finished);
        if (done)
            flags |= kFlag_JustFinished | kFlag_Finished;
    }

    // Prune eased-out controls; a lone control always stays. Only the first
    // one found is removed per update.
    HkaAnimationControl* expired = nullptr;
    bool specialExpired = false;
    int found = 0;
    for (int i = 0; i < skeleton->controlCount && found < 20; ++i) {
        if (skeleton->controlCount < 2)
            continue;
        HkaAnimationControl* c = skeleton->controls[i];
        if (c->easeStatus != HkaAnimationControl::kEase_EasedOut)
            continue;
        if (c->binding->animation->type == HkaAnimation::kType_Special)
            specialExpired = true;
        if (!found)
            expired = c;
        ++found;
    }
    if (expired) {
        HkSkeletonRemoveControl(skeleton, expired);
        if (specialExpired)
            skeleton->field_24 = skeleton->controlCount;
        if (expired == control)
            control = nullptr;
        if (expired == secondary)
            secondary = nullptr;
    }

    if (noEvents)
        return;
    AnimController* ctl = owner;
    if (!(ctl->flags & AnimController::kFlag_Running))
        return;
    bool animate = (ctl->flags & (AnimController::kFlag_AlwaysAnimate | AnimController::kFlag_NearHidden)) != 0;
    if (!animate) {
        ape::world::ApeObj* obj = ctl->object;
        animate = obj->GetType() != ape::world::kObjType_GeomInstance ||
                  static_cast<const ape::world::ApeRenderObj*>(obj)->IsDrawnThisFrame() ||
                  (ctl->flags & AnimController::kFlag_NoLod);
    }
    if (!animate || skeleton->controlCount < 1)
        return;
    ctl->flags |= AnimController::kFlag_PoseDirty;

    // Annotations crossed by the main clip become animation events.
    if (ctl->eventCallback && control && control->binding->animation->annotationTrackCount > 0) {
        HkaAnnotation annotations[16];
        const uint32_t count = HkAnimationGetAnnotations(control->binding->animation, control->localTime, dt,
                                                         annotations, 16);
        for (uint32_t i = 0; i < count; ++i) {
            if (control->localTime <= annotations[i].time) {
                const uint32_t event =
                    uint32_t(reinterpret_cast<uintptr_t>(annotations[i].text) & ~uintptr_t(1));
                ctl->eventCallback(event, true, ctl->eventUserData);
            }
        }
    }
    HkSkeletonSampleIntoPose(skeleton, pose);
}

// @func 0x00354288 AnimLayer::SetBoneWeights
void AnimLayer::SetBoneWeights(const BoneWeightList* list)
{
    if (!control || !list)
        return;
    const HkaSkeleton* bones = skeleton->skeleton;
    for (int i = 0; i < bones->boneCount; ++i)
        control->transformTrackWeights[i] = 0;
    for (uint32_t e = 0; e < list->count; ++e) {
        const BoneWeight& entry = list->entries[e];
        const int bone = FindBone(bones, entry.bone);
        if (bone < 0)
            continue;
        const uint8_t w = WeightByte(std::max(entry.weight, entry.minWeight));
        control->transformTrackWeights[bone] = w;
        // NOTE: with `children` the console rewrites the same bone once per
        // following bone instead of weighting the following bones (the
        // blend layer does weight them); kept.
        if (entry.children) {
            for (int next = bone + 1; next < bones->boneCount; ++next)
                control->transformTrackWeights[bone] = w;
        }
    }
}

// -- blend layer ------------------------------------------------------------------

// @func 0x003516C4 AnimBlendLayer::AnimBlendLayer
AnimBlendLayer::AnimBlendLayer() : AnimLayer(nullptr, 0), weightsA(0x40), weightsB(0x40)
{
    flags |= kFlag_Blend;
}

// @func 0x003544AC AnimBlendLayer::~AnimBlendLayer
// @func 0x00354598 AnimBlendLayer::~AnimBlendLayer (deleting)
AnimBlendLayer::~AnimBlendLayer() = default;

// @func 0x003546D0 AnimBlendLayer::Update
void AnimBlendLayer::Update(float dt, bool flag, bool noEvents)
{
    AnimLayer::Update(dt, flag, noEvents);
    if (!skeleton || skeleton->controlCount == 0)
        return;
    const float end = control ? control->Duration() : 0.0f;
    const float time = control ? control->localTime : 0.0f;
    if (end == time) {
        const float speed = control ? control->playbackSpeed : 0.0f;
        if (speed == 0.0f && !holdLastFrame) {
            // Holding the last frame: fade out over a quarter second.
            const float t = std::clamp(fadeTime, 0.0f, 0.25f);
            effectiveWeight = 1.0f - t * 4.0f;
            fadeTime = t + dt;
            return;
        }
    }
    fadeTime = 0.0f;
    effectiveWeight = weight;
}

// @func 0x003547EC AnimBlendLayer::SetBoneWeights
void AnimBlendLayer::SetBoneWeights(const BoneWeightList* list)
{
    if (!skeleton || list == weightList)
        return;
    weightsA.Clear();
    weightsB.Clear();
    const HkaSkeleton* bones = skeleton->skeleton;
    for (uint32_t e = 0; e < list->count; ++e) {
        const BoneWeight& entry = list->entries[e];
        const int bone = FindBone(bones, entry.bone);
        if (bone < 0)
            continue;
        const int last = entry.children ? bones->boneCount - 1 : bone;
        for (int b = bone; b <= last; ++b) {
            weightsA.Remove(uint32_t(b));
            weightsA.Add(uint32_t(b), entry.weight);
            weightsB.Remove(uint32_t(b));
            weightsB.Add(uint32_t(b), entry.minWeight);
        }
    }
    weightList = list;
}

// @func 0x0034CCB8 AnimBlendLayer::BoneWeightA
float AnimBlendLayer::BoneWeightA(uint32_t bone) const
{
    return *const_cast<ape::ApeTSortedMap<float>&>(weightsA).Find(bone) * effectiveWeight;
}

// @func 0x0034CD04 AnimBlendLayer::BoneWeightB
float AnimBlendLayer::BoneWeightB(uint32_t bone) const
{
    return *const_cast<ape::ApeTSortedMap<float>&>(weightsB).Find(bone) * effectiveWeight;
}

} // namespace anim
} // namespace sly
