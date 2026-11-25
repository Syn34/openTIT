// openTIT - Job manager (console pointer 0x00F64C20): the episodes and jobs
// of the game and the goal the player is tracking. Only the lookups and the
// small accessors are ported (0x002E11D8-0x002E6E28); the per-player
// progress record they read is modelled as far as these use it.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace sly {
namespace goals { class Goal; }

namespace game {
using goals::Goal;

// Progress record of the current profile (game world +0x50, one 0xFD8-byte
// record per profile, indexed by the profile byte at +0x1070). NOTE: only the
// fields the job manager touches are named; offsets are record-relative.
struct EpisodeProgress {
    uint16_t flags = 0;              // bits 3-4 started/finished, bit 5 NOTE: set while its job runs
    uint16_t field_02 = 0;
};
struct PlayerProgress {
    uint32_t field_70 = 0;           // +0x070 NOTE: read by 0x00465D5C, meaning unknown
    uint16_t jobBits[0x28] = {};     // +0x07C per job group, one bit per collected item. NOTE: count assumed
    uint32_t field_A0 = 0;           // +0x0A0 value of the running job (job +0x32C)
    uint8_t field_149 = 0;           // +0x149 NOTE: a yes/no option
    uint32_t field_14C = 0;          // +0x14C
    uint8_t inJob = 0;               // +0x150 1 while a job runs
    uint32_t currentJobValue = 0;    // +0x154 job record +0x2D4 of the running job
    EpisodeProgress episodes[0x20];  // +0x1A0 NOTE: count assumed
    uint32_t unlockBits[2] = {};     // +0xF64 40 unlock flags
    uint32_t abilityBits[1] = {};    // +0xFB0 two bits per ability (see PlayerAbilities.h)
};
// The current profile's record. NOTE: the game world does not model its
// profile records yet; a stand-in record is used (see JobManager.cpp).
PlayerProgress& CurrentProgress();
// The record of profile 'profile' (stand-in, see CurrentProgress).
PlayerProgress& ProfileProgress(uint8_t profile);

// Job record (0x35C bytes).
struct Job {
    uint8_t data[0x2D4] = {};
    uint32_t field_2D4 = 0;          // +0x2D4 copied to the progress record when it starts
    uint8_t pad_2D8[0x10] = {};
    uint32_t nameHash = 0;           // +0x2E8
    uint8_t pad_2EC[0x40] = {};
    uint32_t field_32C = 0;          // +0x32C
    uint8_t pad_330[6] = {};
    uint8_t episode = 0;             // +0x336
    uint8_t pad_337[2] = {};
    uint8_t bitGroup = 0xFF;         // +0x339 index into PlayerProgress::jobBits, 0xFF = none
    uint8_t pad_33A[2] = {};
    uint8_t active = 0;              // +0x33C
};

// Episode record (0x218 bytes).
struct Episode {
    uint8_t data[0x1D4] = {};
    uint32_t nameHash = 0;           // +0x1D4
};

class JobManager {
public:
    // Episode by name hash, or null / its index, or -1 (0x002E1220 / 0x002E1278).
    Episode* FindEpisode(uint32_t nameHash) const;
    uint32_t EpisodeIndex(uint32_t nameHash) const;
    // Tells the tracked goal the level was loaded (0x002E11D8).
    void NotifyTrackedGoalLoaded();
    // Description of the tracked goal, or null (0x002E207C).
    const char* TrackedGoalDescription() const;
    // The goal after the tracked one in the player's goal list (0x002E2214).
    Goal* GoalAfterTracked() const;
    // Last active job whose episode has been started (0x002E23AC).
    Job* LastStartedJob() const;
    // A job with this name exists (0x002E3130).
    bool HasJob(uint32_t nameHash) const;
    // The tracked goal has flag 0x40 (0x002E5034).
    bool TrackedGoalHasFlag40() const;
    // Marks the progress record as inside (or outside) a job (0x002E6DC4).
    void SetInJob(bool inJob);
    // The goal at `goal`'s list position belongs to the job running now
    // (0x002E2104).
    bool IsGoalLive(const Goal* goal) const;
    // Sets or clears flag 0x20 of the running job's episode and records the
    // job's value (0x002E26A8).
    void SetCurrentEpisodeFlag(bool on);
    // Jobs whose episode has flag 0x20 and that have a value (0x002E2A98).
    int CountFlaggedJobs() const;
    // Collected bits over all jobs (0x002E6270).
    int CountJobBits() const;
    // Awards the trophies for finishing the first and the sixth episode
    // (counts episodes with flag 0x40) (0x002E619C).
    void CheckEpisodeTrophies() const;
    // Hands a pending level request to the game world: unless the game is
    // already restarting (state layer 1 at 10) it switches layer 1 to 10 and
    // layer 2 to 0, then passes the request's two words as the state data
    // (0x002E2E78; 0x002E2FD4 is an identical copy).
    void FlushPendingRequest();
    // Ends the running job: cleans it up, empties the shared anim script
    // library and resets the players' job state; the current job is kept
    // only with `keepCurrent` (0x002E0860).
    void EndJob(bool keepCurrent);
    // Makes the job whose value (+0x2D4) is `value` the current one, ending
    // the running job (0x002E22A0).
    void SelectJobByValue(uint32_t value);
    // Appends every job named `nameHash` to `out` (0x002E2D7C).
    void CollectJobs(uint32_t nameHash, ape::ApeTArray<Job*>& out) const;
    // Ends the running job instance, first updating its episode's flag 0x20
    // (0x002E5060).
    void FinishJobInstance();
    // The job with value `value` (0: the current job) has its episode's
    // flag 0x20; the running level's own job counts as flagged (0x002E2740).
    bool IsJobValueFlagged(uint32_t value) const;

