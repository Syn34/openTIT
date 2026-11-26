// openTIT - The placement loader: one placement per entity element of a
// level file, built by the loader of the element's class.
#include "sly/objects/EntityPlacements.h"
#include "sly/objects/Trigger.h"
#include "sly/objects/TriggerCamera.h"
#include "sly/objects/TriggerEnv.h"
#include "sly/objects/TriggerFx.h"
#include "sly/objects/TriggerTypes.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace objects {

// TriggerDeath.h and TriggerEnv.h cannot both be included (their attribute
// constants clash); the death trigger loader is declared here.
EntityPlacement* ParseDeathTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                            const char* name);

namespace {

// The common attributes every placement element may carry.
constexpr uint32_t kAttr_Field7C   = 0x465FBA18u;
constexpr uint32_t kAttr_Field80   = 0x01B6B516u;
constexpr uint32_t kAttr_Bit29     = 0x1E0C8C0Cu;
constexpr uint32_t kElem_Attachments = 0xCDA892E2u;
constexpr uint32_t kElem_Bindings  = 0xC3B709C0u;

// Placement loaders not ported yet.
// TODO(0x001A5C90 ParseBreakableChainPlacement)
EntityPlacement* ParseBreakableChainPlacement(ape::xml::XmlElement*, ape::xml::XmlElement*, const char*, EntityDef*)
{
    return nullptr;
}
// TODO(0x001D7D98 ParseActorB17990Placement)
EntityPlacement* ParseActorB17990Placement(ape::xml::XmlElement*, ape::xml::XmlElement*, const char*, EntityDef*)
{
    return nullptr;
}
// TODO(0x000C9AB8 ParseAttachmentRecords): the attachment records of a
// placement (0xC0 bytes each, children 0xCCEB9AF9, nested).
void ParseAttachmentRecords(ape::xml::XmlElement*, ape::xml::XmlElement*, EntityAttachmentList*&) {}

// Inline in the loader (element 0xEBE54755).
EntityPlacement* ParsePlacementB138E8(ape::xml::XmlElement* elem, const char* name, EntityDef* def)
{
    auto* placement = new PlacementB138E8(name, def, nullptr, ape::world::kWorldIdentity,
                                          ape::world::kWorldUnitScale, true, false);
    elem->GetHash(0xF89FDD94u, &placement->field_C8, placement->field_C8);
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() != kElem_Bindings)
            continue;
        for (ape::xml::XmlElement* item = child->FirstChildElement(); item; item = item->NextSiblingElement()) {
            ScriptPropertyOverride binding;
            if (!ParseScriptBinding(item, &binding))
                continue;
            if (placement->bindings.Count() >= placement->bindings.Capacity())
                break;
            placement->bindings.PushBack(binding);
        }
    }
    return placement;
}

using ClassLoader = EntityPlacement* (*)(ape::xml::XmlElement*, ape::xml::XmlElement*, const char*, EntityDef*);
using TriggerLoader = EntityPlacement* (*)(ape::xml::XmlElement*, ape::xml::XmlElement*, const char*);

// Which definition word a loader checks before taking the associated
// definition as its own (EntityDef classBits / typeBits / registryFlags).
enum DefWord { kWord_Class, kWord_Type, kWord_Registry };

struct ClassEntry {
    uint32_t element;
    ClassLoader load;
    DefWord word;
    uint32_t mask;
};

// Loaders of the entity classes, by element name hash.
const ClassEntry kClassLoaders[] = {
    {0x942B64F0u, ParseSoundActorB17270Placement, kWord_Type, 0x2000000},
    {0x93DEF0C4u, ParseActorPlacement, kWord_Class, 0x4},
    {0xB47D3A95u, ParseActorPlacement, kWord_Class, 0x4},
    {0x044344C9u, ParseActorPlacement, kWord_Class, 0x4},
    {0x3536D734u, ParseActorPlacement, kWord_Class, 0x4},
    {0x6F429D34u, ParseActorPlacement, kWord_Class, 0x4},
    {0x906A55EAu, ParseSignalActorPlacement, kWord_Type, 0x40000},
    {0x8CD0E8E8u, ParseActorB15E60Placement, kWord_Type, 0x10000},
    {0xB2221492u, ParseHitCounterActorPlacement, kWord_Type, 0x4000},
    {0xAEF4C388u, ParseBreakableChainPlacement, kWord_Type, 0x400},
    {0xA7CFB837u, ParseCompoundEntityPlacement, kWord_Class, 0x400},
    {0xA77EB5E9u, ParsePhysicalEntityPlacement, kWord_Class, 0x2000},
    {0xB9F78EFDu, ParsePhysicalEntityPlacement, kWord_Class, 0x2000},
    {0xCCFB1F40u, ParseTargetableBPlacement, kWord_Class, 0x200},
    {0xBC6FA23Bu, ParseSoundActorB16DB0Placement, kWord_Type, 0x400000},
    {0xB3748711u, ParseInteractivePropAPlacement, kWord_Type, 0x40000000},
    {0xB324BD49u, ParseScriptedEntityPlacement, kWord_Class, 0x2},
    {0x00227313u, ParseTargetableCPlacement, kWord_Class, 0x20},
    {0x35DF6E97u, ParseTargetableCPlacement, kWord_Class, 0x20},
    {0xEE9D8477u, ParseSoundActorPlacement, kWord_Type, 0x80000},
    {0x7CDC084Cu, ParseSoundActorPlacement, kWord_Type, 0x80000},
    {0xDF511592u, ParseSoundActorB16678Placement, kWord_Type, 0x100000},
    {0x0E5B671Du, ParseRigidEntityB19198Placement, kWord_Class, 0x1000},
    {0x04B6FABDu, ParseRigidPropPlacement, kWord_Class, 0x80},
    {0x03D1DEA1u, ParseChainPlacement, kWord_Type, 0x800},
    {0x0026B78Du, ParseSafePlacement, kWord_Type, 0x10000000},
    {0x002678F2u, ParseRopePlacement, kWord_Type, 0x1000},
    {0x55B67D7Cu, ParseTweenActorPlacement, kWord_Registry, 0x8},
    {0x554FC0D9u, ParseScriptObjectActorPlacement, kWord_Type, 0x200},
    {0x3F0EA991u, ParseAlterEgoPlacement, kWord_Class, 0x800},
    {0x24A57117u, ParseSoundActorB168E0Placement, kWord_Type, 0x200000},
    {0x2190D8D7u, ParseSoundActorB16B48Placement, kWord_Type, 0x1000000},
    {0x201FA54Fu, ParseActorGroupPlacement, kWord_Type, 0x2000},
    {0x70542137u, ParseActorB17990Placement, kWord_Registry, 0x1},
    {0x6927A078u, ParseInteractivePropBPlacement, kWord_Registry, 0x10},
    {0x633A7385u, ParseSoundActorB17728Placement, kWord_Type, 0x4000000},
    {0x786E8593u, ParsePushBlockPlacement, kWord_Type, 0x20000},
    {0x71FE40F4u, ParseTargetableAPlacement, kWord_Class, 0x100},
};

