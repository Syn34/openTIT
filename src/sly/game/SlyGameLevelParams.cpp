// openTIT - The level parameters parser (0x00481A88 and its element
// readers 0x0047F108-0x00481828).
#include "sly/game/LevelParams.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/entity/Abilities.h"

#include <cstring>

namespace sly {

using ape::xml::XmlElement;

namespace {

// Child elements of "LevelParams".
constexpr uint32_t kElem_WindowSilhouette = 0x2C9A3F08u;
constexpr uint32_t kElem_Radar = 0x04A21AE6u;
constexpr uint32_t kElem_Mechanic = 0xF7327F84u;
constexpr uint32_t kElem_Headline = 0xEC2F7DD4u;    // "headline"
constexpr uint32_t kElem_AnimSets = 0xD965639Eu;    // NOTE: name unknown
constexpr uint32_t kElem_Player = 0x8CD18EE1u;      // "player"
constexpr uint32_t kElem_Character = 0x0353CB09u;
constexpr uint32_t kElem_Sequence = 0x7F1667C1u;
constexpr uint32_t kElem_Layer = 0x044DDD31u;
constexpr uint32_t kElem_Set = 0x00014042u;         // "SET"
constexpr uint32_t kElem_Group = 0x040EFE5Fu;
constexpr uint32_t kElem_Abilities = 0x08F0F328u;   // NOTE: name unknown

constexpr uint32_t kAttr_Name = 0x0024728Bu;        // "Name"
constexpr uint32_t kAttr_Camera = 0x760CB725u;      // "camera"
constexpr uint32_t kAttr_Type = 0x0027873Au;        // "type"

// Mechanic types.
constexpr uint32_t kMech_Hideout = 0x61313EACu;
constexpr uint32_t kMech_SystemCracker = 0x0C8B8AB8u;
constexpr uint32_t kMech_AlterEgo = 0xA7CFB837u;
constexpr uint32_t kMech_2 = 0x871F673Du;           // NOTE: name unknown

// Boolean attributes of the flag word +0x15C4, by bit.
struct FlagAttribute {
    uint32_t name;
    uint32_t bit;
};
constexpr FlagAttribute kFlagAttributes[] = {
    {0x6260931Du, 31}, {0x34C40D74u, 30}, {0xB6BFA0BEu, 29}, {0xB865B3C5u, 28},
    {0xF7D706D9u, 27}, {0x2A535E79u, 26}, {0x35AD9F0Bu, 24}, {0x23CDEDD0u, 25},
    {0x0D091614u, 23}, {0x434B9649u, 22}, {0x0E23DFC4u, 21}, {0x258F3428u, 20},
};

void ReadFlag(XmlElement* element, uint32_t name, uint32_t bit, uint32_t& flags)
{
    bool value = (flags >> bit) & 1;
    element->GetBool(name, &value, value);
    flags = value ? (flags | (1u << bit)) : (flags & ~(1u << bit));
}

// TODO(0x0047F588 ParseAnimGroup): a "Group" of a SET; until it is ported
// every group is dropped (and with it the SETs and the animation sets).
bool ParseAnimGroup(ape::xml::XmlDocument&, XmlElement*, LevelParams::AnimGroup&)
{
    return false;
}

void ReadView(XmlElement* element, LevelParams::StationView& view)
{
    element->GetHash(0xD86FA37Cu, &view.field_00, 0);
    element->GetHash(0x6BDF55EFu, &view.field_04, 0);
    element->GetHash(0x9C990F5Du, &view.field_08, 0);
    element->GetHash(0x815D083Cu, &view.field_0C, 0);
}

void CopyString(char* out, size_t size, const char* text)
{
    if (text && text[0] != '\0')
        ape::StrNCopy(out, text, size - 1);
}

// @func 0x00480AE8 ParseHideoutStation
// A station without a "Def" is dropped.
void ParseHideoutStation(XmlElement* element, LevelParams::HideoutMechanic& out)
{
    LevelParams::HideoutStation station;
    element->GetHash(0x000107E5u, &station.def, 0);   // "Def"
    if (station.def == 0)
        return;
    CopyString(station.field_04, sizeof(station.field_04), element->GetString(0xA9CD8AAAu));
    CopyString(station.field_86, sizeof(station.field_86), element->GetString(0xE9BDFD43u));
    CopyString(station.field_45, sizeof(station.field_45), element->GetString(0x41FA331Eu));
    element->GetHash(0x3295FD03u, &station.field_C8, 0);
    element->GetHash(0x5745BCC6u, &station.field_CC, 0);
    element->GetHash(0x3B2EA066u, &station.field_D0, 0);
    element->GetHash(0xC1168D61u, &station.field_D4, 0);
    element->GetHash(0xC12B4394u, &station.field_D8, 0);
    element->GetHash(0xEF8AB98Au, &station.field_DC, 0);
    element->GetHash(0xEC96E888u, &station.field_E0, 0);
    element->GetHash(0x4319F4EFu, &station.field_E8, 0);
    element->GetHash(0x82BF101Au, &station.field_EC, 0);
    element->GetHash(0xBF4244BFu, &station.field_F0, 0);
    element->GetHash(0x39B44A1Du, &station.field_F4, 0);
    element->GetBool(0x047D4CDDu, &station.field_F8, false);
    element->GetBool(0xF9B989D5u, &station.field_F9, false);
    element->GetBool(0xAE14078Bu, &station.field_FA, false);
    element->GetBool(0x092E4909u, &station.field_FB, true);
    element->GetBool(0x3B757A97u, &station.field_FC, false);
    // NOTE: the console does not bound the count (ten records fit).
    if (out.stationCount < static_cast<uint32_t>(LevelParams::HideoutMechanic::kMaxStations))
        out.stations[out.stationCount++] = station;
}

// @func 0x00480740 ParseCostumeStation
// Up to eight "Costume" children; each keeps its old values as defaults.
void ParseCostumeStation(XmlElement* element, LevelParams::HideoutMechanic& out)
{
    auto& block = out.costumeSelect;
    ReadView(element, block.view);
    for (XmlElement* child = element->FirstChildElement(0x63C971C0u); child;   // "Costume"
         child = child->NextSiblingElement(0x63C971C0u)) {
        LevelParams::CostumeEntry& entry = block.costumes[block.count];
        child->GetHash(0x4FBC8495u, &entry.instance, entry.instance);   // "instance"
        child->GetHash(0x5A07A589u, &entry.field_04, entry.field_04);
        child->GetHash(0xE6ED234Au, &entry.field_08, entry.field_08);
        child->GetHash(0xBB8E19CAu, &entry.field_0C, entry.field_0C);
        child->GetHash(0xBB8D028Au, &entry.field_10, entry.field_10);
        if (const char* title = child->GetString("uiTitle"))
            ape::StrCopyBounded(entry.uiTitle, title, sizeof(entry.uiTitle));
        if (const char* desc = child->GetString("uiDesc"))
            ape::StrCopyBounded(entry.uiDesc, desc, sizeof(entry.uiDesc));
        child->GetHash(0x79F40122u, &entry.field_114, entry.field_114);
        child->GetHash(0x38E6FC55u, &entry.lockedScript, entry.lockedScript);       // "lockedScript"
        child->GetHash(0xE376E96Eu, &entry.unlockedScript, entry.unlockedScript);   // "unlockedScript"
        if (++block.count >= LevelParams::HideoutMechanic::kMaxCostumes)
            break;
    }
}

// @func 0x00480574 ParseTreasureStation
// Up to four groups of "treasure" spawners.
void ParseTreasureStation(XmlElement* element, LevelParams::HideoutMechanic& out)
{
    auto& block = out.treasures;
    ReadView(element, block.view);
    int group = 0;
    for (XmlElement* child = element->FirstChildElement(0x2D8478ABu);
         child && group < LevelParams::HideoutMechanic::kTreasureGroups;
         child = child->NextSiblingElement(0x2D8478ABu), ++group) {
        // NOTE: the console bounds the group index here instead of the
        // treasure index, so a group of more than twenty treasures overruns.
        int i = 0;
        for (XmlElement* treasure = child->FirstChildElement(0x811B12AFu);   // "treasure"
             treasure && i < LevelParams::HideoutMechanic::kTreasuresPerGroup;
             treasure = treasure->NextSiblingElement(0x811B12AFu), ++i) {
            uint32_t& spawner = block.spawners[group][i];
            treasure->GetHash(0xB2E59948u, &spawner, spawner);   // "spawner"
        }
    }
}

// @func 0x00480E68 ParseHideoutMechanic
void ParseHideoutMechanic(ape::xml::XmlDocument&, XmlElement* element, LevelParams::Mechanic& mechanic)
{
    constexpr uint32_t kElem_LevelSelect = 0x204AB160u;
    constexpr uint32_t kElem_CharacterSelect = 0xF7B9C945u;
    constexpr uint32_t kElem_StationBCAA = 0xBCAAEE0Fu;
    constexpr uint32_t kElem_Station = 0xBA3BD856u;
    constexpr uint32_t kElem_Station9AF3 = 0x9AF323B5u;
    constexpr uint32_t kElem_CostumeSelect = 0x9A62C465u;
    constexpr uint32_t kElem_Treasures = 0x8FFEC609u;
    constexpr uint32_t kSly = 0x00014120u;       // "Sly"
    constexpr uint32_t kBentley = 0x1D8C2817u;   // "Bentley"

    LevelParams::HideoutMechanic& out = mechanic.hideout;
    element->GetFloat(0x52265A94u, &out.field_00, 0.0f);
    element->GetHash(0x49E23F80u, &out.field_04, 0);
    element->GetHash(0xC3905B44u, &out.field_08, 0);
    element->GetHash(0x907E07E3u, &out.field_0C, 0);
    out.stationCount = 0;

    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        switch (child->GetNameHash()) {
        case kElem_LevelSelect: {
            ReadView(child, out.levelSelect.view);
            static constexpr uint32_t kAttrs[10] = {0x93248323u, 0x0E942BDAu, 0xDB81D879u, 0x85470B15u,
                                                   0x03EC0451u, 0xD0D9B0F0u, 0x5EF64B3Eu, 0xC5A01613u,
                                                   0x928DC2B2u, 0xD8D4CA3Cu};
            for (int i = 0; i < 10; ++i)
                child->GetHash(kAttrs[i], &out.levelSelect.field_10[i], 0);
            break;
        }
        case kElem_CharacterSelect: {
            ReadView(child, out.characterSelect.view);
            static constexpr uint32_t kAttrs[6] = {0x7BB427C9u, 0x48A1D468u, 0x824DD4C6u,
                                                  0x76CC881Fu, 0x43BA34BEu, 0xBC008CB0u};
            for (int i = 0; i < 6; ++i)
                child->GetHash(kAttrs[i], &out.characterSelect.field_10[i], 0);
            break;
        }
        case kElem_StationBCAA: {
            auto& s = out.stationBCAA;
            ReadView(child, s.view);
            child->GetHash(0x0D604D70u, &s.player1, kSly);       // "Player1"
            child->GetHash(0x0D604D71u, &s.player2, kBentley);   // "Player2"
            child->GetFloat(0x7546CE83u, &s.field_18, 0.0f);
            child->GetFloat(0xB1073168u, &s.field_1C, 0.0f);
            child->GetInt(0x480B47AEu, &s.field_20, 3);
            break;
        }
        case kElem_Station:
            ParseHideoutStation(child, out);
            break;
        case kElem_Station9AF3: {
            auto& s = out.station9AF3;
            ReadView(child, s.view);
            child->GetHash(0x5E32A4A0u, &s.field_10, s.field_10);
            child->GetHash(0x0F43FA2Bu, &s.field_1C, 0);
            child->GetHash(0x10B312D6u, &s.field_20, 0);
            child->GetHash(0xCCD3652Eu, &s.field_18, 0);
            child->GetHash(0xD72DF2D3u, &s.field_14, 0);
            break;
        }
        case kElem_CostumeSelect:
            ParseCostumeStation(child, out);
            break;
        case kElem_Treasures:
            ParseTreasureStation(child, out);
            break;
        default:
            break;
        }
    }
    element->GetHash(0x76522CB5u, &out.field_1520, 0);
    element->GetHash(0x5449CB0Eu, &out.field_1524, 0);
}

// TODO(0x003F0990 ParseAlterEgoMechanic): the rest of the alter ego block
// (outside this range).
void ParseAlterEgoMechanic(ape::xml::XmlDocument&, XmlElement*, LevelParams::Mechanic&) {}

} // namespace

