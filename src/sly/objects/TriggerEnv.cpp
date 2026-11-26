// openTIT - Damage and wind trigger volumes.
#include "sly/objects/TriggerEnv.h"

#include "ape/anim/ApeAnimScript.h"

#include "sly/objects/TriggerTypes.h"

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"
#include "ape/math/ApeMatrixOps.h"

#include <cmath>

namespace sly {
namespace objects {

namespace {
constexpr float kDegToRad = 0.017453292f;

void ReadRotation(ape::xml::XmlElement* elem, ape::Vec3& out, const ape::Vec3& fallback)
{
    float value[3] = { out.x, out.y, out.z };
    float def[3] = { fallback.x, fallback.y, fallback.z };
    // Angles are authored in degrees; inherited values are already radians.
    if (elem->GetVec3(kAttr_DamageRotation, value, def) == ape::xml::kQuery_Ok)
        out = { value[0] * kDegToRad, value[1] * kDegToRad, value[2] * kDegToRad, 0.0f };
    else
        out = { value[0], value[1], value[2], 0.0f };
}

void ReadDamageScalars(ape::xml::XmlElement* elem, DamageParams& out, const DamageParams& fallback)
{
    elem->GetHash(kAttr_ReactType, &out.reactType, fallback.reactType);
    elem->GetFloat(kAttr_DamageAmount, &out.amount, fallback.amount);
    elem->GetFloat(kAttr_DamageValue2, &out.value2, fallback.value2);
    elem->GetFloat(kAttr_KnockbackImpulse, &out.knockback, fallback.knockback);
    elem->GetBool(kAttr_DamageFlag0, &out.flags[0], fallback.flags[0]);
    elem->GetBool(kAttr_DamageFlag1, &out.flags[1], fallback.flags[1]);
    elem->GetBool(kAttr_DamageFlag2, &out.flags[2], fallback.flags[2]);
    elem->GetBool(kAttr_DamageFlag3, &out.flags[3], fallback.flags[3]);
}

// Wind type names (table 0x00AD38B0).
constexpr uint32_t kWindTypeHashes[3] = {
    0x1F0FC3CA,   // "Directional"
    0x9D9722F0,   // NOTE: name not recovered
    0xDCC384C1,   // NOTE: name not recovered
};
} // namespace

// @func 0x0021AA68 ReadDamageTriggerPlacement
static void ReadDamageTriggerPlacement(ape::xml::XmlElement* elem, DamageTriggerPlacement& placement)
{
    ParseTriggerPlacementFlags(elem, placement);
    ParseDamageParams(elem, placement.params);
}

void ParseDamageParams(ape::xml::XmlElement* elem, DamageParams& params)
{
    DamageParams current = params;
    ReadRotation(elem, params.rotation, current.rotation);
    ReadDamageScalars(elem, params, current);
}

ape::Vec4 QuaternionFromEuler(const ape::Vec3& angles)
{
    float hx = angles.x * 0.5f, hy = angles.y * 0.5f, hz = angles.z * 0.5f;
    float cx = ape::FastCos(hx), cy = ape::FastCos(hy), cz = ape::FastCos(hz);
    float sx = ape::FastSin(hx), sy = ape::FastSin(hy), sz = ape::FastSin(hz);
    ape::Vec4 q;
    q.x = sy * sz * cx + cy * cz * sx;
    q.y = cz * sy * cx + cy * sz * sx;
    q.z = cy * sz * cx - cz * sy * sx;
    q.w = cy * cz * cx - sy * sz * sx;
    return q;
}

// ===========================================================================
// Damage
// ===========================================================================

DamageTriggerDefDesc::DamageTriggerDefDesc(const EntityDefDesc& loader)
    : TriggerDefDesc(kDefClass_DamageTrigger | kDefClass_Trigger, loader, "TriggerDamage_")
{
    field_20 = 1;
}

// @func 0x00219FF8 DamageTriggerDefDesc::Parse
void DamageTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    // Defaults: a damage parent's values (only its first two flags), otherwise
    // the description defaults.
    DamageParams fallback;
    if (parent && (parent->classBits & kDefClass_DamageTrigger)) {
        const DamageParams& from = static_cast<const DamageTriggerDef*>(parent)->params;
        fallback.rotation = from.rotation;
        fallback.reactType = from.reactType;
        fallback.amount = from.amount;
        fallback.value2 = from.value2;
        fallback.knockback = from.knockback;
        fallback.flags[0] = from.flags[0];
        fallback.flags[1] = from.flags[1];
    }
    ReadRotation(elem, params.rotation, fallback.rotation);
    ReadDamageScalars(elem, params, fallback);
}

DamageTriggerDef::DamageTriggerDef(const DamageTriggerDefDesc& desc)
    : TriggerDef(desc), params(desc.params)
{
}

// @func 0x0021A310 CreateDamageTriggerDef
EntityDef* CreateDamageTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent)
{
    DamageTriggerDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    return new DamageTriggerDef(desc);
}

