// openTIT - Projectiles (see Projectiles.h).
#include "sly/entity/Projectiles.h"
#include "sly/entity/EntityHits.h"

#include "ape/sys/ApeGeometry.h"

#include <cmath>

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

// @func 0x000F5E68 EntityProjectileMine::OnHitCategory2
void EntityProjectileMine::OnHitCategory2(EntityHitRecord& hit)
{
    constexpr uint32_t kType_Blast = 0x03C599D8u;
    const float damage = hit.data.blast.damage;
    if (damage <= 0.0f)
        return;
    ScriptedEntity::Damage(damage, hit.source, kType_Blast);
    if (health <= 0.0f)
        SetPhysState(4, 0);
}

// @func 0x000F2FB0 Projectile::OnLaunched
void Projectile::OnLaunched()
{
    const ape::Vec3& aim = carriedTransform.translation;
    const float dx = launchPoint.x - aim.x;
    const float dy = launchPoint.y - aim.y;
    const float dz = launchPoint.z - aim.z;
    const float factor = Def()->arcFactor;
    const float speedSq = launchVelocity.x * launchVelocity.x + launchVelocity.y * launchVelocity.y +
                          launchVelocity.z * launchVelocity.z;
    arc = (factor * 2.0f * speedSq) / std::sqrt(dz * dz + dx * dx + dy * dy);
    launchVelocity.y += factor * std::sqrt(speedSq);
    PhysicalEntity::OnLaunched();
}

// @func 0x000F4094 ProjectileB::AimAt
void ProjectileB::AimAt(Entity* target)
{
    if (field_290 < field_2A0)
        PhysicalEntity::AimAt(target);
}

// @func 0x000F4418 ProjectileB::OnLaunched
void ProjectileB::OnLaunched()
{
    PhysicalEntity::OnLaunched();
}

// @func 0x000F4448 ProjectileB::FlightPoint
ape::Vec3 ProjectileB::FlightPoint(float t) const
{
    const ape::Vec3& start = launchPoint;
    const ape::Vec3& end = carriedTransform.translation;
    if (start.x == end.x && start.y == end.y && start.z == end.z)
        return {start.x, start.y, start.z, 0.0f};
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;
    const float lengthSq = dz * dz + dx * dx + dy * dy;
    const float inv = 1.0f / std::sqrt(lengthSq);
    const float reach = (lengthSq > 0.0f ? std::sqrt(lengthSq) : 1.0f) * forwardScale;
    const ape::Vec3 control = {field_280.x * sideScale + dx * inv * reach + start.x,
                               field_280.y * sideScale + dy * inv * reach + start.y,
                               field_280.z * sideScale + dz * inv * reach + start.z, 0.0f};
    const float u = 1.0f - t;
    return {end.x * t * t + start.x * u * u + control.x * 2.0f * u * t,
            end.y * t * t + start.y * u * u + control.y * 2.0f * u * t,
            end.z * t * t + start.z * u * u + control.z * 2.0f * u * t, 0.0f};
}

// @func 0x000F45E0 ProjectileB::UpdateFromBody
void ProjectileB::UpdateFromBody(float dt)
{
    const float duration = field_2A0;
    float time = field_290 <= duration ? field_290 : duration;
    if (time < 0.0f)
        time = 0.0f;
    const ape::Vec3 previous = FlightPoint((time - dt) / duration);
    const ape::Vec3 current = FlightPoint(time / duration);
    ape::Vec3 next;
    if (field_290 < field_2A0) {
        next = FlightPoint(field_290 / field_2A0);
    } else {
        const ape::Vec3& at = Position();
        next = {at.x + (current.x - previous.x), at.y + (current.y - previous.y), at.z + (current.z - previous.z),
                0.0f};
    }
    ape::Matrix34 frame;
    SweepTo(transform, next, frame, false);
    if (Def()->defFlags & PhysicalEntityDef::kDefFlag_FaceMotion) {
        // Face along the curve, upright.
        ape::Vec3 forward = {current.x - previous.x, current.y - previous.y, current.z - previous.z, 0.0f};
        if (!(forward.x == 0.0f && forward.y == 0.0f && forward.z == 0.0f)) {
            const float inv = 1.0f / std::sqrt(forward.y * forward.y + forward.x * forward.x + forward.z * forward.z);
            forward = {forward.x * inv, forward.y * inv, forward.z * inv, 0.0f};
        }
        const ape::Vec3 up = {0.0f, 1.0f, 0.0f, 0.0f};
        const ape::Vec3 side = {up.y * forward.z - up.z * forward.y, up.z * forward.x - up.x * forward.z,
                                up.x * forward.y - up.y * forward.x, 0.0f};
        const float inv = 1.0f / std::sqrt(side.y * side.y + side.x * side.x + side.z * side.z);
        frame.xAxis = {side.x * inv, side.y * inv, side.z * inv, frame.xAxis.pad};
        frame.yAxis = {up.x, up.y, up.z, frame.yAxis.pad};
        frame.zAxis = {forward.x, forward.y, forward.z, frame.zAxis.pad};
        ape::Matrix34NormalizeAxes(frame);
        // NOTE: the console places the frame at the wanted point, not where
        // the sweep ended.
        frame.translation = {next.x, next.y, next.z, frame.translation.pad};
    }
    SetTransform(frame);
}

} // namespace entity
} // namespace sly
