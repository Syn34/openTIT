// openTIT - The physical definition: description defaults, parser and
// constructor (see PhysicalEntity.h).
#include "sly/entity/ActorDefLoaders.h"
#include "sly/entity/PhysicalEntity.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/entity/Projectiles.h"

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>

namespace sly {
namespace entity {

namespace {

constexpr float kDegToRad = 0.017453292f;

// Reads boolean attribute `attr` into bit `bit` of `word` (default: the bit of `defaults`).
void ReadBit(ape::xml::XmlElement* elem, uint32_t attr, uint32_t& word, uint32_t defaults, int bit)
{
    bool on = false;
    elem->GetBool(attr, &on, ((defaults >> bit) & 1) != 0);
    word = (word & ~(1u << bit)) | (uint32_t(on) << bit);
}

// The flag attributes by bit, in the order the parser reads them.
struct FlagAttr {
    uint32_t attr;
    int bit;
};
constexpr FlagAttr kFlagAttrs[] = {
    {0xBA6158CDu, 31}, {0xA499B8DAu, 30}, {0x0264927Cu, 29}, {0x4E552307u, 28}, {0x023E5375u, 27},
    {0xB1445260u, 26}, {0xAA5EBEECu, 25}, {0xE0C44379u, 24}, {0x579B202Bu, 23}, {0xE019565Du, 22},
    {0x92A78501u, 19}, {0x6BEE8F45u, 17}, {0x56FD4A4Du, 16}, {0x8CB95A92u, 15}, {0xDCEB5C01u, 14},
    {0xAFD54C62u, 13}, {0x002700BAu, 12}, {0x0BC0A396u, 11}, {0x0F5DE5C0u, 10}, {0x1667C898u, 9},
    {0x884ECC2Au, 21}, {0x909A908Du, 20},
};
// Flag bits a physical parent passes on (the rest stay the description's).
constexpr uint32_t kInheritedFlags = 0xFFC9FE00u;

// The damage types of child element 0x7C28C95E, by bit.
constexpr uint32_t kDamageTypeAttrs[13] = {0x3B8B3D28u, 0xCB378DD0u, 0xB4AC34A8u, 0x786E8593u, 0xF66BC199u,
                                           0xF91ADF94u, 0x001F32A2u, 0x044DC6ABu, 0x001FE77Fu, 0xA3C4677Au,
                                           0xCBEBC6C2u, 0x635FB09Fu, 0x974D338Cu};
constexpr uint32_t kElem_DamageTypes = 0x7C28C95Eu;
constexpr uint32_t kElem_Field164 = 0x1DD0B478u;
constexpr uint32_t kElem_Field150 = 0x03C599D8u;

} // namespace

PhysicalEntityDefDesc::PhysicalEntityDefDesc(const EntityDefDesc& loader)
    : ScriptedEntityDefDesc(kDefClass_Scripted | kDefClass_Physical, loader)
{
    geomName = loader.geomName;
    templateName = loader.templateName;
    damageFlags = 0x8200;
    for (int i = 0; i < 5; ++i)
        stageHealth[i] = float(((damageFlags >> 4) & 0xF) - i);
    field_14C = 0xA77EB5E9u;   // "projectile"
    field_150[0] = 1.0f;
}

// @func 0x000C4020 PhysicalEntityDefDesc::Parse
void PhysicalEntityDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent)
{
    (void)root;   // only the warnings (stripped) quote the document path
    ScriptedEntityDefDesc::Parse(root, elem, loader, parent);

    // Defaults: what the description holds, or a physical parent's values.
    PhysicalEntityDefDesc defaults = *this;
    if (parent && parent->IsClass(kDefClass_Physical)) {
        const auto* physical = static_cast<const PhysicalEntityDef*>(parent);
        defaults.offset = physical->offset;
        if (void* body = physical->collisionShape) {
            defaults.bodyName = HavokBodyName(body);
            RigidBodyMaterial material = RigidBodyGetMaterial(body);
            defaults.mass = material.mass;
            defaults.friction = material.friction;
            defaults.restitution = material.restitution;
            defaults.linearDamping = material.linearDamping;
            defaults.angularDamping = material.angularDamping;
        }
        const uint32_t ownFlags = defaults.defFlags;
        const uint32_t damageTypes = defaults.damageTypes;
        static_cast<PhysicalDefFields&>(defaults) = *physical;
        defaults.defFlags = (physical->defFlags & kInheritedFlags) | (ownFlags & ~kInheritedFlags);
        defaults.damageTypes = damageTypes;
        // Not inherited: these keep the description's values.
        defaults.field_11C = field_11C;
        defaults.field_120 = field_120;
        defaults.field_124 = field_124;
        defaults.field_128 = field_128;
    }

    if (!geomName || !*geomName)
        geomName = defaults.geomName;
    if (!templateName || !*templateName)
        templateName = defaults.templateName;

    elem->GetVec3(0x63662D61u, &offset.x, &defaults.offset.x);
    const char* name = elem->GetString("colObjName");
    bodyName = name ? name : defaults.bodyName;
    elem->GetFloat(0x0023FEF4u, &mass, defaults.mass);
    elem->GetFloat(0xD9BA76FAu, &friction, defaults.friction);
    elem->GetFloat(0x5AF2884Au, &restitution, defaults.restitution);
    elem->GetFloat(0xA1FDE21Du, &linearDamping, defaults.linearDamping);
    elem->GetFloat(0x8933D0E0u, &angularDamping, defaults.angularDamping);
    elem->GetFloat(0xCEDD379Au, &gravityScale, defaults.gravityScale);
    elem->GetFloat(0x43B894DBu, &field_C4, defaults.field_C4);
    elem->GetFloat(0xFA012435u, &field_C8, defaults.field_C8);
    field_C8 = std::max(field_C8, 0.0f);
    elem->GetFloat(0x824F3B6Fu, &impactScale, defaults.impactScale);
    elem->GetFloat(0xAD6099F3u, &blastScale, defaults.blastScale);
    elem->GetFloat(0x825857B0u, &meleeScale, defaults.meleeScale);
    elem->GetFloat(0x86D97BF8u, &projectileScale, defaults.projectileScale);
    elem->GetFloat(0x6D6DEDACu, &field_DC, defaults.field_DC);
    elem->GetFloat(0x48110963u, &field_E0, defaults.field_E0);
    if (elem->GetVec3(0xBCEEA49Fu, &angles.x, &defaults.angles.x) == ape::xml::kQuery_Ok) {
        angles.x *= kDegToRad;
        angles.y *= kDegToRad;
        angles.z *= kDegToRad;
    }
    if (elem->GetFloat(0x0026F042u, &rollRate, defaults.rollRate) == ape::xml::kQuery_Ok)
        rollRate *= kDegToRad;
    if (elem->GetFloat(0x287EDCD7u, &field_104, defaults.field_104) == ape::xml::kQuery_Ok)
        field_104 *= kDegToRad;
    elem->GetFloat(0xE6233EB6u, &field_108, defaults.field_108);
    elem->GetFloat(0x38D66ECBu, &field_114, defaults.field_114);
    elem->GetFloat(0x6C01B0DCu, &field_118, defaults.field_118);
    elem->GetFloat(0x717526C2u, &field_120, field_DC);   // NOTE: defaults to the value just read
    if (elem->GetFloat(0x56C86E35u, &field_124, field_124) == ape::xml::kQuery_Ok)
        field_124 *= kDegToRad;
    if (elem->GetVec2(0x0D13FD3Fu, field_10C, field_10C) == ape::xml::kQuery_Ok)
        field_10C[1] *= kDegToRad * 0.5f;
    elem->GetFloat(0xDD16AC83u, &field_128, field_128);
    float degrees = 60.0f;
    if (elem->GetFloat(0x7DEAD86Bu, &degrees, 60.0f) == ape::xml::kQuery_Ok)
        field_11C = (degrees <= 0.0f || degrees >= 360.0f) ? -1.0f : ape::FastCos(degrees * kDegToRad);
    elem->GetFloat(0x263F3CA1u, &launchValue, defaults.launchValue);
    elem->GetFloat(0xF4521F7Fu, &field_130, defaults.field_130);
    if (field_130 > 0.0f)
        field_130 *= field_130;
    elem->GetFloat(0xA11783B1u, &field_134, defaults.field_134);
    if (field_134 > 0.0f)
        field_134 *= field_134;
    elem->GetFloat(0xF58DBE1Fu, &field_138, defaults.field_138);
    elem->GetHash(0x2D9FE345u, &impactSoundName, defaults.impactSoundName);
    elem->GetHash(0xC8E6EA68u, &field_140, defaults.field_140);
    elem->GetHash(0x51D7D9E6u, &carriedDefName, defaults.carriedDefName);
    for (const FlagAttr& flag : kFlagAttrs)
        ReadBit(elem, flag.attr, defFlags, defaults.defFlags, flag.bit);
    elem->GetHash(0x64129428u, &field_148, defaults.field_148);
    elem->GetHash(0x632DA119u, &field_14C, defaults.field_14C);

    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t kind = child->IsValid() ? child->GetNameHash() : 0;
        if (kind == kElem_DamageTypes) {
            // Each type given sets or clears its bit.
            for (int i = 0; i < 13; ++i) {
                bool on = false;
                if (child->GetBool(kDamageTypeAttrs[i], &on, false) == ape::xml::kQuery_Ok)
                    damageTypes = on ? (damageTypes | (1u << i)) : (damageTypes & ~(1u << i));
            }
        } else if (kind == kElem_Field164) {
            defFlags |= PhysicalEntityDef::kDefFlag_Field164;
            child->GetFloat(0xC40CA429u, &field_164[0], defaults.field_164[0]);
            child->GetFloat(0x17D91276u, &field_164[1], defaults.field_164[1]);
            child->GetFloat(0x52EB1B8Fu, &field_164[2], defaults.field_164[2]);
        } else if (kind == kElem_Field150) {
            hasField150 = true;
            child->GetFloat(0x77C17F6Fu, &field_150[0], defaults.field_150[0]);
            child->GetFloat(0x9D9B30D5u, &field_150[1], defaults.field_150[1]);
            child->GetFloat(0x6CA62860u, &field_150[2], defaults.field_150[2]);
            child->GetFloat(0x6CA62772u, &field_150[3], defaults.field_150[3]);
            child->GetBool(0xB38E5179u, &field_160, defaults.field_160);
        }
    }
}

