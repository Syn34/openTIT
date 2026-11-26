// openTIT - Goals that run a minigame (Bentley vs Penelope, penguin games,
// fencing, fishing, QTE, bartender).
#include "sly/game/goals/GoalTypes.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/goals/GoalHooks.h"

#include <cstdio>
#include <cstring>

namespace sly {
namespace goals {

namespace {

constexpr uint32_t kEvent_MinigameWon  = 0x0D;
constexpr uint32_t kEvent_MinigameLost = 0x0E;

enum class ParamType : uint8_t { Hash, Float, Int };

// One configuration word: the attribute it comes from and its default.
struct ConfigParam {
    int word;
    ParamType type;
    uint32_t attr;
    int32_t intDefault;   // integers only; hashes default to 0
    float floatDefault = 0.0f;
};

void ReadConfig(ape::xml::XmlElement* xml, MinigameConfig& config, const ConfigParam* params,
                size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const ConfigParam& p = params[i];
        uint32_t& word = config.words[p.word];
        switch (p.type) {
        case ParamType::Hash:
            xml->GetHash(p.attr, &word, 0);
            break;
        case ParamType::Float: {
            float value = p.floatDefault;
            xml->GetFloat(p.attr, &value, p.floatDefault);
            std::memcpy(&word, &value, sizeof(word));
            break;
        }
        case ParamType::Int: {
            int32_t value = int32_t(word);
            xml->GetInt(p.attr, &value, p.intDefault);
            word = uint32_t(value);
            break;
        }
        }
    }
}

template <class T, size_t N>
Goal* CreatePenguinGoal(ape::xml::XmlElement* xml, const GoalDesc* desc,
                        const ConfigParam (&params)[N])
{
    MinigameConfig config;
    ReadConfig(xml, config, params, N);
    T* goal = new T(desc);
    goal->config = config;
    return goal;
}

} // namespace

// ===========================================================================
// MinigameGoal

// @func 0x003286D8 MinigameGoal::~MinigameGoal
// @func 0x003289BC MinigameGoal::~MinigameGoal (deleting)
MinigameGoal::~MinigameGoal()
{
    if (minigame && GameActiveMinigame(game) == minigame)
        GameEndActiveMinigame(game);
}

// @func 0x00328A04 MinigameGoal::Update
uint8_t MinigameGoal::Update(int player, const GameEvent& e)
{
    if (result & (kResult_Won | kResult_Lost)) {
        // Act on the result one update after the minigame reported it.
        const bool won = (result & kResult_Won) != 0;
        result &= ~(kResult_Won | kResult_Lost);
        if (won && minigame && GameActiveMinigame(game) == minigame)
            GameEndActiveMinigame(game);
        SetStatus(won ? kGoal_Complete : kGoal_Failed, false);
    } else if (status == kGoal_Active) {
        gameplay::Minigame* active = GameActiveMinigame(game);
        if (!active) {
            SetStatus(kGoal_Complete, false);
        } else if (e.type == kEvent_MinigameWon) {
            if (reinterpret_cast<gameplay::Minigame*>(e.arg0) == active)
                result |= kResult_Won;
        } else if (e.type == kEvent_MinigameLost) {
            if (reinterpret_cast<gameplay::Minigame*>(e.arg0) == active)
                result |= kResult_Lost;
        }
    }
    return Goal::Update(player, e);
}

// @func 0x00328B90 MinigameGoal::CollectMarkers
void MinigameGoal::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    if (gameplay::Minigame* active = GameActiveMinigame(game))
        MinigameCollectMarkers(active, list);
}

// @func 0x00328BF0 MinigameGoal::SetStatus
void MinigameGoal::SetStatus(uint8_t newStatus, bool force)
{
    if (force || newStatus != status) {
        if (newStatus == kGoal_Active) {
            if (!GameActiveMinigame(game)) {
                InstallMinigame();
                minigame = GameActiveMinigame(game);
                if (!force)
                    MinigameStart(minigame);
            }
        } else if (newStatus != kGoal_Complete && newStatus != kGoal_Failed) {
            if (minigame && GameActiveMinigame(game) == minigame)
                GameEndActiveMinigame(game);
        }
    }
    Goal::SetStatus(newStatus, force);
}

// ===========================================================================
// Bentley vs Penelope

// @func 0x0032A328 GoalBentleyVsPenelope::~GoalBentleyVsPenelope
// @func 0x0032A354 GoalBentleyVsPenelope::~GoalBentleyVsPenelope (deleting)
GoalBentleyVsPenelope::~GoalBentleyVsPenelope() = default;

// @func 0x0032A3B4 GoalBentleyVsPenelope::Create
Goal* GoalBentleyVsPenelope::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    (void)xml;
    return new GoalBentleyVsPenelope(desc);
}

// @func 0x0032A440 GoalBentleyVsPenelope::InstallMinigame
void GoalBentleyVsPenelope::InstallMinigame()
{
    GameInstallMinigame(game, CreateMinigame(MinigameKind::BentleyVsPenelope, game, nullptr, this));
}

