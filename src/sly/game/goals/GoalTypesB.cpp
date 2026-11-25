// openTIT - Camera entity and interaction goals.
#include "sly/game/goals/GoalTypes.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"
#include "agl/AglOcclusion.h"
#include "agl/AglRenderThread.h"
#include "ape/level/ApeGui.h"
#include "ape/xml/ApeStringTable.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/tuning/BinocucomGoalTuning.h"

#include <cmath>

namespace sly {
namespace goals {

namespace {

constexpr uint32_t kAttr_Name   = 0x0024728B;   // "Name"
constexpr uint32_t kAttr_Target = 0x7A30D7E3;

// Hint script variables/state of the camera goal.
constexpr uint32_t kVar_Binocucom     = 0xEB336A2D;
constexpr uint32_t kVar_TargetName    = 0x93116591;
constexpr uint32_t kState_NewTarget   = 0xDC175839;
// Vertical field of view assumed for the size test (degrees).
constexpr float kCameraFov = 55.0f;

const float kZero[3] = {0.0f, 0.0f, 0.0f};

void SetVec3(ape::Vec3& v, const float in[3])
{
    v.x = in[0];
    v.y = in[1];
    v.z = in[2];
}

} // namespace

// ===========================================================================
// GoalCameraEntity

// @func 0x0032DA14 GoalCameraEntity::~GoalCameraEntity
// @func 0x0032DCB4 GoalCameraEntity::~GoalCameraEntity (deleting)
GoalCameraEntity::~GoalCameraEntity() = default;

// @func 0x0032DD10 GoalCameraEntity::Create
Goal* GoalCameraEntity::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float cooldown = 15.0f;
    float timer = 0.0f;
    xml->GetFloat(0xDBDD1EA5u, &cooldown, 15.0f);
    xml->GetFloat(0x71F634BFu, &timer, 0.0f);

    GoalCameraEntity* goal = new GoalCameraEntity(desc);
    goal->countsForGroup = false;
    goal->timer = timer;
    goal->cooldown = cooldown;
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kAttr_Target); child;
         child = child->NextSiblingElement(kAttr_Target)) {
        CameraEntityTarget target;
        if (child->GetHash(kAttr_Name, &target.name, 0) != 0)
            continue;   // the console warns about the missing name
        float offset[3];
        child->GetVec3(0x8ACA5893u, offset, kZero);
        SetVec3(target.offset, offset);
        child->GetFloat(0xBBBB80B0u, &target.screenRadius, target.screenRadius);
        child->GetFloat(0x8E727ACCu, &target.minSize, target.minSize);
        // NOTE: the console reads this attribute into the screen radius slot
        // again (overwriting 0xBBBB80B0); the maximum size stays 50.
        child->GetFloat(0x8E7279DEu, &target.screenRadius, target.screenRadius);
        child->GetBool(0x1EE7AC93u, &target.binocucomOnly, false);
        if (goal->targetCount >= kMaxTargets)
            break;
        target.entity = nullptr;
        target.maxSize = 50.0f;
        goal->targets[goal->targetCount++] = target;
    }
    return goal;
}

// @func 0x0032E038 GoalCameraEntity::SelectTarget
CameraEntityTarget* GoalCameraEntity::SelectTarget()
{
    binocucomMode = false;
    float camPos[3];
    float camFwd[3];
    bool haveCamera = false;
    if (PlayerCharacter(0)) {
        haveCamera = GameCameraFrame(camPos, camFwd);
        binocucomMode = PlayerBinocucomActive();
    }
    if (timer > 0.0f || !haveCamera)
        return nullptr;

    CameraEntityTarget* best = nullptr;
    float bestDot = 0.0f;
    const float viewScale = 2.0f * std::tan(0.5f * kCameraFov * 0.017453292f);
    for (int i = 0; i < targetCount; ++i) {
        CameraEntityTarget& target = targets[i];
        if (!target.entity || target.binocucomOnly != binocucomMode)
            continue;
        float pos[3];
        EntityPosition(target.entity, pos);
        const float d[3] = {pos[0] - camPos[0], pos[1] - camPos[1], pos[2] - camPos[2]};
        const float depth = camFwd[0] * d[0] + camFwd[1] * d[1] + camFwd[2] * d[2];
        if (depth <= 0.0f)
            continue;
        // Width of the view at the entity's depth must be in range.
        const float width = depth * viewScale;
        if (width < target.minSize || width > target.maxSize)
            continue;
        float screen[2] = {0.0f, 0.0f};
        float sx = 0.0f;
        float sy = 0.0f;
        if (GameWorldToScreen(pos, screen)) {
            sx = (screen[0] - 0.5f) * 0.5f;
            sy = (screen[1] - 0.5f) * 0.5f;
        }
        if (std::sqrt(sx * sx + sy * sy) > target.screenRadius)
            continue;
        const float inv = 1.0f / std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float cosine = (camFwd[0] * d[0] + camFwd[1] * d[1] + camFwd[2] * d[2]) * inv;
        if (cosine > bestDot) {
            bestDot = cosine;
            best = &target;
        }
    }
    return best;
}

// @func 0x0032E560 GoalCameraEntity::Update
uint8_t GoalCameraEntity::Update(int player, const GameEvent& e)
{
    if (status != kGoal_Active)
        return Goal::Update(player, e);

    if (e.type == kEvent_06) {
        CameraEntityTarget* target = SelectTarget();
        if (timer > 0.0f) {
            timer -= e.arg0Float;
            if (timer < 0.0f)
                timer = 0.0f;
        }
        if (target != current) {
            if (target && hintScript) {
                if (ape::ApeAnimScriptVar* var = hintScript->FindVar(kVar_Binocucom))
                    if (var->GetType() == ape::AnimScriptVarType::Int)
                        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(binocucomMode ? 1 : 0);
                const uint32_t name = GoalEntityName(target->entity);
                if (ape::ApeAnimScriptVar* var = hintScript->FindVar(kVar_TargetName)) {
                    if (var->GetType() == ape::AnimScriptVarType::Hash)
                        static_cast<ape::ApeAnimScriptHashVar*>(var)->SetHash(name);
                    else if (var->GetType() == ape::AnimScriptVarType::Int)
                        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(int32_t(name));
                }
                hintScript->SetStateAndUpdate(kState_NewTarget);
            }
            if (target)
                timer = cooldown;
            current = target;
        }
    }
    // Never holds up its job.
    return kGoal_Complete;
}

