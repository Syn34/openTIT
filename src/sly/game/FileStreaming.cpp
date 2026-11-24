// openTIT - Streamed file groups (0x002E80C0-0x002EC2F8). See FileStreaming.h.
#include "sly/game/FileStreaming.h"

#include "ape/core/ApeString.h"
#include "ape/file/AsyncFile.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/memory/ApeAllocator.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"
#include "ape/tuning/ApeTuning.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace sly {
namespace game {

using namespace ape::file;

namespace {

constexpr const char* kStreamFolder = "Streaming";
constexpr const char* kPackExtension = ".ftd";
constexpr uint32_t kFtdVersion = 1;

// Shared character packs the prefetcher warms the cache with (0x00AD940C).
const char* const kCharacterPacks[] = {
    "Bentley_Animation", "Bentley_Sound", "BentleyEpilogue_Animation",
    "BentleyAllRolledUp_Animation", "Carmelita_Sound", "CarmelitaFox_Animation",
    "CavemanCooper_Animation", "CavemanCooper_Sound", "KidCooper_Animation", "KidCooper_Sound",
    "Murray_Animation", "Murray_Sound", "Rioichi_Animation", "Rioichi_Sound", "Salim_Animation",
    "Salim_Sound", "SirGalleth_Animation", "SirGalleth_Sound", "Sly_Animation", "Sly_Sound",
    "SlyJail", "SlyRobin", "SlySabertooth", "Sly_SamuraiArmor", "Sly_SamuraiArmor_NoReact",
    "SlySinbad",
};

// The request completed (or failed); the console polls the FIOS op's
// completion word.
bool IsDone(const AsyncFileRequest* request)
{
    return GetRequestError(request) != kAsyncFileError_InProgress;
}

void Destroy(AsyncFileRequest*& request)
{
    DestroyRequest(request);
    request = nullptr;
}

uint32_t LoadLE32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

} // namespace

// Module state (0x010686A8-0x010687D1).
struct FileStreamingState {
    ape::ApeTSortedMap<StreamGroup*> groups{ 0x80 };            // 0x010686A8 by group id
    uint32_t groupCount = 0;                                    // 0x010686C0 s_nFileGroups
    ape::ApeTSortedMap<StreamFileCallback> onLoaded{ 4 };       // 0x010686C4 by file type
    ape::ApeTSortedMap<StreamFileCallback> onUnloaded{ 4 };     // 0x010686DC
    ape::ApeTSortedMap<StreamFileFactory> factories{ 4 };       // 0x010686F4
    std::vector<std::string> prefetchQueue;                     // 0x0106870C (capacity 0x20)
    const char* currentPackName = nullptr;                      // 0x01068720 set by ASTH
    char packName[0x41] = {};                                   // 0x010687D0
    AsyncFileRequest* prefetch = nullptr;                       // 0x00F64C34
};

// @func 0x002EBF94 StaticInit_FileStreaming
// @func 0x002EBDB0 StaticDestroy_FileStreaming
// Compiler-generated initialiser / destructor of the globals below.
static FileStreamingState s_streaming;
static ape::TuningBool s_debugDisplayFileStreaming("Debug.Display.FileStreaming", false);
static ape::TuningBool s_useAnimGlobs("Loading.Use Anim Globs", true);

// ---------------------------------------------------------------------------
// StreamDiskFile
// ---------------------------------------------------------------------------

StreamDiskFile::StreamDiskFile()
{
    allocator = ape::GetDefaultAllocator();
}

void StreamDiskFile::ReleaseRequest()
{
    Destroy(request);
}

// @func 0x002E84FC StreamDiskFile::Start
void StreamDiskFile::Start()
{
    if (!name.empty()) {
        request = AsyncOpen(name.c_str(), "rb", AsyncFileParams());
        state = request ? kOpening : kFailed;
    }
    started = true;
}

// @func 0x002E80C0 StreamDiskFile::Update
int StreamDiskFile::Update(bool /*wait*/)
{
    switch (state) {
    case kFailed:
        return kStreamFile_Failed;
    case kDone:
        return kStreamFile_Loaded;
    case kIdle:
        return kStreamFile_Pending;

    case kOpening:
        if (!IsDone(request))
            break;
        ReportRequestError(request);
        handle = GetRequestHandle(request);
        ReleaseRequest();
        if (!handle) {
            state = kFailed;
            return kStreamFile_Failed;
        }
        request = AsyncGetSize(handle, &size, AsyncFileParams());
        state = request ? kSizing : kFailed;
        break;

    case kSizing: {
        if (!IsDone(request))
            break;
        ReportRequestError(request);
        ReleaseRequest();
        if (!buffer)
            buffer = allocator->Alloc((size_t)size, false, 0x10);
        AsyncFileParams params;
        params.flags = readFlags;
        request = AsyncRead(handle, buffer, (uint32_t)size, 0, params);
        state = request ? kReading : kFailed;
        break;
    }

    case kReading:
        if (!IsDone(request))
            break;
        ReportRequestError(request);
        ReleaseRequest();
        request = AsyncClose(handle, AsyncFileParams());
        state = kClosing;
        break;

    case kClosing:
        if (!IsDone(request))
            break;
        ReportRequestError(request);
        ReleaseRequest();
        handle = nullptr;
        state = kDone;
        if (onLoaded)
            onLoaded(id, NameOrNull(), buffer, (uint32_t)size, userData);
        // The callback has taken what it needs.
        if (allocator && buffer) {
            allocator->Free(buffer, (size_t)size);
            buffer = nullptr;
        }
        return kStreamFile_Loaded;
    }
    return kStreamFile_Pending;
}

// @func 0x002E8598 StreamDiskFile::Cancel
void StreamDiskFile::Cancel()
{
    if (request) {
        WaitRequest(request);
        ReportRequestError(request);
        if (!handle && state == kOpening)
            handle = GetRequestHandle(request);
        ReleaseRequest();
    }
    if (handle && state != kClosing) {
        request = AsyncClose(handle, AsyncFileParams());
        WaitRequest(request);
        ReportRequestError(request);
        ReleaseRequest();
    }
    if (allocator && buffer) {
        allocator->Free(buffer, (size_t)size);
        buffer = nullptr;
    }
    if (state == kDone && onUnloaded)
        onUnloaded(id, NameOrNull(), buffer, (uint32_t)size, userData);
    buffer = nullptr;
    handle = nullptr;
    size = 0;
    state = kIdle;
    started = false;
}

// ---------------------------------------------------------------------------
// StreamPackedFile
// ---------------------------------------------------------------------------

void StreamPackedFile::CurrentData(const void** data, uint32_t* size) const
{
    *data = nullptr;
    *size = 0;
    if (group->state == FtdGroup::kReadingEntries) {
        *data = group->buffer;
        *size = group->entries[(size_t)group->currentEntry].size;
        return;
    }
    for (const FtdGroup::Entry& entry : group->entries) {
        if (entry.id == id) {
            *size = entry.size;
            return;
        }
    }
}

// @func 0x002EA218 StreamPackedFile::Update
int StreamPackedFile::Update(bool /*wait*/)
{
    if (!loaded) {
        const void* data;
        uint32_t size;
        CurrentData(&data, &size);
        loaded = true;
        if (onLoaded)
            onLoaded(id, NameOrNull(), data, size, userData);
    }
    return kStreamFile_Loaded;
}

// @func 0x002EA344 StreamPackedFile::Cancel
void StreamPackedFile::Cancel()
{
    if (loaded && onUnloaded) {
        const void* data;
        uint32_t size;
        CurrentData(&data, &size);
        onUnloaded(id, NameOrNull(), data, size, userData);
    }
    loaded = false;
    started = false;
}

// ---------------------------------------------------------------------------
// StreamGroup
// ---------------------------------------------------------------------------

// @func 0x002E87D4 StreamGroup::~StreamGroup
// @func 0x002E8954 StreamGroup::~StreamGroup (deleting)
StreamGroup::~StreamGroup()
{
    for (StreamFile*& file : files) {
        file->Cancel();
        delete file;
        file = nullptr;
    }
    files.clear();
    if (prerequisite) {
        prerequisite->Cancel();
        delete prerequisite;
        prerequisite = nullptr;
    }
}

bool StreamGroup::UpdateFiles(bool wait)
{
    for (StreamFile* file : files) {
        if (!file->started)
            file->Start();
        if (!file->Update(wait))
            return false;
    }
    return true;
}

void StreamGroup::SetComplete(bool complete)
{
    if (complete)
        flags = (uint8_t)((flags & ~kGroup_Running) | kGroup_Complete);
    else
        flags = (uint8_t)(flags & ~kGroup_Complete);
}

// @func 0x002E899C StreamGroup::Update
void StreamGroup::Update(bool wait)
{
    if (!(flags & kGroup_Running) || (flags & kGroup_Complete))
        return;
    bool ready = false;
    if (prerequisite) {
        if (!prerequisite->started)
            prerequisite->Start();
        ready = prerequisite->IsLoaded();
    }
    // NOTE: with a prerequisite that is not loaded yet an empty group still
    // counts as complete (the console compares the files done with the count).
    bool complete = files.empty();
    if (ready || !prerequisite)
        complete = UpdateFiles(wait);
    SetComplete(complete);

    // The prerequisite's bytes are not needed once the files have them.
    if (complete && prerequisite && prerequisite->data && prerequisite->allocator) {
        prerequisite->allocator->Free(prerequisite->data, prerequisite->size);
        prerequisite->data = nullptr;
        prerequisite->field_48 = 0;
        prerequisite->size = 0;
    }
}

// @func 0x002E8BC8 StreamGroup::Start
void StreamGroup::Start(bool wait)
{
    if (flags & kGroup_Complete)
        return;
    flags = (uint8_t)((flags & ~kGroup_Complete) | kGroup_Running);
    if (prerequisite && !prerequisite->started)
        prerequisite->Start();
    do {
        Update(wait);
    } while (!(flags & kGroup_Complete) && wait);
}

// @func 0x002E8CA0 StreamGroup::Cancel
void StreamGroup::Cancel()
{
    flags = (uint8_t)(flags & ~(kGroup_Running | kGroup_Complete));
    for (StreamFile* file : files)
        file->Cancel();
    if (prerequisite)
        prerequisite->Cancel();
}

// ---------------------------------------------------------------------------
// FtdGroup
// ---------------------------------------------------------------------------

// @func 0x002E8DFC FtdGroup::FtdGroup
FtdGroup::FtdGroup()
{
    flags = (uint8_t)(flags & 0x1F);
}

FtdGroup::~FtdGroup()
{
    Cancel();
}

const char* FtdGroup::Path()
{
    if (!path.empty())
        return path.c_str();
    if (handle) {
        const char* open = GetHandlePath(handle);
        if (open && *open)
            return open;
    }
    path = std::string(kStreamFolder) + "/" + name + kPackExtension;
    return path.c_str();
}

void FtdGroup::ReleaseRequest()
{
    Destroy(request);
}

bool FtdGroup::RequestDone() const
{
    return IsDone(request);
}

// The console formats the message into a local buffer that the release build
// never prints; the port logs it.
// @func 0x002EA67C FtdGroup::Fail
void FtdGroup::Fail(const char* fmt, ...)
{
    char message[0x100] = {};
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(message, sizeof message, fmt, args);
    va_end(args);
    plat::Log(plat::LogLevel::Warning, "%s\n", message);
    state = kError;
}

// @func 0x002EA728 FtdGroup::IssueRead
bool FtdGroup::IssueRead(uint32_t bytes, uint64_t offset)
{
    readSize = bytes;
    if (bufferCapacity < bytes) {
        if (buffer) {
            ape::MemFree(buffer);
            buffer = nullptr;
        }
        buffer = static_cast<uint8_t*>(ape::MemAlloc(bytes));
        bufferCapacity = bytes;
    }
    request = AsyncRead(handle, buffer, bytes, offset, AsyncFileParams());
    if (!request) {
        Fail("Creation of async read op of FTD header failed on %s", Path());
        return false;
    }
    return true;
}

// @func 0x002EA92C FtdGroup::FinishRead
bool FtdGroup::FinishRead()
{
    WaitRequestAndReport(request);
    if (GetRequestError(request) != kAsyncFileError_None) {
        Fail("Read of %llu bytes from %s failed: %s", (unsigned long long)readSize, Path(),
             GetRequestErrorName(request));
        return false;
    }
    const uint64_t transferred = GetRequestTransferred(request);
    if (transferred != readSize) {
        Fail("Read of %llu bytes from %s failed: only read %llu bytes", (unsigned long long)readSize,
             Path(), (unsigned long long)transferred);
        return false;
    }
    ReleaseRequest();
    return true;
}

// @func 0x002EB4EC FtdGroup::OnOpened
void FtdGroup::OnOpened()
{
    WaitRequestAndReport(request);
    if (GetRequestError(request) != kAsyncFileError_None) {
        Fail("Async Open of %s failed: %s", Path(), GetRequestErrorName(request));
        return;
    }
    handle = GetRequestHandle(request);
    if (!handle) {
        if (path.empty())
            path = std::string(kStreamFolder) + "/" + name + kPackExtension;
        Fail("Async Open of %s did not return a file handle, loading failed", path.c_str());
        return;
    }
    ReleaseRequest();
    path.clear();   // later messages use the handle's own path
    if (IssueRead(0x18, 0))
        state = kReadingHeader;
}

// Header (24 bytes, little endian): version, data offset, offset table
// position, size table position, unused, entry count.
// @func 0x002EB234 FtdGroup::OnHeaderRead
void FtdGroup::OnHeaderRead()
{
    if (!FinishRead())
        return;
    uint32_t header[6];
    for (int i = 0; i < 6; ++i)
        header[i] = LoadLE32(buffer + i * 4);
    if (header[0] != kFtdVersion) {
        Fail("%s contains an unknown version (%llu)", Path(), (unsigned long long)header[0]);
        return;
    }
    entryCount = header[5];
    sizeTableOffset = header[3];
    dataOffset = header[1];
    if (IssueRead((entryCount & 0x1FFFFFFF) * 8, header[2]))
        state = kReadingOffsets;
}

// (id, offset) pairs; offsets are relative to the data block.
// @func 0x002EB070 FtdGroup::OnOffsetTableRead
void FtdGroup::OnOffsetTableRead()
{
    if (!FinishRead())
        return;
    if (entryCount)
        entries.SetCapacity(entryCount);
    entries.Clear();
    for (uint32_t i = 0; i < entryCount; ++i) {
        const uint8_t* pair = buffer + i * 8;
        Entry entry;
        entry.id = LoadLE32(pair);
        entry.size = 0;
        entry.offset = LoadLE32(pair + 4) + dataOffset;
        entries.PushBack(entry);
    }
    if (IssueRead((entryCount & 0x3FFFFFFF) * 4, sizeTableOffset))
        state = kReadingSizes;
}

void FtdGroup::Finish()
{
    currentEntry = -1;
    state = kDelivering;
    if (buffer) {
        ape::MemFree(buffer);
        buffer = nullptr;
    }
    bufferCapacity = 0;
    readSize = 0;
    sizeTableOffset = 0;
    dataOffset = 0;
    if (handle) {
        CloseFileSync(handle, AsyncFileParams());
        handle = nullptr;
    }
}

// Issues the read of the entry `currentEntry` (or the next one that can be
// read). NOTE: when a read cannot be issued the console overwrites the error
// state and moves on to the next entry; kept.
void FtdGroup::ReadEntriesFrom()
{
    while ((uint32_t)currentEntry < entryCount) {
        const Entry& entry = entries[(size_t)currentEntry];
        if (IssueRead(entry.size, entry.offset))
            return;
        ++currentEntry;
        state = kReadingEntries;
    }
    Finish();
}

// Entry sizes, in table order; the entries are then read in file order into
// one buffer as large as the largest entry.
// @func 0x002EAE24 FtdGroup::OnSizeTableRead
void FtdGroup::OnSizeTableRead()
{
    if (!FinishRead())
        return;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < entryCount; ++i) {
        const uint32_t size = LoadLE32(buffer + i * 4);
        entries[i].size = size;
        largest = std::max(largest, size);
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.offset < b.offset; });
    if (bufferCapacity != largest) {
        if (buffer) {
            ape::MemFree(buffer);
            buffer = nullptr;
        }
        bufferCapacity = largest;
        buffer = static_cast<uint8_t*>(ape::MemAlloc(largest));
    }
    currentEntry = 0;
    state = kReadingEntries;
    ReadEntriesFrom();
}

