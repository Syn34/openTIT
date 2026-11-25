// openTIT - IFF chunk handler registry and chunk reader helpers.
//
// Engine data files are IFF style chunk streams. Every chunk id (FourCC) is
// dispatched to a handler registered with RegisterChunkHandler(). A handler
// receives the reader, whose chunk stack describes the chunk being parsed,
// and pulls the payload with the Read* helpers. Payload integers are stored
// little-endian.
//
// Objects created while a file is loaded are collected in a list
// (BeginLoadedObjectList / AddLoadedObject) and can also be put in one of two
// numbered slots for chunks that refer to an earlier object.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::file {
class File;
}

namespace ape::iff {

// Builds a chunk id from its four characters in file order.
constexpr uint32_t MakeId(char a, char b, char c, char d)
{
    return (uint32_t)(uint8_t)a | ((uint32_t)(uint8_t)b << 8) | ((uint32_t)(uint8_t)c << 16) |
           ((uint32_t)(uint8_t)d << 24);
}

struct IffChunk {
    uint32_t id;
    uint32_t start;   // file offset of the payload
    uint32_t size;    // payload size
};

// Chunks currently open, outermost first.
struct IffChunkStack {
    static constexpr int kMaxDepth = 20;
    int32_t capacity = kMaxDepth;
    int32_t top = -1;            // index of the current chunk, -1 when empty
    IffChunk chunks[kMaxDepth] = {};
};

enum IffReaderFlags : uint8_t {
    kIffReader_Flag02 = 0x02,    // copied from g_iffReaderFlag; NOTE: meaning unknown
};

struct IffReader {
    IffChunkStack* stack = nullptr;
    file::File* file = nullptr;
    uint32_t bytesRead = 0;      // also the file offset parsing has reached
    uint8_t flags = 0;

    // Creates / frees the chunk stack.
    void Init();
    void Destroy();
    // Attaches a file without parsing it.
    bool Open(file::File* file, uint8_t flags);
    bool Close();
    // Parses a whole file, passing `arg` to every chunk handler.
    bool Parse(file::File* file, void* arg);
    // Dispatches chunks to their handlers until the enclosing chunk (or the
    // file) is exhausted, a handler fails or no further chunk header can be
    // read. Returns the last handler result, false when no chunk was read.
    bool ParseChunks(void* arg);
    // Reads a chunk header and opens the chunk. Returns null at the end of
    // the file.
    IffChunk* PushChunk();

    // Reads raw bytes and counts them.
    void Read(void* dst, uint32_t size);
    void ReadU8(uint8_t* value);
    void ReadU16(uint16_t* value);
    void ReadU32(uint32_t* value);
    void ReadFloat(float* value);
    void ReadVec3(float value[3]);
    void ReadVec4(float value[4]);
    // Raw copies (no byte order conversion).
    void ReadRaw4(void* dst);
    void ReadRaw32(void* dst);

    const IffChunk* CurrentChunk() const;
    // Size of the current chunk (0 when there is none).
    uint32_t GetChunkSize() const;
    // Bytes of the current chunk not read yet.
    uint32_t GetChunkRemaining() const;
};

using ChunkHandler = int (*)(IffReader* reader, void* arg);

// Called for chunk ids without a handler; parsing stops after it (0x00F8870C).
using UnknownChunkHandler = void (*)(IffReader* reader, uint32_t id);
void SetUnknownChunkHandler(UnknownChunkHandler handler);
// Called each time a top-level chunk has been parsed.
// NOTE: on the console this is the frame clock's progress callback (0x005DBB60).
void SetTopLevelChunkHook(void (*hook)(void* arg), void* arg);
// Default for IffReader::flags bit 1 (0x01164E4C).
extern bool g_iffReaderFlag;

// Handler of an "include" chunk: the payload names another file, relative to
// the directory of the current one, which is parsed with a fresh reader.
// The resolved path is copied to `outPath` when it is not null.
bool ParseIncludedFile(IffReader* reader, char* outPath);

enum ChunkFlags : uint32_t {
    kChunk_QuietReplace = 0x08,   // no message when an existing handler is replaced
    kChunk_Flag10 = 0x10,         // NOTE: stored with the handler, meaning unknown
};

struct ChunkDesc {
    uint32_t id;
    ChunkHandler handler;
    uint32_t param;
    const char* name;
    uint32_t flags;               // only kChunk_Flag10 is kept
};

// Registers (or replaces) the handler of a chunk id.
void RegisterChunkHandler(uint32_t id, ChunkHandler handler, uint32_t param, const char* name, uint32_t flags);
// Handler description for a chunk id, or null.
const ChunkDesc* FindChunkHandler(uint32_t id);

// Handler of 'TAGG' chunks: accepts the chunk without reading it (the parser
// then skips its payload).
int TaggChunkHandler(IffReader* reader, void* arg);

// Loaded object bookkeeping.
void BeginLoadedObjectList(ApeTArray<void*>* list);
void ClearLoadedObjectSlots();
void AddLoadedObject(int slot, void* object);

// Base of objects created by chunk handlers and owned by the loaded object
// list (vtable 0x00B24110): a block of data and its size.
class ApeLoadedData {
public:
    ApeLoadedData() = default;
    virtual ~ApeLoadedData();
    ApeLoadedData(const ApeLoadedData&) = delete;
    ApeLoadedData& operator=(const ApeLoadedData&) = delete;

    uint32_t size = 0;      // +0x04
    void* data = nullptr;   // +0x08
};
void* GetLoadedObjectSlot(int slot);

} // namespace ape::iff