// @func 0x0032E760 GoalCameraEntity::ResetHints
void GoalCameraEntity::ResetHints(bool stopHintScript)
{
    for (int i = 0; i < targetCount; ++i) {
        if (targets[i].entity) {
            GoalEntityRelease(targets[i].entity);
            targets[i].entity = nullptr;
        }
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x0032E838 GoalCameraEntity::ReapplyHints
void GoalCameraEntity::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    for (int i = 0; i < targetCount; ++i) {
        if (void* entity = GoalFindEntity(targets[i].name)) {
            targets[i].entity = entity;
            GoalEntityAddRef(entity);
        }
    }
}

// @func 0x0032E918 GoalCameraEntity::SetStatus
void GoalCameraEntity::SetStatus(uint8_t newStatus, bool force)
{
    const bool changed = force || newStatus != status;
    if (changed && newStatus != kGoal_Active)
        ResetHints(false);
    Goal::SetStatus(newStatus, force);
    if (changed && newStatus == kGoal_Active) {
        ResetHints(false);
        ReapplyHints(nullptr);
    }
}

// ===========================================================================
// GoalInteraction

namespace {
// Check names (table 0x00ADB918), in Check order.
constexpr uint32_t kCheckNames[5] = {0x7A599AA9, 0x9F632718, 0x72C27306, 0x3ECC2A7C, 0xA5E5BC74};
constexpr uint32_t kAttr_Type = 0x000009E1;
} // namespace

// @func 0x0032FA74 GoalInteraction::~GoalInteraction
// @func 0x0032FD40 GoalInteraction::~GoalInteraction (deleting)
GoalInteraction::~GoalInteraction() = default;

// @func 0x0032FD94 GoalInteraction::Create
Goal* GoalInteraction::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t typeName = 0;
    uint32_t check = 0;
    if (xml->GetHash(kAttr_Type, &typeName, 0) == 0) {
        while (check < 5 && kCheckNames[check] != typeName)
            ++check;
        if (check > 4)
            check = 0;   // the console warns about the unknown type
    }
    GoalInteraction* goal = new GoalInteraction(desc);
    goal->check = check;

    static const uint32_t kOptionAttrs[6] = {
        0x5D28E3FC, 0x9446A097, 0xD9159FE7, 0x09DA53D5, 0x04656B91, 0x310F51B2,
    };
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kAttr_Target); child;
         child = child->NextSiblingElement(kAttr_Target)) {
        InteractionLink link;
        if (child->GetHash(kAttr_Name, &link.name, 0) != 0)
            continue;   // the console warns about the missing name
        float offset[3];
        child->GetVec3(0x4A4D4D4Du, offset, kZero);
        SetVec3(link.offset, offset);
        child->GetBool(0x76E96977u, &link.showMarker, false);
        bool unused = false;
        child->GetBool(0x3B66FD69u, &unused, false);
        for (int i = 0; i < 6; ++i) {
            bool value = false;
            child->GetBool(kOptionAttrs[i], &value, false);
            if (value)
                link.options |= 1u << (30 - i);
        }
        goal->links.PushBack(link);
    }
    return goal;
}

// @func 0x00330174 GoalInteraction::Update
uint8_t GoalInteraction::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active) {
        uint32_t satisfied = 0;
        for (InteractionLink& link : links) {
            void* interaction = link.interaction;
            if (!interaction)
                continue;
            const int state = HintInteractionState(interaction);
            bool ok = false;
            switch (check) {
            case kCheck_Available: ok = state < 2; break;
            case kCheck_State1:    ok = state == 1; break;
            case kCheck_Flag22:    ok = InteractionFlag22(interaction); break;
            case kCheck_State3:    ok = state == 3; break;
            case kCheck_Touch:
                InteractionTouch(interaction);
                ok = true;
                break;
            default: break;
            }
            if (ok)
                ++satisfied;
        }
        if (satisfied != 0 && satisfied == links.Count())
            SetStatus(kGoal_Complete, false);
    }
    return Goal::Update(player, e);
}

// @func 0x00330334 GoalInteraction::ResetHints
void GoalInteraction::ResetHints(bool stopHintScript)
{
    for (InteractionLink& link : links)
        link.interaction = nullptr;
    Goal::ResetHints(stopHintScript);
}

// @func 0x00330394 GoalInteraction::ReapplyHints
void GoalInteraction::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    for (InteractionLink& link : links) {
        void* entity = GoalFindEntity(link.name);
        link.interaction = entity ? HintEntityInteraction(entity) : nullptr;
        // The "available" check starts from a fresh interaction.
        if (link.interaction && check == kCheck_Available)
            HintInteractionSetState(link.interaction, 0);
    }
}

// @func 0x003304A0 GoalInteraction::CollectMarkers
void GoalInteraction::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (InteractionLink& link : links) {
        if (!link.showMarker || !link.interaction)
            continue;
        if (void* owner = InteractionOwner(link.interaction))
            GoalAddEntityMarker(list, owner);
    }
}

// @func 0x00330670 GoalInteraction::SetStatus
void GoalInteraction::SetStatus(uint8_t newStatus, bool force)
{
    if ((force || newStatus != status) && newStatus != kGoal_Active)
        ResetHints(false);
    Goal::SetStatus(newStatus, force);
}

} // namespace goals
} // namespace sly

#include "ape/core/ApeString.h"

#include <cstdio>