// ===========================================================================
// Penguin games

namespace {

// Attribute tables of the penguin goals (word index, type, attribute hash,
// integer or float default).
// The balance game passes the words' initial values as defaults from word 6
// on (word 8 starts uninitialised on the console; 0 here).
constexpr ConfigParam kBalanceParams[] = {
    {0, ParamType::Hash, 0xCF4A1DD4, 0},  {2, ParamType::Hash, 0x760CB725, 0},
    {3, ParamType::Hash, 0x9E635873, 0},  {4, ParamType::Float, 0x000110AD, 0},
    {5, ParamType::Float, 0x79792193, 0}, {6, ParamType::Float, 0x591E0E5E, 0, 1.0f},
    {7, ParamType::Float, 0xDAC6B163, 0, 0.1f}, {8, ParamType::Float, 0x6EB97B5A, 0},
    {9, ParamType::Float, 0x7F8F56C5, 0, 5.0f}, {10, ParamType::Float, 0x6AEF6477, 0, 1.0f},
    {11, ParamType::Float, 0xEA445994, 0, 45.0f}, {12, ParamType::Float, 0xB45F6FDB, 0, 4.0f},
    {13, ParamType::Float, 0xB45F6EED, 0, 5.0f}, {14, ParamType::Float, 0xD791BA44, 0, 1.0f},
    {15, ParamType::Float, 0x6993ED21, 0, 10.0f}, {16, ParamType::Float, 0xAF21AF1C, 0, 1.0f},
    {17, ParamType::Float, 0x58BE11B6, 0, 4.0f}, {18, ParamType::Float, 0x35092DB8, 0, 4.0f},
    {19, ParamType::Float, 0xFA04CC11, 0, 4.0f}, {20, ParamType::Float, 0xFA04CB23, 0, 4.0f},
    {21, ParamType::Float, 0x74C4E83D, 0, 4.0f}, {1, ParamType::Float, 0xB7EA3D94, 0, 20.0f},
    {22, ParamType::Int, 0x543CAC2D, 0x19},
};

constexpr ConfigParam kDodgeParams[] = {
    {0, ParamType::Hash, 0xA2A0911A, 0},  {1, ParamType::Hash, 0x441D9642, 0},
    {2, ParamType::Hash, 0x441D9643, 0},  {3, ParamType::Hash, 0x441D9644, 0},
    {4, ParamType::Hash, 0x441D9645, 0},  {5, ParamType::Hash, 0x5BCAE834, 0},
    {6, ParamType::Hash, 0x5BCAE835, 0},  {7, ParamType::Hash, 0x5BCAE836, 0},
    {8, ParamType::Hash, 0x5BCAE837, 0},  {9, ParamType::Hash, 0x760CB725, 0},
    {10, ParamType::Float, 0x000110AD, 0}, {11, ParamType::Float, 0x79792193, 0},
    {12, ParamType::Float, 0x33BEF09B, 0}, {13, ParamType::Int, 0x543CAC2D, 0},
    {14, ParamType::Int, 0x0C605253, 0},  {15, ParamType::Int, 0xF2813FD9, 0},
    {16, ParamType::Int, 0x1B6E6190, 3},
};

constexpr ConfigParam kPunchParams[] = {
    {0, ParamType::Hash, 0xA2A0911A, 0},  {1, ParamType::Hash, 0xAE38532C, 0},
    {2, ParamType::Hash, 0xBB68E539, 0},  {3, ParamType::Hash, 0x760CB725, 0},
    {4, ParamType::Float, 0x000110AD, 0}, {5, ParamType::Float, 0x79792193, 0},
    {6, ParamType::Float, 0x33BEF09B, 0}, {7, ParamType::Float, 0xFEC0361E, 0},
    {8, ParamType::Int, 0x543CAC2D, 0},   {9, ParamType::Int, 0xF2813FD9, 0},
    {11, ParamType::Int, 0x1B6E6190, 3},  {10, ParamType::Int, 0x89EBE64D, 0},
};

constexpr ConfigParam kSlingshotParams[] = {
    {0, ParamType::Hash, 0xCF4A1DD4, 0},  {1, ParamType::Hash, 0x760CB725, 0},
    {2, ParamType::Float, 0x000110AD, 0}, {3, ParamType::Float, 0x79792193, 0},
    {4, ParamType::Hash, 0xA2A0911A, 0},  {5, ParamType::Hash, 0x33BEE3CF, 0},
    {6, ParamType::Hash, 0xB37A3E80, 0},  {7, ParamType::Hash, 0xB37A3E81, 0},
    {8, ParamType::Hash, 0xB37A3E82, 0},  {9, ParamType::Hash, 0x87D98D43, 0},
    {10, ParamType::Hash, 0xBA1A6687, 0}, {11, ParamType::Hash, 0x8A223FF8, 0},
    {12, ParamType::Hash, 0x93116591, 0}, {13, ParamType::Float, 0x33BEF09B, 0},
    {14, ParamType::Int, 0xB390E3A6, 0},  {15, ParamType::Int, 0xBE36FB14, 0},
    {16, ParamType::Int, 0x1B6E6190, 3},  {17, ParamType::Float, 0x4359F70E, 0},
};

// NOTE: word 4 (0x9E635873) is read twice on the console; kept.
constexpr ConfigParam kSumoParams[] = {
    {0, ParamType::Hash, 0xA2A0911A, 0},  {1, ParamType::Hash, 0xCAD37C1E, 0},
    {4, ParamType::Hash, 0x9E635873, 0},  {2, ParamType::Hash, 0xCF4A1DD4, 0},
    {3, ParamType::Float, 0x7F8F56C5, 0}, {4, ParamType::Hash, 0x9E635873, 0},
    {5, ParamType::Hash, 0x760CB725, 0},  {6, ParamType::Float, 0x000110AD, 0},
    {7, ParamType::Float, 0x79792193, 0}, {8, ParamType::Float, 0x71E2D9D1, 0},
    {9, ParamType::Float, 0x0857649E, 0}, {10, ParamType::Float, 0xCAD388EA, 0},
    {11, ParamType::Float, 0xDA3FA842, 0}, {12, ParamType::Float, 0xDA3FA930, 0},
    {15, ParamType::Float, 0x42E92547, 0}, {16, ParamType::Float, 0x0C93BE83, 0},
    {13, ParamType::Float, 0xB6E3CD4B, 0}, {14, ParamType::Float, 0x592BFAF6, 0},
};

constexpr ConfigParam kWhackParams[] = {
    {0, ParamType::Hash, 0xA2A0911A, 0},  {1, ParamType::Hash, 0x60DE0653, 0},
    {2, ParamType::Hash, 0x987C585E, 0},  {3, ParamType::Hash, 0x9AE17203, 0},
    {4, ParamType::Hash, 0x7D754C8B, 0},  {6, ParamType::Hash, 0x760CB725, 0},
    {7, ParamType::Float, 0x000110AD, 0}, {8, ParamType::Float, 0x79792193, 0},
    {9, ParamType::Float, 0xED3A6C6F, 0}, {10, ParamType::Float, 0xEED63AC8, 0},
    {11, ParamType::Float, 0x1C3937F0, 0}, {12, ParamType::Float, 0xB7EA3D94, 0},
    {13, ParamType::Int, 0xD015A596, 0},  {14, ParamType::Int, 0x29DD8505, 0},
    {15, ParamType::Int, 0x1B6E6190, 3},  {5, ParamType::Hash, 0x12380FB5, 0},
};

} // namespace

