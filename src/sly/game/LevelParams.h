// openTIT - Level parameters (the "LevelParams" element of a level's data
// file, 0x15D0 bytes on the console, built by 0x00481A88): the headline
// shown when the level starts, per-job headline overrides, the playable
// characters, the radar and window silhouette settings, the level's
// mechanic (hideout, system cracker, alter ego) and its game settings.
//
// NOTE: most attribute names are unknown; their hashes are given with the
// fields. The record arrays are ApeTArray pointers on the console.
#pragma once

#include "ape/xml/ApeXml.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ape::xml { class XmlDocument; class XmlElement; }

namespace sly {

struct LevelParams {
    // A playable character of the level ("player", 0x3C-byte records).
    struct PlayerDef {
        char defName[0x20] = {};          // +0x00 "playerControllableDefName"
        uint32_t defNameHash = 0;         // +0x24 the same attribute read as a hash
        uint32_t abilities[4] = {};       // +0x28 one bit per ability enabled (child 0x08F0F328)
        bool flag38 = false;              // +0x38 attribute 0x9EA962C9
        bool flag39 = false;              // +0x39 attribute 0x68420EF9
    };
    // Radar settings ("Radar", 4 bytes).
    struct RadarDef {
        bool flags[4] = {true, true, false, false};   // 0x10FB7036, 0x36B2F929, 0xF479AF0B, 0xD704CE34
    };
    // Animation layer of the level's animation sets ("layer", 0x10 bytes).
    struct AnimLayer {
        uint32_t name = 0;                // +0x00 "Name"
        float speed = 0.0f;               // +0x04 "speed"
        int32_t count = 0;                // +0x08 attribute 0xBF27DE6F
        bool flag0C = false;              // +0x0C attribute 0x332BE350
    };
    // A group of a SET (0x40 bytes, parsed by 0x0047F588, not ported).
    struct AnimGroup {
        uint32_t field_00[2] = {};
        float field_10[4] = {};           // +0x10 from the default vector 0x01164C90
        uint32_t field_20[7] = {};
    };
    struct AnimSet {                      // "SET" (8 bytes)
        std::vector<AnimGroup> groups;    // +0x00 "Group" children
        uint32_t name = 0;                // +0x04 "Name"
    };
    struct SequenceStep {                 // "SET" child of a sequence (8 bytes)
        uint32_t set = 0;                 // +0x00 "SET"
        float duration = 0.0f;            // +0x04 "duration"
    };
    struct AnimSequence {                 // "Sequence" (8 bytes)
        std::vector<SequenceStep> steps;  // +0x00
        uint32_t name = 0;                // +0x04 "Name"
    };
    // Element 0xD965639E (0x14 bytes at +0x08).
    struct AnimSets {
        std::vector<AnimLayer> layers;        // +0x00
        std::vector<AnimSet> sets;            // +0x04
        std::vector<AnimSequence> sequences;  // +0x08
        uint32_t field_0C = 0;                // +0x0C attribute 0x0DE7D203
        bool valid = false;                   // +0x10 sets and sequences present
    };
    // A character shown in a window silhouette (0x10 bytes).
    struct SilhouetteCharacter {
        std::string text;                 // +0x00 attribute 0x6A73C305 (copied)
        uint32_t name = 0;                // +0x04 "Name"
        uint32_t field_08 = 0;            // +0x08 attribute 0x31F8E7E5
        uint32_t field_0C = 0;            // +0x0C attribute 0xB2E59A9F
    };
    // "WindowSilhouette" (0x54 bytes).
    struct WindowSilhouette {
        uint32_t field_00 = 0;            // +0x00 attribute 0x1285C1F8
        uint32_t field_04 = 0;            // +0x04 attribute 0x5CEEFBCB
        uint32_t camera = 0;              // +0x08 "camera"
        ape::xml::Color8 bgColor0{0, 0, 0, 0xFF};   // +0x0C "BGColor0"
        ape::xml::Color8 bgColor1{0, 0, 0, 0xFF};   // +0x10 "BGColor1"
        SilhouetteCharacter characters[4];          // +0x14 "Character" children
    };
    // Headline override for one job ("headline", 0x28-byte records).
    struct JobHeadline {
        uint32_t jobName = 0;             // +0x00 attribute 0x0437B578
        char text[0x24] = {};             // +0x04 "headlineString"
    };
    // The level's mechanic ("Mechanic", 0x1528 bytes at +0x1C; its kind at
    // +0x1544).
    enum MechanicKind : int32_t {
        kMechanic_None = 0, kMechanic_Hideout = 1, kMechanic_2 = 2, kMechanic_SystemCracker = 3,
        kMechanic_AlterEgo = 4,
    };
    // The hideout's mechanic block (parsed by 0x00480E68; on the console it
    // shares the 0x1528 bytes of Mechanic with the other kinds). Each
    // station element starts with the same four hashes (attributes
    // 0xD86FA37C, 0x6BDF55EF, 0x9C990F5D, 0x815D083C). NOTE: names unknown.
    struct StationView {
        uint32_t field_00 = 0;
        uint32_t field_04 = 0;
        uint32_t field_08 = 0;
        uint32_t field_0C = 0;
    };
    // Element 0xBA3BD856 (0x100-byte records).
    struct HideoutStation {
        uint32_t def = 0;                 // +0x00 "Def" (a record without one is dropped)
        char field_04[0x41] = {};         // +0x04 attribute 0xA9CD8AAA
        char field_45[0x41] = {};         // +0x45 attribute 0x41FA331E
        char field_86[0x42] = {};         // +0x86 attribute 0xE9BDFD43
        uint32_t field_C8 = 0;            // +0xC8 attribute 0x3295FD03
        uint32_t field_CC = 0;            // +0xCC attribute 0x5745BCC6
        uint32_t field_D0 = 0;            // +0xD0 attribute 0x3B2EA066
        uint32_t field_D4 = 0;            // +0xD4 attribute 0xC1168D61
        uint32_t field_D8 = 0;            // +0xD8 attribute 0xC12B4394
        uint32_t field_DC = 0;            // +0xDC attribute 0xEF8AB98A
        uint32_t field_E0 = 0;            // +0xE0 attribute 0xEC96E888
        uint32_t field_E8 = 0;            // +0xE8 attribute 0x4319F4EF
        uint32_t field_EC = 0;            // +0xEC attribute 0x82BF101A
        uint32_t field_F0 = 0;            // +0xF0 attribute 0xBF4244BF
        uint32_t field_F4 = 0;            // +0xF4 attribute 0x39B44A1D
        bool field_F8 = false;            // +0xF8 attribute 0x047D4CDD
        bool field_F9 = false;            // +0xF9 attribute 0xF9B989D5
        bool field_FA = false;            // +0xFA attribute 0xAE14078B
        bool field_FB = true;             // +0xFB attribute 0x092E4909
        bool field_FC = false;            // +0xFC attribute 0x3B757A97
    };
    // A "Costume" of the costume station (0x120-byte records).
    struct CostumeEntry {
        uint32_t instance = 0;            // +0x00 "instance"
        uint32_t field_04 = 0;            // +0x04 attribute 0x5A07A589
        uint32_t field_08 = 0;            // +0x08 attribute 0xE6ED234A
        uint32_t field_0C = 0;            // +0x0C attribute 0xBB8E19CA
        uint32_t field_10 = 0;            // +0x10 attribute 0xBB8D028A
        char uiTitle[0x80] = {};          // +0x14 "uiTitle"
        char uiDesc[0x80] = {};           // +0x94 "uiDesc"
        uint32_t field_114 = 0;           // +0x114 attribute 0x79F40122
        uint32_t lockedScript = 0;        // +0x118 "lockedScript"
        uint32_t unlockedScript = 0;      // +0x11C "unlockedScript"
    };
    struct HideoutMechanic {
        static constexpr int kMaxStations = 10;
        static constexpr int kMaxCostumes = 8;
        static constexpr int kTreasureGroups = 4;
        static constexpr int kTreasuresPerGroup = 20;

