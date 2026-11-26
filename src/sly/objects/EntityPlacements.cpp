// openTIT - Placement destructors (see EntityPlacements.h). Each class has its
// own copy of the scripted entity placement's destructor body on the console.
#include "sly/objects/EntityPlacements.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/entity/PropEntities.h"
#include "sly/entity/ActorDefs.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>

namespace sly {
namespace objects {

// @func 0x00739C5C ScriptedEntityPlacement::~ScriptedEntityPlacement
// @func 0x00747430 ScriptedEntityPlacement::~ScriptedEntityPlacement (deleting)
ScriptedEntityPlacement::~ScriptedEntityPlacement() = default;

// @func 0x007469B4 ActorPlacement::~ActorPlacement
// @func 0x007475DC ActorPlacement::~ActorPlacement (deleting)
ActorPlacement::~ActorPlacement() = default;

// @func 0x00746A70 PhysicalEntityPlacement::~PhysicalEntityPlacement
// @func 0x00748164 PhysicalEntityPlacement::~PhysicalEntityPlacement (deleting)
PhysicalEntityPlacement::~PhysicalEntityPlacement() = default;

// @func 0x0074D330 RigidPropPlacement::~RigidPropPlacement
// @func 0x0074EE9C RigidPropPlacement::~RigidPropPlacement (deleting)
RigidPropPlacement::~RigidPropPlacement() = default;

// @func 0x0074DF24 AlterEgoPlacement::~AlterEgoPlacement
// @func 0x0074F208 AlterEgoPlacement::~AlterEgoPlacement (deleting)
AlterEgoPlacement::~AlterEgoPlacement() = default;

// @func 0x0074E610 TargetableBPlacement::~TargetableBPlacement
// @func 0x0074F4EC TargetableBPlacement::~TargetableBPlacement (deleting)
TargetableBPlacement::~TargetableBPlacement() = default;

// @func 0x00739D18 TargetableCPlacement::~TargetableCPlacement
// @func 0x00758174 TargetableCPlacement::~TargetableCPlacement (deleting)
TargetableCPlacement::~TargetableCPlacement() = default;

// @func 0x00755ABC TargetableCLinkedPlacement::~TargetableCLinkedPlacement
// @func 0x00758AB4 TargetableCLinkedPlacement::~TargetableCLinkedPlacement (deleting)
TargetableCLinkedPlacement::~TargetableCLinkedPlacement() = default;

// @func 0x00757A9C RigidEntityB19198Placement::~RigidEntityB19198Placement
// @func 0x00758F1C RigidEntityB19198Placement::~RigidEntityB19198Placement (deleting)
RigidEntityB19198Placement::~RigidEntityB19198Placement() = default;

// @func 0x0075005C ActorGroupPlacement::~ActorGroupPlacement
// @func 0x00751240 ActorGroupPlacement::~ActorGroupPlacement (deleting)
ActorGroupPlacement::~ActorGroupPlacement() = default;

// @func 0x00750118 HitCounterActorPlacement::~HitCounterActorPlacement
// @func 0x007519D0 HitCounterActorPlacement::~HitCounterActorPlacement (deleting)
HitCounterActorPlacement::~HitCounterActorPlacement() = default;

// @func 0x007501D4 SoundActorPlacement::~SoundActorPlacement
// @func 0x0075247C SoundActorPlacement::~SoundActorPlacement (deleting)
SoundActorPlacement::~SoundActorPlacement() = default;

// @func 0x00750290 SignalActorPlacement::~SignalActorPlacement
// @func 0x007525D8 SignalActorPlacement::~SignalActorPlacement (deleting)
SignalActorPlacement::~SignalActorPlacement() = default;

// @func 0x0075034C ActorB15E60Placement::~ActorB15E60Placement
// @func 0x00752BDC ActorB15E60Placement::~ActorB15E60Placement (deleting)
ActorB15E60Placement::~ActorB15E60Placement() = default;

// @func 0x00750988 ScriptObjectActorPlacement::~ScriptObjectActorPlacement
// @func 0x00753B34 ScriptObjectActorPlacement::~ScriptObjectActorPlacement (deleting)
ScriptObjectActorPlacement::~ScriptObjectActorPlacement() = default;

// @func 0x00750A44 SoundActorB16678Placement::~SoundActorB16678Placement
// @func 0x00754138 SoundActorB16678Placement::~SoundActorB16678Placement (deleting)
SoundActorB16678Placement::~SoundActorB16678Placement() = default;

// @func 0x00750B00 SoundActorB168E0Placement::~SoundActorB168E0Placement
// @func 0x007544B8 SoundActorB168E0Placement::~SoundActorB168E0Placement (deleting)
SoundActorB168E0Placement::~SoundActorB168E0Placement() = default;

// @func 0x00750BBC SoundActorB16B48Placement::~SoundActorB16B48Placement
// @func 0x007548E8 SoundActorB16B48Placement::~SoundActorB16B48Placement (deleting)
SoundActorB16B48Placement::~SoundActorB16B48Placement() = default;

// @func 0x00750C78 SoundActorB16DB0Placement::~SoundActorB16DB0Placement
// @func 0x00754AA0 SoundActorB16DB0Placement::~SoundActorB16DB0Placement (deleting)
SoundActorB16DB0Placement::~SoundActorB16DB0Placement() = default;

// @func 0x00750D34 SoundActorB17270Placement::~SoundActorB17270Placement
// @func 0x007550BC SoundActorB17270Placement::~SoundActorB17270Placement (deleting)
SoundActorB17270Placement::~SoundActorB17270Placement() = default;

// @func 0x00750DF0 SoundActorB17728Placement::~SoundActorB17728Placement
// @func 0x007552A0 SoundActorB17728Placement::~SoundActorB17728Placement (deleting)
SoundActorB17728Placement::~SoundActorB17728Placement() = default;

// @func 0x00755590 ActorB17990Placement::~ActorB17990Placement
// @func 0x00756080 ActorB17990Placement::~ActorB17990Placement (deleting)
ActorB17990Placement::~ActorB17990Placement() = default;

// @func 0x0075564C InteractivePropAPlacement::~InteractivePropAPlacement
// @func 0x00756684 InteractivePropAPlacement::~InteractivePropAPlacement (deleting)
InteractivePropAPlacement::~InteractivePropAPlacement() = default;

// @func 0x00755A00 SafePlacement::~SafePlacement
// @func 0x00756C00 SafePlacement::~SafePlacement (deleting)
SafePlacement::~SafePlacement() = default;

// @func 0x00755B78 TweenActorPlacement::~TweenActorPlacement
// @func 0x007574A8 TweenActorPlacement::~TweenActorPlacement (deleting)
TweenActorPlacement::~TweenActorPlacement() = default;

// @func 0x00755F3C InteractivePropBPlacement::~InteractivePropBPlacement
// @func 0x00757604 InteractivePropBPlacement::~InteractivePropBPlacement (deleting)
InteractivePropBPlacement::~InteractivePropBPlacement() = default;

// @func 0x00750408 PlacementB16088::~PlacementB16088
// @func 0x00753158 PlacementB16088::~PlacementB16088 (deleting)
PlacementB16088::~PlacementB16088() = default;

// NOTE: the console reads the definition's pieces without a null check.
// @func 0x0074F680 BreakableChainPlacement::BreakableChainPlacement
BreakableChainPlacement::BreakableChainPlacement(const char* placementName, BreakableChainDef* ownerDef,
                                                 ape::world::ApeObj* source, const ape::Matrix34& placementTransform,
                                                 const ape::Vec3& scale, bool bit31, bool bit30,
                                                 uint32_t field74)
    : EntityPlacement(placementName, ownerDef, source, placementTransform, scale, bit31, bit30, field74)
{
    typeBits |= 0x400;
    if (!ownerDef)
        return;
    for (const BreakableChainPiece& piece : ownerDef->pieces)
        pieces.PushBack(piece);
}

// @func 0x0074F9D8 BreakableChainPlacement::~BreakableChainPlacement
// @func 0x00750F94 BreakableChainPlacement::~BreakableChainPlacement (deleting)
BreakableChainPlacement::~BreakableChainPlacement() = default;

// @func 0x0074FA80 ChainPlacement::ChainPlacement
ChainPlacement::ChainPlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                               const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31,
                               bool bit30, uint32_t field74, uint32_t valueB4)
    : EntityPlacement(placementName, ownerDef, source, placementTransform, scale, bit31, bit30, field74),
      field_B4(valueB4)
{
    typeBits |= 0x800;
}

// @func 0x00750680 RopePlacement::RopePlacement
RopePlacement::RopePlacement(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                             const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31,
                             bool bit30, uint32_t field74, uint32_t valueE0, const ape::Vec3& pointC0,
                             const ape::Vec3& pointD0, uint32_t valueE4, bool flagE8)
    : EntityPlacement(placementName, ownerDef, source, placementTransform, scale, bit31, bit30, field74),
      field_C0(pointC0), field_D0(pointD0), field_E0(valueE0), field_E4(valueE4), field_E8(flagE8)
{
    typeBits |= 0x1000;
}

// @func 0x00760088 PlacementB193A8::PlacementB193A8
PlacementB193A8::PlacementB193A8(const char* placementName, EntityDef* ownerDef,
                                 const ape::Matrix34& placementTransform, bool bit31, uint32_t valueB4,
                                 bool flagB8)
    : EntityPlacement(placementName, ownerDef, nullptr, placementTransform, ape::Vec3{}, bit31, false, 0),
      field_B4(valueB4), field_B8(flagB8)
{
    field_0C |= 0x80;
}

// @func 0x0074FD10 ChainPlacement::~ChainPlacement
// @func 0x00751130 ChainPlacement::~ChainPlacement (deleting)
ChainPlacement::~ChainPlacement() = default;

// @func 0x0073B790 RopePlacement::~RopePlacement
// @func 0x00753A24 RopePlacement::~RopePlacement (deleting)
RopePlacement::~RopePlacement() = default;

// @func 0x007592B8 PlacementB193A8::~PlacementB193A8
// @func 0x007593B0 PlacementB193A8::~PlacementB193A8 (deleting)
PlacementB193A8::~PlacementB193A8() = default;

} // namespace objects
} // namespace sly