// @func 0x0032A4D0 GoalPenguinBalanceGame::~GoalPenguinBalanceGame
// @func 0x0032A4FC GoalPenguinBalanceGame::~GoalPenguinBalanceGame (deleting)
GoalPenguinBalanceGame::~GoalPenguinBalanceGame() = default;
// @func 0x0032A560 GoalPenguinBalanceGame::Create
Goal* GoalPenguinBalanceGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinBalanceGame>(xml, desc, kBalanceParams);
}
// @func 0x0032A9C4 GoalPenguinBalanceGame::InstallMinigame
void GoalPenguinBalanceGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x0032AA60 GoalPenguinBalanceGame::BuildMinigame
gameplay::Minigame* GoalPenguinBalanceGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinBalance, game, &config, this);
}

// @func 0x003352C0 GoalPenguinDodgeGame::~GoalPenguinDodgeGame
// @func 0x003352EC GoalPenguinDodgeGame::~GoalPenguinDodgeGame (deleting)
GoalPenguinDodgeGame::~GoalPenguinDodgeGame() = default;
// @func 0x00335350 GoalPenguinDodgeGame::Create
Goal* GoalPenguinDodgeGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinDodgeGame>(xml, desc, kDodgeParams);
}
// @func 0x003356A8 GoalPenguinDodgeGame::InstallMinigame
void GoalPenguinDodgeGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x00335744 GoalPenguinDodgeGame::BuildMinigame
gameplay::Minigame* GoalPenguinDodgeGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinDodge, game, &config, this);
}

// @func 0x003357C8 GoalPenguinPunchGame::~GoalPenguinPunchGame
// @func 0x003357F4 GoalPenguinPunchGame::~GoalPenguinPunchGame (deleting)
GoalPenguinPunchGame::~GoalPenguinPunchGame() = default;
// @func 0x00335858 GoalPenguinPunchGame::Create
Goal* GoalPenguinPunchGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinPunchGame>(xml, desc, kPunchParams);
}
// @func 0x00335B14 GoalPenguinPunchGame::InstallMinigame
void GoalPenguinPunchGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x00335BB0 GoalPenguinPunchGame::BuildMinigame
gameplay::Minigame* GoalPenguinPunchGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinPunch, game, &config, this);
}