DamageTriggerInstanceDesc::DamageTriggerInstanceDesc(const char* instanceName, DamageTriggerDef* ownerDef)
    : TriggerInstanceDesc(instanceName, ownerDef)
{
    if (!ownerDef)
        return;
    const DamageParams& p = ownerDef->params;
    rotation = p.rotation;
    reactType = p.reactType;
    amount = p.amount;
    value2 = p.value2;
    knockback = p.knockback;
    // NOTE: the instance has five flag slots; slot 2 is not fed by the definition.
    flags[0] = p.flags[0];
    flags[1] = p.flags[1];
    flags[2] = false;
    flags[3] = p.flags[2];
    flags[4] = p.flags[3];
}

// @func 0x0021A6E8 DamageTriggerDef::Create
Entity* DamageTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kDefClass_DamageTrigger) != 0))
        return nullptr;
    DamageTriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    DamageTrigger* trigger = new DamageTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

DamageTriggerPlacement::DamageTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    flags |= kDefClass_DamageTrigger;
}

// @func 0x0021AD0C ParseDamageTriggerPlacement
EntityPlacement* ParseDamageTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                             const char* name)
{
    (void)root;
    DamageTriggerPlacement* placement = new DamageTriggerPlacement(name);
    ReadDamageTriggerPlacement(elem, *placement);
    return placement;
}

// @func 0x0021AE1C DamageTriggerPlacement::FillInstanceDesc
void DamageTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    DamageTriggerInstanceDesc& desc = static_cast<DamageTriggerInstanceDesc&>(baseDesc);
    desc.rotation = params.rotation;
    desc.reactType = params.reactType;
    desc.amount = params.amount;
    desc.value2 = params.value2;
    desc.knockback = params.knockback;
    // NOTE: the console skips instance flag slot 2 here as well.
    desc.flags[0] = params.flags[0];
    desc.flags[1] = params.flags[1];
    desc.flags[3] = params.flags[2];
    desc.flags[4] = params.flags[3];
}

// @func 0x0021A518 DamageTrigger::DamageTrigger
DamageTrigger::DamageTrigger(const DamageTriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x20, true),
      m_orientation(QuaternionFromEuler(desc.rotation)),
      m_reactType(desc.reactType),
      m_amount(desc.amount),
      m_value2(desc.value2),
      m_knockback(desc.knockback)
{
    for (int i = 0; i < 5; ++i)
        m_flags[i] = desc.flags[i];
}

// @func 0x0021B088 DamageTrigger::~DamageTrigger (deleting)
DamageTrigger::~DamageTrigger() = default;

uint32_t DamageTrigger::OccupantBit(const Entity* entity) const
{
    // The console shifts by the index as a 64-bit amount, so "not found" (-1)
    // and slots past 31 give no bit in the 32-bit mask.
    int index = FindOccupant(entity);
    return (index >= 0 && index < 32) ? (1u << index) : 0u;
}

