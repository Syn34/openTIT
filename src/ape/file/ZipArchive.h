// openTIT - Ape engine zip archive reader and archive file objects.
//
// The engine mounts plain .zip archives (Image.zip, and "*.sca" archives whose
// end-of-central-directory signature is scrambled with a 4 byte key stored at
// the very end of the file). On mount the central directory is walked once
// and every entry name (prefixed with the mount prefix) is put into a
// 128-bucket hash table, so later lookups only touch the matching entry.
//
// Entries are either stored or deflated. An open entry keeps its own decode
// state (ZipEntryReader) and shares the archive's underlying stream.
#pragma once

#include "ape/file/ApeFile.h"
#include "ape/file/Inflate.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ape::file {

class ZipArchive;

// Result codes of the zip layer (the engine uses 1 for success).
enum ZipResult : int {
    kZip_Ok          = 1,
    kZip_ReadError   = 0,
    kZip_EndOfList   = -100,
    kZip_BadZipFile  = -102,
    kZip_OutOfMemory = -103,
};

// Lookup flavours for entry names.
enum ZipCaseMode : int {
    kZipCase_Insensitive = 0, // ASCII letters compare equal regardless of case
    kZipCase_Sensitive   = 1,
};

// Decode state of one open entry.
struct ZipEntryReader {
    File* stream = nullptr;          // the archive's stream (not owned)
    int32_t baseOffset = 0;          // bytes in front of the zip data
    uint32_t method = 0;             // 0 = stored, 8 = deflated
    uint64_t dataOffset = 0;         // start of the entry data (relative to baseOffset)
    uint64_t readOffset = 0;         // next compressed byte to fetch
    uint32_t compressedLeft = 0;
    uint32_t uncompressedLeft = 0;
    uint32_t uncompressedSize = 0;
    uint32_t compressedSize = 0;
    uint32_t crc = 0;                // running CRC-32 of the data produced so far
    uint32_t expectedCrc = 0;
    uint32_t produced = 0;           // bytes decoded so far
    uint32_t targetPosition = 0;     // where the next Read() should start
    std::vector<uint8_t> buffer;     // compressed input staging
    size_t bufferPos = 0;
    size_t bufferLen = 0;
    Inflater inflater;
};

class ZipArchive {
public:
    ZipArchive();
    ~ZipArchive();

    // Opens the archive file at `path`; entry names get `mountPrefix` prepended.
    bool OpenPath(const char* path, const char* mountPrefix);
    // Uses an already open stream, which stays owned by the caller.
    bool OpenStream(File* stream, const char* mountPrefix);

    // Makes `name` the current entry. Returns kZip_Ok or kZip_EndOfList.
    int Locate(const char* name, int caseMode);
    // True if `name` is in the archive (does not change the current entry).
    bool Contains(const char* name, int caseMode) const;
    // Opens the current entry for reading. Returns kZip_Ok on success.
    int OpenCurrentEntry(std::unique_ptr<ZipEntryReader>* outReader);

    // Reads from an entry, honouring its target position.
    uint32_t ReadEntry(ZipEntryReader& reader, void* dst, uint32_t bytes);
    // Closes an entry; false if it was read completely and the CRC is wrong.
    bool CloseEntry(std::unique_ptr<ZipEntryReader> reader);

    uint32_t CurrentEntrySize() const { return m_current.uncompressedSize; }
    uint32_t IndexedEntryCount() const { return m_indexedCount; }

private:
    // Central directory record of the current entry.
    struct EntryInfo {
        uint16_t versionMadeBy = 0;
        uint16_t versionNeeded = 0;
        uint16_t flags = 0;
        uint16_t method = 0;
        uint32_t dosDateTime = 0;
        uint32_t crc = 0;
        uint32_t compressedSize = 0;
        uint32_t uncompressedSize = 0;
        uint16_t nameLength = 0;
        uint16_t extraLength = 0;
        uint16_t commentLength = 0;
        uint16_t diskStart = 0;
        uint16_t internalAttr = 0;
        uint32_t externalAttr = 0;
        uint32_t localHeaderOffset = 0;
    };

    struct IndexEntry {
        uint32_t centralOffset;      // offset of the record in the central directory
        char name[256];
        IndexEntry* next;
    };

    static constexpr int kBuckets = 128;

    bool Open(const char* mountPrefix, bool scrambled);
    bool ReadU32(uint32_t* out);
    bool ReadU16(uint16_t* out);
    int CheckLocalHeader(uint32_t* extraFieldSize, uint64_t* dataOffset, uint32_t* localExtraSize);
    int ReadCurrentEntryInfo();
    int BuildIndex();
    uint32_t FindCentralDirEnd(bool scrambled);
    void ReadCentralDirEnd(bool scrambled, int* result, uint32_t* centralDirEnd);
    int GoToNextEntry();
    const IndexEntry* FindIndexEntry(const char* name, int caseMode) const;
    int ReadChunk(ZipEntryReader& reader, uint8_t* dst, uint32_t bytes);
    void RestartEntry(ZipEntryReader& reader);
    void ClearIndex();

    uint8_t m_key[4] = {};           // .sca trailer (EOCD signature scramble key)
    File* m_stream = nullptr;
    bool m_ownsStream = true;
    std::vector<uint8_t> m_scratch;  // skip buffer for forward seeks; also sizes entry buffers
    uint32_t m_entryCount = 0;       // from the end-of-central-directory record
    uint16_t m_commentLength = 0;
    int32_t m_baseOffset = 0;        // bytes before the zip data (self-extractors etc.)
    uint32_t m_entryIndex = 0;       // index of the current entry
    uint32_t m_entryOffset = 0;      // central directory offset of the current entry
    bool m_currentValid = false;
    uint32_t m_centralDirEnd = 0;    // position of the end-of-central-directory record
    uint32_t m_centralDirSize = 0;
    uint32_t m_centralDirOffset = 0;
    EntryInfo m_current;
    char m_currentName[256] = {};
    uint8_t m_currentExtra[16] = {};
    uint8_t m_currentComment[16] = {};
    uint32_t m_indexedCount = 0;
    IndexEntry* m_buckets[kBuckets] = {};
    char m_mountPrefix[256] = {};
};

// File object for an open zip entry.
class ArchiveFile final : public File {
public:
    ArchiveFile(const char* name, ZipArchive* archive, std::unique_ptr<ZipEntryReader> reader);

    bool Close() override;
    uint32_t Read(void* dst, uint64_t bytes, bool* completed) override;
    uint32_t Write(const void* src, uint64_t bytes, bool* completed) override;
    int64_t Seek(int64_t offset, int origin) override;
    uint64_t GetSize() override;

private:
    ~ArchiveFile() override;

    ZipArchive* m_archive;
    std::unique_ptr<ZipEntryReader> m_reader;
};

// --- Mounted archives -------------------------------------------------------

// Mounts the zip at `path`. Returns the new number of mounted archives, or
// 0 on failure.
int MountZipArchive(const char* path, const char* mountPrefix);
// Mounts a zip read from an open stream (the stream stays owned by the caller).
int MountZipStream(File* stream, const char* mountPrefix);

// Checks the mounted archives for `path` / opens it from them.
bool IsInMountedArchive(const char* path);
File* OpenArchiveFile(const char* path);

} // namespace ape::file
