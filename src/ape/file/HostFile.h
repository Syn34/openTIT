// openTIT - Ape engine device files (buffered files on the async file device).
//
// A HostFile opened for reading keeps a small ring of read-ahead blocks
// (ReadCache): a read is served from a cached block if possible, otherwise
// the block is read synchronously, and after every read the next block is
// requested asynchronously so sequential reading overlaps with decoding.
// Files opened for writing collect data in a 32 KiB buffer that is flushed
// when full and on Close().
#pragma once

#include "ape/file/ApeFile.h"
#include "ape/file/AsyncFile.h"

#include <cstdint>

namespace ape::file {

class HostFile;

class ReadCache {
public:
    static constexpr int kMaxBlocks = 2;

    void Init(uint32_t blockSize);
    void Release();
    // Reads from `file` at its current position (does not move it).
    uint64_t Read(HostFile& file, void* dst, uint64_t bytes);

private:
    struct Block {
        uint8_t* data = nullptr;
        uint64_t start = UINT64_MAX;    // file range held, [start, end)
        uint64_t end = UINT64_MAX;
        AsyncFileRequest* pending = nullptr; // read-ahead in flight
    };

    int FindBlock(uint64_t offset, bool waitForPending);
    int NextPrefetchBlock(uint64_t offset, int lastBlock, uint64_t* prefetchOffset);
    int AcquireBlock(uint64_t offset, int lastBlock);
    bool StartPrefetch(HostFile& file, uint64_t offset, int block);
    static void DropPending(Block& block);

    uint8_t* m_memory = nullptr;
    uint32_t m_blockCount = kMaxBlocks;
    uint32_t m_blockSize = 0;
    Block m_blocks[kMaxBlocks];
};

struct WriteBuffer {
    uint8_t* data = nullptr;
    uint32_t capacity = 0;
    uint64_t used = 0;
};

class HostFile final : public File {
public:
    static HostFile* Open(const char* path, const char* mode);

    bool Close() override;
    uint32_t Read(void* dst, uint64_t bytes, bool* completed) override;
    uint32_t Write(const void* src, uint64_t bytes, bool* completed) override;
    int64_t Seek(int64_t offset, int origin) override;
    uint64_t GetSize() override;

private:
    friend class ReadCache;

    explicit HostFile(const char* name);
    ~HostFile() override;

    uint32_t WriteBuffered(const void* src, uint64_t bytes);
    void Flush();

    AsyncFileHandle* m_handle = nullptr;
    // NOTE: 4 for .xmb/.xml files, 0 otherwise; not used by the file code
    // itself (meaning unknown).
    uint32_t m_kindFlags = 0;
    uint64_t m_size = 0;
    ReadCache m_cache;
    WriteBuffer* m_writeBuffer = nullptr;
};

File* OpenHostFile(const char* path, const char* mode);

} // namespace ape::file