namespace sly {
namespace objects {

namespace {

// Attributes and child elements of the scripted placements.
constexpr uint32_t kAttr_Field_DC  = 0xA7D74568u;
constexpr uint32_t kAttr_Field_E0  = 0xF89FDD94u;
constexpr uint32_t kAttr_Field_E4  = 0x7973EFA5u;
constexpr uint32_t kAttr_Field_E8  = 0x2243757Fu;
constexpr uint32_t kElem_BindingsA = 0xC3B709C0u;
constexpr uint32_t kElem_BindingsB = 0xB921AB51u;
constexpr uint32_t kAttr_PhysicalVecA = 0xADDC2CDDu;
constexpr uint32_t kAttr_PhysicalVecB = 0x0948D097u;   // in degrees
constexpr float kDegToRad = 0.017453292f;

// Every binding the element's children describe, up to the list's capacity
// (the console helper 0x0049EF40 on an ApeArray of 0x4C-byte records).
void ParseBindings(ape::xml::XmlElement* elem, ape::ApeTArray<ScriptPropertyOverride>& list)
{
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        ScriptPropertyOverride binding;
        if (!ParseScriptBinding(child, &binding))
            continue;
        if (list.Count() >= list.Capacity())
            return;
        list.PushBack(binding);
    }
}

// Rotation attributes are given in degrees.
void ToRadians(ape::Vec3& angles)
{
    angles.x *= kDegToRad;
    angles.y *= kDegToRad;
    angles.z *= kDegToRad;
}

// Orientation from Euler angles in degrees (half angles through
// FastCos/FastSin; inlined by the console parsers).
ape::Vec4 QuatFromEulerDegrees(const ape::Vec3& euler)
{
    const float hx = euler.x * kDegToRad * 0.5f;
    const float hy = euler.y * kDegToRad * 0.5f;
    const float hz = euler.z * kDegToRad * 0.5f;
    const float cx = ape::FastCos(hx), cy = ape::FastCos(hy), cz = ape::FastCos(hz);
    const float sx = ape::FastSin(hx), sy = ape::FastSin(hy), sz = ape::FastSin(hz);
    ape::Vec4 q;
    q.x = sy * sz * cx + cy * cz * sx;
    q.y = cz * sy * cx + cy * sz * sx;
    q.z = cy * sz * cx - cz * sy * sx;
    q.w = cy * cz * cx - sy * sz * sx;
    return q;
}

// The two vectors every physical placement reads.
void ParsePhysicalSettings(ape::xml::XmlElement* elem, PhysicalSettings& settings)
{
    elem->GetVec3(kAttr_PhysicalVecA, &settings.field_00.x, &settings.field_00.x);
    elem->GetVec3(kAttr_PhysicalVecB, &settings.rotation.x, &settings.rotation.x);
}

} // namespace

// @func 0x000CC188 ScriptedEntityPlacement::ScriptedEntityPlacement
// @func 0x000CC4D4 ScriptedEntityPlacement::ScriptedEntityPlacement (thunk)
ScriptedEntityPlacement::ScriptedEntityPlacement(float defaultField_DC, const char* placementName,
                                                 EntityDef* ownerDef, ape::world::ApeObj* source,
                                                 const ape::Matrix34& placementTransform, const ape::Vec3& scale,
                                                 bool bit31, bool bit30, uint32_t field74)
    : EntityPlacement(placementName, ownerDef, source, placementTransform, scale, bit31, bit30, field74)
{
    // NOTE: the definition is taken to be a scripted one, as on the console.
    const auto* scriptDef = static_cast<const entity::ScriptedEntityDef*>(ownerDef);
    field_DC = scriptDef ? scriptDef->field_8C : defaultField_DC;
    field_E0 = scriptDef ? scriptDef->lifetimeScriptName : 0;
    field_E4 = scriptDef ? scriptDef->field_98 : 0;
    field_E8 = scriptDef ? (scriptDef->damageFlags & 0x8000u) != 0 : true;
    flags |= kPlace_Scripted;
}

// @func 0x000CBE10 ScriptedEntityPlacement::Parse
void ScriptedEntityPlacement::Parse(ape::xml::XmlElement* elem)
{
    elem->GetFloat(kAttr_Field_DC, &field_DC, field_DC);
    elem->GetHash(kAttr_Field_E0, &field_E0, field_E0);
    elem->GetHash(kAttr_Field_E4, &field_E4, field_E4);
    elem->GetBool(kAttr_Field_E8, &field_E8, field_E8);
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() == kElem_BindingsA)
            ParseBindings(child, overridesA);
        else if (child->GetNameHash() == kElem_BindingsB)
            ParseBindings(child, overridesB);
    }
}

