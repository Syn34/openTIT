// openTIT - Byte streams the animation loader hands to Havok's packfile
// reader: an IFF chunk exposed as a stream (vtable 0x00B1F608) and the
// buffered reader built on it (vtable 0x00B1F480, 0x0034640C), plus the
// chunk handlers of the animation files that use them.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { namespace iff { struct IffReader; } namespace file { class File; } namespace world { struct ApeSkeletonDef; } }

namespace sly {
namespace anim {

// Stand-in for Havok's stream reader interface (console vtable slots 4-12;
// slots 0-3 are the reference counted object's). Results of SetMark,
// RewindToMark and Seek: 0 success, 1 failure.
class HkStreamReader {
public:
    virtual ~HkStreamReader() = default;
    virtual bool IsOk() const = 0;                          // slot 4
    virtual int Read(void* buffer, int size) = 0;           // slot 5
    virtual int Skip(int size) { (void)size; return 0; }    // slot 6
    virtual bool MarkSupported() const { return false; }    // slot 7
    virtual int SetMark(int limit) { (void)limit; return 1; }   // slot 8
    virtual int RewindToMark() { return 1; }                // slot 9
    virtual bool SeekTellSupported() const { return false; }    // slot 10
    virtual int Seek(int offset, int whence) { (void)offset; (void)whence; return 1; }   // slot 11
    virtual int Tell() const { return -1; }                 // slot 12
};

// The open chunk of an IFF reader as a stream (vtable 0x00B1F608, 0x10
// bytes). Reads stop at the end of the chunk.
class IffStreamReader : public HkStreamReader {
public:
    explicit IffStreamReader(ape::iff::IffReader* reader) : file(reader), ok(reader != nullptr) {}
    // Slot 4 (0x00771DBC, other module).
    bool IsOk() const override { return ok; }
    // Slot 5 (0x003555BC).
    int Read(void* buffer, int size) override;

    ape::iff::IffReader* file = nullptr;   // +0x08
    bool ok = false;                       // +0x0C
};

// A game file as a stream (vtable 0x00B1F338, 0x20 bytes; built in place
// by its users).
class FileStreamReader : public HkStreamReader {
public:
    FileStreamReader(ape::file::File* f, bool ownsFile) : file(f), ok(f != nullptr), owns(ownsFile) {}
    // 0x003431C0 / 0x00347A4C: closes the file when it owns it.
    ~FileStreamReader() override;
    bool IsOk() const override;                    // 0x00347B04
    int Read(void* buffer, int size) override;     // 0x00347B2C
    int Skip(int size) override;                   // 0x00347BFC
    bool MarkSupported() const override { return true; }      // slot 7 (0x00770E78)
    int SetMark(int limit) override;               // 0x00347CB8
    int RewindToMark() override;                   // 0x00347D38
    bool SeekTellSupported() const override { return true; }  // slot 10 (0x00770E84)
    int Seek(int offset, int whence) override;     // 0x00347DE8
    int Tell() const override;                     // 0x00347EA0

    ape::file::File* file = nullptr;   // +0x08
    int64_t mark = 0;                  // +0x10
    bool ok = false;                   // +0x18
    bool owns = false;                 // +0x19
};

// Buffered stream over the current IFF chunk (vtable 0x00B1F480, 0x0C
// bytes); every call goes to the inner buffered reader (+0x08).
class AnimStreamReader : public HkStreamReader {
public:
    // 0x0034640C: `bufferSize` bytes of buffering over the chunk.
    AnimStreamReader(ape::iff::IffReader* reader, int bufferSize);
    // 0x003463B0 / 0x00347608.
    ~AnimStreamReader() override;

    bool IsOk() const override;                    // 0x003476A8
    int Read(void* buffer, int size) override;     // 0x00347718
    int Skip(int size) override;                   // 0x00347780
    bool MarkSupported() const override;           // 0x003477E8
    int SetMark(int limit) override;               // 0x00347858
    int RewindToMark() override;                   // 0x003478C0
    bool SeekTellSupported() const override;       // 0x00347914
    int Seek(int offset, int whence) override;     // 0x00347984
    int Tell() const override;                     // 0x003479EC