namespace sly {
namespace goals {

// ===========================================================================
// GoalScore

namespace {
constexpr uint32_t kScore_StateScored  = 0x04B14672;
constexpr uint32_t kScore_StatCategory = 0xA8B960FF;
constexpr uint32_t kScore_StatName     = 0x36FB367F;
constexpr uint32_t kScore_TargetEvent  = 0x879F2BDA;   // kGameEventNames[1]

// Adds `delta` to a completion count, clamped to the score range when set.
uint8_t ClampProgress(uint32_t value, const GoalScore& goal)
{
    uint8_t progress = uint8_t(value);
    if (goal.scoreFlags & GoalScore::kScore_HasMin) {
        const uint8_t low = uint8_t(int(goal.minScore));
        if (progress < low)
            progress = low;
    }
    if (goal.scoreFlags & GoalScore::kScore_HasMax) {
        const uint8_t high = uint8_t(int(goal.maxScore));
        if (high < progress)
            progress = high;
    }
    return progress;
}
} // namespace

namespace {
// Comparisons ("op", table 0x00ADBB18), in GoalScore::Compare order.
constexpr uint32_t kCompareNames[5] = {
    0x00239999,   // "Less"
    0xF6F514EE,   // NOTE: name unknown (at most)
    0x03F26F14,   // "Equal" (within "range")
    0x3BB2F7DA,   // "greater"
    0x037AFE6F,   // NOTE: name unknown (at least)
};
constexpr uint32_t kAttr_ScoreOp = 0x000009E1;         // "op"
constexpr uint32_t kAttr_ScoreTarget = 0x93116591;     // "Target"
constexpr uint32_t kAttr_Range = 0x04A2411D;           // "range"
constexpr uint32_t kAttr_Min = 0x00012A32;             // "Min"
constexpr uint32_t kAttr_Max = 0x00012944;             // "Max"
constexpr uint32_t kAttr_NotifyAdvance = 0x512CF5D9;   // NOTE: name unknown
constexpr uint32_t kAttr_ReportStat = 0x58FA06CA;      // NOTE: name unknown
constexpr uint32_t kElem_Value = 0x0027873A;           // "Type": points per event group
constexpr uint32_t kAttr_Count = 0x03D558EF;           // "Count"
constexpr uint32_t kAttr_Parent = 0x8C3CFCAA;          // NOTE: name unknown
constexpr uint32_t kAttr_Data = 0x001FE7AA;            // "Data"
constexpr uint32_t kAttr_MarkerDef = 0x6E6EDD6B;       // NOTE: "markerDef"
constexpr uint32_t kAttr_Offset = 0x4A4D4D4D;
// Source flag attributes, bit 31 downwards (bits 29 and 24 come from
// 0x3B66FD69 when it is set).
constexpr uint32_t kAttr_ShowMarker = 0x76E96977;      // "ShowMarker", bit 31
constexpr uint32_t kAttr_Unmark = 0x0137F514;          // bit 30 (default on)
constexpr uint32_t kAttr_HideMarker = 0x5D28E3FC;      // "HideMarker", bit 28
constexpr uint32_t kAttr_Bit27 = 0x9446A097;
constexpr uint32_t kAttr_Bit26 = 0xD9149FE7;
constexpr uint32_t kAttr_Bit25 = 0x09DA53D5;
constexpr uint32_t kAttr_Both = 0x3B66FD69;            // sets bits 29 and 24
constexpr uint32_t kAttr_Bit29 = 0x04656B91;
constexpr uint32_t kAttr_Bit24 = 0x310F51B2;
constexpr uint32_t kAttr_CountMissing = 0x5D1967C0;    // bit 23

int32_t ReadPoints(ape::xml::XmlElement* xml)
{
    int32_t points = 1;
    xml->GetInt(kAttr_Count, &points, 1);
    return points < -32 ? -32 : (points > 32 ? 32 : points);
}

float ClampScoreLimit(float value)
{
    return value < -128.0f ? -128.0f : (value > 127.0f ? 127.0f : value);
}
} // namespace

// Needs a "scoreType", a known "op" and a "Target"; <Type> children give
// points per event group, <Target> children the scoring objects.
// @func 0x0033E7D8 GoalScore::Create
Goal* GoalScore::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    const char* scoreType = xml->GetString("scoreType");
    const char* customData = xml->GetString("customData");
    if (!scoreType || !*scoreType) {
        plat::Log(plat::LogLevel::Warning, "%s: GoalScore without a scoreType\n", xml->GetLocationString());
        return nullptr;
    }
    uint32_t op = 0;
    if (xml->GetHash(kAttr_ScoreOp, &op, 0) != 0) {
        plat::Log(plat::LogLevel::Warning, "%s: GoalScore without an op\n", xml->GetLocationString());
        return nullptr;
    }
    uint8_t compare = 0;
    while (compare < 5 && kCompareNames[compare] != op)
        ++compare;
    if (compare >= 5) {
        plat::Log(plat::LogLevel::Warning, "%s: GoalScore with an unknown op\n", xml->GetLocationString());
        return nullptr;
    }
    float target = 0.0f, range = 0.0f, minScore = 0.0f, maxScore = 0.0f;
    if (xml->GetFloat(kAttr_ScoreTarget, &target, 0.0f) != 0) {
        plat::Log(plat::LogLevel::Warning, "%s: GoalScore without a Target\n", xml->GetLocationString());
        return nullptr;
    }
    xml->GetFloat(kAttr_Range, &range, 0.0f);
    const bool hasMin = xml->GetFloat(kAttr_Min, &minScore, 0.0f) == 0;
    const bool hasMax = xml->GetFloat(kAttr_Max, &maxScore, 0.0f) == 0;
    bool notifyAdvance = false;
    xml->GetBool(kAttr_NotifyAdvance, &notifyAdvance, false);

