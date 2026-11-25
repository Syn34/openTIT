// openTIT - "BossGrizz" AI component: the Grizz boss fight data.
//
// An AI component (sly/levels/ai/AIComponent.h) whose description also carries
// the boss encounter: stalactite spawners, path point pairs, ice barrier
// pieces and, for each of the three patterns ("Pattern1".."Pattern3"), the
// action sequences, the QTE rounds and the ice barrier set-up. Component type
// 0x30; description vtable 0x00B0EF20, component vtable 0x00B0EED8.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/levels/ai/AIComponent.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace levels {
namespace ai {

constexpr uint32_t kComp_BossGrizz = 0x30;

// Index of an action element name in the table 0x00F557FC.
enum GrizzActionType : uint32_t {
    kGrizzAction_SuperIceSlice   = 0,
    kGrizzAction_01              = 1,   // 0x233D1C2F; reads AnimStart/AnimActivate/AnimEnd
    kGrizzAction_02              = 2,   // 0x5854BB34; reads up to three "Spray" anim sets
    kGrizzAction_WhaleSplash     = 3,
    kGrizzAction_04              = 4,   // 0xCEE90897
    kGrizzAction_Idle            = 5,   // reads "Time"
    kGrizzAction_Taunt           = 6,
    kGrizzAction_ButtStomp       = 7,   // reads StunnedTimeMax and two more values
    kGrizzAction_IceBarrierIdle  = 8,   // reads "Time"
    kGrizzAction_09              = 9,   // 0xE8C8744B
    kGrizzAction_10              = 10,  // 0x42F06AE3
    kGrizzAction_IceBarrierTaunt = 11,
    kGrizzAction_12              = 12,  // 0xA31C3B3F
    kGrizzActionTypeCount        = 13,
};

struct GrizzAnimSet {
    uint32_t start;      // "AnimStart"
    uint32_t activate;   // "AnimActivate"
    uint32_t end;        // "AnimEnd"
};

// One entry of an action sequence (0x2C bytes).
struct GrizzAction {
    uint32_t type;                 // +0x00 GrizzActionType
    union {
        GrizzAnimSet anims[3];     // +0x04 action 1 (one set) and action 2 (sprays)
        float time;                // +0x04 actions 5 and 8
        struct {
            float stunnedTimeMax;  // +0x04 "StunnedTimeMax"
            float value_08;        // +0x08 attribute 0x1D129FB8 (name unknown)
            bool flag_0C;          // +0x0C attribute 0x9392D188 (name unknown)
        } stun;                    // action 7
    };
    uint32_t sprayCount;           // +0x28 complete "Spray" sets (action 2)
};

// Buttons of the QTE ("type" attribute of a <Button>).
enum GrizzButton : uint32_t {
    kGrizzButton_Square   = 0,
    kGrizzButton_Triangle = 1,
    kGrizzButton_Circle   = 2,
    kGrizzButton_Cross    = 3,
};

// One button press of a QTE sequence (0x14 bytes, 0x11 copied).
struct GrizzQteButton {
    uint32_t button;     // +0x00 GrizzButton
    float time;          // +0x04 "Time"
    uint32_t hash_08;    // +0x08 attribute 0xCC846986 (name unknown)
    uint32_t hash_0C;    // +0x0C attribute 0x38C9D0C3 (name unknown)
    bool neverFreeze;    // +0x10 "NeverFreeze"
};

using GrizzQteSequence = ape::ApeTArray<GrizzQteButton>;     // one <Sequence>
using GrizzQteRound = ape::ApeTArray<GrizzQteSequence*>;     // one <QTERound>

// Data of one pattern (0x74 bytes; the description holds it at +0x58, the
// component at +0x5C).
struct GrizzPattern {
    GrizzPattern();
    ~GrizzPattern();
    GrizzPattern(const GrizzPattern&) = delete;
    GrizzPattern& operator=(const GrizzPattern&) = delete;

    // Deletes the QTE rounds and their sequences.
    void ReleaseQteRounds();

    ape::ApeTArray<GrizzQteRound*> qteRounds;       // +0x00 "QTERound"
    ape::ApeTArray<GrizzAction> actions;            // +0x14 "ActionSequence"
    ape::ApeTArray<uint32_t> icePieces;             // +0x28 <IceBarrier><IcePiece part=.../>
    ape::ApeTArray<GrizzAction> iceBarrierActions;  // +0x3C <IceBarrier><ActionSequence>
    ape::ApeTArray<GrizzAction> iceBarrierActions2; // +0x50 <IceBarrier> element 0x0A774695
    int32_t stalactiteCount = 0;                    // +0x64 <Stalactite> int 0xE84187ED (name unknown)
    uint32_t hash_68 = 0;                           // +0x68 attribute 0x79FF28B6 (name unknown)
    float value_6C = 0.0f;                          // +0x6C attribute 0x9C9AE9A8 (name unknown)
    float value_70 = 0.0f;                          // +0x70 attribute 0xDD63AD17 (name unknown)
};

// A start/target point pair (element 0x3593A64A: "StartPt" / "TgtPt").
struct GrizzPointPair {
    uint32_t startPt;
    uint32_t targetPt;
};

// Description (0x1C4 bytes).
struct BossGrizzDesc : AIComponentDesc {
    explicit BossGrizzDesc(uint32_t nameHash);
    ~BossGrizzDesc() override;

    ape::ApeTArray<uint32_t> stalactiteSpawners;  // +0x1C <Stalactite SpawnerName=...> (max 64)
    ape::ApeTArray<GrizzPointPair> pointPairs;    // +0x30 (max 8)
    ape::ApeTArray<uint32_t> iceBarrierPieces;    // +0x44 <IceBarrierPiece Part=...> (max 64)
    GrizzPattern patterns[3];                     // +0x58
    uint32_t hash_1B4 = 0;                        // attribute 0x70C47F9B (name unknown)
    uint32_t hash_1B8 = 0;                        // attribute 0x99A74FD6 (name unknown)
    uint32_t hash_1BC = 0;                        // attribute 0xD8C27F87 (name unknown)
    uint32_t hash_1C0 = 0;                        // attribute 0x0D8C13F3 (name unknown)
};

// "<BossGrizz>" component description parser (0x00063D54).
entity::ComponentDesc* ParseBossGrizzComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                   uint8_t scope);

// The component: an AI component with its own copy of the encounter data.
class BossGrizzComponent : public AIComponent {
public:
    explicit BossGrizzComponent(const BossGrizzDesc* desc);
    ~BossGrizzComponent() override;

    ape::ApeTArray<uint32_t> stalactiteSpawners;  // +0x20
    ape::ApeTArray<GrizzPointPair> pointPairs;    // +0x34
    ape::ApeTArray<uint32_t> iceBarrierPieces;    // +0x48
    GrizzPattern patterns[3];                     // +0x5C
    uint32_t hash_1B8 = 0;
    uint32_t hash_1BC = 0;
    uint32_t hash_1C0 = 0;
    uint32_t hash_1C4 = 0;
};

} // namespace ai
} // namespace levels
} // namespace sly
