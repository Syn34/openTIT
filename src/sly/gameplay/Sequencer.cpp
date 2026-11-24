// openTIT - The gameplay sequencer (see Sequencer.h).
#include "sly/gameplay/Sequencer.h"

#include "sly/gameplay/GameplayWorld.h"

#include "ape/tuning/ApeTuning.h"

#include <algorithm>

namespace sly {
namespace gameplay {

namespace {

// @func 0x0042CB0C StaticInit_Sequencer
// @func 0x0042CA78 StaticDestroy_Sequencer
// Compiler-generated initialiser/destructor pair for the globals below.
struct SequencerState {
    SequencerState()
    {
        sequences.reserve(0x160);
        startedThisFrame.reserve(0x160);
    }

    std::vector<Sequence*> sequences;          // 0x010AA694
    std::vector<Sequence*> startedThisFrame;   // 0x010AA6A8
    std::vector<SequenceEntity> entities;      // 0x00F65858
    Sequence* building = nullptr;              // 0x00F6585C
};

SequencerState g_sequencer;

ape::TuningBool g_debugDisplaySequencer("Debug.Display.Sequencer", false);             // 0x010AA6C0
ape::TuningBool g_debugDisplaySequencerActive("Debug.Display.SequencerActive", false); // 0x010AA710

SequencerState& State()
{
    return g_sequencer;
}

// Removes element 'index' by moving the last element into its place.
template <typename T>
void SwapRemove(std::vector<T>& list, size_t index)
{
    if (index + 1 != list.size())
        list[index] = list.back();
    list.pop_back();
}

uint32_t EntryKey(const SequenceEntity& entry)
{
    return entry.alias != 0 ? entry.alias : EntityNameHash(entry.entity);
}

// @func 0x0042AEE8 RestoreSequenceEntity
// Gives an entity back to the world in the state it had before a sequence
// took it over.
void RestoreSequenceEntity(SequenceEntity& entry)
{
    WorldObject* entity = entry.entity;
    if (entity == nullptr)
        return;

    if (!entry.keepShownState)
        EntitySetShown(entity, entry.hideOnRelease ? false : entry.wasShown);
    entry.wasShown = false;

    EntitySetRenderFlag12(entity, entry.renderFlag12);
    EntitySetRenderFlag10(entity, entry.renderFlag10);
    EntitySetSequenceControlled(entity, false);
    EntitySetCollision(entity, entry.hadCollision);
    EntityRelease(entity, false);
    if (entry.destroyOnRelease)
        EntityRelease(entity, true);
    entry.entity = nullptr;
}

void ReleaseEntitiesOf(Sequence* sequence)
{
    std::vector<SequenceEntity>& entities = State().entities;
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i].owner != sequence)
            continue;
        RestoreSequenceEntity(entities[i]);
        SwapRemove(entities, i);
        --i;
    }
}

// Runs one sequence and releases its entities once it has finished.
void TickSequence(Sequence* sequence, float dt)
{
    if (sequence->started && sequence->enabled)
        sequence->Update(dt);
    if (sequence->finished)
        ReleaseEntitiesOf(sequence);
}

Sequence* NewSequence()
{
    Sequence* sequence = new Sequence();
    sequence->commands.reserve(0x20);
    return sequence;
}

} // namespace

// ---------------------------------------------------------------------------
// SequenceCommand
// ---------------------------------------------------------------------------

// @func 0x0042C9D0 SequenceCommand::SequenceCommand
SequenceCommand::SequenceCommand(bool blocking)
    : m_blocking(blocking)
{
}

// @func 0x0042C9FC SequenceCommand::~SequenceCommand
// @func 0x0042CA50 SequenceCommand::~SequenceCommand (base part called by subclasses)
// @func 0x0042CA0C SequenceCommand::~SequenceCommand (deleting)
SequenceCommand::~SequenceCommand() = default;

// @func 0x0042CA60 SequenceCommand::Execute
void SequenceCommand::Execute()
{
    MarkStarted();
}

// @func 0x0042CA70 SequenceCommand::Update
bool SequenceCommand::Update(float /*dt*/)
{
    return true;
}

// ---------------------------------------------------------------------------
// Sequence
// ---------------------------------------------------------------------------

Sequence::~Sequence()
{
    DeleteCommands();
}

void Sequence::DeleteCommands()
{
    for (SequenceCommand* command : commands)
        delete command;
    commands.clear();
}

// @func 0x0042BF50 StopSequence
void Sequence::Stop()
{
    DeleteCommands();
    started = false;
    finished = true;
}

