// openTIT - Job manager lookups and progress accessors.
#include "sly/game/goals/JobManager.h"

#include "sly/entity/Awards.h"
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/GameScripts.h"
#include "sly/game/SlyGame.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "sly/game/goals/Goal.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/game/hud/HudManager.h"

namespace sly {
namespace game {

namespace {
PlayerProgress g_progressStandIn;
constexpr uint32_t kNoIndex = 0xFFFFFFFFu;
} // namespace

PlayerProgress& CurrentProgress() { return g_progressStandIn; }
PlayerProgress& ProfileProgress(uint8_t) { return g_progressStandIn; }
uint32_t PlayerGoalCount() { return g_game ? g_game->goals.Count() : 0; }
Goal* PlayerGoalListAt(uint32_t index) { return index < PlayerGoalCount() ? g_game->goals[index] : nullptr; }
uint32_t LiveJobId() { return 0; }
int GameWorldState(int) { return 0; }
void PlayerResetJobState(int) {}
// TODO(0x00488D68): the game world state machine is not ported.
void GameWorldRestartState() {}
// TODO(game world +0x1070 / +0x1280F0 / +0x128160 / +0x1281E0 / +0x128114,
// 0x00486460, 0x0012BF70, 0x00487670, 0x00484BE0, 0x00488178): the players
// block, the playable characters and the level instances are not modelled.
int PlayerCharacterSlot() { return 0; }
entity::Entity* PlayerEntity() { return nullptr; }
ape::Matrix34& PlayerSpawnTransform()
{
    static ape::Matrix34 transform{};
    return transform;
}
entity::Entity* PlayableCharacter(int) { return nullptr; }
void DefaultSpawnTransform(int, ape::Matrix34*) {}
bool PointInstanceWorldTransform(uint32_t, ape::Matrix34*) { return false; }
bool TakeRespawnOverride(ape::Matrix34*, bool* wasSet)
{
    *wasSet = false;
    return false;
}
void CharacterSnapToGround(entity::Entity*) {}
void GameCameraReset() {}
void GameWorldUseAssociatedPlayer(Goal*) {}
void GameWorldSwitchPlayerCharacter(uint32_t) {}
void GameWorldRespawnPlayer(const ape::Matrix34&) {}
uint32_t RunningLevel() { return 0; }
uint8_t PlayerBlockProfile(int) { return 0; }
entity::Entity* PlayerEntityOf(int) { return nullptr; }
// TODO(0x0029A40C / 0x0029F26C / 0x00276558): gadget, targeting and anim
// driver components live in sly/components.
bool GadgetAllowsStop(entity::EntityComponent*) { return true; }
void TargetingClearTargets(entity::EntityComponent*) {}
void AnimDriverStopScript(entity::EntityComponent*) {}
uint8_t& ProfilePendingCount(uint8_t profile)
{
    static uint8_t counts[8] = {};
    return counts[profile & 7];
}
uint8_t* ProfileNotifyPair(uint8_t profile)
{
    static uint8_t pairs[8][2] = {};
    return pairs[profile & 7];
}
void GameWorldReloadLevel() { sly::RequestLevelRestart(); }
void GameWorldSetState(int, int) {}
void GameWorldSetStateData(const char*, uint32_t) {}
bool CurrentLevelJobValue(uint32_t*) { return false; }
void TargetingSetEnabled(void*, bool) {}

// @func 0x002E1220 JobManager::FindEpisode
Episode* JobManager::FindEpisode(uint32_t nameHash) const
{
    if (nameHash == 0)
        return nullptr;
    for (uint32_t i = 0; i < episodeCount; ++i) {
        if (episodes[i].nameHash == nameHash)
            return &episodes[i];
    }
    return nullptr;
}

// @func 0x002E1278 JobManager::EpisodeIndex
uint32_t JobManager::EpisodeIndex(uint32_t nameHash) const
{
    for (uint32_t i = 0; i < episodeCount; ++i) {
        if (episodes[i].nameHash == nameHash)
            return i;
    }
    return kNoIndex;
}

// @func 0x002E11D8 JobManager::NotifyTrackedGoalLoaded
void JobManager::NotifyTrackedGoalLoaded()
{
    if (trackedGoal)
        trackedGoal->OnLevelLoaded();
}

// @func 0x002E207C JobManager::TrackedGoalDescription
const char* JobManager::TrackedGoalDescription() const
{
    return trackedGoal ? trackedGoal->GetDescription() : nullptr;
}

// @func 0x002E2214 JobManager::GoalAfterTracked
Goal* JobManager::GoalAfterTracked() const
{
    if (!trackedGoal)
        return nullptr;
    const uint32_t count = PlayerGoalCount();
    for (uint32_t i = 0; i < count; ++i) {
        if (PlayerGoalListAt(i) == trackedGoal)
            return i + 1 < count ? PlayerGoalListAt(i + 1) : nullptr;
    }
    return nullptr;
}

// @func 0x002E23AC JobManager::LastStartedJob
Job* JobManager::LastStartedJob() const
{
    const PlayerProgress& progress = CurrentProgress();
    for (uint32_t i = jobCount; i > 0; --i) {
        Job& job = jobs[i - 1];
        if (job.active && ((progress.episodes[job.episode].flags >> 3) & 3) != 0)
            return &job;
    }
    return nullptr;
}

// @func 0x002E3130 JobManager::HasJob
bool JobManager::HasJob(uint32_t nameHash) const
{
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].nameHash == nameHash)
            return true;
    }
    return false;
}

