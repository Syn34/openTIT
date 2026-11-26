// openTIT - Combat attack and reaction definitions (see CombatDefs.h).
#include "sly/components/CombatDefs.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/entity/EntityHooks.h"

namespace sly {
namespace components {

using ape::xml::XmlElement;

namespace {

constexpr uint32_t kElem_Attack = 0x73B1E248u;     // "Attack"
constexpr uint32_t kElem_Reaction = 0xFAF0CA29u;   // "Reaction"

namespace attr {
constexpr uint32_t kType = 0x0027873Au;            // "Type"
constexpr uint32_t kFreeze = 0x7C172E97u;          // "Freeze"
constexpr uint32_t kAnimVel = 0xF7B80BCCu;         // "AnimVel"
constexpr uint32_t kAnimRot = 0xF7B7FE06u;         // "AnimRot"
constexpr uint32_t kAnimGnd = 0xF7B7D48Cu;         // "AnimGnd"
constexpr uint32_t kAnimAir = 0xF7B7BD79u;         // "AnimAir"
constexpr uint32_t kAnimWall = 0xFF49D35Bu;        // "AnimWall"
constexpr uint32_t kAnimLoop = 0xFF450835u;        // "AnimLoop"
constexpr uint32_t kTimeMin = 0xDC7AD045u;         // "TimeMin"
constexpr uint32_t kTimeMax = 0xDC7ACF57u;         // "TimeMax"
constexpr uint32_t kReact24 = 0x716A8C33u;         // NOTE: name unknown
constexpr uint32_t kReact28 = 0xB2E21D07u;         // NOTE: name unknown
constexpr uint32_t kKnockback = 0xE0490B9Bu;       // "Knockback"
constexpr uint32_t kReact30 = 0x0906364Fu;         // NOTE: name unknown
constexpr uint32_t kFrictionScalar = 0x7FD0E766u;  // "FrictionScalar"
constexpr uint32_t kScript = 0x9178C4CBu;          // "Script"
constexpr uint32_t kReact3C = 0x2DD296EBu;         // NOTE: name unknown
constexpr uint32_t kReactionNext = 0x23E2109Cu;    // "ReactionNext"
constexpr uint32_t kReactionDeath = 0x57D2C1CBu;   // "ReactionDeath"
constexpr uint32_t kReactionLand = 0x23E11794u;    // "ReactionLand"
// Shooting attacks.
constexpr uint32_t kAnimW = 0x03B88866u;           // "AnimW"
constexpr uint32_t kAnimA = 0x03B88850u;           // "AnimA"
constexpr uint32_t kAnimF = 0x03B88855u;           // "AnimF"
constexpr uint32_t kLayer = 0x044DDD31u;           // "Layer"
constexpr uint32_t kAdditive = 0xE589ABCCu;        // "Additive"
constexpr uint32_t kTimeWindup = 0x89040DB0u;      // "TimeWindup"
constexpr uint32_t kTimeActive = 0x6327B013u;      // "TimeActive"
constexpr uint32_t kShoot48 = 0xED297827u;         // NOTE: name unknown
constexpr uint32_t kAttackNext = 0xA80C013Bu;      // "AttackNext"
} // namespace attr

// Stance names of the shooting attacks (0x00AD5534).
constexpr uint32_t kStanceNames[9] = {
    0x7DD0CD87u,   // "ground"
    0x663CDE6Du, 0x5B1EF01Du,
    0x0000FD2Au,   // "AIR"
    0xEFB68490u, 0xABBE3000u,
    0x03D3CE95u,   // "CLIMB"
    0x419CA87Bu, 0x829677ABu,
};

} // namespace

ape::ApeTSortedMap<CombatReaction*>& CombatReactions()
{
    static ape::ApeTSortedMap<CombatReaction*> map;   // 0x01057544
    return map;
}

ape::ApeTSortedMap<void*>& CombatAttacks()
{
    static ape::ApeTSortedMap<void*> map;   // 0x0105752C
    return map;
}

// @func 0x00273E90 CombatReaction::CombatReaction
CombatReaction::CombatReaction(const Desc& desc)
{
    type = desc.type;
    flags = (desc.freeze ? kReact_Freeze : 0u) | (desc.animVel ? kReact_AnimVel : 0u) |
            (desc.animRot ? kReact_AnimRot : 0u);
    field_54 = desc.field_30;
    knockbackSpeed = -desc.knockback;
    frictionScalar = desc.frictionScalar;
    field_6C = desc.field_3C;
    reactionNext = desc.reactionNext;
    reactionDeath = desc.reactionDeath;
    reactionLand = desc.reactionLand;
    nameHash = ape::HashNameNoCase(desc.name);
    ape::StrNCopy(name, desc.name, sizeof(name) - 1);
    animGnd = desc.animGnd;
    animAir = desc.animAir ? desc.animAir : desc.animGnd;
    animWall = desc.animWall ? desc.animWall : desc.animGnd;
    animLoop = desc.animLoop;
    timeMax = desc.timeMax;
    timeMin = desc.timeMin;
    field_40 = desc.field_24;
    field_44 = desc.field_28;
    if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0))
        script = library->FindScript(desc.script);
    CombatReactions().Add(nameHash, this);
}

// @func 0x00274144 CombatReaction::~CombatReaction
// @func 0x0027418C CombatReaction::~CombatReaction (deleting)
CombatReaction::~CombatReaction()
{
    CombatReactions().Remove(nameHash);
}

