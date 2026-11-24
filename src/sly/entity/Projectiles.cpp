// openTIT - Projectiles (see Projectiles.h).
#include "sly/entity/Projectiles.h"

namespace sly {
namespace entity {

// @func 0x000F23A0 ProjectileDef::~ProjectileDef
// @func 0x000F23CC ProjectileDef::~ProjectileDef (deleting)
ProjectileDef::~ProjectileDef() = default;

// @func 0x000F3934 ProjectileBDef::~ProjectileBDef
// @func 0x000F3960 ProjectileBDef::~ProjectileBDef (deleting)
ProjectileBDef::~ProjectileBDef() = default;

// @func 0x000F4900 ProjectileMineDef::~ProjectileMineDef
// @func 0x000F492C ProjectileMineDef::~ProjectileMineDef (deleting)
ProjectileMineDef::~ProjectileMineDef() = default;

// @func 0x000F2B64 Projectile::~Projectile
// @func 0x000F2BA4 Projectile::~Projectile (deleting)
// @func 0x000F2C10 Projectile::~Projectile (deleting thunk, +0x180)
// @func 0x000F2C80 Projectile::~Projectile (deleting thunk, +0x17C)
// @func 0x000F2CF0 Projectile::~Projectile (thunk, +0x180)
// @func 0x000F2D38 Projectile::~Projectile (thunk, +0x17C)
Projectile::~Projectile() = default;

// @func 0x000F3E54 ProjectileB::~ProjectileB
// @func 0x000F3E94 ProjectileB::~ProjectileB (deleting)
// @func 0x000F3F00 ProjectileB::~ProjectileB (deleting thunk, +0x180)
// @func 0x000F3F70 ProjectileB::~ProjectileB (deleting thunk, +0x17C)
// @func 0x000F3FE0 ProjectileB::~ProjectileB (thunk, +0x180)
// @func 0x000F4028 ProjectileB::~ProjectileB (thunk, +0x17C)
ProjectileB::~ProjectileB() = default;

// @func 0x000F43DC ProjectileB::PostUpdate
bool ProjectileB::PostUpdate(float dt)
{
    return PhysicalEntity::PostUpdate(dt);
}

// @func 0x000F4070 ProjectileB::Release
bool ProjectileB::Release(bool kill)
{
    return PhysicalEntity::Release(kill);
}

// @func 0x000F4ED0 EntityProjectileMine::~EntityProjectileMine
// @func 0x000F4F40 EntityProjectileMine::~EntityProjectileMine (deleting)
// @func 0x000F4FCC EntityProjectileMine::~EntityProjectileMine (deleting thunk, +0x180)
// @func 0x000F505C EntityProjectileMine::~EntityProjectileMine (deleting thunk, +0x17C)
// @func 0x000F50EC EntityProjectileMine::~EntityProjectileMine (thunk, +0x180)
// @func 0x000F5160 EntityProjectileMine::~EntityProjectileMine (thunk, +0x17C)
EntityProjectileMine::~EntityProjectileMine() = default;

// @func 0x000F51D4 EntityProjectileMine::Release
bool EntityProjectileMine::Release(bool kill)
{
    return PhysicalEntity::Release(kill);
}

// @func 0x000F498C EntityProjectileMine::SetState
void EntityProjectileMine::SetState(int newState)
{
    armed = newState == 1 && Def()->armOnApproach;
    state = newState;
}

// @func 0x000F5E28 EntityProjectileMine::SetTargetPoint
void EntityProjectileMine::SetTargetPoint(const ape::Vec3& point)
{
    targetPoint = { point.x, point.y, point.z, targetPoint.pad };
    SetState(1);
}

} // namespace entity
} // namespace sly