// Hands the entry just read to the files with its id, then reads the next.
// @func 0x002EAC2C FtdGroup::OnEntryRead
void FtdGroup::OnEntryRead()
{
    if (!FinishRead())
        return;
    const uint32_t entryId = entries[(size_t)currentEntry].id;
    for (StreamFile* file : files) {
        if (file->id != entryId)
            continue;
        if (!file->started)
            file->Start();
        file->Update(false);
    }
    const uint32_t next = (uint32_t)currentEntry + 1;
    if (next < entryCount) {
        currentEntry = (int32_t)next;
        const Entry& entry = entries[next];
        // NOTE: a read that cannot be issued leaves the group waiting on no
        // request, as on the console.
        if (!IssueRead(entry.size, entry.offset))
            state = kReadingEntries;
    } else {
        Finish();
    }
}

// @func 0x002EB7BC FtdGroup::Update
void FtdGroup::Update(bool wait)
{
    for (;;) {
        switch (state) {
        case kIdle:
        case kComplete:
        case kError:
            return;
        case kOpening:
        case kReadingHeader:
        case kReadingOffsets:
        case kReadingSizes:
        case kReadingEntries:
            if (wait)
                WaitRequestAndReport(request);
            if (RequestDone()) {
                ReportRequestError(request);
                switch (state) {
                case kOpening: OnOpened(); break;
                case kReadingHeader: OnHeaderRead(); break;
                case kReadingOffsets: OnOffsetTableRead(); break;
                case kReadingSizes: OnSizeTableRead(); break;
                default: OnEntryRead(); break;
                }
            }
            break;
        case kDelivering: {
            const bool complete = UpdateFiles(wait);
            SetComplete(complete);
            if (complete)
                state = kComplete;
            break;
        }
        }
        if (!wait || state == kIdle || state == kComplete || state == kError)
            return;
    }
}