// @func 0x00335C34 GoalPenguinSlingshotGame::~GoalPenguinSlingshotGame
// @func 0x00335C60 GoalPenguinSlingshotGame::~GoalPenguinSlingshotGame (deleting)
GoalPenguinSlingshotGame::~GoalPenguinSlingshotGame() = default;
// @func 0x00335CC8 GoalPenguinSlingshotGame::Create
Goal* GoalPenguinSlingshotGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinSlingshotGame>(xml, desc, kSlingshotParams);
}
// @func 0x00336060 GoalPenguinSlingshotGame::InstallMinigame
void GoalPenguinSlingshotGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x003360FC GoalPenguinSlingshotGame::BuildMinigame
gameplay::Minigame* GoalPenguinSlingshotGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinSlingshot, game, &config, this);
}

// @func 0x00336180 GoalPenguinSumoGame::~GoalPenguinSumoGame
// @func 0x003361AC GoalPenguinSumoGame::~GoalPenguinSumoGame (deleting)
GoalPenguinSumoGame::~GoalPenguinSumoGame() = default;
// @func 0x00336210 GoalPenguinSumoGame::Create
Goal* GoalPenguinSumoGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinSumoGame>(xml, desc, kSumoParams);
}
// @func 0x003365C4 GoalPenguinSumoGame::InstallMinigame
void GoalPenguinSumoGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x00336660 GoalPenguinSumoGame::BuildMinigame
gameplay::Minigame* GoalPenguinSumoGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinSumo, game, &config, this);
}

// @func 0x00338D58 GoalPenguinWhackGame::~GoalPenguinWhackGame
// @func 0x00338D84 GoalPenguinWhackGame::~GoalPenguinWhackGame (deleting)
GoalPenguinWhackGame::~GoalPenguinWhackGame() = default;
// @func 0x00338DE8 GoalPenguinWhackGame::Create
Goal* GoalPenguinWhackGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    return CreatePenguinGoal<GoalPenguinWhackGame>(xml, desc, kWhackParams);
}
// @func 0x00339128 GoalPenguinWhackGame::InstallMinigame
void GoalPenguinWhackGame::InstallMinigame()
{
    GameInstallMinigame(game, BuildMinigame());
}
// @func 0x003391C4 GoalPenguinWhackGame::BuildMinigame
gameplay::Minigame* GoalPenguinWhackGame::BuildMinigame()
{
    return CreateMinigame(MinigameKind::PenguinWhack, game, &config, this);
}

// ===========================================================================
// Fencing

// @func 0x0033070C GoalFencing::~GoalFencing
// @func 0x00330738 GoalFencing::~GoalFencing (deleting)
GoalFencing::~GoalFencing() = default;

// @func 0x00330798 GoalFencing::Create
Goal* GoalFencing::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    int32_t a = 0;
    uint32_t b = 0;
    float c = 4.0f;
    xml->GetInt(0x04B8CADEu, &a, 0);
    xml->GetHash(0x922D3BD1u, &b, 0);
    xml->GetFloat(0x39FB22A9u, &c, 4.0f);
    GoalFencing* goal = new GoalFencing(desc);
    goal->field_A14 = a;
    goal->field_A18 = b;
    goal->field_A1C = c;
    return goal;
}

// @func 0x003308B0 GoalFencing::InstallMinigame
void GoalFencing::InstallMinigame()
{
    GameInstallMinigame(game, CreateMinigame(MinigameKind::Fencing, game, &field_A14, this));
}

// ===========================================================================
// Fishing, QTE, bartender (configuration parsers not ported)

// @func 0x00330958 GoalFishingGame::GoalFishingGame
GoalFishingGame::GoalFishingGame(const GoalDesc* desc, const FishingConfig& parsed)
    : MinigameGoal(desc), config(parsed)
{
    minigame = nullptr;
    result = 0;
}

namespace {
constexpr uint32_t kFish_Camera = 0x760CB725u;     // "Camera"
constexpr uint32_t kFish_Fish = 0x75181ABAu;       // NOTE: element name unknown
constexpr uint32_t kFish_General = 0x260494C8u;    // "General"
constexpr uint32_t kFish_Track = 0x04C5F92Bu;      // "Track"
constexpr uint32_t kFish_Input = 0x04295FAAu;      // "Input"
constexpr uint32_t kFish_Move = 0x0020EE58u;       // NOTE: element name unknown
constexpr uint32_t kFish_Capture = 0x0001151Bu;    // NOTE: element name unknown

void ReadVec3(ape::xml::XmlElement* xml, uint32_t attribute, float out[3])
{
    const float current[3] = {out[0], out[1], out[2]};
    xml->GetVec3(attribute, out, current);
}

void ReadVec2(ape::xml::XmlElement* xml, uint32_t attribute, float out[2])
{
    const float current[2] = {out[0], out[1]};
    xml->GetVec2(attribute, out, current);
}
} // namespace

