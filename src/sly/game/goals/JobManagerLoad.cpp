// openTIT - Job manager construction and the episode list loader
// (0x002DB968-0x002DDD24).
#include "sly/game/goals/JobManager.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/WorldInterface.h"
#include "sly/game/goals/Goal.h"
#include "sly/levels/GameServices.h"

#include <vector>

namespace sly {
namespace game {

namespace {

using ape::xml::XmlDocument;
using ape::xml::XmlElement;

JobManager* g_jobManager = nullptr;  // 0x00F64C20

ape::ApeTSortedMap<MovieEntry*> g_movieEntries(0x10);     // 0x010681E4
ape::ApeTSortedMap<uint8_t*> g_entityIndices(0x10);       // 0x010681CC
std::vector<Unlockable*> g_unlockables;                   // 0x010681FC

constexpr uint32_t kSharedScriptLibrary = 0x7719A548u;

// Elements and attributes of Scripts/Episodes.xml and the episode files.
constexpr uint32_t kHash_EpisodeList = 0x37408E3Au;      // root. NOTE: name unknown
constexpr uint32_t kHash_MovieEntry = 0x5B5D34CDu;       // NOTE: name unknown
constexpr uint32_t kHash_MovieUnlockable = 0x1776D1AEu;
constexpr uint32_t kHash_Episode = 0xCEBD49DBu;
constexpr uint32_t kHash_EntityIndex = 0x9A5AAD6Eu;      // NOTE: name unknown
constexpr uint32_t kHash_Level = 0x044FA364u;
constexpr uint32_t kHash_Job = 0x00011F9Du;
constexpr uint32_t kHash_ScriptFile = 0x849B0778u;       // NOTE: name unknown
constexpr uint32_t kHash_HideoutCharacter = 0x54004DDDu;

constexpr uint32_t kAttr_Id = 0x0000091Bu;               // "id"
constexpr uint32_t kAttr_Name = 0x0024728Bu;             // "name"
constexpr uint32_t kAttr_Text = 0x00273D2Du;             // "text"
constexpr uint32_t kAttr_MovieFile = 0xECE92C8Cu;
constexpr uint32_t kAttr_MovieName = 0xECECB19Bu;
constexpr uint32_t kAttr_DefaultUnlocked = 0xA942CC04u;
constexpr uint32_t kAttr_Type = 0x0027873Au;             // "type"
constexpr uint32_t kAttr_EntityName = 0xC490A0EEu;
constexpr uint32_t kAttr_Active = 0xC53AF420u;           // NOTE: name unknown
constexpr uint32_t kAttr_NumBottles = 0x16C4FD27u;
constexpr uint32_t kAttr_IsDebugEpisode = 0xBD667672u;
constexpr uint32_t kAttr_EpisodeWeight = 0x8D3B7013u;
constexpr uint32_t kAttr_JobWeight = 0x26046C55u;
constexpr uint32_t kAttr_MaskWeight = 0xE5505464u;
constexpr uint32_t kAttr_BottleWeight = 0x6E38D7BEu;
constexpr uint32_t kAttr_TreasureWeight = 0xF2CC49E7u;
constexpr uint32_t kAttr_SafeWeight = 0x1B345045u;
constexpr uint32_t kAttr_LevelHub = 0xAF980AF1u;
constexpr uint32_t kAttr_LevelHideout = 0xA989C2C8u;
constexpr uint32_t kAttr_AssociatedDef = 0x2584611Fu;
constexpr uint32_t kAttr_UnlockGroup = 0x4D69959Bu;
constexpr uint32_t kAttr_DisguiseLocked = 0xA66B7CC9u;
constexpr uint32_t kAttr_Selectable = 0xBA3BD856u;

constexpr uint32_t kMovieTypes[2] = { 0x055F2BDDu /* Cinematic */, 0xF9AB1627u };

// Copies a non-empty string attribute into `out` (at most `count` chars).
void CopyString(XmlElement* element, const char* attribute, char* out, size_t count)
{
    const char* value = element->GetString(attribute);
    if (value && *value)
        ape::StrNCopy(out, value, count);
}

void LoadJob(Episode& job, XmlElement* element)
{
    CopyString(element, "name", job.name, 0x40);
    CopyString(element, "desc", job.desc, 0x40);
    CopyString(element, "jobScript", job.jobScript, 0x80);
    element->GetHash(kAttr_Id, &job.nameHash, 0);
    element->GetHash(0x53B55808u, &job.field_1D8, 0);
    element->GetHash(0xC9F8B7B3u, &job.field_1DC, 0);
    element->GetHash(0xA3A3941Fu, &job.field_1E0, 0);
    element->GetHash(0x41299858u, &job.field_1E4, 0);
    CopyString(element, "movieOnJobComplete", job.movieOnJobComplete, 0x80);
    if (const char* abbrev = element->GetString("abbrev"); abbrev && *abbrev)
        ape::StrCopyBounded(job.abbrev, abbrev, sizeof(job.abbrev));
    element->GetHash(0x6E6EDD6Bu, &job.field_1E8, job.field_1E8);
    element->GetHash(kAttr_AssociatedDef, &job.associatedDef, job.associatedDef);
    element->GetFloat(0x74BC9C7Bu, &job.field_1F0, job.field_1F0);

    int32_t unlockGroup = -1;
    element->GetInt(kAttr_UnlockGroup, &unlockGroup, -1);
    job.order = static_cast<uint8_t>(unlockGroup);

    bool flag = job.field_1F7 != 0;
    element->GetBool(kAttr_DisguiseLocked, &flag, flag);
    job.field_1F7 = flag;
    flag = job.field_1F8 != 0;
    element->GetBool(0xE587C810u, &flag, flag);
    job.field_1F8 = flag;
    flag = job.field_1F9 != 0;
    element->GetBool(0x6AAB7C29u, &flag, flag);
    job.field_1F9 = flag;

    int32_t value = job.field_1FA;
    element->GetInt(0xBE8B7B06u, &value, value);
    job.field_1FA = static_cast<uint8_t>(value > 0xFF ? 0xFF : (value < 0 ? 0 : value));

    // Text messages: every child the text parser accepts, while there is room.
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        TextField text;
        if (!levels::TextFieldParse(child, &text))
            continue;
        if (job.texts.Count() >= job.texts.Capacity())
            break;
        job.texts.PushBack(text);
    }

