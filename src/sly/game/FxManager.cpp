// openTIT - Effect manager walkers and camera shake queue. See FxManager.h.
#include "sly/game/FxManager.h"

#include "sly/game/FxEffects.h"

#include "agl/AglRenderThread.h"

#include "platform/Platform.h"
#include "sly/entity/Entity.h"

namespace sly {
namespace game {

bool g_fxPaused = false;
static FxManager* s_fxManager = nullptr;   // 0x00F64C38 (created by the game world, not ported)

FxManager* GetFxManager() { return s_fxManager; }

namespace {

// Draw callback of the "Effects1" bucket: nothing in this build.
// @func 0x002EF4DC FxDrawEffectsCallback
void FxDrawEffectsCallback() {}

// TODO(0x0051A79C): inserts a callback (with three arguments and a
// priority) into the named draw bucket of the renderer.
void AddDrawBucketCallback(const char* bucket, void (*callback)(), int priority)
{
    (void)bucket; (void)callback; (void)priority;
}

} // namespace

std::vector<FxOcclusionRecord*>& FxOcclusionRecords()
{
    static std::vector<FxOcclusionRecord*> records;   // 0x01068818
    return records;
}

// @func 0x002EE200 ReleaseFxOcclusionRecords
void ReleaseFxOcclusionRecords()
{
    for (FxOcclusionRecord*& record : FxOcclusionRecords()) {
        agl::QueueEndOfFrameRelease(record->query);
        delete record;
        record = nullptr;
    }
    FxOcclusionRecords().clear();
}

// @func 0x002EE304 FxManager::FxManager
FxManager::FxManager() : effects(0x80)
{
    shakes.Init(kMaxShakes, nullptr, false);
    s_fxManager = this;
    effects.SetGrowable(false);
    AddDrawBucketCallback("Effects1", &FxDrawEffectsCallback, 0);
}

// @func 0x002EE3C8 FxManager::~FxManager
FxManager::~FxManager()
{
    // NOTE: the console also releases the global array 0x0106882C (contents
    // unknown).
    ReleaseFxOcclusionRecords();
    s_fxManager = nullptr;
}

// @func 0x002EE458 FxManager::ClearShakes
void FxManager::ClearShakes()
{
    shakes.Clear();
}

// @func 0x002EF36C FxManager::UpdateEffects
void FxManager::UpdateEffects()
{
    if (g_fxPaused)
        return;
    for (int32_t i = effects.FirstIndex(); i != effects.kNone; i = effects.NextIndex(i))
        effects.At(i)->Update();
}

// @func 0x002EF4E0 FxManager::KillEffectsOn
void FxManager::KillEffectsOn(ape::world::ApeObj* object)
{
    for (int32_t i = effects.FirstIndex(); i != effects.kNone; i = effects.NextIndex(i))
        effects.At(i)->KillIfAttachedTo(object);
}

// @func 0x002EF648 FxManager::KillEntityEffects
void FxManager::KillEntityEffects(entity::Entity* entity)
{
    if (!entity)
        return;
    ape::world::ApeObj* object = entity->object;
    for (int32_t i = effects.FirstIndex(); i != effects.kNone; i = effects.NextIndex(i))
        effects.At(i)->KillIfAttachedTo(object);
}

// @func 0x002EF7B8 FxManager::SetTimeScaleOn
void FxManager::SetTimeScaleOn(ape::world::ApeObj* object, float timeScale)
{
    for (int32_t i = effects.FirstIndex(); i != effects.kNone; i = effects.NextIndex(i))
        effects.At(i)->SetValueIfAttachedTo(object, timeScale);
}

// @func 0x002EF13C FxManager::DeleteAll
void FxManager::DeleteAll()
{
    for (int32_t i = effects.FirstIndex(); i != effects.kNone; i = effects.NextIndex(i)) {
        delete effects.At(i);
        effects.At(i) = nullptr;
    }
    effects.Clear();
    shakes.Clear();
}

// @func 0x002F0E0C FxManager::Play
FxEffect* FxManager::Play(const FxEffectDesc& desc, uint32_t* handle)
{
    if (effects.Count() >= effects.Capacity())
        return nullptr;
    FxEffect* effect = nullptr;
    switch (desc.kind) {
    case kFx_ObjShake:
        effect = new FxObjShakeEffect(static_cast<const FxObjShakeDesc&>(desc));
        break;
    case kFx_Trail:
        effect = new FxTrailEffect(desc);
        break;
    case kFx_GeoDisplay:
        effect = new FxGeoDisplayEffect(desc);
        break;
    case kFx_MotionTrail:
        effect = new FxMotionTrailEffect(static_cast<const FxMotionTrailDesc&>(desc));
        break;
    case kFx_Delay:
        effect = new FxDelayEffect(static_cast<const FxDelayDesc&>(desc));
        break;
    default:
        return nullptr;
    }
    // The console takes the list node first and stores the new effect in it.
    *handle = effects.PushBack(effect);
    return effect;
}

// @func 0x002F20A4 FxManager::StopEffect
void FxManager::StopEffect(uint32_t handle)
{
    if (FxEffect** effect = effects.Get(handle))
        (*effect)->Stop();
}

// @func 0x002F2140 FxManager::AddCameraShake
void FxManager::AddCameraShake(const CameraShake& shake)
{
    if (shakes.Count() >= kMaxShakes) {
        plat::Log(plat::LogLevel::Warning,
                  "Too many active camera shakes.  Ask a programmer to increase the limit (%u)\n",
                  (unsigned)kMaxShakes);
        return;
    }
    shakes.PushBack(shake);
}

} // namespace game
} // namespace sly