// @func 0x002E5034 JobManager::TrackedGoalHasFlag40
bool JobManager::TrackedGoalHasFlag40() const
{
    return trackedGoal && (trackedGoal->flags & 0x40u);
}

// @func 0x002E6DC4 JobManager::SetInJob
void JobManager::SetInJob(bool inJob)
{
    PlayerProgress& progress = CurrentProgress();
    progress.inJob = inJob ? 1 : 0;
    progress.currentJobValue = (inJob && currentJob) ? currentJob->field_2D4 : 0;
}

// @func 0x002E2104 JobManager::IsGoalLive
bool JobManager::IsGoalLive(const Goal* goal) const
{
    if (!goal || goal->index < 0 || static_cast<uint32_t>(goal->index) >= PlayerGoalCount())
        return false;
    Goal* listed = PlayerGoalListAt(static_cast<uint32_t>(goal->index));
    if (!listed || listed->Field960() == 0)
        return false;
    const uint32_t live = LiveJobId();
    return live != 0 && live == listed->Field960();
}

// @func 0x002E26A8 JobManager::SetCurrentEpisodeFlag
void JobManager::SetCurrentEpisodeFlag(bool on)
{
    if (!currentJob)
        return;
    PlayerProgress& progress = CurrentProgress();
    uint16_t& flags = progress.episodes[currentJob->episode].flags;
    flags = on ? static_cast<uint16_t>(flags | 0x20u) : static_cast<uint16_t>(flags & ~0x20u);
    progress.field_A0 = currentJob->field_32C;
}

// @func 0x002E2A98 JobManager::CountFlaggedJobs
int JobManager::CountFlaggedJobs() const
{
    const PlayerProgress& progress = CurrentProgress();
    int count = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if ((progress.episodes[jobs[i].episode].flags & 0x20u) && jobs[i].field_32C != 0)
            ++count;
    }
    return count;
}

// @func 0x002E6270 JobManager::CountJobBits
int JobManager::CountJobBits() const
{
    const PlayerProgress& progress = CurrentProgress();
    int count = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].bitGroup == 0xFF)
            continue;
        for (uint32_t bits = progress.jobBits[jobs[i].bitGroup]; bits != 0; bits &= bits - 1)
            ++count;
    }
    return count;
}

// @func 0x002E619C JobManager::CheckEpisodeTrophies
void JobManager::CheckEpisodeTrophies() const
{
    constexpr uint32_t kTrophy_FirstEpisode = 0xC2900B80u;   // NOTE: award names unknown
    constexpr uint32_t kTrophy_SixEpisodes = 0x368BAA71u;
    if (jobCount == 0)
        return;
    const PlayerProgress& progress = CurrentProgress();
    uint32_t finished = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].episode != 0xFF && (progress.episodes[jobs[i].episode].flags & 0x40u))
            ++finished;
    }
    if (finished == 1)
        AwardTrophyByName(kTrophy_FirstEpisode);
    else if (finished > 5)
        AwardTrophyByName(kTrophy_SixEpisodes);
}

// @func 0x002E2E78 JobManager::FlushPendingRequest
// @func 0x002E2FD4 JobManager::FlushPendingRequest (copy 2)
void JobManager::FlushPendingRequest()
{
    constexpr uint32_t kPending = 1u << 29;
    if (!(flags & kPending))
        return;
    if (GameWorldState(0) == 1 && GameWorldState(1) != 10) {
        if (GameWorldState(1) != 5) {
            GameWorldSetState(1, 10);
            GameWorldSetState(2, 0);
        }
        GameWorldSetStateData(failMessage, failPlayer);
    }
    failMessage = nullptr;
    failPlayer = 0;
    flags &= ~kPending;
}

void JobManager::ResetRunningJob()
{
    constexpr uint32_t kSharedScriptLibrary = 0x7719A548u;
    ClearGoals();
    if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kSharedScriptLibrary))
        library->Clear();
    instance = nullptr;
    trackedGoal = nullptr;
    field_30 = 0;
    const int players = g_game ? g_game->localPlayerCount : 0;
    for (int i = 0; i < players; ++i)
        PlayerResetJobState(i);
}

// @func 0x002E0860 JobManager::EndJob
void JobManager::EndJob(bool keepCurrent)
{
    ResetRunningJob();
    if (!keepCurrent)
        currentJob = nullptr;
}

// @func 0x002E22A0 JobManager::SelectJobByValue
void JobManager::SelectJobByValue(uint32_t value)
{
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].field_2D4 == value) {
            ResetRunningJob();
            currentJob = &jobs[i];
            return;
        }
    }
}