struct TriggerEntry {
    uint32_t element;
    TriggerLoader load;
};

// Trigger volumes: their loaders take no definition.
const TriggerEntry kTriggerLoaders[] = {
    {0x9F7993AFu, ParseFilteredTriggerPlacement},
    {0xA0C4F7BCu, ParseDeathTriggerPlacement},
    {0x6E2FB705u, ParseJobTriggerPlacement},
    {0x7F6CE46Au, ParseFxTriggerPlacement},
    {0x77A6CF07u, ParseDamageTriggerPlacement},
    {0x75F206BDu, ParseCameraTriggerPlacement},
    // The plain trigger loader serves every other trigger element.
    {0xA09DEEA4u, ParseTriggerPlacement},
    {0x8782E7E5u, ParseTriggerPlacement},
    {0xA5900924u, ParseTriggerPlacement},
    {0xA0BCC96Fu, ParseTriggerPlacement},
    {0xDCC028E7u, ParseTriggerPlacement},
    {0xFDC69C0Fu, ParseTriggerPlacement},
    {0xEB9EE198u, ParseTriggerPlacement},
    {0xE65DDB0Fu, ParseTriggerPlacement},
    {0x184A4683u, ParseTriggerPlacement},
    {0x57CCFDA0u, ParseTriggerPlacement},
    {0x5697EEFBu, ParseTriggerPlacement},
    {0x707C673Cu, ParseTriggerPlacement},
};

uint32_t DefWordOf(const EntityDef* def, DefWord word)
{
    switch (word) {
    case kWord_Class: return def->classBits;
    case kWord_Type: return def->typeBits;
    case kWord_Registry: return def->registryFlags;
    }
    return 0;
}

} // namespace

// @func 0x000CC920 LoadEntityPlacement
EntityPlacement* LoadEntityPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem)
{
    // A warning with the document path is printed for each failure (stripped).
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    const uint32_t nameHash = ape::HashNameNoCase(name);
    if (entity::FindEntityPlacement(nameHash))
        return nullptr;

    EntityDef* associated = nullptr;
    const char* defName = elem->GetString("associatedDef");
    if (defName && defName[0] != '\0')
        associated = entity::FindEntityDef(ape::HashNameNoCase(defName));

    const uint32_t element = elem->GetNameHash();
    EntityPlacement* placement = nullptr;
    bool known = false;
    for (const ClassEntry& entry : kClassLoaders) {
        if (entry.element != element)
            continue;
        EntityDef* def = (associated && (DefWordOf(associated, entry.word) & entry.mask)) ? associated : nullptr;
        placement = entry.load(root, elem, name, def);
        known = true;
        break;
    }
    if (!known) {
        for (const TriggerEntry& entry : kTriggerLoaders) {
            if (entry.element != element)
                continue;
            placement = entry.load(root, elem, name);
            known = true;
            break;
        }
    }
    if (!known && element == 0xEBE54755u) {
        EntityDef* def = (associated && (associated->registryFlags & 0x40)) ? associated : nullptr;
        placement = ParsePlacementB138E8(elem, name, def);
        known = true;
    }
    if (!known)
        return nullptr;   // unknown element (warning stripped)

    if (placement) {
        elem->GetFloat(kAttr_Field7C, &placement->field_7C, placement->field_7C);
        elem->GetFloat(kAttr_Field80, &placement->field_80, placement->field_80);
        bool bit29 = (placement->bits84 & 0x20000000u) != 0;
        elem->GetBool(kAttr_Bit29, &bit29, bit29);
        placement->bits84 = (placement->bits84 & ~0x20000000u) | (bit29 ? 0x20000000u : 0u);
        entity::RegisterEntityPlacement(nameHash, placement);
        if (ape::xml::XmlElement* attachments = elem->FirstChildElement(kElem_Attachments))
            ParseAttachmentRecords(root, attachments, placement->attachments);
    }
    return placement;
}

} // namespace objects
} // namespace sly