    auto* goal = new GoalScore(desc);
    goal->marked.Init(6, nullptr, true);
    goal->sources.Init(0xC, nullptr, true);
    goal->values.Init(0x10, nullptr, true);
    goal->eventName = ape::HashNameNoCase(scoreType);
    goal->customData = ape::HashNameNoCase(customData);
    goal->compare = compare;
    goal->target = target;
    goal->tolerance = range;
    goal->minScore = ClampScoreLimit(minScore);
    goal->maxScore = ClampScoreLimit(maxScore);
    goal->score = 0.0f;
    goal->scoreFlags = (notifyAdvance ? kScore_NotifyAdvance : 0u) | (hasMin ? kScore_HasMin : 0u) |
                       (hasMax ? kScore_HasMax : 0u);
    bool reportStat = false;
    xml->GetBool(kAttr_ReportStat, &reportStat, false);
    if (reportStat)
        goal->scoreFlags |= kScore_ReportStat;

    for (ape::xml::XmlElement* child = xml->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        const uint32_t element = child->GetNameHash();
        if (element == kElem_Value) {
            Value value;
            if (child->GetHash(kAttr_Name, &value.group, 0) != 0 || value.group == 0)
                continue;
            value.points = ReadPoints(child);
            goal->values.PushBack(value);
        } else if (element == kAttr_Target) {
            Source source;
            if (child->GetHash(kAttr_Name, &source.name, 0) != 0 || source.name == 0)
                continue;
            child->GetHash(kAttr_Parent, &source.parent, 0);
            child->GetHash(kAttr_Data, &source.group, 0);
            child->GetHash(kAttr_MarkerDef, &source.markerDef, 0);
            source.points = ReadPoints(child);
            float offset[3] = {};
            child->GetVec3(kAttr_Offset, offset, kZero);
            SetVec3(source.offset, offset);
            bool showMarker = false, unmark = true, hideMarker = false, bit27 = false, bit26 = false,
                 bit25 = false, both = false, bit29 = false, bit24 = false, countMissing = false;
            child->GetBool(kAttr_ShowMarker, &showMarker, false);
            child->GetBool(kAttr_Unmark, &unmark, true);
            child->GetBool(kAttr_HideMarker, &hideMarker, false);
            child->GetBool(kAttr_Bit27, &bit27, false);
            child->GetBool(kAttr_Bit26, &bit26, false);
            child->GetBool(kAttr_Bit25, &bit25, false);
            child->GetBool(kAttr_Both, &both, false);
            if (both) {
                bit29 = bit24 = true;
            } else {
                child->GetBool(kAttr_Bit29, &bit29, false);
                child->GetBool(kAttr_Bit24, &bit24, false);
            }
            child->GetBool(kAttr_CountMissing, &countMissing, false);
            // NOTE: the console leaves the low 23 bits uninitialised.
            source.flags = (showMarker ? 1u << 31 : 0u) | (unmark ? 1u << 30 : 0u) | (bit29 ? 1u << 29 : 0u) |
                           (hideMarker ? 1u << 28 : 0u) | (bit27 ? 1u << 27 : 0u) | (bit26 ? 1u << 26 : 0u) |
                           (bit25 ? 1u << 25 : 0u) | (bit24 ? 1u << 24 : 0u) | (countMissing ? 1u << 23 : 0u);
            goal->sources.PushBack(source);
        }
    }
    return goal;
}

// @func 0x0033F0C8 GoalScore::Update
uint8_t GoalScore::Update(int player, const GameEvent& e)
{
    if (status != kGoal_Active)
        return Goal::Update(player, e);

    const uint32_t type = e.type;
    bool accepted = false;
    bool valueFound = false;
    bool sourceFound = false;
    int32_t valuePoints = 1;
    int32_t sourcePoints = 1;
    uint32_t sourceGroup = 0;
    uint32_t sourceFlags = 0;
    float delta = 0.0f;

    if (type < 0x21 && kGameEventNames[type] == eventName) {
        // Events 7-10 always score; the others only for listed objects.
        const bool always = type >= 7 && type <= 10;
        bool lookup = always || values.Count() != 0 || sources.Count() != 0;
        if (lookup) {
            if (values.Count() != 0 && e.groupId != 0) {
                for (const Value& value : values) {
                    if (value.group == e.groupId) {
                        valuePoints = value.points;
                        valueFound = true;
                        break;
                    }
                }
            }
            if (!valueFound && e.objectId != 0) {
                for (const Source& source : sources) {
                    if (source.name == e.objectId &&
                        (source.parent == e.ownerId || source.parent == 0)) {
                        sourceGroup = source.group;
                        sourcePoints = source.points;
                        sourceFlags = source.flags & 0xC0000000u;
                        sourceFound = true;
                        break;
                    }
                }
            }
        }
        if (!lookup || valueFound || sourceFound) {
            accepted = true;
            if (type == 0x17 && e.arg0 != sourceGroup)
                accepted = false;
            if (type == 0x1A && !e.arg0)
                accepted = false;
            // NOTE: event 0x1A also compares field_CC of its argument with
            // the source's group; the argument type is not modelled.
            if (type == 0x1C && e.arg0 != customData)
                accepted = false;
        } else {
            accepted = always;
        }

        if (accepted) {
            delta = valueFound ? float(valuePoints) : (sourceFound ? float(sourcePoints) : 1.0f);
            if (scoreFlags & kScore_ReportStat)
                GoalReportStat(e.source, kScore_StatCategory, kScore_StatName, int(delta));
            float next = score + delta;
            if ((scoreFlags & kScore_HasMin) && next < minScore)
                next = minScore;
            if ((scoreFlags & kScore_HasMax) && next > maxScore)
                next = maxScore;
            score = next;
            if (hintScript) {
                hintScript->SetStateAndUpdate(kScore_StateScored);
                char state[32];
                std::snprintf(state, sizeof(state), "score%i", unsigned(score));
                hintScript->SetStateAndUpdate(ape::HashNameNoCase(state));
            }
            if (sourceFound && (sourceFlags & kSource_Marked)) {
                if (sourceFlags & kSource_Unmark) {
                    for (uint32_t i = 0; i < marked.Count(); ++i) {
                        void* entity = marked[i].entity;
                        if (entity && GoalEntityName(entity) == e.objectId) {
                            GoalEntityRelease(entity);
                            for (uint32_t j = i; j + 1 < marked.Count(); ++j)
                                marked[j] = marked[j + 1];
                            marked.PopBack();
                            break;
                        }
                    }
                }
                if (eventName == kScore_TargetEvent)
                    JobOwnerUntarget(job, e.objectId);
            }
            if (scoreFlags & kScore_NotifyAdvance)
                JobManagerGroupAdvanced();
        }
    }

    // Without a score change only a single-shot goal re-tests its target.
    if (!accepted && repeatCount != 0)
        return Goal::Update(player, e);

    bool reached = false;
    bool failed = false;
    switch (compare) {
    case kCompare_AtLeast: reached = target <= score; break;
    case kCompare_Greater: reached = target < score; break;
    case kCompare_Near:    reached = std::fabs(score - target) <= tolerance; break;
    case kCompare_AtMost:
        reached = score <= target;
        failed = !reached && accepted;
        break;
    case kCompare_Less:
        reached = score < target;
        failed = !reached && accepted;
        break;
    default:
        return Goal::Update(player, e);
    }
    if (reached) {
        if (repeatCount == 0)
            return kGoal_Complete;
        repeatProgress = ClampProgress(repeatProgress + uint32_t(int(delta)), *this);
        if (repeatCount <= repeatProgress)
            SetStatus(kGoal_Complete, false);
    } else if (failed) {
        SetStatus(kGoal_Failed, false);
    }
    return Goal::Update(player, e);
}

// @func 0x0033F834 GoalScore::Reset
void GoalScore::Reset(bool fromSave, int requested)
{
    if (fromSave && failRestarts) {
        score = 0.0f;
        ReapplyHints(parentGoal);
    }
    Goal::Reset(fromSave, requested);
}

// @func 0x0033FB84 GoalScore::ResetHints
void GoalScore::ResetHints(bool stopHintScript)
{
    score = 0.0f;
    if (marked.Count() != 0) {
        for (Marked& entry : marked)
            if (entry.entity)
                GoalEntityRelease(entry.entity);
        marked.Clear();
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x0033FC68 GoalScore::ReapplyHints
void GoalScore::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    for (const Source& source : sources) {
        if (source.name == 0 || !(source.flags & (kSource_Marked | kSource_CountMissing)))
            continue;
        void* entity = nullptr;
        if (source.parent == 0) {
            entity = GoalFindEntity(source.name);
        } else if (void* parent = GoalFindEntity(source.parent)) {
            entity = EntityFindDescendant(parent, source.name);
        }
        if (entity) {
            if (source.flags & kSource_Marked) {
                GoalEntityAddRef(entity);
                Marked entry;
                entry.entity = entity;
                marked.PushBack(entry);
            }
            continue;
        }
        // Already gone (collected before a reload): count it.
        if ((source.flags & kSource_CountMissing) && repeatCount != 0) {
            repeatProgress = ClampProgress(repeatProgress + uint32_t(source.points), *this);
            if (repeatCount <= repeatProgress)
                SetStatus(kGoal_Complete, false);
        }
    }
}

// @func 0x0033F8D0 GoalScore::GetProgress
bool GoalScore::GetProgress(GoalProgress* out)
{
    if (progressName == 0)
        return false;
    int done = int(score) + progressBase;
    if (status == kGoal_Complete)
        done = int(target);
    out->name = progressName;
    out->done = done;
    out->total = showProgressTotal ? int(target + float(progressBase)) : 0;
    return true;
}

// @func 0x0033F968 GoalScore::CollectMarkers
void GoalScore::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (Marked& entry : marked)
        if (entry.entity && !GoalEntityMarkerSuppressed(entry.entity))
            GoalAddEntityMarker(list, entry.entity);
}

// @func 0x0033FF5C GoalScore::SetStatus
void GoalScore::SetStatus(uint8_t newStatus, bool force)
{
    if (force || newStatus != status) {
        if (status == kGoal_Active)
            GoalStopListening(this);
        if (newStatus == kGoal_Active) {
            if (eventName != 0)
                GoalListenTo(this, eventName);
        } else {
            ResetHints(false);
        }
    }
    Goal::SetStatus(newStatus, force);
}

} // namespace goals
} // namespace sly

