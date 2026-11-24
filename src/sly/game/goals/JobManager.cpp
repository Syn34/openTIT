// openTIT - Job manager lookups and progress accessors.
#include "sly/game/goals/JobManager.h"

#include "sly/entity/Awards.h"
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/Goal.h"

namespace sly {
namespace game {

namespace {
JobManager* g_jobManager = nullptr;
PlayerProgress g_progressStandIn;
constexpr uint32_t kNoIndex = 0xFFFFFFFFu;
} // namespace

JobManager* GetJobManager() { return g_jobManager; }
void SetJobManager(JobManager* manager) { g_jobManager = manager; }
PlayerProgress& CurrentProgress() { return g_progressStandIn; }
uint32_t PlayerGoalCount() { return 0; }
Goal* PlayerGoalListAt(uint32_t) { return nullptr; }
uint32_t LiveJobId() { return 0; }
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

} // namespace game
} // namespace sly
