// openTIT - Byte streams the animation loader hands to Havok's packfile
// reader: an IFF chunk exposed as a stream (vtable 0x00B1F608) and the
// buffered reader built on it (vtable 0x00B1F480, 0x0034640C), plus the
// chunk handlers of the animation files that use them.
#pragma once

#include <cstdint>

namespace ape { namespace iff { struct IffReader; } }

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

// State of the animation file being parsed (0x0106BFA4..).
struct AnimFileState {
    const char* name = nullptr;   // 0x0106BFA4 name of the next resource
    uint32_t nameHash = 0;        // 0x0106BFA8 HashNameNoCase of the first name
    char nameA[0x41] = {};        // 0x0106C0F0 (read from +1)
    char nameB[0x40] = {};        // 0x0106C131
};
AnimFileState& AnimFile();

// 0x0034B844: chunk handler of a resource group (forgets the name, parses
// the sub-chunks).
bool AnimGroupChunk(ape::iff::IffReader* reader);
// 0x0034B884: chunk handler of a resource name: one byte, then two 64 byte
// names; the second becomes the name, the first is hashed.
bool AnimNameChunk(ape::iff::IffReader* reader);

// Havok's buffered stream reader (0x007DFEF8, 0x24 bytes) over `inner`.
HkStreamReader* HkNewBufferedReader(HkStreamReader* inner, int bufferSize);

} // namespace anim
} // namespace sly