// @func 0x002E2D7C JobManager::CollectJobs
void JobManager::CollectJobs(uint32_t nameHash, ape::ApeTArray<Job*>& out) const
{
    if (nameHash == 0)
        return;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].nameHash == nameHash)
            out.PushBack(&jobs[i]);
    }
}

// @func 0x002E5060 JobManager::FinishJobInstance
void JobManager::FinishJobInstance()
{
    if (!instance)
        return;
    uint16_t& episodeFlags = CurrentProgress().episodes[currentJob->episode].flags;
    if (!(episodeFlags & 0x20u)) {
        // NOTE: the flag is only ever set here (the clearing branch leaves it 0).
        if (instance->field_1F7 == 0)
            episodeFlags = static_cast<uint16_t>(episodeFlags | 0x20u);
    }
    ResetRunningJob();
}

// @func 0x002E2740 JobManager::IsJobValueFlagged
bool JobManager::IsJobValueFlagged(uint32_t value) const
{
    const PlayerProgress& progress = CurrentProgress();
    if (value == 0)
        return (progress.episodes[currentJob->episode].flags & 0x20u) != 0;
    uint32_t levelValue = 0;
    if (CurrentLevelJobValue(&levelValue) && levelValue == value)
        return true;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].field_32C == value)
            return (progress.episodes[jobs[i].episode].flags & 0x20u) != 0;
    }
    return false;
}

// @func 0x002E6FC0 CollectUnlockables
void CollectUnlockables(uint32_t group, ape::ApeTArray<Unlockable*>& out)
{
    const uint32_t count = UnlockableCount();
    for (uint32_t i = 0; i < count; ++i) {
        if (UnlockableGroup(i) == group)
            out.PushBack(UnlockableAt(i));
    }
}

// @func 0x002E2874 UnlockByKey
void UnlockByKey(uint32_t key)
{
    const uint32_t count = UnlockableCount();
    for (uint32_t i = 0; i < count; ++i) {
        if (UnlockableKey(i) == key) {
            CurrentProgress().unlockBits[i >> 5] |= 1u << (i & 31);
            return;
        }
    }
    // The console formats the key for a (compiled out) warning.
    (void)ape::TempHexString(key);
}

// @func 0x002E4A0C SetPlayersTargeting
void SetPlayersTargeting(bool enabled)
{
    const int players = g_game ? g_game->localPlayerCount : 0;
    for (int i = 0; i < players; ++i) {
        entity::Entity* player = entity::PlayerEntity(i);
        if (!player)
            continue;
        if (entity::EntityComponent* targeting = entity::EntityTargeting(player))
            TargetingSetEnabled(targeting, enabled);
    }
}

// @func 0x002E2950 IsUnlocked
bool IsUnlocked(uint32_t bit)
{
    if (bit >= 40)
        return false;
    return (CurrentProgress().unlockBits[bit >> 5] & (1u << (bit & 31))) != 0;
}

// @func 0x002E3A70 TrophyCompletion
float TrophyCompletion()
{
    const int total = TotalTrophyPoints();
    if (total < 1)
        return 0.0f;
    return static_cast<float>(EarnedTrophyPoints()) / static_cast<float>(total);
}

// @func 0x002E5394 IsInJob
bool IsInJob()
{
    return CurrentProgress().inJob == 1;
}

// @func 0x002E53D4 ProgressFlag149
bool ProgressFlag149()
{
    return CurrentProgress().field_149 == 1;
}

// @func 0x002E6D30 SetProgressFlag149
void SetProgressFlag149(bool on)
{
    CurrentProgress().field_149 = on ? 1 : 0;
}

// @func 0x002E6D6C SetProgressField14C
void SetProgressField14C(uint32_t value)
{
    CurrentProgress().field_14C = value;
}

// @func 0x002E6D98 ProgressField14C
uint32_t ProgressField14C()
{
    return CurrentProgress().field_14C;
}

// @func 0x002E6E28 CurrentJobValue
uint32_t CurrentJobValue()
{
    return CurrentProgress().currentJobValue;
}

// @func 0x002E6A3C PlayerGoalAt
Goal* PlayerGoalAt(uint32_t index)
{
    if (index == kNoIndex || index >= PlayerGoalCount())
        return nullptr;
    return PlayerGoalListAt(index);
}

// @func 0x002DDAB0 JobManager::ClearGoals
void JobManager::ClearGoals()
{
    constexpr uint32_t kFlag_ClearingGoals = 1u << 30;
    flags |= kFlag_ClearingGoals;
    if (g_game) {
        ape::ApeTArray<Goal*>& list = g_game->goals;
        while (list.Count() != 0) {
            Goal* goal = list[0];
            goals::GoalProgress progress;
            if (goal->GetStatus() == goals::kGoal_Active && goal->GetProgress(&progress))
                hud::ShowByName(progress.name, true, true, false);
            goal->ResetHints(true);
            delete goal;
            // The list keeps its order: later goals move up one place.
            for (uint32_t i = 1; i < list.Count(); ++i)
                list[i - 1] = list[i];
            list.PopBack();
        }
        list.Clear();
        g_game->jobStateBits &= 0x000FFFFFu;
    }
    flags &= ~kFlag_ClearingGoals;
    trackedGoal = nullptr;
}

