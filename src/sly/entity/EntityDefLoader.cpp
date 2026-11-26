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
#include "sly/entity/ActorDefLoaders.h"
#include "sly/entity/ActorEntities.h"
#include "sly/entity/EntityDef.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/entity/PropEntities.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/xml/ApeXml.h"

#include <cstdio>

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
    {0x906A55EAu, CreateSignalActorDef},
    {0x8CD0E8E8u, CreateActorB15E60Def},
    {0x871F883Au, nullptr},                              // 0x0020A298
    {0xA0BCC96Fu, objects::CreateCrawlTriggerDef},
    {0xA09DEEA4u, objects::CreateAlertTriggerDef},
    {0x9F7993AFu, objects::CreateFilteredTriggerDef},
    {0x942B64F0u, nullptr},                              // 0x001CA7C0
    {0xA5900924u, objects::CreateUpdateEnableTriggerDef},
    {0xA273E6E4u, nullptr},                              // 0x00245A30
    {0xA0C4F7BCu, objects::CreateDeathTriggerDef},
    {0xB266AD6Cu, nullptr},                              // 0x00307A78
    {0xB2221492u, CreateHitCounterActorDef},
    {0xAEF4C388u, nullptr},                              // "BreakableChain" 0x001A5AD8
    {0xA8F71270u, nullptr},                              // 0x0023FA28
    {0xA7CFB837u, nullptr},                              // 0x0016ADC4
    {0xBC13B5BCu, nullptr},                              // 0x000C58A8
    {0xB9F78EFDu, nullptr},                              // 0x000C63B0
    {0xB47D3A95u, CreateWatchActorDef},
    {0xB3748711u, nullptr},                              // 0x001DE610
    {0xDCC028E7u, objects::CreateJumpDirTriggerDef},
    {0xCCFB1F40u, nullptr},                              // 0x0018F8E8
    {0xC475D118u, CreateObjectActorDef},                 // "Teleporter"
    {0xBC6FA23Bu, nullptr},                              // 0x001C5408
    {0xEE9D8477u, CreateSoundActorB174C0Def},            // "SentryLight"
    {0xEBE54755u, nullptr},                              // 0x000C77E4
    {0xEB9EE198u, objects::CreateTriggerDef},            // "Trigger"
    {0xE65DDB0Fu, objects::CreateAmbientSoundTriggerDef},
    {0xDF511592u, CreateSoundActorB16678Def},
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
    {0x044344C9u, CreateInteractivePropAUseDef},
    {0x2190D8D7u, CreateSoundActorB16B48Def},
    {0x201FA54Fu, CreateActorGroupDef},
    {0x1B733030u, nullptr},                              // laser field 0x000C1780
    {0x184A4683u, objects::CreateSoundZoneTriggerDef},
    {0x4F590DD4u, CreateSoundActorB17008Def},
    {0x3F0EA991u, nullptr},                              // 0x00176CC0
    {0x35DF6E97u, nullptr},                              // 0x001FE170
    {0x24A57117u, CreateSoundActorB168E0Def},
    {0x21F55D9Cu, nullptr},                              // 0x000BB4B0
    {0x57CCFDA0u, objects::CreateWindTriggerDef},
    {0x5697EEFBu, objects::CreatePushBlockTriggerDef},
    {0x55B67D7Cu, CreateTweenActorDef},
    {0x554FC0D9u, nullptr},                              // 0x001BD668
    {0x6E2FB705u, objects::CreateJobTriggerDef},
    {0x6927A078u, CreateInteractivePropBDef},
    {0x633A7385u, CreateSoundActorB17728Def},
    {0x5EFDA0B2u, nullptr},                              // 0x0022F630
    {0x75F206BDu, objects::CreateCameraTriggerDef},
    {0x71FE40F4u, nullptr},                              // 0x0017D640
    {0x707C673Cu, objects::CreateWeatherTriggerDef},
    {0x70542137u, nullptr},                              // 0x001D71D8
    {0x6F429D34u, CreateSearchlightDef},
    {0x7F6CE46Au, objects::CreateFxTriggerDef},
    {0x7CDC084Cu, CreateSoundActorDef},
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

