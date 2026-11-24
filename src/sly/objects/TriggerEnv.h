// openTIT - Damage and wind trigger volumes.
//
// A damage volume hurts (and knocks back) what enters it, using a reaction type
// and a push orientation given as Euler angles. A wind volume creates a wind
// source of the wind system (sly/game/Wind) for its lifetime.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace sly {
namespace objects {

// ---------------------------------------------------------------------------
// Damage
// ---------------------------------------------------------------------------
constexpr uint32_t kDefClass_DamageTrigger = 0x08000000;   // def class bit and placement flag
// NOTE: only "ReactType" and "KnockbackImpulse" were recovered; the others are
// named after their position in the definition.
constexpr uint32_t kAttr_DamageRotation  = 0x3C6AE50B;   // Vec3, degrees
constexpr uint32_t kAttr_ReactType       = 0x632DA119;   // "ReactType"
constexpr uint32_t kAttr_DamageAmount    = 0x1486FF40;
constexpr uint32_t kAttr_DamageValue2    = 0x96B07E43;
constexpr uint32_t kAttr_KnockbackImpulse = 0x1B75F0DA;  // "KnockbackImpulse"
constexpr uint32_t kAttr_DamageFlag0     = 0xC53B9DE4;
constexpr uint32_t kAttr_DamageFlag1     = 0xF7C51457;
constexpr uint32_t kAttr_DamageFlag2     = 0x013BDD92;
constexpr uint32_t kAttr_DamageFlag3     = 0x23872185;

// The damage parameters shared by the definition, placement and instance.
struct DamageParams {
    ape::Vec3 rotation{};        // radians
    uint32_t reactType = 0;
    float amount = 0.0f;
    float value2 = 0.0f;
    float knockback = 0.0f;
    bool flags[4] = {};
};

// Reads the damage attributes over `params` (angles converted when given).
void ParseDamageParams(ape::xml::XmlElement* elem, DamageParams& params);

struct DamageTriggerDefDesc : TriggerDefDesc {
    DamageParams params;         // 0x60..0x83
    explicit DamageTriggerDefDesc(const EntityDefDesc& loader);
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class DamageTriggerDef : public TriggerDef {
public:
    explicit DamageTriggerDef(const DamageTriggerDefDesc& desc);
    ObjEntity* Create(EntityPlacement* placement) override;
    DamageParams params;         // 0xB0..0xD3
};

class DamageTriggerPlacement : public TriggerPlacement {
public:
    explicit DamageTriggerPlacement(const char* placementName);
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;
    DamageParams params;         // 0xC0..0xE3
};

struct DamageTriggerInstanceDesc : TriggerInstanceDesc {
    DamageTriggerInstanceDesc(const char* instanceName, DamageTriggerDef* ownerDef);
    ape::Vec3 rotation{};        // 0xA0
    uint32_t reactType = 0;      // 0xB0
    float amount = 0.0f;         // 0xB4
    float value2 = 0.0f;         // 0xB8
    float knockback = 0.0f;      // 0xBC
    bool flags[5] = {};          // 0xC0..0xC4
};

class DamageTrigger : public TriggerBase {
public:
    explicit DamageTrigger(const DamageTriggerInstanceDesc& desc);
    ~DamageTrigger() override;

    const ape::Vec4& PushOrientation() const { return m_orientation; }

private:
    ape::Vec4 m_orientation{};   // 0x1B0 quaternion from the rotation angles
    uint32_t m_field1C0 = 0;     // 0x1C0
    uint32_t m_reactType;        // 0x1C4
    float m_amount;              // 0x1C8
    float m_value2;              // 0x1CC
    float m_knockback;           // 0x1D0
    bool m_flags[5] = {};        // 0x1D4 bits 31..27
};

EntityDef* CreateDamageTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseDamageTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                             const char* name);
// Quaternion of XYZ Euler angles (radians), as the damage trigger builds it.
ape::Vec4 QuaternionFromEuler(const ape::Vec3& angles);

// ---------------------------------------------------------------------------
// Wind
// ---------------------------------------------------------------------------
constexpr uint32_t kTrigType_Wind = 0x02;
constexpr uint32_t kAttr_ScriptLifetime  = 0xF89FDD94;   // "ScriptLifetime"
constexpr uint32_t kAttr_WindGroup       = 0x98B61B57;   // "WindGroup"
constexpr uint32_t kAttr_WindType        = 0xEC2CEB42;   // "WindType"
constexpr uint32_t kAttr_WindStrength    = 0x7C8F6049;   // "WindStrength"
constexpr uint32_t kAttr_Wind_64         = 0x8FE21435;   // NOTE: unnamed float
constexpr uint32_t kAttr_Wind_68         = 0x1FA627BC;   // NOTE: unnamed float
constexpr uint32_t kAttr_Wind_6C         = 0x8C014439;   // NOTE: unnamed float
constexpr uint32_t kAttr_AirDampening    = 0xE0E67DAF;   // "AirDampening"
constexpr uint32_t kAttr_AirDampeningTime = 0xE92958FC;  // "AirDampeningTime"
constexpr uint32_t kAttr_Wind_78         = 0x8F8BE00A;   // NOTE: unnamed bool

// Index of a wind type name in the type table (0x00AD38B0: "Directional" and two
// unnamed types), or `fallback` when the name is not listed.
uint32_t WindTypeIndex(uint32_t nameHash, uint32_t fallback);
// Reads only the "WindType" attribute into `type` (0x00223CA0).
void ParseWindType(ape::xml::XmlElement* elem, uint32_t& type);

struct WindTriggerDefDesc : TriggerDefDesc {
    uint32_t scriptLifetime = 0;   // 0x54 anim script driving the wind
    uint32_t windGroup = 0;        // 0x58
    uint32_t windType = 0;         // 0x5C
    float strength = 0.0f;         // 0x60
    float field_64 = 0.0f;         // 0x64
    float field_68 = 0.0f;         // 0x68
    float field_6C = 0.0f;         // 0x6C
    float airDampening = 0.0f;     // 0x70
    float airDampeningTime = 0.0f; // 0x74
    bool field_78 = false;         // 0x78
    explicit WindTriggerDefDesc(const EntityDefDesc& loader);
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class WindTriggerDef : public TriggerDef {
public:
    explicit WindTriggerDef(const WindTriggerDefDesc& desc);
    ObjEntity* Create(EntityPlacement* placement) override;

    uint32_t scriptLifetime = 0;   // 0xB0
    uint32_t windGroup = 0;        // 0xB4
    uint32_t windType = 0;         // 0xB8
    float strength = 0.0f;         // 0xBC
    float field_C0 = 0.0f;         // 0xC0
    float field_C4 = 0.0f;         // 0xC4
    float field_C8 = 0.0f;         // 0xC8
    float airDampening = 0.0f;     // 0xCC
    float airDampeningTime = 0.0f; // 0xD0
    bool field_D4 = false;         // 0xD4
};

class WindTrigger : public TriggerBase {
public:
    explicit WindTrigger(const TriggerInstanceDesc& desc);
    ~WindTrigger() override;

private:
    uint32_t m_scriptLifetime = 0; // 0x1AC
    bool m_field1BC = false;       // 0x1BC
};

EntityDef* CreateWindTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                const EntityDefDesc& loader, const EntityDef* parent);

} // namespace objects
} // namespace sly