// @func 0x002E12D0 JobManager::StartEpisode
bool JobManager::StartEpisode(Job* job, bool stayInLevel)
{
    if (!job)
        return false;
    ResetRunningJob();
    currentJob = job;
    job->Start();
    job->SetFlag80(false);
    if (job->nameHash != 0 && !stayInLevel)
        GameWorldRequestLevel(job->nameHash);
    return true;
}

// @func 0x002E142C JobManager::StartEpisodeById
bool JobManager::StartEpisodeById(uint32_t id)
{
    Job* job = FindJobById(id);
    return job ? StartEpisode(job, false) : false;
}

// @func 0x002E15BC JobManager::RestartEpisodeById
bool JobManager::RestartEpisodeById(uint32_t id)
{
    Job* job = FindJobById(id);
    if (!job)
        return false;
    ResetRunningJob();
    currentJob = job;
    GameWorldRestartState();
    return true;
}

// @func 0x002E1F44 JobManager::StartJob
bool JobManager::StartJob(uint32_t index)
{
    if (index >= episodeCount)
        return false;
    Episode* job = &episodes[index];
    ResetRunningJob();
    currentJob = nullptr;
    if (!job->ParseGoals())
        return false;
    if (g_game)
        job->MoveGoalsTo(g_game->goals);
    instance = job;
    currentJob = &jobs[job->episode];
    return true;
}

// @func 0x002E20CC JobManager::FindGoalToTrack
Goal* JobManager::FindGoalToTrack() const
{
    return PlayerFindGoal(trackedGoalId);
}

// @func 0x002E1E98 JobManager::RequestJobStart
void JobManager::RequestJobStart(entity::Entity* entity, Episode* job, uint32_t value, bool optionA, bool optionB)
{
    constexpr uint64_t kClass_JobStarter = 0x20000;   // NOTE: Entity::classBits bit 17
    if (pendingJob)
        pendingJob->ReleaseStreams();
    pendingKind = 1;
    pendingEntity = entity;
    pendingJob = job;
    pendingValue = value;
    streamFlags = (optionA ? 1u << 31 : 0u) | (optionB ? 1u << 30 : 0u) | (1u << 29);
    if (entity)
        entity->classBits |= kClass_JobStarter;
}

// @func 0x002E4D64 JobManager::CancelPendingStart
void JobManager::CancelPendingStart()
{
    if (!(streamFlags & (1u << 29))) {
        if (instance) {
            for (uint32_t i = 0; i < PlayerGoalCount(); ++i)
                PlayerGoalListAt(i)->ResetHints(true);
        }
        return;
    }
    if (pendingJob)
        pendingJob->ReleaseStreams();
    pendingKind = 0;
    pendingEntity = nullptr;
    pendingJob = nullptr;
    pendingValue = 0;
    streamFlags = 0;
    ClearGoals();
    ResetRunningJob();
    currentJob = nullptr;
}

namespace {
constexpr int kCompanionSlots = 9;
constexpr uint32_t kGoalFlag_HasAssociatedPlayer = 0x20;
constexpr uint32_t kGoalFlag_LockMask = 0x7;
constexpr uint32_t kCharacter_Sly = 0x00014120u;
constexpr uint32_t kManager_ClearingGoals = 1u << 30;
constexpr uint32_t kManager_RestorePending = 1u << 31;
constexpr uint32_t kManager_Bit28 = 1u << 28;

// The checkpoint transform of slot `index` of `goal`: its point instance,
// else the default spawn transform of the slot.
void CheckpointTransform(Goal* goal, int index, ape::Matrix34* out)
{
    const uint32_t point = goal->Field8F4(index);
    if (!point || !PointInstanceWorldTransform(point, out))
        DefaultSpawnTransform(index, out);
}

void ResetCharacterControl(entity::Entity* character)
{
    if (entity::EntityComponent* control = entity::EntityComponent03(character))
        control->ResetState();
}
} // namespace

// @func 0x002DE41C JobManager::PlacePlayerForGoal
void JobManager::PlacePlayerForGoal(Goal* goal)
{
    entity::Entity* player = PlayerEntity();
    int index = goal ? goal->CheckpointIndexOf(player) : -1;
    if (index == -1)
        index = PlayerCharacterSlot();
    ape::Matrix34& spawn = PlayerSpawnTransform();
    bool teleported = false;
    if (goal && goal->Field8F4(index)) {
        CheckpointTransform(goal, index, &spawn);
    } else {
        bool overrideSet = false;
        teleported = TakeRespawnOverride(&spawn, &overrideSet);
        if (!teleported)
            DefaultSpawnTransform(index, &spawn);
    }
    if (player) {
        player->SetTransform(spawn);
        ResetCharacterControl(player);
        if (teleported && (player->typeBits[0] & 0x8u))
            CharacterSnapToGround(player);
        GameCameraReset();
    }
}