// @func 0x0047F108 ParseWindowSilhouette
// Camera, two background colours and up to four characters.
static void ParseWindowSilhouette(XmlElement* element, LevelParams::WindowSilhouette& out)
{
    element->GetHash(0x1285C1F8u, &out.field_00, out.field_00);
    element->GetHash(0x5CEEFBCBu, &out.field_04, out.field_04);
    element->GetHash(kAttr_Camera, &out.camera, out.camera);
    element->GetColor(0xEAB9B1D2u, &out.bgColor0, &out.bgColor0);   // "BGColor0"
    element->GetColor(0xEAB9B1D3u, &out.bgColor1, &out.bgColor1);   // "BGColor1"
    XmlElement* child = element->FirstChildElement(kElem_Character);
    for (LevelParams::SilhouetteCharacter& character : out.characters) {
        if (!child)
            break;
        const char* text = nullptr;
        child->GetStringOrNumber(0x6A73C305u, &text, "");
        if (text && text[0] != '\0')
            character.text = text;
        child->GetHash(kAttr_Name, &character.name, 0);
        child->GetHash(0x31F8E7E5u, &character.field_08, 0);
        child->GetHash(0xB2E59A9Fu, &character.field_0C, 0);
        child = child->NextSiblingElement(kElem_Character);
    }
}