// Hit direction: straight up when flag 1 is set; otherwise outwards from the
// trigger at 60 degrees elevation, or along the trigger's oriented third axis
// when it has an orientation (or flag 2 asks for it). Flag 0 makes that axis
// two-sided: targets behind it get the axis turned half a turn about Y.
// @func 0x0021B0E0 DamageTrigger::ApplyDamage
bool DamageTrigger::ApplyDamage(Entity* target, float amount, uint32_t reactType)
{
    if (!target || amount <= 0.0f)
        return false;
    if (!target->HasTypeBit(kType_Scripted))
        return false;

    const ape::Vec3 up = {0.0f, 1.0f, 0.0f, 0.0f};   // console global 0x01164B60
    const ape::Vec3 zero = {0.0f, 0.0f, 0.0f, 0.0f}; // console global 0x01164B40
    const ape::Vec4 identity = {0.0f, 0.0f, 0.0f, 1.0f};
    ape::Vec3 dir = up;

    // Third axis of the orientation applied to the trigger transform.
    // NOTE: the console multiplies in place with 0x005541F0; the operand order
    // (orientation first) is assumed.
    auto orientedAxis = [&](const ape::Vec4& q) {
        ape::Matrix34 rot, combined;
        ape::Matrix34FromQuat(rot, q, zero);
        ape::Matrix34MulRotation(combined, rot, transform);
        return combined.zAxis;
    };

    if (!m_flags[1]) {
        bool identityOrientation = m_orientation.x == identity.x && m_orientation.y == identity.y &&
                                   m_orientation.z == identity.z && m_orientation.w == identity.w;
        if (!m_flags[2] && identityOrientation) {
            const float elevation = 1.0471976f;   // 60 degrees
            float dx = target->Position().x - Position().x;
            float dz = target->Position().z - Position().z;
            float scale = (1.0f / std::sqrt(dx * dx + dz * dz)) * ape::FastCos(elevation);
            dir.x = dx * scale;
            dir.y = ape::FastSin(elevation);
            dir.z = dz * scale;
        } else {
            dir = orientedAxis(m_orientation);
            if (m_flags[0]) {
                float side = (target->Position().x - Position().x) * dir.x +
                             (target->Position().y - Position().y) * dir.y +
                             (target->Position().z - Position().z) * dir.z;
                if (side < 0.0f) {
                    const ape::Vec3 halfTurn = {0.0f, 3.1415927f, 0.0f, 0.0f};
                    ape::Vec4 turned;
                    ape::QuatMul(turned, m_orientation, QuaternionFromEuler(halfTurn));
                    dir = orientedAxis(turned);
                }
            }
        }
    }

    EntityHit hit;
    hit.source = this;
    hit.reactType = reactType;
    hit.kind = 3;
    hit.fromPlayer = EntityIsPlayer(this);
    hit.flag11 = m_flags[3];
    hit.flag10 = m_flags[4];
    hit.direction = dir;
    hit.amount = amount;
    hit.knockback = m_knockback;
    EntityTakeHit(target, hit);
    return hit.accepted;
}

// @func 0x0021B5E0 DamageTrigger::Update
bool DamageTrigger::Update(float dt)
{
    // Nothing more when the base update reports failure.
    const bool result = TriggerBase::Update(dt);
    if (!result || !m_enabled)
        return result;
    float perFrame = m_value2 * dt;   // damage per second
    for (int i = 0; i < OccupantCount(); ++i) {
        uint32_t bit = i < 32 ? (1u << i) : 0u;
        Entity* occupant = Occupant(i);
        if ((m_pendingHits & bit) && ApplyDamage(occupant, m_amount, m_reactType))
            m_pendingHits &= ~bit;
        if (perFrame > 0.0f)
            ApplyDamage(occupant, perFrame, m_reactType);
    }
    return result;
}

// The entry hit is owed until a frame where the target accepts it.
// @func 0x0021B738 DamageTrigger::OnEnter
void DamageTrigger::OnEnter(Entity* entity)
{
    TriggerBase::OnEnter(entity);
    if (m_amount > 0.0f)
        m_pendingHits |= OccupantBit(entity);
}

// @func 0x0021B7F4 DamageTrigger::OnExit
void DamageTrigger::OnExit(Entity* entity)
{
    m_pendingHits &= ~OccupantBit(entity);
    TriggerBase::OnExit(entity);
}

// ===========================================================================
// Wind
// ===========================================================================

uint32_t WindTypeIndex(uint32_t nameHash, uint32_t fallback)
{
    for (uint32_t i = 0; i < 3; ++i)
        if (kWindTypeHashes[i] == nameHash)
            return i;
    return fallback;
}