    HkStreamReader* inner = nullptr;   // +0x08
};

// Resource kinds of the animation files; each has its own name chunk.
enum AnimResourceKind {
    kAnimRes_Skeletons = 0,     // registered by RegisterAnimSkeletons
    kAnimRes_Animations = 1,    // registered by 0x00348F04
    kAnimRes_SkeletonMaps = 2,  // registered by 0x0034AB90
    kAnimRes_Count
};
// Name of the next resource of one kind (0x0106BFA4 + 8 * kind, with the
// name buffers at 0x0106C0F0 / 0x0106C060 / 0x0106C180).
struct AnimResourceName {
    const char* name = nullptr;   // the second name read (null: none)
    uint32_t nameHash = 0;        // HashNameNoCase of the first name
    char first[0x41] = {};        // read from +1
    char second[0x40] = {};
};
AnimResourceName& AnimResourceNames(int kind);

// Group chunk handlers (0x0034B844 skeletons, 0x0034BB18 skeleton maps):
// forget the name and parse the sub-chunks.
bool AnimSkeletonGroupChunk(ape::iff::IffReader* reader);
bool AnimSkeletonMapGroupChunk(ape::iff::IffReader* reader);
// 0x0034B4F4: group chunk of animations kept in a file of their own: after
// the sub-chunks the named file is loaded (LoadAnimationFile).
bool AnimAnimationGroupChunk(ape::iff::IffReader* reader);
// 0x0034B2C8: loads the Havok file `file` and registers its animations
// under the pending animation name.
bool LoadAnimationFile(ape::file::File* file);
// Name chunk handlers (0x0034B884 skeletons, 0x0034B594 animations,
// 0x0034BB58 skeleton maps): one byte, then two 64 byte names; the second
// becomes the name, the first is hashed.
bool AnimSkeletonNameChunk(ape::iff::IffReader* reader);
bool AnimAnimationNameChunk(ape::iff::IffReader* reader);
bool AnimSkeletonMapNameChunk(ape::iff::IffReader* reader);
// Data chunk handlers (0x0034B9A8 skeletons, 0x0034B6C4 animations,
// 0x0034BC7C skeleton maps): the chunk is a Havok file, loaded through a
// buffered stream and registered under the pending name.
bool AnimSkeletonDataChunk(ape::iff::IffReader* reader);
bool AnimAnimationDataChunk(ape::iff::IffReader* reader);
bool AnimSkeletonMapDataChunk(ape::iff::IffReader* reader);

// 0x0034A5E4: keeps the resource and registers its first skeleton under
// `nameHash`. False when the resource holds no animation container.
bool RegisterAnimSkeletons(const char* name, uint32_t nameHash, void* resource, uint32_t size);
// 0x00348F04 (TODO): registers the animations of a resource.
bool RegisterAnimAnimations(const char* name, uint32_t nameHash, void* resource, uint32_t size, bool flag);
// 0x0034AB90 (TODO): registers the skeleton mapper of a resource.
bool RegisterAnimSkeletonMaps(const char* name, uint32_t nameHash, void* resource, uint32_t size);
// Resources kept by the registrations (0x0106BC2C skeletons, 0x0106BC58
// skeleton maps).
ape::ApeTArray<void*>& AnimSkeletonResources();

// 0x0034B0F8: map from the bones of `def` to the bones of the registered
// Havok skeleton `skeletonName` (entry 0 holds the count, then one Havok
// index per bone, 0xFFFF for none). Havok bone names lose an "xxx:" prefix
// before matching. Null when the skeleton is not registered. Free with
// ape::MemFree.
uint16_t* BuildHavokBoneMap(uint32_t skeletonName, const ape::world::ApeSkeletonDef* def);

// Havok's buffered stream reader (0x007DFEF8, 0x24 bytes) over `inner`.
HkStreamReader* HkNewBufferedReader(HkStreamReader* inner, int bufferSize);

} // namespace anim
} // namespace sly
