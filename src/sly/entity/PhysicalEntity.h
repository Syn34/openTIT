// openTIT - Stand-ins for the physical entity base and its definition base.
//
// Projectiles and other free-moving objects derive from a physical entity
// (console vtable 0x00B12CD8 + 0x00B12E78/0x00B12EA0, destructor 0x000EB0F8)
// that owns a Havok rigid body; its definition (vtable 0x00B12C88,
// destructor 0x000E9820) owns the collision shape. Both live in the entity
// module and are not ported yet: only what the derived classes in
// 0x000F0000-0x000F8000 need is modelled, and the destructors below are
// untagged minimal versions.
#pragma once

#include "sly/entity/Entity.h"
#include "sly/entity/EntityInterfaces.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace entity {

class PhysicalEntityDef : public EntityDef {
public:
    ~PhysicalEntityDef() override;
    void* collisionShape = nullptr;          // 0x190
};

class PhysicalEntity : public Entity, public EntityInterface17C, public EntityInterface180 {
public:
    ~PhysicalEntity() override;
    // Minimal versions of 0x000EC5B8 / 0x000ED35C (entity module).
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;

    void* rigidBody = nullptr;               // 0x224
    audio::SoundName* impactSound = nullptr; // 0x230 NOTE: name from use
};

} // namespace entity
} // namespace sly