namespace {

// Actor attributes.
constexpr uint32_t kAttr_ActorField88       = 0x43B894DBu;
constexpr uint32_t kAttr_NavMode            = 0x5B1E4A49u;   // also per part
constexpr uint32_t kAttr_NavObstacleSize    = 0x354C5B93u;
constexpr uint32_t kAttr_InheritParts       = 0xFE139BBCu;
constexpr uint32_t kElem_Part               = 0x6364B153u;
// Damage scales in ActorDefDesc::damage order (impact .. interaction
// priority). The fourth is only read while the third is positive.
constexpr uint32_t kAttr_Damage[15] = {0x824F3B6Fu, 0x6B5A442Du, 0xAD6099F3u, 0xA274B629u, 0x9AB17C61u,
                                       0x9FF13A0Au, 0x4AD59CEFu, 0x825857B0u, 0x6C74B00Cu, 0x86D97BF8u,
                                       0x30003446u, 0x3E44F9F5u, 0xFDA86B42u, 0x0D5F7C08u, 0x55B0EB25u};
// Definition flags 31 down to 22.
constexpr uint32_t kAttr_DefFlags[10] = {0xBA6158CDu, 0xA499B8DAu, 0xB0D191A7u, 0xF03E41A4u, 0xBD23F15Du,
                                         0x8CB95A92u, 0xA78B0427u, 0x0883B9B7u, 0x1667C898u, 0xB2B78B0Fu};
// Part attributes.
constexpr uint32_t kAttr_PartOffset         = 0x63662D61u;
constexpr uint32_t kAttr_PartField10        = 0x1B89D5F3u;
constexpr uint32_t kAttr_PartField24        = 0x75849CFEu;
constexpr uint32_t kAttr_PartField30        = 0x07A80994u;
constexpr uint32_t kAttr_PartField40[7] = {0x46206B53u, 0xE3E14C49u, 0x471281EFu, 0x9538DE99u,
                                           0x9533AB5Au, 0x040C1D75u, 0x69FE7274u};
constexpr uint32_t kAttr_PartField60[6] = {0x29F45D46u, 0xA1FDE21Du, 0xE2154ADBu, 0x8933D0E0u,
                                           0x0023FEF4u, 0xB89310FDu};
constexpr uint32_t kAttr_PartField78        = 0x725B9AF2u;
constexpr uint32_t kAttr_PartFlags[4] = {0xF8E455ACu, 0xBC07DCA3u, 0x47C04D53u, 0x306B6E24u};   // bits 31..28
constexpr uint32_t kAttr_PartAimOffset      = 0xCC0D6864u;
// A definition keeps at most this many parts.
constexpr uint16_t kMaxActorParts = 0x20;

// Reads boolean attribute `attr` into bit `bit` of `word` (default: the bit of `defaults`).
void ReadBit(ape::xml::XmlElement* elem, uint32_t attr, uint32_t& word, uint32_t defaults, int bit)
{
    bool on = false;
    elem->GetBool(attr, &on, ((defaults >> bit) & 1) != 0);
    word = (word & ~(1u << bit)) | (uint32_t(on) << bit);
}

void AddPart(ape::ApeTArray<ActorPartDesc>*& parts, const ActorPartDesc& part)
{
    if (!parts)
        parts = new ape::ApeTArray<ActorPartDesc>();
    parts->Add(part);
}

// One part element.
ActorPartDesc ParsePart(ape::xml::XmlElement* elem)
{
    ActorPartDesc part;
    elem->GetVec3(kAttr_PartOffset, &part.offset.x, &part.offset.x);
    part.bodyName = elem->GetString("colObjName");
    if (!part.IsDescribed())
        return part;
    elem->GetVec3(kAttr_PartField10, &part.field_10.x, &part.field_10.x);
    elem->GetHash(kAttr_PartField24, &part.field_24, part.field_24);
    for (int i = 0; i < 6; ++i)
        elem->GetFloat(kAttr_PartField60[i], &part.field_60[i], part.field_60[i]);
    elem->GetHash(kAttr_PartField78, &part.field_78, part.field_78);
    for (int i = 0; i < 4; ++i)
        ReadBit(elem, kAttr_PartFlags[i], part.partFlags, part.partFlags, 31 - i);
    if (part.partFlags & (1u << 28)) {
        elem->GetVec3(kAttr_PartField30, &part.field_30.x, &part.field_30.x);
        for (int i = 0; i < 7; ++i)
            elem->GetFloat(kAttr_PartField40[i], &part.field_40[i], part.field_40[i]);
    }
    elem->GetVec3(kAttr_PartAimOffset, &part.aimOffset.x, &part.aimOffset.x);
    elem->GetHash(kAttr_NavMode, &part.navMode, part.navMode);
    return part;
}

} // namespace