// @func 0x00330C88 GoalFishingGame::Create
Goal* GoalFishingGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    FishingConfig config;
    for (ape::xml::XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t element = child->IsValid() ? child->GetNameHash() : 0;
        switch (element) {
        case kFish_Camera:
            ReadVec3(child, 0x8ACA5893u, config.cameraOffset);
            ReadVec3(child, 0x84427283u, config.camera_84427283);
            child->GetHash(0x7D22BC63u, &config.cameraStartDef, 0);
            child->GetFloat(0xEE3F6AE0u, &config.easeInTime, 1.0f);
            child->GetFloat(0x230A334Du, &config.easeOutTime, 0.25f);
            child->GetFloat(0x54B94DD1u, &config.panInterp, 0.15f);
            child->GetFloat(0x000110ADu, &config.fov, 50.0f);
            child->GetFloat(0xDF8F4601u, &config.fovInterp, 0.4f);
            child->GetFloat(0x0460BCBDu, &config.camera_0460BCBD, 25.0f);
            break;
        case kFish_Fish:
            child->GetHash(0x79F40122u, &config.entityDef, 0);
            child->GetFloat(0x25DC2804u, &config.speedThreshold, 10.0f);
            child->GetHash(0xA8FE6D16u, &config.detectPart, 0);
            child->GetFloat(0x1F434EECu, &config.visualYOffset, 0.0f);
            child->GetHash(0xBC0FA89Fu, &config.fish_BC0FA89F, 0);
            child->GetHash(0x708ACE28u, &config.fish_708ACE28, 0);
            break;
        case kFish_General:
            child->GetHash(0x9D597CD8u, &config.general_9D597CD8, 0);
            child->GetHash(0xCF4A1DD4u, &config.lifetimeScript, 0);
            child->GetBool(0x60FC5D53u, &config.general_60FC5D53, false);
            child->GetInt(0xE2AB5482u, &config.general_E2AB5482, 0);
            break;
        case kFish_Track:
            child->GetFloat(0xE965B9B4u, &config.track_E965B9B4, 0.0f);
            for (ape::xml::XmlElement* band = child->FirstChildElement(); band; band = band->NextSiblingElement()) {
                const uint32_t name = band->IsValid() ? band->GetNameHash() : 0;
                int index = -1;
                if (name == 0x00012734u)        // "low"
                    index = 0;
                else if (name == 0x000129ACu)   // NOTE: "med"?
                    index = 1;
                else if (name == 0x0021D5A2u)   // "high"
                    index = 2;
                if (index < 0)
                    continue;
                float& value = config.trackThresholds[index];
                band->GetFloat(0x4BF7A17Au, &value, value);
                value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
            }
            break;
        case kFish_Input:
            ReadVec2(child, 0xE3848D9Fu, config.inputRangeMin);
            ReadVec2(child, 0xE3848CB1u, config.inputRangeMax);
            ReadVec2(child, 0x2443DEE4u, config.input_2443DEE4);
            child->GetFloat(0x4343FBECu, &config.input_4343FBEC, 2.0f);
            child->GetBool(0xD671941Cu, &config.input_D671941C, false);
            break;
        case kFish_Move:
            ReadVec3(child, 0x9B49D0E5u, config.move_9B49D0E5);
            ReadVec3(child, 0xC629C924u, config.move_C629C924);
            if (child->GetFloat(0xD19F7151u, &config.trackDistSq, config.trackDistSq) == 0)
                config.trackDistSq *= config.trackDistSq;
            child->GetFloat(0xE8F7013Bu, &config.moveYOffset, 0.0f);
            child->GetFloat(0x4A78C8CEu, &config.trackSpeedScale, 1.0f);
            child->GetFloat(0x810AA162u, &config.move_810AA162, 0.1f);
            child->GetFloat(0xAE23BF2Fu, &config.move_AE23BF2F, 0.0f);
            child->GetFloat(0x660103CBu, &config.maxStunTime, 0.0f);
            child->GetFloat(0xA4C44A18u, &config.move_A4C44A18, 0.0f);
            child->GetHash(0x31F8E7E5u, &config.idleAnim, 0);
            child->GetHash(0x72BEA0EFu, &config.move_72BEA0EF, 0);
            break;
        case kFish_Capture:
            child->GetHash(0xB908B96Bu, &config.capture_B908B96B, 0);
            child->GetHash(0x521131D6u, &config.capture_521131D6, 0);
            if (const char* capture = child->GetString("captureStr")) {
                if (*capture) {
                    std::strncpy(config.captureStr, capture, 0x40);
                    config.captureStr[0x40] = '\0';
                }
            }
            break;
        default:
            break;
        }
    }
    return new GoalFishingGame(desc, config);
}

// @func 0x00330BF4 GoalFishingGame::~GoalFishingGame
// @func 0x00330C20 GoalFishingGame::~GoalFishingGame (deleting)
GoalFishingGame::~GoalFishingGame() = default;

// @func 0x00331590 GoalFishingGame::InstallMinigame
void GoalFishingGame::InstallMinigame()
{
    GameInstallMinigame(game, CreateMinigame(MinigameKind::Fishing, game, &config, this));
}

// @func 0x0033A308 GoalQteGame::~GoalQteGame
// @func 0x0033A4C4 GoalQteGame::~GoalQteGame (deleting)
GoalQteGame::~GoalQteGame() = default;

