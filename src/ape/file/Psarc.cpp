// openTIT - PlayStation archive (.psarc) reader and host file streams.
#include "ape/file/Psarc.h"

#include "ape/file/Inflate.h"
#include "ape/file/Md5.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace ape::file {

// ---------------------------------------------------------------------------
// Host streams and path helpers
// ---------------------------------------------------------------------------
namespace {

class HostStream final : public FileStream {
public:
    HostStream(std::unique_ptr<plat::File> file, const char* path) : m_file(std::move(file))
    {
        m_path = path ? path : "";
    }
    size_t ReadAt(uint64_t offset, void* dst, size_t bytes) override
    {
        if (!m_file->Seek((int64_t)offset, SEEK_SET))
            return 0;
        return m_file->Read(dst, bytes);
    }
    size_t WriteAt(uint64_t offset, const void* src, size_t bytes) override
    {
        if (!m_file->Seek((int64_t)offset, SEEK_SET))
            return 0;
        return m_file->Write(src, bytes);
    }
    uint64_t Size() const override { return (uint64_t)m_file->Size(); }

private:
    std::unique_ptr<plat::File> m_file;
};

uint32_t ReadBE(const uint8_t* p, int bytes)
{
    uint32_t v = 0;
    for (int i = 0; i < bytes; ++i)
        v = (v << 8) | p[i];
    return v;
}

uint64_t ReadBE40(const uint8_t* p)
{
    return ((uint64_t)p[0] << 32) | ReadBE(p + 1, 4);
}

} // namespace

std::unique_ptr<FileStream> MakeHostStream(std::unique_ptr<plat::File> file, const char* path)
{
    if (!file)
        return nullptr;
    return std::make_unique<HostStream>(std::move(file), path);
}

std::string NormalizeArchivePath(const char* gamePath)
{
    std::string p = gamePath ? gamePath : "";
    for (char& c : p) {
        if (c == '\\')
            c = '/';
        else if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
    }
    static const char* const kMounts[] = { "/app_home/", "/dev_bdvd/ps3_game/usrdir/", "/dev_hdd0/game/" };
    for (const char* m : kMounts) {
        size_t n = std::strlen(m);
        if (p.compare(0, n, m) == 0) {
            p.erase(0, n);
            break;
        }
    }
    while (!p.empty() && p.front() == '/')
        p.erase(0, 1);
    while (p.compare(0, 2, "./") == 0)
        p.erase(0, 2);
    return p;
}

// ---------------------------------------------------------------------------
// Entry stream: decodes blocks on demand, caching the last one.
// ---------------------------------------------------------------------------
class PsarcEntryStream final : public FileStream {
public:
    PsarcEntryStream(PsarcArchive& archive, int index) : m_archive(archive), m_entry(archive.m_entries[index])
    {
        m_path = m_entry.name;
    }

    size_t ReadAt(uint64_t offset, void* dst, size_t bytes) override
    {
        uint8_t* out = static_cast<uint8_t*>(dst);
        size_t done = 0;
        const uint64_t blockSize = m_archive.m_blockSize;
        while (done < bytes && offset < m_entry.size) {
            const uint32_t block = (uint32_t)(offset / blockSize);
            if (!LoadBlock(block))
                break;
            const size_t inBlock = (size_t)(offset - (uint64_t)block * blockSize);
            const size_t n = std::min(bytes - done, m_cache.size() - inBlock);
            std::memcpy(out + done, m_cache.data() + inBlock, n);
            done += n;
            offset += n;
        }
        return done;
    }
    size_t WriteAt(uint64_t, const void*, size_t) override { return 0; } // archives are read-only
    uint64_t Size() const override { return m_entry.size; }
    bool Ok() const override { return !m_failed; }

private:
    bool LoadBlock(uint32_t block)
    {
        if (m_cachedBlock == (int64_t)block)
            return true;
        const uint64_t blockSize = m_archive.m_blockSize;
        const uint64_t start = (uint64_t)block * blockSize;
        const size_t length = (size_t)std::min<uint64_t>(blockSize, m_entry.size - start);
        m_cache.resize(length);
        if (!m_archive.DecodeBlock(m_entry, block, m_cache.data(), length)) {
            m_failed = true;
            m_cachedBlock = -1;
            return false;
        }
        m_cachedBlock = block;
        return true;
    }

