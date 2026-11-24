// openTIT - The gameplay sequencer: scripted lists of commands that run one
// after another (cutscene glue, camera moves, entity control, waits).
//
// Scripts build a sequence by calling BeginSequence(), adding commands with
// AddSequenceCommand() and closing it with EndSequence(). Sequences nest: a
// sequence begun while another is open becomes its child and is deleted with
// it. Every frame UpdateSequencer() runs the front command of each started
// sequence; a blocking command keeps the sequence waiting until its Update()
// reports completion.
//
// While a sequence runs it can take over world entities ("SetEntity"); the
// entity table remembers the state each entity had so it can be restored
// when the owning sequence ends.
//
// Console globals: sequence list 0x010AA694 (count +8), sequences started
// this frame 0x010AA6A8, entity table 0x00F65858 (count 0x010AA690, 0x300
// entries of 0x10 bytes), sequence being built 0x00F6585C.
#pragma once

#include <cstdint>
#include <vector>

namespace sly {

class WorldObject;

namespace gameplay {

class Sequence;

// ---------------------------------------------------------------------------
// SequenceCommand (vtable 0x00B20CC8, 0x0C bytes)
// ---------------------------------------------------------------------------
class SequenceCommand {
public:
    explicit SequenceCommand(bool blocking);
    virtual ~SequenceCommand();

    // Slot 2: called once when the command reaches the front of its sequence
    // (and again every frame for commands that do not mark themselves as
    // started). The base version only marks the command as started.
    virtual void Execute();
    // Slot 3: called every frame for blocking commands; returns true once the
    // command is finished.
    virtual bool Update(float dt);

    Sequence* GetSequence() const { return m_sequence; }
    void SetSequence(Sequence* sequence) { m_sequence = sequence; }
    bool IsBlocking() const { return m_blocking; }
    bool IsStarted() const { return m_started; }

protected:
    void MarkStarted() { m_started = true; }

    Sequence* m_sequence = nullptr;   // +0x04 owning sequence
    bool m_blocking = false;          // +0x08 bit 31
    bool m_started = false;           // +0x08 bit 30
};

// ---------------------------------------------------------------------------
// Sequence (0x24 bytes)
// ---------------------------------------------------------------------------
class Sequence {
public:
    Sequence() = default;
    ~Sequence();
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

    // Runs the front commands; see the file comment.
    void Update(float dt);
    // Deletes every queued command and flags the sequence as finished.
    void Stop();
    // Deletes every queued command.
    void DeleteCommands();

    uint32_t nameHash = 0;                       // +0x00
    std::vector<SequenceCommand*> commands;      // +0x04 (count +0x0C)
    bool started = false;                        // +0x18 bit 31
    bool enabled = true;                         // +0x18 bit 30
    bool finished = false;                       // +0x18 bit 29
    Sequence* parent = nullptr;                  // +0x1C
    float elapsed = 0.0f;                        // +0x20
};

// ---------------------------------------------------------------------------
// Entities taken over by sequences (0x10-byte records)
// ---------------------------------------------------------------------------
struct SequenceEntity {
    Sequence* owner = nullptr;      // +0x00
    WorldObject* entity = nullptr;  // +0x04
    uint32_t alias = 0;             // +0x08 name used by commands (0 = entity name)
    // +0x0C flags
    bool destroyOnRelease = false;  // bit 31
    bool wasShown = false;          // bit 30
    bool keepShownState = false;    // bit 29: do not touch visibility on release
    bool hideOnRelease = false;     // bit 28
    bool renderFlag12 = false;      // bit 27
    bool hadCollision = false;      // bit 26
    bool renderFlag10 = false;      // bit 25
};

// Options of RegisterSequenceEntity().
struct SequenceEntityOptions {
    uint32_t alias = 0;
    bool destroyOnRelease = false;
    bool show = false;              // show the entity while controlled
    bool hideOnRelease = false;
    bool keepShownState = false;
    bool resetPose = false;
    bool neverShow = false;
    bool keepAnimSet = false;
    bool clearAiState = false;
    bool transferIfRegistered = false;  // move an already registered entity to this sequence
};

constexpr int kMaxSequenceEntities = 0x300;

// -- lifetime -------------------------------------------------------------
// Clears everything and creates the root sequence (index 0), which is
// always started and is where top-level commands go.
void ResetSequencer();
void ShutdownSequencer();
// Releases all entities and deletes all sequences.
void ClearSequences();

// -- building -------------------------------------------------------------
Sequence* BeginSequence(uint32_t nameHash);
void EndSequence();
void AddSequenceCommand(SequenceCommand* command);
Sequence* CurrentBuildSequence();

// -- control --------------------------------------------------------------
void UpdateSequencer(float dt);
int SequenceCount();
Sequence* GetSequence(int index);
// First sequence with this name that has not finished.
Sequence* FindSequence(uint32_t nameHash);
// Remembers a sequence started during this frame's update so it also runs
// this frame.
void QueueStartedSequence(Sequence* sequence);
void SetSequenceEnabledByIndex(int index, bool enabled, bool recursive);
void SetSequenceEnabled(uint32_t nameHash, bool enabled, bool recursive);
void StopSequence(Sequence* sequence);
void StopSequenceByName(uint32_t nameHash);
// True if any sequence still has queued commands.
bool AnySequenceRunning();
// True if the named sequence exists and still has queued commands.
bool IsSequenceRunning(uint32_t nameHash);

// -- entities ------------------------------------------------------------
WorldObject* FindSequenceEntity(uint32_t nameHash);
void RegisterSequenceEntity(Sequence* owner, WorldObject* entity,
                            const SequenceEntityOptions& options);
void UnregisterSequenceEntity(Sequence* owner, WorldObject* entity);
Sequence* FindSequenceEntityOwner(const WorldObject* entity);
int SequenceEntityCount();
const SequenceEntity& SequenceEntityAt(int index);

// Debug display switches (tuning variables "Debug.Display.Sequencer" and
// "Debug.Display.SequencerActive").
bool SequencerDebugDisplay();
bool SequencerDebugDisplayActive();

} // namespace gameplay
} // namespace sly
