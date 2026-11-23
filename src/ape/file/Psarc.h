// openTIT - PlayStation archive (.psarc) reader.
//
// On PS3 FIOS mounts Sly4.psarc itself; this is the PC replacement used by
// the asynchronous file device. Format (all integers big-endian):
//
//   header   "PSAR", u16 major, u16 minor, char compression[4] ("zlib"),
//            u32 tocLength (header + toc + block table), u32 tocEntrySize,
//            u32 entryCount, u32 blockSize, u32 archiveFlags
//   toc      entryCount x { u8 md5[16], u32 firstBlock, u40 size, u40 offset }
//   blocks   compressed size of every block, in as many bytes as needed to
//            hold blockSize - 1 (0 = a full, stored block)
//
// Entry 0 is the manifest: the newline separated names of entries 1..n.
// Each block is either stored or an independent zlib stream.
#pragma once

#include "ape/file/FileStream.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace plat { class File; }

namespace ape::file {

class PsarcArchive {
public:
    enum Flags : uint32_t {
        kFlag_IgnoreCase    = 1, // name hashes computed over upper case names
        kFlag_AbsolutePaths = 2, // names start with '/'
    };

    static std::unique_ptr<PsarcArchive> Open(std::unique_ptr<plat::File> file, const char* path);
    ~PsarcArchive();

    // Looks up a game path; returns the entry index or -1.
    int Find(const char* gamePath) const;
    std::unique_ptr<FileStream> OpenEntry(int index);

    size_t EntryCount() const { return m_entries.size(); }
    const std::string& Path() const { return m_path; }

private:
    struct Entry {
        uint8_t md5[16];
        uint32_t firstBlock;
        uint64_t size;
        uint64_t offset;
        std::string name;
    };
    friend class PsarcEntryStream;

    PsarcArchive() = default;
    bool ReadToc();
    bool ReadManifest();
    // Reads the raw (possibly compressed) bytes of one block.
    bool ReadRaw(uint64_t offset, void* dst, size_t bytes);
    // Decodes block `block` of `entry`, whose uncompressed length is `length`.
    bool DecodeBlock(const Entry& entry, uint32_t block, uint8_t* dst, size_t length);
    std::vector<uint8_t> ReadWholeEntry(const Entry& entry);

    std::unique_ptr<plat::File> m_file;
    std::mutex m_fileLock;            // m_file is shared by all entry streams
    std::string m_path;
    uint32_t m_blockSize = 0;
    uint32_t m_flags = 0;
    std::vector<Entry> m_entries;
    std::vector<uint32_t> m_blockSizes;
    std::vector<uint64_t> m_blockStart; // running sum of stored block sizes
    std::unordered_map<std::string, int> m_byName;
};

} // namespace ape::file
