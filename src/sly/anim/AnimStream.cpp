// openTIT - Animation file streams and chunk handlers (0x003463B0-0x0034B884).
#include "sly/anim/AnimStream.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimHavok.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/sys/ApeIff.h"

namespace sly {
namespace anim {

// @func 0x003555BC IffStreamReader::Read
int IffStreamReader::Read(void* buffer, int size)
{
    if (!IsOk() || size < 1 || !file)
        return 0;
    const int remaining = int(file->GetChunkRemaining());
    if (remaining < 0) {
        ok = false;
        return 0;
    }
    const int count = size < remaining ? size : remaining;
    // NOTE: the console returns what the IFF reader reports as read.
    file->Read(buffer, uint32_t(count));
    return count;
}

// @func 0x003431C0 FileStreamReader::~FileStreamReader
// @func 0x00347A4C FileStreamReader::~FileStreamReader (deleting)
FileStreamReader::~FileStreamReader()
{
    if (file && owns)
        file->Close();
    file = nullptr;
}

// @func 0x00347B04 FileStreamReader::IsOk
bool FileStreamReader::IsOk() const
{
    return file && ok;
}

// @func 0x00347B2C FileStreamReader::Read
int FileStreamReader::Read(void* buffer, int size)
{
    if (!IsOk() || size <= 0 || !file)
        return 0;
    const int count = int(file->Read(buffer, uint64_t(size), nullptr));
    if (count >= 0)
        return count;
    ok = false;
    return 0;
}

// @func 0x00347BFC FileStreamReader::Skip
int FileStreamReader::Skip(int size)
{
    if (!IsOk() || size <= 0 || !file)
        return 0;
    const int64_t before = int64_t(file->GetPosition());
    return int(file->Seek(size, ape::file::kSeek_Cur) - before);
}

// @func 0x00347CB8 FileStreamReader::SetMark
int FileStreamReader::SetMark(int)
{
    if (!IsOk() || !file)
        return 1;
    mark = int64_t(file->GetPosition());
    return 0;
}

// @func 0x00347D38 FileStreamReader::RewindToMark
int FileStreamReader::RewindToMark()
{
    if (!IsOk() || !file)
        return 1;
    return file->Seek(mark, ape::file::kSeek_Set) != mark ? 1 : 0;
}

// @func 0x00347DE8 FileStreamReader::Seek
int FileStreamReader::Seek(int offset, int whence)
{
    if (!IsOk() || !file)
        return 1;
    file->Seek(offset, whence);
    return 0;
}

// @func 0x00347EA0 FileStreamReader::Tell
int FileStreamReader::Tell() const
{
    if (!IsOk() || !file)
        return -1;
    return int(file->GetPosition());
}

// @func 0x0034640C AnimStreamReader::AnimStreamReader
AnimStreamReader::AnimStreamReader(ape::iff::IffReader* reader, int bufferSize)
{
    auto* chunk = new IffStreamReader(reader);
    inner = HkNewBufferedReader(chunk, bufferSize);
    // The buffered reader holds its own reference.
    HkRemoveReference(chunk);
}

// @func 0x003463B0 AnimStreamReader::~AnimStreamReader
// @func 0x00347608 AnimStreamReader::~AnimStreamReader (deleting)
AnimStreamReader::~AnimStreamReader()
{
    if (inner) {
        HkRemoveReference(inner);
        inner = nullptr;
    }
}

// @func 0x003476A8 AnimStreamReader::IsOk
bool AnimStreamReader::IsOk() const
{
    return inner ? inner->IsOk() : false;
}

// @func 0x00347718 AnimStreamReader::Read
int AnimStreamReader::Read(void* buffer, int size)
{
    return inner ? inner->Read(buffer, size) : 0;
}

// @func 0x00347780 AnimStreamReader::Skip
int AnimStreamReader::Skip(int size)
{
    return inner ? inner->Skip(size) : 0;
}

// @func 0x003477E8 AnimStreamReader::MarkSupported
bool AnimStreamReader::MarkSupported() const
{
    return inner ? inner->MarkSupported() : false;
}

// @func 0x00347858 AnimStreamReader::SetMark
int AnimStreamReader::SetMark(int limit)
{
    return inner ? inner->SetMark(limit) : 1;
}

// @func 0x003478C0 AnimStreamReader::RewindToMark
int AnimStreamReader::RewindToMark()
{
    return inner ? inner->RewindToMark() : 1;
}

// @func 0x00347914 AnimStreamReader::SeekTellSupported
bool AnimStreamReader::SeekTellSupported() const
{
    return inner ? inner->SeekTellSupported() : false;
}

// @func 0x00347984 AnimStreamReader::Seek
int AnimStreamReader::Seek(int offset, int whence)
{
    return inner ? inner->Seek(offset, whence) : 1;
}

// @func 0x003479EC AnimStreamReader::Tell
int AnimStreamReader::Tell() const
{
    return inner ? inner->Tell() : -1;
}

AnimResourceName& AnimResourceNames(int kind)
{
    static AnimResourceName names[kAnimRes_Count];
    return names[kind];
}

ape::ApeTArray<void*>& AnimSkeletonResources()
{
    static ape::ApeTArray<void*> resources;
    return resources;
}

namespace {

bool ParseGroup(ape::iff::IffReader* reader, int kind)
{
    AnimResourceNames(kind).name = nullptr;
    AnimResourceNames(kind).nameHash = 0;
    reader->ParseChunks(nullptr);
    return true;
}

bool ReadName(ape::iff::IffReader* reader, int kind)
{
    AnimResourceName& state = AnimResourceNames(kind);
    uint8_t version = 0;   // NOTE: read and ignored
    reader->ReadU8(&version);
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&state.first[1 + i]));
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&state.second[i]));
    state.name = state.second;
    state.nameHash = ape::HashNameNoCase(&state.first[1]);
    return true;
}