    for (XmlElement* file = element->FirstChildElement(kHash_ScriptFile); file;
         file = file->NextSiblingElement(kHash_ScriptFile)) {
        const char* path = file->GetString("path");
        if (!path || !*path)
            continue;
        Episode::ScriptPath entry{};
        ape::StrNCopy(entry.path, path, 0x80);
        job.scriptPaths.PushBack(entry);
    }

    for (XmlElement* character = element->FirstChildElement(kHash_HideoutCharacter); character;
         character = character->NextSiblingElement(kHash_HideoutCharacter)) {
        Episode::HideoutCharacter entry;
        if (character->GetHash(kAttr_Name, &entry.name, 0) != 0)
            continue;
        character->GetHash(0x09999BDCu, &entry.field_04, 0);
        character->GetBool(kAttr_Selectable, &entry.selectable, true);
        character->GetBool(0xABB46BE6u, &entry.field_09, false);
        character->GetBool(0xB15553D6u, &entry.field_0A, false);
        job.characters.PushBack(entry);
    }
}

template <class V>
void FreeValues(ape::ApeTSortedMap<V*>& map)
{
    auto* entries = map.Data();
    for (uint16_t i = 0; i < map.Count(); ++i)
        delete entries[i].value;
    map.Clear();
}

} // namespace

JobManager* GetJobManager() { return g_jobManager; }
void SetJobManager(JobManager* manager) { g_jobManager = manager; }

ape::ApeTSortedMap<MovieEntry*>& MovieEntries() { return g_movieEntries; }

// @func 0x002E2D44 FindMovieEntry
MovieEntry* FindMovieEntry(uint32_t key)
{
    MovieEntry** entry = g_movieEntries.Find(key);
    return entry ? *entry : nullptr;
}
ape::ApeTSortedMap<uint8_t*>& EpisodeEntityIndices() { return g_entityIndices; }
uint32_t UnlockableCount() { return static_cast<uint32_t>(g_unlockables.size()); }
Unlockable* UnlockableAt(uint32_t index) { return index < g_unlockables.size() ? g_unlockables[index] : nullptr; }
uint32_t UnlockableKey(uint32_t index) { return UnlockableAt(index) ? g_unlockables[index]->key : 0; }
uint32_t UnlockableGroup(uint32_t index) { return UnlockableAt(index) ? g_unlockables[index]->group : 0; }

// @func 0x002DB968 JobManager::LoadEpisode
void JobManager::LoadEpisode(const char* path, uint8_t* bottleSlot, uint8_t* maskSlot, uint8_t* treasureSlot,
                             bool active)
{
    XmlDocument document(path);
    if (!document.LoadFile(0))
        return;
    XmlElement* root = document.FirstChildElement(kHash_Episode);
    if (!root || jobCount >= kMaxJobs)
        return;

    Job& episode = jobs[jobCount++];
    episode = Job();
    episode.active = active;
    CopyString(root, "name", episode.name, 0x40);
    CopyString(root, "desc", episode.desc, 0x40);
    CopyString(root, "timePeriod", episode.timePeriod, 0x40);
    CopyString(root, "startMovie", episode.startMovie, 0x40);
    CopyString(root, "endMovie", episode.endMovie, 0x40);
    CopyString(root, "endMovie2", episode.endMovie2, 0x40);
    CopyString(root, "endMovie3", episode.endMovie3, 0x40);
    if (const char* abbrev = root->GetString("abbrev"); abbrev && *abbrev)
        ape::StrCopyBounded(episode.abbrev, abbrev, 4);
    root->GetHash(kAttr_Id, &episode.field_2D4, 0);
    root->GetHash(0xC9F8B7B3u, &episode.field_2D8, 0);
    root->GetHash(0xA3A3941Fu, &episode.field_2DC, 0);
    root->GetHash(0x41299858u, &episode.field_2E0, 0);
    root->GetHash(kAttr_LevelHub, &episode.levelHub, 0);
    root->GetHash(kAttr_LevelHideout, &episode.nameHash, 0);
    root->GetHash(0x2909A286u, &episode.field_32C, 0);
    root->GetFloat(0x74BC9C7Bu, &episode.field_330, 0.0f);

    int32_t bottles = g_defaultBottleCount;
    root->GetInt(kAttr_NumBottles, &bottles, g_defaultBottleCount);
    if (bottles < 0 || bottles > 0xFE)
        bottles = g_defaultBottleCount;
    episode.numBottles = static_cast<uint8_t>(bottles);

    bool flag = episode.isDebugEpisode != 0;
    root->GetBool(kAttr_IsDebugEpisode, &flag, flag);
    episode.isDebugEpisode = flag;
    flag = episode.field_33B != 0;
    root->GetBool(0xE587C810u, &flag, flag);
    episode.field_33B = flag;
    root->GetBool(0x37C7C4ADu, &flag, false);
    episode.field_33D = flag;
    root->GetFloat(kAttr_EpisodeWeight, &episode.episodeWeight, 0.0f);
    root->GetFloat(kAttr_JobWeight, &episode.jobWeight, episode.jobWeight);
    root->GetFloat(kAttr_MaskWeight, &episode.maskWeight, episode.maskWeight);
    root->GetFloat(kAttr_BottleWeight, &episode.bottleWeight, episode.bottleWeight);
    root->GetFloat(kAttr_TreasureWeight, &episode.treasureWeight, episode.treasureWeight);
    root->GetFloat(kAttr_SafeWeight, &episode.safeWeight, episode.safeWeight);
    root->GetHash(0xEED7AB50u, &episode.field_358, 0);
    episode.episode = static_cast<uint8_t>(jobCount - 1);

    // Debug episodes get no collectible words.
    if (!episode.isDebugEpisode) {
        if (episode.levelHub != 0)
            episode.bottleSlot = (*bottleSlot)++;
        if (episode.levelHub != 0 || episode.field_33D)
            episode.maskSlot = (*maskSlot)++;
        if (episode.nameHash != 0)
            episode.bitGroup = (*treasureSlot)++;
    }

    for (XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t type = child->GetNameHash();
        if (type == kHash_Level) {
            if (episode.levelCount < 16)
                child->GetHash(kAttr_Name, &episode.levels[episode.levelCount++], 0);
        } else if (type == kHash_Job) {
            if (episodeCount >= kMaxEpisodes)
                continue;
            Episode& job = episodes[episodeCount++];
            if (!episode.stages)
                episode.stages = &job;
            ++episode.stageCount;
            LoadJob(job, child);
            job.episode = static_cast<uint8_t>(jobCount - 1);
            job.index = static_cast<uint8_t>(episodeCount - 1);
        }
        // <Mask>, <Treasure> and <safeTreasure> children name collectibles;
        // the release build reads their name and drops it.
    }
}

// @func 0x002DCDD0 JobManager::Load
void JobManager::Load(const char* path)
{
    XmlDocument document(path);
    if (!document.LoadFile(0))
        return;
    XmlElement* root = document.FirstChildElement(kHash_EpisodeList);
    if (!root)
        return;

    uint8_t bottleSlot = 0;
    uint8_t maskSlot = 0;
    uint8_t treasureSlot = 0;
    uint8_t entityIndex = 0;
    int32_t movieIndex = 0;
    for (XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t type = child->GetNameHash();
        if (type == kHash_MovieEntry) {
            uint32_t key = 0;
            child->GetHash(kAttr_MovieFile, &key, 0);
            if (key == 0 || *g_movieEntries.Find(key))
                continue;
            auto* entry = new MovieEntry();
            if (const char* text = child->GetString(kAttr_Text))
                ape::StrNCopy(entry->text, text, 0x1F);
            child->GetInt(0xBF873E4Bu, &entry->field_00, 0);
            child->GetInt(0x95E16172u, &entry->field_04, 0);
            g_movieEntries.Add(key, entry);
        } else if (type == kHash_MovieUnlockable) {
            uint32_t key = 0;
            child->GetHash(kAttr_MovieFile, &key, 0);
            if (key == 0)
                continue;
            bool known = false;
            for (const Unlockable* unlockable : g_unlockables)
                known = known || unlockable->key == key;
            if (known)
                continue;
            auto* unlockable = new Unlockable();
            unlockable->key = key;
            if (const char* name = child->GetString(kAttr_MovieName))
                ape::StrNCopy(unlockable->movieName, name, 0x1F);
            if (const char* file = child->GetString(kAttr_MovieFile))
                ape::StrNCopy(unlockable->movieFile, file, 0x3F);
            child->GetBool(kAttr_DefaultUnlocked, &unlockable->defaultUnlocked, unlockable->defaultUnlocked);
            uint32_t movieType = kMovieTypes[0];
            child->GetHash(kAttr_Type, &movieType, kMovieTypes[0]);
            for (uint32_t i = 0; i < 2; ++i) {
                if (kMovieTypes[i] == movieType) {
                    unlockable->group = i;
                    break;
                }
            }
            unlockable->index = movieIndex++;
            g_unlockables.push_back(unlockable);
        } else if (type == kHash_Episode) {
            bool active = true;
            child->GetBool(kAttr_Active, &active, true);
            const char* file = child->GetString("file");
            if (file)
                LoadEpisode(file, &bottleSlot, &maskSlot, &treasureSlot, active);
        } else if (type == kHash_EntityIndex) {
            uint32_t name = 0;
            if (child->GetHash(kAttr_EntityName, &name, 0) != 0 || name == 0)
                continue;
            if (*g_entityIndices.Find(name)) {
                child->GetDocumentPath();  // the release build's warning prints nothing
                continue;
            }
            g_entityIndices.Add(name, new uint8_t(entityIndex++));
        }
    }
}

// @func 0x002DD4FC JobManager::JobManager
JobManager::JobManager()
{
    flags = 0x08000000u;
    g_jobManager = this;
    jobs = new Job[kMaxJobs];
    episodes = new Episode[kMaxEpisodes];
    Load("Scripts/Episodes.xml");
}

// @func 0x002DD604 JobManager::~JobManager
// NOTE: the console also tells a pending package stream (+0x08) to stop;
// the port does not stream packages.
JobManager::~JobManager()
{
    g_jobManager = nullptr;
    FreeValues(TreasureTable());
    FreeValues(MaskTable());
    FreeValues(SafeTable());
    FreeValues(g_entityIndices);
    FreeValues(g_movieEntries);
    for (Unlockable* unlockable : g_unlockables)
        delete unlockable;
    g_unlockables.clear();
    delete[] jobs;
    delete[] episodes;
    jobs = nullptr;
    episodes = nullptr;
    jobCount = 0;
    episodeCount = 0;
}

// @func 0x002DD898 JobManager::LoadJobScripts
void JobManager::LoadJobScripts()
{
    if (!instance)
        return;
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kSharedScriptLibrary);
    if (!library || library->Scripts().Count() > 0)
        return;
    // TODO(0x002DA5E0): a job whose packages are streamed (load state 1)
    // reads its scripts from the stream buffers; the port loads them from
    // their files.
    for (const Episode::ScriptPath& script : instance->scriptPaths)
        library->LoadFile(script.path);
}

