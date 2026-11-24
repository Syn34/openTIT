// openTIT - Tag blocks: packed parameter tables attached to world objects.
//
// Level files can attach parameters ("tags") to a definition or instance
// with "TAG2" chunks (and the older "ATAG" form). Each chunk becomes one
// ApeTagData: a reference counted, packed key/value table. Up to four of
// them are collected in the object's ApeTagBlock; objects created as copies
// share the block.
//
// Packed layout of a table (offsets from the start of the data block):
//   keys     u32[count]   name hashes of the parameters
//   offsets  u16[count]   offset of every value inside the value area
//   values   ...          the parameter values themselves
#pragma once

#include <cstdint>

namespace ape::iff {
struct IffReader;
}

namespace ape::world {

class ApeBase;

// Header of a packed table as stored in a TAG2 chunk.
struct ApeTagHeader {
    uint32_t id = 0;            // NOTE: tag set id (a name hash in ATAG data)
    uint32_t field_04 = 0;      // NOTE: always 0 in ATAG data
    uint16_t count = 0;         // number of parameters
    uint16_t keysOffset = 0;
    uint16_t valueOffsetsOffset = 0;
    uint16_t valuesOffset = 0;
    uint16_t field_10 = 0;      // NOTE: read from the chunk, not used
    uint16_t field_12 = 0;
    uint32_t field_14 = 0;
    uint32_t field_18 = 0;
};

struct ApeTagData {
    int16_t refCount = 1;
    uint32_t field_04 = 0;                  // header field_04
    uint32_t id = 0;                        // header id
    uint16_t count = 0;
    const uint32_t* keys = nullptr;
    const uint16_t* valueOffsets = nullptr;
    const uint8_t* values = nullptr;
    uint8_t* data = nullptr;                // owned copy of the packed table
    uint32_t size = 0;

    // Takes the header and a copy of `size` bytes of packed data.
    void Init(const ApeTagHeader& header, const void* packed, uint32_t packedSize);

    void AddRef() { ++refCount; }
    // Drops a reference; frees the table and the object at zero.
    void Release();
};

// Replaces a counted tag data reference.
void AssignTagData(ApeTagData*& slot, ApeTagData* value);

struct ApeTagBlock {
    static constexpr int kMaxEntries = 4;

    int16_t refCount = 1;
    ApeTagData* entries[kMaxEntries] = {};
    uint8_t count = 0;

    void AddRef() { ++refCount; }
    // Drops a reference; at zero releases every table and frees the block.
    void Release();
    // Appends a table (taking a reference).
    void Add(ApeTagData* data);
};

// Replaces a counted tag block reference.
void AssignTagBlock(ApeTagBlock*& slot, ApeTagBlock* value);

// Chunk handler for "TAG2": `arg` is the object (ApeBase) being loaded.
int Tag2ChunkHandler(iff::IffReader* reader, void* arg);

// Registers the tag chunk handlers and creates the shared tag table cache.
bool InitTagSystem();
// Releases every table held by the shared cache.
void ReleaseCachedTags();

} // namespace ape::world
