// openTIT - <SimplePhysics> component (0x002BBC18-0x002BC1E0).
#include "sly/components/SimplePhysics.h"

namespace sly {
namespace components {

// @func 0x002BBC18 SimplePhysicsComponent::~SimplePhysicsComponent
// @func 0x002BBD90 SimplePhysicsComponent::~SimplePhysicsComponent (deleting)
SimplePhysicsComponent::~SimplePhysicsComponent()
{
    if (!proxy)
        return;
    void* phantom = ProxyShapePhantom(proxy);
    if (PhysicsWorldPresent(physicsWorld)) {
        PhysicsWorldRemovePhantom(physicsWorld, phantom);
        HavokRelease(phantom);
    }
    HavokRelease(proxy);
    proxy = nullptr;
}

// @func 0x002BC128 SimplePhysicsComponent::ResetState
void SimplePhysicsComponent::ResetState()
{
    const bool enabled = (flags & kFlag_Enabled) != 0;
    if (((flags & kFlag_InWorld) != 0) == enabled)
        return;
    flags = enabled ? (flags | kFlag_InWorld) : (flags & ~kFlag_InWorld);
    if (!proxy)
        return;
    void* phantom = ProxyShapePhantom(proxy);
    if (!PhysicsWorldPresent(physicsWorld))
        return;
    if (enabled)
        PhysicsWorldAddPhantom(physicsWorld, phantom);
    else
        PhysicsWorldRemovePhantom(physicsWorld, phantom);
}

// @func 0x002BC124 SimplePhysicsComponent::PostUpdate
void SimplePhysicsComponent::PostUpdate(float) {}

// @func 0x002BC1D4 SimplePhysicsComponent::RenderInstance
void SimplePhysicsComponent::RenderInstance() {}

// @func 0x002BC1E0 SimplePhysicsComponent::CenterOffset
ape::Vec3 SimplePhysicsComponent::CenterOffset() const
{
    return {0.0f, height * 0.5f, 0.0f, 0.0f};
}

// Physics hooks (the Havok layer is not ported).
void* ProxyShapePhantom(void* proxy) { (void)proxy; return nullptr; }
bool PhysicsWorldPresent(void* physicsWorld) { (void)physicsWorld; return false; }
void PhysicsWorldAddPhantom(void* physicsWorld, void* phantom) { (void)physicsWorld; (void)phantom; }
void PhysicsWorldRemovePhantom(void* physicsWorld, void* phantom) { (void)physicsWorld; (void)phantom; }
void HavokRelease(void* object) { (void)object; }

} // namespace components
} // namespace sly
