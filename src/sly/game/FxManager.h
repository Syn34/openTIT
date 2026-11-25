// openTIT - Effect manager (console pointer 0x00F64C38): running effects
// (object shakes, trails, delayed effects) in a handle list and the active
// camera shakes. Only the list walkers and the shake queue are ported
// (0x002EF36C-0x002F2294); the effect classes are not.
#pragma once

#include "ape/container/ApeTHandleList.h"
#include "ape/memory/ApeTArray.h"
#include "sly/gameplay/GameplayWorld.h"

#include <cstdint>

namespace ape { namespace world { class ApeObj; } }
namespace sly { namespace entity { class Entity; } }

namespace sly {
namespace game {

// A running effect. NOTE: only the virtual slots the manager calls are
// declared; the classes live in the effects module (vtables 0x00B1D4D8..).
class FxEffect {
public:
    virtual ~FxEffect() = default;
    virtual void Update() {}                                // slot 3 (+0x0C)
    virtual void Stop() {}                                  // slot 5 (+0x14)
    // Ends the effect when it is attached to `object`.
    virtual void KillIfAttachedTo(ape::world::ApeObj* object) { (void)object; }   // slot 6 (+0x18)
};

// One queued camera shake (0x60 bytes).
using CameraShake = gameplay::CameraShakeParams;

class FxManager {
public:
    static constexpr int kMaxShakes = 0x10;

    // Updates every effect unless effects are paused (0x002EF36C).
    void UpdateEffects();
    // Ends the effects attached to `object` (0x002EF4E0) / to an entity's
    // world object (0x002EF648).
    void KillEffectsOn(ape::world::ApeObj* object);
    void KillEntityEffects(entity::Entity* entity);
    // Stops the effect of a handle if it is still alive (0x002F20A4).
    void StopEffect(uint32_t handle);
    // Queues a camera shake (at most 16) (0x002F2140).
    void AddCameraShake(const CameraShake& shake);

    ape::ApeTHandleList<FxEffect*> effects;     // +0x00
    ape::ApeTArray<CameraShake> shakes;         // +0x24
};

FxManager* GetFxManager();
// 0x01068890: effects do not update (debug pause).
extern bool g_fxPaused;

} // namespace game
} // namespace sly
