// openTIT - Agl level chunk handlers for textures and mesh topology headers.
//
// Level files are IFF chunk streams (ape::iff). These handlers read the
// texture and topology description chunks; the reader's flag bit 2 marks a
// "resolve only" pass in which textures are looked up instead of created.
#include "agl/AglPrimitives.h"
#include "agl/AglResources.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace agl {

namespace {

constexpr uint8_t kReaderResolveOnly = 0x04;

void (*g_textureCreateHook)(AglTexture*, void*, int) = nullptr;   // 0x00F8F118 + 8
void* g_textureCreateUserdata = nullptr;                           // 0x00F8F118 + 0xC
void (*g_materialCreateHook)(AglMaterial*, void*, int) = nullptr; // 0x00F8F114 + 8
void* g_materialCreateUserdata = nullptr;                          // 0x00F8F114 + 0xC
bool g_defaultForLooseTextures = false;                            // 0x010C5AC0 bit 7

} // namespace

void SetTextureCreateHook(void (*hook)(AglTexture*, void*, int), void* userdata)
{
    g_textureCreateHook = hook;
    g_textureCreateUserdata = userdata;
}

void SetMaterialCreateHook(void (*hook)(AglMaterial*, void*, int), void* userdata)
{
    g_materialCreateHook = hook;
    g_materialCreateUserdata = userdata;
}

void RunMaterialCreateHook(AglMaterial* material, int phase)
{
    if (g_materialCreateHook)
        g_materialCreateHook(material, g_materialCreateUserdata, phase);
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

// @func 0x0051D210 TextureChunkHandler
// A texture record: the header child creates the texture (or, in a resolve
// pass, only looks it up), the image children fill it. A texture that ends
// up with an image is announced to the create hook and entered in the name
// dictionary.
int TextureChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    AglTexture* texture = nullptr;
    if (reader->flags & kReaderResolveOnly) {
        reader->ParseChunks(&texture);
        return 1;
    }
    if (reader->ParseChunks(&texture) && texture && texture->HasImage()) {
        if (g_textureCreateHook)
            g_textureCreateHook(texture, g_textureCreateUserdata, 1);
        // NOTE: the console appends the texture to the dictionary array and
        // marks it unsorted; the PC dictionary is a map.
        RegisterTextureName(texture);
    }
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

// @func 0x005399CC TextureGtfChunkHandler
// The chunk payload is a GTF file; it is loaded into GPU memory as is.
int TextureGtfChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglTexture* texture = *static_cast<AglTexture**>(arg);
    if (reader->flags & kReaderResolveOnly) {
        if (!texture)
            return 1;
        // The image is loaded again below.
        MemBlockFree(&texture->image.mem);
        texture->image.mem.data = nullptr;
    }
    if (g_defaultForLooseTextures && texture->UseDefaultImage())
        return 1;

    const uint32_t size = reader->GetChunkSize();
    MemBlockAlloc(&texture->image.mem, size, 0x80, gpu::MemoryLocation::Local, kAglMem_Texture);
    if (texture->image.mem.data)
        reader->Read(texture->image.mem.data, size);
    texture->ParseGtfImage(size);
    texture->image.format = FindFormatByTextureFormat(texture->image.desc.format);
    return 1;
}

namespace {

// The part of a DDS file header the loader reads (little endian).
struct DdsHeader {
    uint32_t height;
    uint32_t width;
    uint32_t fourCC;
};

constexpr uint32_t FourCC(char a, char b, char c, char d)
{
    return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8) | (uint32_t(uint8_t(c)) << 16) |
           (uint32_t(uint8_t(d)) << 24);
}

uint32_t ReadLE32At(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

bool IsPowerOfTwo(uint32_t value)
{
    return (value & (0u - value)) == value;
}

} // namespace

