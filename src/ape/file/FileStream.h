// openTIT - Positional byte streams behind the asynchronous file device.
//
// FIOS reads and writes always carry an explicit file offset. A FileStream is
// what an AsyncFileHandle refers to: either a host file or one entry of a
// mounted PSARC archive.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace plat { class File; }

namespace ape::file {

class FileStream {
public:
    virtual ~FileStream() = default;
    // Return the number of bytes transferred; short counts mean end of file
    // (reads) or an I/O error.
    virtual size_t ReadAt(uint64_t offset, void* dst, size_t bytes) = 0;
    virtual size_t WriteAt(uint64_t offset, const void* src, size_t bytes) = 0;
    virtual uint64_t Size() const = 0;
    virtual bool Ok() const { return true; } // false after a decode failure
    const std::string& Path() const { return m_path; }

protected:
    std::string m_path;
};

// Wraps a host file opened through the platform layer.
std::unique_ptr<FileStream> MakeHostStream(std::unique_ptr<plat::File> file, const char* path);

// Normalises a game path for archive lookups: '\\' -> '/', console mount
// prefixes and leading slashes removed, ASCII lower case.
std::string NormalizeArchivePath(const char* gamePath);

} // namespace ape::file