// @func 0x0047F310 ParseAnimSequence
// The "SET" steps of a sequence (with a set name) and its name; false
// without steps or name.
static bool ParseAnimSequence(XmlElement* element, LevelParams::AnimSequence& out)
{
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid() || child->GetNameHash() != kElem_Set)
            continue;
        LevelParams::SequenceStep step;
        child->GetHash(kElem_Set, &step.set, step.set);
        if (step.set == 0)
            continue;
        child->GetFloat(0xB7EA3D94u, &step.duration, step.duration);   // "duration"
        out.steps.push_back(step);
    }
    if (out.steps.empty())
        return false;
    element->GetHash(kAttr_Name, &out.name, out.name);
    return out.name != 0;
}

// @func 0x0047FCF0 ParseAnimSet
// The groups of a "SET" and its name; false without groups or name.
static bool ParseAnimSet(ape::xml::XmlDocument& document, XmlElement* element, LevelParams::AnimSet& out)
{
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid() || child->GetNameHash() != kElem_Group)
            continue;
        LevelParams::AnimGroup group;
        if (ParseAnimGroup(document, child, group))
            out.groups.push_back(group);
    }
    if (out.groups.empty())
        return false;
    element->GetHash(kAttr_Name, &out.name, out.name);
    return out.name != 0;
}

