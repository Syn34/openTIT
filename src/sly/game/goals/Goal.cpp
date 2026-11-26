// openTIT - Job goal base class (console vtable 0x00B1E1F8).
#include "sly/game/goals/Goal.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "sly/game/FileStreaming.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/gameplay/Sequencer.h"

#include <cstring>
#include <vector>

namespace sly {
namespace goals {

namespace {

// Library of the goal scripts and sequences.
constexpr uint32_t kGoalScriptLibrary = 0x7719A548;
// Hint script states.
constexpr uint32_t kHintState_End      = 0x00010CBB;   // "End"
constexpr uint32_t kHintState_Activate = 0xDCCA6EB0;
constexpr uint32_t kHintState_Complete = 0x264D6485;
// Message id (kGameEventNames entry 0x17) that is forwarded as event 0x17.
constexpr uint32_t kMsg_GoalEvent = 0x71445310;
// Checkpoint name that matches the player character.
constexpr uint32_t kCheckpoint_Sly = 0x00014120;       // "Sly"
// NOTE: fade time of the cycling texts. The console reads it from a global
// (0x0106B2AC) whose initialiser was not found; 0.5 s is an assumption.
constexpr float kCycleFadeTime = 0.5f;
// First update of a freshly started anim script.
constexpr float kFirstUpdateStep = 1.0f / 60.0f;

uint32_t ScriptCount(const GoalScriptCursor& cursor)
{
    return cursor.list ? cursor.list->Count() : 0;
}

GoalScript* ScriptAt(const GoalScriptCursor& cursor, uint32_t i)
{
    return (*cursor.list->scripts)[i];
}

} // namespace

const uint32_t kGameEventNames[0x21] = {
    0x00000000, 0x879F2BDA, 0x6362ED0A, 0x03B7913C, 0xA0D76C20, 0x57C506B6, 0x04C20F25,
    0xBE194E0B, 0x084A5778, 0x1EE967E4, 0xBE52171E, 0x2990B813, 0x0886499B, 0x6A46FCAF,
    0xAC70D594, 0xCC517C5C, 0x69988172, 0xFE2D0293, 0x65631C62, 0xD8F4C1C9, 0xDFEA6076,
    0x7113F25D, 0xB1BC9747, 0x71445310, 0x62C083CB, 0x78B40788, 0x3F7B9656, 0xCDCED4B4,
    0x77297F71, 0x2B1FBAB8, 0xFB0005EA, 0x5E503067, 0x1DD1C81B,
};

// @func 0x00321288 GoalCycleEntry::SetText
void GoalCycleEntry::SetText(const char* value)
{
    delete[] text;
    text = nullptr;
    if (value && *value) {
        text = new char[std::strlen(value) + 1];
        std::strcpy(text, value);
    }
}

namespace {

// Bounded copy that always terminates (at most `size - 1` characters).
void CopyText(char* dst, const char* src, size_t size)
{
    dst[0] = '\0';
    if (src) {
        std::strncpy(dst, src, size - 1);
        dst[size - 1] = '\0';
    }
}

// Deep copy of a parsed script list: each script keeps its bindings, its
// name and whether it is a sequence / restarts; the run state starts clear.
GoalScriptList* CopyScriptList(const GoalScriptList& source)
{
    auto* list = new GoalScriptList();
    for (uint32_t i = 0; i < source.Count(); ++i) {
        const GoalScript* from = (*source.scripts)[i];
        auto* script = new GoalScript();
        script->bindings.Init(4, nullptr, false);
        for (const GoalScriptBinding& binding : from->bindings)
            script->bindings.PushBack(binding);
        script->name = from->name;
        script->flags = from->flags & (kScript_Restart | kScript_IsSequence);
        if (!list->scripts)
            list->scripts = new ape::ApeTArray<GoalScript*>();
        list->scripts->PushBack(script);
    }
    return list;
}

} // namespace

// @func 0x00769BD8 Goal::Goal
Goal::Goal(const GoalDesc* desc)
{
    game = desc->game;
    job = desc->job;
    repeatCount = (uint8_t)desc->repeatCount;
    repeatProgress = 0;
    status = kGoal_None;
    flags = desc->flags;
    initialFlags = desc->flags;
    targetName = desc->targetName;
    field_8A0 = desc->field_20;
    progressName = desc->progressName;
    progressBase = desc->progressBase;
    hintScriptName = desc->hintScriptName;
    hintScript = nullptr;
    field_8B4 = desc->field_30;
    field_8B5 = desc->field_31;
    completesParent = desc->completesParent;
    showProgressTotal = desc->showProgressTotal;
    for (GoalScriptCursor& cursor : scripts)
        cursor = GoalScriptCursor();

    field_960 = desc->field_DC;
    field_964 = desc->field_E0;
    field_968 = desc->field_E4;
    field_96C = desc->field_E8;
    index = desc->index;
    showDescription = desc->showDescription;
    field_9B0 = desc->field_130;
    field_9B4 = desc->field_134;

    for (int i = 0; i < 4; ++i) {
        const GoalBlob& from = desc->blobs[i];
        GoalBlob& to = blobs[i];
        delete[] to.data;
        to.extra = from.extra;
        to.size = from.size;
        to.data = new uint8_t[from.size];
        if (from.size)
            std::memcpy(to.data, from.data, from.size);
    }

    cycleIndex = 0;
    countsForGroup = true;
    failRestarts = desc->failRestarts;
    targetFlag = desc->targetFlag;
    nameHash = ape::HashNameNoCase(desc->name);
    nextGoal = nullptr;
    parentGoal = nullptr;

    for (int slot = 0; slot < kScriptSlotCount; ++slot) {
        const GoalScriptList* source = desc->scripts[slot].list;
        if (source && source->Count())
            scripts[slot].list = CopyScriptList(*source);
    }

    hints = nullptr;
    if (desc->hints && desc->hints->Count()) {
        for (const GoalHint& hint : *desc->hints) {
            if (!hints)
                hints = new ape::ApeTArray<GoalHint>();
            hints->PushBack(hint);
        }
    }

    std::memcpy(field_8F4, desc->field_70, sizeof(field_8F4));
    std::memcpy(checkpoints, desc->checkpoints, sizeof(checkpoints));
    CopyText(name, desc->name, sizeof(name));
    CopyText(description, desc->description, sizeof(description));
    CopyText(field_49C, desc->text2, sizeof(field_49C));
    std::memcpy(field_8E4, desc->field_60, sizeof(field_8E4));

    // Goals are numbered in creation order within their job.
    saveSlot = JobTakeGoalSaveSlot(job);

    cycle = nullptr;
    cycleCount = desc->cycleCount;
    if (cycleCount) {
        cycle = new GoalCycleEntry[(size_t)cycleCount];
        for (int i = 0; i < cycleCount; ++i) {
            cycle[i].duration = desc->cycle[i].duration;
            cycle[i].SetText(desc->cycle[i].text);
        }
    }
    CopyText(field_99D, desc->field_120, sizeof(field_99D));
}

// NOTE: the console goal derives from the event-listener base (vtable
// 0x00B21A30) whose destructor 0x0048F6B0 leaves every listener group the
// object joined; GoalStopListening stands in for it.
// @func 0x00769480 Goal::~Goal
// @func 0x00769744 Goal::~Goal (deleting)
Goal::~Goal()
{
    delete[] cycle;
    cycle = nullptr;
    for (GoalScriptCursor& cursor : scripts) {
        GoalScriptList* list = cursor.list;
        if (!list)
            continue;
        if (list->scripts) {
            for (GoalScript* script : *list->scripts)
                delete script;
            delete list->scripts;
        }
        delete list;
        cursor.list = nullptr;
    }
    for (int i = 3; i >= 0; --i) {
        delete[] blobs[i].data;
        blobs[i].data = nullptr;
    }
    delete hints;
    hints = nullptr;
    GoalStopListening(this);
}

// @func 0x00321BE4 Goal::HandleMessage
void Goal::HandleMessage(const GoalMessage& msg)
{
    // Only one message is understood: it is replayed as game event 0x17 for
    // the player that sent it.
    if (msg.id != kMsg_GoalEvent)
        return;
    GameEvent e;
    WorldObject* sender = static_cast<WorldObject*>(msg.sender);
    InitEvent17(e, sender, msg.arg, 0, 0, 0);
    if (e.type == 0x21)   // NOTE: kept from the console; InitEvent17 never sets it
        return;
    Update(ObjectPlayerIndex(sender), e);
}

// @func 0x003252D8 Goal::Update
uint8_t Goal::Update(int player, const GameEvent& e)
{
    (void)player;
    switch (status) {
    case kGoal_Active:
        RunScriptSlot(kScripts_Active);
        if (e.type == kEvent_09) {
            ResetCycle();
        } else if (e.type == kEvent_0A && cycle) {
            // Cycling texts: fade in, hold, fade out over the last
            // kCycleFadeTime seconds, then move to the next entry.
            const float timer = cycleTimer;
            const float dt = e.arg0Float;
            if (timer > kCycleFadeTime || timer <= 0.0f) {
                if (cycleFade < 1.0f) {
                    cycleFade += dt / kCycleFadeTime;
                    if (cycleFade > 1.0f)
                        cycleFade = 1.0f;
                }
            } else {
                cycleFade = timer / kCycleFadeTime;
            }
            if (timer > 0.0f) {
                cycleTimer = timer - dt;
                if (cycleTimer <= 0.0f) {
                    if (++cycleIndex >= cycleCount)
                        cycleIndex = 0;
                    cycleFade = 0.0f;
                    cycleTimer = cycle[cycleIndex].duration;
                }
            }
        }
        break;
    case kGoal_Complete:
        RunScriptSlot(kScripts_Complete);
        break;
    case kGoal_Failed:
        RunScriptSlot(kScripts_Failed);
        break;
    default:
        break;
    }
    return status;
}

// @func 0x0032549C Goal::Reset
void Goal::Reset(bool fromSave, int requested)
{
    // Rewind every script list. A script that already ran and is marked
    // "restart" counts as done; anything else runs again.
    for (GoalScriptCursor& cursor : scripts) {
        bool anyPending = false;
        const uint32_t count = ScriptCount(cursor);
        for (uint32_t i = 0; i < count; ++i) {
            GoalScript* script = ScriptAt(cursor, i);
            script->instance = nullptr;
            const bool keep = (script->flags & kScript_Started) && (script->flags & kScript_Restart);
            script->flags &= ~(kScript_Done | kScript_Prepared | kScript_Running | kScript_Started);
            if (keep)
                script->flags |= kScript_Started | kScript_Done;
            else
                anyPending = true;
        }
        cursor.next = uint8_t((!anyPending && cursor.list) ? count : 0);
        cursor.finished = !anyPending;
    }

    bool force = fromSave;
    bool savedComplete = false;
    if (fromSave) {
        // A saved status past a lock state removes that lock.
        const uint32_t saved = JobGoalSaveWord(job, saveSlot) >> 28;
        if (saved > 1)
            flags &= ~kGoalFlag_Lock1;
        if (saved > 2)
            flags &= ~kGoalFlag_Lock2;
        if (saved > 3)
            flags &= ~kGoalFlag_Lock3;
        savedComplete = saved == kGoal_Complete;
    }

    int newStatus;
    if (requested == -1) {
        if (savedComplete) {
            newStatus = kGoal_Complete;
            force = true;
            repeatProgress = repeatCount;
        } else {
            repeatProgress = 0;
            newStatus = kGoal_Active;
            if (flags & kGoalFlag_Lock3)
                newStatus = kGoal_Locked3;
            if (flags & kGoalFlag_Lock2)
                newStatus = kGoal_Locked2;
            if (flags & kGoalFlag_Lock1)
                newStatus = kGoal_Locked1;
        }
    } else {
        repeatProgress = requested == kGoal_Complete ? repeatCount : 0;
        if (requested == kGoal_Complete || !(flags & kGoalFlag_KeepComplete) ||
            status != kGoal_Complete) {
            force = requested == kGoal_Complete || fromSave;
            newStatus = requested;
        } else {
            newStatus = kGoal_None;
        }
    }

    if (hints && hints->Count() != 0) {
        const bool live = IsGoalInLiveJob(this);
        for (GoalHint& hint : *hints) {
            hint.force = kHintForce_None;
            if ((hint.applied || newStatus == kGoal_Complete) && hint.persistent)
                hint.force = hint.invert ? kHintForce_Off : kHintForce_On;
            hint.applied = false;
            hint.missing = false;
            hint.object = nullptr;
            if (live)
                hint.Resolve(false);
        }
    }

    if (newStatus != kGoal_None)
        SetStatus(uint8_t(newStatus), force);

    if (status == kGoal_Complete) {
        // A completed goal does not replay the scripts it already ran.
        auto markDone = [](GoalScriptCursor& cursor) {
            cursor.next = uint8_t(ScriptCount(cursor));
            cursor.finished = true;
        };
        if (!force) {
            markDone(scripts[kScripts_CompleteIdle]);
            markDone(scripts[kScripts_ActiveIdle]);
        } else {
            markDone(scripts[kScripts_Active]);
            markDone(scripts[kScripts_Complete]);
            markDone(scripts[kScripts_Failed]);
            JobSetGoalSaveWord(job, saveSlot, JobGoalSaveWord(job, saveSlot) & ~0x04000000u);
        }
    }
}

// @func 0x00325A30 Goal::ResetHints
void Goal::ResetHints(bool stopHintScript)
{
    if (hints) {
        for (GoalHint& hint : *hints) {
            hint.force = kHintForce_None;
            if ((hint.applied || status == kGoal_Complete) && hint.persistent)
                hint.force = hint.invert ? kHintForce_Off : kHintForce_On;
            hint.applied = false;
            hint.missing = false;
            hint.object = nullptr;
        }
    }
    if (stopHintScript && hintScript) {
        hintScript->SetState(kHintState_End);
        hintScript->SetRemoveWhenFinished(true);
        hintScript = nullptr;
    }
}

// @func 0x00325B58 Goal::PrepareScriptSlot
void Goal::PrepareScriptSlot(int slot)
{
    if (slot == -1)
        return;
    const GoalScriptCursor& cursor = scripts[slot];
    for (uint32_t i = 0; i < ScriptCount(cursor); ++i) {
        GoalScript* script = ScriptAt(cursor, i);
        if (script->name != 0 && !(script->flags & kScript_Prepared)) {
            SequenceStreamRequest(script->name);
            script->flags |= kScript_Prepared;
        }
    }
}

// @func 0x003265A4 Goal::ScriptSlotStreamed
bool Goal::ScriptSlotStreamed(int slot) const
{
    if (slot == -1)
        return true;
    const GoalScriptCursor& cursor = scripts[slot];
    for (uint32_t i = 0; i < ScriptCount(cursor); ++i) {
        const GoalScript* script = ScriptAt(cursor, i);
        if (script->name == 0 || !(script->flags & kScript_Prepared))
            continue;
        const game::StreamGroup* group = game::FindStreamGroup(script->name);
        if (group && (group->flags & game::StreamGroup::kGroup_Running) && !group->Complete())
            return false;
    }
    return true;
}

// @func 0x00325FCC Goal::ReleaseScriptSlot
void Goal::ReleaseScriptSlot(int slot)
{
    if (slot == -1)
        return;
    const GoalScriptCursor& cursor = scripts[slot];
    for (uint32_t i = 0; i < ScriptCount(cursor); ++i) {
        GoalScript* script = ScriptAt(cursor, i);
        if (script->name != 0 && (script->flags & kScript_Prepared)) {
            if (SequenceStreamRelease(script->name))
                GameMarkStreamsChanged(game);
            script->flags &= ~kScript_Prepared;
        }
    }
}

// @func 0x00325C58 Goal::PrepareScripts
void Goal::PrepareScripts()
{
    if (!IsGoalInLiveJob(this))
        return;
    if (status == kGoal_Complete) {
        PrepareScriptSlot(kScripts_CompleteIdle);
        RunScriptSlot(kScripts_CompleteIdle);
    } else if (status == kGoal_Active) {
        PrepareScriptSlot(kScripts_ActiveIdle);
        RunScriptSlot(kScripts_ActiveIdle);
    }
}

// @func 0x00325CE4 Goal::ReapplyHints
void Goal::ReapplyHints(Goal* owner)
{
    if (status != kGoal_Active || !IsGoalInLiveJob(this) || !hints)
        return;
    // Retry the hints whose target was missing (it may have spawned since).
    for (GoalHint& hint : *hints) {
        if (!hint.missing)
            continue;
        hint.applied = false;
        hint.Resolve(true);
        hint.Apply(owner, true);
    }
}

// @func 0x00325DD0 Goal::GetTarget
bool Goal::GetTarget(GoalTarget* out)
{
    if (targetName != 0) {
        out->name = targetName;
        out->flag = targetFlag;
    }
    return targetName != 0;
}

// @func 0x00325DF8 Goal::CurrentCycleText
const char* Goal::CurrentCycleText(float* fade)
{
    if (!cycle || cycleIndex >= cycleCount)
        return nullptr;
    if (fade)
        *fade = cycleFade;
    return cycle[cycleIndex].text;
}

// @func 0x00325E4C Goal::ResetCycle
void Goal::ResetCycle()
{
    cycleIndex = 0;
    cycleFade = 0.0f;
    if (cycle)
        cycleTimer = cycle[0].duration;
}

// @func 0x00325E70 Goal::CheckpointIndex
int Goal::CheckpointIndex(uint32_t checkpointName) const
{
    if (checkpointName == 0)
        return -1;
    for (int i = 0; i < 9; ++i)
        if (checkpoints[i] == checkpointName)
            return i;
    return -1;
}

// @func 0x00325EBC Goal::CheckpointIndexOf
int Goal::CheckpointIndexOf(const void* entity) const
{
    if (!entity)
        return -1;
    const uint32_t group = EntityGroupName(entity);
    for (int i = 0; i < 9; ++i) {
        const uint32_t cp = checkpoints[i];
        if (cp == group)
            return i;
        // "Sly" matches the Sly character, or a character whose group is Sly's.
        if (cp == kCheckpoint_Sly &&
            (EntityIsSlyClass(entity) ||
             (EntityIsCharacter(entity) && EntityGroupField_C0(entity) == kCheckpoint_Sly)))
            return i;
    }
    return -1;
}

// @func 0x00325F7C Goal::FailScriptsDone
bool Goal::FailScriptsDone() const
{
    const GoalScriptCursor& cursor = scripts[kScripts_Failed];
    if (ScriptCount(cursor) == 0)
        return true;
    return cursor.finished;
}

// @func 0x00324B08 Goal::LaunchScript
ape::ApeAnimScriptInstance* Goal::LaunchScript(uint32_t scriptName, uint32_t library,
                                               GoalScript& script, bool asSequence, int slot)
{
    if (scriptName == 0)
        return nullptr;

    size_t jobBindingCount = 0;
    const GoalScriptBinding* jobBindings = JobManagerJobBindings(&jobBindingCount);

    if (asSequence) {
        // The completion sequence of the job's last goal (or of the tracked
        // goal when the following one is not live) is flagged for the game.
        bool lastAndComplete = false;
        Goal* self = JobGoalAt(job, uint32_t(index));
        if (uint32_t(index) == JobGoalCount(job) - 1 && self && self->status == kGoal_Complete)
            lastAndComplete = true;
        bool flag = false;
        if (slot == kScripts_Complete) {
            if (lastAndComplete) {
                flag = true;
            } else if (JobManagerTrackedGoal() == self) {
                Goal* next = JobManagerGoalAfterTracked();
                if (next && !IsGoalInLiveJob(next))
                    flag = true;
            }
        }
        if (status == kGoal_Failed)
            JobManagerBeforeFailSequence();

        std::vector<GoalScriptBinding> bindings(script.bindings.begin(), script.bindings.end());
        if (jobBindings)
            bindings.insert(bindings.end(), jobBindings, jobBindings + jobBindingCount);
        GameQueueSequence(game, scriptName, library, bindings.empty() ? nullptr : bindings.data(),
                          bindings.size(), flag);
        return nullptr;
    }

    ape::ApeAnimScript* animScript = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, library, true);
    if (!animScript)
        return nullptr;
    auto* instance = new ape::ApeAnimScriptInstance(animScript);
    BindScriptVariables(instance, script.bindings.Data(), script.bindings.Count());
    if (jobBindings)
        BindScriptVariables(instance, jobBindings, jobBindingCount);
    instance->SetRemoveWhenFinished(true);
    ape::ApeAnimScriptInstance::Register(instance);
    return instance;
}

// @func 0x00324F58 Goal::StartScript
void Goal::StartScript(GoalScript& script, int slot)
{
    const uint32_t old = script.flags;
    script.flags |= kScript_Started;
    if (!(old & kScript_Prepared)) {
        SequenceStreamRequest(script.name);
        script.flags |= kScript_Prepared;
    }
    if (script.flags & kScript_Prepared)
        SequenceStreamLoadNow(script.name);
    script.flags &= ~kScript_Running;
    script.instance = nullptr;

    const bool isSequence = (script.flags & kScript_IsSequence) != 0;
    ape::ApeAnimScriptInstance* instance =
        LaunchScript(script.name, kGoalScriptLibrary, script, isSequence, slot);
    if (instance) {
        if (!isSequence) {
            script.instance = instance;
            instance->SetRemoveWhenFinished(false);
        }
        instance->Update(kFirstUpdateStep);
        script.flags |= kScript_Running;
    }
    if (!(script.flags & kScript_Running) && GameIsSequenceQueued(game, script.name))
        script.flags |= kScript_Running;
}

// @func 0x003250A0 Goal::RunScriptSlot
void Goal::RunScriptSlot(int slot)
{
    if (slot == -1)
        return;
    GoalScriptCursor& cursor = scripts[slot];
    if (cursor.finished || ScriptCount(cursor) == 0)
        return;

    // Scripts of a slot run one after the other: start the next one, wait
    // until it has finished, then move on.
    while (cursor.next < ScriptCount(cursor)) {
        GoalScript* script = ScriptAt(cursor, cursor.next);
        if (!(script->flags & kScript_Started))
            StartScript(*script, slot);
        if (!(script->flags & kScript_Done)) {
            if (!(script->flags & kScript_Started))
                return;
            const bool playing = (script->flags & kScript_Running) &&
                                 (!(script->flags & kScript_IsSequence) ||
                                  GameIsSequenceQueued(game, script->name));
            if (playing) {
                if (!script->instance || !script->instance->IsFinished(false))
                    return;
                if (gameplay::IsSequenceRunning(script->name))
                    return;
            }
        }
        if (script->instance) {
            script->instance->SetRemoveWhenFinished(true);
            script->instance = nullptr;
        }
        script->flags |= kScript_Done;
        ++cursor.next;
        cursor.finished = cursor.next >= ScriptCount(cursor);
        if ((script->flags & kScript_Prepared) && SequenceStreamRelease(script->name))
            GameMarkStreamsChanged(game);
    }
}

// @func 0x003260F0 Goal::SetStatus
void Goal::SetStatus(uint8_t newStatus, bool force)
{
    if (flags & kGoalFlag_StatusFrozen)
        return;
    if (newStatus == kGoal_Failed)
        JobManagerGoalFailed(this, JobOwnerFlag(job));

    if (force || status != newStatus) {
        status = newStatus;
        uint32_t word = JobGoalSaveWord(job, saveSlot);
        word = (word & 0x0FFFFFFFu) | (uint32_t(newStatus) << 28) | 0x04000000u;
        JobSetGoalSaveWord(job, saveSlot, word);
    }

    const bool live = IsGoalInLiveJob(this);
    if (newStatus == kGoal_Active) {
        repeatProgress = 0;
        // Stream in the scripts the next goal of the job will need.
        for (uint32_t i = 0; i < JobGoalCount(job); ++i) {
            if (JobGoalAt(job, i) != this)
                continue;
            if (i + 1 < JobGoalCount(job)) {
                Goal* next = JobGoalAt(job, i + 1);
                next->PrepareScriptSlot(kScripts_Active);
                if (!next->BlocksNextScripts()) {
                    next->PrepareScriptSlot(kScripts_Complete);
                    next->PrepareScriptSlot(kScripts_Failed);
                }
            }
            break;
        }
        if (nextGoal)
            nextGoal->PrepareScriptSlot(kScripts_Active);
        PrepareScriptSlot(kScripts_Complete);
        PrepareScriptSlot(kScripts_Failed);

        if (!hintScript) {
            ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kGoalScriptLibrary);
            ape::ApeAnimScript* script = library ? library->FindScript(hintScriptName) : nullptr;
            if (script) {
                hintScript = new ape::ApeAnimScriptInstance(script);
                hintScript->SetRemoveWhenFinished(false);
                ape::ApeAnimScriptInstance::Register(hintScript);
                hintScript->Update(kFirstUpdateStep);
            }
        } else {
            hintScript->SetStateAndUpdate(kHintState_Activate);
        }
        if (hints && hints->Count() != 0 && live)
            for (GoalHint& hint : *hints)
                hint.Apply(this, true);
        return;
    }

