// openTIT - Streamed file groups (0x002E80C0-0x002EC2F8).
//
// Character animation and sound data is streamed in groups. A group is a set
// of files loaded together; each file has a type, and the game registers per
// type a "loaded" and an "unloaded" callback plus an optional factory for the
// file object (RegisterStreamType, called from SlyGame's constructor).
//
// Groups come from two places:
//  * the global stream table (IFF chunks ASTF / ASTH / ASTD): each group is
//    one packed file "Streaming/<name>.ftd" whose header lists its entries;
//    the entries are read one after the other into a shared buffer and each
//    file object of the group receives the bytes of its entry;
//  * the level definition's async file groups: one plain file per entry,
//    opened, measured, read and closed on its own.
// Groups are started (async or blocking) and cancelled by id. A low priority
// prefetcher warms the file cache with the shared character packs.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ape {
class ApeAllocator;
class ApeLevelDef;
namespace file {
struct AsyncFileHandle;
struct AsyncFileRequest;
}
namespace iff {
struct IffReader;
}
}

namespace sly {
namespace game {

// Called when a file's bytes are available (the buffer is only valid during
// the call) and when a loaded file is dropped. NOTE: the console passes the
// size as a 64-bit value for plain files; all sizes fit 32 bits.
using StreamFileCallback = void (*)(uint32_t id, const char* name, const void* data, uint32_t size,
                                    void* userData);

// Result of StreamFile::Update.
enum StreamFileStatus : int32_t {
    kStreamFile_Pending = 0,
    kStreamFile_Loaded  = 1,
    kStreamFile_Failed  = 2,
};

// One file of a group (base vtable 0x00B1D434).
class StreamFile {
public:
    // Frees the name.
    // @func 0x00766B6C StreamFile::~StreamFile
    // @func 0x00766BC0 StreamFile::~StreamFile (deleting)
    virtual ~StreamFile() = default;
    // slot 2: advances the load; non-zero once it has finished.
    virtual int Update(bool wait) = 0;
    // slot 3 / 4: mark the file started / stopped.
    // @func 0x00766C2C StreamFile::Start
    virtual void Start() { started = true; }
    // @func 0x00766C38 StreamFile::Cancel
    virtual void Cancel() { started = false; }

    uint32_t id = 0;                         // +0x04
    uint32_t type = 0;                       // +0x08
    std::string name;                        // +0x0C (plain files: the path)
    StreamFileCallback onLoaded = nullptr;   // +0x10
    StreamFileCallback onUnloaded = nullptr; // +0x14
    void* userData = nullptr;                // +0x18
    bool started = false;                    // +0x1C

protected:
    const char* NameOrNull() const { return name.empty() ? nullptr : name.c_str(); }
};

// Creates the file object of a registered type.
using StreamFileFactory = StreamFile* (*)();

// A plain file read on its own (vtable 0x00B1D450, 0x48 bytes).
class StreamDiskFile : public StreamFile {
public:
    StreamDiskFile();
    // @func 0x0076716C StreamDiskFile::~StreamDiskFile
    // @func 0x007671C0 StreamDiskFile::~StreamDiskFile (deleting)
    ~StreamDiskFile() override = default;

    int Update(bool wait) override;         // 0x002E80C0
    void Start() override;                  // 0x002E84FC
    void Cancel() override;                 // 0x002E8598

    enum State : int32_t {
        kIdle, kOpening, kSizing, kReading, kClosing, kDone, kFailed,
    };

    State state = kIdle;                             // +0x20
    ape::file::AsyncFileHandle* handle = nullptr;    // +0x24
    ape::file::AsyncFileRequest* request = nullptr;  // +0x28
    uint64_t size = 0;                               // +0x30
    void* buffer = nullptr;                          // +0x38
    ape::ApeAllocator* allocator = nullptr;          // +0x3C
    uint32_t readFlags = 0;                          // +0x40 AsyncFileFlags of the read

private:
    void ReleaseRequest();
};

class FtdGroup;

// An entry of a packed group (vtable 0x00B1D498, 0x28 bytes): hands the
// group's buffer to the callback while its entry is the one just read.
class StreamPackedFile : public StreamFile {
public:
    explicit StreamPackedFile(FtdGroup* owner) : group(owner) {}
    // @func 0x0076722C StreamPackedFile::~StreamPackedFile
    // @func 0x00767280 StreamPackedFile::~StreamPackedFile (deleting)
    ~StreamPackedFile() override = default;
    int Update(bool wait) override;         // 0x002EA218
    void Cancel() override;                 // 0x002EA344

    FtdGroup* group = nullptr;              // +0x20
    bool loaded = false;                    // +0x24

private:
    // Data and size the callbacks get (the buffer only while it holds this
    // entry).
    void CurrentData(const void** data, uint32_t* size) const;
};

// The optional prerequisite a group waits for before its files (+0x14).
// NOTE: no class in this module creates one; its layout (allocator +0x3C,
// buffer +0x44/+0x48, size +0x4C) and slot 5 come from how the group uses it.
class StreamBlob : public StreamFile {
public:
    // slot 5
    virtual bool IsLoaded() = 0;

    ape::ApeAllocator* allocator = nullptr;   // +0x3C
    void* data = nullptr;                     // +0x44
    uint32_t field_48 = 0;                    // +0x48
    uint32_t size = 0;                        // +0x4C
};

// A group of files (vtable 0x00B1D478).
class StreamGroup {
public:
    virtual ~StreamGroup();                   // 0x002E87D4 / 0x002E8954
    // slot 2: advances every file; marks the group complete once all are.
    virtual void Update(bool wait);           // 0x002E899C
    // slot 3: starts the group; with `wait` it loads it completely.
    virtual void Start(bool wait);            // 0x002E8BC8
    // slot 4
    virtual void Cancel();                    // 0x002E8CA0