// @func 0x002DDA54 JobManager::ReloadJobScripts
// @func 0x002DE1F4 JobManager::ReloadJobScripts (copy 2)
void JobManager::ReloadJobScripts()
{
    if (!instance)
        return;
    if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kSharedScriptLibrary)) {
        library->Clear();
        LoadJobScripts();
    }
}

// TODO(0x00465F30 / 0x004674C4): the players block and the level request
// live in the game world.
Goal* PlayerFindGoal(uint32_t) { return nullptr; }
void GameWorldRequestLevel(uint32_t) {}

// @func 0x002DDDCC JobManager::OnLevelStart
void JobManager::OnLevelStart()
{
    if (Job* episode = CurrentEpisode()) {
        collectedBottles = static_cast<uint32_t>(episode->CollectedBottles());
        collectedMasks = static_cast<uint32_t>(CountCollectedMasks());
    } else {
        collectedBottles = 0;
        collectedMasks = 0;
    }

    Goal* goal = nullptr;
    if (trackedGoalId != 0) {
        goal = PlayerFindGoal(trackedGoalId);
    } else if (instance) {
        for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
            Goal* candidate = PlayerGoalListAt(i);
            if (candidate && candidate->status != goals::kGoal_Complete) {
                goal = candidate;
                break;
            }
        }
    }
    trackedGoal = goal;
    field_4C = 0;
    field_50 = 0;
    field_40 = 0;
    field_44 = 0;
    pendingRequest[0] = 0;
    pendingRequest[1] = 0;
    field_54 = 0;
    flags &= ~0x30000000u;

    if (streamFlags & (1u << 29)) {
        // NOTE: the console stops the package stream here (not ported).
        streamFlags = 0;
        ClearGoals();
    }

    if (trackedGoal) {
        const uint32_t level = trackedGoal->Field960();
        if (level != 0 && level != LiveJobId())
            GameWorldRequestLevel(level);
    }
}

