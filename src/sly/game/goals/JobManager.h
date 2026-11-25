// openTIT - Job manager (console pointer 0x00F64C20): the episodes and jobs
// of the game and the goal the player is tracking. Only the lookups and the
// small accessors are ported (0x002E11D8-0x002E6E28); the per-player
// progress record they read is modelled as far as these use it.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"
#include "sly/game/Upgrades.h"

#include <cstdint>

namespace ape { class ApeAllocator; struct Matrix34; }

namespace sly {
class Spawner;
struct GameEvent;
namespace goals { class Goal; }
namespace entity { class Entity; }

namespace game {
using goals::Goal;
class StreamDiskFile;

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
    static constexpr int kCostumeSlots = 6;
    static constexpr int kArcadeTables = 64;
    static constexpr int kUnlockEntries = 64;

    // Fresh record of a new profile: everything cleared, the costume slots
    // named, the unlock entries keyed and the arcade tables filled with the
    // default scores (0x000ADE04).
    void Reset();
    // Adds a pending count to the counter at +0x144 and clears the pending
    // value (0x000AE3FC).
    void AddPendingCount(uint8_t& pending);
    // Notification flags 15 and 16 as a pair of booleans (0x000AE418 reads
    // them, 0x000AE454 writes them; a byte of 1 sets a flag).
    void GetNotifyPair(uint8_t out[2]) const;
    void SetNotifyPair(const uint8_t in[2]);

    uint8_t field_000 = 0;           // +0x000 NOTE: from 0x010B1804 at reset
    uint8_t field_001 = 0;           // +0x001 NOTE: from 0x00F65BF4 (1) at reset
    uint16_t field_002 = 0;          // +0x002 NOTE: from 0x00F65BF5 (1) at reset
    uint32_t field_020 = 0;          // +0x020
    uint32_t bottleBits[kEpisodeSlots] = {};  // +0x034 one bit per clue bottle (Job::bottleSlot)
    uint16_t maskBits[kEpisodeSlots] = {};    // +0x064 one bit per mask (Job::maskSlot)
    uint32_t field_70 = 0;           // +0x070 NOTE: read by 0x00465D5C, meaning unknown
    uint8_t entityBits[kEpisodeSlots] = {};   // +0x094 per episode: entities of the index table found
    uint16_t jobBits[kEpisodeSlots] = {};     // +0x07C one bit per treasure (Job::bitGroup)
    uint32_t field_A0 = 0;           // +0x0A0 value of the running job (job +0x32C)
    uint32_t field_134 = 0;          // +0x134
    uint64_t field_138 = 0;          // +0x138
    uint16_t field_140 = 0;          // +0x140
    uint16_t field_142 = 0;          // +0x142
    uint8_t field_144 = 0;           // +0x144 NOTE: a counter fed by AddPendingCount
    uint8_t field_145 = 0;           // +0x145
    uint8_t field_146 = 0;           // +0x146
    uint8_t penguinsShot = 0;        // +0x147 penguins Carmelita shot (stops at 0xFF)
    uint8_t field_149 = 0;           // +0x149 NOTE: a yes/no option
    uint32_t field_14C = 0;          // +0x14C
    uint8_t inJob = 0;               // +0x150 1 while a job runs
    uint32_t currentJobValue = 0;    // +0x154 job record +0x2D4 of the running job
    uint8_t field_158 = 0;           // +0x158
    uint32_t notifyBits = 0;         // +0x15C named notification flags (sly/levels PlayerNotifyFlags)
    // Costume choice per slot ("Head", "Body", "hands", ...): slot name and
    // the chosen option (read by the CustomModel component).
    struct CostumeChoice {
        uint32_t slot = 0;
        uint32_t option = 0;
    };
    CostumeChoice costume[kCostumeSlots];   // +0x160
    EpisodeProgress episodes[kEpisodeSlots];  // +0x1A0
    uint8_t jobStates[0x90] = {};    // +0x1D0 JobState by Episode::index (144 jobs)
    // Arcade high score tables (0x2C bytes each): five scores with the
    // initials of who made them, and whether a top score was reached.
    struct ArcadeScores {
        int32_t scores[5] = {};
        char initials[5][4] = {};
        uint8_t beaten = 0;          // +0x28
        uint8_t pad[3] = {};
    };
    ArcadeScores arcade[kArcadeTables];   // +0x260
    // Unlockable entries keyed by name (table 0x00F64494).
    struct UnlockEntry {
        uint32_t name = 0;
        uint32_t state = 0;
    };
    UnlockEntry unlocks[kUnlockEntries];  // +0xD60
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

// Episode record (0x218 bytes, console constructor 0x002D9740). The job
// manager's running instance (+0x28) points at one.
struct Episode {
    Episode();