namespace sly {
namespace goals {

// ===========================================================================
// GoalBinocucomPhoto

// @func 0x0032B02C GoalBinocucomPhoto::~GoalBinocucomPhoto
// @func 0x0032B300 GoalBinocucomPhoto::~GoalBinocucomPhoto (deleting)
GoalBinocucomPhoto::~GoalBinocucomPhoto()
{
    SetStatus(kGoal_None, true);
}

namespace {
// Active photo goals (linked through nextActive), the goal and target whose
// occlusion is being measured, and the query doing it.
GoalBinocucomPhoto* s_activePhotoGoals = nullptr;   // 0x00F64C3C
GoalBinocucomPhoto* s_currentPhotoGoal = nullptr;   // 0x00F64C40
agl::AglOcclusionQuery* s_photoQuery = nullptr;     // 0x00F64C44
int s_currentPhotoTarget = 0;                       // 0x0106B544

constexpr uint32_t kPhotoAttr_Name = 0x0024728Bu;            // "Name"
constexpr uint32_t kPhotoAttr_Entity = 0x7A30D7E3u;          // "Entity" (child element)
constexpr uint32_t kPhotoAttr_Offset = 0x8ACA5893u;          // "Offset"
constexpr uint32_t kPhotoAttr_MarkerOffset = 0x4A4D4D4Du;
constexpr uint32_t kPhotoAttr_DisplayName = 0xE68DF62Du;     // "DisplayName"
constexpr float kPhotoDegToRad = 0.017453292f;

void DropPhotoQuery()
{
    if (s_photoQuery) {
        agl::QueueEndOfFrameRelease(s_photoQuery);
        s_photoQuery = nullptr;
    }
}

void ReadPhotoVec3(ape::xml::XmlElement* xml, uint32_t attribute, ape::Vec3& value, const ape::Vec3& fallback)
{
    float v[3];
    const float def[3] = {fallback.x, fallback.y, fallback.z};
    xml->GetVec3(attribute, v, def);
    value.x = v[0];
    value.y = v[1];
    value.z = v[2];
}

// Reads one option bit, defaulting to its current state.
void ReadPhotoOption(ape::xml::XmlElement* xml, uint32_t attribute, uint32_t& options, uint32_t bit)
{
    bool on = (options & bit) != 0;
    xml->GetBool(attribute, &on, on);
    options = on ? (options | bit) : (options & ~bit);
}
} // namespace

// @func 0x0032B358 GoalBinocucomPhoto::Create
Goal* GoalBinocucomPhoto::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t filter = 0;
    bool gadgetFlag = false, reportGroup = false, characterFlag = false;
    xml->GetHash(0x1F6AE7F0u, &filter, 0);
    xml->GetBool(0x254881FCu, &gadgetFlag, false);
    xml->GetBool(0x81213F1Du, &reportGroup, false);
    xml->GetBool(0x5845B273u, &characterFlag, false);

