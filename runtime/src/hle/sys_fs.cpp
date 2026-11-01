// sys_fs (cellFs*): file I/O against the VFS.
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "../hle.h"
#include "../log.h"
#include "../vfs.h"

using namespace opentit;
using namespace opentit::rtm;
namespace fs = std::filesystem;

namespace {

// CELL_FS_O_* (octal in the SDK)
constexpr u32 O_ACCMODE = 3, O_WRONLY = 1, O_RDWR = 2, O_CREAT = 0x40, O_TRUNC = 0x200, O_APPEND = 0x400;

struct DirEntry {
    std::string name;
    bool is_dir;
    u64 size;
};

struct Handle {
    std::FILE* file = nullptr;
    fs::path path;
    bool is_dir = false;
    std::vector<DirEntry> entries;
    std::size_t dir_pos = 0;
};

std::mutex g_fs_mutex;
std::unordered_map<u32, Handle> g_handles;
u32 g_next_fd = 3;

int seek64(std::FILE* f, s64 off, int whence) {
#if defined(_WIN32)
    return _fseeki64(f, off, whence);
#else
    return fseeko(f, off, whence);
#endif
}
s64 tell64(std::FILE* f) {
#if defined(_WIN32)
    return _ftelli64(f);
#else
    return ftello(f);
#endif
}

// CellFsStat: s32 mode, uid, gid; s64 atime, mtime, ctime; u64 size, blksize (packed, 0x34 bytes)
void write_stat(u32 out, bool is_dir, u64 size) {
    mem::w32(out + 0, is_dir ? (0040000 | 0777) : (0100000 | 0666));
    mem::w32(out + 4, 0);
    mem::w32(out + 8, 0);
    mem::w64(out + 12, 0);
    mem::w64(out + 20, 0);
    mem::w64(out + 28, 0);
    mem::w64(out + 36, is_dir ? 0 : size);
    mem::w64(out + 44, 0x1000);
}

// CellFsDirent: u8 type, u8 namlen, char name[256]
void write_dirent(u32 out, const DirEntry& e) {
    mem::w8(out, e.is_dir ? 1 : 2);
    const std::size_t n = std::min<std::size_t>(e.name.size(), 255);
    mem::w8(out + 1, static_cast<u8>(n));
    mem::write_bytes(out + 2, e.name.data(), n);
    mem::w8(out + 2 + static_cast<u32>(n), 0);
}

std::optional<fs::path> host_path(u32 guest_path_addr, std::string* guest) {
    *guest = mem::read_string(guest_path_addr, 1024);
    return vfs::translate(*guest);
}

}  // namespace

OPENTIT_HLE(sys_fs, cellFsOpen) {
    // (const char* path, s32 flags, s32* fd, const void* arg, u64 size)
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    const u32 flags = arg32(ctx, 1);
    if (!p) {
        ret(ctx, CELL_ENOENT);
        return;
    }
    std::error_code ec;
    if (fs::is_directory(*p, ec)) {
        ret(ctx, CELL_EISDIR);
        return;
    }
    const char* mode = "rb";
    const u32 acc = flags & O_ACCMODE;
    if (acc == O_WRONLY || acc == O_RDWR) {
        if (flags & O_TRUNC) mode = acc == O_RDWR ? "w+b" : "wb";
        else if (flags & O_APPEND) mode = acc == O_RDWR ? "a+b" : "ab";
        else if (fs::exists(*p, ec)) mode = "r+b";
        else if (flags & O_CREAT) mode = acc == O_RDWR ? "w+b" : "wb";
        else {
            ret(ctx, CELL_ENOENT);
            return;
        }
        fs::create_directories(p->parent_path(), ec);
    }
    std::FILE* f = std::fopen(p->string().c_str(), mode);
    if (!f) {
        LOG_DEBUG("fs", "cellFsOpen('%s') -> ENOENT", guest.c_str());
        ret(ctx, CELL_ENOENT);
        return;
    }
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    const u32 fd = g_next_fd++;
    Handle h;
    h.file = f;
    h.path = *p;
    g_handles[fd] = std::move(h);
    mem::w32(arg32(ctx, 2), fd);
    LOG_INFO("fs", "cellFsOpen('%s', 0x%X) -> fd %u", guest.c_str(), flags, fd);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsSdataOpen) {
    // Same signature as cellFsOpen; SDATA (NPD-wrapped) files are not decrypted yet.
    hle::todo("cellFsSdataOpen (treated as a plain file)");
    hle_cellFsOpen(ctx);
}

