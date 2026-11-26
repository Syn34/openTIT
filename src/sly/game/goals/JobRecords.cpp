// openTIT - The job manager's 0x35C-byte records: per-profile progress,
// collectible counts and the completion formula (0x002DA734-0x002DB8B0).
#include "sly/game/Upgrades.h"
#include "sly/game/goals/JobManager.h"

#include "ape/core/ApeString.h"
#include "sly/entity/Awards.h"
#include "sly/entity/Entity.h"
#include "sly/game/GameEvent.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/WorldInterface.h"
#include "sly/game/goals/Goal.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/game/hud/HudManager.h"

#include "ape/anim/ApeAnimScript.h"

#include <cstdio>
#include "sly/tuning/GoalTuning.h"

#include <cstring>

namespace sly {
namespace game {

namespace {

constexpr uint32_t kNoIndex = 0xFFFFFFFFu;

ape::ApeTSortedMap<CollectibleRecord*> g_masks(0x10);      // 0x0106819C
ape::ApeTSortedMap<CollectibleRecord*> g_treasures(0x10);  // 0x01068184
ape::ApeTSortedMap<CollectibleRecord*> g_safes(0x10);      // 0x010681B4

int CountBits(uint32_t bits)
{
    int count = 0;
    for (; bits != 0; bits &= bits - 1)
        ++count;
    return count;
}

// Records of `table` that belong to the episode at job manager index `index`.
int CountRecordsOf(ape::ApeTSortedMap<CollectibleRecord*>& table, uint32_t index)
{
    int count = 0;
    auto* entries = table.Data();
    for (uint16_t i = 0; i < table.Count(); ++i) {
        if (entries[i].value->episode == index)
            ++count;
    }
    return count;
}

// Job manager index of the record with id `id`, or -1.
uint32_t IndexOfId(uint32_t id)
{
    JobManager* manager = GetJobManager();
    return manager ? manager->JobIndexById(id) : kNoIndex;
}

// The bottle word index is checked against the record's twelve slots.
bool ValidBottleSlot(uint8_t slot)
{
    if (slot < PlayerProgress::kEpisodeSlots)
        return true;
    ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer increase "
                    "the size of the array.",
                    slot, PlayerProgress::kEpisodeSlots);
    return false;
}

} // namespace

int g_defaultBottleCount = 30;
// Masks counted for the mask counter's total (0x010682A4). NOTE: written by
// code that is not ported yet; 0 shows the count alone.
int g_maskCounterTotal = 0;

ape::ApeTSortedMap<CollectibleRecord*>& MaskTable() { return g_masks; }
ape::ApeTSortedMap<CollectibleRecord*>& TreasureTable() { return g_treasures; }
ape::ApeTSortedMap<CollectibleRecord*>& SafeTable() { return g_safes; }

void UnlockJobRewards(uint32_t jobId) { UnlockUpgradesOfJob(jobId); }

// @func 0x002D9740 Episode::Episode
Episode::Episode()
    : scriptPaths(4)
    , texts(0x10)
    , characters(8)
{
    name[0] = '\0';
    desc[0] = '\0';
    jobScript[0] = '\0';
    movieOnJobComplete[0] = '\0';
    abbrev[0] = '\0';
}

// @func 0x002D98A4 Episode::State
uint8_t Episode::State() const
{
    return CurrentProgress().jobStates[index];
}

// @func 0x002D98DC Episode::StoreText
void Episode::StoreText(const TextField& text)
{
    for (TextField& stored : texts) {
        if (stored.id == text.id) {
            std::memcpy(stored.text, text.text, sizeof(stored.text));
            stored.flagB = true;
            return;
        }
    }
}

// @func 0x002D9994 Episode::LoadText
bool Episode::LoadText(TextField& text) const
{
    for (const TextField& stored : texts) {
        if (stored.id == text.id) {
            text = stored;
            return true;
        }
    }
    return false;
}

// @func 0x002DA734 Job::Job
Job::Job()
{
    name[0] = '\0';
    desc[0] = '\0';
    timePeriod[0] = '\0';
    startMovie[0] = '\0';
    endMovie[0] = '\0';
    endMovie2[0] = '\0';
    endMovie3[0] = '\0';
    abbrev[0] = '\0';
    numBottles = static_cast<uint8_t>(g_defaultBottleCount);
    jobWeight = g_gameEpisodeCompletionFormulaJobWeight;
    maskWeight = g_gameEpisodeCompletionFormulaMaskWeight;
    bottleWeight = g_gameEpisodeCompletionFormulaBottleWeight;
    treasureWeight = g_gameEpisodeCompletionFormulaTreasureWeight;
    safeWeight = g_gameEpisodeCompletionFormulaSafeWeight;
}

EpisodeProgress& Job::Progress() const
{
    return CurrentProgress().episodes[episode];
}

// @func 0x002DA85C Job::ProgressState
int Job::ProgressState() const
{
    return (Progress().flags >> 3) & 3;
}

// @func 0x002DA89C Job::IsCompleted
bool Job::IsCompleted() const
{
    return (Progress().flags & 0x40) != 0;
}

// @func 0x002DA8E8 Job::SetFlag100
void Job::SetFlag100(bool on)
{
    uint16_t& flags = Progress().flags;
    flags = on ? (flags | 0x100) : (flags & ~0x100);
}

// @func 0x002DA944 Job::Flag100
bool Job::Flag100() const
{
    return (Progress().flags & 0x100) != 0;
}

// @func 0x002DA990 Job::SafeState
int Job::SafeState() const
{
    return Progress().flags & 7;
}

// @func 0x002DA9D0 Job::CollectedTreasures
int Job::CollectedTreasures() const
{
    if (bitGroup == 0xFF)
        return 0;
    return CountBits(CurrentProgress().jobBits[bitGroup]);
}

// @func 0x002DAA54 Job::TreasureCount
int Job::TreasureCount() const
{
    return CountRecordsOf(g_treasures, IndexOfId(field_2D4));
}

// @func 0x002DAB04 Job::CollectedBottles
int Job::CollectedBottles() const
{
    if (bottleSlot == 0xFF || !ValidBottleSlot(bottleSlot))
        return 0;
    return CountBits(CurrentProgress().bottleBits[bottleSlot]);
}

// @func 0x002DABCC JobManager::FindJobById
Job* JobManager::FindJobById(uint32_t id) const
{
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].field_2D4 == id)
            return &jobs[i];
    }
    return nullptr;
}