// @func 0x002DE794 JobManager::PlaceCompanionsForGoal
void JobManager::PlaceCompanionsForGoal(Goal* goal)
{
    if (!goal)
        return;
    for (int slot = 0; slot < kCompanionSlots; ++slot) {
        entity::Entity* character = PlayableCharacter(slot);
        if (slot == PlayerCharacterSlot() || !character)
            continue;
        int index = goal->CheckpointIndexOf(character);
        if (index == -1)
            index = slot;
        ape::Matrix34 transform{};
        CheckpointTransform(goal, index, &transform);
        character->SetTransform(transform);
        ResetCharacterControl(character);
    }
}

// @func 0x002DEA50 JobManager::SelectGoal
void JobManager::SelectGoal(Goal* goal, bool keepPlayers, bool requestLevel)
{
    if (!goal)
        return;
    flags |= kManager_ClearingGoals;
    if (!keepPlayers) {
        if (goal->checkpoints[0] != 0 && (goal->flags & kGoalFlag_HasAssociatedPlayer))
            GameWorldUseAssociatedPlayer(goal);
        PlaceCompanionsForGoal(goal);
    }
    for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
        Goal* candidate = PlayerGoalListAt(i);
        if (candidate != goal) {
            // Every goal before the selected one counts as completed.
            candidate->Reset(true, goals::kGoal_Complete);
            const uint32_t word = goals::JobGoalSaveWord(candidate->job, candidate->saveSlot);
            goals::JobSetGoalSaveWord(candidate->job, candidate->saveSlot, word & ~0x0C000000u);
            continue;
        }
        goal->SetFlags(goal->flags & ~kGoalFlag_LockMask);
        trackedGoal = goal;
        goal->Reset(true, -1);
        if (keepPlayers)
            break;
        PlacePlayerForGoal(goal);
        if (!(goal->flags & kGoalFlag_HasAssociatedPlayer)) {
            uint32_t character = goal->field_964;
            if (character == 0 && instance)
                character = instance->associatedDef;
            entity::Entity* player = PlayerEntity();
            if (character != 0 && player) {
                const uint32_t playerDef = player->def ? player->def->nameHash : 0;
                const bool isSly = character == kCharacter_Sly &&
                                   (goals::EntityIsSlyClass(player) ||
                                    (goals::EntityIsCharacter(player) && goals::EntityGroupField_C0(player) == kCharacter_Sly));
                if (playerDef != character && !isSly) {
                    GameWorldSwitchPlayerCharacter(character);
                    break;
                }
            }
        }
        GameWorldRespawnPlayer(PlayerSpawnTransform());
        break;
    }
    flags &= ~kManager_ClearingGoals;
    const uint32_t level = trackedGoal ? trackedGoal->Field960() : 0;
    if (level != 0 && requestLevel && RunningLevel() != level)
        GameWorldRequestLevel(level);
}

// @func 0x002DEDB4 JobManager::RestoreGoals
void JobManager::RestoreGoals(bool fromSave, bool restoreFlags)
{
    if (!instance)
        return;
    if (fromSave && (flags & kManager_RestorePending)) {
        flags &= ~kManager_RestorePending;
        if (trackedGoalId != 0) {
            SelectGoal(PlayerFindGoal(trackedGoalId), false, false);
            trackedGoalId = 0;
        }
    } else {
        bool anyActive = false;
        uint32_t lastComplete = 0xFFFFFFFFu;
        const uint32_t count = PlayerGoalCount();
        for (uint32_t i = 0; i < count; ++i) {
            Goal* goal = PlayerGoalListAt(i);
            if (restoreFlags)
                goal->flags = goal->initialFlags;
            goal->Reset(fromSave, -1);
            if (fromSave) {
                if (goal->GetStatus() == goals::kGoal_Complete)
                    lastComplete = i;
                if (!anyActive && goal->GetStatus() == goals::kGoal_Active)
                    anyActive = true;
            }
        }
        if (!anyActive && lastComplete != 0xFFFFFFFFu && lastComplete + 1 < count) {
            Goal* next = PlayerGoalListAt(lastComplete + 1);
            next->SetFlags(next->flags & ~kGoalFlag_LockMask);
            next->Reset(fromSave, -1);
            trackedGoal = next;
        }
    }
    flags &= ~kManager_Bit28;
}

// @func 0x002E0008 JobManager::ApplyJobStart
void JobManager::ApplyJobStart(Episode* job)
{
    if (!job)
        return;
    PlayerProgress& progress = CurrentProgress();
    const bool episodeActive = jobs[job->episode].active != 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        Job& episode = jobs[i];
        const bool before = i < job->episode && episodeActive;
        bool markRunning = false;   // flag 0x20
        bool markLater = false;     // flag 0x80
        for (uint8_t s = 0; s < episode.stageCount; ++s) {
            Episode& stage = episode.stages[s];
            uint8_t& state = progress.jobStates[stage.index];
            if (before || state == kJobState_Complete) {
                state = kJobState_Complete;
                UnlockJobRewards(stage.nameHash);
                if (stage.field_1F7 == 0)
                    markRunning = true;
            }
            if (&stage == job && s != 0)
                markLater = true;
        }
        uint16_t& flags = progress.episodes[episode.episode].flags;
        if (before)
            flags = static_cast<uint16_t>((flags & ~0x18) | 0x10);
        if (markLater)
            flags |= 0x80;
        if (markRunning) {
            flags |= 0x20;
            progress.field_A0 = episode.field_32C;
        }
    }

    Job& episode = jobs[job->episode];
    EpisodeProgress& record = progress.episodes[episode.episode];
    if (progress.jobStates[job->index] != kJobState_Complete)
        record.stage = job->order;
    if (((record.flags >> 3) & 3) != 2)
        record.flags = static_cast<uint16_t>((record.flags & ~0x18) | 0x08);
    if (!(record.flags & 0x20))
        record.flags = job->field_1F7 == 0 ? static_cast<uint16_t>(record.flags | 0x20)
                                           : static_cast<uint16_t>(record.flags & ~0x20);
    if (record.flags & 0x20)
        progress.field_A0 = episode.field_32C;
    episode.UpdateJobStates();
}