// @func 0x000CBF88 PhysicalEntityPlacement::Parse
void PhysicalEntityPlacement::Parse(ape::xml::XmlElement* elem)
{
    ScriptedEntityPlacement::Parse(elem);
    elem->GetVec3(kAttr_PhysicalVecA, &settings.field_00.x, &settings.field_00.x);
    elem->GetVec3(kAttr_PhysicalVecB, &settings.rotation.x, &settings.rotation.x);
    ToRadians(settings.rotation);
}

// @func 0x000CC728 ParseScriptedEntityPlacement
EntityPlacement* ParseScriptedEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new ScriptedEntityPlacement(name, def);
    placement->Parse(elem);
    return placement;
}

// @func 0x000CC510 ParseActorPlacement
EntityPlacement* ParseActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                     EntityDef* def)
{
    (void)root;
    auto* placement = new ActorPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    placement->Parse(elem);
    return placement;
}

// Inline in the placement loader (0x000CC920).
EntityPlacement* ParsePhysicalEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new PhysicalEntityPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Physical;
    placement->Parse(elem);
    return placement;
}

// @func 0x00167F58 ParseRigidPropPlacement
EntityPlacement* ParseRigidPropPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                         EntityDef* def)
{
    (void)root;
    auto* placement = new RigidPropPlacement(name, def);
    placement->flags |= 0x80;
    placement->Parse(elem);
    ParsePhysicalSettings(elem, placement->settings);
    ToRadians(placement->settings.rotation);
    return placement;
}

// @func 0x0016CBE0 ParseCompoundEntityPlacement
EntityPlacement* ParseCompoundEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new CompoundEntityPlacement(name, def);
    placement->flags |= 0x400;
    placement->Parse(elem);
    return placement;
}

// @func 0x00177D78 ParseAlterEgoPlacement
// The definition's flag word (+0x100) bit 31 gives the placement's bit 30.
EntityPlacement* ParseAlterEgoPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                        EntityDef* def)
{
    (void)root;
    auto* placement = new AlterEgoPlacement(name, def);
    if (def) {
        const bool bit = (static_cast<const entity::AlterEgoDef*>(def)->deathFlags & 0x80000000u) != 0;
        placement->bits84 = (placement->bits84 & ~0x40000000u) | (bit ? 0x40000000u : 0u);
    }
    placement->flags |= 0x800;
    placement->Parse(elem);
    ParsePhysicalSettings(elem, placement->settings);
    elem->GetFloat(0x0023FEF4u, &placement->field_118, placement->field_118);
    elem->GetFloat(0xCEDD379Au, &placement->field_11C, placement->field_11C);
    ToRadians(placement->settings.rotation);
    return placement;
}

// @func 0x0017EF48 TargetableAPlacement::Parse
void TargetableAPlacement::Parse(ape::xml::XmlElement* elem)
{
    ScriptedEntityPlacement::Parse(elem);
    ParsePhysicalSettings(elem, settings);
    elem->GetFloat(0xCEDD379Au, &settings.field_20, settings.field_20);
    elem->GetFloat(0x04B70827u, &field_114, field_114);
    elem->GetFloat(0x001FE6E0u, &field_118, field_118);
    elem->GetFloat(0x94E315E2u, &field_11C, field_11C);
    elem->GetFloat(0x53CAD6F8u, &field_124, field_124);
    elem->GetFloat(0x7D4D7117u, &field_120, field_120);
    elem->GetHash(0x6C3F4814u, &field_128, field_128);
    elem->GetFloat(0xD1A77009u, &field_12C, field_12C);
    elem->GetBool(0x07248538u, &field_130, field_130);
    elem->GetHash(0x7DFA87B7u, &field_134, field_134);
    elem->GetHash(0xD49CCAAEu, &field_138, field_138);
    elem->GetBool(0xB8A0F299u, &field_13C, field_13C);
    ToRadians(settings.rotation);
}

