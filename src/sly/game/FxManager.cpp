// openTIT - Effect manager walkers and camera shake queue. See FxManager.h.
#include "sly/game/FxManager.h"

#include "sly/game/FxEffects.h"

#include "platform/Platform.h"
#include "sly/entity/Entity.h"

namespace sly {
namespace game {

bool g_fxPaused = false;
static FxManager* s_fxManager = nullptr;   // 0x00F64C38 (created by the game world, not ported)

FxManager* GetFxManager() { return s_fxManager; }

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
        effect = new FxDelayEffect(desc);
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