uint32_t JobManager::JobIndexById(uint32_t id) const
{
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].field_2D4 == id)
            return i;
    }
    return kNoIndex;
}

// @func 0x002DAC24 Job::CollectedMasks
// The masks found here count, and so do this episode's masks found in
// another episode, as far as that episode's mask word has their bit.
int Job::CollectedMasks(const PlayerProgress& progress) const
{
    if (maskSlot == 0xFF)
        return 0;
    int count = CountBits(progress.maskBits[maskSlot]);

    const uint32_t index = IndexOfId(field_2D4);
    JobManager* manager = GetJobManager();
    auto* entries = g_masks.Data();
    for (uint16_t i = 0; i < g_masks.Count(); ++i) {
        const CollectibleRecord* mask = entries[i].value;
        if (mask->episode != index || mask->field_08 == 0 || mask->foundIn == 0)
            continue;
        const Job* other = manager ? manager->FindJobById(mask->foundIn) : nullptr;
        if (!other)
            continue;
        CollectibleRecord** found = g_masks.Find(mask->key);
        if (*found && (progress.maskBits[other->maskSlot] & (1u << ((*found)->bit & 0x3F))) != 0)
            ++count;
    }
    return count;
}

// @func 0x002DAE5C Job::CollectedMasks
int Job::CollectedMasks() const
{
    return CollectedMasks(CurrentProgress());
}

// @func 0x002DAEA0 Job::MaskCount
int Job::MaskCount() const
{
    return CountRecordsOf(g_masks, IndexOfId(field_2D4));
}

// @func 0x002DAF54 Job::Flag80
bool Job::Flag80() const
{
    return (Progress().flags & 0x80) != 0;
}

// @func 0x002DAFA0 Job::SetFlag80
void Job::SetFlag80(bool on)
{
    uint16_t& flags = Progress().flags;
    flags = on ? (flags | 0x80) : (flags & ~0x80);
}

// @func 0x002DAFFC Job::CurrentStage
uint8_t Job::CurrentStage() const
{
    return Progress().stage;
}

// @func 0x002DB040 Job::Completion
// Each part adds its weight to the total and its collected share of the
// weight to the score; the safe counts in full once opened.
float Job::Completion(const PlayerProgress& progress) const
{
    float total = 0.0f;
    float score = 0.0f;

    if (stageCount != 0) {
        int done = 0;
        for (uint8_t i = 0; i < stageCount; ++i) {
            if (progress.jobStates[stages[i].index] == kJobState_Complete)
                ++done;
        }
        total += jobWeight;
        score += (static_cast<float>(done) / stageCount) * jobWeight;
    }

    const uint32_t index = IndexOfId(field_2D4);
    const int masks = CountRecordsOf(g_masks, index);
    if (masks != 0) {
        total += maskWeight;
        score += static_cast<float>(static_cast<double>(CollectedMasks(progress)) / masks) * maskWeight;
    }

    if (numBottles != 0) {
        int bottles = 0;
        if (bottleSlot != 0xFF && ValidBottleSlot(bottleSlot))
            bottles = CountBits(progress.bottleBits[bottleSlot]);
        total += bottleWeight;
        score += (static_cast<float>(bottles) / numBottles) * bottleWeight;
    }

    const int treasures = CountRecordsOf(g_treasures, index);
    if (treasures != 0) {
        const int found = bitGroup == 0xFF ? 0 : CountBits(progress.jobBits[bitGroup]);
        const float safe = (progress.episodes[episode].flags & 7) == 3 ? safeWeight : 0.0f;
        score += (static_cast<float>(found) / treasures) * treasureWeight + safe;
        total += treasureWeight + safeWeight;
    }
    return (score / total) * 100.0f;
}

