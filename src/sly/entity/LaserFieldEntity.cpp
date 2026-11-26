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
