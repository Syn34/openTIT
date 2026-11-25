// openTIT - IFF chunk handler registry and chunk reader helpers.
#include "ape/sys/ApeIff.h"

#include "ape/file/ApeFile.h"
#include "platform/Platform.h"

#include <cstring>
#include <map>
#include <string>

namespace ape::iff {

namespace {

// NOTE: the console keeps the descriptors in a tree at 0x01164E30 with nodes
// from ApeTListAllocator<IFF::ChunkDesc>; a std::map does the same job.
std::map<uint32_t, ChunkDesc>& Registry()
{
    static std::map<uint32_t, ChunkDesc> s_registry;
    return s_registry;
}

// Current loaded object list and slots (0x01164DF4, 0x01164DEC).
ApeTArray<void*>* s_objectList = nullptr;
void* s_objectSlots[2] = {};

UnknownChunkHandler s_unknownChunkHandler = nullptr;
void (*s_topLevelHook)(void*) = nullptr;
void* s_topLevelHookArg = nullptr;

uint32_t LE32(const uint8_t* b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

char IdChar(uint32_t id, int i)
{
    return (char)((id >> (8 * i)) & 0xFF);
}

} // namespace

// @func 0x0055902C RegisterChunkHandler
void RegisterChunkHandler(uint32_t id, ChunkHandler handler, uint32_t param, const char* name, uint32_t flags)
{
    std::map<uint32_t, ChunkDesc>& registry = Registry();
    const auto existing = registry.find(id);
    if (existing != registry.end()) {
        if (!(flags & kChunk_QuietReplace)) {
            plat::Log(plat::LogLevel::Warning, "Replacing %c%c%c%c (%s) with %s\n", IdChar(id, 0), IdChar(id, 1),
                      IdChar(id, 2), IdChar(id, 3), existing->second.name ? existing->second.name : "",
                      name ? name : "");
        }
        registry.erase(existing);
    }
    registry[id] = ChunkDesc{ id, handler, param, name, flags & kChunk_Flag10 };
}

const ChunkDesc* FindChunkHandler(uint32_t id)
{
    const std::map<uint32_t, ChunkDesc>& registry = Registry();
    const auto it = registry.find(id);
    return it != registry.end() ? &it->second : nullptr;
}

bool g_iffReaderFlag = false;

void SetUnknownChunkHandler(UnknownChunkHandler handler)
{
    s_unknownChunkHandler = handler;
}

void SetTopLevelChunkHook(void (*hook)(void* arg), void* arg)
{
    s_topLevelHook = hook;
    s_topLevelHookArg = arg;
}

// --- reader -------------------------------------------------------------------------

// @func 0x00569280 IffReader::Init
void IffReader::Init()
{
    file = nullptr;
    stack = new IffChunkStack();
}

// @func 0x005692E8 IffReader::Destroy
void IffReader::Destroy()
{
    delete stack;
    stack = nullptr;
}

// @func 0x00569338 IffReader::Open
bool IffReader::Open(file::File* newFile, uint8_t newFlags)
{
    file = newFile;
    bytesRead = 0;
    flags = (uint8_t)((newFlags & ~kIffReader_Flag02) | (g_iffReaderFlag ? kIffReader_Flag02 : 0));
    return true;
}

// @func 0x00569378 IffReader::Close
bool IffReader::Close()
{
    file = nullptr;
    return true;
}

// @func 0x005695C4 IffReader::Parse
bool IffReader::Parse(file::File* newFile, void* arg)
{
    Open(newFile, 0);
    ParseChunks(arg);
    file = nullptr;
    return true;
}

// @func 0x00569108 IffReader::PushChunk
IffChunk* IffReader::PushChunk()
{
    uint8_t header[8];
    const uint32_t got = file->Read(header, sizeof header);
    bytesRead += got;
    if (got == 0)
        return nullptr;

    const uint32_t start = (uint32_t)file->GetPosition();
    if (stack->top < stack->capacity) {
        IffChunk& chunk = stack->chunks[++stack->top];
        chunk.id = LE32(header);
        chunk.start = start;
        chunk.size = LE32(header + 4) - 4;
    }
    return CurrentChunk() ? &stack->chunks[stack->top] : nullptr;
}

// @func 0x00569388 IffReader::ParseChunks
bool IffReader::ParseChunks(void* arg)
{
    bool result = true;
    IffChunk* chunk = nullptr;
    do {
        if (!result)
            break;
        chunk = PushChunk();
        if (!chunk)
            break;

        if (const ChunkDesc* desc = FindChunkHandler(chunk->id)) {
            result = desc->handler(this, arg) != 0;
        } else if (s_unknownChunkHandler) {
            s_unknownChunkHandler(this, chunk->id);
            result = false;
        }

        // Close the chunk and continue right after it, whatever the handler read.
        // NOTE: with an empty stack the console reuses the first entry.
        const IffChunk& done = stack->chunks[stack->top >= 0 ? stack->top : 0];
        const uint32_t end = done.start + done.size;
        if (stack->top >= 0)
            --stack->top;
        if (stack->top == 0 && s_topLevelHook)
            s_topLevelHook(s_topLevelHookArg);
        file->Seek(end, file::kSeek_Set);
        bytesRead = end;
    } while (GetChunkRemaining() != 0);

    return chunk ? result : false;
}

// @func 0x00563768 IffReader::Read
void IffReader::Read(void* dst, uint32_t size)
{
    bytesRead += file->Read(dst, size);
}

// @func 0x005643C4 IffReader::ReadU8
// @func 0x0056998C IffReader::ReadU8 (copy)
void IffReader::ReadU8(uint8_t* value)
{
    Read(value, 1);
}

// @func 0x005698F4 IffReader::ReadFloat
void IffReader::ReadFloat(float* value)
{
    uint8_t bytes[4] = { 0, 0, 0, 0 };
    Read(bytes, 4);
    const uint32_t bits = LE32(bytes);
    std::memcpy(value, &bits, sizeof bits);
}

// @func 0x005699F0 IffReader::ReadVec3
void IffReader::ReadVec3(float value[3])
{
    uint8_t bytes[12] = {};
    Read(bytes, sizeof bytes);
    for (int i = 0; i < 3; ++i) {
        const uint32_t bits = LE32(bytes + 4 * i);
        std::memcpy(&value[i], &bits, sizeof bits);
    }
}

// @func 0x00569AE8 IffReader::ReadVec4
void IffReader::ReadVec4(float value[4])
{
    uint8_t bytes[16] = {};
    Read(bytes, sizeof bytes);
    for (int i = 0; i < 4; ++i) {
        const uint32_t bits = LE32(bytes + 4 * i);
        std::memcpy(&value[i], &bits, sizeof bits);
    }
}

// @func 0x00569C74 IffReader::ReadRaw4
void IffReader::ReadRaw4(void* dst)
{
    Read(dst, 4);
}

// @func 0x00569C10 IffReader::ReadRaw32
void IffReader::ReadRaw32(void* dst)
{
    Read(dst, 32);
}

// @func 0x00564340 IffReader::ReadU16
void IffReader::ReadU16(uint16_t* value)
{
    uint8_t bytes[2] = { 0, 0 };
    Read(bytes, 2);
    *value = (uint16_t)(bytes[0] | (bytes[1] << 8));
}

// @func 0x005642AC IffReader::ReadU32
void IffReader::ReadU32(uint32_t* value)
{
    uint8_t bytes[4] = { 0, 0, 0, 0 };
    Read(bytes, 4);
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
             ((uint32_t)bytes[3] << 24);
}

const IffChunk* IffReader::CurrentChunk() const
{
    return (stack && stack->top >= 0) ? &stack->chunks[stack->top] : nullptr;
}

// NOTE: the console reads a wild address when the stack is empty.
// @func 0x005637D4 IffReader::GetChunkSize
uint32_t IffReader::GetChunkSize() const
{
    const IffChunk* chunk = CurrentChunk();
    return chunk ? chunk->size : 0;
}

// @func 0x0056424C IffReader::GetChunkRemaining
uint32_t IffReader::GetChunkRemaining() const
{
    const IffChunk* chunk = CurrentChunk();
    if (!chunk)
        return 0;
    return chunk->start + chunk->size - (uint32_t)file->GetPosition();
}

// @func 0x00569638 ParseIncludedFile
bool ParseIncludedFile(IffReader* reader, char* outPath)
{
    // The payload is the file name.
    char name[256] = {};
    uint32_t size = reader->GetChunkSize();
    if (size > sizeof name - 1)
        size = sizeof name - 1;   // NOTE: the console does not bound the read
    reader->Read(name, size);

    // Directory of the current file, separator included.
    char path[256];
    const char* current = reader->file->GetName();
    int cut = (int)std::strlen(current) - 1;
    while (cut >= 0 && current[cut] != '/' && current[cut] != ':' && current[cut] != '\\')
        --cut;
    const size_t dirLength = cut > 0 ? (size_t)cut + 1 : 0;
    const std::string joined = std::string(current, dirLength) + name;
    const size_t length = joined.size() < sizeof path - 1 ? joined.size() : sizeof path - 1;
    std::memcpy(path, joined.data(), length);
    path[length] = '\0';
    if (outPath)
        std::strcpy(outPath, path);

    file::File* included = file::OpenFile(path, "rb");
    if (!included)
        return false;
    IffReader nested;
    nested.Init();
    nested.Open(included, 0);
    nested.ParseChunks(nullptr);
    included->Close();
    nested.Destroy();
    return true;
}

// @func 0x00564C54 TaggChunkHandler
int TaggChunkHandler(IffReader* /*reader*/, void* /*arg*/)
{
    return 1;
}

// --- loaded objects -------------------------------------------------------------------

// @func 0x00564F44 BeginLoadedObjectList
void BeginLoadedObjectList(ApeTArray<void*>* list)
{
    s_objectSlots[0] = nullptr;
    s_objectSlots[1] = nullptr;
    s_objectList = list;
}

// @func 0x007A39B4 ApeLoadedData::~ApeLoadedData
// @func 0x007A39C4 ApeLoadedData::~ApeLoadedData (deleting)
ApeLoadedData::~ApeLoadedData() = default;

// @func 0x00564F2C ClearLoadedObjectSlots
void ClearLoadedObjectSlots()
{
    s_objectSlots[0] = nullptr;
    s_objectSlots[1] = nullptr;
}

// @func 0x00564F60 AddLoadedObject
void AddLoadedObject(int slot, void* object)
{
    s_objectSlots[slot] = object;
    s_objectList->PushBack(object);
}

void* GetLoadedObjectSlot(int slot)
{
    return s_objectSlots[slot];
}

} // namespace ape::iff