// @func 0x002DB5D4 Job::Completion
float Job::Completion() const
{
    return Completion(CurrentProgress());
}

// @func 0x002DB618 Job::IsStageComplete
bool Job::IsStageComplete() const
{
    if (!stages)
        return true;
    const PlayerProgress& progress = CurrentProgress();
    const uint8_t stage = progress.episodes[episode].stage;
    if (stage == 0xFF)
        return true;
    for (uint8_t i = 0; i < stageCount; ++i) {
        if (stages[i].order == stage && progress.jobStates[stages[i].index] != kJobState_Complete)
            return false;
    }
    return true;
}

// @func 0x002DB70C Job::UpdateJobStates
bool Job::UpdateJobStates() const
{
    if (!stages)
        return false;
    PlayerProgress& progress = CurrentProgress();
    const uint8_t stage = progress.episodes[episode].stage;
    if (stage == 0xFF)
        return false;

    bool anyInStage = false;
    for (uint8_t i = 0; i < stageCount; ++i) {
        const Episode& job = stages[i];
        uint8_t& state = progress.jobStates[job.index];
        if (job.order == stage) {
            if (state != kJobState_Complete)
                state = kJobState_Available;
            anyInStage = true;
        } else if (state == kJobState_Complete || job.order < stage) {
            state = kJobState_Complete;
            UnlockJobRewards(job.nameHash);
        }
    }
    return anyInStage;
}

// @func 0x002DB8B0 Job::Start
void Job::Start()
{
    EpisodeProgress& progress = Progress();
    if (((progress.flags >> 3) & 3) == 2)
        return;
    progress.flags = static_cast<uint16_t>((progress.flags & ~0x18) | 0x08);
    progress.stage = 0;
    UpdateJobStates();
}

// ---------------------------------------------------------------------------
// Queries over the current episode and the whole game (0x002E16E8-0x002E3A70)

namespace {

// Characters that also answer to a second name (the variant of the same
// character used by some jobs).
bool CharacterMatches(uint32_t jobCharacter, uint32_t character)
{
    struct Alias { uint32_t name, variant; };
    static constexpr Alias kAliases[] = {
        {0x1D8C2817u, 0x9AA76568u},   // "Bentley"
        {0x00014120u, 0x1D5B6571u},   // "Sly"
        {0xDF0611F9u, 0xB4D78147u},   // "CarmelitaFox"
        {0x88397820u, 0xC92661E2u},   // "Murray"
    };
    for (const Alias& alias : kAliases) {
        if (jobCharacter == alias.name)
            return character == alias.name || character == alias.variant;
    }
    return jobCharacter == character;
}

const PlayerProgress& ProgressOrCurrent(const PlayerProgress* progress)
{
    return progress ? *progress : CurrentProgress();
}

} // namespace

uint32_t g_defaultJobValue = 0;

// @func 0x002E1900 JobManager::CurrentStageJobForCharacter
Episode* JobManager::CurrentStageJobForCharacter(uint32_t character) const
{
    const Job* job = currentJob;
    if (!job || character == 0)
        return nullptr;
    const PlayerProgress& progress = CurrentProgress();
    const uint8_t stage = progress.episodes[job->episode].stage;
    for (uint8_t i = 0; i < job->stageCount; ++i) {
        Episode& candidate = job->stages[i];
        if (progress.jobStates[candidate.index] == kJobState_Complete || candidate.order != stage)
            continue;
        if (candidate.associatedDef != 0 && CharacterMatches(candidate.associatedDef, character))
            return &candidate;
    }
    return nullptr;
}

// @func 0x002E16E8 JobManager::CurrentStageUsesCharacter
bool JobManager::CurrentStageUsesCharacter(uint32_t character) const
{
    return CurrentStageJobForCharacter(character) != nullptr;
}

