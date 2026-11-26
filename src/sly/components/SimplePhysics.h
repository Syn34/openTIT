// openTIT - <SimplePhysics> component (type 0x1F, vtable 0x00B1CF80, 0x60
// bytes): a capsule character proxy (radius, height, mass from the
// description) that keeps the owner out of the world geometry.
//
// The constructor (0x00256FD0) builds the Havok capsule shape, phantom and
// proxy (vector code and Havok calls); it is not ported, so the factory does
// not create the component yet.
#pragma once

#include "sly/components/ComponentDescs.h"
#include "sly/entity/Entity.h"

namespace sly {
namespace components {

class SimplePhysicsComponent : public entity::EntityComponent {
public:
    ~SimplePhysicsComponent() override;
    // Slot 4: puts the proxy's phantom in the physics world or takes it out
    // to match the enabled bit.
    void ResetState() override;
    // Slots 7 and 8: nothing.
    void PostUpdate(float dt) override;
    void RenderInstance() override;
    // 0x002BC1E0: the capsule's centre relative to the owner (half its
    // height up).
    ape::Vec3 CenterOffset() const;

    enum Flags : uint32_t {
        kFlag_InWorld = 0x80000000u,      // the phantom is in the physics world
        kFlag_Enabled = 0x40000000u,      // "startEnabled"
    };

    void* physicsWorld = nullptr;         // 0x18 NOTE: object whose +0x04 is the Havok world
    void* proxy = nullptr;                // 0x1C hkpCharacterProxy
    uint32_t flags = 0;                   // 0x20 Flags
    float radius = 0.5f;                  // 0x50
    float height = 1.0f;                  // 0x54
    float mass = 0.0f;                    // 0x58
};

// Hooks into the physics module: the proxy's shape phantom (0x009CEEAC),
// whether a physics world is set (+0x04 of the world object), and the
// phantom's world add/remove (0x00931D10 / 0x00931F00) and reference drop
// (0x007D4EA8).
void* ProxyShapePhantom(void* proxy);
bool PhysicsWorldPresent(void* physicsWorld);
void PhysicsWorldAddPhantom(void* physicsWorld, void* phantom);
void PhysicsWorldRemovePhantom(void* physicsWorld, void* phantom);
void HavokRelease(void* object);

} // namespace components
} // namespace sly