// @func 0x00539C7C TextureDdsChunkHandler
// The chunk payload is a DDS file: DXT1/3/5 images and the signed two
// channel formats (D3D formats 60 V8U8 and 117 CxV8U8). Power of two DXT
// images use the swizzled formats.
int TextureDdsChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    // NOTE: the console uses the chunk argument itself as the texture; the
    // other texture chunks (and this port) read the texture from it.
    AglTexture* texture = *static_cast<AglTexture**>(arg);

    // Until the image is loaded the texture shows default texture 0.
    const AglTexture* fallback = GetDefaultTexture(kAglTex_White);
    texture->image.desc = fallback->image.desc;
    texture->image.format = fallback->image.format;
    MemBlockMakeView(&texture->image.mem, fallback->image.mem);
    texture->attachment = nullptr;

    uint8_t magic[4];
    uint8_t header[0x7C];
    reader->Read(magic, sizeof(magic));
    reader->Read(header, sizeof(header));
    DdsHeader dds;
    dds.height = ReadLE32At(header + 0x08);
    dds.width = ReadLE32At(header + 0x0C);
    dds.fourCC = ReadLE32At(header + 0x50);
    texture->width = uint16_t(dds.width);
    texture->height = uint16_t(dds.height);

    const bool powerOfTwo = IsPowerOfTwo(dds.width) && IsPowerOfTwo(dds.height);
    uint32_t format;
    bool signedChannels = false;
    if (dds.fourCC == FourCC('D', 'X', 'T', '5') || dds.fourCC == FourCC('D', 'X', 'T', '4'))
        format = powerOfTwo ? kAglFmt_DXT5 : kAglFmt_LinDXT5;
    else if (dds.fourCC == FourCC('D', 'X', 'T', '3') || dds.fourCC == FourCC('D', 'X', 'T', '2'))
        format = powerOfTwo ? kAglFmt_DXT3 : kAglFmt_LinDXT3;
    else if (dds.fourCC == FourCC('D', 'X', 'T', '1'))
        format = powerOfTwo ? kAglFmt_DXT1 : kAglFmt_LinDXT1;
    else if (dds.fourCC == 117 || dds.fourCC == 60) {
        format = kAglFmt_LinR8G8;
        signedChannels = true;
    } else {
        return 0;
    }

    TextureCreate(&texture->image, uint16_t(dds.width), uint16_t(dds.height), 1, format, false, nullptr, false);
    texture->sampler[0] = 1;
    texture->sampler[1] = 1;
    texture->sampler[2] = 0;
    texture->sampler[6] |= 1;

    const uint32_t size = texture->image.mem.size;
    std::vector<uint8_t> pixels(size);
    reader->Read(pixels.data(), size);
    // Signed channels are stored doubled in the unsigned format.
    if (signedChannels) {
        for (uint8_t& value : pixels)
            value = uint8_t(value << 1);
    }
    if (texture->image.mem.data)
        std::memcpy(texture->image.mem.data, pixels.data(), size);
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

// @func 0x00525E40 MeshChunkHandler
// A mesh record: a new topology filled by the header and submesh children.
// Parts without a material of their own use their submesh's material; the
// finished topology enters the dictionary.
int MeshChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    AglTopology* topology = AglTopology::Create();
    reader->ParseChunks(topology);
    for (uint16_t i = 0; i < topology->meshCount; ++i) {
        AglTopology::MeshEntry& entry = topology->meshes[i];
        if (!entry.material) {
            auto* material = static_cast<AglMaterial*>(entry.mesh->owner);
            entry.material = material;
            if (material)
                material->AddRef();
        }
    }
    RegisterTopology(topology);
    return 1;
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

namespace {

uint16_t Swap16(uint16_t v)
{
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

uint32_t Swap32(uint32_t v)
{
    return (v >> 24) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) | (v << 24);
}

// Vertex attributes whose data may need its byte order swapped (the
// console checks this list of attribute registers, 0x00F67190).
constexpr uint32_t kSwappedAttributes[] = { 0, 2, 3, 8, 14, 15, 1, 7, 5, 6 };

} // namespace

// @func 0x00525174 SwapVertexWords
// Reverses the byte order of the first 'words' 32-bit words of 'count'
// vertices, 'stride' bytes apart.
void SwapVertexWords(uint32_t count, uint8_t* data, uint32_t words, uint32_t stride)
{
    for (uint32_t v = 0; v < count; ++v, data += stride) {
        for (uint32_t w = 0; w < words && w < 4; ++w) {
            uint32_t value;
            std::memcpy(&value, data + w * 4, 4);
            value = Swap32(value);
            std::memcpy(data + w * 4, &value, 4);
        }
    }
}

// @func 0x0052528C SwapVertexData
// Fixes the byte order of a submesh's vertex buffer, attribute by attribute:
// float components are swapped as 32-bit words, half and normalized short
// components as 16-bit words; bytes (and the formats the console leaves
// alone) are untouched.
void SwapVertexData(AglSubMesh* mesh)
{
    const AglVertexDecl* decl = mesh->vertexDecl;
    auto* base = static_cast<uint8_t*>(mesh->vertices.data);
    if (!decl || !base)
        return;
    for (uint32_t attribute : kSwappedAttributes) {
        const AglVertexStream* stream = FindVertexStream(decl->streams, attribute, 0);
        if (!stream)
            continue;
        uint8_t* data = base + stream->offset;
        switch (static_cast<gpu::VertexAttribType>(stream->type)) {
        case gpu::VertexAttribType::Float:
            SwapVertexWords(mesh->vertexCount, data, stream->size, decl->stride);
            break;
        case gpu::VertexAttribType::Half:
        case gpu::VertexAttribType::ShortNorm:
            for (uint32_t v = 0; v < mesh->vertexCount; ++v) {
                uint8_t* vertex = data + v * decl->stride;
                for (uint32_t c = 0; c < stream->size; ++c) {
                    uint16_t value;
                    std::memcpy(&value, vertex + c * 2, 2);
                    value = Swap16(value);
                    std::memcpy(vertex + c * 2, &value, 2);
                }
            }
            break;
        default:
            break;
        }
    }
}