// @func 0x002DE198 JobManager::OnLevelLoaded
void JobManager::OnLevelLoaded()
{
    if (instance) {
        if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(kSharedScriptLibrary))
            library->LinkAll();
    }
    OnLevelStart();
}

// @func 0x002DF010 JobManager::ActivateMaskSpawner
void JobManager::ActivateMaskSpawner(Spawner* spawner, const uint16_t* maskBits) const
{
    if (!spawner || !maskBits)
        return;
    const uint32_t name = WorldObjectKey(spawner->spawnPoint);
    if (name != 0) {
        const CollectibleRecord* mask = *MaskTable().Find(name);
        if (mask && (mask->field_08 == 0 || mask->foundIn == 0)) {
            const uint8_t slot = jobs[mask->episode].maskSlot;
            if (slot == 0xFF)
                return;
            if (slot >= PlayerProgress::kEpisodeSlots) {
                ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer "
                                "increase the size of the array.",
                                slot, PlayerProgress::kEpisodeSlots);
                return;
            }
            if ((maskBits[slot] & (1u << (mask->bit & 0x3F))) == 0)
                spawner->Activate();
            return;
        }
    }
    ape::TempFormat("Couldn't find mask spawner '%s' in the mask info map! Please have an engineer check.",
                    WorldObjectName(spawner->spawnPoint));
}