// @func 0x002E1B1C JobManager::HideoutCharacterSelectable
// A finished episode still offers the characters of its last stage.
bool JobManager::HideoutCharacterSelectable(uint32_t name) const
{
    const Job* job = currentJob;
    if (!job)
        return true;
    const EpisodeProgress& progress = CurrentProgress().episodes[job->episode];
    uint32_t stage = progress.stage;
    if (((progress.flags >> 3) & 3) == 2 && stage != 0)
        --stage;
    for (uint8_t i = 0; i < job->stageCount; ++i) {
        const Episode& candidate = job->stages[i];
        if (candidate.order != stage)
            continue;
        for (const Episode::HideoutCharacter& character : candidate.characters) {
            if (character.name == name)
                return character.selectable;
        }
    }
    return false;
}

// @func 0x002E1C88 JobManager::HideoutCharacterFlag09
bool JobManager::HideoutCharacterFlag09(uint32_t name) const
{
    const Job* job = currentJob;
    if (!job)
        return false;
    const uint8_t stage = CurrentProgress().episodes[job->episode].stage;
    for (uint8_t i = 0; i < job->stageCount; ++i) {
        const Episode& candidate = job->stages[i];
        if (candidate.order != stage)
            continue;
        for (const Episode::HideoutCharacter& character : candidate.characters) {
            if (character.name == name)
                return character.field_09;
        }
    }
    return false;
}

// @func 0x002E2438 JobManager::CollectHideoutCharacters
void JobManager::CollectHideoutCharacters(ape::ApeTArray<Episode::HideoutCharacter>& out) const
{
    const Job* job = LastStartedJob();
    if (!job)
        return;
    const PlayerProgress& progress = CurrentProgress();
    for (uint8_t i = job->stageCount; i > 0; --i) {
        const Episode& candidate = job->stages[i - 1];
        const uint8_t state = progress.jobStates[candidate.index];
        if (state == kJobState_Locked)
            continue;
        for (const Episode::HideoutCharacter& character : candidate.characters) {
            if (!character.field_0A || state == kJobState_Complete)
                out.PushBack(character);
        }
        return;
    }
}

// @func 0x002E29B0 RunningLevelJobValue
uint32_t RunningLevelJobValue()
{
    const PlayerProgress& progress = CurrentProgress();
    const uint32_t profileValue = progress.field_A0 ? progress.field_A0 : g_defaultJobValue;
    uint32_t value = 0;
    if (!CurrentLevelJobValue(&value))
        return profileValue;
    if (value != 0)
        return value;
    JobManager* manager = GetJobManager();
    const Job* job = manager ? manager->currentJob : nullptr;
    if (!job)
        return profileValue;
    if (progress.episodes[job->episode].flags & 0x20)
        return job->field_32C;
    return 0;
}

// @func 0x002E2B34 JobManager::MarkEpisodeEntityFound
void JobManager::MarkEpisodeEntityFound(uint32_t entity) const
{
    constexpr uint32_t kTrophy_FirstFound = 0xC3FB61E2u;
    const Job* job = CurrentEpisode();
    if (!job)
        return;
    if (job->episode >= PlayerProgress::kEpisodeSlots) {
        ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer increase "
                        "the size of the array.",
                        job->episode, PlayerProgress::kEpisodeSlots);
        return;
    }
    if (entity == 0)
        return;
    uint8_t** index = EpisodeEntityIndices().Find(entity);
    if (!index || !*index)
        return;
    PlayerProgress& progress = CurrentProgress();
    progress.entityBits[job->episode] |= (uint8_t)(1u << (**index & 0x3F));
    int found = 0;
    for (uint32_t i = 0; i < jobCount; ++i)
        found += CountBits(progress.entityBits[jobs[i].episode]);
    if (found == 1)
        AwardTrophyByName(kTrophy_FirstFound);
}

// @func 0x002E3188 JobManager::GameCompletion
float JobManager::GameCompletion(const PlayerProgress* progress) const
{
    const PlayerProgress& record = ProgressOrCurrent(progress);
    float weights = 0.0f;
    float score = 0.0f;
    for (uint32_t i = 0; i < jobCount; ++i) {
        weights += jobs[i].episodeWeight;
        score += jobs[i].Completion(record) * jobs[i].episodeWeight * 0.01f;
    }
    return weights > 0.0f ? (score / weights) * 100.0f : 0.0f;
}

// @func 0x002E32D8 JobManager::BottlePercent
float JobManager::BottlePercent(const PlayerProgress* progress) const
{
    const PlayerProgress& record = ProgressOrCurrent(progress);
    uint32_t total = 0;
    uint32_t found = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        const Job& job = jobs[i];
        if (job.bottleSlot != 0xFF && ValidBottleSlot(job.bottleSlot))
            found += CountBits(record.bottleBits[job.bottleSlot]);
        total += job.numBottles;
    }
    return total ? (static_cast<float>(found) / total) * 100.0f : 0.0f;
}