// @func 0x0017F1A0 ParseTargetableAPlacement
EntityPlacement* ParseTargetableAPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new TargetableAPlacement(name, def);
    placement->flags |= 0x100;
    placement->Parse(elem);
    return placement;
}

// @func 0x00190C28 TargetableBPlacement::Parse
// NOTE: the console reads attribute 0x7ECB8527 with field_130 as the default.
void TargetableBPlacement::Parse(ape::xml::XmlElement* elem)
{
    ScriptedEntityPlacement::Parse(elem);
    ParsePhysicalSettings(elem, settings);
    elem->GetFloat(0xCEDD379Au, &settings.field_20, settings.field_20);
    elem->GetFloat(0x04B70827u, &field_114, field_114);
    elem->GetFloat(0x9FC855CAu, &field_118, field_118);
    elem->GetFloat(0xB595B151u, &field_11C, field_11C);
    elem->GetBool(0x7883C439u, &field_120, field_120);
    elem->GetHash(0x4578545Fu, &field_124, field_124);
    elem->GetHash(0x00241C53u, &field_128, field_128);
    elem->GetFloat(0x5161B474u, &field_12C, field_12C);
    elem->GetFloat(0x8FA16092u, &field_130, field_130);
    elem->GetFloat(0x7ECB8527u, &field_134, field_130);
    elem->GetFloat(0xDBAF7E49u, &field_138, field_138);
    elem->GetFloat(0x00DE534Eu, &field_13C, field_13C);
    elem->GetBool(0x641217B3u, &field_140, field_140);
    ToRadians(settings.rotation);
}

// @func 0x00190EA0 ParseTargetableBPlacement
EntityPlacement* ParseTargetableBPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new TargetableBPlacement(name, def);
    placement->flags |= 0x200;
    placement->Parse(elem);
    return placement;
}

// @func 0x001AA360 ParseActorGroupPlacement
EntityPlacement* ParseActorGroupPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new ActorGroupPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* groupDef = static_cast<const entity::ActorGroupDef*>(def);
        for (int i = 0; i < 4; ++i)
            placement->field_F0[i] = groupDef->field_130[i];
        placement->field_100 = groupDef->field_140;
    }
    placement->field_0C |= 1;
    placement->Parse(elem);
    elem->GetFloat(0x4E14723Fu, &placement->field_F0[0], placement->field_F0[0]);
    elem->GetFloat(0x6A96C7DBu, &placement->field_F0[1], placement->field_F0[1]);
    elem->GetFloat(0xEAEDFCE1u, &placement->field_F0[2], placement->field_F0[2]);
    elem->GetFloat(0x0A924DA2u, &placement->field_F0[3], placement->field_F0[3]);
    elem->GetBool(0x317F6BB4u, &placement->field_100, placement->field_100);
    return placement;
}

// @func 0x001AB7A0 ParseHitCounterActorPlacement
EntityPlacement* ParseHitCounterActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                               const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new HitCounterActorPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    uint32_t defFlags = 0x80000000u;
    if (def) {
        const auto* counterDef = static_cast<const entity::HitCounterActorDef*>(def);
        for (int i = 0; i < 5; ++i)
            placement->field_F0[i] = counterDef->field_130[i];
        defFlags = counterDef->flags_148;
    }
    placement->flags_104 = (placement->flags_104 & 0x3FFFFFFFu) | (defFlags & 0xC0000000u);
    placement->field_0C |= 1;
    placement->Parse(elem);
    elem->GetFloat(0x8722570Bu, &placement->field_F0[0], placement->field_F0[0]);
    elem->GetFloat(0x1E6104E5u, &placement->field_F0[1], placement->field_F0[1]);
    elem->GetFloat(0x08B6A993u, &placement->field_F0[2], placement->field_F0[2]);
    elem->GetFloat(0x14EE18CDu, &placement->field_F0[3], placement->field_F0[3]);
    elem->GetFloat(0x4E14723Fu, &placement->field_F0[4], placement->field_F0[4]);
    bool bit31 = (placement->flags_104 & 0x80000000u) != 0;
    bool bit30 = (placement->flags_104 & 0x40000000u) != 0;
    elem->GetBool(0x609A61B6u, &bit31, bit31);
    elem->GetBool(0x994327ADu, &bit30, bit30);
    placement->flags_104 = (placement->flags_104 & 0x3FFFFFFFu) | (bit31 ? 0x80000000u : 0u) |
                           (bit30 ? 0x40000000u : 0u);
    return placement;
}

// @func 0x001A7C70 ParseChainPlacement
EntityPlacement* ParseChainPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                     EntityDef* def)
{
    (void)root;
    auto* placement = new ChainPlacement(name, def, nullptr, ape::world::kWorldIdentity,
                                         ape::world::kWorldUnitScale, true, false, 0, 0xFFFFFFFFu);
    int32_t value = int32_t(placement->field_B4);
    elem->GetInt(0xBEC3D7F3u, &value, value);
    placement->field_B4 = uint32_t(value);
    return placement;
}

// @func 0x001BB830 ParseRopePlacement
EntityPlacement* ParseRopePlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                    EntityDef* def)
{
    (void)root;
    auto* placement = new RopePlacement(name, def, nullptr, ape::world::kWorldIdentity, ape::world::kWorldUnitScale,
                                        true, false, 0, 0, ape::world::kWorldZero, ape::world::kWorldZero,
                                        0xFFFFFFFFu, false);
    int32_t value = int32_t(placement->field_E4);
    elem->GetInt(0x803BA106u, &value, value);
    placement->field_E4 = uint32_t(value);
    return placement;
}

// @func 0x001AE130 ParseSoundActorPlacement
EntityPlacement* ParseSoundActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorPlacement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    placement->Parse(elem);
    return placement;
}

void SoundActorPlacement::Parse(ape::xml::XmlElement* elem)
{
    static const uint32_t kSlotAttrs[7] = {0xD5F932F0u, 0xEABE8E36u, 0x800ACF3Au, 0xF9217A56u,
                                           0x7BA17740u, 0xA9262FCFu, 0xA88C3581u};
    ScriptedEntityPlacement::Parse(elem);
    elem->GetFloat(0x7B2F95A0u, &field_10C, field_10C);
    for (int i = 0; i < 7; ++i)
        elem->GetFloat(kSlotAttrs[i], &slotTimes[i], slotTimes[i]);
}