// @func 0x002E0498 JobManager::PlayGoal
void JobManager::PlayGoal(Goal* goal, bool forceReload, bool levelRequestFlag)
{
    constexpr uint32_t kSharedScriptLibrary = 0x7719A548u;
    if (!goal)
        return;
    const int players = g_game ? g_game->localPlayerCount : 0;
    for (int i = 0; i < players; ++i)
        PlayerResetJobState(i);
    ApplyJobStart(instance);

    auto reloadScripts = [this]() {
        if (!instance)
            return;
        if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kSharedScriptLibrary)) {
            library->Clear();
            LoadJobScripts();
        }
    };
    const uint32_t level = goal->Field960();
    if (level == 0 || (!forceReload && RunningLevel() == level)) {
        reloadScripts();
        SelectGoal(goal, false, false);
        return;
    }
    RestoreGoals(false, true);
    if (RunningLevel() == level) {
        GameWorldReloadLevel();
        reloadScripts();
    } else {
        // NOTE: the console passes `levelRequestFlag` along with the level.
        (void)levelRequestFlag;
        GameWorldRequestLevel(level);
    }
    flags |= 1u << 31;
    trackedGoalId = goal->nameHash;
}

// @func 0x002E06D8 JobManager::BuildRunningJobGoals
bool JobManager::BuildRunningJobGoals(bool forceReload, bool levelRequestFlag, bool track)
{
    bool built = false;
    Episode* job = instance;
    if (job && job->ParseGoals()) {
        built = true;
        if (g_game)
            job->MoveGoalsTo(g_game->goals);
        instance = job;
        currentJob = &jobs[job->episode];
        if (track) {
            Goal* goal = nullptr;
            if (field_30 != 0) {
                for (uint32_t i = 0; i < PlayerGoalCount() && !goal; ++i) {
                    if (PlayerGoalListAt(i)->nameHash == field_30)
                        goal = PlayerGoalListAt(i);
                }
            }
            if (!goal)
                goal = PlayerGoalCount() ? PlayerGoalListAt(0) : nullptr;
            if (goal)
                PlayGoal(goal, forceReload, levelRequestFlag);
            else
                built = false;
        }
    }
    field_30 = 0;
    return built;
}

// @func 0x002E56F4 JobManager::SelectGoalByName
void JobManager::SelectGoalByName(uint32_t nameHash, bool keepPlayers, bool requestLevel)
{
    Goal* goal = nullptr;
    if (nameHash != 0) {
        for (uint32_t i = 0; i < PlayerGoalCount() && !goal; ++i) {
            if (PlayerGoalListAt(i)->nameHash == nameHash)
                goal = PlayerGoalListAt(i);
        }
    }
    SelectGoal(goal, keepPlayers, requestLevel);
}

// @func 0x002E1DB0 JobManager::StartJobFromHubByName
void JobManager::StartJobFromHubByName(uint32_t jobName, uint32_t goalName, bool toHub, bool toHideout,
                                       bool forceReload, bool levelRequestFlag, bool skipBookkeeping)
{
    Episode* job = jobName ? FindEpisode(jobName) : nullptr;
    StartJobFromHub(job, goalName, toHub, toHideout, forceReload, levelRequestFlag, skipBookkeeping, true);
}

// @func 0x002E0930 JobManager::StartJobFromHub
bool JobManager::StartJobFromHub(Episode* job, uint32_t goalName, bool toHub, bool toHideout, bool forceReload,
                                 bool levelRequestFlag, bool skipBookkeeping, bool track)
{
    constexpr uint32_t kLevel_Dummy = 0x03E60C88u;     // "Dummy"
    constexpr uint32_t kScript_InitJob = 0x9EECE88Du;  // "INITJOB"
    constexpr float kFirstUpdateStep = 0.0177f;
    const uint8_t profile = PlayerBlockProfile(0);
    PlayerProgress& record = ProfileProgress(profile);
    record.AddPendingCount(ProfilePendingCount(profile));
    record.GetNotifyPair(ProfileNotifyPair(profile));
    if (!job)
        return false;
    ResetRunningJob();
    currentJob = nullptr;
    if (instance)
        return false;

    if (!skipBookkeeping)
        ApplyJobStart(job);
    Job& episode = jobs[job->episode];
    uint32_t level = 0;
    if (toHub && episode.levelHub != 0 && episode.levelHub != kLevel_Dummy)
        level = episode.levelHub;
    else if (toHideout && episode.nameHash != 0)
        level = episode.nameHash;
    currentJob = &episode;
    if (level != 0 && RunningLevel() != level) {
        GameWorldRequestLevel(level);
    } else if (level == 0) {
        instance = job;
        field_30 = goalName;
        BuildRunningJobGoals(forceReload, levelRequestFlag, track);
    }
    if (ape::ApeAnimScriptInstance* script = StartObjectScript(kScript_InitJob, nullptr, 0, true))
        script->Update(kFirstUpdateStep);
    return true;
}