// @func 0x00223CA0 ParseWindType
void ParseWindType(ape::xml::XmlElement* elem, uint32_t& type)
{
    uint32_t name = 0;
    elem->GetHash(kAttr_WindType, &name, 0);
    if (name != 0)
        type = WindTypeIndex(name, type);
}

WindTriggerDefDesc::WindTriggerDefDesc(const EntityDefDesc& loader)
    : TriggerDefDesc(kDefClass_Trigger, loader, "TriggerWind_")
{
    typeBits = kTrigType_Wind;
}

// @func 0x00223D38 WindTriggerDefDesc::Parse
void WindTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                               const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    field_20 = 1;
    // A parent that is not a wind trigger only triggers a warning (stripped).
    elem->GetHash(kAttr_ScriptLifetime, &scriptLifetime, scriptLifetime);
    elem->GetHash(kAttr_WindGroup, &windGroup, 0);
    windType = 0;
    ParseWindType(elem, windType);
    elem->GetFloat(kAttr_WindStrength, &strength, 0.0f);
    elem->GetFloat(kAttr_Wind_64, &field_64, 0.0f);
    elem->GetFloat(kAttr_Wind_68, &field_68, 0.0f);
    elem->GetFloat(kAttr_Wind_6C, &field_6C, 0.0f);
    elem->GetFloat(kAttr_AirDampening, &airDampening, 0.0f);
    elem->GetFloat(kAttr_AirDampeningTime, &airDampeningTime, 0.0f);
    elem->GetBool(kAttr_Wind_78, &field_78, false);
}

WindTriggerDef::WindTriggerDef(const WindTriggerDefDesc& desc)
    : TriggerDef(desc),
      scriptLifetime(desc.scriptLifetime),
      windGroup(desc.windGroup),
      windType(desc.windType),
      strength(desc.strength),
      field_C0(desc.field_64),
      field_C4(desc.field_68),
      field_C8(desc.field_6C),
      airDampening(desc.airDampening),
      airDampeningTime(desc.airDampeningTime),
      field_D4(desc.field_78)
{
}

// @func 0x00223F94 CreateWindTriggerDef
EntityDef* CreateWindTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                const EntityDefDesc& loader, const EntityDef* parent)
{
    WindTriggerDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    return new WindTriggerDef(desc);
}

// @func 0x00224720 WindTriggerDef::Create
Entity* WindTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_Wind) != 0))
        return nullptr;
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    WindTrigger* trigger = new WindTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

namespace {
constexpr uint32_t kScriptVarEntered = 0x1ABD6576;   // NOTE: name unknown (last entity in)
constexpr uint32_t kScriptVarExited = 0x2C73353E;    // NOTE: name unknown (last entity out)
constexpr uint32_t kScriptStateSpawn = 0x04B6FB5B;   // "Spawn"
constexpr uint32_t kScriptStateOnEnter = 0xDC175839; // "OnEnter"
constexpr uint32_t kScriptStateOnExit = 0x8B3AB37D;  // "OnExit"
constexpr uint64_t kClassWindAffected = 0x100000;    // entity class bit honoured by restricted winds
} // namespace