// @func 0x002E34A8 JobManager::MaskPercent
float JobManager::MaskPercent(const PlayerProgress* progress) const
{
    const PlayerProgress& record = ProgressOrCurrent(progress);
    uint32_t total = 0;
    uint32_t found = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        const Job& job = jobs[i];
        if (!job.active)
            continue;
        found += job.CollectedMasks(record);
        total += CountRecordsOf(g_masks, IndexOfId(job.field_2D4));
    }
    return total ? (static_cast<float>(found) / total) * 100.0f : 0.0f;
}

// @func 0x002E3698 JobManager::TreasurePercent
float JobManager::TreasurePercent(const PlayerProgress* progress) const
{
    const PlayerProgress& record = ProgressOrCurrent(progress);
    uint32_t total = 0;
    uint32_t found = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        const Job& job = jobs[i];
        if (job.bitGroup != 0xFF)
            found += CountBits(record.jobBits[job.bitGroup]);
        total += CountRecordsOf(g_treasures, IndexOfId(job.field_2D4));
    }
    return total ? (static_cast<float>(found) / total) * 100.0f : 0.0f;
}

// @func 0x002E3898 JobManager::SafePercent
float JobManager::SafePercent(const PlayerProgress* progress) const
{
    const PlayerProgress& record = ProgressOrCurrent(progress);
    uint32_t withSafe = 0;
    uint32_t opened = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        const Job& job = jobs[i];
        if (CountRecordsOf(g_treasures, IndexOfId(job.field_2D4)) == 0)
            continue;
        ++withSafe;
        if ((record.episodes[job.episode].flags & 7) == 3)
            ++opened;
    }
    return withSafe ? (static_cast<float>(opened) / withSafe) * 100.0f : 0.0f;
}

// @func 0x002E57A8 JobManager::TotalMaskCount
int JobManager::TotalMaskCount() const
{
    int count = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].active)
            count += CountRecordsOf(g_masks, IndexOfId(jobs[i].field_2D4));
    }
    return count;
}

// @func 0x002E4ED0 JobManager::IsJobCharacter
bool JobManager::IsJobCharacter(const entity::Entity* entity, const Episode* job) const
{
    constexpr uint64_t kClass_SlyDisguise = 0x20;   // NOTE: Entity::classBits bit 5
    if (!entity || !entity->IsAlive())
        return false;
    const uint32_t name = entity->def ? entity->def->nameHash : 0;
    if (name == 0 || !job || job->associatedDef == 0)
        return false;
    if (job->associatedDef == 0x00014120u && (entity->classBits & kClass_SlyDisguise))
        return true;
    return CharacterMatches(job->associatedDef, name);
}

// @func 0x002E5180 JobManager::OnGameEvent
void JobManager::OnGameEvent(int player, const GameEvent& e)
{
    (void)player;
    constexpr uint32_t kFlag_ClearingGoals = 1u << 30;
    constexpr uint32_t kGroup_Penguin = 0x021D9F0Cu;        // "Penguin"
    constexpr uint32_t kHit_Projectile = 0xA77EB5E9u;       // "Projectile"
    constexpr uint32_t kCharacter_Carmelita = 0xDF0611F9u;
    constexpr uint32_t kTrophy_Penguins = 0xADE2B55Bu;
    constexpr uint64_t kClass_Player = 0x1;

    if (!(flags & kFlag_ClearingGoals)) {
        if (e.type == kEvent_03)
            lastEventEntity = reinterpret_cast<entity::Entity*>(e.arg0);
        for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
            Goal* goal = PlayerGoalListAt(i);
            if ((goals::JobGoalSaveWord(goal->job, goal->saveSlot) >> 28) == goals::kGoal_Active && IsGoalLive(goal))
                goal->Update(0, e);
        }
        lastEventEntity = nullptr;
    }

    if (e.type != kEvent_01 || e.groupId != kGroup_Penguin || e.arg0 == 0)
        return;
    const auto* shooter = reinterpret_cast<const entity::Entity*>(e.arg0);
    if (!(shooter->classBits & kClass_Player))
        return;
    const uint32_t name = shooter->def ? shooter->def->nameHash : 0;
    if (name != kCharacter_Carmelita || e.arg1 != kHit_Projectile)
        return;
    PlayerProgress& progress = CurrentProgress();
    if (progress.penguinsShot == 0xFF)
        return;
    ++progress.penguinsShot;
    if (progress.penguinsShot >= g_gameEpisodePenguinAwardNumPenguins)
        AwardTrophyByName(kTrophy_Penguins);
}