    GoalBinocucomPhoto* goal = new GoalBinocucomPhoto(desc);
    goal->targets.Init(8, nullptr, true);
    goal->nextActive = nullptr;
    goal->photoFilter = filter;
    goal->photoFlags = (gadgetFlag ? uint32_t(kPhoto_GadgetFlag) : 0u) | (reportGroup ? uint32_t(kPhoto_ReportGroup) : 0u) |
                       (characterFlag ? uint32_t(kPhoto_CharacterFlag) : 0u);

    const ape::Vec3 zero = {0.0f, 0.0f, 0.0f, 0.0f};   // console global 0x01164B40
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kPhotoAttr_Entity); child;
         child = child->NextSiblingElement(kPhotoAttr_Entity)) {
        Target t;
        t.markerOffset = zero;
        t.offset = zero;
        t.angles = zero;
        t.options = kTarget_OptionDE86C5DE | kTarget_Option9896A7D0 | kTarget_OptionFAA75B88;
        if (child->GetHash(kPhotoAttr_Name, &t.name, 0) != ape::xml::kQuery_Ok)
            continue;   // the console warns with the document path
        child->GetHash(0x39F79104u, &t.bone, t.bone);
        ReadPhotoVec3(child, 0xB095DE33u, t.angles, t.angles);
        child->GetFloat(0x03B880B3u, &t.angle, g_binocucomPhotoDefaultAngle.Get());
        child->GetFloat(0x8E727ACCu, &t.distMin, g_binocucomPhotoDefaultDistMin.Get());
        child->GetFloat(0x8E7279DEu, &t.distMax, g_binocucomPhotoDefaultDistMax.Get());
        child->GetFloat(0x37FA4A8Cu, &t.distMinClose, g_binocucomPhotoDefaultDistMinClose.Get());
        child->GetFloat(0xA1D8E6BAu, &t.distMaxClose, g_binocucomPhotoDefaultDistMaxClose.Get());
        child->GetFloat(0x3FD88018u, &t.distCull, g_binocucomPhotoDefaultDistCull.Get());
        ReadPhotoVec3(child, kPhotoAttr_Offset, t.offset, zero);
        child->GetFloat(0x001D691Du, &t.displayTime, g_binocucomPhotoDefaultDisplayTime.Get());
        child->GetFloat(0x3450503Fu, &t.occlusionRadius, g_binocucomPhotoDefaultOcclusionRadius.Get());
        child->GetFloat(0x6E3DE0E2u, &t.visibleFraction, 1.0f);
        if (t.visibleFraction > 1.0f)
            t.visibleFraction = 1.0f;
        if (t.visibleFraction < 0.0f)
            t.visibleFraction = 0.0f;
        ReadPhotoVec3(child, kPhotoAttr_MarkerOffset, t.markerOffset, t.markerOffset);

        ReadPhotoOption(child, 0x3B66FD69u, t.options, kTarget_Option3B66FD69);
        ReadPhotoOption(child, 0x5D28E3FCu, t.options, kTarget_Option5D28E3FC);
        ReadPhotoOption(child, 0x9446A097u, t.options, kTarget_Option9446A097);
        ReadPhotoOption(child, 0xD9159FE7u, t.options, kTarget_OptionD9159FE7);
        ReadPhotoOption(child, 0x09DA53D5u, t.options, kTarget_Option09DA53D5);
        ReadPhotoOption(child, 0x65567820u, t.options, kTarget_Option65567820);
        ReadPhotoOption(child, 0xDE86C5DEu, t.options, kTarget_OptionDE86C5DE);
        ReadPhotoOption(child, 0x9896A7D0u, t.options, kTarget_Option9896A7D0);
        ReadPhotoOption(child, 0xFAA75B88u, t.options, kTarget_OptionFAA75B88);

        // Texts: "DisplayName" fills all three, the specific ids override.
        if (ape::StringTable* table = ape::GetGuiStringTable()) {
            uint32_t id = 0;
            if (child->GetHash(kPhotoAttr_DisplayName, &id, 0) == ape::xml::kQuery_Ok && id != 0) {
                const char* text = table->Find(id, "[Missing Display Name]", false);
                t.checkBoxText = t.targetText = t.photoText = text;
            }
            id = 0;
            child->GetHash(0x6D28A670u, &id, 0);
            if (id != 0)
                t.checkBoxText = table->Find(id, "[Missing CheckBox Name]", false);
            id = 0;
            child->GetHash(0x37A369BEu, &id, 0);
            if (id != 0)
                t.targetText = table->Find(id, "[Missing Target Name]", false);
            id = 0;
            child->GetHash(0xC619D13Bu, &id, 0);
            if (id != 0)
                t.photoText = table->Find(id, "[Missing Photo Name]", false);
        }
        child->GetFloat(0x2D72DAD4u, &t.field_84, t.field_84);
        if (goal->targets.Count() < 8)
            goal->targets.PushBack(t);
    }
    return goal;
}

