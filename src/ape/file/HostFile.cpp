// openTIT - Ape engine device files (buffered files on the async file device).
#include "ape/file/HostFile.h"

#include "platform/Platform.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ape::file {

namespace {

constexpr uint32_t kReadBlockSize = 0x8000;
constexpr uint32_t kWriteBufferSize = 0x8000;

AsyncFileParams UrgentParams()
{
    AsyncFileParams params;
    params.urgent = true;
    return params;
}

bool EqualsNoCase(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
        if (ca != cb)
            return false;
        if (ca == 0)
            return true;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Read cache
// ---------------------------------------------------------------------------

// Cancels (if still running), waits for and frees a block's read-ahead.
void ReadCache::DropPending(Block& block)
{
    if (!block.pending)
        return;
    if (GetRequestError(block.pending) == kAsyncFileError_InProgress)
        CancelRequest(block.pending);
    WaitRequestAndReport(block.pending);
    DestroyRequest(block.pending);
    block.pending = nullptr;
}

// @func 0x00568490 ReadCache_Init
void ReadCache::Init(uint32_t blockSize)
{
    m_blockSize = blockSize;
    m_memory = static_cast<uint8_t*>(std::malloc((size_t)blockSize * m_blockCount));
    for (uint32_t i = 0; i < m_blockCount; ++i)
        m_blocks[i].data = m_memory ? m_memory + (size_t)i * blockSize : nullptr;
}

// @func 0x005682B4 ReadCache_Release
void ReadCache::Release()
{
    for (uint32_t i = 0; i < m_blockCount; ++i) {
        Block& b = m_blocks[i];
        DropPending(b);
        b.data = nullptr;
        b.start = b.end = UINT64_MAX;
    }
    std::free(m_memory);
    m_memory = nullptr;
}

// Returns the block holding `offset`, or -1. A block whose read-ahead is
// still in flight counts as holding its requested range; with
// `waitForPending` the read is completed first and the block trimmed to what
// was actually read.
// @func 0x00568A28 ReadCache_FindBlock
int ReadCache::FindBlock(uint64_t offset, bool waitForPending)
{
    for (int i = 0; i < (int)m_blockCount; ++i) {
        Block& b = m_blocks[i];
        if (b.start == UINT64_MAX || offset < b.start || offset >= b.end)
            continue;
        if (!b.pending || !waitForPending)
            return i;

        WaitRequestAndReport(b.pending);
        const uint32_t error = GetRequestError(b.pending);
        const uint64_t transferred = GetRequestTransferred(b.pending);
        DropPending(b);
        if (error == kAsyncFileError_None) {
            b.end = b.start + transferred;
            if (offset >= b.start && offset < b.end)
                return i;
        } else {
            b.start = b.end = UINT64_MAX;
        }
    }
    return -1;
}

// Picks the block the next read-ahead should go to and the file offset it
// should start at. Returns -1 when the data after `offset` is already
// cached or being fetched.
// @func 0x00568D30 ReadCache_NextPrefetchBlock
int ReadCache::NextPrefetchBlock(uint64_t offset, int lastBlock, uint64_t* prefetchOffset)
{
    const int current = FindBlock(offset, false);
    if (prefetchOffset)
        *prefetchOffset = offset;
    if (current < 0) {
        if (lastBlock >= 0)
            return (lastBlock + 1) % (int)m_blockCount;
        for (uint32_t i = 0; i < m_blockCount; ++i)
            if (m_blocks[i].start == UINT64_MAX)
                return (int)i;
        return 0;
    }
    if (prefetchOffset)
        *prefetchOffset = m_blocks[current].end;
    const int next = (current + 1) % (int)m_blockCount;
    if (m_blocks[current].end == m_blocks[next].start)
        return -1;
    return next;
}

// Chooses a block to receive a synchronous read of `offset` and empties it.
// @func 0x00568E74 ReadCache_AcquireBlock
int ReadCache::AcquireBlock(uint64_t offset, int lastBlock)
{
    int index;
    if (lastBlock >= 0) {
        index = (lastBlock + 1) % (int)m_blockCount;
    } else {
        const int current = FindBlock(offset, false);
        if (current < 0) {
            index = 0;
            for (uint32_t i = 0; i < m_blockCount; ++i) {
                if (m_blocks[i].start == UINT64_MAX) {
                    index = (int)i;
                    break;
                }
            }
        } else {
            index = (current + 1) % (int)m_blockCount;
            if (m_blocks[current].end == m_blocks[index].start)
                index = 0;
        }
    }
    Block& b = m_blocks[index];
    DropPending(b);
    b.start = b.end = UINT64_MAX;
    return index;
}

bool ReadCache::StartPrefetch(HostFile& file, uint64_t offset, int block)
{
    Block& b = m_blocks[block];
    const uint64_t length = std::min<uint64_t>(m_blockSize, file.m_size - offset);
    DropPending(b);
    b.start = offset;
    b.end = offset + length;
    b.pending = AsyncRead(file.m_handle, b.data, length, offset, UrgentParams());
    if (!b.pending)
        b.start = b.end = UINT64_MAX;
    return b.pending != nullptr;
}

uint64_t ReadCache::Read(HostFile& file, void* dst, uint64_t bytes)
{
    uint8_t* out = static_cast<uint8_t*>(dst);
    uint64_t position = file.m_position;
    bytes = std::min(bytes, file.m_size - std::min(position, file.m_size));

    uint64_t copied = 0;
    int block = -1;
    int lastBlock = -1;
    while (bytes != 0) {
        if (block < 0)
            block = FindBlock(position, true);
        if (block < 0) {
            // Miss: read the block synchronously, then copy on the next pass.
            block = AcquireBlock(position, lastBlock);
            Block& b = m_blocks[block];
            const uint64_t length = std::min<uint64_t>(m_blockSize, file.m_size - position);
            const uint64_t n = ReadFileSync(file.m_handle, b.data, length, position, UrgentParams());
            b.start = position;
            b.end = position + n;
            if (n == 0)
                return copied;
            continue;
        }
        Block& b = m_blocks[block];
        const uint64_t n = std::min(bytes, b.end - position);
        std::memcpy(out, b.data + (position - b.start), (size_t)n);
        out += n;
        bytes -= n;
        copied += n;
        position += n;
        lastBlock = block;
        block = -1;
    }

    if (position < file.m_size) {
        uint64_t prefetchOffset = 0;
        const int next = NextPrefetchBlock(position, lastBlock, &prefetchOffset);
        if (next >= 0 && prefetchOffset < file.m_size)
            StartPrefetch(file, prefetchOffset, next);
    }
    return copied;
}

// ---------------------------------------------------------------------------
// Host files
// ---------------------------------------------------------------------------

HostFile::HostFile(const char* name) : File(name) {}

HostFile::~HostFile()
{
    m_cache.Release();
    if (m_writeBuffer) {
        std::free(m_writeBuffer->data);
        delete m_writeBuffer;
    }
}

// @func 0x005706FC HostFile_Open
HostFile* HostFile::Open(const char* path, const char* mode)
{
    // (The PS3 build formats "%s - Open" for a debug print compiled out of release.)
    HostFile* file = new HostFile(path);

    AsyncFileParams params = UrgentParams();
    if (IsQuiet())
        params.flags |= kAsyncFileFlag_Quiet;
    file->m_handle = OpenFileSync(path, mode, params);

    const char* ext = GetFileExtension(path);
    const bool isXml = ext && (EqualsNoCase(ext, "xmb") || EqualsNoCase(ext, "xml"));
    file->m_kindFlags = isXml ? 4 : 0;

    const bool ok = file->m_handle != nullptr;
    if (IsReadMode(mode)) {
        if (ok) {
            file->m_size = file->GetSize();
            file->m_cache.Init(kReadBlockSize);
        }
    } else {
        file->m_writeBuffer = new WriteBuffer();
        file->m_writeBuffer->capacity = kWriteBufferSize;
        file->m_writeBuffer->data = static_cast<uint8_t*>(std::malloc(kWriteBufferSize));
    }
    if (!ok) {
        delete file;
        return nullptr;
    }
    return file;
}

// @func 0x00570B64 OpenHostFile
File* OpenHostFile(const char* path, const char* mode)
{
    return HostFile::Open(path, mode);
}

void HostFile::Flush()
{
    if (!m_writeBuffer || m_writeBuffer->used == 0)
        return;
    const uint64_t written = WriteFileSync(m_handle, m_writeBuffer->data, m_writeBuffer->used, m_position,
                                           UrgentParams());
    m_position += written;
    m_writeBuffer->used = 0;
}

// Flushes pending writes, closes the device file and destroys the object.
// @func 0x00571DC4 HostFile_Close
bool HostFile::Close()
{
    Flush();
    if (m_writeBuffer) {
        std::free(m_writeBuffer->data);
        delete m_writeBuffer;
        m_writeBuffer = nullptr;
    }
    CloseFileSync(m_handle, UrgentParams());
    m_handle = nullptr;
    delete this;
    return true;
}

// @func 0x00571F70 HostFile_Read
uint32_t HostFile::Read(void* dst, uint64_t bytes, bool* completed)
{
    const uint32_t n = (uint32_t)m_cache.Read(*this, dst, bytes);
    m_position += n;
    if (completed)
        *completed = true;
    return n;
}

// Appends to the write buffer, writing it out each time it fills up.
uint32_t HostFile::WriteBuffered(const void* src, uint64_t bytes)
{
    WriteBuffer& wb = *m_writeBuffer;
    const uint8_t* in = static_cast<const uint8_t*>(src);
    uint64_t left = bytes;
    while (left != 0) {
        const uint64_t n = std::min<uint64_t>(left, wb.capacity - wb.used);
        std::memcpy(wb.data + wb.used, in, (size_t)n);
        wb.used += n;
        in += n;
        left -= n;
        if (wb.used == wb.capacity)
            Flush();
    }
    return (uint32_t)bytes;
}

// @func 0x00571FE4 HostFile_Write
uint32_t HostFile::Write(const void* src, uint64_t bytes, bool* completed)
{
    const uint32_t n = m_writeBuffer ? WriteBuffered(src, bytes) : 0;
    if (completed)
        *completed = true;
    return n;
}

// @func 0x0057203C HostFile_Seek
int64_t HostFile::Seek(int64_t offset, int origin)
{
    switch (origin) {
    case kSeek_Set: m_position = (uint64_t)offset; break;
    case kSeek_Cur: m_position += (uint64_t)offset; break;
    case kSeek_End: m_position = m_size - (uint64_t)offset; break;
    default: m_position = 0; break;
    }
    return (int64_t)m_position;
}

// The size is queried once from the device and cached.
// @func 0x00572118 HostFile_GetSize
uint64_t HostFile::GetSize()
{
    if (m_size == 0)
        m_size = GetFileSizeSync(m_handle, UrgentParams());
    return m_size;
}

} // namespace ape::file