// @func 0x000C30E8 PhysicalEntityDef::PhysicalEntityDef
PhysicalEntityDef::PhysicalEntityDef(const PhysicalEntityDefDesc& desc)
    : ScriptedEntityDef(desc), PhysicalDefFields(desc), offset(desc.offset)
{
    defFlags = desc.defFlags & 0xFFFFFE00u;
    collisionShape = PhysicalDefCreateBody(desc.bodyName, desc.offset, desc.mass, desc.friction,
                                           desc.restitution, desc.linearDamping, desc.angularDamping,
                                           (defFlags & kDefFlag_HighQuality) != 0);
}

// -- Projectile and ProjectileB ---------------------------------------------------
namespace {

constexpr uint32_t kClass_Projectile = 0x4000;
constexpr uint32_t kClass_ProjectileB = 0x10000;

struct ProjectileDefDesc : PhysicalEntityDefDesc {
    explicit ProjectileDefDesc(const EntityDefDesc& loader) : PhysicalEntityDefDesc(loader)
    {
        classBits |= kClass_Projectile;
    }
    float field_158[5] = {0.0f, 0.0f, -1.0f, 0.0f, 0.0f};   // 0x158
    uint32_t field_16C = 0;                                 // 0x16C
    float field_170 = 0.0f;                                 // 0x170
    float field_174 = 0.0f;                                 // 0x174
    bool field_178 = false;                                 // 0x178

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x000C5FF0 ProjectileDefDesc::Parse
// NOTE: the two angles are converted from degrees even when they come from
// a parent (whose values are radians already), as on the console.
void ProjectileDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                              const EntityDefDesc& loader, const EntityDef* parent)
{
    PhysicalEntityDefDesc::Parse(root, elem, loader, parent);
    float defaults[5] = {0.0f, 0.0f, -1.0f, 0.0f, 0.0f};
    float minAngle = -90.0f, maxAngle = 270.0f;
    bool defaultFlag = false;
    if (parent && (parent->classBits & kClass_Projectile)) {
        const auto* projectile = static_cast<const ProjectileDef*>(parent);
        defaults[0] = projectile->field_194;
        defaults[1] = projectile->field_198;
        defaults[2] = projectile->field_19C;
        defaults[3] = projectile->field_1A0;
        defaults[4] = projectile->arcFactor;
        minAngle = projectile->field_1AC;
        maxAngle = projectile->field_1B0;
        defaultFlag = projectile->field_1B4;
    }
    static constexpr uint32_t kAttrs[5] = {0xB81F0E21u, 0xB81F0D33u, 0x4BC4DED1u, 0x4BA728F9u, 0x04F32F5Cu};
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(kAttrs[i], &field_158[i], defaults[i]);
    static constexpr uint32_t kValues[2] = {0x85D98325u, 0x3619362Du};
    uint32_t value = 0;
    if (elem->GetHash(0xE8DB4898u, &value, 0) == ape::xml::kQuery_Ok) {
        for (uint32_t i = 0; i < 2; ++i) {
            if (kValues[i] == value) {
                field_16C = i;
                break;
            }
        }
    }
    elem->GetFloat(0x39F5434Cu, &field_170, minAngle);
    elem->GetFloat(0xA3D3DF7Au, &field_174, maxAngle);
    field_170 *= kDegToRad;
    field_174 *= kDegToRad;
    elem->GetBool(0x99A462A8u, &field_178, defaultFlag);
}

struct ProjectileBDefDesc : PhysicalEntityDefDesc {
    explicit ProjectileBDefDesc(const EntityDefDesc& loader) : PhysicalEntityDefDesc(loader)
    {
        classBits |= kClass_ProjectileB;
    }
    float field_158[4] = {1.0f, 0.5f, 1.0f, 0.0f};   // 0x158
    float field_168 = 1.0f;                          // 0x168

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x000C5B30 ProjectileBDefDesc::Parse
void ProjectileBDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                               const EntityDefDesc& loader, const EntityDef* parent)
{
    PhysicalEntityDefDesc::Parse(root, elem, loader, parent);
    float defaults[4] = {1.0f, 0.5f, 1.0f, 0.0f};
    if (parent && (parent->classBits & kClass_ProjectileB)) {
        const auto* projectile = static_cast<const ProjectileBDef*>(parent);
        defaults[0] = projectile->field_194;
        defaults[1] = projectile->field_198;
        defaults[2] = projectile->field_19C;
        defaults[3] = projectile->field_1A0;
    }
    static constexpr uint32_t kAttrs[4] = {0x7ECB8527u, 0xCCF74C48u, 0x9E8B1865u, 0x21684859u};
    for (int i = 0; i < 4; ++i)
        elem->GetFloat(kAttrs[i], &field_158[i], defaults[i]);
}

} // namespace