// @func 0x00525818 VertexDataChunkHandler
// Vertex buffer of a submesh, one vertex stream per chunk.
// NOTE: on the console the stream blocks follow each other from +0x18; a
// second stream would overlap the vertex count, so meshes have exactly one.
int VertexDataChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglSubMesh* mesh = static_cast<AglSubMesh*>(arg);
    const uint32_t size = reader->GetChunkSize();
    mesh->AddVertexStream();
    MemBlockAlloc(&mesh->vertices, size, 0x80, gpu::MemoryLocation::Local, 2);
    reader->Read(mesh->vertices.data, size);
    if (reader->flags & ape::iff::kIffReader_Flag02)
        SwapVertexData(mesh);
    return 1;
}

// @func 0x00524500 SubMeshChunkHandler
// A submesh record of the topology being loaded: the new submesh joins the
// topology (which keeps the only reference) and its children fill it.
int SubMeshChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    auto* topology = static_cast<AglTopology*>(arg);
    AglSubMesh* mesh = AglSubMesh::Create();
    topology->AddMesh(mesh, static_cast<AglMaterial*>(mesh->owner));
    mesh->Release();
    reader->ParseChunks(mesh);
    return 1;
}

// TODO(0x00524B68 SubMeshHeaderChunkHandler): submesh header (material,
// primitive, vertex/index counts, vertex declaration). Skipped for now.
int SubMeshHeaderChunkHandler(ape::iff::IffReader* /*reader*/, void* /*arg*/)
{
    return 1;
}

// @func 0x00525A1C VertexDescChunkHandler
// The stored vertex description is not needed: the submesh header already
// selected the declaration. The chunk is accepted unread.
int VertexDescChunkHandler(ape::iff::IffReader* /*reader*/, void* /*arg*/)
{
    return 1;
}

// @func 0x00525904 SubMeshIndexChunkHandler
// 16-bit index buffer of a submesh (index count set by its header).
int SubMeshIndexChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglSubMesh* mesh = static_cast<AglSubMesh*>(arg);
    const uint32_t count = mesh->indexCount;
    mesh->field_3C = 1;
    MemBlockAlloc(&mesh->indices, count * 2, 0x80, gpu::MemoryLocation::Local, 2);
    auto* indices = static_cast<uint16_t*>(mesh->indices.data);
    reader->Read(indices, count * 2);
    if (reader->flags & ape::iff::kIffReader_Flag02) {
        for (uint32_t i = 0; i < count; ++i)
            indices[i] = Swap16(indices[i]);
    }
    return 1;
}

// @func 0x00525A24 SkinWeightChunkHandler
// Per-vertex bone indices and weights of a skinned submesh. The chunk holds
// either the packed runtime layout (four index bytes, four weight bytes) or
// the older 12-byte layout with 16-bit weights, of which only the high byte
// is kept. At most three influences survive.
int SkinWeightChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglSubMesh* mesh = static_cast<AglSubMesh*>(arg);
    const uint32_t vertexCount = mesh->vertexCount;
    mesh->skinWeights = static_cast<uint8_t*>(ape::MemAlloc(vertexCount * 8, ape::kMemClass_Default, 16));
    const uint32_t size = reader->GetChunkSize();
    if (size == vertexCount * 12) {
        struct WideInfluences {
            uint8_t bones[4];
            uint16_t weights[4];
        };
        std::vector<WideInfluences> wide(vertexCount);
        reader->Read(wide.data(), size);
        const bool swap = (reader->flags & ape::iff::kIffReader_Flag02) != 0;
        for (uint32_t v = 0; v < vertexCount; ++v) {
            uint8_t* out = mesh->skinWeights + v * 8;
            for (int i = 0; i < 4; ++i) {
                uint16_t weight = wide[v].weights[i];
                if (swap)
                    weight = static_cast<uint16_t>((weight >> 8) | (weight << 8));
                out[i] = wide[v].bones[i];
                out[4 + i] = static_cast<uint8_t>(weight >> 8);
            }
        }
    } else if (size == vertexCount * 8) {
        reader->Read(mesh->skinWeights, size);
    }
    mesh->LimitBoneInfluences(3);
    return 1;
}

// @func 0x00548CD4 RegisterMemoryBlockChunk
void RegisterMemoryBlockChunk()
{
    ape::iff::RegisterChunkHandler(ape::iff::MakeId('M', 'B', 'L', 'K'), MemoryBlockChunkHandler, 0, nullptr, 0);
}

} // namespace agl