    PsarcArchive& m_archive;
    const PsarcArchive::Entry& m_entry;
    std::vector<uint8_t> m_cache;
    int64_t m_cachedBlock = -1;
    bool m_failed = false;
};

// ---------------------------------------------------------------------------
// Archive
// ---------------------------------------------------------------------------
PsarcArchive::~PsarcArchive() = default;

std::unique_ptr<PsarcArchive> PsarcArchive::Open(std::unique_ptr<plat::File> file, const char* path)
{
    if (!file)
        return nullptr;
    std::unique_ptr<PsarcArchive> archive(new PsarcArchive());
    archive->m_file = std::move(file);
    archive->m_path = path ? path : "";
    if (!archive->ReadToc() || !archive->ReadManifest())
        return nullptr;
    return archive;
}

bool PsarcArchive::ReadRaw(uint64_t offset, void* dst, size_t bytes)
{
    std::lock_guard<std::mutex> lock(m_fileLock);
    return m_file->Seek((int64_t)offset, SEEK_SET) && m_file->Read(dst, bytes) == bytes;
}

bool PsarcArchive::ReadToc()
{
    uint8_t header[32];
    if (!ReadRaw(0, header, sizeof(header)) || std::memcmp(header, "PSAR", 4) != 0) {
        plat::Log(plat::LogLevel::Error, "%s: not a PSARC archive\n", m_path.c_str());
        return false;
    }
    if (std::memcmp(header + 8, "zlib", 4) != 0) {
        plat::Log(plat::LogLevel::Error, "%s: unsupported compression '%.4s'\n", m_path.c_str(),
                  (const char*)header + 8);
        return false;
    }
    const uint32_t tocLength = ReadBE(header + 12, 4);
    const uint32_t entrySize = ReadBE(header + 16, 4);
    const uint32_t entryCount = ReadBE(header + 20, 4);
    m_blockSize = ReadBE(header + 24, 4);
    m_flags = ReadBE(header + 28, 4);
    if (entrySize < 30 || entryCount == 0 || m_blockSize == 0 || tocLength < 32 ||
        (uint64_t)entrySize * entryCount > tocLength - 32)
        return false;

    std::vector<uint8_t> toc(tocLength - 32);
    if (!ReadRaw(32, toc.data(), toc.size()))
        return false;

    m_entries.resize(entryCount);
    for (uint32_t i = 0; i < entryCount; ++i) {
        const uint8_t* e = toc.data() + (size_t)i * entrySize;
        Entry& entry = m_entries[i];
        std::memcpy(entry.md5, e, 16);
        entry.firstBlock = ReadBE(e + 16, 4);
        entry.size = ReadBE40(e + 20);
        entry.offset = ReadBE40(e + 25);
    }

    // Width of a block size field: enough bytes for blockSize - 1.
    int width = 1;
    while (width < 4 && ((uint64_t)1 << (8 * width)) < m_blockSize)
        ++width;
    const size_t tableBytes = toc.size() - (size_t)entrySize * entryCount;
    const uint8_t* table = toc.data() + (size_t)entrySize * entryCount;
    m_blockSizes.resize(tableBytes / width);
    m_blockStart.resize(m_blockSizes.size() + 1);
    m_blockStart[0] = 0;
    for (size_t i = 0; i < m_blockSizes.size(); ++i) {
        m_blockSizes[i] = ReadBE(table + i * width, width);
        m_blockStart[i + 1] = m_blockStart[i] + (m_blockSizes[i] ? m_blockSizes[i] : m_blockSize);
    }
    return true;
}