// @func 0x002EBAF8 FtdGroup::Start
void FtdGroup::Start(bool wait)
{
    if (state == kComplete || state == kError)
        return;
    if (state != kIdle) {
        if (wait)
            Update(true);
        return;
    }
    const char* file = Path();
    request = AsyncOpen(file, "rb", AsyncFileParams());
    if (!request) {
        Fail("Failed to open %s", file);
        return;
    }
    state = kOpening;
    StreamGroup::Start(wait);
}

// @func 0x002EA464 FtdGroup::Cancel
void FtdGroup::Cancel()
{
    StreamGroup::Cancel();
    if (request) {
        CancelRequest(request);
        WaitRequestAndReport(request);
        if (state == kOpening) {
            AsyncFileHandle* opened = GetRequestHandle(request);
            if (opened && opened != handle)
                CloseFileSync(opened, AsyncFileParams());
        }
        ReleaseRequest();
    }
    if (handle) {
        CloseFileSync(handle, AsyncFileParams());
        handle = nullptr;
    }
    if (buffer) {
        ape::MemFree(buffer);
        buffer = nullptr;
    }
    bufferCapacity = 0;
    entries.Release();
    entryCount = 0;
    state = kIdle;
    currentEntry = -1;
}

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

// @func 0x002E9FE0 RegisterStreamType
void RegisterStreamType(uint32_t type, StreamFileCallback onLoaded, StreamFileCallback onUnloaded,
                        StreamFileFactory factory)
{
    if (!*s_streaming.onLoaded.Find(type))
        s_streaming.onLoaded.Add(type, onLoaded);
    if (!*s_streaming.onUnloaded.Find(type))
        s_streaming.onUnloaded.Add(type, onUnloaded);
    if (!*s_streaming.factories.Find(type))
        s_streaming.factories.Add(type, factory);
}