// @func 0x001B0398 ParseSignalActorPlacement
EntityPlacement* ParseSignalActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SignalActorPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    const auto* signalDef = def ? static_cast<const entity::SignalActorDef*>(def) : nullptr;
    placement->field_F0[0] = signalDef ? signalDef->field_15C : 18.849556f;
    placement->field_F0[1] = signalDef ? signalDef->field_160 : 0.08726646f;
    placement->field_F0[2] = signalDef ? signalDef->field_168 : 0.0f;
    const uint32_t defFlags = signalDef ? signalDef->flags_174 : 0x00800000u;
    placement->flags_FC = (placement->flags_FC & 0x3FFFFFFFu) | ((defFlags & 0x00800000u) << 8) |
                          ((defFlags & 0x00400000u) << 8);
    placement->typeBits |= 0x40000;
    placement->Parse(elem);
    // The two angles are given in degrees.
    if (elem->GetFloat(0x26903F9Cu, &placement->field_F0[0], placement->field_F0[0]) == ape::xml::kQuery_Ok)
        placement->field_F0[0] *= kDegToRad;
    if (elem->GetFloat(0xC3017FF2u, &placement->field_F0[1], placement->field_F0[1]) == ape::xml::kQuery_Ok)
        placement->field_F0[1] *= kDegToRad;
    bool bit31 = (placement->flags_FC & 0x80000000u) != 0;
    bool bit30 = (placement->flags_FC & 0x40000000u) != 0;
    elem->GetBool(0x4D97C801u, &bit31, bit31);
    elem->GetBool(0x3AF62E9Eu, &bit30, bit30);
    placement->flags_FC = (placement->flags_FC & 0x3FFFFFFFu) | (bit31 ? 0x80000000u : 0u) | (bit30 ? 0x40000000u : 0u);
    elem->GetFloat(0xAA8C2237u, &placement->field_F0[2], placement->field_F0[2]);
    return placement;
}

// @func 0x001B2718 ParseActorB15E60Placement
// The first four values are not negative, and the first is at most the second.
EntityPlacement* ParseActorB15E60Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def)
{
    static const uint32_t kAttrs[5] = {0xCEA8E637u, 0x06CD2139u, 0x0DE78502u, 0x0B0BD5E6u, 0x285C1380u};
    static const float kDefaults[5] = {1.0f, 2.0f, 1.0f, 1.0f, 0.0f};
    (void)root;
    auto* placement = new ActorB15E60Placement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    for (int i = 0; i < 5; ++i)
        placement->field_F0[i] = def ? static_cast<const entity::ActorB15E60Def*>(def)->field_13C[i] : kDefaults[i];
    placement->typeBits |= 0x10000;
    placement->Parse(elem);
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(kAttrs[i], &placement->field_F0[i], placement->field_F0[i]);
    for (int i = 0; i < 4; ++i)
        placement->field_F0[i] = std::max(placement->field_F0[i], 0.0f);
    if (placement->field_F0[1] < placement->field_F0[0])
        placement->field_F0[0] = placement->field_F0[1];
    return placement;
}

// @func 0x001BDDC0 ParseScriptObjectActorPlacement
EntityPlacement* ParseScriptObjectActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                 const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new ScriptObjectActorPlacement(name, def);
    const auto* objectDef = def ? static_cast<const entity::ScriptObjectActorDef*>(def) : nullptr;
    const ape::Vec3& vec = objectDef ? objectDef->field_140 : ape::world::kWorldZero;
    placement->field_F0[0] = vec.x;
    placement->field_F0[1] = vec.y;
    placement->field_F0[2] = vec.z;
    placement->field_100 = objectDef ? objectDef->field_130 : 0.0f;
    placement->typeBits |= 0x200;
    placement->Parse(elem);
    elem->GetVec3(0x7DDB2524u, placement->field_F0, placement->field_F0);
    elem->GetFloat(0x7DDB32EAu, &placement->field_100, placement->field_100);
    elem->GetFloat(0xE14281CCu, &placement->field_104, placement->field_104);
    elem->GetBool(0x7D0D427Bu, &placement->field_108[0], placement->field_108[0]);
    elem->GetBool(0xE019565Du, &placement->field_108[1], placement->field_108[1]);
    elem->GetBool(0x846B9DA2u, &placement->field_108[2], placement->field_108[2]);
    return placement;
}

// @func 0x001BF2A0 SoundActorB16678Placement::Parse
// The switch state is named ("off" / "on"; anything else is "off").
void SoundActorB16678Placement::Parse(ape::xml::XmlElement* elem)
{
    static const uint32_t kStates[2] = {0x0001314Fu /* "off" */, 0x000009DFu /* "on" */};
    SoundActorPlacement::Parse(elem);
    uint32_t state = 0;
    elem->GetHash(0xC03F596Fu, &state, 0);
    switchState = 0;
    for (uint32_t i = 0; i < 2; ++i) {
        if (kStates[i] == state) {
            switchState = i;
            break;
        }
    }
    elem->GetFloat(0xA6710C18u, &field_114, field_114);
    elem->GetBool(0xD28465E5u, &field_118[0], field_118[0]);
    elem->GetBool(0xF0169515u, &field_118[1], field_118[1]);
}

// @func 0x001BF4E0 ParseSoundActorB16678Placement
EntityPlacement* ParseSoundActorB16678Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB16678Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    const auto* switchDef = def ? static_cast<const entity::SoundActorB16678Def*>(def) : nullptr;
    placement->switchState = switchDef ? switchDef->field_190 : 0;
    placement->field_114 = switchDef ? switchDef->field_194 : -1.0f;
    placement->field_118[0] = switchDef && switchDef->field_198;
    placement->field_118[1] = switchDef && switchDef->field_199;
    placement->typeBits |= 0x100000;
    placement->Parse(elem);
    return placement;
}

