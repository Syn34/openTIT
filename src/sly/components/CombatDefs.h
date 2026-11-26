// openTIT - The <Combat> element of the entity files (0x00272588-0x00274E00):
// named attacks (<Attack>, records of vtable 0x00B1C020 in the map
// 0x0105752C) and hit reactions (<Reaction>, records of vtable 0x00B1C030 in
// the map 0x01057544) that combat components refer to by name.
#pragma once

#include "ape/container/ApeTSortedMap.h"

#include <cstdint>

namespace ape { class ApeAnimScript; }
namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

// A hit reaction (0x80 bytes). Attribute names in quotes are recovered, the
// others keep their hash.
class CombatReaction {
public:
    // The parsed attributes (filled by ParseCombatReaction).
    struct Desc {
        const char* name = nullptr;     // +0x00 "name"
        uint32_t type = 0;              // +0x04 "Type"
        bool freeze = false;            // +0x08 "Freeze"
        bool animVel = false;           // +0x09 "AnimVel"
        bool animRot = false;           // +0x0A "AnimRot"
        uint32_t animGnd = 0;           // +0x0C "AnimGnd"
        uint32_t animAir = 0;           // +0x10 "AnimAir"
        uint32_t animWall = 0;          // +0x14 "AnimWall"
        uint32_t animLoop = 0;          // +0x18 "AnimLoop"
        float timeMin = 3.4028235e38f;  // +0x1C "TimeMin"
        float timeMax = -1.0f;          // +0x20 "TimeMax"
        float field_24 = 0.0f;          // +0x24 attribute 0x716A8C33
        float field_28 = 0.0f;          // +0x28 attribute 0xB2E21D07
        float knockback = 0.0f;         // +0x2C "Knockback"
        float field_30 = 0.0f;          // +0x30 attribute 0x0906364F
        float frictionScalar = 1.0f;    // +0x34 "FrictionScalar"
        uint32_t script = 0;            // +0x38 "Script"
        uint32_t field_3C = 0;          // +0x3C attribute 0x2DD296EB
        uint32_t reactionNext = 0;      // +0x40 "ReactionNext"
        uint32_t reactionDeath = 0;     // +0x44 "ReactionDeath"
        uint32_t reactionLand = 0;      // +0x48 "ReactionLand"
    };

    // Builds the reaction and registers it by name.
    explicit CombatReaction(const Desc& desc);
    // Leaves the registry.
    virtual ~CombatReaction();

    // Flags (+0x60).
    enum Flags : uint32_t {
        kReact_Freeze  = 0x80000000u,
        kReact_AnimVel = 0x40000000u,
        kReact_AnimRot = 0x20000000u,
    };

    char name[0x20] = {};               // +0x04
    uint32_t nameHash = 0;              // +0x24
    uint32_t animGnd = 0;               // +0x28
    uint32_t animAir = 0;               // +0x2C (AnimGnd when not given)
    uint32_t animWall = 0;              // +0x30 (AnimGnd when not given)
    uint32_t animLoop = 0;              // +0x34
    float timeMax = -1.0f;              // +0x38
    float timeMin = 0.0f;               // +0x3C
    float field_40 = 0.0f;              // +0x40
    float field_44 = 0.0f;              // +0x44
    uint32_t type = 0;                  // +0x48
    uint32_t field_50 = 0;              // +0x50
    float field_54 = 0.0f;              // +0x54
    float knockbackSpeed = 0.0f;        // +0x58 the negated "Knockback"
    uint32_t flags = 0;                 // +0x60 Flags
    float frictionScalar = 1.0f;        // +0x64
    ape::ApeAnimScript* script = nullptr;   // +0x68 resolved in the global library
    uint32_t field_6C = 0;              // +0x6C
    uint32_t reactionNext = 0;          // +0x70
    uint32_t reactionDeath = 0;         // +0x74
    uint32_t reactionLand = 0;          // +0x78
};

// A shooting attack of the <Shooting> element (0x50 bytes, vtable
// 0x00B1D020, map 0x0105ABB8).
class ShootingAttack {
public:
    // Stances an attack can be used in (attribute "Type"; index into the
    // table 0x00AD5534 of nine hashes). NOTE: only three names are known.
    enum Stance : uint32_t {
        kStance_Ground = 0,   // "ground"
        kStance_Air = 3,      // "AIR"
        kStance_Climb = 6,    // "CLIMB"
    };

    explicit ShootingAttack(const char* attackName);
    // Leaves the registry.
    virtual ~ShootingAttack();

    char name[0x20] = {};          // +0x04
    uint32_t nameHash = 0;         // +0x24
    uint32_t animW = 0;            // +0x28 "AnimW"
    uint32_t animA = 0;            // +0x2C "AnimA"
    uint32_t animF = 0;            // +0x30 "AnimF"
    uint32_t layer = 0;            // +0x34 "Layer"
    bool additive = false;         // +0x38 "Additive"
    uint32_t stance = 0;           // +0x3C Stance
    float timeWindup = -1.0f;      // +0x40 "TimeWindup"
    float timeActive = -1.0f;      // +0x44 "TimeActive"
    float field_48 = 3.4028235e38f;   // +0x48 attribute 0xED297827
    uint32_t attackNext = 0;       // +0x4C "AttackNext"
};

ape::ApeTSortedMap<ShootingAttack*>& ShootingAttacks();
// One <Attack> element of a <Shooting> block: a new attack unless the name is
// missing or already taken (null then).
ShootingAttack* ParseShootingAttack(ape::xml::XmlElement* root, ape::xml::XmlElement* element);

// Registries of the named records.
ape::ApeTSortedMap<CombatReaction*>& CombatReactions();
ape::ApeTSortedMap<void*>& CombatAttacks();

// One <Reaction> element: a new reaction unless the name is missing or
// already taken (null then).
CombatReaction* ParseCombatReaction(ape::xml::XmlElement* root, ape::xml::XmlElement* element);
// One <Attack> element (0x00273088, TODO).
void* ParseCombatAttack(ape::xml::XmlElement* root, ape::xml::XmlElement* element, uint8_t scope);

} // namespace components
} // namespace sly
