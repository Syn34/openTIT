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

// A running effect (base vtable 0x00B1D4D8). The effect classes themselves
// live in the effects module (vtables 0x00B1D500..) and are not ported.
class FxEffect {
public:
    // @func 0x00767A38 FxEffect::~FxEffect
    // @func 0x00767A48 FxEffect::~FxEffect (deleting)
    virtual ~FxEffect() = default;
    // Slot 2 (+0x08). NOTE: empty in the base; meaning unknown.
    // @func 0x00767A8C FxEffect::Slot2
    virtual void Slot2() {}
    // @func 0x00767A90 FxEffect::Update
    virtual void Update() {}                                // slot 3 (+0x0C)
    // Slot 4 (+0x10): the effect has ended (the manager drops it).
    // @func 0x00767A94 FxEffect::IsFinished
    virtual bool IsFinished() const { return finished; }
    // Slot 5 (+0x14): ends the effect.
    // @func 0x00767A9C FxEffect::Stop
    virtual void Stop() { finished = true; }
    // Ends the effect when it is attached to `object`.
    // @func 0x00767AA8 FxEffect::KillIfAttachedTo
    virtual void KillIfAttachedTo(ape::world::ApeObj* object) { (void)object; }   // slot 6 (+0x18)
    // Slot 7 (+0x1C): attached effects take `value` (+0x04) when attached to
    // `object`. NOTE: meaning of the value unknown.
    // @func 0x00767AAC FxEffect::SetValueIfAttachedTo
    virtual void SetValueIfAttachedTo(ape::world::ApeObj* object, float value) { (void)object; (void)value; }

    float value = 0.0f;       // +0x04
    bool finished = false;    // +0x08
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