    // A <HideoutCharacter> entry (0xC bytes, 0xB copied).
    struct HideoutCharacter {
        uint32_t name = 0;
        uint32_t field_04 = 0;       // NOTE: attribute 0x09999BDC
        bool selectable = true;
        bool field_09 = false;       // NOTE: attribute 0xABB46BE6
        bool field_0A = false;       // NOTE: attribute 0xB15553D6
    };
    struct ScriptPath {
        char path[0x81];
    };

    // State of the job in the current profile's record (0x002D98A4).
    uint8_t State() const;
    // Replaces the stored copy of the text with the same id by `text`'s
    // string and marks it changed (0x002D98DC).
    void StoreText(const TextField& text);
    // Copies the stored text with `text`'s id over `text`; false if there is
    // none (0x002D9994).
    bool LoadText(TextField& text) const;

    char name[0x41];                 // +0x000
    char desc[0x41];                 // +0x041
    char jobScript[0x81];            // +0x082
    char movieOnJobComplete[0x81];   // +0x103
    char abbrev[0x10];               // +0x184
    uint8_t field_194 = 0;           // +0x194
    ape::ApeTArray<ScriptPath> scriptPaths;          // +0x198
    ape::ApeTArray<TextField> texts;                 // +0x1AC
    ape::ApeTArray<HideoutCharacter> characters;     // +0x1C0
    uint32_t nameHash = 0;           // +0x1D4 id
    uint32_t field_1D8 = 0;          // +0x1D8 NOTE: attribute 0x53B55808
    uint32_t field_1DC = 0;          // +0x1DC NOTE: attribute 0xC9F8B7B3
    uint32_t field_1E0 = 0;          // +0x1E0 NOTE: attribute 0xA3A3941F
    uint32_t field_1E4 = 0;          // +0x1E4 NOTE: attribute 0x41299858
    uint32_t field_1E8 = 0xD433715Au;  // +0x1E8 NOTE: attribute 0x6E6EDD6B
    uint32_t associatedDef = 0;      // +0x1EC
    float field_1F0 = -1.0f;         // +0x1F0 NOTE: attribute 0x74BC9C7B
    uint8_t episode = 0;             // +0x1F4 index of its 0x35C-byte record
    uint8_t order = 0;               // +0x1F5 stage it belongs to
    uint8_t index = 0;               // +0x1F6 PlayerProgress::jobStates slot
    uint8_t field_1F7 = 0;           // +0x1F7 attribute "disguiseLocked"
    uint8_t field_1F8 = 0;           // +0x1F8 NOTE: bool attribute 0xE587C810
    uint8_t field_1F9 = 1;           // +0x1F9 NOTE: bool attribute 0x6AAB7C29
    uint8_t field_1FA = 0;           // +0x1FA NOTE: int attribute 0xBE8B7B06
    // Streaming of the job's goal file and script files (0x002D9A14-
    // 0x002DA718).
    enum LoadState : int32_t { kLoad_None = 0, kLoad_Loading = 1, kLoad_Loaded = 2 };
    // One file being streamed (0x10 bytes): the request and, once loaded,
    // the data it handed over (freed through `allocator`).
    struct FileLoad {
        StreamDiskFile* file = nullptr;          // +0x00
        ape::ApeAllocator* allocator = nullptr;  // +0x04
        void* data = nullptr;                    // +0x08
        uint32_t size = 0;                       // +0x0C
    };
    // Drops every request, loaded file and parsed goal (0x002D9A14).
    void ReleaseStreams();
    // Starts streaming the goal file and every script file (0x002DA2A4).
    void StartLoading();
    // Advances the streams; all files read: loaded (0x002DA438). FinishLoading
    // blocks on the first file still busy (0x002DA5E0).
    void UpdateLoading();
    void FinishLoading();
    // Builds the job's goals from its goal file (the streamed copy, else the
    // file on disk); false when there are none (0x002D9CC0).
    bool ParseGoals();
    // Hands the parsed goals over to `owner`'s goal list (0x002DA04C).
    void MoveGoalsTo(ape::ApeTArray<Goal*>& goalList);