// @func 0x0032AAE8 GoalBinocucomPhoto::BuildMarker
void GoalBinocucomPhoto::BuildMarker(const Target& target, GoalMarker* marker) const
{
    marker->position[0] = target.markerOffset.x;
    marker->position[1] = target.markerOffset.y;
    marker->position[2] = target.markerOffset.z;
    marker->kind = GoalMarker::kKind_Photo;
    marker->entity = target.entity;
    // Options 0-5 go to bits 28-23 of the second flag word, 6-8 to bits
    // 23-21 of the first one (below the occlusion byte).
    marker->flags78 = (marker->flags78 & ~0x1F800000u) | ((target.options >> 3) & 0x1F800000u);
    marker->angle = target.angle;
    marker->bone = target.bone;
    marker->angles[0] = target.angles.x * kPhotoDegToRad;
    marker->angles[1] = target.angles.y * kPhotoDegToRad;
    marker->angles[2] = target.angles.z * kPhotoDegToRad;
    marker->distMin = target.distMin;
    marker->distMax = target.distMax;
    marker->distMinClose = target.distMinClose;
    marker->distMaxClose = target.distMaxClose;
    marker->distCull = target.distCull;
    marker->texts[0] = target.checkBoxText;
    marker->texts[1] = target.targetText;
    marker->texts[2] = target.photoText;
    marker->pending = target.pending;
    marker->displayTime = target.displayTime;
    marker->flags74 = (marker->flags74 & 0x001FFFFFu) | (uint32_t(target.occluded ? 1 : 0) << 24) |
                      ((target.options >> 2) & 0x00E00000u);
    marker->field_7C = target.field_84;
    marker->offset[0] = target.offset.x;
    marker->offset[1] = target.offset.y;
    marker->offset[2] = target.offset.z;
}

void GoalBinocucomPhoto::AddToActiveList()
{
    if (!s_activePhotoGoals) {
        s_currentPhotoTarget = 0;
        s_activePhotoGoals = this;
        s_currentPhotoGoal = this;
        // NOTE: the console leaves nextActive as it was; a goal removed from
        // the middle of the list would keep a stale link, so it is cleared.
        nextActive = nullptr;
        DropPhotoQuery();
        return;
    }
    for (GoalBinocucomPhoto* g = s_activePhotoGoals; g; g = g->nextActive)
        if (g == this)
            return;
    nextActive = s_activePhotoGoals;
    s_activePhotoGoals = this;
}

void GoalBinocucomPhoto::RemoveFromActiveList()
{
    if (s_activePhotoGoals == this) {
        s_activePhotoGoals = nextActive;
    } else {
        for (GoalBinocucomPhoto* g = s_activePhotoGoals; g; g = g->nextActive) {
            if (g->nextActive == this) {
                g->nextActive = nextActive;
                break;
            }
        }
    }
    if (s_currentPhotoGoal == this) {
        s_currentPhotoGoal = s_activePhotoGoals;
        DropPhotoQuery();
    }
}

// @func 0x0032AC78 GoalBinocucomPhoto::SetStatus
void GoalBinocucomPhoto::SetStatus(uint8_t newStatus, bool force)
{
    if (force || newStatus != status) {
        void* character = JobOwnerCharacter(job);
        if (newStatus == kGoal_Active) {
            AddToActiveList();
            if (targets.Count() == 0) {
                // Nothing to photograph: complete straight away.
                Goal::SetStatus(newStatus, force);
                SetStatus(kGoal_Complete, false);
                return;
            }
            if (void* binocucom = character ? CharacterBinocucom(character) : nullptr) {
                if (photoFilter != 0) {
                    // The console warns when the filter does not exist.
                    if (void* filter = FindPhotoFilter(photoFilter))
                        BinocucomSetPhotoFilter(binocucom, filter);
                }
                if (photoFlags & kPhoto_GadgetFlag)
                    BinocucomSetPhotoFlag(binocucom, true);
            }
            if ((photoFlags & kPhoto_CharacterFlag) && character && EntityIsCharacter(character))
                CharacterSetPhotoFlag(character, true);
        } else if (status == kGoal_Active) {
            RemoveFromActiveList();
            if ((photoFlags & kPhoto_GadgetFlag) && character) {
                if (void* binocucom = CharacterBinocucom(character)) {
                    BinocucomSetPhotoFlag(binocucom, false);
                    if (EntityIsCharacter(character))
                        CharacterInputMapSetBit(character, 0x800);
                }
            }
            if ((photoFlags & kPhoto_CharacterFlag) && character && EntityIsCharacter(character))
                CharacterSetPhotoFlag(character, false);
        }
    }
    Goal::SetStatus(newStatus, force);
}

