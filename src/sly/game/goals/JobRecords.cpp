// openTIT - The job manager's 0x35C-byte records: per-profile progress,
// collectible counts and the completion formula (0x002DA734-0x002DB8B0).
#include "sly/game/goals/JobManager.h"

#include "ape/core/ApeString.h"
#include "sly/tuning/GoalTuning.h"

#include <cstring>

namespace sly {
namespace game {

namespace {

constexpr uint32_t kNoIndex = 0xFFFFFFFFu;

ape::ApeTSortedMap<CollectibleRecord*> g_masks(0x10);      // 0x0106819C
ape::ApeTSortedMap<CollectibleRecord*> g_treasures(0x10);  // 0x01068184

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

ape::ApeTSortedMap<CollectibleRecord*>& MaskTable() { return g_masks; }
ape::ApeTSortedMap<CollectibleRecord*>& TreasureTable() { return g_treasures; }

// TODO(0x0049E038): sets the profile's unlock flags of the job's rewards.
void UnlockJobRewards(uint32_t) {}

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

} // namespace game
} // namespace sly
