// openTIT - Follow, health monitor and entity marker goals.
#include "sly/game/goals/GoalTypes.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/goals/GoalHooks.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>

namespace sly {
namespace goals {

namespace {

constexpr uint32_t kAttr_Target = 0x7A30D7E3;
const float kZero[3] = {0.0f, 0.0f, 0.0f};

// The option attributes shared by the entity goals, highest bit first.
constexpr uint32_t kOptionAttrs[6] = {
    0x5D28E3FC, 0x9446A097, 0xD9159FE7, 0x09DA53D5, 0x04656B91, 0x310F51B2,
};

uint32_t ReadOptions(ape::xml::XmlElement* xml, const uint32_t* attrs, int count, int topBit)
{
    uint32_t bits = 0;
    for (int i = 0; i < count; ++i) {
        bool value = false;
        xml->GetBool(attrs[i], &value, false);
        if (value)
            bits |= 1u << (topBit - i);
    }
    return bits;
}

void SetVec3(ape::Vec3& v, const float in[3])
{
    v.x = in[0];
    v.y = in[1];
    v.z = in[2];
}

// Follow hint script states.
constexpr uint32_t kFollow_StateWarnOn   = 0x7CBE9D59;
constexpr uint32_t kFollow_StateWarnOff  = 0x684DE37B;
constexpr uint32_t kFollow_StateFar      = 0xB56E3B03;
constexpr uint32_t kFollow_StateMid      = 0xAADA026B;
constexpr uint32_t kFollow_StateNear     = 0x08BFC31E;

} // namespace

// ===========================================================================
// GoalFollow

// @func 0x0033162C GoalFollow::GoalFollow
GoalFollow::GoalFollow(const GoalDesc* desc, const float offsetIn[3], uint32_t targetName,
                       float minDistance, float maxDistance, float failAfter, float completeAfter,
                       uint32_t options)
    : Goal(desc)
{
    SetVec3(offset, offsetIn);
    target = targetName;
    minDistanceSq = minDistance * minDistance;
    maxDistanceSq = maxDistance * maxDistance;
    failTime = failAfter;
    completeTime = completeAfter;
    entity = nullptr;
    farTime = 0.0f;
    nearTime = 0.0f;
    followFlags = options;
    lastDistanceSq = -1.0f;
}

// @func 0x00331788 GoalFollow::~GoalFollow
// @func 0x00331A28 GoalFollow::~GoalFollow (deleting)
GoalFollow::~GoalFollow() = default;

// @func 0x00331A78 GoalFollow::Create
Goal* GoalFollow::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float offset[3];
    xml->GetVec3(0x4A4D4D4Du, offset, kZero);
    uint32_t target = 0;
    xml->GetHash(0x1A30BED4u, &target, 0);
    float minDistance = 0.0f;
    float maxDistance = 0.0f;
    float failAfter = 0.0f;
    float completeAfter = -1.0f;
    xml->GetFloat(0x2FA14783u, &minDistance, 0.0f);
    xml->GetFloat(0x9E3D6E98u, &maxDistance, 0.0f);
    xml->GetFloat(0x56EF5AEBu, &failAfter, 0.0f);
    xml->GetFloat(0xC26BCCA9u, &completeAfter, -1.0f);
    bool marker = false;
    xml->GetBool(0x76E96977u, &marker, false);
    bool unused = false;
    xml->GetBool(0x3B66FD69u, &unused, false);
    // NOTE: the order the six option bools land in bits 30-25 is assumed.
    uint32_t options = ReadOptions(xml, kOptionAttrs, 6, 30);
    if (marker)
        options |= kFollow_ShowMarker;
    return new GoalFollow(desc, offset, target, minDistance, maxDistance, failAfter, completeAfter,
                          options);
}

// @func 0x00331D90 GoalFollow::Update
uint8_t GoalFollow::Update(int player, const GameEvent& e)
{
    if (status != kGoal_Active)
        return Goal::Update(player, e);
    if (!entity)
        return kGoal_Complete;
    if (e.type != kEvent_06)
        return Goal::Update(player, e);

    if (!GoalEntityAlive(entity)) {
        SetStatus(kGoal_Failed, false);
        return Goal::Update(player, e);
    }
    void* character = PlayerCharacter(player);
    if (!character)
        return Goal::Update(player, e);
    if (!GoalEntityAlive(character)) {
        SetStatus(kGoal_Failed, false);
        return Goal::Update(player, e);
    }

    float a[3];
    float b[3];
    EntityPosition(entity, a);
    EntityPosition(character, b);
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    const float distSq = dz * dz + dx * dx + dy * dy;
    const bool far = distSq > maxDistanceSq;
    const bool mid = distSq > minDistanceSq && distSq <= maxDistanceSq;
    const bool near = distSq <= minDistanceSq;

    if (lastDistanceSq != distSq) {
        bool wasFar = false;
        bool wasMid = false;
        bool wasNear = false;
        if (lastDistanceSq >= 0.0f) {
            wasFar = lastDistanceSq > maxDistanceSq;
            wasMid = lastDistanceSq > minDistanceSq && lastDistanceSq <= maxDistanceSq;
            wasNear = lastDistanceSq <= minDistanceSq;
        }
        // Out of the inner range: warn on the HUD and in the hint script.
        const bool warn = mid || far;
        if (warn != ((followFlags & kFollow_TooFar) != 0)) {
            HudSetFollowWarning(warn);
            followFlags = warn ? (followFlags | kFollow_TooFar) : (followFlags & ~kFollow_TooFar);
            if (hintScript)
                hintScript->SetStateAndUpdate(warn ? kFollow_StateWarnOn : kFollow_StateWarnOff);
        }
        if (far && !wasFar) {
            if (hintScript)
                hintScript->SetStateAndUpdate(kFollow_StateFar);
        } else if (mid && !wasMid) {
            if (hintScript)
                hintScript->SetStateAndUpdate(kFollow_StateMid);
        } else if (near && !wasNear && hintScript) {
            hintScript->SetStateAndUpdate(kFollow_StateNear);
        }
        lastDistanceSq = distSq;
    }

    if (near) {
        farTime = 0.0f;
        nearTime += e.arg0Float;
    } else if (far) {
        nearTime = 0.0f;
        farTime += e.arg0Float;
    }
    if (failTime >= 0.0f && failTime < farTime) {
        SetStatus(kGoal_Failed, false);
        return kGoal_Failed;
    }
    // NOTE: byte +0x10 (the group repeat count) gates the timed completion.
    if (repeatCount == 0 || completeTime <= 0.0f)
        return kGoal_Complete;
    if (completeTime <= nearTime) {
        SetStatus(kGoal_Complete, false);
        return kGoal_Complete;
    }
    return Goal::Update(player, e);
}

// @func 0x00332330 GoalFollow::ResetHints
void GoalFollow::ResetHints(bool stopHintScript)
{
    farTime = 0.0f;
    nearTime = 0.0f;
    if (entity) {
        JobOwnerUntarget(job, GoalEntityName(entity));
        GoalEntityRelease(entity);
        entity = nullptr;
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x003323EC GoalFollow::ReapplyHints
void GoalFollow::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    if (!entity && target != 0) {
        entity = GoalFindEntity(target);
        if (entity)
            GoalEntityAddRef(entity);
    }
}

// @func 0x00332490 GoalFollow::CollectMarkers
void GoalFollow::CollectMarkers(GoalMarkerList* list)
{
    if (status == kGoal_Active && entity && (followFlags & kFollow_ShowMarker))
        GoalAddEntityMarker(list, entity);
}

// @func 0x00332624 GoalFollow::SetStatus
void GoalFollow::SetStatus(uint8_t newStatus, bool force)
{
    if ((force || newStatus != status) && newStatus != kGoal_Active)
        ResetHints(false);
    if (newStatus == kGoal_Active) {
        followFlags &= ~kFollow_TooFar;
    } else if (followFlags & kFollow_TooFar) {
        HudSetFollowWarning(false);
        followFlags &= ~kFollow_TooFar;
        if (hintScript)
            hintScript->SetStateAndUpdate(kFollow_StateWarnOff);
    }
    Goal::SetStatus(newStatus, force);
}

// ===========================================================================
// GoalHealthMonitor

// @func 0x0033273C GoalHealthMonitor::GoalHealthMonitor
GoalHealthMonitor::GoalHealthMonitor(const GoalDesc* desc, const ape::ApeTArray<float>& list,
                                     uint32_t targetName, bool isPercent)
    : Goal(desc), thresholds(4)
{
    entity = nullptr;
    lastHealth = 0.0f;
    target = targetName;
    percent = isPercent;
    for (float value : list)
        thresholds.PushBack(value);
}

// @func 0x00332DF4 CompareThresholds
// Sort order of the thresholds: highest first (the console's qsort callback).
static int CompareThresholds(const void* a, const void* b)
{
    const float x = *static_cast<const float*>(a);
    const float y = *static_cast<const float*>(b);
    if (x < y)
        return 1;
    if (y < x)
        return -1;
    return 0;
}

// @func 0x00332898 GoalHealthMonitor::Create
Goal* GoalHealthMonitor::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    ape::ApeTArray<float> list(4);
    uint32_t target = 0;
    bool percent = false;
    xml->GetHash(kAttr_Target, &target, 0);
    xml->GetBool(0x4FCD42D9u, &percent, false);
    for (ape::xml::XmlElement* child = xml->FirstChildElement(); child;
         child = child->NextSiblingElement()) {
        if (child->GetNameHash() != 0x49F9F44Bu)
            continue;
        float value = 0.0f;
        child->GetFloat(0x0F9A3C75u, &value, 0.0f);
        list.PushBack(value);
    }
    std::qsort(list.Data(), list.Count(), sizeof(float), CompareThresholds);
    return new GoalHealthMonitor(desc, list, target, percent);
}

// @func 0x00332AA8 GoalHealthMonitor::Update
uint8_t GoalHealthMonitor::Update(int player, const GameEvent& e)
{
    if (status != kGoal_Active)
        return Goal::Update(player, e);
    if (entity && GoalEntityAlive(entity) && e.type == kEvent_06) {
        float health = GoalEntityHealth(entity);
        float last = lastHealth;
        if (health < last) {
            const float maxHealth = GoalEntityMaxHealth(entity);
            if (percent && maxHealth > 0.0f) {
                last /= maxHealth;
                health /= maxHealth;
            }
            for (uint32_t i = 0; i < thresholds.Count(); ++i) {
                const float t = thresholds[i];
                if (t <= last && health <= t) {
                    if (hintScript) {
                        char state[32];
                        std::snprintf(state, sizeof(state), "Threshold%i_Passed", int(i + 1));
                        hintScript->SetStateAndUpdate(ape::HashNameNoCase(state));
                    }
                    break;
                }
            }
        }
        // NOTE: with `percent` the console stores the normalised value.
        lastHealth = health;
    }
    return kGoal_Complete;
}

// @func 0x00332C00 GoalHealthMonitor::ResetHints
void GoalHealthMonitor::ResetHints(bool stopHintScript)
{
    lastHealth = 0.0f;
    if (entity) {
        GoalEntityRelease(entity);
        entity = nullptr;
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x00332C84 GoalHealthMonitor::ReapplyHints
void GoalHealthMonitor::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    if (entity || target == 0)
        return;
    void* found = GoalFindEntity(target);
    if (found && GoalEntityIsActor(found)) {
        entity = found;
        lastHealth = GoalEntityHealth(found);
        GoalEntityAddRef(found);
    }
}

// @func 0x00332D58 GoalHealthMonitor::SetStatus
void GoalHealthMonitor::SetStatus(uint8_t newStatus, bool force)
{
    if ((force || newStatus != status) && newStatus != kGoal_Active)
        ResetHints(false);
    Goal::SetStatus(newStatus, force);
}

// ===========================================================================
// GoalMarkEntity

// @func 0x00332E28 GoalMarkEntity::GoalMarkEntity
GoalMarkEntity::GoalMarkEntity(const GoalDesc* desc, uint32_t targetName, const float offsetIn[3],
                               uint32_t options, uint32_t value)
    : Goal(desc)
{
    SetVec3(offset, offsetIn);
    target = targetName;
    field_A10 = value;
    entity = nullptr;
    countsForGroup = false;
    markFlags = options;
}

// @func 0x00332F44 GoalMarkEntity::~GoalMarkEntity
// @func 0x003331E4 GoalMarkEntity::~GoalMarkEntity (deleting)
GoalMarkEntity::~GoalMarkEntity() = default;

// @func 0x0033322C GoalMarkEntity::Create
Goal* GoalMarkEntity::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t target = 0;
    xml->GetHash(kAttr_Target, &target, 0);
    float offset[3];
    xml->GetVec3(0x8ACA5893u, offset, kZero);
    bool unused = false;
    xml->GetBool(0x3B66FD69u, &unused, false);
    // NOTE: assumed bit order: the shared options in bits 31-26, then the
    // goal's own 0x37785A87 in bit 25 (the only bit it tests).
    static const uint32_t kAttrs[7] = {
        0x5D28E3FC, 0x9446A097, 0xD9159FE7, 0x09DA53D5, 0x04656B91, 0x310F51B2, 0x37785A87,
    };
    uint32_t value = 0;
    xml->GetHash(0x6E6EDD6Bu, &value, 0);
    const uint32_t options = ReadOptions(xml, kAttrs, 7, 31);
    return new GoalMarkEntity(desc, target, offset, options, value);
}

// @func 0x00333478 GoalMarkEntity::Update
uint8_t GoalMarkEntity::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active)
        return kGoal_Complete;
    return Goal::Update(player, e);
}

