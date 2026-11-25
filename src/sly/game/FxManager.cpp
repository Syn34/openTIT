// openTIT - Effect manager walkers and camera shake queue. See FxManager.h.
#include "sly/game/FxManager.h"

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
