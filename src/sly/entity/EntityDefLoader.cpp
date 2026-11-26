// openTIT - The entity definition loader: one child element of an entity
// file's definition list becomes one EntityDef of the class the element
// names.
//
// The loader reads the attributes every definition shares into an
// EntityDefDesc (taking the defaults from a "from" parent definition) and
// hands it to the factory of the element's class. Each factory builds its own
// description from it, parses its own attributes and constructs the
// definition. Factories not ported yet are listed with their console address
// and leave the definition out.
#include "sly/entity/ActorEntities.h"
#include "sly/entity/EntityDef.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PropEntities.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/xml/ApeXml.h"

namespace sly {

// The trigger factories (sly/objects; their headers define overlapping
// attribute constants, so they are declared here).
namespace objects {
#define DECLARE_DEF_FACTORY(name)                                                                                 \
    entity::EntityDef* name(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,                               \
                            const entity::EntityDefDesc& loader, const entity::EntityDef* parent)
DECLARE_DEF_FACTORY(CreateTriggerDef);
DECLARE_DEF_FACTORY(CreateCrawlTriggerDef);
DECLARE_DEF_FACTORY(CreateAlertTriggerDef);
DECLARE_DEF_FACTORY(CreateFilteredTriggerDef);
DECLARE_DEF_FACTORY(CreateUpdateEnableTriggerDef);
DECLARE_DEF_FACTORY(CreateJumpDirTriggerDef);
DECLARE_DEF_FACTORY(CreateJobTriggerDef);
DECLARE_DEF_FACTORY(CreateDeathTriggerDef);
DECLARE_DEF_FACTORY(CreateAmbientSoundTriggerDef);
DECLARE_DEF_FACTORY(CreateSoundZoneTriggerDef);
DECLARE_DEF_FACTORY(CreateWindTriggerDef);
DECLARE_DEF_FACTORY(CreateDamageTriggerDef);
DECLARE_DEF_FACTORY(CreatePushBlockTriggerDef);
DECLARE_DEF_FACTORY(CreateFxTriggerDef);
DECLARE_DEF_FACTORY(CreateCameraTriggerDef);
DECLARE_DEF_FACTORY(CreateWeatherTriggerDef);
#undef DECLARE_DEF_FACTORY
} // namespace objects

namespace entity {

namespace {

using DefFactory = EntityDef* (*)(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                  const EntityDef* parent);

// Child elements of a definition element.
constexpr uint32_t kElem_Components  = 0x023E6F16u;   // component template list
constexpr uint32_t kElem_Component   = 0x52A72CDDu;   // one template (attribute kAttr_Template)
constexpr uint32_t kAttr_Template    = 0x0024728Bu;
constexpr uint32_t kElem_Attachments = 0xCDA892E2u;

// Shared attributes.
constexpr uint32_t kAttr_Flags[8] = {0xCA8DBA6Bu, 0xA8C8CC1Cu, 0x71362F02u, 0x6611E802u,
                                     0x3395D35Bu, 0x1E0C8C0Cu, 0xE90F4FE5u, 0xC8CB4F86u};
constexpr uint32_t kAttr_Field24        = 0x465FBA18u;
constexpr uint32_t kAttr_Field28        = 0x01B6B516u;
constexpr uint32_t kAttr_PropertyType   = 0x161EABC1u;
constexpr uint32_t kAttr_Field34        = 0xFB89446Eu;
constexpr uint32_t kAttr_HavokSkeleton  = 0x46F408DBu;
constexpr uint32_t kAttr_Field3C        = 0xDD1DC180u;
constexpr uint32_t kAttr_BoundingRadius = 0xE2EA9FD0u;

// Classes built by the loader itself.
constexpr uint32_t kClass_Actor      = 0x93DEF0C4u;   // ActorDef
constexpr uint32_t kClass_Projectile = 0xA77EB5E9u;   // "projectile": PhysicalEntityDef
constexpr uint32_t kClass_Clone      = 0x03D3E53Du;   // "clone": nothing to build, no warning

EntityDef* CreateActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                          const EntityDef* parent)
{
    ActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    EntityDef* def = new ActorDef(desc);
    delete desc.parts;
    return def;
}

// TODO(0x000C4020 PhysicalEntityDefDesc::Parse) TODO(0x000C30E8 PhysicalEntityDef::PhysicalEntityDef):
// the physical definition (description 0x1A0 bytes) is not ported.
EntityDef* CreatePhysicalEntityDef(ape::xml::XmlElement*, ape::xml::XmlElement*, const EntityDefDesc&, const EntityDef*)
{
    return nullptr;
}

struct DefClass {
    uint32_t elementHash;
    DefFactory factory;   // null: not ported (console factory in the comment)
};

// The definition classes by element name hash (the console dispatches with a
// binary search over the same hashes).
const DefClass kDefClasses[] = {
    {kClass_Actor, CreateActorDef},
    {kClass_Projectile, CreatePhysicalEntityDef},
    {0x906A55EAu, nullptr},                              // 0x001B0120
    {0x8CD0E8E8u, nullptr},                              // 0x001B24D0
    {0x871F883Au, nullptr},                              // 0x0020A298
    {0xA0BCC96Fu, objects::CreateCrawlTriggerDef},
    {0xA09DEEA4u, objects::CreateAlertTriggerDef},
    {0x9F7993AFu, objects::CreateFilteredTriggerDef},
    {0x942B64F0u, nullptr},                              // 0x001CA7C0
    {0xA5900924u, objects::CreateUpdateEnableTriggerDef},
    {0xA273E6E4u, nullptr},                              // 0x00245A30
    {0xA0C4F7BCu, objects::CreateDeathTriggerDef},
    {0xB266AD6Cu, nullptr},                              // 0x00307A78
    {0xB2221492u, nullptr},                              // 0x001AB548
    {0xAEF4C388u, nullptr},                              // "BreakableChain" 0x001A5AD8
    {0xA8F71270u, nullptr},                              // 0x0023FA28
    {0xA7CFB837u, nullptr},                              // 0x0016ADC4
    {0xBC13B5BCu, nullptr},                              // 0x000C58A8
    {0xB9F78EFDu, nullptr},                              // 0x000C63B0
    {0xB47D3A95u, nullptr},                              // 0x001AC858
    {0xB3748711u, nullptr},                              // 0x001DE610
    {0xDCC028E7u, objects::CreateJumpDirTriggerDef},
    {0xCCFB1F40u, nullptr},                              // 0x0018F8E8
    {0xC475D118u, nullptr},                              // "Teleporter" 0x001EB2B8
    {0xBC6FA23Bu, nullptr},                              // 0x001C5408
    {0xEE9D8477u, nullptr},                              // "SentryLight" 0x001D0208
    {0xEBE54755u, nullptr},                              // 0x000C77E4
    {0xEB9EE198u, objects::CreateTriggerDef},            // "Trigger"
    {0xE65DDB0Fu, objects::CreateAmbientSoundTriggerDef},
    {0xDF511592u, nullptr},                              // 0x001BED98
    {0x00227313u, nullptr},                              // "Item" 0x001F9BA0
    {0xFDC69C0Fu, nullptr},                              // 0x002183FC
    {0xF8ADA400u, nullptr},                              // 0x002F8EE0
    {0xF004220Bu, nullptr},                              // 0x0001BFE0
    {0x03D1DEA1u, CreateChainDef},                       // "Chain"
    {0x0353CB09u, nullptr},                              // "Character" 0x0011E2B8
    {0x0026B78Du, nullptr},                              // 0x001E6B18
    {0x002678F2u, nullptr},                              // 0x001BB6C4
    {0x109C1EFAu, nullptr},                              // 0x000C5DC8
    {0x0E5B671Du, nullptr},                              // 0x00203BE4
    {0x04B6FABDu, nullptr},                              // 0x00167080
    {0x044344C9u, nullptr},                              // 0x001DBE80
    {0x2190D8D7u, nullptr},                              // 0x001C28E8
    {0x201FA54Fu, nullptr},                              // 0x001AA158
    {0x1B733030u, nullptr},                              // laser field 0x000C1780
    {0x184A4683u, objects::CreateSoundZoneTriggerDef},
    {0x4F590DD4u, nullptr},                              // 0x001C8780
    {0x3F0EA991u, nullptr},                              // 0x00176CC0
    {0x35DF6E97u, nullptr},                              // 0x001FE170
    {0x24A57117u, nullptr},                              // 0x001C0658
    {0x21F55D9Cu, nullptr},                              // 0x000BB4B0
    {0x57CCFDA0u, objects::CreateWindTriggerDef},
    {0x5697EEFBu, objects::CreatePushBlockTriggerDef},
    {0x55B67D7Cu, nullptr},                              // 0x001ECD08
    {0x554FC0D9u, nullptr},                              // 0x001BD668
    {0x6E2FB705u, objects::CreateJobTriggerDef},
    {0x6927A078u, nullptr},                              // 0x001F5BA8
    {0x633A7385u, nullptr},                              // 0x001D3E58
    {0x5EFDA0B2u, nullptr},                              // 0x0022F630
    {0x75F206BDu, objects::CreateCameraTriggerDef},
    {0x71FE40F4u, nullptr},                              // 0x0017D640
    {0x707C673Cu, objects::CreateWeatherTriggerDef},
    {0x70542137u, nullptr},                              // 0x001D71D8
    {0x6F429D34u, nullptr},                              // 0x001EA3B8
    {0x7F6CE46Au, objects::CreateFxTriggerDef},
    {0x7CDC084Cu, nullptr},                              // 0x001AD7F0 (SoundActorDef)
    {0x786E8593u, nullptr},                              // "PushBlock" 0x001B4394
    {0x77A6CF07u, objects::CreateDamageTriggerDef},
};

} // namespace

// @func 0x000CA038 LoadEntityDef
EntityDef* LoadEntityDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope)
{
    (void)root;   // only the warnings (stripped) quote the document path
    const uint32_t classHash = elem->IsValid() ? elem->GetNameHash() : 0;

    // A unique name is required.
    const char* name = elem->GetString("name");
    if (!name || FindEntityDef(ape::HashNameNoCase(name)))
        return nullptr;

    // The parent ("from") gives the defaults of the shared fields.
    EntityDefDesc loader;
    loader.name = name;
    loader.loadScope = scope;
    const EntityDef* parent = nullptr;
    const char* parentGeom = "";
    const char* parentTemplate = "";
    bool flagDefaults[8] = {};
    float field24Default = -1.0f;
    float field28Default = -1.0f;
    uint32_t field34Default = 0;
    uint32_t skeletonDefault = 0;
    const char* from = elem->GetString("from");
    if (from && *from) {
        const uint32_t fromHash = ape::HashNameNoCase(from);
        parent = fromHash ? FindEntityDef(fromHash) : nullptr;
        if (!parent)
            return nullptr;
        if (parent->geomDef)
            parentGeom = parent->geomDef->GetName();
        if (parent->templateObject)
            parentTemplate = parent->templateObject->GetName();
        for (int i = 0; i < 8; ++i)
            flagDefaults[i] = (parent->flags >> (31 - i)) & 1;
        field24Default = parent->field_24;
        field28Default = parent->field_28;
        field34Default = parent->field_78;
        skeletonDefault = parent->havokSkeleton;
    }

    loader.geomName = elem->GetString("objDefName");
    if (!loader.geomName)
        loader.geomName = parentGeom;
    loader.templateName = elem->GetString("objInstName");
    if (!loader.templateName)
        loader.templateName = parentTemplate;
    for (int i = 0; i < 8; ++i)
        elem->GetBool(kAttr_Flags[i], &loader.flags[i], flagDefaults[i]);
    elem->GetFloat(kAttr_Field24, &loader.field_24, field24Default);
    elem->GetFloat(kAttr_Field28, &loader.field_28, field28Default);
    elem->GetHash(kAttr_PropertyType, &loader.propertyType, 0);
    int32_t field34 = 0;
    elem->GetInt(kAttr_Field34, &field34, int32_t(field34Default));
    loader.field_34 = uint32_t(field34);
    elem->GetHash(kAttr_HavokSkeleton, &loader.havokSkeleton, skeletonDefault);
    elem->GetFloat(kAttr_Field3C, &loader.field_3C, 0.0f);
    elem->GetFloat(kAttr_BoundingRadius, &loader.boundingRadius, -1.0f);

    // Component templates by name.
    ape::ApeTArray<uint32_t>* componentNames = nullptr;
    if (ape::xml::XmlElement* list = elem->FirstChildElement(kElem_Components)) {
        for (ape::xml::XmlElement* child = list->FirstChildElement(kElem_Component); child;
             child = child->NextSiblingElement(kElem_Component)) {
            uint32_t templateName = 0;
            child->GetHash(kAttr_Template, &templateName, 0);
            if (!componentNames)
                componentNames = new ape::ApeTArray<uint32_t>(4);
            componentNames->PushBack(templateName);
        }
    }
    if (componentNames && componentNames->Count() != 0)
        loader.componentNames = &componentNames;

    // Attachment records.
    EntityAttachmentList* attachments = nullptr;
    if (ape::xml::XmlElement* list = elem->FirstChildElement(kElem_Attachments))
        ParseEntityAttachments(root, list, attachments);
    if (attachments && attachments->Count() != 0)
        loader.attachments = &attachments;

    EntityDef* def = nullptr;
    if (classHash != kClass_Clone) {
        DefFactory factory = nullptr;
        for (const DefClass& entry : kDefClasses) {
            if (entry.elementHash == classHash) {
                factory = entry.factory;
                break;
            }
        }
        // Unknown classes only earn a warning (stripped).
        if (factory)
            def = factory(root, elem, loader, parent);
    }

    // The new definition keeps a copy of every attachment record.
    if (def && loader.attachments) {
        for (const EntityAttachmentDesc& record : *attachments) {
            if (!def->attachments)
                def->attachments = new EntityAttachmentList();
            def->attachments->PushBack(record);
        }
    }

    delete attachments;
    delete componentNames;
    return def;
}

ActorDefDesc::ActorDefDesc(const EntityDefDesc& loader)
    : ScriptedEntityDefDesc(kDefClass_Scripted | kDefClass_Actor, loader)
{
    geomName = loader.geomName;
    templateName = loader.templateName;
    damageFlags = 0x8200;
    for (int i = 0; i < 5; ++i)
        stageHealth[i] = float(((damageFlags >> 4) & 0xF) - i);
    field_88 = -1.0f;
    navMode = 0x9260C26Eu;   // "Static"
    navObstacleSize = {};
    // Impact .. projectile scales, the four attacker scales and the
    // interaction priority (ActorDef::impactScale onwards).
    static const float kDamageDefaults[15] = {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.3f, -1.0f, 1.0f,
                                              1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    for (int i = 0; i < 15; ++i)
        damage[i] = kDamageDefaults[i];
    defFlags = 1u << 26;
    for (uint8_t& need : interactionNeeds)
        need = 1;
}

// TODO(0x000C85A8 ActorDefDesc::Parse): only the scripted attributes are read.
void ActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                         const EntityDef* parent)
{
    ScriptedEntityDefDesc::Parse(root, elem, loader, parent);
}

} // namespace entity
} // namespace sly