// @func 0x003334C8 GoalMarkEntity::ResetHints
void GoalMarkEntity::ResetHints(bool stopHintScript)
{
    if (entity) {
        JobOwnerUntarget(job, GoalEntityName(entity));
        GoalEntityRelease(entity);
        entity = nullptr;
    }
    Goal::ResetHints(stopHintScript);
}

// @func 0x00333578 GoalMarkEntity::ReapplyHints
void GoalMarkEntity::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    if (!entity && target != 0) {
        entity = GoalFindEntity(target);
        if (entity)
            GoalEntityAddRef(entity);
    }
}

// @func 0x0033361C GoalMarkEntity::CollectMarkers
void GoalMarkEntity::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active || !entity)
        return;
    if ((markFlags & kMark_HideWhileTargeted) && GoalEntityMarkerSuppressed(entity))
        return;
    if (GoalEntityAllowsMarker(entity))
        GoalAddEntityMarker(list, entity);
}

// @func 0x0033380C GoalMarkEntity::SetStatus
void GoalMarkEntity::SetStatus(uint8_t newStatus, bool force)
{
    const bool changed = force || newStatus != status;
    if (changed && newStatus != kGoal_Active)
        ResetHints(false);
    Goal::SetStatus(newStatus, force);
    if (changed && newStatus == kGoal_Active && !entity)
        ReapplyHints(nullptr);
}

} // namespace goals
} // namespace sly