    if (newStatus == kGoal_Complete) {
        if (hints && hints->Count() != 0 && live)
            for (GoalHint& hint : *hints)
                hint.Apply(this, false);
        ReleaseScriptSlot(kScripts_Failed);
        if (hintScript)
            hintScript->SetStateAndUpdate(kHintState_Complete);
    }
}

// ---------------------------------------------------------------------------
// Hints

// @func 0x00321324 GoalHint::Resolve
void GoalHint::Resolve(bool report)
{
    (void)report;   // the console only formats a (compiled out) warning
    missing = false;
    if (object)
        return;

    switch (kind) {
    case kHint_Entity: {
        void* entity = HintFindEntity(target);
        if (entity && HintEntityTargetable(entity))
            object = entity;
        if (entity && HintEntityNeedsRefresh(entity))
            HintEntityRefresh(entity);
        break;
    }
    case kHint_Interaction: {
        void* entity = HintFindEntity(target);
        if (entity)
            object = HintEntityInteraction(entity);
        break;
    }
    case kHint_Spawner:
        object = g_spawnManager ? g_spawnManager->FindSpawner(target) : nullptr;
        break;
    case kHint_SpawnGroup:
        object = g_spawnManager ? g_spawnManager->FindGroup(target) : nullptr;
        break;
    case kHint_LaserGroup:
        object = target ? HintFindLaserGroup(target) : nullptr;
        break;
    case kHint_Laser:
        object = target ? HintFindLaser(target) : nullptr;
        break;
    case kHint_LaserField:
        object = target ? HintFindLaserField(target) : nullptr;
        break;
    case kHint_Interactable: {
        void* entity = HintFindEntity(target);
        if (entity && HintEntityInteractable(entity))
            object = entity;
        break;
    }
    default:
        return;
    }
    if (!object) {
        // Not there (yet): ReapplyHints() retries it later.
        applied = true;
        missing = true;
    }
}

