// openTIT - Agl level chunk handlers for textures and mesh topology headers.
//
// Level files are IFF chunk streams (ape::iff). These handlers read the
// texture and topology description chunks; the reader's flag bit 2 marks a
// "resolve only" pass in which textures are looked up instead of created.
#include "agl/AglResources.h"
#include "agl/AglTexture.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"

#include <cstdio>
#include <cstring>

namespace agl {

namespace {

constexpr uint8_t kReaderResolveOnly = 0x04;

void (*g_textureCreateHook)(AglTexture*, void*, int) = nullptr;   // 0x00F8F118 + 8
void* g_textureCreateUserdata = nullptr;                           // 0x00F8F118 + 0xC
bool g_defaultForLooseTextures = false;                            // 0x010C5AC0 bit 7

} // namespace

void SetTextureCreateHook(void (*hook)(AglTexture*, void*, int), void* userdata)
{
    g_textureCreateHook = hook;
    g_textureCreateUserdata = userdata;
}

void SetUseDefaultForLooseTextures(bool enable)
{
    g_defaultForLooseTextures = enable;
}

// Raw data chunks copied into RSX local memory (memory class 4): "TBLK"
// texture blocks (vtable 0x00B24120, loaded object slot 0) and "MBLK"
// blocks (vtable 0x00B24180, slot 1).
class AglTextureBlock : public ape::iff::ApeLoadedData {
public:
    explicit AglTextureBlock(uint32_t bytes);
    ~AglTextureBlock() override;
};

class AglMemoryBlock : public ape::iff::ApeLoadedData {
public:
    explicit AglMemoryBlock(uint32_t bytes);
    ~AglMemoryBlock() override;
};

AglTextureBlock::AglTextureBlock(uint32_t bytes)
{
    data = ape::SysAlloc(0x80, bytes, ape::kMemClass_HeapA, 1);
    size = bytes;
}

// @func 0x007A3A08 AglTextureBlock::~AglTextureBlock
// @func 0x007A3A60 AglTextureBlock::~AglTextureBlock (deleting)
AglTextureBlock::~AglTextureBlock()
{
    ape::SysFree(data, ape::kMemClass_HeapA, 1);
}

AglMemoryBlock::AglMemoryBlock(uint32_t bytes)
{
    data = ape::SysAlloc(0x80, bytes, ape::kMemClass_HeapA, 1);
    size = bytes;
}

// @func 0x007A3B1C AglMemoryBlock::~AglMemoryBlock
// @func 0x007A3B74 AglMemoryBlock::~AglMemoryBlock (deleting)
AglMemoryBlock::~AglMemoryBlock()
{
    ape::SysFree(data, ape::kMemClass_HeapA, 1);
}

// @func 0x00542630 TextureBlockChunkHandler
int TextureBlockChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    const uint32_t bytes = reader->GetChunkSize();
    auto* block = new AglTextureBlock(bytes);
    ape::iff::AddLoadedObject(0, block);
    reader->Read(block->data, bytes);
    return 1;
}

// @func 0x00548C00 MemoryBlockChunkHandler
int MemoryBlockChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    const uint32_t bytes = reader->GetChunkSize();
    auto* block = new AglMemoryBlock(bytes);
    ape::iff::AddLoadedObject(1, block);
    reader->Read(block->data, bytes);
    return 1;
}

// @func 0x0051D4B4 TextureHeaderChunkHandler
int TextureHeaderChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    auto** out = static_cast<AglTexture**>(arg);
    uint8_t version = 0;
    AglTextureHeader header = {};
    reader->ReadU8(&version);
    reader->ReadU32(&header.nameHash);
    reader->ReadU16(&header.width);
    reader->ReadU16(&header.height);
    reader->ReadU8(&header.field_8[0]);   // depth
    reader->ReadU8(&header.field_8[1]);   // bits per pixel
    reader->ReadU8(&header.field_8[2]);   // type
    reader->ReadRaw32(header.name);
    reader->ReadU32(&header.packOffset);
    reader->ReadU32(&header.packSize);

    if (reader->flags & kReaderResolveOnly) {
        *out = FindTexture(header.nameHash);
        return 1;
    }
    // A texture of that name is already loaded: the chunk is rejected.
    if (FindTexture(header.nameHash))
        return 0;
    AglTexture* texture = AglTexture::Create();
    if (g_textureCreateHook)
        g_textureCreateHook(texture, g_textureCreateUserdata, 0);
    *out = texture;
    texture->ApplyHeader(header);
    return 1;
}