bool ActorPartDesc::IsDescribed() const
{
    return (bodyName && *bodyName) || offset.x != 0.0f || offset.y != 0.0f || offset.z != 0.0f;
}

// @func 0x000C85A8 ActorDefDesc::Parse
void ActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                         const EntityDef* parent)
{
    (void)root;   // only the warnings (stripped) quote the document path
    ScriptedEntityDefDesc::Parse(root, elem, loader, parent);

    // Defaults: the class defaults, or an actor parent's values and parts.
    ActorDefDesc defaults(loader);
    defaults.geomName = "";
    defaults.templateName = "";
    ape::ApeTArray<ActorPartDesc>* parentParts = nullptr;
    if (parent && parent->IsClass(kDefClass_Actor)) {
        const auto* actor = static_cast<const ActorDef*>(parent);
        defaults.defFlags = (defaults.defFlags & 0x003FFFFFu) | (actor->defFlags & 0xFFC00000u);
        defaults.field_88 = actor->field_CC;
        defaults.navMode = actor->navMode;
        defaults.navObstacleSize = actor->navObstacleSize;
        const float damage[15] = {actor->impactScale, actor->field_F4, actor->blastScale, actor->field_FC,
                                  actor->collisionScale, actor->collisionThreshold, actor->collisionMax,
                                  actor->meleeScale, actor->field_110, actor->projectileScale,
                                  actor->attackerScale[0], actor->attackerScale[1], actor->attackerScale[2],
                                  actor->attackerScale[3], actor->interactionPriority};
        for (int i = 0; i < 15; ++i)
            defaults.damage[i] = damage[i];
        for (int i = 0; i < 6; ++i)
            defaults.interactionNeeds[i] = actor->interactionNeeds[i];
        if (actor->parts) {
            for (const ActorDef::Part& part : *actor->parts) {
                ActorPartDesc desc;
                static_cast<ActorPartFields&>(desc) = part;
                desc.partFlags &= 0xF0000000u;
                desc.bodyName = part.havokObject ? HavokBodyName(part.havokObject) : nullptr;
                AddPart(parentParts, desc);
            }
        }
    }

    if (!geomName || !*geomName)
        geomName = defaults.geomName;
    if (!templateName || !*templateName)
        templateName = defaults.templateName;

    elem->GetFloat(kAttr_ActorField88, &field_88, defaults.field_88);
    elem->GetHash(kAttr_NavMode, &navMode, defaults.navMode);
    elem->GetVec3(kAttr_NavObstacleSize, &navObstacleSize.x, &defaults.navObstacleSize.x);
    for (int i = 0; i < 15; ++i) {
        if (i == 3 && !(damage[2] > 0.0f)) {
            damage[3] = 0.0f;
            continue;
        }
        elem->GetFloat(kAttr_Damage[i], &damage[i], defaults.damage[i]);
    }
    for (int i = 0; i < 10; ++i)
        ReadBit(elem, kAttr_DefFlags[i], defFlags, defaults.defFlags, 31 - i);

    // Interaction needs per kind: the defaults, or six numbers.
    for (int i = 0; i < 6; ++i)
        interactionNeeds[i] = defaults.interactionNeeds[i];
    if (const char* targeting = elem->GetString("targetingFlags")) {
        int values[6];
        if (std::sscanf(targeting, "%d,%d,%d,%d,%d,%d", &values[0], &values[1], &values[2], &values[3],
                        &values[4], &values[5]) == 6) {
            for (int i = 0; i < 6; ++i)
                interactionNeeds[i] = uint8_t(values[i]);
        }
    }

    // The definition's own parts.
    for (ape::xml::XmlElement* child = elem->FirstChildElement(kElem_Part); child;
         child = child->NextSiblingElement(kElem_Part)) {
        if (parts && parts->Count() >= kMaxActorParts)
            continue;   // too many parts (warning stripped)
        const ActorPartDesc part = ParsePart(child);
        if (part.IsDescribed())
            AddPart(parts, part);
    }

    // The parent's parts when asked for or when there are none of our own.
    bool inheritParts = false;
    elem->GetBool(kAttr_InheritParts, &inheritParts, false);
    if (parentParts && parentParts->Count() != 0 && (inheritParts || !parts || parts->Count() == 0)) {
        for (const ActorPartDesc& part : *parentParts) {
            if (parts && parts->Count() >= kMaxActorParts)
                continue;
            if (part.IsDescribed())
                AddPart(parts, part);
        }
    }
    delete parentParts;
}

} // namespace entity
} // namespace sly
