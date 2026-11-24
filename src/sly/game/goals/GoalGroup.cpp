// openTIT - Goal groups (console vtable 0x00B1E278).
#include "sly/game/goals/GoalTypes.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "sly/game/goals/GoalHooks.h"

#include <cstdio>

namespace sly {
namespace goals {

namespace {

// Hint script variables and states of a progress group.
constexpr uint32_t kVar_LastCompleted  = 0x502765C7;
constexpr uint32_t kVar_CompletedCount = 0x4D9D2995;
constexpr uint32_t kVar_AllComplete    = 0xD808DBB9;
constexpr uint32_t kState_ChildDone    = 0xB35324B5;
constexpr uint32_t kState_AllComplete  = 0xD808DBB9;

void SetIntVar(ape::ApeAnimScriptInstance* instance, uint32_t name, int32_t value)
{
    ape::ApeAnimScriptVar* var = instance->FindVar(name);
    if (var && var->GetType() == ape::AnimScriptVarType::Int)
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(value);
}

float DistanceSquared(const float a[3], const float b[3])
{
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

// @func 0x003266E4 GoalGroup::~GoalGroup
// @func 0x003269F0 GoalGroup::~GoalGroup (deleting)
// @func 0x00326A38 GoalGroup::~GoalGroup (base object)
GoalGroup::~GoalGroup()
{
    for (Goal* child = firstChild; child;) {
        Goal* next = child->nextGoal;
        delete child;
        child = next;
    }
    firstChild = nullptr;
}

// @func 0x00321CF4 GoalGroup::AddChild
void GoalGroup::AddChild(Goal* child)
{
    if (!firstChild) {
        firstChild = child;
    } else {
        Goal* last = firstChild;
        while (last->nextGoal)
            last = last->nextGoal;
        last->nextGoal = child;
    }
    child->parentGoal = this;
}

void GoalGroup::PlayChildCompleteState(int count)
{
    char name[32];
    std::snprintf(name, sizeof(name), "childComplete%i", count);
    hintScript->SetStateAndUpdate(ape::HashNameNoCase(name));
}

void GoalGroup::ActivateChild(Goal* child)
{
    child->SetFlags(child->flags & ~kGoalFlag_Lock3);
    child->SetStatus(kGoal_Active, false);
    child->ReapplyHints(this);
    if (child->showDescription) {
        const char* text = child->GetDescription();
        if (text && *text && uint32_t(child->index) < JobGoalCount(job)) {
            // Flag the job goal's save word so the HUD shows the new text.
            Goal* jobGoal = JobGoalAt(job, uint32_t(child->index));
            JobSetGoalSaveWord(job, jobGoal->saveSlot,
                               JobGoalSaveWord(job, jobGoal->saveSlot) | 0x08000000u);
        }
    }
    if (groupFlags & kGroup_NotifyAdvance)
        JobManagerGroupAdvanced();
}

// @func 0x00326A58 GoalGroup::Update
uint8_t GoalGroup::Update(int player, const GameEvent& e)
{
    if (status != kGoal_Active)
        return Goal::Update(player, e);

    bool done = false;
    bool failed = firstChild == nullptr;
    bool evaluate = true;   // false: skip straight to the failure test

    switch (mode) {
    case kMode_All:
    case kMode_NotAll:
        if (firstChild) {
            bool all = true;
            for (Goal* child = firstChild; child; child = child->nextGoal) {
                const uint8_t result = child->Update(player, e);
                if (child->countsForGroup)
                    all = all && result == kGoal_Complete;
                failed = failed || child->status == kGoal_Failed;
            }
            done = all;
        }
        if (mode == kMode_NotAll)
            done = !done;
        break;

    case kMode_Any:
    case kMode_None:
        // NOTE: in these modes a completed child that does not count makes
        // the group fail (kept from the console).
        for (Goal* child = firstChild; child && !done; child = child->nextGoal) {
            const uint8_t result = child->Update(player, e);
            if (child->countsForGroup)
                done = result == kGoal_Complete;
            failed = failed || child->status == kGoal_Complete;
        }
        if (mode == kMode_None)
            done = !done;
        break;

    case kMode_AllProgress: {
        if (!firstChild) {
            done = true;
            break;
        }
        int completed = 0;
        int lastNew = 0;
        bool allComplete = true;
        bool everyChild = true;
        int i = 0;
        for (Goal* child = firstChild; child; child = child->nextGoal, ++i) {
            const uint8_t before = child->status;
            const uint8_t result = child->Update(player, e);
            allComplete = allComplete && result == kGoal_Complete;
            failed = failed || result == kGoal_Failed;
            if (result == kGoal_Complete) {
                if (before != kGoal_Complete)
                    lastNew = i;
                ++completed;
            } else {
                everyChild = false;
            }
        }
        if (GameHudLockCount(game) == 0) {
            if (hintScript && completed != completedCount) {
                SetIntVar(hintScript, kVar_LastCompleted, lastNew + 1);
                SetIntVar(hintScript, kVar_CompletedCount, completed);
                SetIntVar(hintScript, kVar_AllComplete, everyChild ? 1 : 0);
                hintScript->SetStateAndUpdate(kState_ChildDone);
                PlayChildCompleteState(completed);
                if (everyChild)
                    hintScript->SetStateAndUpdate(kState_AllComplete);
            }
            completedCount = completed;
        }
        done = allComplete;
        break;
    }

    case kMode_Sequence: {
        if (!firstChild) {
            evaluate = false;
            break;
        }
        int completed = 0;
        failed = true;
        for (Goal* child = firstChild; child; child = child->nextGoal) {
            const uint8_t before = child->status;
            const uint8_t result = child->Update(player, e);
            if (child == firstChild && child->status < kGoal_Active) {
                ActivateChild(child);
            } else {
                done = child->status == kGoal_Complete;
                if (!done) {
                    failed = child->status == kGoal_Failed;
                    break;
                }
                ++completed;
                Goal* next = child->nextGoal;
                if (next && next->status < kGoal_Active)
                    ActivateChild(next);
            }
            if ((groupFlags & kGroup_StopOnNewCompletion) && result == kGoal_Complete &&
                before != kGoal_Complete) {
                done = false;
                failed = false;
                break;
            }
        }
        if (GameHudLockCount(game) == 0) {
            if (hintScript && completed != completedCount)
                PlayChildCompleteState(completed);
            completedCount = completed;
        }
        break;
    }

    default:
        evaluate = false;
        break;
    }

    if (evaluate && done) {
        if (groupFlags & kGroup_WaitActiveScripts)
            done = scripts[kScripts_Active].finished;
        if (done) {
            if (repeatCount == 0)
                return kGoal_Complete;
            if (++repeatProgress >= repeatCount) {
                SetStatus(kGoal_Complete, false);
                // Settle the children: the inverted modes fail them.
                for (Goal* child = firstChild; child; child = child->nextGoal) {
                    if (mode == kMode_None || mode == kMode_NotAll) {
                        if (!(child->flags & kGoalFlag_KeepComplete) || child->status != kGoal_Complete)
                            child->SetStatus(kGoal_Failed, false);
                    } else {
                        child->SetStatus(kGoal_Complete, false);
                    }
                }
            }
            return Goal::Update(player, e);
        }
    }

    if (failed) {
        if (!failRestarts) {
            SetStatus(kGoal_Failed, false);
            for (Goal* child = firstChild; child; child = child->nextGoal) {
                if (mode == kMode_NotAll || mode == kMode_None)
                    child->SetStatus(kGoal_Complete, false);
                else if (!(child->flags & kGoalFlag_KeepComplete) || child->status != kGoal_Complete)
                    child->SetStatus(kGoal_Failed, false);
            }
        } else {
            Reset(true, -1);
        }
    }
    return Goal::Update(player, e);
}

// @func 0x00327440 GoalGroup::OnLevelLoaded
void GoalGroup::OnLevelLoaded()
{
    for (Goal* child = firstChild; child; child = child->nextGoal)
        child->OnLevelLoaded();
}

// @func 0x003277E4 GoalGroup::Reset
void GoalGroup::Reset(bool fromSave, int requested)
{
    Goal::Reset(fromSave, requested);
    const uint8_t own = status;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        // A running sequence restarts its unfinished children from scratch.
        if (mode == kMode_Sequence && own == kGoal_Active &&
            (child->status != kGoal_Complete || (failRestarts && fromSave))) {
            child->SetFlags(child->flags | kGoalFlag_Lock3);
            child->Reset(false, -1);
        } else {
            child->Reset(fromSave, own);
        }
    }
}

// @func 0x003274D0 GoalGroup::ResetHints
void GoalGroup::ResetHints(bool stopHintScript)
{
    for (Goal* child = firstChild; child; child = child->nextGoal)
        child->ResetHints(stopHintScript);
    Goal::ResetHints(stopHintScript);
}

// @func 0x0032758C GoalGroup::PrepareScripts
void GoalGroup::PrepareScripts()
{
    Goal::PrepareScripts();
    for (Goal* child = firstChild; child; child = child->nextGoal)
        child->PrepareScripts();
}

// @func 0x00327684 GoalGroup::ReapplyHints
void GoalGroup::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    for (Goal* child = firstChild; child; child = child->nextGoal)
        child->ReapplyHints(this);
}

// @func 0x00327920 GoalGroup::SetFlags
void GoalGroup::SetFlags(uint32_t value)
{
    flags = value;
}

// @func 0x003280F8 GoalGroup::GetDescription
const char* GoalGroup::GetDescription()
{
    if (status == kGoal_Active) {
        for (Goal* child = firstChild; child; child = child->nextGoal) {
            if (child->status != kGoal_Active || !child->showDescription)
                continue;
            const char* text = child->GetDescription();
            if (text && *text)
                return child->GetDescription();
        }
    }
    return Goal::GetDescription();
}

// @func 0x00327928 GoalGroup::GetProgress
bool GoalGroup::GetProgress(GoalProgress* out)
{
    if (progressName == 0) {
        for (Goal* child = firstChild; child; child = child->nextGoal)
            if (child->GetProgress(out))
                return true;
        return false;
    }
    out->name = progressName;
    out->done = progressBase;
    out->total = progressBase;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        if (!(child->flags & kGoalFlag_CountsProgress))
            continue;
        if (child->IsComplete())
            ++out->done;
        ++out->total;
    }
    return true;
}

// @func 0x00328470 GoalGroup::IsComplete
bool GoalGroup::IsComplete()
{
    if (status == kGoal_Complete)
        return true;
    for (Goal* child = firstChild; child; child = child->nextGoal)
        if (child->completesParent && child->status == kGoal_Complete)
            return true;
    return false;
}

// @func 0x00327A58 GoalGroup::SelectNearestMarkers
void GoalGroup::SelectNearestMarkers(GoalMarkerList* list, const GoalMarkerList& found) const
{
    const uint32_t count = found.markers.Count();
    const uint32_t keep = count < maxMarkers ? count : maxMarkers;
    if (count > 16 || count <= maxMarkers) {
        // Too many to sort (or few enough to show all): take them in order.
        for (uint32_t i = 0; i < keep; ++i)
            list->markers.PushBack(found.markers[i]);
        return;
    }

    // Keep the `keep` nearest markers, nearest first (insertion by score,
    // the score being the negated squared distance to the player).
    float bestScore[16];
    const GoalMarker* best[16];
    for (uint32_t i = 0; i < keep; ++i) {
        bestScore[i] = -3.4028235e38f;
        best[i] = nullptr;
    }
    float player[3];
    PlayerPosition(player);
    for (uint32_t i = 0; i < count; ++i) {
        const GoalMarker& marker = found.markers[i];
        float score = 0.0f;
        if (marker.entity) {
            float pos[3];
            EntityPosition(marker.entity, pos);
            score = -DistanceSquared(player, pos);
        }
        if (score <= bestScore[0])
            continue;
        uint32_t slot = 1;
        while (slot < keep && score >= bestScore[slot]) {
            bestScore[slot - 1] = bestScore[slot];
            best[slot - 1] = best[slot];
            ++slot;
        }
        bestScore[slot - 1] = score;
        best[slot - 1] = &marker;
    }
    for (uint32_t i = 0; i < keep; ++i)
        if (best[i])
            list->markers.PushBack(*best[i]);
}

// @func 0x00327CB8 GoalGroup::CollectMarkers
void GoalGroup::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    GoalMarkerList found;
    GoalMarkerList* target = maxMarkers == 0 ? list : &found;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        if (child->status != kGoal_Active)
            continue;
        child->CollectMarkers(target);
        if (mode == kMode_Sequence)
            break;
    }
    if (maxMarkers != 0)
        SelectNearestMarkers(list, found);
}

