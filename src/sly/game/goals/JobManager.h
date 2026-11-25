// openTIT - Job manager (console pointer 0x00F64C20): the episodes and jobs
// of the game and the goal the player is tracking. Only the lookups and the
// small accessors are ported (0x002E11D8-0x002E6E28); the per-player
// progress record they read is modelled as far as these use it.
#pragma once

#include "ape/container/ApeTSortedMap.h"
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
    uint16_t flags = 0;              // bits 0-2 safe state (3 = opened), bits 3-4 started (1) /
                                     // finished (2), bit 5 NOTE: set while its job runs, bit 6
                                     // completed, bits 7-8 NOTE: meaning unknown
    uint8_t stage = 0xFF;            // +0x02 job order being played, 0xFF = none
    uint8_t pad_03 = 0;
};
// Job states (record +0x1D0, one byte per job, indexed by Episode::index).
enum JobState : uint8_t {
    kJobState_Locked = 0,
    kJobState_Available = 1,
    kJobState_Complete = 2,
};
struct PlayerProgress {
    static constexpr int kEpisodeSlots = 12;
    uint32_t bottleBits[kEpisodeSlots] = {};  // +0x034 one bit per clue bottle (Job::bottleSlot)
    uint16_t maskBits[kEpisodeSlots] = {};    // +0x064 one bit per mask (Job::maskSlot)
    uint32_t field_70 = 0;           // +0x070 NOTE: read by 0x00465D5C, meaning unknown
    uint16_t jobBits[kEpisodeSlots] = {};     // +0x07C one bit per treasure (Job::bitGroup)
    uint32_t field_A0 = 0;           // +0x0A0 value of the running job (job +0x32C)
    uint8_t field_149 = 0;           // +0x149 NOTE: a yes/no option
    uint32_t field_14C = 0;          // +0x14C
    uint8_t inJob = 0;               // +0x150 1 while a job runs
    uint32_t currentJobValue = 0;    // +0x154 job record +0x2D4 of the running job
    EpisodeProgress episodes[kEpisodeSlots];  // +0x1A0
    uint8_t jobStates[0x100] = {};   // +0x1D0 JobState by Episode::index. NOTE: count assumed
    uint32_t unlockBits[2] = {};     // +0xF64 40 unlock flags
    uint32_t abilityBits[1] = {};    // +0xFB0 two bits per ability (see PlayerAbilities.h)
};
// The current profile's record. NOTE: the game world does not model its
// profile records yet; a stand-in record is used (see JobManager.cpp).
PlayerProgress& CurrentProgress();
// The record of profile 'profile' (stand-in, see CurrentProgress).
PlayerProgress& ProfileProgress(uint8_t profile);

// NOTE: the console's own terms are the other way round: the 0x35C-byte
// records ("Job" here) are its episodes (<Episode>: timePeriod, start and end
// movies, a hub and a hideout level) and the 0x218-byte records ("Episode"
// here) its jobs (<Job>: jobScript, movieOnJobComplete). The port keeps the
// names the job manager was first written with.
struct Episode;

// Job record (0x35C bytes, console constructor 0x002DA734).
struct Job {
    Job();

    char name[0x41];                 // +0x000
    char desc[0x41];                 // +0x041
    char timePeriod[0x41];           // +0x082
    char startMovie[0x81];           // +0x0C3
    char endMovie[0x81];             // +0x144
    char endMovie2[0x81];            // +0x1C5
    char endMovie3[0x81];            // +0x246
    char abbrev[5];                  // +0x2C7
    Episode* stages = nullptr;       // +0x2CC first of its 0x218-byte records
    uint8_t stageCount = 0;          // +0x2D0
    uint32_t field_2D4 = 0;          // +0x2D4 id; copied to the progress record when it starts
    uint32_t field_2D8 = 0;          // +0x2D8 NOTE: attribute 0xC9F8B7B3
    uint32_t field_2DC = 0;          // +0x2DC NOTE: attribute 0xA3A3941F
    uint32_t field_2E0 = 0;          // +0x2E0 NOTE: attribute 0x41299858
    uint32_t levelHub = 0;           // +0x2E4
    uint32_t nameHash = 0;           // +0x2E8 (attribute "levelHideout")
    uint32_t levels[16] = {};        // +0x2EC <Level name=...> children
    uint32_t field_32C = 0;          // +0x32C
    float field_330 = -1.0f;         // +0x330 NOTE: attribute 0x74BC9C7B
    uint8_t levelCount = 0;          // +0x334
    uint8_t numBottles = 0;          // +0x335
    uint8_t episode = 0;             // +0x336 own index (the progress record's episode slot)
    uint8_t bottleSlot = 0xFF;       // +0x337 index into PlayerProgress::bottleBits
    uint8_t maskSlot = 0xFF;         // +0x338 index into PlayerProgress::maskBits
    uint8_t bitGroup = 0xFF;         // +0x339 index into PlayerProgress::jobBits (treasures), 0xFF = none
    uint8_t isDebugEpisode = 0;      // +0x33A
    uint8_t field_33B = 0;           // +0x33B NOTE: bool attribute 0xE587C810
    uint8_t active = 1;              // +0x33C
    uint8_t field_33D = 0;           // +0x33D NOTE: bool attribute 0x37C7C4AD (gives a mask slot)
    float episodeWeight = 0.0f;      // +0x340 share in the game's completion
    float jobWeight = 0.0f;          // +0x344 completion formula weights
    float maskWeight = 0.0f;         // +0x348
    float bottleWeight = 0.0f;       // +0x34C
    float treasureWeight = 0.0f;     // +0x350
    float safeWeight = 0.0f;         // +0x354
    uint32_t field_358 = 0;          // +0x358 NOTE: attribute 0xEED7AB50