// @func 0x000C63B0 CreateProjectileDef
EntityDef* CreateProjectileDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ProjectileDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ProjectileDef(desc);
    def->field_194 = desc.field_158[0];
    def->field_198 = desc.field_158[1];
    def->field_19C = desc.field_158[2];
    def->field_1A0 = desc.field_158[3];
    def->arcFactor = desc.field_158[4];
    def->field_1A8 = desc.field_16C;
    def->field_1AC = desc.field_170;
    def->field_1B0 = desc.field_174;
    def->field_1B4 = desc.field_178;
    return def;
}

// @func 0x000C5DC8 CreateProjectileBDef
EntityDef* CreateProjectileBDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    ProjectileBDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ProjectileBDef(desc);
    def->field_194 = desc.field_158[0];
    def->field_198 = desc.field_158[1];
    def->field_19C = desc.field_158[2];
    def->field_1A0 = desc.field_158[3];
    def->field_1A4 = desc.field_168;
    return def;
}

// -- EntityProjectileMine ---------------------------------------------------------
namespace {

constexpr uint32_t kClass_ProjectileMine = 0x20000;

struct ProjectileMineDefDesc : PhysicalEntityDefDesc {
    explicit ProjectileMineDefDesc(const EntityDefDesc& loader) : PhysicalEntityDefDesc(loader)
    {
        classBits |= kClass_ProjectileMine;
    }
    uint32_t field_158 = 0;     // 0x158
    float field_15C = 1.0f;     // 0x15C
    bool field_160 = false;     // 0x160
    float field_164 = 1.0f;     // 0x164
    float field_168 = 0.0f;     // 0x168
    float field_16C = 1.0f;     // 0x16C
    float field_170 = 0.0f;     // 0x170
    float field_174 = 0.0f;     // 0x174
    bool field_178 = false;     // 0x178
    bool field_179 = true;      // 0x179
    bool field_17A = false;     // 0x17A

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x000C5528 ProjectileMineDefDesc::Parse
// Five values come from a mine parent; the rest have fixed defaults.
void ProjectileMineDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent)
{
    PhysicalEntityDefDesc::Parse(root, elem, loader, parent);
    uint32_t defaultHash = 0;
    float default15C = 1.0f, default164 = 1.0f;
    bool default160 = false, default17A = false;
    if (parent && (parent->classBits & kClass_ProjectileMine)) {
        const auto* mine = static_cast<const ProjectileMineDef*>(parent);
        defaultHash = mine->field_194;
        default15C = mine->field_198;
        default160 = mine->field_19C;
        default164 = mine->field_1A0;
        default17A = mine->field_1B6;
    }
    elem->GetHash(0xFEBE4776u, &field_158, defaultHash);
    elem->GetFloat(0xD93B86F6u, &field_15C, default15C);
    elem->GetBool(0xFBCDDB3Bu, &field_160, default160);
    elem->GetFloat(0xD63894F0u, &field_164, default164);
    elem->GetFloat(0x35FE9ECFu, &field_168, 0.0f);
    elem->GetFloat(0xB289BA5Eu, &field_170, 0.0f);
    elem->GetFloat(0x9EB4518Au, &field_174, 0.0f);
    elem->GetBool(0x0927884Eu, &field_178, false);
    elem->GetBool(0x572A6E83u, &field_179, true);
    elem->GetBool(0x644AB625u, &field_17A, default17A);
}

} // namespace

// @func 0x000C58A8 CreateProjectileMineDef
// The mine's body goes on collision layer 23.
EntityDef* CreateProjectileMineDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent)
{
    ProjectileMineDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ProjectileMineDef(desc);
    def->field_194 = desc.field_158;
    def->field_198 = desc.field_15C;
    def->field_19C = desc.field_160;
    def->field_1A0 = desc.field_164;
    def->field_1A4 = desc.field_168;
    def->field_1A8 = desc.field_16C;
    def->field_1AC = desc.field_170;
    def->field_1B0 = desc.field_174;
    def->armOnApproach = desc.field_178;
    def->field_1B5 = desc.field_179;
    def->field_1B6 = desc.field_17A;
    if (def->collisionShape)
        RigidBodySetCollisionLayer(def->collisionShape, 23);
    return def;
}

} // namespace entity
} // namespace sly
