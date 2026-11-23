// openTIT - Ape engine file objects and file system front end.
#include "ape/file/ApeFile.h"

#include "ape/core/ApeString.h"
#include "ape/file/AsyncFile.h"
#include "ape/file/HostFile.h"
#include "ape/file/ZipArchive.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ape::file {

bool g_disableImageArchive = false;

namespace {

// File system globals (0x01164CE0 onwards on PS3).
struct FileSystemState {
    bool logOpens = false;            // flag bit 7
    int quietDepth = 0;               // flag bits 5-6: nested QuietScopes
    char rootPrefix[256] = "";        // prepended to archive lookups
    FileOpenHook openHook = nullptr;
    int imageArchiveCount = 0;        // archive count right after mounting Image.zip
};

FileSystemState g_fs;

// Hook for platform specific file system setup; empty on PS3.
// @func 0x00570B88 PlatformFileSystemInit
void PlatformFileSystemInit()
{
}

} // namespace

// ---------------------------------------------------------------------------
// File base
// ---------------------------------------------------------------------------

// NOTE: the PS3 constructor also pings a profiler hook (0x005DBB60).
// @func 0x005594C4 File_File
File::File(const char* name)
{
    StrCopyBounded(m_name, name ? name : "", sizeof(m_name));
    m_position = 0;
    do {
        m_openTime = plat::GetTimeBase();
    } while (m_openTime == 0);
}

// One destructor covers the complete and deleting variants the compiler
// emitted for File (and an identical copy).
// @func 0x00560B28 File_Destroy
// @func 0x00560B38 File_Delete
// @func 0x00560B80 File_Destroy2
File::~File() = default;

// ---------------------------------------------------------------------------
// Memory files
// ---------------------------------------------------------------------------

MemoryFile::MemoryFile(void* data, uint64_t size, const char* name)
    : File(name), m_data(static_cast<uint8_t*>(data)), m_size(size)
{
}

// @func 0x00567124 MemoryFile_Close
bool MemoryFile::Close()
{
    m_data = nullptr;
    m_position = 0;
    m_size = 0;
    delete this;
    return true;
}

// @func 0x00567184 MemoryFile_Read
uint32_t MemoryFile::Read(void* dst, uint64_t bytes, bool* completed)
{
    if (!m_data)
        return 0;
    const uint64_t left = m_size > m_position ? m_size - m_position : 0;
    const uint32_t n = (uint32_t)std::min(bytes, left);
    if (n)
        std::memcpy(dst, m_data + m_position, n);
    m_position += n;
    if (completed)
        *completed = true;
    return n;
}

// @func 0x0056724C MemoryFile_Write
uint32_t MemoryFile::Write(const void* src, uint64_t bytes, bool* completed)
{
    if (!m_data)
        return 0;
    const uint64_t left = m_size > m_position ? m_size - m_position : 0;
    const uint32_t n = (uint32_t)std::min(bytes, left);
    if (n)
        std::memcpy(m_data + m_position, src, n);
    m_position += n;
    if (completed)
        *completed = true;
    return n;
}

// @func 0x0056731C MemoryFile_Seek
int64_t MemoryFile::Seek(int64_t offset, int origin)
{
    switch (origin) {
    case kSeek_Set: m_position = (uint64_t)offset; break;
    case kSeek_Cur: m_position += (uint64_t)offset; break;
    case kSeek_End: m_position = m_size - (uint64_t)offset; break;
    default: m_position = 0; break;
    }
    return (int64_t)m_position;
}

