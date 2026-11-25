// openTIT - The game's animation controller (0x0034A398-0x00351A40).
#include "sly/anim/AnimController.h"

#include "sly/anim/AnimLayer.h"
#include "sly/entity/EntityHooks.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeBucketAlloc.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"

#include <cmath>

namespace sly {
namespace anim {

ape::ApeTArray<AnimController*>& AnimControllers()
{
    static ape::ApeTArray<AnimController*> controllers;
    return controllers;
}

AnimSettings& Anim()
{
    static AnimSettings settings;
    return settings;
}

ape::ApeTSortedMap<void*>& AnimClips()
{
    static ape::ApeTSortedMap<void*> clips;
    return clips;
}

// @func 0x0034A398 FindAnimClip
void* FindAnimClip(uint32_t name)
{
    return *AnimClips().Find(name);
}

ape::ApeTSortedMap<void*>& AnimTrees()
{
    static ape::ApeTSortedMap<void*> trees;
    return trees;
}

ape::ApeTSortedMap<void*>& AnimBlendMaps()
{
    static ape::ApeTSortedMap<void*> maps;
    return maps;
}

ape::ApeTSortedMap<void*>& AnimSkeletons()
{
    static ape::ApeTSortedMap<void*> skeletons;
    return skeletons;
}

// @func 0x003482B0 FindAnimTree
void* FindAnimTree(uint32_t name)
{
    return *AnimTrees().Find(name);
}

// @func 0x00348650 FindAnimBlendMap
void* FindAnimBlendMap(uint32_t name)
{
    return *AnimBlendMaps().Find(name);
}

// @func 0x00348ECC FindAnimSkeleton
void* FindAnimSkeleton(uint32_t name)
{
    return *AnimSkeletons().Find(name);
}

// @func 0x003489B0 FindBoneWeightList
BoneWeightList* FindBoneWeightList(uint32_t name)
{
    return *BoneWeightLists().Find(name);
}

namespace {

float DistanceToCamera(const ape::world::ApeObj* object)
{
    const ape::Vec3 camera = entity::GameCameraPosition();
    const ape::Vec3& pos = object->GetWorld().translation;
    const float dx = pos.x - camera.x;
    const float dy = pos.y - camera.y;
    const float dz = pos.z - camera.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool IsDrawnGeometry(const ape::world::ApeObj* object)
{
    return static_cast<const ape::world::ApeRenderObj*>(object)->IsDrawnThisFrame();
}

} // namespace

// @func 0x0034BDEC AnimController::AnimController
AnimController::AnimController(ape::world::ApeObj* object_) : object(object_)
{
    flags = kFlag_20 | kFlag_40 | kFlag_LayerFlag | kFlag_2000 | kFlag_Running | kFlag_NoLod;
    // Controllers take turns at the distance LOD updates.
    AnimSettings& anim = Anim();
    const int phases = anim.lodPhases > 1 ? anim.lodPhases : 1;
    phase = uint8_t(anim.nextPhase % phases);
    ++anim.nextPhase;
    lodDistance = anim.defaultLodDistance;
    AnimControllers().PushBack(this);
}

// @func 0x0034BF9C AnimController::~AnimController
AnimController::~AnimController()
{
    // Bone objects go back to the default pivot.
    if (boneObjects && rig) {
        if (object->GetType() == ape::world::kObjType_CameraInstance) {
            object->m_pivot = &ape::world::kWorldIdentity;
        } else {
            for (int32_t i = 0; i < rig->skeleton->boneCount; ++i) {
                if (boneObjects[i])
                    boneObjects[i]->m_pivot = &ape::world::kWorldIdentity;
            }
        }
    }
    if (boneBuffer) {
        ape::BucketFree(boneBuffer, boneBufferSize);
        boneBuffer = nullptr;
    }
    if (boneObjects) {
        ape::BucketFree(boneObjects, boneObjectsSize);
        boneObjects = nullptr;
    }
    ape::ApeTArray<AnimController*>& all = AnimControllers();
    for (uint32_t i = 0; i < all.Count(); ++i) {
        if (all[i] == this) {
            all.RemoveAtSwap(i);
            break;
        }
    }
    if (pose) {
        HkPoseDelete(pose);
        pose = nullptr;
    }
    if (rig) {
        HkRemoveReference(rig);
        rig = nullptr;
    }
    if (pose2) {
        HkPoseDelete(pose2);
        pose2 = nullptr;
    }
    if (rig2) {
        HkRemoveReference(rig2);
        rig2 = nullptr;
    }
    if (block74) {
        ape::BucketFree(block74, 0xB0);
        block74 = nullptr;
    }
    if (records6C) {
        for (AnimRecord140& record : *records6C)
            delete record.object;
    }
    if (layers) {
        for (AnimLayer* layer : *layers)
            delete layer;
    }
    delete field_70;
    delete records6C;
    delete field_68;
    delete records64;
    delete records60;
    delete layers;
    field_70 = nullptr;
    records6C = nullptr;
    field_68 = nullptr;
    records64 = nullptr;
    records60 = nullptr;
    layers = nullptr;
    // NOTE: the console's listener pool (+0x44) goes back to its allocator;
    // the port keeps the listeners in place.
}

// @func 0x0034CC34 AnimController::FindLayer
AnimLayer* AnimController::FindLayer(uint32_t anim) const
{
    if (layers) {
        for (AnimLayer* layer : *layers) {
            if (layer->anim == anim)
                return layer;
        }
    }
    if (anim == 0 && layers && layers->Count() >= 2)
        return (*layers)[0];
    return nullptr;
}

// @func 0x0034F690 AnimController::BindSkeleton
void AnimController::BindSkeleton(const HkaSkeleton* bones, bool force)
{
    if ((rig && !force) || !bones)
        return;
    if (rig) {
        HkRemoveReference(rig);
        rig = nullptr;
    }
    if (pose) {
        HkPoseDelete(pose);
        pose = nullptr;
    }
    if (pose2) {
        HkPoseDelete(pose2);
        pose2 = nullptr;
    }
    if (rig2) {
        HkRemoveReference(rig2);
        rig2 = nullptr;
    }
    rig = HkNewAnimatedSkeleton(bones);
    if (rig)
        rig->referencePoseWeightThreshold = 0.01f;
    pose = HkNewReferencePose(bones);

    if (boneObjects) {
        ape::BucketFree(boneObjects, boneObjectsSize);
        boneObjectsSize = 0;
    }
    if (boneBuffer) {
        ape::BucketFree(boneBuffer, boneBufferSize);
        boneBufferSize = 0;
    }
    boneObjects = nullptr;
    boneBuffer = nullptr;
    // Objects with a skeleton of their own are posed through it.
    if (object->GetSkeleton())
        return;

    auto initPivot = [](BonePivot& pivot) {
        pivot.transform = ape::world::kWorldIdentity;
        pivot.scale = ape::world::kWorldUnitScale;
    };
    if (object->GetType() == ape::world::kObjType_CameraInstance) {
        // A camera is posed as a whole.
        boneBufferSize = sizeof(BonePivot);
        auto* pivots = static_cast<BonePivot*>(ape::BucketAlloc(boneBufferSize));
        boneObjectsSize = sizeof(ape::world::ApeObj*);
        boneObjects = static_cast<ape::world::ApeObj**>(ape::BucketAlloc(boneObjectsSize));
        initPivot(pivots[0]);
        boneBuffer = pivots;
        object->m_pivot = &pivots[0].transform;
        boneObjects[0] = object;
        return;
    }
    // One pivot per Havok bone, placed on the object of the same name
    // (bone 0 is the object itself).
    const int32_t count = bones->boneCount;
    boneBufferSize = uint32_t(count) * sizeof(BonePivot);
    auto* pivots = static_cast<BonePivot*>(ape::BucketAlloc(boneBufferSize));
    boneObjectsSize = uint32_t(count) * sizeof(ape::world::ApeObj*);
    boneObjects = static_cast<ape::world::ApeObj**>(ape::BucketAlloc(boneObjectsSize));
    boneBuffer = pivots;
    for (int32_t i = 0; i < count; ++i) {
        const char* name = reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(bones->bones[i].name) & ~uintptr_t(1));
        const char* colon = ape::StrRChr(name, ':');
        if (colon && ape::StrLen(colon) > 1)
            name = colon + 1;
        ape::world::ApeObj* boneObject = i == 0 ? object : FindDescendant(object, ape::HashNameNoCase(name));
        initPivot(pivots[i]);
        boneObjects[i] = boneObject;
        if (boneObject)
            boneObject->m_pivot = &pivots[i].transform;
    }
}

// @func 0x0035133C AnimController::AddLayer
AnimLayer* AnimController::AddLayer(uint32_t anim)
{
    auto* layer = new AnimLayer(this, anim);
    layer->Bind(rig ? rig->skeleton : nullptr, false);
    if (!layers)
        layers = new ape::ApeTArray<AnimLayer*>();
    layers->PushBack(layer);
    return layer;
}

// @func 0x00350764 AnimController::SetSkeleton
void AnimController::SetSkeleton(uint32_t name, bool layersToo, bool force)
{
    auto* bones = static_cast<const HkaSkeleton*>(FindAnimSkeleton(name));
    if (!bones)
        return;   // NOTE: the console formats a (compiled-out) warning with the name
    BindSkeleton(bones, force);
    if (layersToo && layers) {
        for (AnimLayer* layer : *layers)
            layer->Bind(bones, force);
    }
}

// @func 0x00351898 AnimController::AddBlendLayer
AnimLayer* AnimController::AddBlendLayer(uint32_t anim, uint32_t weightList, bool holdLastFrame)
{
    auto* layer = new AnimBlendLayer();
    layer->anim = anim;
    layer->owner = this;
    layer->Bind(rig ? rig->skeleton : nullptr, false);
    layer->holdLastFrame = holdLastFrame;
    layer->SetBoneWeights(FindBoneWeightList(weightList));
    if (layer->weightList)
        layer->defaultWeightList = layer->weightList;
    if (!layers)
        layers = new ape::ApeTArray<AnimLayer*>();
    layers->PushBack(layer);
    return layer;
}

// @func 0x00351A40 AnimController::BuildLayers
void AnimController::BuildLayers(const AnimTree* tree)
{
    if (!tree)
        return;
    for (uint32_t i = 0; i < tree->count; ++i) {
        const AnimTreeLayer& entry = tree->layers[i];
        if (entry.kind == AnimTreeLayer::kKind_Plain)
            AddLayer(entry.anim);
        else if (entry.kind == AnimTreeLayer::kKind_Blend)
            AddBlendLayer(entry.anim, entry.weightList, entry.holdLastFrame);
    }
}

// @func 0x003514AC AnimController::Play
AnimLayer* AnimController::Play(HkaAnimationBinding* binding, float speed, float blendTime, float startTime,
                                bool loop, uint32_t listener)
{
    if (!rig) {
        // The clip names the skeleton it was made for.
        const char* skeletonName = reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(binding->skeletonName) & ~uintptr_t(1));
        auto* bones = static_cast<const HkaSkeleton*>(FindAnimSkeleton(ape::HashNameNoCase(skeletonName)));
        // NOTE: a missing skeleton only fed a compiled-out warning.
        BindSkeleton(bones, false);
    }
    AnimLayer* layer = (layers && layers->Count() != 0) ? (*layers)[0] : AddLayer(0);
    layer->Play(binding, speed, blendTime, startTime, loop);
    layer->AttachListener(listener);
    return layer;
}

// @func 0x0034CDB0 AnimController::IsAnimatedThisFrame
bool AnimController::IsAnimatedThisFrame() const
{
    if (!(flags & kFlag_Running))
        return false;
    if ((flags & (kFlag_AlwaysAnimate | kFlag_NearHidden)) || object->GetType() != ape::world::kObjType_GeomInstance)
        return true;
    return IsDrawnGeometry(object) || (flags & kFlag_NoLod);
}

// @func 0x0034C918 AnimController::UpdateLayers
void AnimController::UpdateLayers(float dt)
{
    AnimSettings& anim = Anim();
    if (!anim.enabled || !(flags & kFlag_Running))
        return;

    // Far away controllers only sample their layers on their turn.
    bool skip = false;
    if (anim.distanceLod && (flags & kFlag_DistanceLod) && !(flags & kFlag_NoLod)) {
        const float distance = lodDistance >= 0.0f ? lodDistance : anim.defaultLodDistance;
        if (distance > 0.0f && distance - DistanceToCamera(object) < 0.0f) {
            const uint32_t step = anim.lodPhases > 1 ? 1u : anim.lodPhases;
            if (step != 0) {
                const int left = int(phase) - int(step);
                if (left < 0) {
                    phase = uint8_t(left + anim.lodPhases);
                } else {
                    phase = uint8_t(left);
                    skip = true;
                }
            }
        }
    }
    flags &= ~(kFlag_PoseDirty | kFlag_NearHidden);

    // Hidden objects keep animating within their cull distance.
    if (!skip && cullDistance > 0.0f && !IsDrawnGeometry(object)) {
        if (cullDistance - DistanceToCamera(object) > 0.0f)
            flags |= kFlag_NearHidden;
    }

    if (layers) {
        const bool layerFlag = (flags & kFlag_LayerFlag) != 0;
        for (AnimLayer* layer : *layers)
            layer->Update(dt, layerFlag, skip);
    }
}

// TODO(0x0034D1A8 AnimController::BuildPose): blends the sampled layers'
// poses (weights, bone overrides, IK) into the object's skeleton.
void AnimController::BuildPose(float dt)
{
    (void)dt;
}

// @func 0x0034E2BC BuildPendingPoses
void BuildPendingPoses(float dt)
{
    if (!Anim().enabled)
        return;
    for (AnimController* controller : AnimControllers()) {
        if (controller->flags & AnimController::kFlag_PoseDirty)
            controller->BuildPose(dt);
    }
}

// @func 0x0034E364 BuildPendingPoses (list)
void BuildPendingPoses(float dt, const ape::ApeTArray<AnimController*>& list)
{
    if (!Anim().enabled)
        return;
    for (AnimController* controller : list) {
        if (controller->flags & AnimController::kFlag_PoseDirty)
            controller->BuildPose(dt);
    }
}

// @func 0x0034F518 AnimController::AddListener
void AnimController::AddListener(void* callback, void* context)
{
    // NOTE: the console's fixed pool writes one entry before its storage
    // when all 16 are taken; the port drops the listener instead.
    if (listenerCount >= kMaxListeners)
        return;
    listeners[listenerCount].callback = callback;
    listeners[listenerCount].context = context;
    ++listenerCount;
}

// @func 0x0034F580 AnimController::RemoveListener
void AnimController::RemoveListener(void* callback, void* context)
{
    for (uint32_t i = 0; i < listenerCount; ++i) {
        if (listeners[i].callback == callback && listeners[i].context == context) {
            // The last entry takes the freed slot.
            listeners[i] = listeners[listenerCount - 1];
            --listenerCount;
            return;
        }
    }
}

// @func 0x0034F600 FindDescendant
ape::world::ApeObj* FindDescendant(ape::world::ApeObj* root, uint32_t name)
{
    if (!root)
        return nullptr;
    for (ape::world::ApeObj* child = root->GetFirstChild(); child; child = child->GetNextSibling()) {
        if (child->GetNameHash() == name)
            return child;
        if (ape::world::ApeObj* found = FindDescendant(child, name))
            return found;
    }
    return nullptr;
}

} // namespace anim
} // namespace sly