// @func 0x0033B91C GoalQteGame::InstallMinigame
void GoalQteGame::InstallMinigame()
{
    GameInstallMinigame(game, CreateMinigame(MinigameKind::Qte, game, &steps, this));
}

// @func 0x00328D24 GoalBartenderGame::GoalBartenderGame
GoalBartenderGame::GoalBartenderGame(const GoalDesc* desc, const Config& parsed,
                                     const ape::ApeTArray<Entry>& slots)
    : MinigameGoal(desc), config(parsed), entries(16)
{
    minigame = nullptr;
    result &= ~(kResult_Won | kResult_Lost);
    for (uint32_t i = 0; i < slots.Count(); ++i)
        entries.PushBack(slots[i]);
}

// @func 0x003291DC GoalBartenderGame::~GoalBartenderGame
// @func 0x00329240 GoalBartenderGame::~GoalBartenderGame (deleting)
GoalBartenderGame::~GoalBartenderGame() = default;

namespace {
constexpr uint32_t kBar_General = 0x260494C8u;     // "General"
constexpr uint32_t kBar_Player = 0x8CD18EE1u;      // "Player"
constexpr uint32_t kBar_Camera = 0x760CB725u;      // "Camera"
constexpr uint32_t kBar_Input = 0x04295FAAu;       // "Input"
constexpr uint32_t kBar_Beer = 0x001F0C30u;        // "Beer"
constexpr uint32_t kBar_Hud = 0x000118D7u;         // "hud"
constexpr uint32_t kBar_Slot = 0x0026E1FEu;        // "SLOT"
constexpr uint32_t kBar_StartDef = 0x7D22BC63u;    // "StartDef"
constexpr uint32_t kBar_MaxSlots = 16;
constexpr float kDegToRad = 0.017453292f;

// Difficulty block of a <threshold...> element, -1 if the name is not one.
int ThresholdIndex(uint32_t name)
{
    switch (name) {
    case 0x24FD94A9u: return 0;   // "thresholdTutorial"
    case 0x182132CDu: return 1;   // "thresholdEasy"
    case 0xB674CD81u: return 2;   // "thresholdMed"
    case 0x18228FB6u: return 3;   // "thresholdHard"
    case 0x9C29F735u: return 4;   // "thresholdInsane"
    default: return -1;
    }
}

// Customer block of an <Enemy...> element, -1 if the name is not one.
int CustomerIndex(uint32_t name)
{
    switch (name) {
    case 0x8A2B267Fu: return 0;   // "EnemySmall"
    case 0x97FC3944u: return 1;   // "EnemyMed"
    case 0x89C34CB3u: return 2;   // "EnemyLarge"
    default: return -1;
    }
}

void ReadHash(ape::xml::XmlElement* xml, uint32_t attribute, uint32_t& value)
{
    xml->GetHash(attribute, &value, value);
}

void ReadFloat(ape::xml::XmlElement* xml, uint32_t attribute, float& value)
{
    xml->GetFloat(attribute, &value, value);
}

void ReadInt(ape::xml::XmlElement* xml, uint32_t attribute, int32_t& value)
{
    xml->GetInt(attribute, &value, value);
}

// Reads a vector attribute in place; true if the attribute was there.
bool ReadVec3InPlace(ape::xml::XmlElement* xml, uint32_t attribute, ape::Vec3& value)
{
    float v[3] = {value.x, value.y, value.z};
    const bool found = xml->GetVec3(attribute, v, v) == ape::xml::kQuery_Ok;
    value.x = v[0];
    value.y = v[1];
    value.z = v[2];
    return found;
}

void ReadAngles(ape::xml::XmlElement* xml, uint32_t attribute, ape::Vec3& value)
{
    if (ReadVec3InPlace(xml, attribute, value)) {
        value.x *= kDegToRad;
        value.y *= kDegToRad;
        value.z *= kDegToRad;
    }
}
} // namespace

