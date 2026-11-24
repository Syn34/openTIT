// openTIT - "BossGrizz" AI component and its description (0x00062824-0x00064B8C).
#include "sly/levels/ai/AIBossGrizz.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace levels {
namespace ai {

using ape::xml::XmlElement;

namespace {

constexpr uint32_t kAttr_Type = 0x0027873Au;             // "type"
constexpr uint32_t kAttr_Setting = 0x3A034393u;          // "AISettingName"
constexpr uint32_t kAttr_1B4 = 0x70C47F9Bu;              // NOTE: name unknown
constexpr uint32_t kAttr_1B8 = 0x99A74FD6u;              // NOTE: name unknown
constexpr uint32_t kAttr_1BC = 0xD8C27F87u;              // NOTE: name unknown
constexpr uint32_t kAttr_1C0 = 0x0D8C13F3u;              // NOTE: name unknown
constexpr uint32_t kAttr_SpawnerName = 0x288C27D3u;      // "SpawnerName"
constexpr uint32_t kAttr_StartPt = 0xB9B6CDC6u;          // "StartPt"
constexpr uint32_t kAttr_TgtPt = 0x04C14205u;            // "TgtPt"
constexpr uint32_t kAttr_Part = 0x00255BF3u;             // "Part"
constexpr uint32_t kAttr_Pattern68 = 0x79FF28B6u;        // NOTE: name unknown
constexpr uint32_t kAttr_Pattern6C = 0x9C9AE9A8u;        // NOTE: name unknown
constexpr uint32_t kAttr_Pattern70 = 0xDD63AD17u;        // NOTE: name unknown
constexpr uint32_t kAttr_StalactiteCount = 0xE84187EDu;  // NOTE: name unknown
constexpr uint32_t kAttr_Time = 0x00274ACDu;             // "Time"
constexpr uint32_t kAttr_ButtonHash08 = 0xCC846986u;     // NOTE: name unknown
constexpr uint32_t kAttr_ButtonHash0C = 0x38C9D0C3u;     // NOTE: name unknown
constexpr uint32_t kAttr_NeverFreeze = 0x71FE8003u;      // "NeverFreeze"
constexpr uint32_t kAttr_AnimStart = 0xE9C0B4D1u;        // "AnimStart"
constexpr uint32_t kAttr_AnimActivate = 0x5F41DE04u;     // "AnimActivate"
constexpr uint32_t kAttr_AnimEnd = 0xF7B7CD0Au;          // "AnimEnd"
constexpr uint32_t kAttr_StunnedTimeMax = 0x42ADFD84u;   // "StunnedTimeMax"
constexpr uint32_t kAttr_Stun08 = 0x1D129FB8u;           // NOTE: name unknown
constexpr uint32_t kAttr_Stun0C = 0x9392D188u;           // NOTE: name unknown

constexpr uint32_t kElem_Stalactite = 0x618C8814u;       // "Stalactite"
constexpr uint32_t kElem_PointPair = 0x3593A64Au;        // NOTE: name unknown
constexpr uint32_t kElem_IceBarrierPiece = 0xFEF85B82u;  // "IceBarrierPiece"
constexpr uint32_t kElem_ActionSequence = 0xCFB643F7u;   // "ActionSequence"
constexpr uint32_t kElem_QTERound = 0xB431C02Cu;         // "QTERound"
constexpr uint32_t kElem_IceBarrier = 0x8D7B96CCu;       // "IceBarrier"
constexpr uint32_t kElem_IceBarrierActions2 = 0x0A774695u; // NOTE: name unknown
constexpr uint32_t kElem_IcePiece = 0xF1BF4343u;         // "IcePiece"
constexpr uint32_t kElem_Sequence = 0x7F1667C1u;         // "Sequence"
constexpr uint32_t kElem_Button = 0x75751B32u;           // "Button"
constexpr uint32_t kElem_Spray = 0x04B7388Du;            // "Spray"

constexpr uint32_t kMaxStalactites = 64;
constexpr uint32_t kMaxPointPairs = 8;
constexpr uint32_t kMaxPieces = 64;
constexpr uint32_t kMaxSprays = 3;

// "Pattern1".."Pattern3" (0x00F557E0).
constexpr uint32_t kPatternElems[3] = { 0x78EA76C1u, 0x78EA76C2u, 0x78EA76C3u };

// Action element names by GrizzActionType (0x00F557FC).
constexpr uint32_t kActionElems[kGrizzActionTypeCount] = {
    0xB190E242u,   // "SuperIceSlice"
    0x233D1C2Fu,
    0x5854BB34u,
    0x9ED528B0u,   // "WhaleSplash"
    0xCEE90897u,
    0x002237D4u,   // "Idle"
    0x04BE8B4Eu,   // "Taunt"
    0xA70409FEu,   // "ButtStomp"
    0xA51D88A0u,   // "IceBarrierIdle"
    0xE8C8744Bu,
    0x42F06AE3u,
    0xFF2D5402u,   // "IceBarrierTaunt"
    0xA31C3B3Fu,
};

// <Button type=...> values.
constexpr uint32_t kButton_Square = 0x923F4D1Du;
constexpr uint32_t kButton_Triangle = 0x87EBFA08u;
constexpr uint32_t kButton_Circle = 0x767FB0D0u;
constexpr uint32_t kButton_Cross = 0x03D6A020u;

// Capacities the console gives the arrays.
constexpr uint32_t kHashListCapacity = 64;
constexpr uint32_t kPairCapacity = 8;
constexpr uint32_t kActionCapacity = 16;
constexpr uint32_t kRoundCapacity = 4;
constexpr uint32_t kSequenceCapacity = 16;

GrizzButton ButtonFromHash(uint32_t hash)
{
    switch (hash) {
    case kButton_Triangle: return kGrizzButton_Triangle;
    case kButton_Circle:   return kGrizzButton_Circle;
    case kButton_Cross:    return kGrizzButton_Cross;
    case kButton_Square:
    default:               return kGrizzButton_Square;
    }
}

template <class T>
void AppendAll(ape::ApeTArray<T>& to, const ape::ApeTArray<T>& from)
{
    for (const T& value : from)
        to.PushBack(value);
}

// Appends a hash attribute when it is present and non-zero.
void AppendHashAttribute(XmlElement* elem, uint32_t attr, ape::ApeTArray<uint32_t>& list, uint32_t maxCount)
{
    if (list.Count() >= maxCount)
        return;
    uint32_t value = 0;
    if (elem->GetHash(attr, &value, 0) == 0 && value != 0)
        list.PushBack(value);
}

// "<QTERound>": a list of <Sequence> elements, each a list of <Button>s.
GrizzQteRound* ParseQteRound(XmlElement* elem)
{
    auto* round = new GrizzQteRound(kRoundCapacity);
    for (XmlElement* seqElem = elem->FirstChildElement(); seqElem; seqElem = seqElem->NextSiblingElement()) {
        if (seqElem->GetNameHash() != kElem_Sequence)
            continue;
        auto* sequence = new GrizzQteSequence(kSequenceCapacity);
        for (XmlElement* button = seqElem->FirstChildElement(); button; button = button->NextSiblingElement()) {
            if (button->GetNameHash() != kElem_Button)
                continue;
            GrizzQteButton entry{};
            button->GetFloat(kAttr_Time, &entry.time, 0.0f);
            button->GetHash(kAttr_ButtonHash08, &entry.hash_08, entry.hash_08);
            button->GetHash(kAttr_ButtonHash0C, &entry.hash_0C, entry.hash_0C);
            button->GetBool(kAttr_NeverFreeze, &entry.neverFreeze, entry.neverFreeze);
            uint32_t type = 0;
            if (button->GetHash(kAttr_Type, &type, 0) != 0 || type == 0)
                continue;   // a button without a type is dropped
            entry.button = ButtonFromHash(type);
            sequence->PushBack(entry);
        }
        round->PushBack(sequence);
    }
    return round;
}

// Deep copy of a QTE round; empty sequences are left out. Null when the
// round has no sequences.
GrizzQteRound* CopyQteRound(const GrizzQteRound& from)
{
    if (from.Count() == 0)
        return nullptr;
    auto* round = new GrizzQteRound(kRoundCapacity);
    for (const GrizzQteSequence* sequence : from) {
        if (sequence->Count() == 0)
            continue;
        auto* copy = new GrizzQteSequence(kSequenceCapacity);
        AppendAll(*copy, *sequence);
        round->PushBack(copy);
    }
    return round;
}

// Copies the data of one pattern from the description into the component.
void CopyPattern(GrizzPattern& to, const GrizzPattern& from)
{
    AppendAll(to.icePieces, from.icePieces);
    AppendAll(to.iceBarrierActions, from.iceBarrierActions);
    AppendAll(to.iceBarrierActions2, from.iceBarrierActions2);
    to.stalactiteCount = from.stalactiteCount;
    to.hash_68 = from.hash_68;
    to.value_6C = from.value_6C;
    to.value_70 = from.value_70;
    AppendAll(to.actions, from.actions);
    for (const GrizzQteRound* round : from.qteRounds) {
        if (GrizzQteRound* copy = CopyQteRound(*round))
            to.qteRounds.PushBack(copy);
    }
}

} // namespace

GrizzPattern::GrizzPattern()
    : qteRounds(kRoundCapacity)
    , actions(kActionCapacity)
    , icePieces(kHashListCapacity)
    , iceBarrierActions(kActionCapacity)
    , iceBarrierActions2(kActionCapacity)
{
}

GrizzPattern::~GrizzPattern()
{
    ReleaseQteRounds();
}

void GrizzPattern::ReleaseQteRounds()
{
    for (GrizzQteRound* round : qteRounds) {
        for (GrizzQteSequence* sequence : *round)
            delete sequence;
        round->Clear();
        delete round;
    }
    qteRounds.Clear();
}

// -- description ------------------------------------------------------------------

// @func 0x00062824 BossGrizzDesc::BossGrizzDesc
BossGrizzDesc::BossGrizzDesc(uint32_t nameHash)
    : AIComponentDesc(nameHash)
    , stalactiteSpawners(kHashListCapacity)
    , pointPairs(kPairCapacity)
    , iceBarrierPieces(kHashListCapacity)
{
    type = kComp_BossGrizz;
}

// @func 0x00062B20 BossGrizzDesc::~BossGrizzDesc
// @func 0x00062DD0 BossGrizzDesc::~BossGrizzDesc (deleting)
BossGrizzDesc::~BossGrizzDesc() = default;   // the patterns free their QTE rounds

// @func 0x00062E20 ParseGrizzActionSequence
// Appends one action per child element whose name is an action type.
// NOTE: the console reuses one stack record for every child without clearing
// it, so attributes an action does not read keep the previous action's
// values; the port starts each action from zero.
static void ParseActionSequence(XmlElement* elem, ape::ApeTArray<GrizzAction>& actions)
{
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t name = child->GetNameHash();
        for (uint32_t type = 0; type < kGrizzActionTypeCount; ++type) {
            if (kActionElems[type] != name)
                continue;
            GrizzAction action{};
            action.type = type;
            switch (type) {
            case kGrizzAction_01:
                child->GetHash(kAttr_AnimStart, &action.anims[0].start, 0);
                child->GetHash(kAttr_AnimActivate, &action.anims[0].activate, 0);
                child->GetHash(kAttr_AnimEnd, &action.anims[0].end, 0);
                break;
            case kGrizzAction_02:
                // Up to three sprays; a spray counts only when it names all
                // three animations.
                for (XmlElement* spray = child->FirstChildElement(); spray; spray = spray->NextSiblingElement()) {
                    if (spray->GetNameHash() != kElem_Spray || action.sprayCount >= kMaxSprays)
                        continue;
                    GrizzAnimSet& set = action.anims[action.sprayCount];
                    const bool hasStart = spray->GetHash(kAttr_AnimStart, &set.start, 0) == 0;
                    const bool hasActivate = spray->GetHash(kAttr_AnimActivate, &set.activate, 0) == 0;
                    const bool hasEnd = spray->GetHash(kAttr_AnimEnd, &set.end, 0) == 0;
                    if (hasStart && hasActivate && hasEnd)
                        ++action.sprayCount;
                }
                break;
            case kGrizzAction_Idle:
            case kGrizzAction_IceBarrierIdle:
                child->GetFloat(kAttr_Time, &action.time, 0.0f);
                break;
            case kGrizzAction_ButtStomp:
                child->GetFloat(kAttr_StunnedTimeMax, &action.stun.stunnedTimeMax, 0.0f);
                child->GetFloat(kAttr_Stun08, &action.stun.value_08, 0.0f);
                child->GetBool(kAttr_Stun0C, &action.stun.flag_0C, false);
                break;
            default:
                break;
            }
            actions.PushBack(action);
        }
    }
}