bool PsarcArchive::DecodeBlock(const Entry& entry, uint32_t block, uint8_t* dst, size_t length)
{
    const size_t index = (size_t)entry.firstBlock + block;
    if (index >= m_blockSizes.size())
        return false;
    // Blocks of one entry are stored back to back starting at its offset.
    const uint64_t offset = entry.offset + (m_blockStart[index] - m_blockStart[entry.firstBlock]);

    const uint32_t stored = m_blockSizes[index] ? m_blockSizes[index] : m_blockSize;
    if (stored == length && m_blockSizes[index] == 0)
        return ReadRaw(offset, dst, length);

    std::vector<uint8_t> raw(stored);
    if (!ReadRaw(offset, raw.data(), raw.size()))
        return false;
    // A block that did not shrink is kept uncompressed. Compressed blocks are
    // zlib streams (0x78 header byte).
    if (stored == length && !(raw.size() >= 2 && raw[0] == 0x78 && ((raw[0] << 8) | raw[1]) % 31 == 0)) {
        std::memcpy(dst, raw.data(), length);
        return true;
    }
    const size_t n = Inflater::InflateBuffer(raw.data(), raw.size(), dst, length, true);
    if (n == length)
        return true;
    if (stored == length) { // header lookalike that was really stored data
        std::memcpy(dst, raw.data(), length);
        return true;
    }
    plat::Log(plat::LogLevel::Error, "%s: corrupt block %u\n", m_path.c_str(), (unsigned)index);
    return false;
}

std::vector<uint8_t> PsarcArchive::ReadWholeEntry(const Entry& entry)
{
    std::vector<uint8_t> data((size_t)entry.size);
    size_t done = 0;
    uint32_t block = 0;
    while (done < data.size()) {
        const size_t length = std::min<size_t>(m_blockSize, data.size() - done);
        if (!DecodeBlock(entry, block, data.data() + done, length))
            return {};
        done += length;
        ++block;
    }
    return data;
}

bool PsarcArchive::ReadManifest()
{
    std::vector<uint8_t> manifest = ReadWholeEntry(m_entries[0]);
    if (manifest.empty() && m_entries[0].size != 0)
        return false;
    m_entries[0].name = "<manifest>";

    std::vector<std::string> names;
    size_t start = 0;
    for (size_t i = 0; i <= manifest.size(); ++i) {
        if (i == manifest.size() || manifest[i] == '\n' || manifest[i] == '\0') {
            if (i > start) {
                std::string name((const char*)manifest.data() + start, i - start);
                if (!name.empty() && name.back() == '\r')
                    name.pop_back();
                names.push_back(std::move(name));
            }
            start = i + 1;
        }
    }

    // Names are listed in table order; the MD5 of each name (upper-cased when
    // the archive ignores case) identifies its entry, which lets us cope with
    // archives whose table was sorted by hash.
    std::unordered_map<std::string, int> byHash;
    for (size_t i = 1; i < m_entries.size(); ++i)
        byHash.emplace(std::string((const char*)m_entries[i].md5, 16), (int)i);

    for (size_t n = 0; n < names.size(); ++n) {
        std::string hashed = names[n];
        if (m_flags & kFlag_IgnoreCase)
            for (char& c : hashed)
                if (c >= 'a' && c <= 'z')
                    c = (char)(c - 'a' + 'A');
        const Md5Digest digest = Md5(hashed.data(), hashed.size());
        int index = (int)n + 1;
        auto it = byHash.find(std::string((const char*)digest.bytes, 16));
        if (it != byHash.end())
            index = it->second;
        if (index >= (int)m_entries.size())
            continue;
        m_entries[index].name = names[n];
        m_byName.emplace(NormalizeArchivePath(names[n].c_str()), index);
    }
    plat::Log(plat::LogLevel::Info, "Mounted %s (%u files)\n", m_path.c_str(),
              (unsigned)m_byName.size());
    return true;
}

int PsarcArchive::Find(const char* gamePath) const
{
    auto it = m_byName.find(NormalizeArchivePath(gamePath));
    return it == m_byName.end() ? -1 : it->second;
}

std::unique_ptr<FileStream> PsarcArchive::OpenEntry(int index)
{
    if (index <= 0 || index >= (int)m_entries.size())
        return nullptr;
    return std::make_unique<PsarcEntryStream>(*this, index);
}

} // namespace ape::file
