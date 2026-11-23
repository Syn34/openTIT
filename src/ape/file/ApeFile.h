// openTIT - Ape engine file objects and file system front end.
//
// Everything the engine reads or writes goes through ape::file::File, an
// abstract stream with three implementations:
//
//   HostFile    a file on the device (see HostFile.h), buffered
//   ArchiveFile an entry of a mounted .zip archive (see ZipArchive.h)
//   MemoryFile  a caller supplied memory block (the boot "RamDisk")
//
// OpenFile() picks the implementation: read-only opens are served from the
// mounted zip archives first, everything else goes to the device. Releasing
// a file is done with Close(), which also destroys the object.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape::file {

enum SeekOrigin : int {
    kSeek_Set = 0,
    kSeek_Cur = 1,
    // NOTE: every file type in the original computes `size - offset` for
    // this origin (not `size + offset`); kept as is.
    kSeek_End = 2,
};

class File {
public:
    explicit File(const char* name);
    virtual ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    // Flushes and releases the file, then deletes the object.
    virtual bool Close() = 0;
    // Both return the number of bytes transferred. `completed` (optional)
    // is set once the transfer finished; every implementation is blocking.
    virtual uint32_t Read(void* dst, uint64_t bytes, bool* completed = nullptr) = 0;
    virtual uint32_t Write(const void* src, uint64_t bytes, bool* completed = nullptr) = 0;
    virtual int64_t Seek(int64_t offset, int origin) = 0;
    virtual uint64_t GetSize() = 0;

    const char* GetName() const { return m_name; }
    uint64_t GetPosition() const { return m_position; }
    uint64_t GetOpenTime() const { return m_openTime; }

protected:
    static constexpr size_t kMaxName = 256;
    char m_name[kMaxName];
    uint64_t m_position = 0;
    uint64_t m_openTime = 0; // time base value when the file was opened
};

// A read/write view of a caller owned memory block.
class MemoryFile final : public File {
public:
    MemoryFile(void* data, uint64_t size, const char* name);

    bool Close() override;
    uint32_t Read(void* dst, uint64_t bytes, bool* completed) override;
    uint32_t Write(const void* src, uint64_t bytes, bool* completed) override;
    int64_t Seek(int64_t offset, int origin) override;
    uint64_t GetSize() override { return m_size; }

private:
    ~MemoryFile() override = default;

    uint8_t* m_data;
    uint64_t m_size;
};

// --- Opening files ----------------------------------------------------------

// Opens `path` with a C stdio style mode string ("rb", "wb", "ab").
File* OpenFile(const char* path, const char* mode);
bool FileExists(const char* path);

// Reads a whole file into a newly allocated buffer (free with
// FreeFileData). Returns null if the file cannot be opened.
void* LoadFile(const char* path, uint32_t* outSize);
void FreeFileData(void* data);

// Wraps a memory block in a File. Returns null for an empty block.
File* CreateMemoryFile(void* data, uint64_t size, const char* name);

// --- Path helpers -----------------------------------------------------------

// Pointer to the text after the last '.', the whole string when there is
// no '.', or null when the only '.' is the first character.
const char* GetFileExtension(const char* path);
bool StringHasChar(const char* s, char c);
// True for modes that open for reading ("r", "rb", "r+").
bool IsReadMode(const char* mode);
// Drops any "device:" prefix and makes sure the file system root prefix is
// part of the path. May return a temporary buffer.
const char* MakeRootedPath(const char* path);
// Full host style path of a game file ("<media root>/<path>", '/' separated).
const char* GetDevicePath(const char* path);

// --- File system state ------------------------------------------------------

using FileOpenHook = void (*)(const char* path);

// Logging of every open (debug builds) and the hook it calls.
void SetFileOpenLogging(bool enabled, FileOpenHook hook);
void SetRootPrefix(const char* prefix);
const char* GetRootPrefix();

// While a QuietScope is alive, failing opens are not reported (used when
// probing for optional files).
class QuietScope {
public:
    QuietScope();
    ~QuietScope();
    QuietScope(const QuietScope&) = delete;
    QuietScope& operator=(const QuietScope&) = delete;
};
bool IsQuiet();

// Mounts a .psarc archive on the device; see ape::file::MountArchive.
struct AsyncFileHandle;
struct AsyncFileParams;
AsyncFileHandle* MountDeviceArchive(const char* path, const AsyncFileParams& params);

// Mounts "Image.zip" from the data root if present (and not disabled).
bool MountImageArchive();
extern bool g_disableImageArchive; // NOTE: global at 0x010C5A7C, set by unknown code

} // namespace ape::file