// <IceBarrier> of a pattern.
static void ParseIceBarrier(XmlElement* elem, GrizzPattern& pattern)
{
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        switch (child->GetNameHash()) {
        case kElem_IceBarrierActions2:
            ParseActionSequence(child, pattern.iceBarrierActions2);
            break;
        case kElem_IcePiece:
            if (pattern.icePieces.Count() < kMaxPieces) {
                // NOTE: unlike the top-level lists a missing "Part" is
                // appended as 0.
                uint32_t part = 0;
                child->GetHash(kAttr_Part, &part, 0);
                pattern.icePieces.PushBack(part);
            }
            break;
        case kElem_ActionSequence:
            ParseActionSequence(child, pattern.iceBarrierActions);
            break;
        default:
            break;
        }
    }
}

// "<PatternN>".
static void ParsePattern(XmlElement* elem, GrizzPattern& pattern)
{
    elem->GetHash(kAttr_Pattern68, &pattern.hash_68, pattern.hash_68);
    elem->GetFloat(kAttr_Pattern6C, &pattern.value_6C, pattern.value_6C);
    elem->GetFloat(kAttr_Pattern70, &pattern.value_70, pattern.value_70);
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        switch (child->GetNameHash()) {
        case kElem_Stalactite:
            child->GetInt(kAttr_StalactiteCount, &pattern.stalactiteCount, pattern.stalactiteCount);
            break;
        case kElem_ActionSequence:
            ParseActionSequence(child, pattern.actions);
            break;
        case kElem_QTERound:
            pattern.qteRounds.PushBack(ParseQteRound(child));
            break;
        case kElem_IceBarrier:
            ParseIceBarrier(child, pattern);
            break;
        default:
            break;
        }
    }
}