// @func 0x002E6A88 JobManager::RecordArcadeScore
void JobManager::RecordArcadeScore(int table, int rank) const
{
    constexpr uint32_t kTrophy_FirstHighScore = 0x08047E80u;   // "HIGHSCOREFirst"
    constexpr uint32_t kTrophy_AllHighScores = 0x2480DD71u;    // "HIGHSCOREAll"
    if (rank >= g_gameEpisodeArcadeAwardScoreIndex)
        return;
    PlayerProgress& progress = CurrentProgress();
    progress.arcade[table].beaten = 1;
    uint32_t beaten = 0;
    for (const PlayerProgress::ArcadeScores& scores : progress.arcade)
        beaten += scores.beaten;
    if (beaten != 0)
        AwardTrophyByName(kTrophy_FirstHighScore);
    if (beaten > 5)
        AwardTrophyByName(kTrophy_AllHighScores);
}

namespace {
CollectibleRecord* RecordOfSpawnPoint(ape::ApeTSortedMap<CollectibleRecord*>& table, const entity::Entity* entity)
{
    if (!entity || !entity->field_DC)
        return nullptr;
    const auto* spawner = static_cast<const Spawner*>(entity->field_DC);
    const uint32_t key = spawner->spawnPoint ? WorldObjectKey(spawner->spawnPoint) : 0;
    if (key == 0)
        return nullptr;
    CollectibleRecord** record = table.Find(key);
    return record ? *record : nullptr;
}
} // namespace

// @func 0x002E696C TreasureRecordOf
CollectibleRecord* TreasureRecordOf(const entity::Entity* entity) { return RecordOfSpawnPoint(g_treasures, entity); }

// @func 0x002E69D4 SafeRecordOf
CollectibleRecord* SafeRecordOf(const entity::Entity* entity) { return RecordOfSpawnPoint(g_safes, entity); }

// @func 0x002E6320 JobManager::OnTreasureCollected
void JobManager::OnTreasureCollected(Job* job) const
{
    constexpr uint32_t kTrophy_Treasures = 0xA2474384u;   // NOTE: award name unknown
    PlayerProgress& progress = CurrentProgress();
    auto collectedOf = [&](const Job& j) { return j.bitGroup == 0xFF ? 0 : CountBits(progress.jobBits[j.bitGroup]); };
    auto countOf = [](const Job& j) { return CountRecordsOf(g_treasures, IndexOfId(j.field_2D4)); };

    int collected = 0;
    int total = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        collected += collectedOf(jobs[i]);
        total += countOf(jobs[i]);
    }
    if (job) {
        const int count = countOf(*job);
        if (count != 0 && count <= collectedOf(*job))
            progress.episodes[job->episode].flags |= 0x40;
    }
    if (collected >= g_gameEpisodeTreasureAwardNumTreasures)
        AwardTrophyByName(kTrophy_Treasures);
    if (total > 0 && total <= collected) {
        for (uint32_t i = 0; i < jobCount; ++i)
            progress.episodes[jobs[i].episode].flags |= 0x40;
    }
    CheckEpisodeTrophies();
}

// @func 0x002E6B8C JobManager::MaskSpawnerAvailable
bool JobManager::MaskSpawnerAvailable(const Spawner* spawner, uint8_t profile) const
{
    if (!spawner)
        return false;
    const uint32_t key = spawner->spawnPoint ? WorldObjectKey(spawner->spawnPoint) : 0;
    if (key != 0) {
        CollectibleRecord** found = g_masks.Find(key);
        const CollectibleRecord* mask = found ? *found : nullptr;
        if (mask && (mask->field_08 == 0 || mask->foundIn == 0)) {
            const Job& job = jobs[mask->episode];
            if (job.maskSlot == 0xFF)
                return false;
            if (job.maskSlot >= PlayerProgress::kEpisodeSlots) {
                ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer "
                                "increase the size of the array.",
                                job.maskSlot, PlayerProgress::kEpisodeSlots);
                return false;
            }
            return (ProfileProgress(profile).maskBits[job.maskSlot] & (1u << (mask->bit & 0x3F))) == 0;
        }
    }
    ape::TempFormat("Couldn't find mask spawner '%s' in the mask info map! Please have an engineer check.",
                    spawner->spawnPoint ? WorldObjectName(spawner->spawnPoint) : "");
    return false;
}