// @func 0x00224170 WindTrigger::VolumeTransform
ape::Matrix34 WindTrigger::VolumeTransform() const
{
    ape::Matrix34 transform{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
    if (!m_volume || !VolumeIsInWorld(m_volume))
        return transform;
    const int shape = VolumeShapeType(m_volume);
    if (shape == 0) {
        // AABB phantoms: centre of the box, no rotation.
        const ape::Aabb box = VolumeGetAabb(m_volume);
        ape::AabbCenter(box, transform.translation);
    } else if (shape == 1 || shape == 2) {
        transform = VolumeWorldTransform(m_volume);
    }
    return transform;
}

// @func 0x00224448 WindTrigger::WindTrigger
WindTrigger::WindTrigger(const TriggerInstanceDesc& desc)
    : TriggerBase(desc, WindTriggerCapacity(), true)
{
    const ape::Matrix34 transform = VolumeTransform();
    const WindTriggerDef* windDef = static_cast<const WindTriggerDef*>(def);
    m_scriptLifetime = windDef->scriptLifetime;
    m_restricted = windDef->field_D4;
    // The lifetime script receives the trigger and gets "Spawn" on the first update.
    if (void* script = ape::ApeAnimScriptLibrary::FindScriptAnywhere(m_scriptLifetime, 0, true)) {
        m_script = EntityStartScript(this, script);
        if (m_script) {
            ScriptInstanceSetPaused(m_script, false);
            m_spawnPending = true;
            ScriptSetTriggerVar(m_script, this);
        }
    }
    m_wind = WindFieldCreate(transform, def);
}

// @func 0x0022499C WindTrigger::~WindTrigger
// @func 0x00224B00 WindTrigger::~WindTrigger (deleting)
WindTrigger::~WindTrigger()
{
    if (m_script)
        ScriptInstanceSetPaused(m_script, true);
}

// @func 0x00224B48 WindTrigger::Release
bool WindTrigger::Release(bool immediate)
{
    if (immediate && m_wind) {
        WindFieldDestroy(m_wind);
        m_wind = nullptr;
    }
    return TriggerBase::Release(immediate);
}

bool WindTrigger::AffectsEntity(const Entity* entity) const
{
    return !m_restricted || (entity->classBits & kClassWindAffected) != 0;
}

// @func 0x00224BC8 WindTrigger::Update
bool WindTrigger::Update(float dt)
{
    if (m_spawnPending) {
        m_spawnPending = false;
        if (m_script)
            ScriptSetState(m_script, kScriptStateSpawn);
    }
    WindFieldUpdate(m_wind, dt);
    const bool result = TriggerBase::Update(dt);
    // Keep the field's entity set in step with the occupants that are alive.
    for (Entity* occupant : m_occupants) {
        if (!occupant->IsAlive())
            continue;
        const bool wanted = AffectsEntity(occupant);
        const bool present = WindFieldHasEntity(m_wind, occupant);
        if (present && !wanted)
            WindFieldRemoveEntity(m_wind, occupant, true);
        else if (!present && wanted)
            WindFieldAddEntity(m_wind, occupant);
    }
    return result;
}

// @func 0x00224D50 WindTrigger::SetTransform
void WindTrigger::SetTransform(const ape::Matrix34& m)
{
    // NOTE: the trigger base part (0x0020D7E8, which also moves the phantom)
    // is not ported; the entity transform is set instead.
    Entity::SetTransform(m);
    if (m_wind)
        WindFieldSetTransform(m_wind, VolumeTransform());
}

// @func 0x00224E20 WindTrigger::OnEnter
void WindTrigger::OnEnter(Entity* entity)
{
    TriggerBase::OnEnter(entity);
    if (AffectsEntity(entity) && m_wind && WindFieldAddEntity(m_wind, entity) && m_onEnter)
        m_onEnter(this, entity);
    if (m_script) {
        if (ScriptGetEntityVar(m_script, kScriptVarEntered) == entity)
            ScriptSetEntityVar(m_script, kScriptVarEntered, nullptr);
        ScriptSetEntityVar(m_script, kScriptVarExited, entity);
        ScriptSetState(m_script, kScriptStateOnEnter);
    }
}

// @func 0x00224F48 WindTrigger::OnExit
void WindTrigger::OnExit(Entity* entity)
{
    if (m_wind)
        WindFieldRemoveEntity(m_wind, entity, false);
    if (m_script) {
        if (ScriptGetEntityVar(m_script, kScriptVarExited) == entity)
            ScriptSetEntityVar(m_script, kScriptVarExited, nullptr);
        ScriptSetEntityVar(m_script, kScriptVarEntered, entity);
        ScriptSetState(m_script, kScriptStateOnExit);
    }
    TriggerBase::OnExit(entity);
}

// @func 0x00225010 WindTrigger::RemoveOccupant
void WindTrigger::RemoveOccupant(Entity* entity)
{
    if (m_wind)
        WindFieldRemoveEntity(m_wind, entity, true);
    TriggerBase::RemoveOccupant(entity);
}

// @func 0x0022506C WindTrigger::SetEnabled
void WindTrigger::SetEnabled(bool enabled)
{
    if (m_wind)
        WindFieldSetEnabled(m_wind, enabled);
    ChangeEnabled(enabled);
}

} // namespace objects
} // namespace sly
