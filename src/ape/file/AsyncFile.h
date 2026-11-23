// openTIT - Ape asynchronous file device (APE_ASYNC_FILE_CMD_*).
//
// On PS3 every disk access of the engine goes through Sony's FIOS I/O
// scheduler: the engine wraps each FIOS operation in a small request object
// (open / close / read / write / get size / prefetch), can either wait on it
// right away (the *Sync helpers) or poll it later, and FIOS transparently
// serves files out of mounted .psarc archives.
//
// Here the scheduler is a single background I/O thread that executes
// requests in submission order against host files (plat::File) or entries of
// mounted PSARC archives. The request API and its error reporting follow the
// original; the error codes keep the console's CELL_FIOS values so the
// engine's code -> name table stays meaningful.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace ape::file {

enum AsyncFileCmd : int32_t {
    kAsyncFileCmd_Invalid  = 0,
    kAsyncFileCmd_Open     = 1,
    kAsyncFileCmd_Close    = 2,
    kAsyncFileCmd_Read     = 3,
    kAsyncFileCmd_Write    = 4,
    kAsyncFileCmd_GetSize  = 5,
    kAsyncFileCmd_Prefetch = 6,
};

enum AsyncFileError : uint32_t {
    kAsyncFileError_None         = 0,
    kAsyncFileError_InProgress   = 0x80010701,
    kAsyncFileError_UnknownEvent = 0x80010702,
    kAsyncFileError_BadPath      = 0x80010705,
    kAsyncFileError_BadHandle    = 0x80010706,
    kAsyncFileError_BadParam     = 0x80010707,
    kAsyncFileError_Cancelled    = 0x80010708,
    kAsyncFileError_EOF          = 0x80010709,
    kAsyncFileError_NoMem        = 0x8001070A,
    kAsyncFileError_BadOffset    = 0x8001070B,
    kAsyncFileError_Access       = 0x8001070E,
    kAsyncFileError_Decompress   = 0x8001070F,
    kAsyncFileError_ReadOnly     = 0x80010710,
    kAsyncFileError_NoPath       = 0x80010712,
};

// Request option flags (the high word of the packed "params" argument the
// PS3 build passes around).
enum AsyncFileFlags : uint32_t {
    kAsyncFileFlag_Hint1    = 0x01, // NOTE: forwarded to FIOS op flags 0x0C; meaning unknown
    kAsyncFileFlag_Hint2    = 0x02, // -> FIOS op flags 0x10
    kAsyncFileFlag_Hint4    = 0x04, // -> FIOS op flags 0x0E
    kAsyncFileFlag_Hint8    = 0x08, // -> FIOS op flags 0x40
    kAsyncFileFlag_Quiet    = 0x10, // do not report errors (used while probing for files)
    kAsyncFileFlag_Async    = 0x20, // open flag 0x100; MountArchive polls instead of blocking
    kAsyncFileFlag_Open200  = 0x40, // open flag 0x200; meaning unknown
};

struct AsyncFileRequest;
using AsyncFileCallback = void (*)(AsyncFileRequest* request);

// Per request options. `urgent` selects the highest scheduling priority.
struct AsyncFileParams {
    uint32_t flags = 0;
    bool urgent = true;
    uint64_t deadline = 0;              // NOTE: carried through, unused by the PC scheduler
    AsyncFileCallback callback = nullptr;
    void* userData = nullptr;
};

// An open file on the device. On PS3 this is a heap cell holding the FIOS
// file handle; it is allocated empty by an open request and filled in when
// the open completes.
class FileStream;
struct AsyncFileHandle {
    AsyncFileHandle();                // 0x00571D80
    ~AsyncFileHandle();               // 0x00571D98
    std::unique_ptr<FileStream> stream;
};

// Scheduler side state of a request (FIOS op). Opaque to callers.
struct AsyncFileOp;

struct AsyncFileRequest {
    AsyncFileOp* op;              // null if the request could not be issued
    AsyncFileCmd cmd;
    // The handle the command works on. An open request owns its new handle
    // until it succeeds; a failed open or a successful close frees it.
    AsyncFileHandle* handle;
    AsyncFileCallback callback;
    void* userData;
    bool reportErrors;            // cleared once the first error was reported
};

// --- request primitives -------------------------------------------------
void InitRequest(AsyncFileRequest* request, AsyncFileHandle* handle, AsyncFileCmd cmd,
                 const AsyncFileParams& params);
void WaitRequest(AsyncFileRequest* request);
void CancelRequest(AsyncFileRequest* request);
uint32_t GetRequestError(const AsyncFileRequest* request);
const char* GetRequestErrorName(const AsyncFileRequest* request);
uint64_t GetRequestTransferred(const AsyncFileRequest* request);
const char* GetRequestPath(const AsyncFileRequest* request);
const char* GetHandlePath(const AsyncFileHandle* handle);
void ReportRequestError(AsyncFileRequest* request);
void WaitRequestAndReport(AsyncFileRequest* request);
void FinalizeRequest(AsyncFileRequest* request);
void ReleaseRequest(AsyncFileRequest* request);
AsyncFileHandle* GetRequestHandle(const AsyncFileRequest* request);
// Releases a request together with its memory.
void DestroyRequest(AsyncFileRequest* request);

// --- asynchronous commands (return null if the request could not start) ---
AsyncFileRequest* AsyncOpen(const char* path, const char* mode, const AsyncFileParams& params);
AsyncFileRequest* AsyncClose(AsyncFileHandle* handle, const AsyncFileParams& params);
AsyncFileRequest* AsyncRead(AsyncFileHandle* handle, void* dst, uint64_t bytes, uint64_t offset,
                            const AsyncFileParams& params);
AsyncFileRequest* AsyncGetSize(AsyncFileHandle* handle, uint64_t* outSize, const AsyncFileParams& params);
AsyncFileRequest* AsyncPrefetch(const char* path, uint64_t offset, uint64_t bytes,
                                const AsyncFileParams& params);

// --- blocking helpers ---------------------------------------------------
AsyncFileHandle* OpenFileSync(const char* path, const char* mode, const AsyncFileParams& params);
void CloseFileSync(AsyncFileHandle* handle, const AsyncFileParams& params);
uint64_t ReadFileSync(AsyncFileHandle* handle, void* dst, uint64_t bytes, uint64_t offset,
                      const AsyncFileParams& params);
uint64_t WriteFileSync(AsyncFileHandle* handle, const void* src, uint64_t bytes, uint64_t offset,
                       const AsyncFileParams& params);
uint64_t GetFileSizeSync(AsyncFileHandle* handle, const AsyncFileParams& params);

// Mounts a .psarc archive so that later opens are served from it.
// `params.flags & kAsyncFileFlag_Async` opens it without blocking the
// system callback pump. Returns the archive's file handle (null on failure).
AsyncFileHandle* MountArchive(const char* path, const AsyncFileParams& params);

} // namespace ape::file