    enum Flags : uint8_t {
        kGroup_Named    = 0x20,   // the name was read from the table (version 2)
        kGroup_Complete = 0x40,
        kGroup_Running  = 0x80,
    };
    bool Complete() const { return (flags & kGroup_Complete) != 0; }

    std::string name;                         // +0x04
    uint32_t id = 0;                          // +0x08
    uint32_t type = 0;                        // +0x0C
    std::vector<StreamFile*> files;           // +0x10 / +0x18 count
    StreamBlob* prerequisite = nullptr;       // +0x14
    uint8_t flags = 0;                        // +0x1C

protected:
    // Updates every file in order until one is still pending; true when all
    // have finished. Starts files that were not started yet.
    bool UpdateFiles(bool wait);
    // Records the result of an update pass in the running/complete bits.
    void SetComplete(bool complete);
};

// A group stored as one packed file "Streaming/<name>.ftd" (vtable
// 0x00B1D4B8, 0x70 bytes).
class FtdGroup : public StreamGroup {
public:
    FtdGroup();                               // 0x002E8DFC
    ~FtdGroup() override;
    void Update(bool wait) override;          // 0x002EB7BC
    void Start(bool wait) override;           // 0x002EBAF8
    void Cancel() override;                   // 0x002EA464

    enum State : int32_t {
        kIdle,
        kOpening,          // 1 open request in flight
        kReadingHeader,    // 2
        kReadingOffsets,   // 3 (id, offset) table
        kReadingSizes,     // 4 size table
        kReadingEntries,   // 5 one entry after the other
        kDelivering,       // 6 files are being updated
        kComplete,         // 7
        kError,            // 8
    };

    // One packed entry (16 bytes).
    struct Entry {
        uint32_t id;
        uint32_t size;
        uint64_t offset;
    };

    State state = kIdle;                             // +0x20
    std::string path;                                // +0x24 cached "Streaming/<name>.ftd"
    uint8_t* buffer = nullptr;                       // +0x28
    uint32_t bufferCapacity = 0;                     // +0x2C
    uint32_t readSize = 0;                           // +0x30 bytes of the read in flight
    uint64_t sizeTableOffset = 0;                    // +0x38
    uint64_t dataOffset = 0;                         // +0x40
    uint32_t entryCount = 0;                         // +0x48
    ape::ApeTArray<Entry> entries;                   // +0x4C
    ape::file::AsyncFileHandle* handle = nullptr;    // +0x60
    ape::file::AsyncFileRequest* request = nullptr;  // +0x64
    int32_t currentEntry = -1;                       // +0x68

private:
    // The file's path for messages (cached path, the open handle's path, or
    // the built name).
    const char* Path();
    void Fail(const char* fmt, ...);                 // 0x002EA67C
    bool IssueRead(uint32_t bytes, uint64_t offset); // 0x002EA728
    bool FinishRead();                               // 0x002EA92C
    void OnOpened();                                 // 0x002EB4EC
    void OnHeaderRead();                             // 0x002EB234
    void OnOffsetTableRead();                        // 0x002EB070
    void OnSizeTableRead();                          // 0x002EAE24
    void OnEntryRead();                              // 0x002EAC2C
    void ReadEntriesFrom();
    void Finish();
    void ReleaseRequest();
    bool RequestDone() const;
};

// --- module functions ------------------------------------------------------
// Registers the callbacks and factory of a file type (first registration
// wins) (0x002E9FE0).
void RegisterStreamType(uint32_t type, StreamFileCallback onLoaded, StreamFileCallback onUnloaded,
                        StreamFileFactory factory);
// Registers the ASTF / ASTH / ASTD chunk handlers (0x002E8D74).
void RegisterStreamChunkHandlers();
// Builds a plain-file group per async file group of the level definition,
// unless groups exist already (0x002E9338).
void CreateStreamGroupsFromLevel(const ape::ApeLevelDef* level);
// Per frame: advances every group and the cache prefetcher (0x002E9688).
void UpdateFileStreaming();
// Deletes every group and drops the prefetch queue (0x002E98CC).
void ClearStreamGroups();
// Cancels every group except the character ones (0x002E9A28).
void CancelNonCharacterGroups();
// Starts (0x002E9B08), cancels (0x002E9B78) or loads at once (0x002E9BE4) the
// group `id`; false if there is none.
bool StartStreamGroup(uint32_t id);
bool CancelStreamGroup(uint32_t id);
bool LoadStreamGroupNow(uint32_t id);
// Loads at once every group of `type`, except the group `exceptId` (0: all)
// (0x002E9C54).
bool LoadStreamGroupsOfType(uint32_t type, uint32_t exceptId);
// Queues the shared character packs for the cache prefetcher when anim globs
// are enabled (0x002E9D48).
void QueueCharacterPackPrefetch();

// Group type of the character packs (HashNameNoCase("Character")).
constexpr uint32_t kStreamGroup_Character = 0x0353CB09u;
// NOTE: a second type kept by CancelNonCharacterGroups; its name is unknown.
constexpr uint32_t kStreamGroup_Kept = 0x63C971C0u;

} // namespace game
} // namespace sly
