// openTIT - Camera entity and interaction goals.
#include "sly/game/goals/GoalTypes.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/goals/GoalHooks.h"

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
