// openTIT - Goal XML parser (0x00321D3C-0x003248B0).
#include "sly/game/goals/GoalParser.h"
#include "sly/game/goals/GoalSpawnerHunt.h"
#include "sly/game/goals/GoalShootingGallery.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"
#include "sly/game/Upgrades.h"
#include "sly/game/goals/GoalTypes.h"
#include "sly/levels/script/ScriptCommand.h"

#include <cstdio>
#include <cstring>

namespace sly {
namespace goals {

namespace {

using ape::xml::XmlElement;

constexpr int kMaxScriptsPerSlot = 10;
constexpr uint32_t kMaxHints = 0x20;
constexpr int kMaxLinkedGoals = 4;
constexpr int kCheckpointCount = 9;

// Group operators ("op"), in GoalGroup::Mode order.
constexpr uint32_t kGroupOps[7] = {
    0x0024A738,   // "NONE"
    0x0000FDB7,   // "AND"   kMode_All
    0x002472A9,   // "NAND"  kMode_NotAll
    0x000009E3,   // "OR"    kMode_Any
    0x00012EB1,   // "NOR"   kMode_None
    0x0000FD81,   // "ALL"   kMode_AllProgress
    0x13234CFF,   // "Sequential"
};
constexpr uint32_t kAttr_Op = 0x000009E1;                    // "op"
constexpr uint32_t kAttr_MaxTargets = 0xA3F9EF1E;            // "MaxTargets"
constexpr uint32_t kAttr_GroupNotify = 0xA7BC1270;           // NOTE: name unknown
constexpr uint32_t kAttr_GroupWaitScripts = 0x8E0B3401;      // NOTE: name unknown
constexpr uint32_t kAttr_GroupStopOnCompletion = 0x9412EDCD; // NOTE: name unknown

// Common goal attributes.
constexpr uint32_t kAttr_MinCount = 0xDD50BDFD;              // "MinCount"
constexpr uint32_t kAttr_TargetName = 0xCC153529;            // NOTE: "GuidanceSpline"?
constexpr uint32_t kAttr_TargetFlag = 0x49D7FB54;            // NOTE: name unknown
constexpr uint32_t kAttr_Field20 = 0x343CE318;               // NOTE: name unknown
constexpr uint32_t kAttr_CounterType = 0x11CE1B16;           // "CounterType"
constexpr uint32_t kAttr_CounterOffset = 0xCD1B656F;         // "CounterOffset"
constexpr uint32_t kAttr_LifetimeScript = 0xCF4A1DD4;        // "LifetimeScript"
constexpr uint32_t kAttr_Field30 = 0x95098150;               // NOTE: name unknown
constexpr uint32_t kAttr_Field31 = 0xC4CE9D6F;               // NOTE: name unknown
constexpr uint32_t kAttr_CompletesParent = 0x0B63B076;       // NOTE: name unknown
constexpr uint32_t kAttr_ShowCounterTotal = 0x28D7A4A5;      // NOTE: name unknown
constexpr uint32_t kAttr_FailRestarts = 0x0CB424BF;          // NOTE: name unknown
constexpr uint32_t kAttr_AssociatedLevel = 0xD661961E;       // "associatedLevel"
constexpr uint32_t kAttr_AssociatedDef = 0x2584611F;         // "associatedDef"
constexpr uint32_t kAttr_FieldE8 = 0x0F61FBD2;               // NOTE: name unknown
constexpr uint32_t kAttr_ShowDescription = 0xCBBA30AD;       // NOTE: name unknown
constexpr uint32_t kAttr_Field130 = 0xBE8B7B06;              // NOTE: name unknown (int, >= 0)
constexpr uint32_t kAttr_HintChance = 0x74BC9C7B;            // "hintChance"
constexpr uint32_t kAttr_DisplayTime = 0xE690CE6F;           // "DisplayTime"

// Goal flag attributes (Goal::flags bit by bit).
struct FlagAttribute {
    uint32_t attribute;
    uint32_t flag;
    bool defaultValue;
};
constexpr FlagAttribute kFlagAttributes[] = {
    {0x301E4C6B, 0x001, false},   // "inactive"
    {0x7F0191AA, 0x002, false},   // NOTE: name unknown (shown as "???")
    {0x918E42B0, 0x004, false},   // "Secret"
    {0x19337BA2, 0x008, false},   // "autoActivate"
    {0x3B757A97, 0x010, false},   // "Persistent"
    {0xB9E753E3, 0x040, false},   // "Stealth"
    {0x63E2E1CE, 0x100, true},    // "Counted"
};
constexpr uint32_t kFlag_HasAssociatedPlayer = 0x20;

// Checkpoints: attribute names of the first checkpoint (the others use
// "<name>%d" with numbers from 2).
constexpr uint32_t kAttr_CheckPoint = 0xFEF24188;            // "checkPoint"
constexpr uint32_t kAttr_RespawnAtEntity = 0x9AD77EDE;
constexpr uint32_t kAttr_RespawnAtEntityYOffset = 0xAD789D8E;
constexpr uint32_t kAttr_RespawnAtCheckPoint = 0x8DA6F303;
constexpr uint32_t kAttr_AttachToSplineAtCheckPoint = 0x08127FCC;
constexpr uint32_t kAttr_TeleportToCheckPoint = 0xD5422CEE;
constexpr uint32_t kAttr_AttachToWallAtCheckPoint = 0xAB118045;
constexpr uint32_t kAttr_AssociatedPlayer = 0xFCFDF367;

// Script attributes: name, slot, sequence ("Cutscene") or anim script, restart.
struct ScriptAttribute {
    uint32_t attribute;
    int slot;
    bool isSequence;
    bool restart;
};
constexpr ScriptAttribute kScriptAttributes[] = {
    {0xA62F3A7D, kScripts_Active, true, true},     // "CutsceneActivate"
    {0x94FE720C, kScripts_Active, true, false},    // NOTE: name unknown
    {0xE52896C3, kScripts_Complete, true, false},  // "CutsceneComplete"
    {0xA8BA99A8, kScripts_Failed, true, false},    // "CutsceneFail"
    {0x5CAA8EBE, kScripts_Active, false, false},   // "ScriptActivate"
    {0x9BA3EB04, kScripts_CompleteIdle, false, false},   // "ScriptComplete" (also slot 2)
    {0xEAC43669, kScripts_Failed, false, false},   // "ScriptFail"
};
constexpr uint32_t kAttr_ScriptComplete = 0x9BA3EB04;

// Child elements.
constexpr uint32_t kElem_Script = 0x9178C4CB;         // "Script"
constexpr uint32_t kElem_LinkedGoal = 0xC03D4AB6;     // NOTE: name unknown; up to four "Goal" names
constexpr uint32_t kElem_BinocucomText = 0x1EE9D214;  // "BinocucomText" (cycling texts)
constexpr uint32_t kAttr_Name = 0x0024728B;           // "Name"
constexpr uint32_t kAttr_Goal = 0x00217713;           // "Goal"
constexpr uint32_t kAttr_Type = 0x0027873A;           // "Type"
constexpr uint32_t kAttr_IsCutscene = 0x0F89DEB4;     // "isCutscene"
constexpr uint32_t kAttr_PlayOnce = 0x9EADDFB5;       // "PlayOnce"
constexpr uint32_t kAttr_Disable = 0x8E69AEC8;        // "disable"
constexpr uint32_t kAttr_Persistent = 0x3B757A97;     // "Persistent"
constexpr uint32_t kAttr_HintKeepAlive = 0x6557DBDD;  // NOTE: name unknown

// <Script Type=...>: script slots for each type (-1 = none).
constexpr uint32_t kType_OnActivate = 0x5B757FD2;     // "OnActivate"
constexpr uint32_t kType_OnFail = 0x8B3AD17D;         // "OnFail"
constexpr uint32_t kType_OnComplete = 0x9A6EDC18;     // "OnComplete"
constexpr uint32_t kType_3 = 0x41C1DBFE;              // NOTE: name unknown (slot 3)
constexpr uint32_t kType_4 = 0x02C87FB8;              // NOTE: name unknown (slot 4)
constexpr uint32_t kType_CompleteBoth = 0x771B9729;   // NOTE: name unknown (slots 2 and 3)
constexpr uint32_t kType_ActivateBoth = 0x2FE786E3;   // NOTE: name unknown (slots 0 and 3)

// Hint elements and the kind each one makes.
struct HintElement {
    uint32_t element;
    int32_t kind;
};
constexpr HintElement kHintElements[] = {
    {0xEB9EE198, kHint_Entity},         // "trigger"
    {0xEE3E62C2, kHint_Interaction},    // "Interactive"
    {0xB2E59948, kHint_Spawner},        // "Spawner"
    {0xE89DF217, kHint_SpawnGroup},     // "SpawnerGroup"
    {0x044DC6AB, kHint_Laser},          // "laser"
    {0xAB27550F, kHint_LaserField},     // "LaserField"
    {0xAB39AA94, kHint_LaserGroup},     // "laserGroup"
    {0xC475D118, kHint_Interactable},   // "teleporter"
};

// Goal element names and their factories.
using GoalFactory = Goal* (*)(XmlElement* xml, const GoalDesc* desc);
struct GoalElement {
    uint32_t element;
    GoalFactory create;
};
const GoalElement kGoalElements[] = {
    {0x9F453A75, GoalPenguinBalanceGame::Create},
    {0x9E69AF49, GoalSpawnerHunt::Create},
    {0x967EAB30, GoalPenguinMontageGame::Create},
    {0x8E1A6F4D, GoalCostume::Create},
    {0xBF16DC69, GoalFishingGame::Create},
    {0xAC477985, GoalBentleyVsPenelope::Create},
    {0xAB0D3FB0, GoalPenguinDodgeGame::Create},
    {0xA78D932B, GoalHealthMonitor::Create},
    {0x00288975, GoalWait::Create},               // "Wait"
    {0xF54BD90F, GoalPenguinSumoGame::Create},
    {0xD4D8DBBC, GoalPenguinSlingshotGame::Create},
    {0xC766004C, GoalInteraction::Create},
    {0x1A2AB2B1, GoalBinocucomPower::Create},
    {0x1A2767DE, GoalBinocucomPhoto::Create},
    {0x17DEB587, GoalPenguinPunchGame::Create},
    {0x11F2CC84, GoalFollow::Create},
    {0x225F0BF2, GoalTimer::Create},
    {0x224E433F, GoalScore::Create},                // "GoalScore"
    {0x21C65C77, GoalInput::Create},
    {0x1B6E4E1B, GoalFencing::Create},
    {0x4226626F, GoalShootingGallery::Create},    // "ShootingGallery"
    {0x36610C4F, CreateGoalSet},                  // "GoalSet"
    {0x2DFF7805, GoalBinocucomRecord::Create},
    {0x5F3BC8F0, GoalMarkEntity::Create},
    {0x49836CC6, GoalBartenderGame::Create},
    {0x44390A06, GoalEntityChild::Create},
    {0x7A6E6B01, CreateQteGoal},                  // "GoalQTEGame"
    {0x70DEDF48, GoalCameraEntity::Create},
    {0x6AB6FFCB, GoalOperationMousetrap::Create},   // "OperationMousetrap"
    {0x618B173D, GoalPenguinWhackGame::Create},
};
// Elements that are recognised but make no goal.
constexpr uint32_t kNoGoalElements[] = {
    0x3660E4E2,   // NOTE: "GoalHUB"?
    0x5CEC06CA,   // NOTE: name unknown
};
// Child elements of a goal that are skipped when they appear as goals.
constexpr uint32_t kGoalChildElements[] = {
    0xC475D118, 0xC03D4AB6, 0xB2E59948, 0x9178C4CB, 0x1EE9D214, 0xEE3E62C2, 0xEB9EE198, 0xE89DF217,
};

bool Contains(const uint32_t* list, size_t count, uint32_t value)
{
    for (size_t i = 0; i < count; ++i)
        if (list[i] == value)
            return true;
    return false;
}

uint32_t ElementName(const XmlElement* xml) { return xml->IsValid() ? xml->GetNameHash() : 0; }

void SetBit(GoalBlob& blob, int bit)
{
    blob.data[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}

// Reads one checkpoint: index 0 uses the plain attribute names, the others
// the numbered names.
void ParseCheckpoint(XmlElement* xml, GoalDesc& desc, int i)
{
    uint32_t* const names = desc.field_70;            // checkpoint names [0..8]
    uint32_t* const respawnEntity = desc.field_70 + 9;
    float yOffset = 0.0f;
    bool respawnAt = false, attachToSpline = false, teleportTo = false, attachToWall = false;
    if (i == 0) {
        xml->GetHash(kAttr_CheckPoint, &names[0], 0);
        xml->GetHash(kAttr_RespawnAtEntity, &respawnEntity[0], 0);
        xml->GetFloat(kAttr_RespawnAtEntityYOffset, &yOffset, 0.0f);
        xml->GetBool(kAttr_RespawnAtCheckPoint, &respawnAt, false);
        xml->GetBool(kAttr_AttachToSplineAtCheckPoint, &attachToSpline, false);
        xml->GetBool(kAttr_TeleportToCheckPoint, &teleportTo, false);
        xml->GetBool(kAttr_AttachToWallAtCheckPoint, &attachToWall, false);
        if (xml->GetHash(kAttr_AssociatedPlayer, &desc.checkpoints[0], desc.checkpoints[0]) == 0)
            desc.flags |= kFlag_HasAssociatedPlayer;
    } else {
        const int n = i + 1;
        char name[64];
        std::snprintf(name, sizeof name, "checkPoint%d", n);
        xml->GetHash(name, &names[i], 0);
        std::snprintf(name, sizeof name, "respawnAtEntity%d", n);
        xml->GetHash(name, &respawnEntity[i], 0);
        std::snprintf(name, sizeof name, "respawnAtEntityYOffset%d", n);
        xml->GetFloat(name, &yOffset, 0.0f);
        std::snprintf(name, sizeof name, "respawnAtCheckPoint%d", n);
        xml->GetBool(name, &respawnAt, false);
        std::snprintf(name, sizeof name, "attachToSplineAtCheckPoint%d", n);
        xml->GetBool(name, &attachToSpline, false);
        std::snprintf(name, sizeof name, "teleportToCheckPoint%d", n);
        xml->GetBool(name, &teleportTo, false);
        std::snprintf(name, sizeof name, "associatedPlayer%d", n);
        xml->GetHash(name, &desc.checkpoints[i], 0);
        std::snprintf(name, sizeof name, "attachToWallAtCheckPoint%d", n);
        xml->GetBool(name, &attachToWall, false);
    }
    std::memcpy(&desc.field_70[18 + i], &yOffset, sizeof yOffset);
    if (respawnAt)
        SetBit(desc.blobs[0], i);
    if (attachToSpline)
        SetBit(desc.blobs[1], i);
    if (teleportTo)
        SetBit(desc.blobs[2], i);
    if (attachToWall)
        SetBit(desc.blobs[3], i);
}

// <Script Name=... Type=...> with its text bindings.
void ParseScriptElement(XmlElement* child, GoalDesc& desc)
{
    levels::TextFieldList texts;
    levels::ParseTextFieldList(child, texts, 0xFFFF);
    std::vector<GoalScriptBinding> bindings(texts.size());
    static_assert(sizeof(TextField) <= sizeof(GoalScriptBinding), "a binding holds one text field");
    for (size_t i = 0; i < texts.size(); ++i)
        std::memcpy(bindings[i].data, &texts[i], sizeof(TextField));

    uint32_t name = 0;
    child->GetHash(kAttr_Name, &name, 0);
    if (!name)
        return;
    uint32_t type = 0;
    bool isSequence = false, playOnce = false;
    child->GetHash(kAttr_Type, &type, 0);
    child->GetBool(kAttr_IsCutscene, &isSequence, false);
    child->GetBool(kAttr_PlayOnce, &playOnce, false);
    const GoalScriptBinding* data = bindings.empty() ? nullptr : bindings.data();
    const uint32_t count = (uint32_t)bindings.size();
    auto add = [&](int slot) { AddGoalScript(desc.scripts, slot, name, data, count, isSequence, playOnce); };

    if (type == kType_CompleteBoth) {
        add(kScripts_Complete);
        add(kScripts_CompleteIdle);
    } else if (type == kType_ActivateBoth) {
        add(kScripts_Active);
        add(kScripts_CompleteIdle);
    }
    int slot = -1;
    switch (type) {
    case kType_OnActivate: slot = kScripts_Active; break;
    case kType_3:          slot = kScripts_CompleteIdle; break;
    case kType_4:          slot = kScripts_ActiveIdle; break;
    case kType_OnComplete: slot = kScripts_Complete; break;
    case kType_OnFail:     slot = kScripts_Failed; break;
    default: break;
    }
    if (slot >= 0)
        add(slot);
}

// A hint child (trigger, spawner, laser...); at most 32 per goal.
void ParseHintElement(XmlElement* child, int32_t kind, GoalDesc& desc)
{
    uint32_t name = 0;
    child->GetHash(kAttr_Name, &name, 0);
    if (!name || (desc.hints && desc.hints->Count() >= kMaxHints))
        return;
    GoalHint hint;
    hint.target = name;
    hint.kind = kind;
    child->GetBool(kAttr_Disable, &hint.invert, false);
    child->GetBool(kAttr_Persistent, &hint.persistent, hint.persistent);
    if (kind == kHint_SpawnGroup || kind == kHint_Spawner) {
        hint.keepAlive = true;
        child->GetBool(kAttr_HintKeepAlive, &hint.keepAlive, true);
    }
    if (!desc.hints)
        desc.hints = new ape::ApeTArray<GoalHint>();
    desc.hints->PushBack(hint);
}

} // namespace

// -- group options --------------------------------------------------------------------

// @func 0x00321D3C ParseGoalGroupSetup
bool ParseGoalGroupSetup(XmlElement* xml, GoalGroupSetup* out)
{
    uint32_t op = kGroupOps[out->mode];
    if (out->mode == 0 && xml->GetHash(kAttr_Op, &op, op) != 0) {
        plat::Log(plat::LogLevel::Warning, "%s: goal group without an operator\n", xml->GetLocationString());
        return false;
    }
    int mode = 0;
    while (mode < 7 && kGroupOps[mode] != op)
        ++mode;
    if (mode >= 7) {
        plat::Log(plat::LogLevel::Warning, "%s: unknown goal group operator\n", xml->GetLocationString());
        return false;
    }
    out->mode = mode;
    xml->GetBool(kAttr_GroupNotify, &out->notifyAdvance, false);
    xml->GetBool(kAttr_GroupWaitScripts, &out->waitActiveScripts, false);
    xml->GetInt(kAttr_MaxTargets, &out->maxMarkers, 0);
    xml->GetBool(kAttr_GroupStopOnCompletion, &out->stopOnNewCompletion, false);
    return true;
}

void GoalGroupSetup::ApplyTo(GoalGroup* group) const
{
    group->mode = mode;
    group->completedCount = 0;
    group->firstChild = nullptr;
    uint32_t flags = group->groupFlags & ~(GoalGroup::kGroup_NotifyAdvance | GoalGroup::kGroup_WaitActiveScripts |
                                           GoalGroup::kGroup_StopOnNewCompletion);
    if (notifyAdvance)
        flags |= GoalGroup::kGroup_NotifyAdvance;
    if (waitActiveScripts)
        flags |= GoalGroup::kGroup_WaitActiveScripts;
    if (stopOnNewCompletion)
        flags |= GoalGroup::kGroup_StopOnNewCompletion;
    group->groupFlags = flags;
    group->maxMarkers = (uint32_t)maxMarkers;
}

// -- scripts --------------------------------------------------------------------------

// @func 0x00322AC0 AddGoalScript
void AddGoalScript(GoalScriptCursor* scripts, int slot, uint32_t name, const GoalScriptBinding* bindings,
                   uint32_t bindingCount, bool isSequence, bool restart)
{
    if (slot < 0 || name == 0)
        return;
    GoalScriptList*& list = scripts[slot].list;
    if (!list)
        list = new GoalScriptList();
    if (list->Count() >= kMaxScriptsPerSlot) {
        plat::Log(plat::LogLevel::Warning, "Too many goal scripts in one slot\n");
        return;
    }
    auto* script = new GoalScript();
    script->bindings.Init(4, nullptr, false);
    script->name = name;
    script->flags = (isSequence ? kScript_IsSequence : 0u) | (restart ? kScript_Restart : 0u);
    for (uint32_t i = 0; i < bindingCount; ++i)
        script->bindings.PushBack(bindings[i]);
    if (!list->scripts)
        list->scripts = new ape::ApeTArray<GoalScript*>();
    list->scripts->PushBack(script);
}

// The description's constructor: the given identity and texts, every other
// field cleared, no scripts, four empty nine-bit checkpoint sets. NOTE: the
// console constructor takes every field as a parameter; ParseGoal passes
// zero for the ones not listed here.
// @func 0x00768E30 InitGoalDesc
void InitGoalDesc(GoalDesc& desc, SlyGame* game, Job* job, uint32_t repeatCount, const char* name,
                  const char* description, float hintChance)
{
    desc = GoalDesc();
    desc.game = game;
    desc.job = job;
    desc.repeatCount = repeatCount;
    desc.name = name;
    desc.description = description;
    std::memcpy(&desc.field_134, &hintChance, sizeof hintChance);
    for (GoalBlob& blob : desc.blobs) {
        blob.size = 2;
        blob.extra = kCheckpointCount;
        blob.data = new uint8_t[2]();
    }
}

// The description's destructor: the script lists, hints, checkpoint sets
// and cycling texts it owns.
// @func 0x007691D4 ReleaseGoalDesc
void ReleaseGoalDesc(GoalDesc& desc)
{
    for (GoalScriptCursor& cursor : desc.scripts) {
        if (!cursor.list)
            continue;
        if (cursor.list->scripts) {
            for (GoalScript* script : *cursor.list->scripts)
                delete script;
            delete cursor.list->scripts;
        }
        delete cursor.list;
        cursor.list = nullptr;
    }
    delete desc.hints;
    desc.hints = nullptr;
    for (GoalBlob& blob : desc.blobs) {
        delete[] blob.data;
        blob = GoalBlob();
    }
    delete[] desc.cycle;
    desc.cycle = nullptr;
    desc.cycleCount = 0;
}

// -- goals ----------------------------------------------------------------------------

// @func 0x00322D88 ParseGoal
Goal* ParseGoal(SlyGame* game, Job* job, XmlElement* xml, int32_t index)
{
    const uint32_t element = ElementName(xml);
    if (Contains(kGoalChildElements, sizeof kGoalChildElements / sizeof kGoalChildElements[0], element))
        return nullptr;

    const float noChance = -1.0f;
    GoalDesc desc;
    InitGoalDesc(desc, game, job, 1, "<unknown goal name>", "<unknown goal description>", noChance);

    // NOTE: the console overwrites the default texts even when an attribute
    // is missing (the goal then has no name / description).
    desc.name = xml->GetString("name");
    int32_t minCount = 1;
    xml->GetInt(kAttr_MinCount, &minCount, 1);
    desc.repeatCount = (uint32_t)minCount;
    desc.description = xml->GetString("text");
    desc.text2 = xml->GetString("msgFail");
    xml->GetHash(kAttr_TargetName, &desc.targetName, 0);
    xml->GetBool(kAttr_TargetFlag, &desc.targetFlag, false);
    xml->GetHash(kAttr_Field20, &desc.field_20, 0);
    xml->GetHash(kAttr_CounterType, &desc.progressName, 0);
    xml->GetInt(kAttr_CounterOffset, &desc.progressBase, 0);
    xml->GetHash(kAttr_LifetimeScript, &desc.hintScriptName, 0);
    bool value = false;
    xml->GetBool(kAttr_Field30, &value, false);
    desc.field_30 = value;
    xml->GetBool(kAttr_Field31, &value, false);
    desc.field_31 = value;
    xml->GetBool(kAttr_CompletesParent, &desc.completesParent, false);
    xml->GetBool(kAttr_ShowCounterTotal, &desc.showProgressTotal, true);
    xml->GetBool(kAttr_FailRestarts, &desc.failRestarts, false);
    for (const FlagAttribute& attr : kFlagAttributes) {
        bool on = attr.defaultValue;
        xml->GetBool(attr.attribute, &on, attr.defaultValue);
        if (on)
            desc.flags |= attr.flag;
    }

    for (int i = 0; i < kCheckpointCount; ++i)
        ParseCheckpoint(xml, desc, i);

    xml->GetHash(kAttr_AssociatedLevel, &desc.field_DC, 0);
    xml->GetHash(kAttr_AssociatedDef, &desc.field_E0, 0);
    xml->GetBool(kAttr_FieldE8, &desc.field_E8, false);
    xml->GetBool(kAttr_ShowDescription, &desc.showDescription, true);
    if (const char* abbrev = xml->GetString("hintAbbrev")) {
        if (*abbrev) {
            std::strncpy(desc.field_120, abbrev, sizeof desc.field_120 - 1);
            desc.field_120[sizeof desc.field_120 - 1] = '\0';
        }
    }
    int32_t field130 = (int32_t)desc.field_130;
    xml->GetInt(kAttr_Field130, &field130, field130);
    desc.field_130 = field130 > 0 ? (uint32_t)field130 : 0;
    float hintChance = noChance;
    xml->GetFloat(kAttr_HintChance, &hintChance, noChance);
    std::memcpy(&desc.field_134, &hintChance, sizeof hintChance);

    for (const ScriptAttribute& attr : kScriptAttributes) {
        uint32_t script = 0;
        if (xml->GetHash(attr.attribute, &script, 0) != 0 || script == 0)
            continue;
        AddGoalScript(desc.scripts, attr.slot, script, nullptr, 0, attr.isSequence, attr.restart);
        if (attr.attribute == kAttr_ScriptComplete)
            AddGoalScript(desc.scripts, kScripts_Complete, script, nullptr, 0, false, false);
    }

    int linkedGoals = 0;
    for (XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t childName = ElementName(child);
        bool isHint = false;
        for (const HintElement& hint : kHintElements) {
            if (hint.element == childName) {
                ParseHintElement(child, hint.kind, desc);
                isHint = true;
                break;
            }
        }
        if (isHint)
            continue;
        if (childName == kElem_Script) {
            ParseScriptElement(child, desc);
        } else if (childName == kElem_LinkedGoal) {
            if (linkedGoals >= kMaxLinkedGoals ||
                child->GetHash(kAttr_Goal, &desc.field_60[linkedGoals], 0) != 0) {
                plat::Log(plat::LogLevel::Warning, "%s: bad linked goal\n", child->GetLocationString());
                continue;
            }
            ++linkedGoals;
        }
    }

    // Cycling texts.
    int cycleCount = 0;
    for (XmlElement* text = xml->FirstChildElement("BinocucomText"); text;
         text = text->NextSiblingElement("BinocucomText"))
        ++cycleCount;
    GoalCycleEntry* cycle = cycleCount ? new GoalCycleEntry[(size_t)cycleCount] : nullptr;
    int i = 0;
    for (XmlElement* text = xml->FirstChildElement("BinocucomText"); text && i < cycleCount;
         text = text->NextSiblingElement("BinocucomText"), ++i) {
        float duration = 0.0f;
        text->GetFloat(kAttr_DisplayTime, &duration, 0.0f);
        cycle[i].duration = duration;
        cycle[i].SetText(text->GetString("text"));
    }
    desc.cycle = cycle;
    desc.cycleCount = cycleCount;
    desc.index = index;

    Goal* goal = nullptr;
    bool known = false;
    for (const GoalElement& entry : kGoalElements) {
        if (entry.element == element) {
            goal = entry.create(xml, &desc);
            known = true;
            break;
        }
    }
    if (!known && !Contains(kNoGoalElements, 2, element))
        plat::Log(plat::LogLevel::Warning, "%s: unknown goal element %s\n", xml->GetLocationString(),
                  ape::TempHexString(element));
    ReleaseGoalDesc(desc);
    return goal;
}

// @func 0x00321EC8 GoalSet::Create
Goal* CreateGoalSet(XmlElement* xml, const GoalDesc* desc)
{
    GoalGroupSetup setup;
    if (!ParseGoalGroupSetup(xml, &setup))
        return nullptr;
    auto* group = new GoalSet(desc);
    setup.ApplyTo(group);
    for (XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (Goal* goal = ParseGoal(desc->game, desc->job, child, desc->index))
            group->AddChild(goal);
    }
    return group;
}

// -- factories not ported yet ----------------------------------------------------------

// TODO(0x0033A520 GoalQteGame::Create)
Goal* CreateQteGoal(XmlElement*, const GoalDesc*) { return nullptr; }

// -- Operation Mousetrap ------------------------------------------------------------------

// @func 0x00333918 GoalOperationMousetrap::GoalOperationMousetrap
GoalOperationMousetrap::GoalOperationMousetrap(const GoalDesc* desc, const GoalGroupSetup& setup,
                                               const ape::ApeTArray<uint32_t>& setA,
                                               const ape::ApeTArray<uint32_t>& setB)
    : GoalGroup(desc)
{
    setup.ApplyTo(this);
    namesA.Init(3, nullptr, true);
    namesB.Init(3, nullptr, true);
    entitiesA.Init(3, nullptr, true);
    entitiesB.Init(3, nullptr, true);
    helper = nullptr;
    phase = kPhase_None;
    started = false;
    wantSetA = true;
    for (uint32_t name : setA)
        namesA.PushBack(name);
    for (uint32_t name : setB)
        namesB.PushBack(name);
}

// @func 0x003344B8 GoalOperationMousetrap::Create
Goal* GoalOperationMousetrap::Create(XmlElement* xml, const GoalDesc* desc)
{
    // NOTE: the console builds the group even when the options are bad.
    GoalGroupSetup setup;
    ParseGoalGroupSetup(xml, &setup);
    constexpr uint32_t kSetANames[3] = {0x7DCA6B6Au, 0x7DCA6B6Bu, 0x7DCA6B6Cu};   // NOTE: base name unknown
    constexpr uint32_t kSetBNames[3] = {0x0408E082u, 0x0408E083u, 0x0408E084u};   // "Gear1".."Gear3"
    ape::ApeTArray<uint32_t> setA(3, nullptr, true);
    ape::ApeTArray<uint32_t> setB(3, nullptr, true);
    for (uint32_t attribute : kSetANames) {
        uint32_t name = 0;
        xml->GetHash(attribute, &name, 0);
        setA.PushBack(name);
    }
    for (uint32_t attribute : kSetBNames) {
        uint32_t name = 0;
        xml->GetHash(attribute, &name, 0);
        setB.PushBack(name);
    }
    auto* goal = new GoalOperationMousetrap(desc, setup, setA, setB);
    for (XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (Goal* childGoal = ParseGoal(desc->game, desc->job, child, desc->index))
            goal->AddChild(childGoal);
    }
    return goal;
}

} // namespace goals
} // namespace sly