// @func 0x0042B5B0 Sequence::Update
void Sequence::Update(float dt)
{
    elapsed += dt;
    while (!commands.empty()) {
        SequenceCommand* command = commands.front();
        if (!command->IsStarted())
            command->Execute();
        if (command->IsBlocking() && !command->Update(dt))
            break;
        delete command;
        commands.erase(commands.begin());
    }
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

// @func 0x0042B0C8 ClearSequences
void ClearSequences()
{
    SequencerState& state = State();
    for (SequenceEntity& entry : state.entities)
        RestoreSequenceEntity(entry);
    for (Sequence* sequence : state.sequences)
        delete sequence;
    state.sequences.clear();
}

// @func 0x0042B240 ShutdownSequencer
void ShutdownSequencer()
{
    SequencerState& state = State();
    ClearSequences();
    state.entities.clear();
    state.entities.shrink_to_fit();
    state.building = nullptr;
}

// @func 0x0042B29C ResetSequencer
void ResetSequencer()
{
    SequencerState& state = State();
    ClearSequences();
    state.entities.clear();
    state.entities.reserve(kMaxSequenceEntities);
    state.building = nullptr;

    state.sequences.push_back(NewSequence());
    state.building = state.sequences.front();
    state.building->started = true;
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

// @func 0x0042BAE0 BeginSequence
Sequence* BeginSequence(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;

    SequencerState& state = State();
    Sequence* sequence = NewSequence();
    state.sequences.push_back(sequence);
    if (state.building != sequence)
        sequence->parent = state.building;
    sequence->nameHash = nameHash;
    state.building = sequence;
    return sequence;
}

// @func 0x0042BC24 EndSequence
void EndSequence()
{
    SequencerState& state = State();
    if (state.building != nullptr && state.building->parent != nullptr)
        state.building = state.building->parent;
}

// @func 0x0042C138 AddSequenceCommand
void AddSequenceCommand(SequenceCommand* command)
{
    Sequence* building = State().building;
    if (building == nullptr)
        return;
    command->SetSequence(building);
    building->commands.push_back(command);
}

Sequence* CurrentBuildSequence()
{
    return State().building;
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

// @func 0x0042B420 DeleteChildSequences
// Deletes every sequence whose parent is 'parent', depth first.
static void DeleteChildSequences(const Sequence* parent)
{
    std::vector<Sequence*>& sequences = State().sequences;
    for (size_t i = 0; i < sequences.size(); ++i) {
        Sequence* child = sequences[i];
        if (child->parent != parent)
            continue;
        SwapRemove(sequences, i);
        --i;
        DeleteChildSequences(child);
        delete child;
    }
}

// @func 0x0042B720 UpdateSequencer
void UpdateSequencer(float dt)
{
    SequencerState& state = State();
    // Only the root sequence: nothing to do.
    if (state.sequences.size() <= 1)
        return;

    state.startedThisFrame.clear();
    for (size_t i = state.sequences.size(); i-- > 0;)
        TickSequence(state.sequences[i], dt);
    // Sequences started by commands above run in the same frame. The list
    // may grow while it is walked.
    for (size_t i = 0; i < state.startedThisFrame.size(); ++i)
        TickSequence(state.startedThisFrame[i], dt);

    std::vector<Sequence*>& sequences = state.sequences;
    for (size_t i = 0; i < sequences.size(); ++i) {
        Sequence* sequence = sequences[i];
        if (!sequence->finished)
            continue;
        SwapRemove(sequences, i);
        --i;
        DeleteChildSequences(sequence);
        delete sequence;
    }
}

int SequenceCount()
{
    return static_cast<int>(State().sequences.size());
}

// @func 0x0042BCD4 GetSequence
Sequence* GetSequence(int index)
{
    const std::vector<Sequence*>& sequences = State().sequences;
    if (index < 0 || static_cast<size_t>(index) >= sequences.size())
        return nullptr;
    return sequences[static_cast<size_t>(index)];
}

static int FindSequenceIndex(uint32_t nameHash)
{
    const std::vector<Sequence*>& sequences = State().sequences;
    for (size_t i = 0; i < sequences.size(); ++i) {
        if (!sequences[i]->finished && sequences[i]->nameHash == nameHash)
            return static_cast<int>(i);
    }
    return -1;
}

// @func 0x0042BD0C FindSequence
Sequence* FindSequence(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    return GetSequence(FindSequenceIndex(nameHash));
}

// @func 0x0042BC44 QueueStartedSequence
void QueueStartedSequence(Sequence* sequence)
{
    State().startedThisFrame.push_back(sequence);
}

// @func 0x0042BD94 SetSequenceEnabledByIndex
void SetSequenceEnabledByIndex(int index, bool enabled, bool recursive)
{
    Sequence* sequence = GetSequence(index);
    if (sequence == nullptr)
        return;
    sequence->enabled = enabled;
    if (!recursive)
        return;
    // Children are always created after their parent.
    for (int i = index + 1; i < SequenceCount(); ++i) {
        if (GetSequence(i)->parent == sequence)
            SetSequenceEnabledByIndex(i, enabled, recursive);
    }
}

// @func 0x0042BE98 SetSequenceEnabled
void SetSequenceEnabled(uint32_t nameHash, bool enabled, bool recursive)
{
    // NOTE: a zero name addresses the root sequence.
    const int index = nameHash == 0 ? 0 : FindSequenceIndex(nameHash);
    SetSequenceEnabledByIndex(index, enabled, recursive);
}

void StopSequence(Sequence* sequence)
{
    if (sequence != nullptr)
        sequence->Stop();
}

// @func 0x0042C00C StopSequenceByName
void StopSequenceByName(uint32_t nameHash)
{
    if (nameHash == 0)
        return;
    StopSequence(FindSequence(nameHash));
}

// @func 0x0042C1D4 AnySequenceRunning
bool AnySequenceRunning()
{
    for (const Sequence* sequence : State().sequences) {
        if (!sequence->commands.empty())
            return true;
    }
    return false;
}

// @func 0x0042C22C IsSequenceRunning
bool IsSequenceRunning(uint32_t nameHash)
{
    if (nameHash == 0)
        return false;
    // Unlike FindSequence() this also matches finished sequences.
    for (const Sequence* sequence : State().sequences) {
        if (sequence->nameHash == nameHash)
            return !sequence->commands.empty();
    }
    return false;
}

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------

// @func 0x0042C2B0 FindSequenceEntity
WorldObject* FindSequenceEntity(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    for (const SequenceEntity& entry : State().entities) {
        if (entry.entity != nullptr && EntryKey(entry) == nameHash)
            return entry.entity;
    }
    return nullptr;
}

// @func 0x0042C328 RegisterSequenceEntity
void RegisterSequenceEntity(Sequence* owner, WorldObject* entity,
                            const SequenceEntityOptions& options)
{
    if (owner == nullptr || entity == nullptr)
        return;

    std::vector<SequenceEntity>& entities = State().entities;
    const uint32_t key = options.alias != 0 ? options.alias : EntityNameHash(entity);

    bool transferred = false;
    if (key != 0) {
        for (const SequenceEntity& entry : entities) {
            if (entry.entity == nullptr || EntryKey(entry) != key)
                continue;
            if (options.transferIfRegistered) {
                WorldObject* registered = entry.entity;
                for (SequenceEntity& other : entities) {
                    if (other.entity == registered)
                        other.owner = owner;
                }
                transferred = true;
            }
            break;
        }
    }

    if (!transferred) {
        SequenceEntity entry;
        entry.owner = owner;
        entry.entity = entity;
        entry.alias = options.alias;
        entry.destroyOnRelease = options.destroyOnRelease;
        entry.keepShownState = options.keepShownState;
        entry.hideOnRelease = options.hideOnRelease;
        entry.wasShown = EntityIsShown(entity);
        entry.renderFlag12 = EntityRenderFlag12(entity);
        entry.hadCollision = EntityHasCollision(entity);
        entry.renderFlag10 = EntityRenderFlag10(entity);
        entities.push_back(entry);

        EntityAddRef(entity);
        if (!options.keepAnimSet)
            EntityApplySequenceAnimSet(entity);
        EntitySetCollision(entity, true);
        EntitySetRenderFlag12(entity, true);
        EntitySetRenderFlag10(entity, false);
        if (options.resetPose)
            EntityResetPose(entity);
        if (EntityIsCharacter(entity))
            CharacterSuspendAi(entity, options.clearAiState);
        if (!options.neverShow && (options.show || EntityShowsWhenControlled(entity)))
            EntitySetShown(entity, true);
    }

    EntityEnableUpdate(entity, true);
    EntitySetSequenceControlled(entity, true);
}

// @func 0x0042C7DC UnregisterSequenceEntity
void UnregisterSequenceEntity(Sequence* owner, WorldObject* entity)
{
    std::vector<SequenceEntity>& entities = State().entities;
    for (size_t i = 0; i < entities.size(); ++i) {
        SequenceEntity& entry = entities[i];
        if (entry.entity != entity || entry.owner != owner)
            continue;
        // Unlike RestoreSequenceEntity() visibility is left alone.
        EntitySetSequenceControlled(entity, false);
        EntitySetCollision(entity, entry.hadCollision);
        EntitySetRenderFlag12(entity, entry.renderFlag12);
        EntitySetRenderFlag10(entity, entry.renderFlag10);
        EntityRelease(entity, false);
        SwapRemove(entities, i);
        return;
    }
}

// @func 0x0042C980 FindSequenceEntityOwner
Sequence* FindSequenceEntityOwner(const WorldObject* entity)
{
    for (const SequenceEntity& entry : State().entities) {
        if (entry.entity == entity)
            return entry.owner;
    }
    return nullptr;
}

int SequenceEntityCount()
{
    return static_cast<int>(State().entities.size());
}

const SequenceEntity& SequenceEntityAt(int index)
{
    return State().entities[static_cast<size_t>(index)];
}

bool SequencerDebugDisplay()
{
    return g_debugDisplaySequencer;
}

bool SequencerDebugDisplayActive()
{
    return g_debugDisplaySequencerActive;
}

} // namespace gameplay
} // namespace sly