// @func 0x002741F4 ParseCombatReaction
CombatReaction* ParseCombatReaction(XmlElement* root, XmlElement* element)
{
    (void)root;   // the console only names its file in a compiled-out warning
    const char* name = element->GetString("name");
    if (!name || *CombatReactions().Find(ape::HashNameNoCase(name)))
        return nullptr;
    CombatReaction::Desc desc;
    desc.name = name;
    element->GetHash(attr::kAnimGnd, &desc.animGnd, 0);
    element->GetHash(attr::kAnimAir, &desc.animAir, 0);
    element->GetHash(attr::kAnimWall, &desc.animWall, 0);
    element->GetHash(attr::kAnimLoop, &desc.animLoop, 0);
    element->GetHash(attr::kType, &desc.type, 0);
    element->GetFloat(attr::kTimeMin, &desc.timeMin, 3.4028235e38f);
    element->GetFloat(attr::kTimeMax, &desc.timeMax, -1.0f);
    element->GetFloat(attr::kReact24, &desc.field_24, 0.0f);
    element->GetFloat(attr::kReact28, &desc.field_28, 0.0f);
    element->GetBool(attr::kFreeze, &desc.freeze, false);
    element->GetBool(attr::kAnimVel, &desc.animVel, false);
    element->GetBool(attr::kAnimRot, &desc.animRot, false);
    element->GetFloat(attr::kKnockback, &desc.knockback, 0.0f);
    element->GetFloat(attr::kReact30, &desc.field_30, 0.0f);
    element->GetFloat(attr::kFrictionScalar, &desc.frictionScalar, 1.0f);
    element->GetHash(attr::kScript, &desc.script, 0);
    element->GetHash(attr::kReact3C, &desc.field_3C, 0);
    element->GetHash(attr::kReactionNext, &desc.reactionNext, 0);
    element->GetHash(attr::kReactionDeath, &desc.reactionDeath, 0);
    element->GetHash(attr::kReactionLand, &desc.reactionLand, 0);
    return new CombatReaction(desc);
}

ape::ApeTSortedMap<ShootingAttack*>& ShootingAttacks()
{
    static ape::ApeTSortedMap<ShootingAttack*> map;   // 0x0105ABB8
    return map;
}

// NOTE: built inline by ParseShootingAttack on the console.
ShootingAttack::ShootingAttack(const char* attackName)
{
    ape::StrNCopy(name, attackName, sizeof(name) - 1);
    nameHash = ape::HashNameNoCase(name);
}

// @func 0x002C0DB8 ShootingAttack::~ShootingAttack
// @func 0x002C0E14 ShootingAttack::~ShootingAttack (deleting)
ShootingAttack::~ShootingAttack()
{
    ShootingAttacks().Remove(ape::HashNameNoCase(name));
}

// @func 0x002C0EA4 ParseShootingAttack
ShootingAttack* ParseShootingAttack(XmlElement* root, XmlElement* element)
{
    (void)root;   // only named in compiled-out warnings
    const char* name = element->GetString("name");
    if (!name || *ShootingAttacks().Find(ape::HashNameNoCase(name)))
        return nullptr;
    uint32_t animW = 0, animA = 0, animF = 0, layer = 0, attackNext = 0;
    element->GetHash(attr::kAnimW, &animW, 0);
    element->GetHash(attr::kAnimA, &animA, 0);
    element->GetHash(attr::kAnimF, &animF, 0);
    uint32_t stance = 0;
    uint32_t stanceName = 0;
    if (element->GetHash(attr::kType, &stanceName, 0) == ape::xml::kQuery_Ok) {
        uint32_t i = 0;
        while (i < 9 && kStanceNames[i] != stanceName)
            ++i;
        // An unknown stance keeps the default (the console warns).
        if (i < 9)
            stance = i;
    }
    element->GetHash(attr::kLayer, &layer, 0);
    bool additive = false;
    element->GetBool(attr::kAdditive, &additive, false);
    float timeWindup = -1.0f, timeActive = -1.0f, field48 = 3.4028235e38f;
    element->GetFloat(attr::kTimeWindup, &timeWindup, timeWindup);
    element->GetFloat(attr::kTimeActive, &timeActive, timeActive);
    element->GetFloat(attr::kShoot48, &field48, field48);
    element->GetHash(attr::kAttackNext, &attackNext, 0);

    auto* attack = new ShootingAttack(name);
    attack->animW = animW;
    attack->animA = animA;
    attack->animF = animF;
    attack->layer = layer;
    attack->additive = additive;
    attack->stance = stance;
    attack->timeWindup = timeWindup;
    attack->timeActive = timeActive;
    attack->field_48 = field48;
    attack->attackNext = attackNext;
    ShootingAttacks().Add(attack->nameHash, attack);
    return attack;
}

// TODO(0x00273088 ParseCombatAttack): the attack record (vtable 0x00B1C020,
// destructor 0x00272588 also releases its sound name +0x118) with its hit
// volumes, reaction names and flags.
void* ParseCombatAttack(XmlElement* root, XmlElement* element, uint8_t scope)
{
    (void)root; (void)element; (void)scope;
    return nullptr;
}

} // namespace components

namespace entity {

// @func 0x00274CDC LoadCombatDefs
void LoadCombatDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope)
{
    for (ape::xml::XmlElement* e = elem->FirstChildElement(components::kElem_Attack); e;
         e = e->NextSiblingElement(components::kElem_Attack))
        components::ParseCombatAttack(root, e, scope);
    for (ape::xml::XmlElement* e = elem->FirstChildElement(components::kElem_Reaction); e;
         e = e->NextSiblingElement(components::kElem_Reaction))
        components::ParseCombatReaction(root, e);
}

// @func 0x002C9D0C LoadShootingDefs
void LoadShootingDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope)
{
    (void)scope;
    for (ape::xml::XmlElement* e = elem->FirstChildElement(components::kElem_Attack); e;
         e = e->NextSiblingElement(components::kElem_Attack))
        components::ParseShootingAttack(root, e);
}

} // namespace entity
} // namespace sly