// @func 0x000632A8 BossGrizzDesc::Parse
static void ParseBossGrizzBody(XmlElement* elem, BossGrizzDesc* desc)
{
    elem->GetHash(kAttr_Type, &desc->aiType, desc->aiType);
    elem->GetHash(kAttr_Setting, &desc->settingsName, desc->settingsName);
    if (desc->aiType == kAIType_PatternBoss)
        ParseAIStatePatterns(elem);
    elem->GetHash(kAttr_1B4, &desc->hash_1B4, desc->hash_1B4);
    elem->GetHash(kAttr_1B8, &desc->hash_1B8, desc->hash_1B8);
    elem->GetHash(kAttr_1BC, &desc->hash_1BC, desc->hash_1BC);
    elem->GetHash(kAttr_1C0, &desc->hash_1C0, desc->hash_1C0);

    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t name = child->GetNameHash();
        if (name == kElem_Stalactite) {
            AppendHashAttribute(child, kAttr_SpawnerName, desc->stalactiteSpawners, kMaxStalactites);
        } else if (name == kElem_PointPair) {
            if (desc->pointPairs.Count() >= kMaxPointPairs)
                continue;
            GrizzPointPair pair{};
            if (child->GetHash(kAttr_StartPt, &pair.startPt, 0) != 0 || pair.startPt == 0)
                continue;
            if (child->GetHash(kAttr_TgtPt, &pair.targetPt, 0) != 0 || pair.targetPt == 0)
                continue;
            desc->pointPairs.PushBack(pair);
        } else if (name == kElem_IceBarrierPiece) {
            AppendHashAttribute(child, kAttr_Part, desc->iceBarrierPieces, kMaxPieces);
        } else {
            for (int i = 0; i < 3; ++i) {
                if (kPatternElems[i] == name)
                    ParsePattern(child, desc->patterns[i]);
            }
        }
    }
}