// @func 0x00567034 CreateMemoryFile
File* CreateMemoryFile(void* data, uint64_t size, const char* name)
{
    if (!data || (uint32_t)size == 0)
        return nullptr;
    return new MemoryFile(data, size, name);
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

// @func 0x005611DC GetFileExtension
const char* GetFileExtension(const char* path)
{
    if (!path)
        return nullptr;
    int i = (int)std::strlen(path);
    while (i >= 0 && path[i] != '.')
        --i;
    if (i == 0)
        return nullptr;
    return path + i + 1; // i == -1 (no '.') yields the whole string
}

// @func 0x00561270 StringHasChar
bool StringHasChar(const char* s, char c)
{
    return std::strchr(s, c) != nullptr;
}

// @func 0x0055BD00 IsReadMode
bool IsReadMode(const char* mode)
{
    return std::strchr(mode, 'r') != nullptr;
}

// @func 0x0055B7D4 MakeRootedPath
const char* MakeRootedPath(const char* path)
{
    static char s_stripped[512];
    if (const char* colon = std::strchr(path, ':')) {
        StrCopyBounded(s_stripped, colon + 1, sizeof(s_stripped));
        path = s_stripped;
    }
    if (!std::strstr(path, g_fs.rootPrefix))
        path = TempFormat("%s%s", g_fs.rootPrefix, path);
    return path;
}

// NOTE: on PS3 the media root is the game content directory reported by
// cellGameBootCheck; here it is the data root of the platform layer.
// @func 0x00570604 GetDevicePath
const char* GetDevicePath(const char* path)
{
    static char s_path[1024];
    std::string relative = MakeRootedPath(path);
    std::string root = plat::GetDataRoot();
    while (!root.empty() && (root.back() == '/' || root.back() == '\\'))
        root.pop_back();
    std::snprintf(s_path, sizeof(s_path), "%s/%s", root.c_str(), relative.c_str());
    for (char* p = s_path; *p; ++p)
        if (*p == '\\')
            *p = '/';
    return s_path;
}

// ---------------------------------------------------------------------------
// File system state
// ---------------------------------------------------------------------------

void SetFileOpenLogging(bool enabled, FileOpenHook hook)
{
    g_fs.logOpens = enabled;
    g_fs.openHook = hook;
}

void SetRootPrefix(const char* prefix)
{
    StrCopyBounded(g_fs.rootPrefix, prefix ? prefix : "", sizeof(g_fs.rootPrefix));
}

const char* GetRootPrefix()
{
    return g_fs.rootPrefix;
}

QuietScope::QuietScope() { ++g_fs.quietDepth; }
QuietScope::~QuietScope() { --g_fs.quietDepth; }
bool IsQuiet() { return g_fs.quietDepth != 0; }

// ---------------------------------------------------------------------------
// Opening files
// ---------------------------------------------------------------------------

// Read-only opens are served from the mounted zip archives when the file is
// in one of them (paths escaping upwards with "../" never are); everything
// else goes to the device.
// @func 0x0055BD3C OpenFile
File* OpenFile(const char* path, const char* mode)
{
    if (IsReadMode(mode)) {
        // NOTE: the PS3 build calls the hook with an empty string; the
        // formatting of the message was compiled out.
        if (g_fs.logOpens && g_fs.openHook)
            g_fs.openHook(path);
        if (!std::strstr(path, "../") && IsInMountedArchive(path))
            return OpenArchiveFile(path);
    }
    return OpenHostFile(path, mode);
}

// @func 0x0055D78C FileExists
bool FileExists(const char* path)
{
    QuietScope quiet;
    File* file = OpenFile(path, "rb");
    if (!file)
        return false;
    file->Close();
    return true;
}

// @func 0x00560B90 LoadFile
void* LoadFile(const char* path, uint32_t* outSize)
{
    if (outSize)
        *outSize = 0;
    File* file = OpenFile(path, "rb");
    if (!file)
        return nullptr;
    const uint32_t size = (uint32_t)file->GetSize();
    void* data = std::malloc(size ? size : 1);
    if (!data) {
        file->Close(); // NOTE: leaked by the original
        return nullptr;
    }
    file->Read(data, size);
    if (outSize)
        *outSize = size;
    file->Close();
    return data;
}

// @func 0x005611B0 FreeFileData
void FreeFileData(void* data)
{
    std::free(data);
}

// @func 0x00562944 MountDeviceArchive
AsyncFileHandle* MountDeviceArchive(const char* path, const AsyncFileParams& params)
{
    return MountArchive(path, params);
}

// @func 0x0055D830 MountImageArchive
bool MountImageArchive()
{
    SetRootPrefix("");
    PlatformFileSystemInit();
    if (!g_disableImageArchive && FileExists("Image.zip"))
        g_fs.imageArchiveCount = MountZipArchive("Image.zip", nullptr);
    return true;
}

} // namespace ape::file