// @func 0x001C0C64 SoundActorB168E0Placement::Parse
void SoundActorB168E0Placement::Parse(ape::xml::XmlElement* elem)
{
    SoundActorPlacement::Parse(elem);
    elem->GetFloat(0x08C08CA8u, &field_110, field_110);
    bool bit31 = (flags_114 & 0x80000000u) != 0;
    elem->GetBool(0xD28465E5u, &bit31, bit31);
    flags_114 = (flags_114 & 0x7FFFFFFFu) | (bit31 ? 0x80000000u : 0u);
    bool bit30 = (flags_114 & 0x40000000u) != 0;
    elem->GetBool(0xD02B978Au, &bit30, bit30);
    flags_114 = (flags_114 & 0xBFFFFFFFu) | (bit30 ? 0x40000000u : 0u);
}

// @func 0x001C0E70 ParseSoundActorB168E0Placement
EntityPlacement* ParseSoundActorB168E0Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB168E0Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    const auto* soundDef = def ? static_cast<const entity::SoundActorB168E0Def*>(def) : nullptr;
    placement->field_110 = soundDef ? soundDef->field_198 : 0.0f;
    const uint32_t defFlags = soundDef ? soundDef->flags_19C : 0x40000000u;
    placement->flags_114 = (placement->flags_114 & 0x3FFFFFFFu) | (defFlags & 0xC0000000u);
    placement->typeBits |= 0x200000;
    placement->Parse(elem);
    return placement;
}

// @func 0x001C3040 ParseSoundActorB16B48Placement
EntityPlacement* ParseSoundActorB16B48Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB16B48Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    placement->field_110 = def ? static_cast<const entity::SoundActorB16B48Def*>(def)->field_1B8 : 0.0f;
    placement->typeBits |= 0x1000000;
    placement->Parse(elem);
    elem->GetFloat(0x2DF33CE3u, &placement->field_110, placement->field_110);
    return placement;
}

// @func 0x001C6838 SoundActorB16DB0Placement::Parse
// At most six records are read; the orientation is built from Euler angles
// in degrees (half angles through FastCos/FastSin).
void SoundActorB16DB0Placement::Parse(ape::xml::XmlElement* elem)
{
    constexpr uint32_t kElem_Record = 0xEB9EE198u;
    SoundActorPlacement::Parse(elem);
    elem->GetHash(0x2DF2E2B5u, &field_110, field_110);
    elem->GetHash(0x53FFAFC2u, &field_114, field_114);
    elem->GetFloat(0x667AB4F5u, &field_118, field_118);
    elem->GetFloat(0x7A392528u, &field_11C, field_11C);
    bool bit30 = (flags_300 & 0x40000000u) != 0;
    elem->GetBool(0xA8DB95B1u, &bit30, bit30);
    flags_300 = (flags_300 & 0xBFFFFFFFu) | (bit30 ? 0x40000000u : 0u);

    uint32_t count = 0;
    for (ape::xml::XmlElement* child = elem->FirstChildElement(kElem_Record); child;
         child = child->NextSiblingElement(kElem_Record), ++count) {
        if (count >= 6)
            continue;
        SoundActorB16DB0Record& record = records.records[count];
        child->GetHash(0x75849CFEu, &record.name, record.name);
        child->GetVec3(0x2B7B3EB4u, &record.field_10.x, &record.field_10.x);
        child->GetVec3(0x313AB0C1u, &record.field_20.x, &record.field_20.x);
        ape::Vec3 euler{};
        if (child->GetVec3(0x313AB844u, &euler.x, &ape::world::kWorldZero.x) == ape::xml::kQuery_Ok)
            record.rotation = QuatFromEulerDegrees(euler);
        child->GetHash(0x632DA119u, &record.field_40, record.field_40);
        child->GetBool(0x8141EC8Au, &record.field_44, record.field_44);
        child->GetBool(0xF8E455ACu, &record.field_45, record.field_45);
    }
    flags_300 = (flags_300 & 0x7FFFFFFFu) | (count != 0 ? 0x80000000u : 0u);
}

// @func 0x001C6D30 ParseSoundActorB16DB0Placement
EntityPlacement* ParseSoundActorB16DB0Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB16DB0Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* trackDef = static_cast<const entity::SoundActorB16DB0Def*>(def);
        placement->field_110 = trackDef->field_1AC;
        placement->field_114 = trackDef->field_1B0;
        placement->field_118 = trackDef->field_1B4;
        placement->field_11C = trackDef->field_1B8;
        placement->flags_300 = (placement->flags_300 & 0x3FFFFFFFu) | ((trackDef->options & 0x00400000u) << 8);
    } else {
        placement->flags_300 &= 0x3FFFFFFFu;
    }
    placement->typeBits |= 0x400000;
    placement->Parse(elem);
    return placement;
}

// @func 0x001CB740 SoundActorB17270Placement::Parse
void SoundActorB17270Placement::Parse(ape::xml::XmlElement* elem)
{
    SoundActorPlacement::Parse(elem);
    elem->GetHash(0x53FFAFC2u, &field_110, field_110);
    elem->GetFloat(0x09198A77u, &field_114[0], field_114[0]);
    elem->GetFloat(0x58E70C1Bu, &field_114[1], field_114[1]);
    static const uint32_t kClamped[3] = {0x0B109E97u, 0x0B10795Bu, 0x0B13D227u};
    for (int i = 0; i < 3; ++i) {
        float& value = field_114[2 + i];
        if (elem->GetFloat(kClamped[i], &value, value) == ape::xml::kQuery_Ok)
            value = std::max(value, 0.0f);
    }
    elem->GetFloat(0x667AB4F5u, &field_114[5], field_114[5]);
    elem->GetFloat(0xDBA26CDAu, &field_114[6], field_114[6]);
    elem->GetInt(0x9F5A4080u, &field_130, field_130);
    elem->GetBool(0x3EFBF433u, &field_134[0], field_134[0]);
    elem->GetBool(0x4128CDCDu, &field_134[1], field_134[1]);
}

// @func 0x001CBA90 ParseSoundActorB17270Placement
EntityPlacement* ParseSoundActorB17270Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB17270Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* turretDef = static_cast<const entity::SoundActorB17270Def*>(def);
        placement->field_110 = turretDef->field_1B4;
        for (int i = 0; i < 7; ++i)
            placement->field_114[i] = turretDef->field_1B8[i];
        placement->field_130 = turretDef->field_1E0;
        placement->field_134[0] = turretDef->field_1E4;
        placement->field_134[1] = turretDef->field_1E5;
    }
    placement->typeBits |= 0x2000000;
    placement->Parse(elem);
    return placement;
}