// @func 0x003292C8 GoalBartenderGame::Create
Goal* GoalBartenderGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    Config config;
    ape::ApeTArray<Entry> slots(16);

    for (ape::xml::XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t element = child->IsValid() ? child->GetNameHash() : 0;

        if (int index = ThresholdIndex(element); index >= 0) {
            Config::Threshold& t = config.thresholds[index];
            ReadFloat(child, 0x3FBAC754u, t.valueMin);
            ReadFloat(child, 0x3FBAC666u, t.valueMax);
            ReadInt(child, 0x5D241D8Fu, t.field_08);
            ReadInt(child, 0xEE981F14u, t.field_0C);
            continue;
        }
        if (int index = CustomerIndex(element); index >= 0) {
            Config::Customer& c = config.customers[index];
            ReadVec3InPlace(child, 0xC2ABB83Eu, c.position);
            ReadHash(child, kBar_StartDef, c.startDef);
            ReadHash(child, 0x17A49D68u, c.field_14);
            ReadHash(child, 0x474E5E89u, c.field_18);
            ReadFloat(child, 0x6E89F748u, c.field_1C);
            ReadHash(child, 0x41074EEFu, c.field_20);
            ReadHash(child, 0x31F8E7E5u, c.idleAnim);
            ReadHash(child, 0x2449B426u, c.field_28);
            ReadHash(child, 0x2AD2F67Cu, c.outroAnim);
            ReadFloat(child, 0x67F55F35u, c.outroAnimBlend);
            ReadHash(child, 0xDA10225Du, c.introAnim);
            ReadFloat(child, 0x972D77F4u, c.introAnimBlend);
            ReadFloat(child, 0xDA18B319u, c.introTime);
            ReadFloat(child, 0x2ADB8738u, c.outroTime);
            ReadFloat(child, 0x4756EF45u, c.field_44);
            ReadInt(child, 0x71782B46u, c.countMin);
            ReadInt(child, 0x71782A58u, c.countMax);
            ReadFloat(child, 0x1CA14B34u, c.delayMin);
            ReadFloat(child, 0x1CA14A46u, c.delayMax);
            continue;
        }

        switch (element) {
        case kBar_General: {
            const char* text = child->GetString("globalImpatienceStr");
            if (text && *text) {
                std::strncpy(config.globalImpatienceStr, text, sizeof(config.globalImpatienceStr) - 1);
                config.globalImpatienceStr[sizeof(config.globalImpatienceStr) - 1] = '\0';
            }
            ReadHash(child, 0xCF4A1DD4u, config.lifetimeScript);
            ReadFloat(child, 0xE879F570u, config.general_E879F570);
            ReadInt(child, 0x251D3437u, config.general_251D3437);
            ReadInt(child, 0x1051D289u, config.general_1051D289);
            ReadFloat(child, 0xCDBE582Du, config.general_CDBE582D);
            child->GetBool(0xCD9D94BEu, &config.general_CD9D94BE, config.general_CD9D94BE);
            break;
        }
        case kBar_Player:
            ReadHash(child, kBar_StartDef, config.playerStartDef);
            ReadFloat(child, 0xE6DF8EF6u, config.player_E6DF8EF6);
            ReadHash(child, 0x31F8E7E5u, config.playerIdleAnim);
            ReadHash(child, 0xDF38C969u, config.player_DF38C969);
            ReadHash(child, 0x4271AD5Cu, config.player_4271AD5C);
            ReadFloat(child, 0xE3997805u, config.player_E3997805);
            ReadHash(child, 0x5B3F95D7u, config.player_5B3F95D7);
            ReadFloat(child, 0x89CE80B0u, config.player_89CE80B0);
            ReadHash(child, 0x5533B0E5u, config.player_5533B0E5);
            if (config.player_E6DF8EF6 < 0.0f)
                config.player_E6DF8EF6 = 0.0f;
            break;
        case kBar_Camera:
            ReadHash(child, kBar_StartDef, config.cameraStartDef);
            ReadFloat(child, 0x000110ADu, config.cameraFov);
            break;
        case kBar_Input:
            ReadFloat(child, 0x59317222u, config.input[0]);
            ReadFloat(child, 0xD8159D71u, config.input[1]);
            ReadFloat(child, 0x3AE281B8u, config.input[2]);
            ReadFloat(child, 0x6F8C1771u, config.input[3]);
            break;
        case kBar_Beer:
            ReadVec3InPlace(child, 0xA1235AD9u, config.beer_A1235AD9);
            ReadVec3InPlace(child, 0x4036DEFBu, config.beer_4036DEFB);
            ReadAngles(child, 0x8E871BCFu, config.beerAngleMin);
            ReadAngles(child, 0x8E871AE1u, config.beerAngleMax);
            ReadHash(child, kBar_StartDef, config.beerStartDef);
            ReadHash(child, 0xA7E293B8u, config.beer_A7E293B8);
            ReadFloat(child, 0x04B70827u, config.beer_04B70827);
            break;
        case kBar_Hud: {
            static const uint32_t kHudAttributes[10] = {
                0xD022E3C4u, 0xC0FBF301u, 0x5A3AC546u, 0x8A6C8565u, 0x42C471A7u,
                0xFCD882C9u, 0x3C21F2EEu, 0xBD2FAFC8u, 0x4B0999AEu, 0x2D4954B6u,
            };
            for (int i = 0; i < 10; ++i)
                ReadHash(child, kHudAttributes[i], config.hud[i]);
            for (int i = 0; i < 3; ++i) {
                char name[32];
                std::snprintf(name, sizeof(name), "thoughtBubbleBeerPart%i", i + 1);
                ReadHash(child, ape::HashNameNoCase(name), config.thoughtBubbleBeerPart[i]);
            }
            break;
        }
        case kBar_Slot: {
            if (slots.Count() >= kBar_MaxSlots)
                break;
            Entry entry{0, 0, 0};
            child->GetHash(0x1D8E76ACu, &entry.a, 0);
            child->GetHash(0xB68383F7u, &entry.b, entry.b);
            if (entry.a == 0 || entry.b == 0)
                break;
            child->GetHash(0xDC0D416Eu, &entry.c, 0);
            bool isDefault = false;
            child->GetBool(0x6CE66A2Du, &isDefault, false);
            if (isDefault)
                config.defaultSlot = slots.Count();
            slots.PushBack(entry);
            break;
        }
        default:
            break;
        }
    }
    return new GoalBartenderGame(desc, config, slots);
}