// @func 0x0047FFF8 ParseAnimSets
// Layers, SETs and sequences; usable once it has both SETs and sequences.
static void ParseAnimSets(ape::xml::XmlDocument& document, XmlElement* element, LevelParams::AnimSets& out)
{
    element->GetHash(0x0DE7D203u, &out.field_0C, out.field_0C);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        switch (child->GetNameHash()) {
        case kElem_Sequence: {
            LevelParams::AnimSequence sequence;
            if (ParseAnimSequence(child, sequence))
                out.sequences.push_back(std::move(sequence));
            break;
        }
        case kElem_Layer: {
            LevelParams::AnimLayer layer;
            child->GetHash(kAttr_Name, &layer.name, layer.name);
            if (layer.name == 0)
                break;
            child->GetFloat(0x04B70827u, &layer.speed, layer.speed);   // "speed"
            child->GetInt(0xBF27DE6Fu, &layer.count, layer.count);
            if (layer.count == 0)
                break;
            child->GetBool(0x332BE350u, &layer.flag0C, layer.flag0C);
            out.layers.push_back(layer);
            break;
        }
        case kElem_Set: {
            LevelParams::AnimSet set;
            if (ParseAnimSet(document, child, set))
                out.sets.push_back(std::move(set));
            break;
        }
        default:
            break;
        }
    }
    out.valid = !out.sets.empty() && !out.sequences.empty();
}