    Job* jobs = nullptr;             // +0x14
    uint32_t jobCount = 0;           // +0x18
    Episode* episodes = nullptr;     // +0x1C
    uint32_t episodeCount = 0;       // +0x20
    Job* currentJob = nullptr;       // +0x24
    struct JobInstance* instance = nullptr;  // +0x28 the running job (byte +0x1F7: NOTE failed?)
    uint32_t field_30 = 0;           // +0x30
    Goal* trackedGoal = nullptr;     // +0x48
    uint32_t pendingRequest[2] = {}; // +0x58
    uint32_t flags = 0;              // +0x64 bit 29: a request is pending

private:
    void ResetRunningJob();
};

// The running instance of a job (opaque; byte +0x1F7 read).
struct JobInstance {
    uint8_t data[0x1F7] = {};
    uint8_t field_1F7 = 0;
};
// 0x002DDAB0: cleans up the running job (outside this range).
void JobManagerCleanup(JobManager* manager);
// 0x0045C7A8 on a local player's block (game world +0x1070 + i * 0x20C80).
void PlayerResetJobState(int player);

// The job manager (0x00F64C20).
JobManager* GetJobManager();
void SetJobManager(JobManager* manager);

// Unlock flag `bit` (< 40) of the current profile (0x002E2950).
bool IsUnlocked(uint32_t bit);
// Earned share of the trophy points, 0..1 (0x002E3A70).
float TrophyCompletion();
// Progress record accessors (0x002E5394, 0x002E53D4, 0x002E6D30,
// 0x002E6D6C, 0x002E6D98, 0x002E6E28).
bool IsInJob();
bool ProgressFlag149();
void SetProgressFlag149(bool on);
void SetProgressField14C(uint32_t value);
uint32_t ProgressField14C();
uint32_t CurrentJobValue();
// Goal `index` of the player's goal list (game world +0x10C0, count
// +0x10C8), null for -1 or out of range (0x002E6A3C).
Goal* PlayerGoalAt(uint32_t index);

// Sets the unlock flag of the unlockable whose key is `key` (0x002E2874).
void UnlockByKey(uint32_t key);
// Unlockables (array 0x010681FC, count 0x01068204; key at record +0x60).
uint32_t UnlockableCount();
uint32_t UnlockableKey(uint32_t index);
struct Unlockable;
Unlockable* UnlockableAt(uint32_t index);
uint32_t UnlockableGroup(uint32_t index);   // record +0x68
// Appends the unlockables of `group` to `out` (0x002E6FC0).
void CollectUnlockables(uint32_t group, ape::ApeTArray<Unlockable*>& out);
// Job value of the running level (game world +0x128164 level object +0x15A4,
// unless its +0x15C4 bit 27 is set); false when there is none.
bool CurrentLevelJobValue(uint32_t* value);

// Enables or disables the targeting component of every local player
// (0x002E4A0C).
void SetPlayersTargeting(bool enabled);

// Player goal list (game world +0x10C0 / +0x10C8). Stubs until the game
// world models it.
uint32_t PlayerGoalCount();
Goal* PlayerGoalListAt(uint32_t index);
// Game world state machine (slot 8 GetState, 0x004665C0 SetState) and the
// data word pair of its current state (+0x14 buffer, +0x06 offset).
int GameWorldState(int layer);
void GameWorldSetState(int layer, int state);
void GameWorldSetStateData(uint32_t first, uint32_t second);
// Game world +0x128114: id of the job that is running (0 = none).
uint32_t LiveJobId();
// 0x0029D838 on a targeting component.
void TargetingSetEnabled(void* targeting, bool enabled);

} // namespace game
} // namespace sly