        float field_00 = 0.0f;            // +0x00 attribute 0x52265A94
        uint32_t field_04 = 0;            // +0x04 attribute 0x49E23F80
        uint32_t field_08 = 0;            // +0x08 attribute 0xC3905B44
        uint32_t field_0C = 0;            // +0x0C attribute 0x907E07E3
        HideoutStation stations[kMaxStations];   // +0x10
        uint32_t stationCount = 0;        // +0xA10
        // "CharacterSelect".
        struct {
            StationView view;             // +0xA14
            uint32_t field_10[6] = {};    // +0xA24 attributes 0x7BB427C9, 0x48A1D468, 0x824DD4C6,
                                          //        0x76CC881F, 0x43BA34BE, 0xBC008CB0
        } characterSelect;
        // Element 0x9A62C465: the costume station.
        struct {
            StationView view;             // +0xA3C
            uint8_t count = 0;            // +0xA4C
            CostumeEntry costumes[kMaxCostumes];   // +0xA50
        } costumeSelect;
        // "LevelSelect".
        struct {
            StationView view;             // +0x1350
            uint32_t field_10[10] = {};   // +0x1360 attributes 0x93248323, 0x0E942BDA, 0xDB81D879,
                                          //   0x85470B15, 0x03EC0451, 0xD0D9B0F0, 0x5EF64B3E,
                                          //   0xC5A01613, 0x928DC2B2, 0xD8D4CA3C
        } levelSelect;
        // Element 0x9AF323B5.
        struct {
            StationView view;             // +0x1388
            uint32_t field_10 = 0;        // +0x1398 attribute 0x5E32A4A0
            uint32_t field_14 = 0;        // +0x139C attribute 0xD72DF2D3
            uint32_t field_18 = 0;        // +0x13A0 attribute 0xCCD3652E
            uint32_t field_1C = 0;        // +0x13A4 attribute 0x0F43FA2B
            uint32_t field_20 = 0;        // +0x13A8 attribute 0x10B312D6
        } station9AF3;
        // Element 0xBCAAEE0F: a two player station.
        struct {
            StationView view;             // +0x13AC
            uint32_t player1 = 0;         // +0x13BC "Player1" (default Sly)
            uint32_t player2 = 0;         // +0x13C0 "Player2" (default Bentley)
            float field_18 = 0.0f;        // +0x13C4 attribute 0x7546CE83
            float field_1C = 0.0f;        // +0x13C8 attribute 0xB1073168
            int32_t field_20 = 3;         // +0x13CC attribute 0x480B47AE
        } stationBCAA;
        // Element 0x8FFEC609: groups (0x2D8478AB) of "treasure" spawners.
        struct {
            StationView view;             // +0x13D0
            uint32_t spawners[kTreasureGroups][kTreasuresPerGroup] = {};   // +0x13E0 "spawner"
        } treasures;
        uint32_t field_1520 = 0;          // +0x1520 attribute 0x76522CB5
        uint32_t field_1524 = 0;          // +0x1524 attribute 0x5449CB0E
    };
    struct Mechanic {
        uint32_t character = 0;           // +0x00 alter ego: "Character"; system cracker: "camera"
        uint32_t camera = 0;              // +0x04 alter ego: "camera"
        uint32_t spline = 0;              // +0x08 "spline"
        float speedMin = 2.0f;            // +0x0C "SpeedMin"
        float speedMax = 10.0f;           // +0x10 "SpeedMax"
        float field_14 = 0.05f;           // +0x14 attribute 0x313AA4BF
        float field_18 = 0.95f;           // +0x18 attribute 0x313AA3D1
        float field_1C = 0.2f;            // +0x1C attribute 0x00F35D02
        float damping = 0.0f;             // +0x20 "damping"
        // NOTE: the rest of the alter ego block is filled by 0x003F0990 (not
        // ported).
        HideoutMechanic hideout;          // the hideout's block (same bytes on the console)
    };

