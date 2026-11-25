// openTIT - Job manager lookups and progress accessors.
#include "sly/game/goals/JobManager.h"

#include "sly/entity/Awards.h"
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"
#include "sly/game/SlyGame.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "sly/game/goals/Goal.h"
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
void GameWorldSetState(int, int) {}
void GameWorldSetStateData(uint32_t, uint32_t) {}
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
        GameWorldSetStateData(pendingRequest[0], pendingRequest[1]);
    }
    pendingRequest[0] = 0;
    pendingRequest[1] = 0;
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

} // namespace game
} // namespace sly
