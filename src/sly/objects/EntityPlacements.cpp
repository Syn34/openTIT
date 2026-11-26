// openTIT - Placement destructors (see EntityPlacements.h). Each class has its
// own copy of the scripted entity placement's destructor body on the console.
#include "sly/objects/EntityPlacements.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/entity/PropEntities.h"
#include "ape/xml/ApeXml.h"

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

} // namespace objects
} // namespace sly
