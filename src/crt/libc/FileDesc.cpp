// openTIT - C runtime file descriptor layer.
//
// Under the console's stdio sits a minimal POSIX-like descriptor layer:
// descriptors 0, 1 and 2 are the debug console (TTY), every other descriptor
// is a cellFs file handle. Here the console descriptors map to the host's
// standard streams and file descriptors to plat::File objects.
#include "crt/libc/Libc.h"

#include <cerrno>
#include <vector>

namespace crt {

namespace {

constexpr int kStdIn = 0;
constexpr int kStdOut = 1;
constexpr int kStdErr = 2;
constexpr int kFirstFileFd = 3;

struct FileTable {
    plat::Mutex mutex;
    std::vector<std::unique_ptr<plat::File>> files; // slot i holds descriptor i + 3
};

FileTable& GetFileTable()
{
    static FileTable table;
    return table;
}

bool IsConsoleFd(int fd)
{
    return fd >= 0 && fd < kFirstFileFd;
}

// Looks up the file behind a descriptor; null (with errno = EBADF) if none.
plat::File* FindFile(FileTable& table, int fd)
{
    const int slot = fd - kFirstFileFd;
    if (slot < 0 || slot >= (int)table.files.size() || !table.files[slot]) {
        errno = EBADF;
        return nullptr;
    }
    return table.files[slot].get();
}

plat::File::Mode ToFileMode(int flags)
{
    if ((flags & kOpen_AccessMask) == kOpen_ReadOnly)
        return plat::File::Mode::Read;
    if (flags & kOpen_Append)
        return plat::File::Mode::Append;
    if (flags & (kOpen_Truncate | kOpen_Create))
        return plat::File::Mode::Write;
    // NOTE: read-write access to an existing file without truncation ("r+")
    // has no plat::File equivalent; such files are opened for reading.
    return plat::File::Mode::Read;
}

int64_t SeekFile(int fd, int64_t offset, int whence)
{
    if (IsConsoleFd(fd)) {
        errno = EBADF;
        return -1;
    }
    FileTable& table = GetFileTable();
    plat::ScopedLock lock(table.mutex);
    plat::File* file = FindFile(table, fd);
    if (!file)
        return -1;
    if (!file->Seek(offset, whence)) {
        errno = EINVAL;
        return -1;
    }
    return file->Tell();
}

} // namespace

// @func 0x00726964 Open
// open(): opens a file and returns its descriptor, or -1 with errno set.
// `mode` (the permission bits for a created file) has no meaning on the PC.
int Open(const char* path, int flags, int /*mode*/)
{
    std::unique_ptr<plat::File> file = plat::File::Open(path, ToFileMode(flags));
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    FileTable& table = GetFileTable();
    plat::ScopedLock lock(table.mutex);
    for (size_t slot = 0; slot < table.files.size(); ++slot) {
        if (!table.files[slot]) {
            table.files[slot] = std::move(file);
            return (int)slot + kFirstFileFd;
        }
    }
    table.files.push_back(std::move(file));
    return (int)table.files.size() - 1 + kFirstFileFd;
}

// @func 0x00726900 Close
// close(): closing a console descriptor is a no-op. Returns 0 or -1.
int Close(int fd)
{
    if (IsConsoleFd(fd))
        return 0;
    FileTable& table = GetFileTable();
    plat::ScopedLock lock(table.mutex);
    if (!FindFile(table, fd))
        return -1;
    table.files[fd - kFirstFileFd].reset();
    return 0;
}

// @func 0x00726A88 Read
// read(): returns the number of bytes read, 0 at end of file, -1 on error.
// From descriptor 0 the console is read one character at a time up to and
// including the first newline (carriage returns read as newlines). Reading
// from descriptors 1 and 2 is refused with a message.
int Read(int fd, void* dst, size_t bytes)
{
    if (fd == kStdIn) {
        if (!dst) {
            errno = EFAULT;
            return -1;
        }
        char* out = static_cast<char*>(dst);
        int count = 0;
        while ((size_t)count < bytes) {
            int c = std::fgetc(stdin);
            if (c == EOF)
                break;
            if (c == '\r')
                c = '\n';
            out[count++] = (char)c;
            if (c == '\n')
                break;
        }
        return count;
    }
    if (fd == kStdOut || fd == kStdErr) {
        std::fputs("read: reading from stdout or stderr (fd 1 & 2) not allowed\n", stderr);
        errno = EINVAL;
        return -1;
    }
    FileTable& table = GetFileTable();
    plat::ScopedLock lock(table.mutex);
    plat::File* file = FindFile(table, fd);
    if (!file)
        return -1;
    return (int)file->Read(dst, bytes);
}

// @func 0x00726CB0 Write
// write(): descriptors 1 and 2 go to the console (stdout / stderr) and
// report the full length as written; writing to descriptor 0 is refused
// with a message. Returns the number of bytes written or -1.
int Write(int fd, const void* src, size_t bytes)
{
    if (fd == kStdOut || fd == kStdErr) {
        if (!src) {
            errno = EFAULT;
            return -1;
        }
        std::fwrite(src, 1, bytes, fd == kStdOut ? stdout : stderr);
        return (int)bytes;
    }
    if (fd == kStdIn) {
        std::fputs("write: writing from stdin (fd 0) not allowed\n", stderr);
        errno = EINVAL;
        return -1;
    }
    FileTable& table = GetFileTable();
    plat::ScopedLock lock(table.mutex);
    plat::File* file = FindFile(table, fd);
    if (!file)
        return -1;
    return (int)file->Write(src, bytes);
}

// @func 0x007269FC LSeek
// lseek(): moves the file position and returns the new offset, or -1.
// Console descriptors cannot seek (EBADF).
int32_t LSeek(int fd, int32_t offset, int whence)
{
    return (int32_t)SeekFile(fd, offset, whence);
}

// @func 0x00726878 LSeek64
// 64-bit lseek().
int64_t LSeek64(int fd, int64_t offset, int whence)
{
    return SeekFile(fd, offset, whence);
}

// @func 0x0072B878 OpenForStream
// _Fopen(): opens the file behind a new stream. `streamMode` holds the
// stream's mode bits as parsed from the fopen() mode string: 1 = read,
// 2 = write, 4 = append, 8 = truncate, 0x10 = create.
int OpenForStream(const char* path, unsigned streamMode)
{
    static const int kAccess[4] = {kOpen_ReadOnly, kOpen_ReadOnly, kOpen_WriteOnly, kOpen_ReadWrite};
    int flags = kAccess[streamMode & 3];
    if (streamMode & 0x04)
        flags |= kOpen_Append;
    if (streamMode & 0x08)
        flags |= kOpen_Truncate;
    if (streamMode & 0x10)
        flags |= kOpen_Create;
    return Open(path, flags, 0666);
}

} // namespace crt