// @func 0x002E5414 JobManager::CompleteRunningJob
uint32_t JobManager::CompleteRunningJob()
{
    constexpr uint32_t kState_JobEnded = 0xB15553D6u;   // NOTE: state name unknown
    PlayerProgress& progress = CurrentProgress();
    const bool replay = progress.field_149 == 1 || progress.inJob == 1;
    uint32_t nextLevel = currentJob ? currentJob->field_2D8 : 0;
    if (!instance || !currentJob)
        return nextLevel;

    if (replay && currentJob->nameHash != 0)
        nextLevel = currentJob->nameHash;
    else if (instance->field_1DC != 0)
        nextLevel = instance->field_1DC;
    progress.jobStates[instance->index] = kJobState_Complete;
    UnlockJobRewards(instance->nameHash);

    if (!replay) {
        // Notification flags: the second job of episode 1, then finishing
        // episode 1 and the later episodes.
        if (currentJob->episode == 1 && instance->index == 1)
            progress.notifyBits |= 0x4;
        if (currentJob->IsStageComplete()) {
            EpisodeProgress& record = progress.episodes[currentJob->episode];
            ++record.stage;
            if (!currentJob->UpdateJobStates()) {
                record.flags = static_cast<uint16_t>((record.flags & ~0x18) | 0x10);
                if (currentJob->episode == 1)
                    progress.notifyBits |= 0x8;
                if (currentJob->episode > 1)
                    progress.notifyBits |= 0x10;
            }
        }
    }
    lastFinished = instance;
    ResetRunningJob();
    if (field_44) {
        if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(field_44->GetAnimScript()))
            script->SetStateAndUpdate(kState_JobEnded);
    }
    return nextLevel;
}

// @func 0x002E0C38 JobManager::UpdatePendingStart
void JobManager::UpdatePendingStart()
{
    constexpr uint32_t kHud_Loading = 0x24D73841u;   // NOTE: element name unknown
    constexpr uint64_t kClass_JobStarter = 0x20000;
    auto goalToPlay = [this]() -> Goal* {
        if (trackedGoal)
            return trackedGoal;
        if (field_30 != 0) {
            for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
                if (PlayerGoalListAt(i)->nameHash == field_30)
                    return PlayerGoalListAt(i);
            }
        }
        return PlayerGoalCount() ? PlayerGoalListAt(0) : nullptr;
    };

    if (streamFlags & (1u << 29)) {
        if (pendingKind == 1) {
            // Streaming the files while the loading element comes up.
            if (pendingJob->loadState == Episode::kLoad_None)
                pendingJob->StartLoading();
            else if (pendingJob->loadState == Episode::kLoad_Loading)
                pendingJob->UpdateLoading();
            hud::HudObject* loading = hud::Find(kHud_Loading);
            if (!loading) {
                pendingKind = 2;
            } else if (!loading->object || !hud::ObjectVisible(loading->object)) {
                hud::ShowTextByName(-1.0f, kHud_Loading, nullptr, true);
            } else {
                const uint32_t state = loading->animState >> 1;
                if (state == 3 || state == 2)
                    pendingKind = 2;
            }
        } else if (pendingKind == 2) {
            if (pendingJob->loadState == Episode::kLoad_Loading) {
                pendingJob->UpdateLoading();
            } else {
                StartJobFromHub(pendingJob, pendingValue, (streamFlags & (1u << 31)) != 0,
                                (streamFlags & (1u << 30)) != 0, false, false, false, false);
                if (Goal* goal = goalToPlay()) {
                    for (int slot = 0; slot < goals::kScriptSlotCount; ++slot)
                        goal->PrepareScriptSlot(slot);
                }
                pendingKind = 3;
            }
        } else if (pendingKind == 3) {
            Goal* goal = goalToPlay();
            bool streamed = true;
            if (goal) {
                for (int slot = 0; slot < goals::kScriptSlotCount && streamed; ++slot)
                    streamed = goal->ScriptSlotStreamed(slot);
            }
            if (streamed) {
                if (goal)
                    PlayGoal(goal, false, false);
                field_30 = 0;
                if (instance) {
                    for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
                        if (Goal* each = PlayerGoalListAt(i)) {
                            each->PrepareScripts();
                            each->ReapplyHints(nullptr);
                        }
                    }
                }
                pendingJob->ReleaseStreams();
                flags |= 1u << 28;
                if (pendingEntity) {
                    if (entity::EntityComponent* targeting = entity::EntityTargeting(pendingEntity)) {
                        targeting->SetOption(true);
                        TargetingSetEnabled(targeting, false);
                    }
                }
                hud::ShowByName(kHud_Loading, false, true, false);
                if (pendingEntity)
                    pendingEntity->classBits &= ~kClass_JobStarter;
                if (pendingJob)
                    pendingJob->ReleaseStreams();
                pendingKind = 0;
                pendingEntity = nullptr;
                pendingJob = nullptr;
                pendingValue = 0;
                streamFlags = 0;
            }
        }
    }
    if (failingGoal)
        failingGoal->RunScriptSlot(goals::kScripts_Failed);
}

