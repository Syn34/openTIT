// openTIT - Tag blocks: packed parameter tables attached to world objects.
#include "ape/world/ApeTags.h"

#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeBase.h"

#include <cstring>
#include <memory>
#include <vector>

namespace ape::world {

namespace {

constexpr uint32_t kTag2Chunk = iff::MakeId('T', 'A', 'G', '2');
constexpr uint32_t kTaggChunk = iff::MakeId('T', 'A', 'G', 'G');

// Shared cache of tables (0x01164DE8): groups of (key, table) pairs, both
// levels kept as arrays. NOTE: the code that fills it is not ported; only
// its creation and release are.
struct TagCacheGroup {
    uint32_t key = 0;
    std::vector<std::pair<uint32_t, ApeTagData*>> tables;
};
std::unique_ptr<std::vector<std::pair<uint32_t, TagCacheGroup*>>> s_tagCache;

} // namespace

// @func 0x00558480 ApeTagData::Init
void ApeTagData::Init(const ApeTagHeader& header, const void* packed, uint32_t packedSize)
{
    field_04 = header.field_04;
    count = header.count;
    id = header.id;
    data = nullptr;
    if (packedSize != 0) {
        data = static_cast<uint8_t*>(SysAlloc(16, packedSize, 0, 0));
        std::memcpy(data, packed, packedSize);
    }
    keys = reinterpret_cast<const uint32_t*>(data + header.keysOffset);
    valueOffsets = reinterpret_cast<const uint16_t*>(data + header.valueOffsetsOffset);
    values = data + header.valuesOffset;
    size = packedSize;
}

// @func 0x00557C80 ApeTagData::Release
void ApeTagData::Release()
{
    if (--refCount < 1) {
        SysFree(data, 0, 0);
        delete this;
    }
}

void AssignTagData(ApeTagData*& slot, ApeTagData* value)
{
    if (slot == value)
        return;
    ApeTagData* old = slot;
    slot = value;
    if (value)
        value->AddRef();
    if (old)
        old->Release();
}

void ApeTagBlock::Release()
{
    if (--refCount >= 1)
        return;
    for (int i = kMaxEntries - 1; i >= 0; --i)
        if (entries[i])
            entries[i]->Release();
    delete this;
}

void ApeTagBlock::Add(ApeTagData* tagData)
{
    // NOTE: the console does not check the capacity; a fifth table would
    // overwrite the count. Extra tables are dropped here.
    if (count >= kMaxEntries)
        return;
    AssignTagData(entries[count++], tagData);
}

void AssignTagBlock(ApeTagBlock*& slot, ApeTagBlock* value)
{
    if (slot == value)
        return;
    ApeTagBlock* old = slot;
    slot = value;
    if (value)
        value->AddRef();
    if (old)
        old->Release();
}

// A version byte, the table header, then the packed table filling the rest of
// the chunk. The table is appended to the object's tag block (created on
// first use).
// @func 0x00564428 Tag2ChunkHandler
int Tag2ChunkHandler(iff::IffReader* reader, void* arg)
{
    auto* object = static_cast<ApeBase*>(arg);

    uint8_t version = 0;
    reader->Read(&version, 1);
    ApeTagHeader header;
    reader->Read(&header.id, 4);
    reader->Read(&header.field_04, 4);
    reader->Read(&header.count, 2);
    reader->Read(&header.keysOffset, 2);
    reader->Read(&header.valueOffsetsOffset, 2);
    reader->Read(&header.valuesOffset, 2);
    reader->Read(&header.field_10, 2);
    reader->Read(&header.field_12, 2);
    reader->Read(&header.field_14, 4);
    reader->Read(&header.field_18, 4);

    const uint32_t packedSize = reader->GetChunkRemaining();
    ApeMemStack* stack = GetMemStack();
    void* packed = stack->Alloc(packedSize);
    reader->Read(packed, packedSize);

    auto* tagData = new ApeTagData();
    tagData->Init(header, packed, packedSize);

    if (!object->GetTags()) {
        auto* block = new ApeTagBlock();
        object->SetTags(block);
        block->Release();   // the object holds the only reference now
    }
    object->GetTags()->Add(tagData);
    tagData->Release();

    stack->Free(packed);
    return 1;
}

// NOTE: 'ATAG' (0x00563808, older packed form built through 0x00563368)
// is not ported yet; it is left unregistered.
// @func 0x00559310 InitTagSystem
bool InitTagSystem()
{
    iff::RegisterChunkHandler(kTag2Chunk, Tag2ChunkHandler, 0, nullptr, 0);
    iff::RegisterChunkHandler(kTaggChunk, iff::TaggChunkHandler, 0, nullptr, 0);
    s_tagCache = std::make_unique<std::vector<std::pair<uint32_t, TagCacheGroup*>>>();
    s_tagCache->reserve(32);
    return true;
}

// Releases the tables of every cache group (last first) and the groups.
// @func 0x00564C64 ReleaseCachedTags
void ReleaseCachedTags()
{
    if (!s_tagCache)
        return;
    while (!s_tagCache->empty()) {
        TagCacheGroup* group = s_tagCache->back().second;
        while (!group->tables.empty()) {
            group->tables.back().second->Release();
            group->tables.pop_back();
        }
        delete group;
        s_tagCache->pop_back();
    }
}

} // namespace ape::world