    int32_t loadState = kLoad_None;  // +0x1FC LoadState
    FileLoad script;                 // +0x200 the goal file (jobScript)
    ape::ApeTArray<FileLoad*>* packages = nullptr;   // +0x210 one per script path
    ape::ApeTArray<Goal*>* goals = nullptr;          // +0x214 parsed, not yet handed over
};
using JobInstance = Episode;

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
// Safe records (0x010681B4), keyed by the safe spawner's name.
ape::ApeTSortedMap<CollectibleRecord*>& SafeTable();
// Default clue bottle count of an episode (0x00F64C24, 30).
extern int g_defaultBottleCount;
// 0x0049E038: sets the unlock flags tied to a completed job (outside this
// range).
void UnlockJobRewards(uint32_t jobId);

class JobManager {
public:
    // Registers itself as the job manager, makes room for 12 episode and
    // 144 job records and loads "Scripts/Episodes.xml" (0x002DD4FC).
    JobManager();
    // Frees the collectible tables and the records (0x002DD604).
    ~JobManager();
    JobManager(const JobManager&) = delete;
    JobManager& operator=(const JobManager&) = delete;

    static constexpr uint32_t kMaxJobs = 12;       // 0x2850 bytes
    static constexpr uint32_t kMaxEpisodes = 144;  // 0x12D80 bytes