void GoalBinocucomPhoto::UpdateOcclusion()
{
    const int count = targets.Count();
    // Moves the test on to the next target (and goal) once this one is done.
    auto advance = [&]() {
        if (++s_currentPhotoTarget >= count) {
            s_currentPhotoGoal = nextActive ? nextActive : s_activePhotoGoals;
            s_currentPhotoTarget = 0;
        }
    };
    for (int i = 0; i < count; ++i) {
        Target& target = targets[i];
        const bool isCurrent = s_currentPhotoGoal == this && s_currentPhotoTarget == i;
        if (!target.pending) {
            if (isCurrent)
                advance();
            continue;
        }
        if (!target.entity)
            continue;

        // A sphere of the occlusion radius around the target point (the
        // offset in the bone's space). NOTE: the console also builds the
        // rotation of the target's angles into the local transform, but only
        // the resulting translation is used.
        const ape::Matrix34 bone = GoalEntityBoneTransform(target.entity, target.bone);
        ape::Matrix34 sphere;
        const ape::Vec3 radius = {target.occlusionRadius, target.occlusionRadius, target.occlusionRadius, 0.0f};
        ape::Matrix34FromScale(sphere, radius);
        sphere.translation.x = bone.translation.x + target.offset.x * bone.xAxis.x +
                               target.offset.y * bone.yAxis.x + target.offset.z * bone.zAxis.x;
        sphere.translation.y = bone.translation.y + target.offset.x * bone.xAxis.y +
                               target.offset.y * bone.yAxis.y + target.offset.z * bone.zAxis.y;
        sphere.translation.z = bone.translation.z + target.offset.x * bone.xAxis.z +
                               target.offset.y * bone.yAxis.z + target.offset.z * bone.zAxis.z;

        if (!isCurrent)
            continue;
        if (!s_photoQuery) {
            s_photoQuery = agl::CreateOcclusionQuery(sphere, 8, 8);
            continue;
        }
        uint32_t pixels[2] = {0, 0};   // drawn, visible
        if (s_photoQuery->GetResult(pixels, 2)) {
            bool occluded = true;
            if (std::fabs(target.visibleFraction - 1.0f) < 1e-5f)
                occluded = pixels[1] == 0;
            else if (float(pixels[1]) / float(pixels[0]) > target.visibleFraction)
                occluded = false;
            target.occluded = occluded;
            DropPhotoQuery();
            advance();
        } else {
            s_photoQuery->transform.xAxis = {sphere.xAxis.x, sphere.xAxis.y, sphere.xAxis.z, s_photoQuery->transform.xAxis.pad};
            s_photoQuery->transform.yAxis = {sphere.yAxis.x, sphere.yAxis.y, sphere.yAxis.z, s_photoQuery->transform.yAxis.pad};
            s_photoQuery->transform.zAxis = {sphere.zAxis.x, sphere.zAxis.y, sphere.zAxis.z, s_photoQuery->transform.zAxis.pad};
            s_photoQuery->transform.translation = {sphere.translation.x, sphere.translation.y,
                                                   sphere.translation.z, s_photoQuery->transform.translation.pad};
        }
    }
}

// @func 0x0032BCD0 GoalBinocucomPhoto::Update
uint8_t GoalBinocucomPhoto::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active) {
        void* character = JobOwnerCharacter(job);
        if ((photoFlags & kPhoto_GadgetFlag) && character) {
            void* binocucom = CharacterBinocucom(character);
            if (binocucom && !BinocucomViewUp(binocucom) && EntityIsCharacter(character))
                CharacterInputMapSetBit(character, 0x400);
        }
        switch (e.type) {
        case kEvent_06:
            // A target that is still needed has gone away.
            for (const Target& target : targets) {
                if (target.pending && (!target.entity || !GoalEntityAlive(target.entity))) {
                    SetStatus(kGoal_Failed, false);
                    break;
                }
            }
            break;
        case kEvent_07: {
            // A photo was taken of the entity named in the event.
            const uint32_t photographed = uint32_t(e.arg0);
            for (Target& target : targets) {
                if (target.name == photographed) {
                    target.pending = false;
                    if (character)
                        JobOwnerUntarget(job, target.name);
                    ++repeatProgress;
                }
                if ((photoFlags & kPhoto_ReportGroup) && repeatProgress < targets.Count())
                    JobManagerGroupAdvanced();
            }
            if (repeatProgress == targets.Count())
                SetStatus(kGoal_Complete, false);
            break;
        }
        case kEvent_0A:
            UpdateOcclusion();
            break;
        default:
            break;
        }
    }
    return Goal::Update(player, e);
}

bool GoalBinocucomPhoto::ResolveTargets(bool pending)
{
    for (Target& target : targets) {
        if (target.name != 0) {
            target.entity = GoalFindEntity(target.name);
            if (!target.entity || !GoalEntityAlive(target.entity)) {
                SetStatus(kGoal_Failed, false);
                return false;
            }
        }
        target.pending = pending;
    }
    return true;
}

// @func 0x0032C3F4 GoalBinocucomPhoto::Reset
void GoalBinocucomPhoto::Reset(bool fromSave, int requested)
{
    Goal::Reset(fromSave, requested);
    if (requested == kGoal_Active)
        ResolveTargets(true);
}

// @func 0x0032C4E0 GoalBinocucomPhoto::ResetHints
void GoalBinocucomPhoto::ResetHints(bool stopHintScript)
{
    for (Target& target : targets) {
        if (!(flags & kGoalFlag_KeepComplete))
            target.pending = true;
        if (target.entity)
            JobOwnerUntarget(job, GoalEntityName(target.entity));
        target.entity = nullptr;
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x0032C5E4 GoalBinocucomPhoto::ReapplyHints
void GoalBinocucomPhoto::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    // A completed photo goal still resolves its targets while its group runs.
    if (status == kGoal_Active ||
        (status == kGoal_Complete && owner && owner->status == kGoal_Active))
        ResolveTargets(status == kGoal_Active);
}

// @func 0x0032C700 GoalBinocucomPhoto::CollectMarkers
void GoalBinocucomPhoto::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (const Target& target : targets) {
        if (!target.entity || !target.pending || !GoalEntityPhotographable(target.entity))
            continue;
        GoalMarker marker;
        BuildMarker(target, &marker);
        list->markers.PushBack(marker);
    }
}

// @func 0x0032C898 GoalBinocucomPhoto::CollectAllMarkers
void GoalBinocucomPhoto::CollectAllMarkers(GoalMarkerList* list)
{
    for (const Target& target : targets) {
        if (!target.entity)
            continue;
        GoalMarker marker;
        BuildMarker(target, &marker);
        list->markers.PushBack(marker);
    }
}

} // namespace goals
} // namespace sly