static void AddGroup(StreamGroup* group)
{
    s_streaming.groups.Add(group->id, group);
}

// ASTD payload: the group table.
// @func 0x002E8EB4 ReadStreamGroups
static void ReadStreamGroups(ape::iff::IffReader* reader)
{
    uint32_t version = 0;
    reader->ReadU32(&version);
    reader->ReadU32(&s_streaming.groupCount);
    for (uint32_t g = 0; g < s_streaming.groupCount; ++g) {
        FtdGroup* group = new FtdGroup();
        if (version == 2) {
            uint32_t nameSize = 0;
            reader->ReadU32(&nameSize);
            std::string name(nameSize, '\0');
            reader->Read(name.data(), nameSize);
            group->name = name.c_str();
            group->flags |= StreamGroup::kGroup_Named;
            group->id = ape::HashNameNoCase(group->name.c_str());
        } else if (version == 1) {
            reader->ReadU32(&group->id);
            group->flags &= (uint8_t)~StreamGroup::kGroup_Named;
        }
        reader->ReadU32(&group->type);
        uint32_t fileCount = 0;
        reader->ReadU32(&fileCount);
        group->files.reserve(fileCount);
        for (uint32_t f = 0; f < fileCount; ++f) {
            uint32_t fileId = 0, fileType = 0, nameSize = 0;
            reader->ReadU32(&fileId);
            reader->ReadU32(&fileType);
            StreamFileFactory factory = *s_streaming.factories.Find(fileType);
            StreamFile* file = factory ? factory() : new StreamPackedFile(group);
            file->id = fileId;
            file->type = fileType;
            reader->ReadU32(&nameSize);
            std::string name(nameSize, '\0');
            reader->Read(name.data(), nameSize);
            file->name = name.c_str();
            file->onLoaded = *s_streaming.onLoaded.Find(fileType);
            file->onUnloaded = *s_streaming.onUnloaded.Find(fileType);
            group->files.push_back(file);
        }
        AddGroup(group);
    }
}