// @func 0x002E6E54 JobManager::SetGoalCountersSuppressed
void JobManager::SetGoalCountersSuppressed(bool suppressed)
{
    constexpr uint32_t kFlag_CountersSuppressed = 1u << 27;
    if (!suppressed) {
        for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
            Goal* goal = PlayerGoalListAt(i);
            if ((goals::JobGoalSaveWord(goal->job, goal->saveSlot) >> 28) != goals::kGoal_Active)
                continue;
            goals::GoalProgress counter;
            if (goal->GetProgress(&counter) && counter.name != 0 && hud::Hud().objects.Find(counter.name) &&
                *hud::Hud().objects.Find(counter.name))
                hud::ShowByName(counter.name, true, true, false);
        }
    }
    flags = suppressed ? (flags | kFlag_CountersSuppressed) : (flags & ~kFlag_CountersSuppressed);
}

// @func 0x002E66F4 JobManager::CollectTreasure
void JobManager::CollectTreasure(entity::Entity* treasure, entity::Entity* collector) const
{
    constexpr uint64_t kClass_Player = 0x1;
    constexpr uint32_t kNotify_TreasureFound = 1u << 5;
    if (!treasure || !collector || !(collector->classBits & kClass_Player))
        return;
    auto* spawner = static_cast<Spawner*>(treasure->field_DC);
    if (!spawner)
        return;
    // NOTE: the top byte of the entity flags holds the player index.
    const int player = static_cast<int8_t>(collector->flags >> 24);
    PlayerProgress& progress = ProfileProgress(PlayerBlockProfile(player));
    Job* job = CurrentEpisode();
    if (job && job->bitGroup != 0xFF) {
        if (job->bitGroup >= PlayerProgress::kEpisodeSlots) {
            ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer increase "
                            "the size of the array.",
                            job->bitGroup, PlayerProgress::kEpisodeSlots);
        } else {
            const uint32_t key = spawner->spawnPoint ? WorldObjectKey(spawner->spawnPoint) : 0;
            CollectibleRecord** found = key ? g_treasures.Find(key) : nullptr;
            if (found && *found) {
                progress.jobBits[job->bitGroup] |= static_cast<uint16_t>(1u << ((*found)->bit & 0x3F));
                progress.notifyBits |= kNotify_TreasureFound;
            } else {
                ape::TempFormat("Couldn't find treasure '%s' with associated spawner '%s' in the treasure info map!",
                                ape::TempHexString(treasure->nameHash),
                                spawner->spawnPoint ? WorldObjectName(spawner->spawnPoint) : "");
            }
            OnTreasureCollected(currentJob);
        }
    }
    spawner->Deactivate(false);
}

// ---------------------------------------------------------------------------
// Profile record helpers (0x000ADE04-0x000AE4A4)

namespace {
// Globals copied into a fresh record. NOTE: 0x010B1804 is set at run time
// (meaning unknown); 0x00F65BF4/0x00F65BF5 hold 1.
uint8_t g_profileField000 = 0;
constexpr uint8_t kProfileField001 = 1;
constexpr uint8_t kProfileField002 = 1;

constexpr uint32_t kCostumeSlotNames[PlayerProgress::kCostumeSlots] = {
    0x0021C5E0u,   // "Head"
    0x001F31A2u,   // "Body"
    0x041555C4u,   // "hands"
    0x936FF31Du,   // NOTE: name unknown
    0x03C72BA1u,   // "Boots"
    0x03D69F9Cu,   // "Crook"
};

// Unlockable names (table 0x00F64494, 41 entries).
constexpr uint32_t kUnlockNames[] = {
    0xA70C9716u, 0xE3DEF6B5u, 0x467B8C48u, 0x2E01E5EBu, 0x95841725u, 0x2291A3C2u, 0xC201A536u,
    0x59CC1B15u, 0x74C12935u, 0xBF4B1637u, 0x350D543Bu, 0x63910843u, 0x716016C8u, 0x85EA7462u,
    0xDE07516Fu, 0x2986D493u, 0xF285665Bu, 0xC9A34D20u, 0xC25ED889u, 0xF4596EECu, 0x4A2AD4EAu,
    0xFCA16EEFu, 0xAEAE3F00u, 0xBC676503u, 0xACAA7E76u, 0x1D17219Bu, 0xFE492072u, 0xD44B7705u,
    0x29E1A78Cu, 0xA3907718u, 0xE3541252u, 0xF69E553Bu, 0x3FEEDCAFu, 0xFAC4F502u, 0xC7B84D81u,
    0x4192876Du, 0x1AD96E63u, 0x40556837u, 0x46C660F7u, 0xA0355EFBu, 0x0C44F02Du,
};

// Default arcade tables: five scores of the gang, with higher defaults for
// tables 6 to 11.
void ResetArcadeTable(PlayerProgress::ArcadeScores& table, int index)
{
    static const char* const kInitials[5] = {"BTLY", "SLYC", "MURR", "CARM", "DIMI"};
    for (int i = 0; i < 5; ++i) {
        table.scores[i] = 5000 - 1000 * i;
        std::memcpy(table.initials[i], kInitials[i], 4);
    }
    static const int32_t kTable6[5] = {25000, 20000, 18000, 15000, 10000};
    static const int32_t kTable7[5] = {20000, 18000, 15000, 12000, 10000};
    static const int32_t kTable8[5] = {4500, 4000, 3500, 3000, 2000};
    static const int32_t kTable9[5] = {4000, 3700, 3500, 3000, 2000};
    static const int32_t kTable10[5] = {1000, 800, 700, 600, 500};
    const int32_t* scores = nullptr;
    switch (index) {
    case 6: scores = kTable6; break;
    case 7: scores = kTable7; break;
    case 8: case 11: scores = kTable8; break;
    case 9: scores = kTable9; break;
    case 10: scores = kTable10; break;
    default: break;
    }
    if (scores)
        std::memcpy(table.scores, scores, sizeof table.scores);
    table.beaten = 0;
}
} // namespace

