// openTIT - Projectiles (physical entities, 0x000F0000-0x000F6000).
//
// Three projectile classes share a common (abstract, inlined) projectile base
// on top of the physical entity: the plain projectile (vtable 0x00B130E8),
// a second kind (0x00B13308) and EntityProjectileMine (0x00B13528, named in
// its spline error message). Each has its own definition class.
#pragma once

#include "sly/entity/PhysicalEntity.h"
#include "sly/game/Tween.h"

namespace sly {
namespace entity {

// Definition of Projectile (vtable 0x00B130B8, 0x1C0 bytes).
class ProjectileDef : public PhysicalEntityDef {
public:
    ~ProjectileDef() override;
};

// Definition of ProjectileB (vtable 0x00B132D8, 0x1B0 bytes).
class ProjectileBDef : public PhysicalEntityDef {
public:
    ~ProjectileBDef() override;
};

// Definition of EntityProjectileMine (vtable 0x00B134F8, 0x1C0 bytes).
class ProjectileMineDef : public PhysicalEntityDef {
public:
    ~ProjectileMineDef() override;
    bool armOnApproach = false;   // 0x1B4 NOTE: name from use (arms the mine in state 1)
};

// Plain projectile (0x2F0 bytes; constructor 0x000F2488).
class Projectile : public PhysicalEntity {
public:
    ~Projectile() override;
};

// Second projectile kind (0x2B0 bytes). NOTE: its role is not known yet.
class ProjectileB : public PhysicalEntity {
public:
    ~ProjectileB() override;
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;
    // @func 0x000F43FC ProjectileB::SetField280
    void SetField280(const ape::Vec3& value) { field_280 = { value.x, value.y, value.z, field_280.pad }; }

    ape::Vec3 field_280{};    // 0x280
};

// Mine that drifts along a spline path (0x2F0 bytes; constructor 0x000F4A08).
class EntityProjectileMine : public PhysicalEntity {
public:
    ~EntityProjectileMine() override;
    bool Release(bool kill) override;
    // Mine states (0x000F498C); state 1 arms it when the definition says so.
    void SetState(int newState);
    // Heads for `point` and switches to state 1 (0x000F5E28).
    void SetTargetPoint(const ape::Vec3& point);
    const ProjectileMineDef* Def() const { return static_cast<const ProjectileMineDef*>(def); }

    bool armed = false;       // 0x270
    int32_t state = 0;        // 0x274
    ape::Vec3 targetPoint{};  // 0x2E0

    FloatTween tween_284;     // 0x284
    FloatTween tween_2A8;     // 0x2A8
};

} // namespace entity
} // namespace sly