// "ASTF": the stream table form; its children follow.
// @func 0x002E9E84 ChunkStreamTable
static int ChunkStreamTable(ape::iff::IffReader* reader, void* /*arg*/)
{
    s_streaming.currentPackName = nullptr;
    reader->ParseChunks(nullptr);
    return 1;
}

// "ASTH": header with the table's name (a length byte and 64 characters).
// @func 0x002E9EBC ChunkStreamHeader
static int ChunkStreamHeader(ape::iff::IffReader* reader, void* /*arg*/)
{
    uint8_t length = 0;
    reader->ReadU8(&length);
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&s_streaming.packName[1 + i]));
    s_streaming.currentPackName = &s_streaming.packName[1];
    return 1;
}

// "ASTD": the groups.
// @func 0x002E9F88 ChunkStreamData
static int ChunkStreamData(ape::iff::IffReader* reader, void* /*arg*/)
{
    (void)reader->GetChunkSize();
    ReadStreamGroups(reader);
    s_streaming.currentPackName = nullptr;
    return 1;
}

// @func 0x002E8D74 RegisterStreamChunkHandlers
void RegisterStreamChunkHandlers()
{
    using ape::iff::MakeId;
    ape::iff::RegisterChunkHandler(MakeId('A', 'S', 'T', 'F'), ChunkStreamTable, 0, nullptr, 0);
    ape::iff::RegisterChunkHandler(MakeId('A', 'S', 'T', 'H'), ChunkStreamHeader, 0, nullptr, 0);
    ape::iff::RegisterChunkHandler(MakeId('A', 'S', 'T', 'D'), ChunkStreamData, 0, nullptr, 0);
}