    // Reads the episode list: movie unlockables, movie entries, the
    // entity index table and every <Episode file=...> (0x002DCDD0).
    void Load(const char* path);
    // Reads one episode file into the next record; the slot counters hand
    // out the bottle, mask and treasure word indices (0x002DB968).
    void LoadEpisode(const char* path, uint8_t* bottleSlot, uint8_t* maskSlot, uint8_t* treasureSlot,
                     bool active);
    // Loads the running job's script files into the shared job library
    // unless it already holds scripts (0x002DD898); 0x002DDA54 empties the
    // library first.
    void LoadJobScripts();
    void ReloadJobScripts();
    // Level start: counts the collected bottles and masks, picks the goal
    // to track, clears the per-level state and, when the tracked goal
    // belongs to another level, asks for that level (0x002DDDCC).
    void OnLevelStart();
    // Links the job library's scripts, then OnLevelStart (0x002DE198).
    void OnLevelLoaded();
    // Activates a mask spawner unless its mask is collected in `maskBits`
    // (one word per episode slot); warns about spawners missing from the
    // mask table (0x002DF010).
    void ActivateMaskSpawner(class Spawner* spawner, const uint16_t* maskBits) const;
    // Activates a treasure spawner: outside the hideout while its treasure
    // is still missing; in the hideout (`inHideout`) once it is collected,
    // or always when no episode is current (0x002DFAA0).
    void ActivateTreasureSpawner(class Spawner* spawner, const uint16_t* treasureBits, bool inHideout) const;
    // Fires the level's treasure spawners (spawn group 0xFB673D64) and the
    // safe spawners (group 0x5E69B571) of opened safes (0x002DFC2C).
    void SpawnCollectibles() const;
    // Lets every goal of the player prepare its scripts and reapply its
    // hints while a job runs (0x002DFEFC).
    void PrepareGoalScripts() const;
    // Stores the running job's value in the profile record (0x002DFFDC).
    void SetRunningJobValue(uint32_t value) const;
    // Masks collected over every active record (0x002DDC5C).
    int CountCollectedMasks() const;
    // The current record, else the first non-debug record that lists the
    // running level (0x002DDD24).
    Job* CurrentEpisode() const;
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
    // The current episode's stage has an unfinished job whose character
    // (+0x1EC) is `character` (a character also matches its variant, e.g.
    // Sly / Sly in disguise) (0x002E16E8); 0x002E1900 returns that job.
    bool CurrentStageUsesCharacter(uint32_t character) const;
    Episode* CurrentStageJobForCharacter(uint32_t character) const;
    // A hideout character of the current stage's jobs can be selected
    // (0x002E1B1C; true without an episode) / has flag +0x09 (0x002E1C88).
    bool HideoutCharacterSelectable(uint32_t name) const;
    bool HideoutCharacterFlag09(uint32_t name) const;
    // The hideout characters offered by the last started episode: those of
    // its latest unlocked job, the +0x0A ones only once that job is
    // complete (0x002E2438).
    void CollectHideoutCharacters(ape::ApeTArray<Episode::HideoutCharacter>& out) const;
    // Records that entity `entity` of the index table was found in the
    // current episode; the first one found anywhere awards a trophy
    // (0x002E2B34).
    void MarkEpisodeEntityFound(uint32_t entity) const;
    // Completion of the whole game in percent: every episode's completion
    // weighted by its episode weight (0x002E3188).
    float GameCompletion(const PlayerProgress* progress) const;
    // Collected share in percent over all episodes: clue bottles
    // (0x002E32D8), masks of the active episodes (0x002E34A8), treasures
    // (0x002E3698) and opened safes of the episodes with treasures
    // (0x002E3898). A null record means the current profile.
    float BottlePercent(const PlayerProgress* progress) const;
    float MaskPercent(const PlayerProgress* progress) const;
    float TreasurePercent(const PlayerProgress* progress) const;
    float SafePercent(const PlayerProgress* progress) const;
    // Masks of every active episode (0x002E57A8).
    int TotalMaskCount() const;
    // `entity` (alive) is the character `job` is played with (Sly also
    // matches entities with class bit 5) (0x002E4ED0).
    bool IsJobCharacter(const entity::Entity* entity, const Episode* job) const;
    // Passes a game event to the active goals of the running job (unless the
    // goals are being cleared) and counts the penguins Carmelita shoots for
    // the trophy (0x002E5180).
    void OnGameEvent(int player, const GameEvent& e);
    // Records a score of `rank` on arcade table `table`: a top score (rank
    // below the award index) marks the table; the first and the sixth tables
    // marked award trophies (0x002E6A88).
    void RecordArcadeScore(int table, int rank) const;
    // After a treasure was collected: marks the episode (flag 0x40) once all
    // its treasures are found, awards the treasure trophy, marks every
    // episode once all treasures of the game are found, then checks the
    // episode trophies (0x002E6320).
    void OnTreasureCollected(Job* job) const;
    // The mask placed by `spawner` is still to be found by profile
    // `profile`; false (with a warning) for spawners missing from the mask
    // table (0x002E6B8C).
    bool MaskSpawnerAvailable(const Spawner* spawner, uint8_t profile) const;
    // Flag bit 27 of +0x64; switching it off hands the progress counter of
    // every active goal to the HUD (0x002E6E54). NOTE: name from use by the
    // minigame goals.
    void SetGoalCountersSuppressed(bool suppressed);
    // A player picked up `treasure`: its bit is set in the collector's
    // profile (notification flag 5 too), the treasure bookkeeping runs and
    // the treasure's spawner is deactivated (0x002E66F4).
    void CollectTreasure(entity::Entity* treasure, entity::Entity* collector) const;
    // Ends the running job and starts episode `job`: stage 0, flag 0x80
    // cleared and (unless `stayInLevel`) a request for its hideout level
    // (0x002E12D0); 0x002E142C finds the episode by its id first.
    bool StartEpisode(Job* job, bool stayInLevel);
    bool StartEpisodeById(uint32_t id);
    // Ends the running job, makes the episode with id `id` current and
    // restarts the game world state (0x002E15BC).
    bool RestartEpisodeById(uint32_t id);
    // Ends the running job and runs job `index`: its goals are built from
    // its goal file and handed to the players (0x002E1F44).
    bool StartJob(uint32_t index);
    // The player's goal with the id to track after loading (0x002E20CC).
    Goal* FindGoalToTrack() const;
    // Queues the start of `job` (its files are streamed meanwhile) for
    // `entity` (0x002E1E98).
    void RequestJobStart(entity::Entity* entity, Episode* job, uint32_t value, bool optionA, bool optionB);
    // Without a pending start, lets every goal reset its hints; with one,
    // drops it and ends the running job and episode (0x002E4D64).
    void CancelPendingStart();
    // Places the local player at the checkpoint of `goal` that belongs to
    // the player's character (default spawn point without one, or a
    // pending respawn override without a goal) and resets the camera
    // (0x002DE41C).
    void PlacePlayerForGoal(Goal* goal);
    // Places the other playable characters at their checkpoints of `goal`
    // (0x002DE794).
    void PlaceCompanionsForGoal(Goal* goal);
    // Makes `goal` the tracked goal: the goals before it are completed, the
    // players are placed (unless `keepPlayers`), the player switches to the
    // goal's character and, with `requestLevel`, the goal's level is asked
    // for (0x002DEA50).
    void SelectGoal(Goal* goal, bool keepPlayers, bool requestLevel);
    // Resets every goal of the running job (from the save words with
    // `fromSave`, restoring the initial flags with `restoreFlags`) and
    // tracks the goal after the last completed one; a pending goal id is
    // selected instead (0x002DEDB4).
    void RestoreGoals(bool fromSave, bool restoreFlags);
    // Records in the profile that `job` starts: earlier episodes (when the
    // job's episode is active) and their jobs count as finished, the job's
    // episode is started at the job's stage and its job states updated
    // (0x002E0008).
    void ApplyJobStart(Episode* job);
    // Makes `goal` the one to play: in its level (now, unless `forceReload`
    // asks to load it again) it is selected at once; otherwise the goals are
    // reset and the level is (re)loaded with the goal selected afterwards
    // (0x002E0498).
    void PlayGoal(Goal* goal, bool forceReload, bool levelRequestFlag);
    // Builds the running job's goals and, with `track`, plays the goal named
    // by the pending id (else the first goal) (0x002E06D8).
    bool BuildRunningJobGoals(bool forceReload, bool levelRequestFlag, bool track);
    // Selects the goal whose name hash is `nameHash` (null for 0)
    // (0x002E56F4).
    void SelectGoalByName(uint32_t nameHash, bool keepPlayers, bool requestLevel);
    // Starts `job` from the hub: the profile's pending counters are taken
    // over, the running job ends, the start is recorded (unless
    // `skipBookkeeping`) and either the episode's hub / hideout level is
    // asked for or the job's goals are built right away (tracking
    // `goalName`); the "INITJOB" script runs afterwards (0x002E0930).
    // Completes the running job: its state and rewards, the episode's next
    // stage (or the episode itself) unless it is a replay, then ends it.
    // Returns the level to go to next (0x002E5414).
    uint32_t CompleteRunningJob();
    bool StartJobFromHub(Episode* job, uint32_t goalName, bool toHub, bool toHideout, bool forceReload,
                         bool levelRequestFlag, bool skipBookkeeping, bool track);
    // Ends the running job instance, first updating its episode's flag 0x20
    // (0x002E5060).
    void FinishJobInstance();
    // The job with value `value` (0: the current job) has its episode's
    // flag 0x20; the running level's own job counts as flagged (0x002E2740).
    bool IsJobValueFlagged(uint32_t value) const;