OPENTIT_HLE(sys_fs, cellFsClose) {
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || it->second.is_dir) {
        ret(ctx, CELL_EBADF);
        return;
    }
    std::fclose(it->second.file);
    g_handles.erase(it);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsRead) {
    // (fd, void* buf, u64 nbytes, u64* nread)
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || !it->second.file) {
        ret(ctx, CELL_EBADF);
        return;
    }
    const u64 n = std::fread(mem::ptr(arg32(ctx, 1)), 1, arg(ctx, 2), it->second.file);
    if (arg32(ctx, 3)) mem::w64(arg32(ctx, 3), n);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsWrite) {
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || !it->second.file) {
        ret(ctx, CELL_EBADF);
        return;
    }
    const u64 n = std::fwrite(mem::ptr(arg32(ctx, 1)), 1, arg(ctx, 2), it->second.file);
    if (arg32(ctx, 3)) mem::w64(arg32(ctx, 3), n);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsLseek) {
    // (fd, s64 offset, s32 whence, u64* pos)
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || !it->second.file) {
        ret(ctx, CELL_EBADF);
        return;
    }
    const int whence = arg32(ctx, 2) == 1 ? SEEK_CUR : arg32(ctx, 2) == 2 ? SEEK_END : SEEK_SET;
    if (seek64(it->second.file, static_cast<s64>(arg(ctx, 1)), whence) != 0) {
        ret(ctx, CELL_EINVAL);
        return;
    }
    if (arg32(ctx, 3)) mem::w64(arg32(ctx, 3), static_cast<u64>(tell64(it->second.file)));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsFstat) {
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end()) {
        ret(ctx, CELL_EBADF);
        return;
    }
    std::error_code ec;
    const u64 size = it->second.is_dir ? 0 : fs::file_size(it->second.path, ec);
    write_stat(arg32(ctx, 1), it->second.is_dir, size);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsStat) {
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    std::error_code ec;
    if (!p || !fs::exists(*p, ec)) {
        LOG_DEBUG("fs", "cellFsStat('%s') -> ENOENT", guest.c_str());
        ret(ctx, CELL_ENOENT);
        return;
    }
    const bool dir = fs::is_directory(*p, ec);
    write_stat(arg32(ctx, 1), dir, dir ? 0 : fs::file_size(*p, ec));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsOpendir) {
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    std::error_code ec;
    if (!p || !fs::is_directory(*p, ec)) {
        ret(ctx, CELL_ENOENT);
        return;
    }
    Handle h;
    h.is_dir = true;
    h.path = *p;
    h.entries.push_back({".", true, 0});
    h.entries.push_back({"..", true, 0});
    for (const auto& e : fs::directory_iterator(*p, ec)) {
        const bool d = e.is_directory(ec);
        h.entries.push_back({e.path().filename().string(), d, d ? 0 : static_cast<u64>(e.file_size(ec))});
    }
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    const u32 fd = g_next_fd++;
    g_handles[fd] = std::move(h);
    mem::w32(arg32(ctx, 1), fd);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsReaddir) {
    // (fd, CellFsDirent*, u64* nread)
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || !it->second.is_dir) {
        ret(ctx, CELL_EBADF);
        return;
    }
    Handle& h = it->second;
    if (h.dir_pos >= h.entries.size()) {
        mem::w64(arg32(ctx, 2), 0);
    } else {
        write_dirent(arg32(ctx, 1), h.entries[h.dir_pos++]);
        mem::w64(arg32(ctx, 2), 258);
    }
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsGetDirectoryEntries) {
    // (fd, CellFsDirectoryEntry* entries, u32 entries_size, u32* data_count)
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    auto it = g_handles.find(arg32(ctx, 0));
    if (it == g_handles.end() || !it->second.is_dir) {
        ret(ctx, CELL_EBADF);
        return;
    }
    Handle& h = it->second;
    constexpr u32 kEntrySize = 0x34 + 258 + 2;  // stat + dirent, padded to 4
    const u32 max = arg32(ctx, 2) / kEntrySize;
    u32 count = 0;
    u32 out = arg32(ctx, 1);
    while (count < max && h.dir_pos < h.entries.size()) {
        const auto& e = h.entries[h.dir_pos++];
        write_stat(out, e.is_dir, e.size);
        write_dirent(out + 0x34, e);
        out += kEntrySize;
        ++count;
    }
    mem::w32(arg32(ctx, 3), count);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsClosedir) {
    std::lock_guard<std::mutex> lk(g_fs_mutex);
    ret(ctx, g_handles.erase(arg32(ctx, 0)) ? CELL_OK : CELL_EBADF);
}