// @func 0x002E9338 CreateStreamGroupsFromLevel
void CreateStreamGroupsFromLevel(const ape::ApeLevelDef* level)
{
    if (!level || s_streaming.groupCount != 0)
        return;
    s_streaming.groupCount = (uint32_t)level->asyncFileGroups.size();
    for (const auto& source : level->asyncFileGroups) {
        StreamGroup* group = new FtdGroup();
        group->id = source->idHash;
        group->type = source->typeHash;
        group->files.reserve(source->files.size());
        for (const ape::LevelAsyncFile& entry : source->files) {
            StreamDiskFile* file = new StreamDiskFile();
            file->name = entry.fileName;
            file->id = entry.idHash;
            file->onLoaded = *s_streaming.onLoaded.Find(entry.typeHash);
            file->onUnloaded = *s_streaming.onUnloaded.Find(entry.typeHash);
            group->files.push_back(file);
        }
        AddGroup(group);
    }
}

// @func 0x002E9688 UpdateFileStreaming
void UpdateFileStreaming()
{
    for (uint32_t i = 0; i < s_streaming.groupCount; ++i)
        s_streaming.groups.Data()[i].value->Update(false);

    if (!s_streaming.prefetch) {
        if (s_streaming.prefetchQueue.empty())
            return;
        const std::string path = s_streaming.prefetchQueue.back();
        s_streaming.prefetchQueue.pop_back();
        AsyncFileParams params;
        params.flags = kAsyncFileFlag_Quiet;
        params.urgent = false;
        s_streaming.prefetch =
            AsyncPrefetch(path.c_str(), 0, (uint64_t)std::numeric_limits<int64_t>::max(), params);
        return;
    }
    if (IsDone(s_streaming.prefetch)) {
        // (The console prepares a message for unexpected errors here that the
        // release build does not print.)
        ReportRequestError(s_streaming.prefetch);
        Destroy(s_streaming.prefetch);
    }
}