// @func 0x003218B0 GoalHint::Apply
void GoalHint::Apply(Goal* owner, bool enable)
{
    if (owner && owner->status != kGoal_Active && owner->status != kGoal_Complete)
        return;
    if (applied && persistent)
        return;

    switch (force) {
    case kHintForce_None:
        if (invert)
            enable = !enable;
        break;
    case kHintForce_On:
        enable = true;
        break;
    case kHintForce_Off:
        enable = false;
        break;
    default:
        break;
    }
    if (!object)
        return;

    switch (kind) {
    case kHint_Entity:
        HintEntitySetEnabled(object, enable);
        break;
    case kHint_Interaction: {
        // States 3 and 4 are "disabled"; only switch when it changes anything.
        const int state = HintInteractionState(object);
        const bool disabled = state == 4 || state == 3;
        if (disabled != enable)
            return;
        HintInteractionSetState(object, enable ? 0 : 4);
        break;
    }
    case kHint_Spawner: {
        Spawner* spawner = static_cast<Spawner*>(object);
        if (enable) {
            spawner->Activate();
            spawner->Update(0.0f);
        } else {
            spawner->Deactivate(keepAlive);
        }
        break;
    }
    case kHint_SpawnGroup: {
        SpawnGroup* group = static_cast<SpawnGroup*>(object);
        if (enable) {
            group->Activate(false);
            group->SpawnInitial();
        } else {
            group->Deactivate(keepAlive);
        }
        break;
    }
    case kHint_LaserGroup:
        HintLaserGroupEnable(object, enable);
        break;
    case kHint_Laser:
        HintLaserEnable(object, enable);
        break;
    case kHint_LaserField:
        HintLaserFieldEnable(object, enable);
        break;
    case kHint_Interactable:
        HintInteractableSetEnabled(object, enable);
        break;
    default:
        return;
    }
    applied = true;
}

} // namespace goals
} // namespace sly
