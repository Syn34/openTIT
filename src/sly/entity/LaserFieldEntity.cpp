// openTIT - The laser field entity's small overrides (see LaserFieldEntity.h).
#include "sly/entity/LaserFieldEntity.h"
#include "sly/entity/PhysicsHooks.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

namespace {
// Field states (layer 0). NOTE: names guessed from the transitions.
constexpr int kField_Idle = 0;
constexpr int kField_Armed = 1;
constexpr int kField_Tripped = 2;
} // namespace

// @func 0x000C0F08 LaserFieldDef::~LaserFieldDef
// @func 0x000C0F68 LaserFieldDef::~LaserFieldDef (deleting)
LaserFieldDef::~LaserFieldDef()
{
    if (collisionShape)
        HavokRelease(collisionShape);
}

// @func 0x00744358 LaserFieldEntity::~LaserFieldEntity
// @func 0x00744394 LaserFieldEntity::~LaserFieldEntity (deleting)
// @func 0x00744468 LaserFieldEntity::~LaserFieldEntity (thunk, +0x17C)
// @func 0x007443FC LaserFieldEntity::~LaserFieldEntity (deleting thunk, +0x17C)
LaserFieldEntity::~LaserFieldEntity() = default;

// @func 0x000C21EC LaserFieldEntity::Interface17CSlot5
// @func 0x000C23D4 LaserFieldEntity::Interface17CSlot5 (thunk, +0x17C)
void LaserFieldEntity::Interface17CSlot5(const void* event)
{
    ContactEventDisable(event);
    if (!body)
        return;
    void* bodyA = ContactEventBody(event, 0);
    void* bodyB = ContactEventBody(event, 1);
    void* other = bodyA == body ? bodyB : (bodyB == body ? bodyA : nullptr);
    Entity* toucher = nullptr;
    if (other && RigidBodyOwnerType(other) == kBodyOwner_Entity)
        toucher = static_cast<Entity*>(RigidBodyOwner(other));
    if (!toucher || !static_cast<const LaserFieldDef*>(def)->playersTrip || !(toucher->classBits & 1))
        return;
    const int state = EntityGetState(this, 0);
    if (state == 0 || state == 1)
        EntitySetState(this, 0, 2);
}

// @func 0x007444AC LaserFieldEntity::GetRenderObject
ape::world::ApeObj* LaserFieldEntity::GetRenderObject() { return static_cast<ape::world::ApeObj*>(body); }

// The four hit handlers share this body on the console (one copy each).
void LaserFieldEntity::Trip(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    const int state = EntityGetState(this, 0);
    if (state == kField_Idle || state == kField_Armed)
        EntitySetState(this, 0, kField_Tripped);
}

// @func 0x007444B4 LaserFieldEntity::OnHitCategory0
void LaserFieldEntity::OnHitCategory0(EntityHitRecord& hit) { (void)hit; }
// @func 0x0074455C LaserFieldEntity::OnHitCategory1
void LaserFieldEntity::OnHitCategory1(EntityHitRecord& hit) { Trip(hit); }
// @func 0x00744600 LaserFieldEntity::OnHitCategory2
void LaserFieldEntity::OnHitCategory2(EntityHitRecord& hit) { Trip(hit); }
// @func 0x007446A4 LaserFieldEntity::OnHitCategory3
void LaserFieldEntity::OnHitCategory3(EntityHitRecord& hit) { Trip(hit); }
// @func 0x007444B8 LaserFieldEntity::OnHitCategory4
void LaserFieldEntity::OnHitCategory4(EntityHitRecord& hit) { Trip(hit); }

} // namespace entity
} // namespace sly