// @func 0x001D4328 ParseSoundActorB17728Placement
EntityPlacement* ParseSoundActorB17728Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new SoundActorB17728Placement(name, def);
    placement->typeBits |= 0x80000;
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* doorDef = static_cast<const entity::SoundActorB17728Def*>(def);
        placement->field_110 = doorDef->field_190;
        placement->field_114 = doorDef->field_194;
    }
    placement->typeBits |= 0x4000000;
    placement->Parse(elem);
    elem->GetHash(0x53FFAFC2u, &placement->field_110, placement->field_110);
    elem->GetFloat(0x667AB4F5u, &placement->field_114, placement->field_114);
    return placement;
}

// @func 0x001DE9D8 ParseInteractivePropAPlacement
EntityPlacement* ParseInteractivePropAPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new InteractivePropAPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* propDef = static_cast<const entity::InteractivePropADef*>(def);
        placement->field_F0[0] = propDef->field_278[0];
        placement->field_F0[1] = propDef->field_278[1];
    }
    placement->typeBits |= 0x40000000;
    placement->Parse(elem);
    if (elem->GetVec2(0xBF3F0DB1u, placement->field_F0, placement->field_F0) == ape::xml::kQuery_Ok) {
        placement->field_F0[0] *= kDegToRad;
        placement->field_F0[1] *= kDegToRad;
    }
    return placement;
}

// @func 0x001E6D78 ParseSafePlacement
EntityPlacement* ParseSafePlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const char* name,
                                    EntityDef* def)
{
    (void)root;
    auto* placement = new SafePlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* safeDef = static_cast<const entity::SafeDef*>(def);
        placement->field_F0 = safeDef->field_1B0;
        for (int i = 0; i < 3; ++i)
            placement->field_100[i] = safeDef->field_1C0[i];
        placement->field_110 = safeDef->field_1D0;
    }
    placement->typeBits |= 0x10000000;
    placement->Parse(elem);
    elem->GetHash(0x06556689u, &placement->field_F0, placement->field_F0);
    elem->GetVec3(0xC88E4511u, placement->field_100, placement->field_100);
    elem->GetHash(0x675C8395u, &placement->field_110, placement->field_110);
    return placement;
}

// @func 0x001ED004 TweenActorPlacement::Parse
// NOTE: attribute 0xBE5301F3 defaults to a global of the game (0x01053BA8)
// whose value is not known; 0 is assumed.
void TweenActorPlacement::Parse(ape::xml::XmlElement* elem)
{
    static const uint32_t kNameAttrs[7] = {0x161851F3u, 0xEFE3F2BEu, 0x80F96352u, 0x93116591u,
                                           0x3F7939FEu, 0x04623C7Cu, 0xDEC8B85Du};
    static const uint32_t kFlagAttrs[5] = {0x6C2C79ECu, 0x33DA09EFu, 0xF9C0C1B5u, 0x1E85A989u, 0xD6F80AE3u};
    constexpr float kField128Default = 0.0f;
    ScriptedEntityPlacement::Parse(elem);
    elem->GetVec2(0x64231FD8u, field_F0, field_F0);
    elem->GetVec2(0x64231EEAu, field_F8, field_F8);
    for (int i = 0; i < 7; ++i)
        elem->GetHash(kNameAttrs[i], &names[i], names[i]);
    elem->GetHash(0x6350BDB4u, &names[8], names[8]);
    elem->GetFloat(0xC6BA9BDEu, &field_124, field_124);
    for (int i = 0; i < 5; ++i)
        elem->GetBool(kFlagAttrs[i], &field_134[i], field_134[i]);
    elem->GetFloat(0xBE5301F3u, &field_128, kField128Default);
    elem->GetFloat(0x48CDF4C0u, &field_12C, field_12C);
    elem->GetBool(0x850F9FE3u, &field_134[5], field_134[5]);
    elem->GetFloat(0x81B5B597u, &field_130, field_130);
    elem->GetHash(0xF24070D0u, &names[7], names[7]);
    elem->GetBool(0x4DE83B5Au, &field_134[6], field_134[6]);
    elem->GetBool(0x6BDE6A90u, &field_134[7], field_134[7]);
}

// @func 0x001ED358 ParseTweenActorPlacement
// NOTE: the console reads the definition without checking it for null.
EntityPlacement* ParseTweenActorPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                          const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new TweenActorPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    if (def) {
        const auto* tweenDef = static_cast<const entity::TweenActorDef*>(def);
        placement->names[0] = tweenDef->field_134;
        placement->names[1] = tweenDef->field_150;
        placement->names[2] = tweenDef->field_154;
        placement->names[5] = tweenDef->field_1A4;
        placement->names[8] = tweenDef->field_1A0;
        placement->field_130 = tweenDef->field_1A8;
    }
    placement->field_0C |= 8;
    placement->Parse(elem);
    return placement;
}

// @func 0x001F5E50 InteractivePropBPlacement::Parse
void InteractivePropBPlacement::Parse(ape::xml::XmlElement* elem)
{
    ScriptedEntityPlacement::Parse(elem);
    elem->GetHash(0x2899600Fu, &field_F0, field_F0);
    elem->GetVec3(0xEF4DDD13u, &field_100.x, &field_100.x);
    ape::Vec3 euler = ape::world::kWorldZero;
    elem->GetVec3(0xC7400937u, &euler.x, &euler.x);
    rotation = QuatFromEulerDegrees(euler);
    elem->GetFloat(0xDD61F47Cu, &field_120, field_120);
    elem->GetFloat(0x3CC6000Du, &field_124, field_124);
    elem->GetFloat(0x6AA47C5Au, &field_128, field_128);
    elem->GetHash(0x6AA4B8C7u, &field_12C, field_12C);
    elem->GetBool(0xEA3BBF97u, &field_130, field_130);
}

// @func 0x001F60D8 ParseInteractivePropBPlacement
EntityPlacement* ParseInteractivePropBPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new InteractivePropBPlacement(name, def);
    placement->flags |= EntityPlacement::kPlace_Actor;
    placement->field_0C |= 0x10;
    placement->Parse(elem);
    return placement;
}