// @func 0x002E98CC ClearStreamGroups
void ClearStreamGroups()
{
    for (int i = 0; i < (int)s_streaming.groups.Count(); ++i)
        delete s_streaming.groups.Data()[i].value;
    s_streaming.groups.Clear();
    s_streaming.groupCount = 0;
    s_streaming.prefetchQueue.clear();
    if (s_streaming.prefetch)
        Destroy(s_streaming.prefetch);
}

// @func 0x002E9A28 CancelNonCharacterGroups
void CancelNonCharacterGroups()
{
    for (uint32_t i = 0; i < s_streaming.groupCount; ++i) {
        StreamGroup* group = s_streaming.groups.Data()[i].value;
        if (group->type != kStreamGroup_Character && group->type != kStreamGroup_Kept)
            group->Cancel();
    }
}

// @func 0x002E9B08 StartStreamGroup
bool StartStreamGroup(uint32_t id)
{
    StreamGroup* group = *s_streaming.groups.Find(id);
    if (group)
        group->Start(false);
    return group != nullptr;
}

// @func 0x002E9B78 CancelStreamGroup
bool CancelStreamGroup(uint32_t id)
{
    StreamGroup* group = *s_streaming.groups.Find(id);
    if (group)
        group->Cancel();
    return group != nullptr;
}

// @func 0x002E9BE4 LoadStreamGroupNow
bool LoadStreamGroupNow(uint32_t id)
{
    StreamGroup* group = *s_streaming.groups.Find(id);
    if (group)
        group->Start(true);
    return group != nullptr;
}

// @func 0x002E9C54 LoadStreamGroupsOfType
bool LoadStreamGroupsOfType(uint32_t type, uint32_t exceptId)
{
    for (uint32_t i = 0; i < s_streaming.groupCount; ++i) {
        StreamGroup* group = s_streaming.groups.Data()[i].value;
        if (group->type == type && (exceptId == 0 || group->id != exceptId))
            group->Start(true);
    }
    return true;
}

// @func 0x002E9D48 QueueCharacterPackPrefetch
void QueueCharacterPackPrefetch()
{
    if (!s_useAnimGlobs.Get())
        return;
    for (const char* pack : kCharacterPacks)
        s_streaming.prefetchQueue.push_back(std::string(kStreamFolder) + "/" + pack + kPackExtension);
}

} // namespace game
} // namespace sly