// @func 0x00327E0C GoalGroup::GetTarget
bool GoalGroup::GetTarget(GoalTarget* out)
{
    if (status != kGoal_Active)
        return false;
    for (Goal* child = firstChild; child; child = child->nextGoal)
        if (child->status == kGoal_Active && child->GetTarget(out))
            return true;
    return Goal::GetTarget(out);
}

// @func 0x00327EDC GoalGroup::Field8A0
uint32_t GoalGroup::Field8A0()
{
    if (status != kGoal_Active)
        return 0;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        if (child->status != kGoal_Active)
            continue;
        if (uint32_t value = child->Field8A0())
            return value;
    }
    return field_8A0;
}

// @func 0x00327F78 GoalGroup::CollectAllMarkers
void GoalGroup::CollectAllMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        child->CollectAllMarkers(list);
        if (child->status == kGoal_Active && mode == kMode_Sequence)
            break;
    }
}

// @func 0x0032801C GoalGroup::Field8F4
uint32_t GoalGroup::Field8F4(int i)
{
    if (status == kGoal_Active) {
        for (Goal* child = firstChild; child; child = child->nextGoal)
            if (child->status == kGoal_Active && child->Field8F4(i) != 0)
                return child->Field8F4(i);
    }
    return field_8F4[i];
}