OPENTIT_HLE(sys_fs, cellFsMkdir) {
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    std::error_code ec;
    if (!p) {
        ret(ctx, CELL_ENOENT);
        return;
    }
    if (fs::exists(*p, ec)) {
        ret(ctx, CELL_EEXIST);
        return;
    }
    fs::create_directories(*p, ec);
    ret(ctx, ec ? CELL_EIO : CELL_OK);
}

OPENTIT_HLE(sys_fs, cellFsRmdir) {
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    std::error_code ec;
    ret(ctx, p && fs::remove(*p, ec) ? CELL_OK : CELL_ENOENT);
}

OPENTIT_HLE(sys_fs, cellFsUnlink) {
    std::string guest;
    auto p = host_path(arg32(ctx, 0), &guest);
    std::error_code ec;
    ret(ctx, p && fs::remove(*p, ec) ? CELL_OK : CELL_ENOENT);
}

OPENTIT_HLE(sys_fs, cellFsGetFreeSize) {
    // (path, u32* block_size, u64* block_count)
    mem::w32(arg32(ctx, 1), 4096);
    mem::w64(arg32(ctx, 2), 10ull * 1024 * 1024 * 1024 / 4096);  // report 10 GiB free
    ret(ctx, CELL_OK);
}

OPENTIT_HLE_OK(sys_fs, cellFsFsync)

// ---------------------------------------------------------------- AIO
// CellFsAio: u32 fd; u64 offset; u32 buf; u64 size; u64 user_data
// Requests complete synchronously and invoke the guest callback
// func(CellFsAio* aio, s32 error, s32 id, u64 size) before returning.
namespace {
u32 g_aio_id = 1;

void aio_transfer(PPUContext& ctx, bool write) {
    const u32 aio = arg32(ctx, 0);
    const u32 id_out = arg32(ctx, 1);
    const u32 func = arg32(ctx, 2);
    const u32 fd = mem::r32(aio);
    const u64 offset = mem::r64(aio + 8);
    const u32 buf = mem::r32(aio + 16);
    const u64 size = mem::r64(aio + 24);
    u32 id;
    s32 error = 0;
    u64 done = 0;
    {
        std::lock_guard<std::mutex> lk(g_fs_mutex);
        id = g_aio_id++;
        auto it = g_handles.find(fd);
        if (it == g_handles.end() || !it->second.file) {
            error = static_cast<s32>(CELL_EBADF);
        } else {
            seek64(it->second.file, static_cast<s64>(offset), SEEK_SET);
            done = write ? std::fwrite(mem::ptr(buf), 1, size, it->second.file)
                         : std::fread(mem::ptr(buf), 1, size, it->second.file);
        }
    }
    if (id_out) mem::w32(id_out, id);
    if (func) hle::call_guest(ctx, func, {aio, static_cast<u64>(static_cast<s64>(error)), id, done});
    ret(ctx, CELL_OK);
}
}  // namespace

OPENTIT_HLE_OK(sys_fs, cellFsAioInit)
OPENTIT_HLE_OK(sys_fs, cellFsAioFinish)
OPENTIT_HLE_OK(sys_fs, cellFsAioCancel)
OPENTIT_HLE(sys_fs, cellFsAioRead) { aio_transfer(ctx, false); }
OPENTIT_HLE(sys_fs, cellFsAioWrite) { aio_transfer(ctx, true); }