    int32_t pendingKind = 0;         // +0x00 1: a job start is pending
    entity::Entity* pendingEntity = nullptr;   // +0x04 the entity that asked (class bit 17 set)
    Episode* pendingJob = nullptr;   // +0x08 job whose files are streamed for the start
    uint32_t pendingValue = 0;       // +0x0C
    Job* jobs = nullptr;             // +0x14
    uint32_t jobCount = 0;           // +0x18
    Episode* episodes = nullptr;     // +0x1C
    uint32_t episodeCount = 0;       // +0x20
    Job* currentJob = nullptr;       // +0x24
    Episode* instance = nullptr;     // +0x28 the running job
    uint32_t streamFlags = 0;        // +0x10 bit 29: a start is pending; bits 31/30 its two options
    Episode* lastFinished = nullptr; // +0x2C the job that ended last
    uint32_t field_30 = 0;           // +0x30
    uint32_t collectedBottles = 0;   // +0x38 of the current episode, counted at level start
    uint32_t collectedMasks = 0;     // +0x3C over every active episode
    uint32_t field_40 = 0;           // +0x40
    entity::Entity* field_44 = nullptr;   // +0x44 NOTE: an entity whose script is told when a job ends
    Goal* trackedGoal = nullptr;     // +0x48
    uint32_t field_4C = 0;           // +0x4C
    uint32_t field_50 = 0;           // +0x50
    uint32_t field_54 = 0;           // +0x54
    uint32_t pendingRequest[2] = {}; // +0x58
    uint32_t trackedGoalId = 0;      // +0x60 goal to track after loading, 0 = first open goal
    uint32_t flags = 0;              // +0x64 bit 29: a request is pending, bit 30: goals being cleared