using RegisterFn = bool (*)(const char* name, uint32_t nameHash, void* resource, uint32_t size);

bool LoadData(ape::iff::IffReader* reader, int kind, RegisterFn registerFn)
{
    AnimResourceName& state = AnimResourceNames(kind);
    const char* name = state.name;
    const uint32_t nameHash = state.nameHash;
    const uint32_t size = reader->GetChunkSize();
    void* resource = nullptr;
    {
        AnimStreamReader stream(reader, 0x1000);
        resource = HkLoadResource(&stream);
    }
    if (resource) {
        registerFn(name, nameHash, resource, size);
        HkRemoveReference(resource);
    }
    state.name = nullptr;
    return true;
}

bool RegisterAnimationsNoFlag(const char* name, uint32_t nameHash, void* resource, uint32_t size)
{
    return RegisterAnimAnimations(name, nameHash, resource, size, false);
}

} // namespace

// @func 0x0034B844 AnimSkeletonGroupChunk
bool AnimSkeletonGroupChunk(ape::iff::IffReader* reader)
{
    return ParseGroup(reader, kAnimRes_Skeletons);
}

// @func 0x0034BB18 AnimSkeletonMapGroupChunk
bool AnimSkeletonMapGroupChunk(ape::iff::IffReader* reader)
{
    return ParseGroup(reader, kAnimRes_SkeletonMaps);
}

// @func 0x0034B2C8 LoadAnimationFile
bool LoadAnimationFile(ape::file::File* file)
{
    AnimResourceName& state = AnimResourceNames(kAnimRes_Animations);
    const char* name = state.name;
    const uint32_t nameHash = state.nameHash;
    FileStreamReader stream(file, false);
    if (file)
        file->Seek(0, ape::file::kSeek_Set);
    const uint32_t size = uint32_t(file->GetSize());
    if (void* resource = HkLoadResource(&stream)) {
        RegisterAnimAnimations(name, nameHash, resource, size, false);
        HkRemoveReference(resource);
    }
    state.name = nullptr;
    return true;
}

// @func 0x0034B4F4 AnimAnimationGroupChunk
bool AnimAnimationGroupChunk(ape::iff::IffReader* reader)
{
    AnimResourceName& state = AnimResourceNames(kAnimRes_Animations);
    state.name = nullptr;
    state.nameHash = 0;
    reader->ParseChunks(nullptr);
    if (state.name) {
        if (ape::file::File* file = ape::file::OpenFile(state.name, "rb")) {
            LoadAnimationFile(file);
            file->Close();
        }
    }
    return true;
}

// @func 0x0034B884 AnimSkeletonNameChunk
bool AnimSkeletonNameChunk(ape::iff::IffReader* reader)
{
    return ReadName(reader, kAnimRes_Skeletons);
}

// @func 0x0034B594 AnimAnimationNameChunk
bool AnimAnimationNameChunk(ape::iff::IffReader* reader)
{
    return ReadName(reader, kAnimRes_Animations);
}

// @func 0x0034BB58 AnimSkeletonMapNameChunk
bool AnimSkeletonMapNameChunk(ape::iff::IffReader* reader)
{
    return ReadName(reader, kAnimRes_SkeletonMaps);
}

// @func 0x0034B9A8 AnimSkeletonDataChunk
bool AnimSkeletonDataChunk(ape::iff::IffReader* reader)
{
    return LoadData(reader, kAnimRes_Skeletons, &RegisterAnimSkeletons);
}

// @func 0x0034B6C4 AnimAnimationDataChunk
bool AnimAnimationDataChunk(ape::iff::IffReader* reader)
{
    return LoadData(reader, kAnimRes_Animations, &RegisterAnimationsNoFlag);
}

// @func 0x0034BC7C AnimSkeletonMapDataChunk
bool AnimSkeletonMapDataChunk(ape::iff::IffReader* reader)
{
    return LoadData(reader, kAnimRes_SkeletonMaps, &RegisterAnimSkeletonMaps);
}

// @func 0x0034A5E4 RegisterAnimSkeletons
bool RegisterAnimSkeletons(const char*, uint32_t nameHash, void* resource, uint32_t)
{
    HkaAnimationContainer* container = nullptr;
    if (void* root = HkResourceRootContainer(resource)) {
        container = HkRootFindAnimationContainer(root);
    } else {
        container = HkResourceAnimationContainer(resource);
        if (!container)
            return false;
    }
    if (!container)
        return true;
    HkaSkeleton* skeleton = container->numSkeletons ? container->skeletons[0] : nullptr;
    if (!skeleton)
        return false;
    HkAddReference(resource);
    AnimSkeletonResources().PushBack(resource);
    // NOTE: the console drops a reference of the file's first animation.
    if (container->numAnimations > 0)
        HkRemoveReference(container->animations[0]);
    if (nameHash == 0 || *AnimSkeletons().Find(nameHash)) {
        // NOTE: the console warns "Duplicate Havok skeletons in table".
        return true;
    }
    AnimSkeletons().Add(nameHash, skeleton);
    return true;
}

// TODO(0x00348F04 RegisterAnimAnimations): bindings and annotations of a
// file's animations.
bool RegisterAnimAnimations(const char*, uint32_t, void*, uint32_t, bool) { return false; }
// TODO(0x0034AB90 RegisterAnimSkeletonMaps): mapper registration and its
// bone name fix-up (vector code).
bool RegisterAnimSkeletonMaps(const char*, uint32_t, void*, uint32_t) { return false; }

} // namespace anim
} // namespace sly
