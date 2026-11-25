// openTIT - Animation file streams and chunk handlers (0x003463B0-0x0034B884).
#include "sly/anim/AnimStream.h"

#include "sly/anim/AnimHavok.h"

#include "ape/core/ApeString.h"
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

AnimFileState& AnimFile()
{
    static AnimFileState state;
    return state;
}

// @func 0x0034B844 AnimGroupChunk
bool AnimGroupChunk(ape::iff::IffReader* reader)
{
    AnimFile().name = nullptr;
    AnimFile().nameHash = 0;
    reader->ParseChunks(nullptr);
    return true;
}

// @func 0x0034B884 AnimNameChunk
bool AnimNameChunk(ape::iff::IffReader* reader)
{
    AnimFileState& state = AnimFile();
    uint8_t version = 0;   // NOTE: read and ignored
    reader->ReadU8(&version);
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&state.nameA[1 + i]));
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&state.nameB[i]));
    state.name = state.nameB;
    state.nameHash = ape::HashNameNoCase(&state.nameA[1]);
    return true;
}

} // namespace anim
} // namespace sly