    std::vector<PlayerDef> players;       // +0x00 "player"
    std::vector<RadarDef> radar;          // +0x04 "Radar"
    AnimSets animSets;                    // +0x08 element 0xD965639E
    uint8_t field_18 = 0;                 // +0x18
    Mechanic mechanic;                    // +0x1C
    int32_t field_1544 = kMechanic_None;  // +0x1544 the mechanic kind
    std::vector<WindowSilhouette> silhouettes;   // +0x1548 "WindowSilhouette"
    std::vector<JobHeadline> jobHeadlines;       // +0x154C "headline"
    char headline[0x24] = {};             // +0x1550 "headlineString"
    uint32_t hash1574 = 0xBFD63062u;      // +0x1574 attribute 0x040DA3AD
    uint32_t hash1578 = 0x8CEBEA48u;      // +0x1578 attribute 0xD50DD913
    uint32_t hash157C = 0x49D44980u;      // +0x157C attribute 0x464FEE4B
    uint32_t hash1580 = 0x96305389u;      // +0x1580 attribute 0xB8148D94
    uint32_t hash1584 = 0;                // +0x1584 attribute 0x9BBEF6A2
    uint32_t destLevel = 0;               // +0x1588 "DestLevel"
    uint32_t hashes158C[5] = {};          // +0x158C 0xB31AFB4E, 0x989EE661, 0x5210AEEE, 0x64E2C8AC, 0x10B8A275
    uint32_t destObj = 0;                 // +0x15A0 "DestObj"
    uint32_t hash15A4 = 0;                // +0x15A4 attribute 0x6740EDCB
    float float15A8 = -1.0f;              // +0x15A8 attribute 0x465FBA18
    float float15AC = 0.0f;               // +0x15AC attribute 0xDE2FF597
    float float15B0 = 0.0f;               // +0x15B0 attribute 0xE13E6457
    bool bool15B4 = false;                // +0x15B4 attribute 0x08238898
    float float15B8 = 0.0f;               // +0x15B8 attribute 0xDDFBCF87
    bool bool15BC = false;                // +0x15BC attribute 0xE5A96F17
    uint32_t hash15C0 = 0;                // +0x15C0 attribute 0xFA12E627
    // +0x15C4: bits 31..20 from twelve boolean attributes (see
    // kFlagAttributes in SlyGameLevelParams.cpp).
    uint32_t flags = 0;
    uint32_t field_15C8 = 0;              // +0x15C8 attribute 0xCCBA923C (0/1)
    uint8_t field_15CC = 0;               // +0x15CC
};

// Builds the level parameters from their element (0x00481A88).
LevelParams* CreateLevelParams(ape::xml::XmlDocument& document, ape::xml::XmlElement* element);

} // namespace sly