// @func 0x002E3AEC JobManager::RespawnForGoal
void JobManager::RespawnForGoal(Goal* goal, int player)
{
    (void)player;
    entity::Entity* playerEntity = PlayerEntity();
    const uint32_t level = goal->Field960();
    if (level == 0 || level == RunningLevel()) {
        const uint32_t character = goal->field_964;
        bool switched = false;
        if (character != 0 && playerEntity) {
            const uint32_t playerDef = playerEntity->def ? playerEntity->def->nameHash : 0;
            const bool isSly = character == kCharacter_Sly &&
                               (goals::EntityIsSlyClass(playerEntity) ||
                                (goals::EntityIsCharacter(playerEntity) &&
                                 goals::EntityGroupField_C0(playerEntity) == kCharacter_Sly));
            if (playerDef != character && !isSly) {
                if (!goal->Field96C())
                    GameWorldSwitchPlayerCharacter(character);
                switched = true;
            }
        }
        if (!switched && goal->checkpoints[0] != 0 && (goal->flags & kGoalFlag_HasAssociatedPlayer) &&
            !goal->Field96C())
            GameWorldUseAssociatedPlayer(goal);
    } else {
        GameWorldRequestLevel(level);
    }

    int index = goal->CheckpointIndexOf(playerEntity);
    if (index == -1)
        index = PlayerCharacterSlot();
    const uint8_t bit = static_cast<uint8_t>(1u << (index & 7));
    const goals::GoalBlob& teleport = goal->blobs[2];      // "teleportToCheckPoint"
    const goals::GoalBlob& attachToPath = goal->blobs[1];  // "attachToSplineAtCheckPoint"
    if (!teleport.data || !(teleport.data[index >> 3] & bit))
        return;
    const uint32_t point = goal->Field8F4(index);
    ape::Matrix34 transform{};
    if (!point || !PointInstanceWorldTransform(point, &transform))
        return;
    GameWorldRespawnPlayer(transform);
    if (attachToPath.data && (attachToPath.data[index >> 3] & bit))
        CharacterSnapToGround(playerEntity);
}

// @func 0x002E5DD8 JobManager::OnGoalFailed
void JobManager::OnGoalFailed(Goal* goal, uint8_t player)
{
    constexpr uint32_t kFlag_FailPending = 1u << 29;
    constexpr uint32_t kEvent_GoalFailed = 0x9A986484u;   // NOTE: event name unknown
    constexpr uint32_t kType_IgnoresFail = 0x400000u;     // Entity::typeBits[0] bit 22
    constexpr uint64_t kClass_Controllable = 0x100;       // NOTE: Entity::classBits bit 8
    constexpr int kState_FailScreen = 10;
    if ((flags & kFlag_FailPending) || !goal || goal->field_49C[0] == '\0')
        return;
    const int state = GameWorldState(1);
    if (state == kState_FailScreen || state == 6 || state == 5)
        return;
    const char* message = goal->field_49C;
    entity::Entity* notified = lastEventEntity;
    if (goals::GameHudLockCount(g_game) == 0 && notified && !(notified->typeBits[0] & kType_IgnoresFail)) {
        entity::EntitySendGameEvent(nullptr, notified, kEvent_GoalFailed, 0, true);
        failMessage = message;
        failPlayer = player;
        flags |= kFlag_FailPending;
        if (entity::Entity* playerEntity = PlayerEntityOf(player)) {
            entity::EntityComponent* gadget = entity::EntityGadget(playerEntity);
            if (!gadget || GadgetAllowsStop(gadget))
                playerEntity->ResetState();
            playerEntity->classBits &= ~kClass_Controllable;
            if (entity::EntityComponent* targeting = entity::EntityTargeting(playerEntity)) {
                targeting->SetOption(false);
                TargetingClearTargets(targeting);
            }
            for (entity::EntityComponent* driver = playerEntity->GetComponent(0x17, nullptr); driver;
                 driver = playerEntity->GetComponent(0x17, driver)) {
                if (entity::AnimDriverState(driver) == 7)
                    AnimDriverStopScript(driver);
            }
        }
    } else if (GameWorldState(0) == 1 && GameWorldState(1) != kState_FailScreen) {
        if (GameWorldState(1) != 5) {
            GameWorldSetState(1, kState_FailScreen);
            GameWorldSetState(2, 0);
        }
        GameWorldSetStateData(message, player);
    }
    lastEventEntity = nullptr;
    failingGoal = goal;
}

} // namespace game
} // namespace sly