// @func 0x0051D718 TextureFileChunkHandler
int TextureFileChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    if (reader->flags & kReaderResolveOnly)
        return 1;
    AglTexture* texture = *static_cast<AglTexture**>(arg);
    if (g_defaultForLooseTextures && texture->UseDefaultImage())
        return 1;
    uint8_t field0 = 0, field1 = 0;
    char fileName[0x100];
    reader->Read(&field0, 1);
    reader->Read(&field1, 1);
    reader->Read(fileName, reader->GetChunkRemaining() < sizeof(fileName) ? reader->GetChunkRemaining()
                                                                          : sizeof(fileName));
    fileName[sizeof(fileName) - 1] = '\0';
    char path[0x200];
    std::snprintf(path, sizeof(path), "%s%s", GetTextureDirectory(), fileName);
    return texture->LoadExternal(path) ? 1 : 0;
}

// @func 0x0051D83C TextureDefaultChunkHandler
int TextureDefaultChunkHandler(ape::iff::IffReader* /*reader*/, void* arg)
{
    AglTexture* texture = *static_cast<AglTexture**>(arg);
    const AglTexture* fallback = GetDefaultTexture(kAglTex_White);
    texture->image.desc = fallback->image.desc;
    MemBlockMakeView(&texture->image.mem, fallback->image.mem);
    texture->image.surface = nullptr;
    texture->image.isView = true;
    texture->serial = 0;
    return 1;
}

// @func 0x0053A8B0 AglTexture::LoadExternal
bool AglTexture::LoadExternal(const char* path)
{
    // Loose textures are not supported by the shipped game: the file is
    // read but the default image is used.
    uint32_t size = 0;
    if (void* data = ape::file::LoadFile(path, &size))
        ape::file::FreeFileData(data);
    UseDefaultImage();
    return false;
}

// @func 0x00525E18 MeshIncludeChunkHandler
int MeshIncludeChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    char path[0x100];
    ape::iff::ParseIncludedFile(reader, path);
    return 0;
}

// @func 0x0052609C TopologyHeaderChunkHandler
int TopologyHeaderChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglTopology* topology = static_cast<AglTopology*>(arg);
    uint8_t version = 0;
    float center[3] = {};
    float radius = 0.0f;
    uint32_t hash = 0;
    reader->ReadU8(&version);
    reader->ReadVec3(center);
    reader->ReadFloat(&radius);
    reader->ReadU32(&hash);
    if (version >= 1) {
        uint8_t vertexCompressionMode = 0;   // read and ignored
        reader->ReadU8(&vertexCompressionMode);
    }
    std::memcpy(topology->boundCenter, center, sizeof(center));
    topology->boundRadius = radius;
    topology->hash = hash;
    return 1;
}

// Index chunk target: triangle count and the index buffer to fill.
struct AglIndexChunkTarget {
    uint16_t triangleCount;
    uint16_t field_2;
    uint16_t* indices;
};

// @func 0x00524108 IndexChunkHandler
int IndexChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    auto* target = static_cast<AglIndexChunkTarget*>(arg);
    const uint32_t size = reader->GetChunkSize();
    reader->Read(target->indices, size);
    // Flag bit 1: the indices are stored with the other byte order.
    if (reader->flags & ape::iff::kIffReader_Flag02) {
        uint16_t* index = target->indices;
        for (uint32_t i = 0; i < uint32_t(target->triangleCount) * 3; ++i, ++index)
            *index = static_cast<uint16_t>((*index >> 8) | (*index << 8));
    }
    return 1;
}

// @func 0x00525BE8 BoneTableChunkHandler
int BoneTableChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglSubMesh* mesh = static_cast<AglSubMesh*>(arg);
    const uint32_t size = reader->GetChunkSize();
    const uint32_t count = size / 2;
    // Byte 0 holds the number of bones; bone indices (one byte each) follow.
    const size_t bytes = (size & ~1u) + 2;
    auto* table = static_cast<uint8_t*>(ape::MemAlloc(bytes));
    std::memset(table, 0, bytes);
    table[0] = static_cast<uint8_t>(count);
    mesh->skinMatrices = table;
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t bone = 0;
        reader->ReadU16(&bone);
        if (bone < 0x100)
            table[1 + i] = static_cast<uint8_t>(bone);
        else
            --table[0];   // out of range: dropped
    }
    return 1;
}

// @func 0x00548CD4 RegisterMemoryBlockChunk
void RegisterMemoryBlockChunk()
{
    ape::iff::RegisterChunkHandler(ape::iff::MakeId('M', 'B', 'L', 'K'), MemoryBlockChunkHandler, 0, nullptr, 0);
}

} // namespace agl