// @func 0x002DFAA0 JobManager::ActivateTreasureSpawner
void JobManager::ActivateTreasureSpawner(Spawner* spawner, const uint16_t* treasureBits, bool inHideout) const
{
    if (!spawner || !treasureBits)
        return;
    const uint32_t name = WorldObjectKey(spawner->spawnPoint);
    const CollectibleRecord* treasure = name != 0 ? *TreasureTable().Find(name) : nullptr;
    if (!treasure) {
        ape::TempFormat("Couldn't find treasure spawner '%s' in the treasure info map! Please have an engineer "
                        "check.",
                        WorldObjectName(spawner->spawnPoint));
        return;
    }
    const uint8_t slot = jobs[treasure->episode].bitGroup;
    if (slot == 0xFF)
        return;
    if (slot >= PlayerProgress::kEpisodeSlots) {
        ape::TempFormat("Episode Index is out of bounds '%i'! Range is [0 - %i]! Have an engineer increase the "
                        "size of the array.",
                        slot, PlayerProgress::kEpisodeSlots);
        return;
    }
    const bool missing = (treasureBits[slot] & (1u << (treasure->bit & 0x3F))) == 0;
    const bool activate = inHideout ? !(currentJob && missing) : missing;
    if (activate)
        spawner->Activate();
}

