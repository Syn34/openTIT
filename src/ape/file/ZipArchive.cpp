// openTIT - Ape engine zip archive reader and archive file objects.
#include "ape/file/ZipArchive.h"

#include "ape/core/ApeString.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace ape::file {

namespace {

constexpr uint32_t kLocalHeaderSignature   = 0x04034B50; // "PK\3\4"
constexpr uint32_t kCentralHeaderSignature = 0x02014B50; // "PK\1\2"
constexpr uint8_t kEndOfCentralDirSignature[4] = { 'P', 'K', 5, 6 };
constexpr uint32_t kLocalHeaderSize = 30;
constexpr uint32_t kCentralHeaderSize = 46;
constexpr uint32_t kScratchSize = 0x4000;
constexpr uint32_t kMaxCommentSearch = 0xFFFF;
constexpr uint32_t kSearchChunk = 0x400;

// Mounted archives, searched in mount order.
std::vector<ZipArchive*> g_archives;
size_t g_archivePeak = 0;

uint32_t Crc32(uint32_t crc, const uint8_t* data, size_t length)
{
    static const struct Table {
        uint32_t v[256];
        Table()
        {
            for (uint32_t i = 0; i < 256; ++i) {
                uint32_t c = i;
                for (int k = 0; k < 8; ++k)
                    c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                v[i] = c;
            }
        }
    } table;
    crc = ~crc;
    for (size_t i = 0; i < length; ++i)
        crc = table.v[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

// Compares two names, folding only the ASCII letters A-Z to lower case.
int CompareNoCase(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
        if (ca != cb)
            return ca < cb ? -1 : 1;
        if (ca == 0)
            return 0;
    }
}

int CompareNames(const char* a, const char* b, int caseMode)
{
    return caseMode == kZipCase_Sensitive ? std::strcmp(a, b) : CompareNoCase(a, b);
}

// Index hash: 37 * h + c over the name with upper case letters folded down.
uint32_t HashEntryName(const char* name)
{
    uint32_t h = 0;
    for (const char* p = name; *p; ++p) {
        int c = (signed char)*p;
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        h = h * 37u + (uint32_t)c;
    }
    return h;
}

// Converts '\\' to '/' and drops everything up to and including the first
// "./" (so "./Textures/a.dds" and "Textures/a.dds" are the same entry).
const char* NormalizeEntryName(const char* name, char (&buffer)[512])
{
    size_t len = std::strlen(name);
    if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;
    for (size_t i = 0; i < len; ++i)
        buffer[i] = name[i] == '\\' ? '/' : name[i];
    buffer[len] = '\0';
    const char* dot = std::strstr(buffer, "./");
    return dot ? dot + 2 : buffer;
}

int RegisterArchive(ZipArchive* archive)
{
    g_archives.push_back(archive);
    g_archivePeak = std::max(g_archivePeak, g_archives.size());
    return (int)g_archives.size();
}

} // namespace

// ---------------------------------------------------------------------------
// Archive
// ---------------------------------------------------------------------------

ZipArchive::ZipArchive() : m_scratch(kScratchSize) {}

ZipArchive::~ZipArchive()
{
    ClearIndex();
    if (m_ownsStream && m_stream)
        m_stream->Close();
}

void ZipArchive::ClearIndex()
{
    for (IndexEntry*& bucket : m_buckets) {
        while (bucket) {
            IndexEntry* next = bucket->next;
            delete bucket;
            bucket = next;
        }
    }
    m_indexedCount = 0;
}

// Reads a little-endian 32-bit value from the archive stream.
// @func 0x00559534 ZipArchive_ReadU32
bool ZipArchive::ReadU32(uint32_t* out)
{
    uint8_t b[4];
    const bool ok = m_stream->Read(b, 4) == 4;
    *out = ok ? (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24) : 0;
    return ok;
}

bool ZipArchive::ReadU16(uint16_t* out)
{
    uint8_t b[2];
    const bool ok = m_stream->Read(b, 2) == 2;
    *out = ok ? (uint16_t)(b[0] | (b[1] << 8)) : 0;
    return ok;
}

// Verifies that the local header of the current entry agrees with its
// central directory record. Outputs the combined size of the local name and
// extra field, the offset of the local extra field and its size.
// @func 0x005596F0 ZipArchive_CheckLocalHeader
int ZipArchive::CheckLocalHeader(uint32_t* extraFieldSize, uint64_t* dataOffset, uint32_t* localExtraSize)
{
    *extraFieldSize = 0;
    *dataOffset = 0;
    *localExtraSize = 0;
    m_stream->Seek((int64_t)m_current.localHeaderOffset + m_baseOffset, kSeek_Set);

    int result = kZip_Ok;
    auto fail = [&result](int r) {
        if (result == kZip_Ok || r == kZip_ReadError)
            result = r;
    };

    uint32_t signature, value32;
    uint16_t version, flags, method, value16;
    if (!ReadU32(&signature))
        fail(kZip_ReadError);
    else if (signature != kLocalHeaderSignature)
        fail(kZip_BadZipFile);
    if (!ReadU16(&version))
        fail(kZip_ReadError);
    if (!ReadU16(&flags))
        fail(kZip_ReadError);
    if (!ReadU16(&method))
        fail(kZip_ReadError);
    else if (method != m_current.method || (m_current.method != 0 && m_current.method != 8))
        fail(kZip_BadZipFile);

    if (!ReadU32(&value32)) // DOS date/time, not checked
        fail(kZip_ReadError);
    // With bit 3 set the CRC and sizes follow the data instead.
    const bool deferredSizes = (flags & 8) != 0;
    const uint32_t expected[3] = { m_current.crc, m_current.compressedSize, m_current.uncompressedSize };
    for (uint32_t want : expected) {
        if (!ReadU32(&value32))
            fail(kZip_ReadError);
        else if (value32 != want && !deferredSizes)
            fail(kZip_BadZipFile);
    }

    uint16_t nameLength = 0;
    if (!ReadU16(&nameLength))
        fail(kZip_ReadError);
    else if (nameLength != m_current.nameLength)
        fail(kZip_BadZipFile);
    if (!ReadU16(&value16))
        fail(kZip_ReadError);

    *extraFieldSize = (uint32_t)nameLength + value16;
    *dataOffset = (uint64_t)m_current.localHeaderOffset + kLocalHeaderSize + nameLength;
    *localExtraSize = value16;
    return result;
}

// Opens the current entry: validates its local header and prepares the
// decoder (raw deflate for method 8, plain copy for stored entries).
// @func 0x00559D5C ZipArchive_OpenCurrentEntry
int ZipArchive::OpenCurrentEntry(std::unique_ptr<ZipEntryReader>* outReader)
{
    uint32_t headerExtra = 0, localExtraSize = 0;
    uint64_t localExtraOffset = 0;
    if (CheckLocalHeader(&headerExtra, &localExtraOffset, &localExtraSize) != kZip_Ok)
        return kZip_BadZipFile;

    auto reader = std::make_unique<ZipEntryReader>();
    reader->buffer.resize(m_scratch.size());
    reader->stream = m_stream;
    reader->baseOffset = m_baseOffset;
    reader->method = m_current.method;
    reader->expectedCrc = m_current.crc;
    reader->compressedSize = m_current.compressedSize;
    reader->uncompressedSize = m_current.uncompressedSize;
    reader->dataOffset = (uint64_t)m_current.localHeaderOffset + kLocalHeaderSize + headerExtra;
    RestartEntry(*reader);
    *outReader = std::move(reader);
    return kZip_Ok;
}

void ZipArchive::RestartEntry(ZipEntryReader& reader)
{
    reader.readOffset = reader.dataOffset;
    reader.compressedLeft = reader.compressedSize;
    reader.uncompressedLeft = reader.uncompressedSize;
    reader.crc = 0;
    reader.produced = 0;
    reader.bufferPos = reader.bufferLen = 0;
    if (reader.method != 0) {
        ZipEntryReader* r = &reader;
        reader.inflater.Reset([r](uint8_t* dst, size_t capacity) -> size_t {
            // Fetch the next slice of compressed data from the archive.
            const uint32_t n = (uint32_t)std::min<uint64_t>(capacity, r->compressedLeft);
            if (n == 0)
                return 0;
            r->stream->Seek((int64_t)r->readOffset + r->baseOffset, kSeek_Set);
            if (r->stream->Read(dst, n) != n)
                return 0;
            r->readOffset += n;
            r->compressedLeft -= n;
            return n;
        });
    }
}

// Reads the central directory record at m_entryOffset into m_current.
// @func 0x0055A8E4 ZipArchive_ReadCurrentEntryInfo
int ZipArchive::ReadCurrentEntryInfo()
{
    int result = kZip_Ok;
    auto check = [&result](bool ok) {
        if (!ok)
            result = kZip_ReadError;
    };

    m_stream->Seek((int64_t)m_entryOffset + m_baseOffset, kSeek_Set);
    uint32_t signature;
    if (!ReadU32(&signature))
        result = kZip_ReadError;
    else if (signature != kCentralHeaderSignature)
        result = kZip_BadZipFile;

    EntryInfo& e = m_current;
    check(ReadU16(&e.versionMadeBy));
    check(ReadU16(&e.versionNeeded));
    check(ReadU16(&e.flags));
    check(ReadU16(&e.method));
    check(ReadU32(&e.dosDateTime));
    check(ReadU32(&e.crc));
    check(ReadU32(&e.compressedSize));
    check(ReadU32(&e.uncompressedSize));
    check(ReadU16(&e.nameLength));
    check(ReadU16(&e.extraLength));
    check(ReadU16(&e.commentLength));
    check(ReadU16(&e.diskStart));
    check(ReadU16(&e.internalAttr));
    check(ReadU32(&e.externalAttr));
    check(ReadU32(&e.localHeaderOffset));

    // Name (truncated to 255 characters), then the first 16 bytes of the
    // extra field and of the comment.
    uint32_t skip = e.nameLength;
    if (result == kZip_Ok && e.nameLength != 0) {
        const uint32_t n = std::min<uint32_t>(e.nameLength, sizeof(m_currentName) - 1);
        check(m_stream->Read(m_currentName, n) == n);
        m_currentName[n] = '\0';
        skip = e.nameLength - n;
    }
    if (result == kZip_Ok) {
        const uint32_t n = std::min<uint32_t>(e.extraLength, sizeof(m_currentExtra));
        if (skip)
            m_stream->Seek(skip, kSeek_Cur);
        if (e.extraLength)
            check(m_stream->Read(m_currentExtra, n) == n);
        skip = e.extraLength - n;
    } else {
        skip += e.extraLength;
    }
    if (result == kZip_Ok) {
        const uint32_t n = std::min<uint32_t>(e.commentLength, sizeof(m_currentComment));
        if (skip)
            m_stream->Seek(skip, kSeek_Cur);
        if (e.commentLength)
            check(m_stream->Read(m_currentComment, n) == n);
    }
    m_currentValid = result == kZip_Ok;
    return result;
}

int ZipArchive::GoToNextEntry()
{
    if (!m_currentValid || m_entryIndex + 1 == m_entryCount)
        return kZip_EndOfList;
    ++m_entryIndex;
    m_entryOffset += kCentralHeaderSize + m_current.nameLength + m_current.extraLength + m_current.commentLength;
    const int result = ReadCurrentEntryInfo();
    if (!m_currentValid)
        m_currentName[0] = '\0';
    return result;
}

// Walks the whole central directory and hashes every "<prefix><name>" into
// the bucket table. Returns the number of names indexed.
// @func 0x0055C01C ZipArchive_BuildIndex
int ZipArchive::BuildIndex()
{
    if (!m_currentValid)
        return 0;
    ClearIndex();

    m_entryIndex = 0;
    m_entryOffset = m_centralDirOffset;
    ReadCurrentEntryInfo();
    if (!m_currentValid)
        m_currentName[0] = '\0';

    uint32_t count = 0;
    int result;
    do {
        if (m_currentName[0] != '\0') {
            IndexEntry* entry = new IndexEntry();
            const size_t prefixLen = std::min(std::strlen(m_mountPrefix), sizeof(entry->name) - 1);
            std::memcpy(entry->name, m_mountPrefix, prefixLen);
            StrCopyBounded(entry->name + prefixLen, m_currentName, sizeof(entry->name) - prefixLen);
            entry->centralOffset = m_entryOffset;
            IndexEntry*& bucket = m_buckets[HashEntryName(entry->name) % kBuckets];
            entry->next = bucket;
            bucket = entry;
            ++count;
        }
        result = GoToNextEntry();
    } while (result == kZip_Ok);

    m_indexedCount = count;
    return (int)count;
}

// Scans backwards from the end of the file (at most 64 KiB, the largest
// possible archive comment) for the end-of-central-directory signature.
// In scrambled (.sca) archives the last 4 bytes are a key that is XORed
// over the signature. Returns the record's offset, 0 if not found.
// @func 0x0055C780 ZipArchive_FindCentralDirEnd
uint32_t ZipArchive::FindCentralDirEnd(bool scrambled)
{
    uint32_t fileSize = (uint32_t)m_stream->Seek(0, kSeek_End);
    if (scrambled) {
        fileSize -= 4;
        m_stream->Seek(fileSize, kSeek_Set);
        m_stream->Read(m_key, 4);
    }
    const uint32_t maxBack = std::min(kMaxCommentSearch, fileSize);

    uint8_t buffer[kSearchChunk + 4];
    uint32_t back = 4;
    uint32_t found = 0;
    while (back < maxBack) {
        back = std::min(back + kSearchChunk, maxBack);
        const uint32_t readPos = fileSize - back;
        const uint32_t readSize = std::min<uint32_t>(sizeof(buffer), fileSize - readPos);
        m_stream->Seek(readPos, kSeek_Set);
        if (m_stream->Read(buffer, readSize) != readSize)
            break;
        for (int i = (int)readSize - 4; i >= 0; --i) {
            bool match = true;
            for (int k = 0; k < 4 && match; ++k) {
                uint8_t b = buffer[i + k];
                if (scrambled)
                    b ^= m_key[k];
                match = b == kEndOfCentralDirSignature[k];
            }
            if (match) {
                found = readPos + (uint32_t)i;
                break;
            }
        }
        if (found)
            break;
    }
    return found;
}

// Locates and parses the end-of-central-directory record. Single-disk
// archives only.
// @func 0x0055CF1C ZipArchive_ReadCentralDirEnd
void ZipArchive::ReadCentralDirEnd(bool scrambled, int* result, uint32_t* centralDirEnd)
{
    *result = kZip_Ok;
    if (!m_stream) {
        *result = kZip_ReadError;
        return;
    }
    *centralDirEnd = FindCentralDirEnd(scrambled);
    if (*centralDirEnd == 0)
        *result = kZip_ReadError;

    auto check = [result](bool ok) {
        if (!ok)
            *result = kZip_ReadError;
    };
    m_stream->Seek(*centralDirEnd, kSeek_Set);
    uint32_t signature;
    uint16_t diskNumber, centralDirDisk, entriesOnDisk, value16;
    check(ReadU32(&signature)); // scrambled in .sca files, so not compared
    check(ReadU16(&diskNumber));
    check(ReadU16(&centralDirDisk));
    check(ReadU16(&entriesOnDisk));
    m_entryCount = entriesOnDisk;
    check(ReadU16(&value16));
    if (value16 != m_entryCount || centralDirDisk != 0 || diskNumber != 0)
        *result = kZip_BadZipFile;
    check(ReadU32(&m_centralDirSize));
    check(ReadU32(&m_centralDirOffset));
    check(ReadU16(&m_commentLength));
    if (*centralDirEnd < m_centralDirOffset + m_centralDirSize && *result == kZip_Ok)
        *result = kZip_BadZipFile;
}

bool ZipArchive::Open(const char* mountPrefix, bool scrambled)
{
    int result;
    uint32_t centralDirEnd = 0;
    ReadCentralDirEnd(scrambled, &result, &centralDirEnd);
    // NOTE: the original carries on (and crashes) without a stream.
    if (!m_stream)
        return false;
    m_centralDirEnd = centralDirEnd;
    m_baseOffset = (int32_t)(centralDirEnd - (m_centralDirOffset + m_centralDirSize));

    if (mountPrefix) {
        char buffer[512];
        StrCopyBounded(m_mountPrefix, NormalizeEntryName(mountPrefix, buffer), sizeof(m_mountPrefix));
    } else {
        m_mountPrefix[0] = '\0';
    }

    m_entryOffset = m_centralDirOffset;
    m_entryIndex = 0;
    ReadCurrentEntryInfo();
    if (!m_currentValid)
        m_currentName[0] = '\0';
    BuildIndex();
    return true;
}

// @func 0x0055D494 ZipArchive_OpenPath
bool ZipArchive::OpenPath(const char* path, const char* mountPrefix)
{
    const bool scrambled = std::strstr(path, ".sca") != nullptr;
    m_stream = OpenFile(path, "rb");
    m_ownsStream = true;
    return Open(mountPrefix, scrambled);
}

// @func 0x005677A4 ZipArchive_OpenStream
bool ZipArchive::OpenStream(File* stream, const char* mountPrefix)
{
    m_stream = stream;
    m_ownsStream = false;
    return Open(mountPrefix, false);
}

const ZipArchive::IndexEntry* ZipArchive::FindIndexEntry(const char* name, int caseMode) const
{
    char buffer[512];
    const char* key = NormalizeEntryName(name, buffer);
    for (const IndexEntry* e = m_buckets[HashEntryName(key) % kBuckets]; e; e = e->next)
        if (CompareNames(key, e->name, caseMode) == 0)
            return e;
    return nullptr;
}

// @func 0x0055BA70 ZipArchive_Contains
bool ZipArchive::Contains(const char* name, int caseMode) const
{
    return FindIndexEntry(name, caseMode) != nullptr;
}

// @func 0x0055B3C8 ZipArchive_Locate
int ZipArchive::Locate(const char* name, int caseMode)
{
    if (std::strlen(name) >= sizeof(m_currentName))
        return kZip_ReadError;
    if (!m_currentValid)
        return kZip_EndOfList;
    if (m_currentName[0] != '\0' && CompareNames(m_currentName, name, caseMode) == 0)
        return kZip_Ok;

    const uint32_t savedIndex = m_entryIndex;
    const uint32_t savedOffset = m_entryOffset;
    if (const IndexEntry* entry = FindIndexEntry(name, caseMode)) {
        m_entryOffset = entry->centralOffset;
        ReadCurrentEntryInfo();
        if (m_currentValid)
            return kZip_Ok;
        m_currentName[0] = '\0';
    }
    m_entryIndex = savedIndex;
    m_entryOffset = savedOffset;
    return kZip_EndOfList;
}

// Decodes up to `bytes` bytes of the entry from its current decode position.
// @func 0x0056736C ZipArchive_ReadChunk
int ZipArchive::ReadChunk(ZipEntryReader& reader, uint8_t* dst, uint32_t bytes)
{
    if (reader.buffer.empty())
        return kZip_EndOfList;
    bytes = std::min(bytes, reader.uncompressedLeft);
    if (bytes == 0)
        return 0;

    uint32_t total = 0;
    if (reader.method == 0) {
        while (total < bytes) {
            if (reader.bufferPos == reader.bufferLen) {
                const uint32_t n = (uint32_t)std::min<uint64_t>(reader.buffer.size(), reader.compressedLeft);
                if (n == 0)
                    break;
                reader.stream->Seek((int64_t)reader.readOffset + reader.baseOffset, kSeek_Set);
                if (reader.stream->Read(reader.buffer.data(), n) != n)
                    return 0;
                reader.readOffset += n;
                reader.compressedLeft -= n;
                reader.bufferPos = 0;
                reader.bufferLen = n;
            }
            const uint32_t n = (uint32_t)std::min<size_t>(bytes - total, reader.bufferLen - reader.bufferPos);
            std::memcpy(dst + total, reader.buffer.data() + reader.bufferPos, n);
            reader.bufferPos += n;
            total += n;
        }
    } else {
        total = (uint32_t)reader.inflater.Read(dst, bytes);
        if (reader.inflater.Failed())
            return kZip_BadZipFile;
    }
    reader.crc = Crc32(reader.crc, dst, total);
    reader.uncompressedLeft -= total;
    reader.produced += total;
    return (int)total;
}

// Reads from the entry's target position. Forward seeks are performed by
// decoding into the scratch buffer.
// @func 0x00567938 ZipArchive_ReadEntry
uint32_t ZipArchive::ReadEntry(ZipEntryReader& reader, void* dst, uint32_t bytes)
{
    // NOTE: the original never rewinds, so a backward seek silently kept
    // reading from the decoder's position. Restart the entry instead.
    if (reader.produced > reader.targetPosition)
        RestartEntry(reader);
    while (reader.produced < reader.targetPosition) {
        const uint32_t n = std::min<uint32_t>(reader.targetPosition - reader.produced, (uint32_t)m_scratch.size());
        if (ReadChunk(reader, m_scratch.data(), n) <= 0)
            return 0;
    }
    const int n = ReadChunk(reader, static_cast<uint8_t*>(dst), bytes);
    if (n <= 0)
        return 0;
    reader.targetPosition += (uint32_t)n;
    return (uint32_t)n;
}

bool ZipArchive::CloseEntry(std::unique_ptr<ZipEntryReader> reader)
{
    if (!reader)
        return false;
    return !(reader->uncompressedLeft == 0 && reader->crc != reader->expectedCrc);
}

// ---------------------------------------------------------------------------
// Archive file objects
// ---------------------------------------------------------------------------

ArchiveFile::ArchiveFile(const char* name, ZipArchive* archive, std::unique_ptr<ZipEntryReader> reader)
    : File(name), m_archive(archive), m_reader(std::move(reader))
{
}

// @func 0x007A4DF8 ArchiveFile::~ArchiveFile
// @func 0x007A4E08 ArchiveFile::~ArchiveFile (deleting)
ArchiveFile::~ArchiveFile() = default;

// Ends the entry; fails when the data was read to the end and its CRC does
// not match. NOTE: the original never freed the ArchiveFile object itself.
// @func 0x00568034 ArchiveFile_Close
bool ArchiveFile::Close()
{
    const bool ok = m_archive->CloseEntry(std::move(m_reader));
    delete this;
    return ok;
}

// @func 0x00568118 ArchiveFile_Read
uint32_t ArchiveFile::Read(void* dst, uint64_t bytes, bool* completed)
{
    const uint32_t n = m_archive->ReadEntry(*m_reader, dst, (uint32_t)std::min<uint64_t>(bytes, UINT32_MAX));
    m_position += n;
    return n;
}

// Archives are read-only.
// @func 0x00568168 ArchiveFile_Write
uint32_t ArchiveFile::Write(const void* src, uint64_t bytes, bool* completed)
{
    plat::Log(plat::LogLevel::Warning, "%s - Write %d bytes to %p\n", m_name, (int)bytes, src);
    return 0;
}

// @func 0x005681A4 ArchiveFile_Seek
int64_t ArchiveFile::Seek(int64_t offset, int origin)
{
    switch (origin) {
    case kSeek_Set:
        m_reader->targetPosition = (uint32_t)offset;
        m_position = (uint64_t)offset;
        break;
    case kSeek_Cur:
        m_reader->targetPosition += (uint32_t)offset;
        m_position += (uint64_t)offset;
        break;
    case kSeek_End:
        m_reader->targetPosition = m_reader->uncompressedSize - (uint32_t)offset;
        m_position = GetSize() - (uint64_t)offset;
        break;
    default:
        break;
    }
    return 0;
}

// NOTE: the original returns the size of the archive's *current* entry,
// which is only this file's entry until another file is opened from the same
// archive; we keep the size with the entry.
// @func 0x0056829C ArchiveFile_GetSize
uint64_t ArchiveFile::GetSize()
{
    return m_reader->uncompressedSize;
}

// ---------------------------------------------------------------------------
// Mounting
// ---------------------------------------------------------------------------

// @func 0x0055D684 MountZipArchive
int MountZipArchive(const char* path, const char* mountPrefix)
{
    ZipArchive* archive = new ZipArchive();
    if (!archive->OpenPath(path, mountPrefix)) {
        delete archive;
        return 0;
    }
    return RegisterArchive(archive);
}

// @func 0x00567F2C MountZipStream
int MountZipStream(File* stream, const char* mountPrefix)
{
    ZipArchive* archive = new ZipArchive();
    if (!archive->OpenStream(stream, mountPrefix)) {
        delete archive;
        return 0;
    }
    return RegisterArchive(archive);
}

// Unmounts archive `handle` (1-based mount index). The last archive takes
// the freed slot, as on the console. Returns false for an empty slot.
// @func 0x0055E350 UnmountZipArchive
bool UnmountZipArchive(int handle)
{
    const size_t index = size_t(uint16_t(handle)) - 1;
    if (index >= g_archives.size() || !g_archives[index])
        return false;
    delete g_archives[index];
    g_archives[index] = g_archives.back();
    g_archives.pop_back();
    return true;
}

bool IsInMountedArchive(const char* path)
{
    for (const ZipArchive* archive : g_archives)
        if (archive->Contains(path, kZipCase_Insensitive))
            return true;
    return false;
}

// @func 0x0055B87C OpenArchiveFile
File* OpenArchiveFile(const char* path)
{
    const char* rooted = MakeRootedPath(path);
    for (ZipArchive* archive : g_archives) {
        if (archive->Locate(rooted, kZipCase_Insensitive) != kZip_Ok)
            continue;
        std::unique_ptr<ZipEntryReader> reader;
        if (archive->OpenCurrentEntry(&reader) != kZip_Ok)
            return nullptr;
        return new ArchiveFile(rooted, archive, std::move(reader));
    }
    return nullptr;
}

} // namespace ape::file