// @func 0x00063D54 ParseBossGrizzComponentDesc
entity::ComponentDesc* ParseBossGrizzComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    auto* desc = new BossGrizzDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    ParseBossGrizzBody(elem, desc);
    return desc;
}

// -- component --------------------------------------------------------------------

// @func 0x00063E24 BossGrizzComponent::BossGrizzComponent
BossGrizzComponent::BossGrizzComponent(const BossGrizzDesc* desc)
    : AIComponent(desc)
    , stalactiteSpawners(desc->stalactiteSpawners.Count())
    , pointPairs(desc->pointPairs.Count())
    , iceBarrierPieces(desc->iceBarrierPieces.Count())
    , hash_1B8(desc->hash_1B4)
    , hash_1BC(desc->hash_1B8)
    , hash_1C0(desc->hash_1BC)
    , hash_1C4(desc->hash_1C0)
{
    AppendAll(stalactiteSpawners, desc->stalactiteSpawners);
    AppendAll(pointPairs, desc->pointPairs);
    AppendAll(iceBarrierPieces, desc->iceBarrierPieces);
    for (int i = 0; i < 3; ++i)
        CopyPattern(patterns[i], desc->patterns[i]);
}

// @func 0x00064894 BossGrizzComponent::~BossGrizzComponent
// @func 0x00064B8C BossGrizzComponent::~BossGrizzComponent (deleting)
BossGrizzComponent::~BossGrizzComponent() = default;   // patterns free their rounds; AIComponent detaches

} // namespace ai
} // namespace levels
} // namespace sly