// @func 0x0032820C GoalGroup::CurrentCycleText
const char* GoalGroup::CurrentCycleText(float* fade)
{
    // NOTE: the console asks the first child once per child (not each child
    // in turn); kept.
    for (Goal* child = firstChild; child; child = child->nextGoal)
        if (const char* text = firstChild->CurrentCycleText(fade))
            return text;
    return Goal::CurrentCycleText(fade);
}

// @func 0x003282E8 GoalGroup::ResetCycle
void GoalGroup::ResetCycle()
{
    // NOTE: like CurrentCycleText, resets the first child once per child.
    for (Goal* child = firstChild; child; child = child->nextGoal)
        firstChild->ResetCycle();
    Goal::ResetCycle();
}

// @func 0x003283A8 GoalGroup::FreezeStatus
void GoalGroup::FreezeStatus(bool frozen)
{
    Goal::FreezeStatus(frozen);
    for (Goal* child = firstChild; child; child = child->nextGoal)
        child->FreezeStatus(frozen);
}

// @func 0x003284C4 GoalGroup::SetStatus
void GoalGroup::SetStatus(uint8_t newStatus, bool force)
{
    Goal::SetStatus(newStatus, force);
    completedCount = 0;
    for (Goal* child = firstChild; child; child = child->nextGoal) {
        // Children follow the group (a sequence drives its own children),
        // except completed ones that keep their completion.
        if (!force && mode != kMode_Sequence &&
            (newStatus == kGoal_Complete || !(child->flags & kGoalFlag_KeepComplete) ||
             child->status != kGoal_Complete))
            child->SetStatus(newStatus, false);
        if ((child->flags & kGoalFlag_KeepComplete) && child->status == kGoal_Complete)
            ++completedCount;
    }
}

} // namespace goals
} // namespace sly