// @func 0x00481828 ParseMechanic
// The kind named by "type" and its block.
static void ParseMechanic(ape::xml::XmlDocument& document, XmlElement* element, LevelParams& params)
{
    uint32_t type = 0;
    if (element->GetHash(kAttr_Type, &type, 0) != 0)
        return;
    LevelParams::Mechanic& mechanic = params.mechanic;
    switch (type) {
    case kMech_Hideout:
        params.field_1544 = LevelParams::kMechanic_Hideout;
        ParseHideoutMechanic(document, element, mechanic);
        break;
    case kMech_SystemCracker:
        params.field_1544 = LevelParams::kMechanic_SystemCracker;
        element->GetHash(kAttr_Camera, &mechanic.character, 0);
        break;
    case kMech_AlterEgo:
        params.field_1544 = LevelParams::kMechanic_AlterEgo;
        element->GetHash(kElem_Character, &mechanic.character, 0);
        element->GetHash(kAttr_Camera, &mechanic.camera, 0);
        element->GetHash(0x922D3BD1u, &mechanic.spline, 0);           // "spline"
        element->GetFloat(0xAF9EDCABu, &mechanic.speedMin, 2.0f);     // "SpeedMin"
        element->GetFloat(0xAF9EDBBDu, &mechanic.speedMax, 10.0f);    // "SpeedMax"
        element->GetFloat(0x313AA4BFu, &mechanic.field_14, 0.05f);
        element->GetFloat(0x313AA3D1u, &mechanic.field_18, 0.95f);
        element->GetFloat(0x00F35D02u, &mechanic.field_1C, 0.2f);
        element->GetFloat(0x807548E2u, &mechanic.damping, 0.0f);      // "damping"
        ParseAlterEgoMechanic(document, element, mechanic);
        break;
    case kMech_2:
        params.field_1544 = LevelParams::kMechanic_2;
        break;
    default:
        break;
    }
}