// @func 0x000ADE04 PlayerProgress::Reset
void PlayerProgress::Reset()
{
    *this = PlayerProgress();
    field_000 = g_profileField000;
    field_001 = kProfileField001;
    field_002 = kProfileField002;
    for (int i = 0; i < kCostumeSlots; ++i)
        costume[i].slot = kCostumeSlotNames[i];
    for (size_t i = 0; i < sizeof kUnlockNames / sizeof kUnlockNames[0]; ++i)
        unlocks[i].name = kUnlockNames[i];
    for (int i = 0; i < kArcadeTables; ++i)
        ResetArcadeTable(arcade[i], i);
}

// @func 0x000AE3FC PlayerProgress::AddPendingCount
void PlayerProgress::AddPendingCount(uint8_t& pending)
{
    field_144 = static_cast<uint8_t>(field_144 + pending);
    pending = 0;
}

// @func 0x000AE418 PlayerProgress::GetNotifyPair
void PlayerProgress::GetNotifyPair(uint8_t out[2]) const
{
    out[0] = (notifyBits & 0x8000u) != 0;
    out[1] = (notifyBits & 0x10000u) != 0;
}

// @func 0x000AE454 PlayerProgress::SetNotifyPair
void PlayerProgress::SetNotifyPair(const uint8_t in[2])
{
    notifyBits = in[0] == 1 ? (notifyBits | 0x8000u) : (notifyBits & ~0x8000u);
    notifyBits = in[1] == 1 ? (notifyBits | 0x10000u) : (notifyBits & ~0x10000u);
}

// @func 0x002DE258 JobManager::UpdateBottleHud
void JobManager::UpdateBottleHud(bool pickup)
{
    hud::HudObject* counter = hud::Find(hud::g_bottleCounterName);
    if (!counter)
        return;
    // NOTE: compared with the default bottle count, not the episode's own.
    const uint32_t total = uint32_t(g_defaultBottleCount);
    auto* script = static_cast<ape::ApeAnimScriptInstance*>(counter->animScript);
    if (script && total != 0 && total <= collectedBottles)
        script->SetStateAndUpdate(pickup ? 0x90A128CFu : 0x3143F570u);   // NOTE: state names unknown
    if (currentJob && currentJob->numBottles == 0)
        return;
    char text[32];
    const uint32_t fields = counter->def ? counter->def->textFieldCount : 0;
    if (fields < 2) {
        std::snprintf(text, sizeof text, "%d/%d", int(collectedBottles), int(total));
        counter->ShowText(-1.0f, text, true);
    } else {
        // Separate fields for the count and the total.
        std::snprintf(text, sizeof text, "%d", int(collectedBottles));
        counter->ShowText(-1.0f, text, true);
        std::snprintf(text, sizeof text, "%d", int(total));
        counter->SetFieldText(text, 1);
    }
}

// @func 0x002E5C68 JobManager::ShowMaskCount
void JobManager::ShowMaskCount(hud::HudObject* counter) const
{
    if (!counter)
        return;
    // NOTE: the console also passes an uninitialised register as the
    // ShowText flag; the default is used here.
    char text[32];
    const int total = g_maskCounterTotal;
    if (total == 0) {
        std::snprintf(text, sizeof text, "%d", int(collectedMasks));
        counter->ShowText(-1.0f, text, true);
    } else if (!counter->def || counter->def->textFieldCount < 2) {
        std::snprintf(text, sizeof text, "%d/%d", int(collectedMasks), total);
        counter->ShowText(-1.0f, text, true);
    } else {
        std::snprintf(text, sizeof text, "%d", int(collectedMasks));
        counter->ShowText(-1.0f, text, true);
        std::snprintf(text, sizeof text, "%d", total);
        counter->SetFieldText(text, 1);
    }
}

} // namespace game
} // namespace sly