    // Removes and deletes every goal of the player's goal list (hiding the
    // HUD counter of an active goal that shows one) and drops the tracked
    // goal (0x002DDAB0).
    void ClearGoals();

private:
    void ResetRunningJob();
};

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
// The player's goal with id `id` (0x00465F30 on the players block), or null.
Goal* PlayerFindGoal(uint32_t id);
// Asks the game world for level `levelName` (0x004674C4).
void GameWorldRequestLevel(uint32_t levelName);
// Puts the game world's state machine into state 3 (0x00488D68).
void GameWorldRestartState();
// The local player's block (game world +0x1070): character slot (+0x02),
// controlled entity (+0x08) and its spawn transform (+0x10).
int PlayerCharacterSlot();
entity::Entity* PlayerEntity();
ape::Matrix34& PlayerSpawnTransform();
// The playable character entities by slot (game world +0x1280F0, nine).
entity::Entity* PlayableCharacter(int slot);
// Default spawn transform of character slot `slot` (0x00486460).
void DefaultSpawnTransform(int slot, ape::Matrix34* out);
// World transform of a placed point instance; false if there is none.
bool PointInstanceWorldTransform(uint32_t name, ape::Matrix34* out);
// A one-shot respawn point set by scripts (game world +0x128160); taking it
// clears it. False if none is set or the point is missing.
bool TakeRespawnOverride(ape::Matrix34* out, bool* wasSet);
// Character entity snaps to the ground after a teleport (0x0012BF70).
void CharacterSnapToGround(entity::Entity* character);
// The active game camera resets behind the player (game world +0x1281E0,
// camera of slot +0x159, virtual slot 7).
void GameCameraReset();
// Game world helpers of the goal selection: the player switches to the
// goal's associated player (0x00487670), to the character of definition
// `def` (0x00484BE0), or respawns as it is (0x00488178).
void GameWorldUseAssociatedPlayer(Goal* goal);
void GameWorldSwitchPlayerCharacter(uint32_t def);
void GameWorldRespawnPlayer();
// Profile byte of player `player`'s block (game world +0x1070 + player *
// 0x20C80).
uint8_t PlayerBlockProfile(int player);
// Per-profile values the game world keeps for the profile record: a
// pending count (+0x1028 + profile) and a pair of flags (+0x1029 +
// profile * 2).
uint8_t& ProfilePendingCount(uint8_t profile);
uint8_t* ProfileNotifyPair(uint8_t profile);
// Running level (game world +0x128114).
uint32_t RunningLevel();
// Reloads the running level (0x004896F8 with 0).
void GameWorldReloadLevel();
// Goal `index` of the player's goal list (game world +0x10C0, count
// +0x10C8), null for -1 or out of range (0x002E6A3C).
Goal* PlayerGoalAt(uint32_t index);

// Sets the unlock flag of the unlockable whose key is `key` (0x002E2874).
void UnlockByKey(uint32_t key);
// Movie unlockable (<MovieUnlockable>, 0x70 bytes).
struct Unlockable {
    char movieName[0x20] = {};       // +0x00
    char movieFile[0x40] = {};       // +0x20
    uint32_t key = 0;                // +0x60 movieFile hash
    bool defaultUnlocked = false;    // +0x64
    uint32_t group = 0;              // +0x68 0 cinematic, 1 NOTE: hash 0xF9AB1627
    int32_t index = -1;              // +0x6C load order
};
// Movie entry (element 0x5B5D34CD, 0x28 bytes) keyed by movieFile hash.
// NOTE: the element and the two integers are not named.
struct MovieEntry {
    int32_t field_00 = 0;            // NOTE: attribute 0xBF873E4B
    int32_t field_04 = 0;            // NOTE: attribute 0x95E16172
    char text[0x20] = {};            // +0x08
};
ape::ApeTSortedMap<MovieEntry*>& MovieEntries();     // 0x010681E4
// Treasure / safe record of the spawner that placed `entity` (its spawn
// point's key), or null (0x002E696C / 0x002E69D4).
CollectibleRecord* TreasureRecordOf(const entity::Entity* entity);
CollectibleRecord* SafeRecordOf(const entity::Entity* entity);
// Movie entry of a movie file name hash, or null (0x002E2D44).
MovieEntry* FindMovieEntry(uint32_t key);
// Job value of the running level: the level's own, else (while the
// current episode runs a job) the episode's +0x32C; with no level, the
// profile's running job value or the default (0x002E29B0).
uint32_t RunningLevelJobValue();
// 0x010683D4: job value used when nothing else names one. NOTE: written
// outside this module.
extern uint32_t g_defaultJobValue;
// Index of each entity named by the element 0x9A5AAD6E (load order).
ape::ApeTSortedMap<uint8_t*>& EpisodeEntityIndices(); // 0x010681CC
// Unlockables (array 0x010681FC, count 0x01068204; key at record +0x60).
uint32_t UnlockableCount();
uint32_t UnlockableKey(uint32_t index);
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
