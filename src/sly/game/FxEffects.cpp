// openTIT - FX manager effect kinds (see FxEffects.h).
#include "sly/game/FxEffects.h"

#include "agl/AglPostProc.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/tuning/FxTuning.h"

namespace sly {
namespace game {

namespace {

constexpr float kDegToRad = 0.017453292f;

// Game world callback list the object shakes tick on.
constexpr int kCallbackList_Shake = 3;

// Shader class whose geometry never goes into the motion trail. NOTE: the
// class name is unknown.
constexpr uint32_t kShaderClass_NoMotionTrail = 0x64D38214u;

// Pending state of the motion trail post-process.
bool g_motionTrailDirty = false;     // 0x01068841
bool g_motionTrailEnabled = false;   // 0x01068840

agl::AglMotionTrailEffect* MainViewMotionTrail()
{
    if (!g_game || !g_game->mainView || !g_game->mainView->postProc)
        return nullptr;
    return static_cast<agl::AglMotionTrailEffect*>(
        g_game->mainView->postProc->effects[agl::AglPostProcChain::kMotionTrail]);
}

// The geometry uses a material of the excluded shader class.
bool HasNoMotionTrailMaterial(ape::world::ApeObj* geom)
{
    auto* def = static_cast<ape::world::ApeGeomDef*>(geom->GetDef());
    if (!def)
        return false;
    for (int lod = 0; lod < def->m_lodCount; ++lod) {
        auto* topology = reinterpret_cast<agl::AglTopology*>(def->m_lods[lod].mesh);
        if (!topology)
            continue;
        for (uint16_t i = 0; i < topology->meshCount; ++i) {
            agl::AglMaterial* material = topology->meshes[i].material;
            if (material->shaderClass && material->shaderClass->GetNameHash() == kShaderClass_NoMotionTrail)
                return true;
        }
    }
    return false;
}

// Callback of the object shakes (list 3, the context is the effect).
// NOTE: the console passes the frame time in f1; the port's callbacks do
// not receive it, so the game's frame time is used.
// @func 0x002F5BE4 FxObjShakeEffect::TickCallback
void ShakeTickCallback(void* unused, void* context)
{
    (void)unused;
    const float dt = g_game ? g_game->FrameTime() : 0.0f;
    static_cast<FxObjShakeEffect*>(context)->Tick(dt);
}

} // namespace

// ===========================================================================
// Object shake

// @func 0x002EF938 FxObjShakeEffect::FxObjShakeEffect
FxObjShakeEffect::FxObjShakeEffect(const FxObjShakeDesc& desc)
{
    value = desc.timeScale;
    attachedTo = desc.object;
    duration = desc.duration;
    magnitude = desc.magnitude;
    startMagnitude = desc.startMagnitude;
    falloffMode = desc.falloffMode;
    falloffStartTime = desc.falloffStartTime;
    for (int i = 0; i < 3; ++i) {
        angles[i] = desc.angles[i] * kDegToRad;
        frequencies[i] = desc.frequencies[i];
        offsets[i] = desc.offsets[i];
    }
    for (int i = 0; i < 8; ++i)
        extra[i] = desc.extra[i];
    field_78 = desc.field_68;
    const float cull = desc.cullDistance < 0.0f ? g_fXObjShakeDefaultCullDist.Get() : desc.cullDistance;
    cullDistanceSq = cull * cull;
    for (int i = 0; i < 4; ++i)
        flags[i] = desc.flags[i];

    if (attachedTo && desc.bone != 0) {
        if (ape::world::ApeSkeleton* skeleton = attachedTo->GetSkeleton()) {
            const int index = skeleton->FindBone(desc.bone);
            if (index >= 0) {
                bone = &skeleton->bones[index];
                // NOTE: the console only takes parents above bone 0.
                const int16_t parent = bone->def->parent;
                if (parent > 0)
                    parentBone = &skeleton->bones[parent];
            }
        }
        // No such bone: shake the child object of that name instead.
        if (!bone && attachedTo->GetFirstChild() && attachedTo->GetNameHash() != desc.bone)
            child = FindInHierarchy(attachedTo, desc.bone);
    }
    if (attachedTo)
        attachedTo->AddRef();
}

// @func 0x002F48A4 FxObjShakeEffect::~FxObjShakeEffect
// @func 0x002F490C FxObjShakeEffect::~FxObjShakeEffect (deleting)
FxObjShakeEffect::~FxObjShakeEffect()
{
    if (attachedTo)
        attachedTo->Release();
}

// @func 0x002F4998 FxObjShakeEffect::Slot2
void FxObjShakeEffect::Slot2(float dt)
{
    (void)dt;
    if (g_game)
        g_game->AddCallback(kCallbackList_Shake, reinterpret_cast<void*>(&ShakeTickCallback), this, nullptr);
}

// TODO(0x002F4A38 FxObjShakeEffect::Tick): advances `elapsed` (stopping at
// the duration), skips hidden or distant objects, computes the fall-off
// magnitude and the three axis rotations and poses the bone/child. The
// vector part could not be decoded yet; only the timing is done here.
void FxObjShakeEffect::Tick(float dt)
{
    if (!attachedTo) {
        Stop();
        return;
    }
    elapsed += dt;
    if (duration >= 0.0f && duration <= elapsed) {
        elapsed = duration;
        Stop();
    }
}

// @func 0x00767BC4 FxObjShakeEffect::KillIfAttachedTo
void FxObjShakeEffect::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767C0C FxObjShakeEffect::SetValueIfAttachedTo
void FxObjShakeEffect::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// ===========================================================================
// Trail / geometry display
//
// Their constructors are not ported: the stubs keep the head of the
// description and end at once, so the manager drops them.

// TODO(0x002F0058 FxTrailEffect::FxTrailEffect)
FxTrailEffect::FxTrailEffect(const FxEffectDesc& desc)
{
    value = desc.timeScale;
    finished = true;
}

// TODO(0x002EC920 FxGeoDisplayEffect::FxGeoDisplayEffect)
FxGeoDisplayEffect::FxGeoDisplayEffect(const FxEffectDesc& desc)
{
    value = desc.timeScale;
    finished = true;
}

// @func 0x002ED6F4 FxDelayEffect::FxDelayEffect
FxDelayEffect::FxDelayEffect(const FxDelayDesc& desc)
{
    value = desc.timeScale;
    for (int i = 0; i < kMaxCopies; ++i)
        copySettings[i] = desc.copies[i];
    copyCount = uint8_t(desc.copyCount & 0x1F);
    hideOriginal = desc.hideOriginal;
    ownGeometry = desc.ownGeometry;
    attachedTo = desc.object;
    if (attachedTo) {
        attachedTo->AddRef();
        if (ape::world::ApeSkeleton* skeleton = attachedTo->GetSkeleton())
            boneCount = skeleton->numBones;
        if (hideOriginal)
            attachedTo->SetVisible(false, false);
        source = attachedTo;
        // Another object may stand in for the copies when its skeleton
        // matches (NOTE: a mismatch only fed a compiled-out warning).
        if (desc.source) {
            ape::world::ApeSkeleton* skeleton = desc.source->GetSkeleton();
            if (skeleton && skeleton->numBones == boneCount)
                source = desc.source;
        }
        source->AddRef();
        if (ownGeometry && source->GetType() == ape::world::kObjType_GeomInstance) {
            auto* def = static_cast<ape::world::ApeGeomDef*>(source->GetDef());
            ownDef = def->Clone();
            ownDef->SetName("", true);
            // NOTE: the console clears byte +0x0B of the block each LOD
            // topology points at (+0x5C); its meaning is unknown and the
            // port's topology does not model it.
        }
    }
    historySize = desc.historySize ? desc.historySize : 1;
    transforms.assign(historySize, ape::Matrix34{});
    poses.assign(size_t(historySize) * boneCount, ape::Matrix34{});
    times.assign(historySize, 0.0f);
    CreateCopies();
}

// @func 0x002ED468 FxDelayEffect::CreateCopies
void FxDelayEffect::CreateCopies()
{
    for (ape::world::ApeObj*& copy : copies) {
        if (copy) {
            copy->Release();
            copy = nullptr;
        }
    }
    if (source) {
        for (int i = 0; i < copyCount; ++i) {
            ape::world::ApeObj* copy =
                ownDef ? ape::world::CreateGeomInstance(ownDef, false) : source->Clone(ape::world::kClone_04 | ape::world::kClone_NoProcAnim);
            if (!copy)
                continue;
            copy->SetName(ape::TempFormat("FXDly%02d_%s", i + 1, source->GetName()), true);
            copy->SetVisible(false, false);
            copy->VirtualSlot13(uint8_t(int(copySettings[i].alpha * 255.0f) & 0xFF), false);
            if (copy->GetType() >= ape::world::kObjType_FirstInstance &&
                copy->GetType() < ape::world::kObjType_EndRenderable) {
                auto* render = static_cast<ape::world::ApeRenderObj*>(copy);
                if (render->m_flags142 & 0x80) {
                    render->m_flags142 &= uint8_t(~0x80);
                    render->OnBoundsChanged();
                }
            }
            copies[i] = copy;
        }
    }
    writeIndex = 0;
    oldestIndex = 0;
    elapsed = 0.0f;
}

// @func 0x002EDB10 FxDelayEffect::~FxDelayEffect
// @func 0x002EDCA8 FxDelayEffect::~FxDelayEffect (deleting)
FxDelayEffect::~FxDelayEffect()
{
    for (int i = 0; i < copyCount; ++i) {
        if (copies[i])
            copies[i]->Release();
    }
    if (!attachedTo)
        return;
    if (ownDef)
        ownDef->Release();
    source->Release();
    if (hideOriginal)
        attachedTo->SetVisible(true, false);
    attachedTo->Release();
}

// @func 0x002EDCF0 FxDelayEffect::Slot2
void FxDelayEffect::Slot2(float dt)
{
    elapsed += dt;
    times[writeIndex] = elapsed;
    if (source) {
        transforms[writeIndex] = source->GetLocal();
        if (ape::world::ApeSkeleton* skeleton = source->GetSkeleton()) {
            for (uint16_t b = 0; b < boneCount; ++b)
                poses[size_t(writeIndex) * boneCount + b] = *skeleton->bones[b].local;
        }
    }
    if (++writeIndex >= historySize)
        writeIndex = 0;
    if (writeIndex == oldestIndex)
        ++oldestIndex;
    if (oldestIndex >= historySize)
        oldestIndex = 0;

    for (int i = 0; i < copyCount; ++i) {
        ape::world::ApeObj* copy = copies[i];
        if (!copy)
            continue;
        // The newest record at least `delay` old (or the oldest one).
        const float target = elapsed - copySettings[i].delay;
        int index = int(writeIndex) - 1;
        float time = 0.0f;
        for (;;) {
            if (index < 0)
                index = historySize - 1;
            time = times[index];
            if (time <= target || index == oldestIndex)
                break;
            --index;
        }
        if (time > target)
            continue;
        copy->SetLocal(transforms[index]);
        copy->MarkDirty();
        if (boneCount) {
            if (ape::world::ApeSkeleton* skeleton = copy->GetSkeleton()) {
                for (uint16_t b = 0; b < boneCount; ++b)
                    *skeleton->bones[b].local = poses[size_t(index) * boneCount + b];
                skeleton->flags |= ape::world::kSkelFlag_Dirty;
            }
        }
        copy->MarkDirty();
        copy->SetVisible(true, false);
    }
}

// @func 0x00767AB0 FxDelayEffect::KillIfAttachedTo
void FxDelayEffect::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767AF8 FxDelayEffect::SetValueIfAttachedTo
void FxDelayEffect::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// @func 0x00767B0C FxTrailEffect::KillIfAttachedTo
void FxTrailEffect::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767B54 FxTrailEffect::SetValueIfAttachedTo
void FxTrailEffect::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// ===========================================================================
// Motion trail

// @func 0x002F0CD8 FxMotionTrailEffect::FxMotionTrailEffect
FxMotionTrailEffect::FxMotionTrailEffect(const FxMotionTrailDesc& desc)
{
    value = desc.timeScale;
    attachedTo = desc.object;
    if (attachedTo) {
        attachedTo->AddRef();
        if (agl::AglMotionTrailEffect* trail = MainViewMotionTrail()) {
            float step = desc.timeStep;
            if (trail->field_1C < step)
                step = trail->field_1C;
            if (step < trail->field_18)
                step = trail->field_18;
            trail->timeStep = step;
            for (int i = 0; i < 3; ++i)
                trail->tint[i] = desc.tint[i];
            trail->tint[3] = desc.alpha;
            for (int i = 0; i < 4; ++i)
                trail->field_70[i] = desc.params[i];
            trail->field_80 = desc.params[4];
            trail->field_84 = desc.params[5];
            trail->field_88 = desc.distance * desc.distance;
        }
        g_motionTrailDirty = true;
        g_motionTrailEnabled = true;
    }
    drawHidden = desc.drawHidden;
    children = desc.children;
}

// @func 0x002F457C FxMotionTrailEffect::~FxMotionTrailEffect
// @func 0x002F45E8 FxMotionTrailEffect::~FxMotionTrailEffect (deleting)
FxMotionTrailEffect::~FxMotionTrailEffect()
{
    if (attachedTo)
        attachedTo->Release();
}

// @func 0x002F466C FxMotionTrailEffect::QueueObject
void FxMotionTrailEffect::QueueObject(ape::world::ApeObj* object)
{
    if ((object->IsVisible() || drawHidden) && !HasNoMotionTrailMaterial(object))
        QueueMotionTrailDraw(object);
    if (!children)
        return;
    for (ape::world::ApeObj* child = object->GetFirstChild(); child;) {
        ape::world::ApeObj* next = child->GetNextSibling();
        if (child->GetType() == ape::world::kObjType_GeomInstance)
            QueueObject(child);
        child = next;
    }
}

// @func 0x002F47C4 FxMotionTrailEffect::Update
void FxMotionTrailEffect::Update()
{
    QueueObject(attachedTo);
}

// @func 0x002F47E8 FxMotionTrailEffect::Stop
void FxMotionTrailEffect::Stop()
{
    if (attachedTo) {
        g_motionTrailDirty = true;
        g_motionTrailEnabled = false;
    }
    finished = true;
}

// @func 0x00767B68 FxMotionTrailEffect::KillIfAttachedTo
void FxMotionTrailEffect::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767BB0 FxMotionTrailEffect::SetValueIfAttachedTo
void FxMotionTrailEffect::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// @func 0x002F481C ApplyMotionTrailState
void ApplyMotionTrailState()
{
    if (!g_motionTrailDirty)
        return;
    if (agl::AglMotionTrailEffect* trail = MainViewMotionTrail())
        trail->SetEnabled(g_motionTrailEnabled);
    g_motionTrailDirty = false;
}

// TODO(0x005A1548 world object draw into the motion trail mask)
void QueueMotionTrailDraw(ape::world::ApeObj* object)
{
    (void)object;
}

} // namespace game
} // namespace sly