// @func 0x00481A88 CreateLevelParams
LevelParams* CreateLevelParams(ape::xml::XmlDocument& document, ape::xml::XmlElement* element)
{
    auto* params = new LevelParams();
    if (const char* headline = element->GetString("headlineString"); headline && headline[0] != '\0')
        ape::StrNCopy(params->headline, headline, 0x20);

    element->GetHash(0x040DA3ADu, &params->hash1574, params->hash1574);
    element->GetHash(0x9BBEF6A2u, &params->hash1584, params->hash1584);
    element->GetHash(0xB8148D94u, &params->hash1580, params->hash1580);
    element->GetHash(0xD50DD913u, &params->hash1578, params->hash1578);
    element->GetHash(0x464FEE4Bu, &params->hash157C, params->hash157C);
    element->GetHash(0x1C2A8202u, &params->destLevel, params->destLevel);   // "DestLevel"
    const uint32_t hashNames[5] = {0xB31AFB4Eu, 0x989EE661u, 0x5210AEEEu, 0x64E2C8ACu, 0x10B8A275u};
    for (int i = 0; i < 5; ++i)
        element->GetHash(hashNames[i], &params->hashes158C[i], 0);
    element->GetHash(0x879F1EF5u, &params->destObj, params->destObj);       // "DestObj"
    element->GetHash(0x6740EDCBu, &params->hash15A4, params->hash15A4);
    element->GetFloat(0x465FBA18u, &params->float15A8, params->float15A8);
    element->GetFloat(0xDE2FF597u, &params->float15AC, params->float15AC);
    element->GetFloat(0xE13E6457u, &params->float15B0, params->float15B0);
    element->GetBool(0x08238898u, &params->bool15B4, params->bool15B4);
    element->GetFloat(0xDDFBCF87u, &params->float15B8, params->float15B8);
    element->GetBool(0xE5A96F17u, &params->bool15BC, params->bool15BC);
    element->GetHash(0xFA12E627u, &params->hash15C0, params->hash15C0);
    for (const FlagAttribute& flag : kFlagAttributes)
        ReadFlag(element, flag.name, flag.bit, params->flags);
    bool flag15C8 = params->field_15C8 != 0;
    element->GetBool(0xCCBA923Cu, &flag15C8, flag15C8);
    params->field_15C8 = flag15C8 ? 1 : 0;
    // NOTE: the console also reads the int attribute 0xC2E2D454 and drops it.
    params->field_15CC = 0;

    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t name = child->IsValid() ? child->GetNameHash() : 0;
        switch (name) {
        case kElem_WindowSilhouette:
            params->silhouettes.emplace_back();
            ParseWindowSilhouette(child, params->silhouettes.back());
            break;
        case kElem_Radar: {
            LevelParams::RadarDef radar;
            const uint32_t names[4] = {0x10FB7036u, 0x36B2F929u, 0xF479AF0Bu, 0xD704CE34u};
            for (int i = 0; i < 4; ++i)
                child->GetBool(names[i], &radar.flags[i], radar.flags[i]);
            params->radar.push_back(radar);
            break;
        }
        case kElem_Mechanic:
            ParseMechanic(document, child, *params);
            break;
        case kElem_Headline: {
            LevelParams::JobHeadline headline;
            child->GetHash(0x0437B578u, &headline.jobName, 0);
            if (const char* text = child->GetString("headlineString"); text && text[0] != '\0')
                ape::StrNCopy(headline.text, text, 0x20);
            params->jobHeadlines.push_back(headline);
            break;
        }
        case kElem_AnimSets:
            ParseAnimSets(document, child, params->animSets);
            break;
        case kElem_Player: {
            LevelParams::PlayerDef player;
            if (const char* defName = child->GetString("playerControllableDefName"))
                ape::StrNCopy(player.defName, defName, 0x20);
            child->GetHash(0x4D33CAC6u, &player.defNameHash, player.defNameHash);
            if (XmlElement* abilities = child->FirstChildElement(kElem_Abilities)) {
                for (int i = 0; i < entity::kAbilityCount; ++i) {
                    bool enabled = false;
                    if (abilities->GetBool(entity::AbilityHash(i), &enabled, false) == 0 && enabled)
                        player.abilities[i >> 5] |= 1u << (i & 31);
                }
            }
            child->GetBool(0x9EA962C9u, &player.flag38, player.flag38);
            child->GetBool(0x68420EF9u, &player.flag39, player.flag39);
            params->players.push_back(player);
            break;
        }
        default:
            // Unknown elements: the console formats a warning with the
            // document path.
            (void)document.GetDocumentPath();
            break;
        }
    }
    return params;
}

} // namespace sly