    // Progress of this episode in the current profile's record.
    EpisodeProgress& Progress() const;
    // Started (1) / finished (2) state, bits 3-4 (0x002DA85C).
    int ProgressState() const;
    // Completed flag 0x40 (0x002DA89C).
    bool IsCompleted() const;
    // Progress flag 0x100 (0x002DA8E8 / 0x002DA944). NOTE: meaning unknown.
    void SetFlag100(bool on);
    bool Flag100() const;
    // Progress flag 0x80 (0x002DAFA0 / 0x002DAF54). NOTE: meaning unknown.
    void SetFlag80(bool on);
    bool Flag80() const;
    // Safe state, bits 0-2; 3 is opened (0x002DA990).
    int SafeState() const;
    // Job order being played, 0xFF for none (0x002DAFFC).
    uint8_t CurrentStage() const;
    // Treasures collected / placed in this episode (0x002DA9D0 / 0x002DAA54).
    int CollectedTreasures() const;
    int TreasureCount() const;
    // Clue bottles collected (0x002DAB04).
    int CollectedBottles() const;
    // Masks collected in `progress` / the current record (0x002DAC24 /
    // 0x002DAE5C), and the masks that belong to the episode (0x002DAEA0).
    int CollectedMasks(const PlayerProgress& progress) const;
    int CollectedMasks() const;
    int MaskCount() const;
    // Completion in percent from the weighted jobs, masks, bottles,
    // treasures and safe (0x002DB040 / 0x002DB5D4).
    float Completion(const PlayerProgress& progress) const;
    float Completion() const;
    // Every job of the current stage is complete (0x002DB618).
    bool IsStageComplete() const;
    // Makes the current stage's jobs available and completes the earlier
    // ones; true if any job belongs to the current stage (0x002DB70C).
    bool UpdateJobStates() const;
    // Starts the episode unless it is finished: state 1, stage 0 (0x002DB8B0).
    void Start();
};

// Episode record (0x218 bytes).
struct Episode {
    uint8_t data[0x1D4] = {};
    uint32_t nameHash = 0;           // +0x1D4
    uint8_t pad_1D8[0x1C] = {};
    uint8_t episode = 0;             // +0x1F4 index of its 0x35C-byte record
    uint8_t order = 0;               // +0x1F5 stage it belongs to
    uint8_t index = 0;               // +0x1F6 PlayerProgress::jobStates slot
    uint8_t pad_1F7[0x21] = {};
};

// Collectible records of the mask (0x0106819C) and treasure (0x01068184)
// tables, keyed by name hash.
struct CollectibleRecord {
    uint8_t episode = 0;             // +0x00 index of the 0x35C-byte record it belongs to
    uint8_t bit = 0;                 // +0x01 bit in that record's mask / treasure word
    uint8_t pad_02[2] = {};
    uint32_t key = 0;                // +0x04 name hash of the record whose bit is tested
    uint32_t field_08 = 0;           // +0x08 NOTE: must be set for a mask to count
    uint32_t foundIn = 0;            // +0x0C id (+0x2D4) of the episode where it is found
};
ape::ApeTSortedMap<CollectibleRecord*>& MaskTable();
ape::ApeTSortedMap<CollectibleRecord*>& TreasureTable();
// Default clue bottle count of an episode (0x00F64C24, 30).
extern int g_defaultBottleCount;
// 0x0049E038: sets the unlock flags tied to a completed job (outside this
// range).
void UnlockJobRewards(uint32_t jobId);

class JobManager {
public:
    // 0x35C-byte record whose id (+0x2D4) is `id`, or null (0x002DABCC).
    Job* FindJobById(uint32_t id) const;
    // Its index, or -1.
    uint32_t JobIndexById(uint32_t id) const;
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