// @func 0x002DFC2C JobManager::SpawnCollectibles
void JobManager::SpawnCollectibles() const
{
    constexpr uint32_t kTreasureGroup = 0xFB673D64u;  // NOTE: group names unknown
    constexpr uint32_t kSafeGroup = 0x5E69B571u;
    if (!g_spawnManager)
        return;
    const Job* episode = CurrentEpisode();
    PlayerProgress& progress = CurrentProgress();

    if (SpawnGroup* group = g_spawnManager->FindGroup(kTreasureGroup); group && episode) {
        const bool inHideout = episode->nameHash != 0 && episode->nameHash == LiveJobId();
        for (Spawner* spawner = group->firstSpawner; spawner; spawner = spawner->nextInGroup)
            ActivateTreasureSpawner(spawner, progress.jobBits, inHideout);
    }

    if (SpawnGroup* group = g_spawnManager->FindGroup(kSafeGroup)) {
        for (Spawner* spawner = group->firstSpawner; spawner; spawner = spawner->nextInGroup) {
            const uint32_t name = WorldObjectKey(spawner->spawnPoint);
            if (name == 0)
                continue;
            const CollectibleRecord* safe = *SafeTable().Find(name);
            if (!safe)
                continue;
            const Job& owner = jobs[safe->episode];
            if (!owner.isDebugEpisode && (progress.episodes[owner.episode].flags & 7) == 3)
                spawner->Activate();
        }
    }
}

// @func 0x002DFEFC JobManager::PrepareGoalScripts
void JobManager::PrepareGoalScripts() const
{
    if (!instance)
        return;
    for (uint32_t i = 0; i < PlayerGoalCount(); ++i) {
        if (Goal* goal = PlayerGoalListAt(i)) {
            goal->PrepareScripts();
            goal->ReapplyHints(nullptr);
        }
    }
}

// @func 0x002DFFDC JobManager::SetRunningJobValue
void JobManager::SetRunningJobValue(uint32_t value) const
{
    CurrentProgress().field_A0 = value;
}

// @func 0x002DDC5C JobManager::CountCollectedMasks
int JobManager::CountCollectedMasks() const
{
    int count = 0;
    for (uint32_t i = 0; i < jobCount; ++i) {
        if (jobs[i].active)
            count += jobs[i].CollectedMasks(CurrentProgress());
    }
    return count;
}

// @func 0x002DDD24 JobManager::CurrentEpisode
Job* JobManager::CurrentEpisode() const
{
    if (currentJob)
        return currentJob;
    const uint32_t level = LiveJobId();
    for (uint32_t i = 0; i < jobCount; ++i) {
        const Job& episode = jobs[i];
        if (episode.isDebugEpisode)
            continue;
        for (uint8_t l = 0; l < episode.levelCount; ++l) {
            if (episode.levels[l] == level)
                return &jobs[i];
        }
    }
    return nullptr;
}

} // namespace game
} // namespace sly
