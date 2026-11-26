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
    explicit ProjectileDef(const PhysicalEntityDefDesc& desc) : PhysicalEntityDef(desc) {}
    ~ProjectileDef() override;
    float field_194 = 0.0f;       // 0x194 attribute 0xB81F0E21
    float field_198 = 0.0f;       // 0x198 attribute 0xB81F0D33
    float field_19C = -1.0f;      // 0x19C attribute 0x4BC4DED1
    float field_1A0 = 0.0f;       // 0x1A0 attribute 0x4BA728F9
    // 0x1A4 how much of the launch speed is turned upwards into an arc
    // (attribute 0x04F32F5C). NOTE: name from use.
    float arcFactor = 0.0f;
    uint32_t field_1A8 = 0;       // 0x1A8 index of the value of attribute 0xE8DB4898 (two known)
    float field_1AC = 0.0f;       // 0x1AC attribute 0x39F5434C (degrees in the file)
    float field_1B0 = 0.0f;       // 0x1B0 attribute 0xA3D3DF7A (degrees in the file)
    bool field_1B4 = false;       // 0x1B4 attribute 0x99A462A8
};

// Definition of ProjectileB (vtable 0x00B132D8, 0x1B0 bytes).
class ProjectileBDef : public PhysicalEntityDef {
public:
    explicit ProjectileBDef(const PhysicalEntityDefDesc& desc) : PhysicalEntityDef(desc) {}
    ~ProjectileBDef() override;
    float field_194 = 1.0f;       // 0x194 attribute 0x7ECB8527
    float field_198 = 0.5f;       // 0x198 attribute 0xCCF74C48
    float field_19C = 1.0f;       // 0x19C attribute 0x9E8B1865
    float field_1A0 = 0.0f;       // 0x1A0 attribute 0x21684859
    float field_1A4 = 1.0f;       // 0x1A4 NOTE: not read by the parser
};

// Definition of EntityProjectileMine (vtable 0x00B134F8, 0x1C0 bytes).
class ProjectileMineDef : public PhysicalEntityDef {
public:
    explicit ProjectileMineDef(const PhysicalEntityDefDesc& desc) : PhysicalEntityDef(desc) {}
    ~ProjectileMineDef() override;
    uint32_t field_194 = 0;       // 0x194 hash 0xFEBE4776
    float field_198 = 1.0f;       // 0x198 attribute 0xD93B86F6
    bool field_19C = false;       // 0x19C attribute 0xFBCDDB3B
    float field_1A0 = 1.0f;       // 0x1A0 attribute 0xD63894F0
    float field_1A4 = 0.0f;       // 0x1A4 attribute 0x35FE9ECF
    float field_1A8 = 1.0f;       // 0x1A8 NOTE: not read by the parser
    float field_1AC = 0.0f;       // 0x1AC attribute 0xB289BA5E
    float field_1B0 = 0.0f;       // 0x1B0 attribute 0x9EB4518A
    bool armOnApproach = false;   // 0x1B4 attribute 0x0927884E. NOTE: name from use (arms the mine in state 1)
    bool field_1B5 = true;        // 0x1B5 attribute 0x572A6E83
    bool field_1B6 = false;       // 0x1B6 attribute 0x644AB625
};

// Plain projectile (0x2F0 bytes; constructor 0x000F2488).
class Projectile : public PhysicalEntity {
public:
    ~Projectile() override;
    // Slot 95: lobs the projectile (an upward share of the launch speed and
    // the matching arc value) before the base launch.
    void OnLaunched() override;
    const ProjectileDef* Def() const { return static_cast<const ProjectileDef*>(def); }

    ape::Vec3 launchVelocity{};   // 0x2D0
    float arc = 0.0f;         // 0x2E0 NOTE: name from use
};

// Second projectile kind (0x2B0 bytes). NOTE: its role is not known yet.
class ProjectileB : public PhysicalEntity {
public:
    ~ProjectileB() override;
    bool PostUpdate(float dt) override;
    bool Release(bool kill) override;
    // Slot 94: aims only while field_290 is below field_2A0.
    void AimAt(Entity* target) override;
    // Slot 95: the base launch.
    void OnLaunched() override;
    // Slot 101: flies along the curve from the launch point to the aim point
    // (past the end it keeps the last step), facing along it with
    // definition flag 22.
    void UpdateFromBody(float dt) override;
    // Point of the flight curve at `t` (0..1): a quadratic Bezier from the
    // launch point to the aim point whose control point lies sideScale along
    // `side` and forwardScale of the distance towards the aim (0x000F4448).
    ape::Vec3 FlightPoint(float t) const;
    // @func 0x000F43FC ProjectileB::SetField280
    void SetField280(const ape::Vec3& value) { field_280 = { value.x, value.y, value.z, field_280.pad }; }

    ape::Vec3 field_280{};    // 0x280 side direction of the flight curve
    float field_290 = 0.0f;   // 0x290 flight time so far
    float sideScale = 0.0f;   // 0x298
    float forwardScale = 0.0f;   // 0x29C
    float field_2A0 = 0.0f;   // 0x2A0 flight duration
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
    // Blasts hurt the mine; without health left it enters physics state 4.
    void OnHitCategory2(EntityHitRecord& hit) override;
    const ProjectileMineDef* Def() const { return static_cast<const ProjectileMineDef*>(def); }

    bool armed = false;       // 0x270
    int32_t state = 0;        // 0x274
    ape::Vec3 targetPoint{};  // 0x2E0

    FloatTween tween_284;     // 0x284
    FloatTween tween_2A8;     // 0x2A8
};

} // namespace entity
} // namespace sly
