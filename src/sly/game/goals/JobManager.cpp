// openTIT - Job manager lookups and progress accessors.
#include "sly/game/goals/JobManager.h"

#include "sly/entity/Awards.h"
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
        if (job.active && ((progress.episodeState[job.episode] >> 3) & 3) != 0)
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