// @func 0x0032A27C GoalBartenderGame::InstallMinigame
void GoalBartenderGame::InstallMinigame()
{
    // NOTE: the console also hands over `entries` (+0xCB0).
    GameInstallMinigame(game, CreateMinigame(MinigameKind::Bartender, game, &config, this));
}

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// ===========================================================================
// Penguin montage

bool GoalPenguinMontageGame::s_roundLost = false;

// @func 0x00339248 GoalPenguinMontageGame::~GoalPenguinMontageGame
// @func 0x003392D0 GoalPenguinMontageGame::~GoalPenguinMontageGame (deleting)
GoalPenguinMontageGame::~GoalPenguinMontageGame() = default;

// @func 0x00339388 GoalPenguinMontageGame::ParseRound
void GoalPenguinMontageGame::ParseRound(ape::xml::XmlElement* first, Round& round, const GoalDesc* desc)
{
    for (ape::xml::XmlElement* child = first; child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        Goal* goal = nullptr;
        switch (child->GetNameHash()) {
        case 0x618B173Du: goal = GoalPenguinWhackGame::Create(child, desc); break;
        case 0x17DEB587u: goal = GoalPenguinPunchGame::Create(child, desc); break;
        case 0xF54BD90Fu: goal = GoalPenguinSumoGame::Create(child, desc); break;
        case 0xD4D8DBBCu: goal = GoalPenguinSlingshotGame::Create(child, desc); break;
        case 0xAB0D3FB0u: goal = GoalPenguinDodgeGame::Create(child, desc); break;
        case 0x9F453A75u: goal = GoalPenguinBalanceGame::Create(child, desc); break;
        case 0x001F9B50u: {
            RoundEntry entry;
            child->GetHash(0x0024728Bu, &entry.name, 0);        // "Name"
            child->GetFloat(0x98643678u, &entry.weight, 0.0f);  // "Weight"
            round.entries.PushBack(entry);
            continue;
        }
        default:
            continue;
        }
        if (!goal)
            continue;
        SubGame* game = new SubGame();
        game->goal = goal;
        child->GetHash(0x46C3D78Du, &game->field_04, 0);
        child->GetHash(0x3BAE1946u, &game->field_08, 0);
        // NOTE: the console does not bound the count; six fit.
        if (round.gameCount < kMaxSubGames)
            round.games[round.gameCount++] = game;
        else
            delete game;
    }
}

// @func 0x003396A8 GoalPenguinMontageGame::Create
Goal* GoalPenguinMontageGame::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t lifetimeScript = 0;
    xml->GetHash(0xCF4A1DD4u, &lifetimeScript, 0);   // "LifetimeScript"
    GoalPenguinMontageGame* goal = new GoalPenguinMontageGame(desc);
    goal->minigame = nullptr;
    goal->result &= ~(kResult_Won | kResult_Lost);
    goal->field_A14 = lifetimeScript;
    for (ape::xml::XmlElement* child = xml->FirstChildElement(0xF09E81B8u); child;
         child = child->NextSiblingElement(0xF09E81B8u)) {
        // NOTE: the console does not bound the round count either.
        if (goal->roundCount >= kRoundCount)
            break;
        Round& round = goal->rounds[goal->roundCount];
        child->GetFloat(0xA9597E7Du, &round.field_34, 1.0f);
        child->GetInt(0x00012F66u, &round.field_38, 0);
        ParseRound(child->FirstChildElement(), round, desc);
        ++goal->roundCount;
    }
    return goal;
}

// @func 0x00339998 GoalPenguinMontageGame::SetStatus
void GoalPenguinMontageGame::SetStatus(uint8_t newStatus, bool force)
{
    if (newStatus == kGoal_Active)
        s_roundLost = false;
    if (newStatus == kGoal_Complete && !s_roundLost)
        GoalAwardTrophy(kTrophy_NoLoss);
    // Same minigame handling as MinigameGoal::SetStatus (inlined on the
    // console), then the base status change.
    if (force || newStatus != status) {
        if (newStatus == kGoal_Active) {
            if (!GameActiveMinigame(game)) {
                InstallMinigame();
                minigame = GameActiveMinigame(game);
                if (!force)
                    MinigameStart(minigame);
            }
        } else if (newStatus != kGoal_Complete && newStatus != kGoal_Failed) {
            if (minigame && GameActiveMinigame(game) == minigame)
                GameEndActiveMinigame(game);
        }
    }
    Goal::SetStatus(newStatus, force);
}

// @func 0x00339B14 GoalPenguinMontageGame::InstallMinigame
void GoalPenguinMontageGame::InstallMinigame()
{
    GameInstallMinigame(game, CreateMinigame(MinigameKind::PenguinMontage, game, &field_A14, this));
}

} // namespace goals
} // namespace sly