// @func 0x001FAB38 ParseTargetableCPlacement
// The definition's hitFlags bit 31 gives the placement's bit 30.
EntityPlacement* ParseTargetableCPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                           const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new TargetableCPlacement(name, def);
    if (def) {
        const bool bit = (static_cast<const entity::TargetableDefC*>(def)->hitFlags & 0x80000000u) != 0;
        placement->bits84 = (placement->bits84 & ~0x40000000u) | (bit ? 0x40000000u : 0u);
    }
    placement->flags |= 0x20;
    placement->Parse(elem);
    ParsePhysicalSettings(elem, placement->settings);
    elem->GetFloat(0x0023FEF4u, &placement->field_114, placement->field_114);
    elem->GetFloat(0xCEDD379Au, &placement->field_118, placement->field_118);
    ToRadians(placement->settings.rotation);
    return placement;
}

// @func 0x002048A0 ParseRigidEntityB19198Placement
EntityPlacement* ParseRigidEntityB19198Placement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                 const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new RigidEntityB19198Placement(name, def);
    placement->flags |= 0x1000;
    placement->Parse(elem);
    ParsePhysicalSettings(elem, placement->settings);
    ToRadians(placement->settings.rotation);
    elem->GetBool(0x926425E9u, &placement->field_118[0], placement->field_118[0]);
    elem->GetBool(0x7F6CE437u, &placement->field_118[1], placement->field_118[1]);
    elem->GetBool(0x918F1125u, &placement->field_118[2], placement->field_118[2]);
    return placement;
}

} // namespace objects
} // namespace sly

namespace sly {
namespace objects {

// @func 0x000CBB40 PlacementB138E8::PlacementB138E8
PlacementB138E8::PlacementB138E8(const char* placementName, EntityDef* ownerDef, ape::world::ApeObj* source,
                                 const ape::Matrix34& placementTransform, const ape::Vec3& scale, bool bit31,
                                 bool bit30, uint32_t field74)
    : EntityPlacement(placementName, ownerDef, source, placementTransform, scale, bit31, bit30, field74)
{
    // NOTE: the definition is taken to be a scripted one, as on the console.
    field_C8 = ownerDef ? static_cast<const entity::ScriptedEntityDef*>(ownerDef)->field_B4 : 0;
    field_0C |= 0x40;
}

// @func 0x007468F8 PlacementB138E8::~PlacementB138E8
// @func 0x007470CC PlacementB138E8::~PlacementB138E8 (deleting)
PlacementB138E8::~PlacementB138E8() = default;

// @func 0x000DE454 PlacementB138E8::FillInstanceDesc
void PlacementB138E8::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<InstanceDescB138E8&>(baseDesc);
    desc.field_A4 = field_C8;
    for (const ScriptPropertyOverride& binding : bindings)
        desc.bindings.PushBack(binding);
}

// @func 0x001B5438 PlacementB16088::Parse
void PlacementB16088::Parse(ape::xml::XmlElement* elem)
{
    constexpr uint32_t kElem_Point = 0x65AF9A49u;
    ScriptedEntityPlacement::Parse(elem);
    elem->GetFloat(0xE6DF8EF6u, &field_100, field_100);
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != kElem_Point || field_EC.Count() >= 4)
            continue;
        Entry point = {{0.0f, 0.0f}};
        if (child->GetVec2(0x01968AD4u, point.xy, point.xy) == ape::xml::kQuery_Ok)
            field_EC.PushBack(point);
    }
}

// @func 0x001B55C8 ParsePushBlockPlacement
// NOTE: the console copies the definition's points without checking it for null.
EntityPlacement* ParsePushBlockPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                         const char* name, EntityDef* def)
{
    (void)root;
    auto* placement = new PlacementB16088(name, def);
    if (def) {
        const auto* blockDef = static_cast<const entity::PushBlockDef*>(def);
        placement->field_100 = blockDef->field_E8;
        for (const entity::PushBlockDef::Point& point : blockDef->points)
            placement->field_EC.PushBack(point);
    }
    placement->typeBits |= 0x20000;
    placement->Parse(elem);
    return placement;
}

// @func 0x001A5C90 ParseBreakableChainPlacement
// A piece count (attribute 0x2086522B) replaces the definition's pieces with
// that many pieces of 3.0; piece children (0x048992CE, 1-based index
// attribute 0x042930B2) override a piece's three values (not below zero).
// NOTE: the console tests the placement element's own name instead of the
// child's when looking for piece children (kept), so they are normally
// ignored.
EntityPlacement* ParseBreakableChainPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                              const char* name, EntityDef* def)
{
    constexpr uint32_t kElem_Piece = 0x048992CEu;
    static const uint32_t kValueAttrs[3] = {0xEABE8E36u, 0x800ACF3Au, 0xA88C3581u};
    (void)root;
    auto* placement = new BreakableChainPlacement(name, static_cast<BreakableChainDef*>(def), nullptr,
                                                  ape::world::kWorldIdentity, ape::world::kWorldUnitScale, true,
                                                  false, 0);
    int32_t count = 0;
    if (elem->GetInt(0x2086522Bu, &count, 0) == ape::xml::kQuery_Ok && count > 0) {
        placement->pieces.Clear();
        for (int32_t i = 0; i < count; ++i)
            placement->pieces.PushBack(BreakableChainPiece{{3.0f, 3.0f, 3.0f}});
    }
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (elem->GetNameHash() != kElem_Piece)
            continue;
        int32_t index = 0;
        if (child->GetInt(0x042930B2u, &index, 0) != ape::xml::kQuery_Ok)
            continue;
        if (index < 1 || index > int32_t(placement->pieces.Count()))
            continue;   // warning "BreakableChain Piece index is out of range" (stripped)
        BreakableChainPiece& piece = placement->pieces[size_t(index - 1)];
        for (int k = 0; k < 3; ++k) {
            float value = 0.0f;
            if (child->GetFloat(kValueAttrs[k], &value, piece.values[k]) == ape::xml::kQuery_Ok)
                piece.values[k] = std::max(value, 0.0f);
        }
    }
    return placement;
}

} // namespace objects
} // namespace sly
