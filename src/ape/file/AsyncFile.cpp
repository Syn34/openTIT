// openTIT - Ape asynchronous file device (APE_ASYNC_FILE_CMD_*).
#include "ape/file/AsyncFile.h"

#include "ape/core/ApeString.h"
#include "ape/core/ApeSysUtil.h"
#include "ape/file/FileStream.h"
#include "ape/file/Psarc.h"
#include "platform/Platform.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ape::file {

// ---------------------------------------------------------------------------
// Scheduler (stands in for the FIOS scheduler)
// ---------------------------------------------------------------------------

struct AsyncFileOp {
    AsyncFileCmd cmd = kAsyncFileCmd_Invalid;
    AsyncFileRequest* request = nullptr; // for the completion callback
    bool notify = false;

    // Inputs.
    std::string path;
    plat::File::Mode openMode = plat::File::Mode::Read;
    AsyncFileHandle* handle = nullptr;
    void* dst = nullptr;
    const void* src = nullptr;
    uint64_t bytes = 0;
    uint64_t offset = 0;
    uint64_t* outSize = nullptr;

    // Results.
    std::atomic<bool> done{ false };
    std::atomic<bool> cancelled{ false };
    bool settled = false;                 // done and callback dispatched (guarded by the scheduler lock)
    uint32_t error = kAsyncFileError_InProgress;
    uint64_t transferred = 0;
};

namespace {

enum OpEvent : uint8_t {
    kOpEvent_None      = 0,
    kOpEvent_Completed = 1,
    kOpEvent_Deleted   = 2,
};

void OnRequestComplete(AsyncFileRequest* request);

class Scheduler {
public:
    static Scheduler& Get()
    {
        static Scheduler s;
        return s;
    }

    void Submit(AsyncFileOp* op)
    {
        if (std::this_thread::get_id() == m_thread.get_id()) {
            // Issued from a completion callback: run in place, the worker
            // would otherwise wait on itself.
            Execute(op);
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_queue.push_back(op);
        }
        m_wake.notify_all();
    }

    // Waits until the op finished and its completion callback (if any) ran.
    void Wait(AsyncFileOp* op)
    {
        if (std::this_thread::get_id() == m_thread.get_id() && op->done)
            return; // waiting on itself from inside its own callback
        std::unique_lock<std::mutex> lock(m_lock);
        m_finished.wait(lock, [op] { return op->settled; });
    }

    void Delete(AsyncFileOp* op)
    {
        if (!op->done)
            op->cancelled = true;
        Wait(op);
        delete op;
    }

    void AddArchive(std::unique_ptr<PsarcArchive> archive)
    {
        std::lock_guard<std::mutex> lock(m_archiveLock);
        m_archives.push_back(std::move(archive));
    }

    std::unique_ptr<FileStream> OpenFromArchives(const char* path)
    {
        std::lock_guard<std::mutex> lock(m_archiveLock);
        // Most recently mounted archives take precedence.
        for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
            int index = (*it)->Find(path);
            if (index >= 0)
                return (*it)->OpenEntry(index);
        }
        return nullptr;
    }

private:
    Scheduler() : m_thread([this] { Run(); }) {}
    ~Scheduler()
    {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_quit = true;
        }
        m_wake.notify_all();
        m_thread.join();
    }

    void Run()
    {
        for (;;) {
            AsyncFileOp* op;
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_wake.wait(lock, [this] { return m_quit || !m_queue.empty(); });
                if (m_queue.empty())
                    return;
                op = m_queue.front();
                m_queue.pop_front();
            }
            Execute(op);
        }
    }

    void Execute(AsyncFileOp* op)
    {
        uint32_t error = op->cancelled ? kAsyncFileError_Cancelled : Perform(op);
        {
            std::lock_guard<std::mutex> lock(m_lock);
            op->error = error;
            op->done = true;
        }
        // The callback runs before waiters are released, so a waiter cannot
        // free the request underneath it.
        if (op->notify)
            DispatchEvent(op, kOpEvent_Completed);
        {
            std::lock_guard<std::mutex> lock(m_lock);
            op->settled = true;
        }
        m_finished.notify_all();
    }

    uint32_t Perform(AsyncFileOp* op);
    static int DispatchEvent(AsyncFileOp* op, uint8_t event);

    std::mutex m_lock;
    std::condition_variable m_wake;
    std::condition_variable m_finished;
    std::deque<AsyncFileOp*> m_queue;
    bool m_quit = false;

    std::mutex m_archiveLock;
    std::vector<std::unique_ptr<PsarcArchive>> m_archives;

    std::thread m_thread; // last: starts running once everything else exists
};

uint32_t Scheduler::Perform(AsyncFileOp* op)
{
    switch (op->cmd) {
    case kAsyncFileCmd_Open: {
        std::unique_ptr<FileStream> stream;
        if (op->openMode == plat::File::Mode::Read)
            stream = OpenFromArchives(op->path.c_str());
        if (!stream)
            stream = MakeHostStream(plat::File::Open(op->path.c_str(), op->openMode), op->path.c_str());
        if (!stream)
            return kAsyncFileError_BadPath;
        op->handle->stream = std::move(stream);
        return kAsyncFileError_None;
    }
    case kAsyncFileCmd_Close:
        if (!op->handle || !op->handle->stream)
            return kAsyncFileError_BadHandle;
        op->handle->stream.reset();
        return kAsyncFileError_None;
    case kAsyncFileCmd_Read: {
        if (!op->handle || !op->handle->stream)
            return kAsyncFileError_BadHandle;
        FileStream& s = *op->handle->stream;
        op->transferred = s.ReadAt(op->offset, op->dst, (size_t)op->bytes);
        if (!s.Ok())
            return kAsyncFileError_Decompress;
        return op->transferred < op->bytes ? kAsyncFileError_EOF : kAsyncFileError_None;
    }
    case kAsyncFileCmd_Write: {
        if (!op->handle || !op->handle->stream)
            return kAsyncFileError_BadHandle;
        op->transferred = op->handle->stream->WriteAt(op->offset, op->src, (size_t)op->bytes);
        return op->transferred < op->bytes ? kAsyncFileError_Access : kAsyncFileError_None;
    }
    case kAsyncFileCmd_GetSize:
        if (!op->handle || !op->handle->stream)
            return kAsyncFileError_BadHandle;
        if (op->outSize)
            *op->outSize = op->handle->stream->Size();
        return kAsyncFileError_None;
    case kAsyncFileCmd_Prefetch:
        // NOTE: FIOS warms its disk cache here. Host files need no help.
        return kAsyncFileError_None;
    default:
        return kAsyncFileError_BadParam;
    }
}

// Scheduler callback for requests that asked to be notified: forwards the
// completion to the request (FIOS op callback trampoline).
// @func 0x0056D068 AsyncFileOpCallback
int Scheduler::DispatchEvent(AsyncFileOp* op, uint8_t event)
{
    if (event == kOpEvent_Completed) {
        op->request->op = op;
        OnRequestComplete(op->request);
        return 0;
    }
    if (event == kOpEvent_Deleted)
        return 0;
    return (int)kAsyncFileError_UnknownEvent;
}

struct NamedCode {
    uint32_t code;
    const char* name;
};

const char* const kCommandNames[] = {
    "APE_ASYNC_FILE_CMD_INVALID", "APE_ASYNC_FILE_CMD_OPEN",     "APE_ASYNC_FILE_CMD_CLOSE",
    "APE_ASYNC_FILE_CMD_READ",    "APE_ASYNC_FILE_CMD_WRITE",    "APE_ASYNC_FILE_CMD_GET_SIZE",
    "APE_ASYNC_FILE_CMD_PREFETCH",
};

const NamedCode kErrorNames[] = {
    { 0x00000000, "CELL_FIOS_NOERROR" },
    { 0x80010701, "CELL_FIOS_ERROR_INPROGRESS" },
    { 0x80010702, "CELL_FIOS_ERROR_UNKNOWNCALLBACKEVENT" },
    { 0x80010703, "CELL_FIOS_ERROR_NOTREADY" },
    { 0x80010704, "CELL_FIOS_ERROR_TIMEOUT" },
    { 0x80010705, "CELL_FIOS_ERROR_BADPATH" },
    { 0x80010706, "CELL_FIOS_ERROR_BADHANDLE" },
    { 0x80010707, "CELL_FIOS_ERROR_BADPARAM" },
    { 0x80010708, "CELL_FIOS_ERROR_CANCELLED" },
    { 0x80010709, "CELL_FIOS_ERROR_EOF" },
    { 0x8001070A, "CELL_FIOS_ERROR_NOMEM" },
    { 0x8001070B, "CELL_FIOS_ERROR_BADOFFSET" },
    { 0x8001070C, "CELL_FIOS_ERROR_BADCALLBACK" },
    { 0x8001070D, "CELL_FIOS_ERROR_PLATFORM" },
    { 0x8001070E, "CELL_FIOS_ERROR_ACCESS" },
    { 0x8001070F, "CELL_FIOS_ERROR_DECOMPRESSION" },
    { 0x80010710, "CELL_FIOS_ERROR_READONLY" },
    { 0x80010711, "CELL_FIOS_ERROR_NOBLOCK" },
    { 0x80010712, "CELL_FIOS_ERROR_NOPATH" },
    { 0x80010713, "CELL_FIOS_ERROR_MEDIAGONE" },
    { 0x80010714, "CELL_FIOS_ERROR_CANTIMMED" },
    { 0x80029520, "SCE_NP_DRM_ERROR_BUSY" },
    { 0x80029534, "SCE_NP_DRM_ERROR_K_LICENSEE" },
    { 0x80029513, "SCE_NP_DRM_ERROR_NO_ENTITLEMENT" },
};

const char* CommandName(int cmd)
{
    if (cmd >= 0 && cmd < (int)(sizeof(kCommandNames) / sizeof(kCommandNames[0])))
        return kCommandNames[cmd];
    return TempFormat("Unknown:0x%x", (unsigned)cmd);
}

AsyncFileOp* NewOp(AsyncFileRequest* request, AsyncFileCmd cmd, const AsyncFileParams& params)
{
    AsyncFileOp* op = new AsyncFileOp();
    op->cmd = cmd;
    op->request = request;
    op->notify = params.callback != nullptr;
    return op;
}

// Hands a filled in op to the scheduler and attaches it to its request.
bool StartOp(AsyncFileRequest* request, AsyncFileOp* op)
{
    request->op = op;
    Scheduler::Get().Submit(op);
    return true;
}

// Allocates a request; frees it again (with its op) if `issue` fails.
template <typename IssueFn>
AsyncFileRequest* NewRequest(AsyncFileHandle* handle, AsyncFileCmd cmd, const AsyncFileParams& params,
                             IssueFn issue)
{
    AsyncFileRequest* request = new AsyncFileRequest();
    InitRequest(request, handle, cmd, params);
    if (!issue(request)) {
        DestroyRequest(request);
        return nullptr;
    }
    return request;
}

// --- Issue helpers: build the scheduler op for each command -------------

// @func 0x00571178 IssueOpen
bool IssueOpen(AsyncFileRequest* request, AsyncFileHandle* handle, const char* path, const char* mode,
               const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_Open, params);
    op->path = path ? path : "";
    op->handle = handle;
    // Read-only unless the mode lacks 'r'; then truncate, or append for 'a'.
    if (std::strchr(mode, 'r'))
        op->openMode = plat::File::Mode::Read;
    else if (std::strchr(mode, 'a'))
        op->openMode = plat::File::Mode::Append;
    else
        op->openMode = plat::File::Mode::Write;
    // NOTE: kAsyncFileFlag_Async / kAsyncFileFlag_Open200 select FIOS open
    // flags 0x100 / 0x200, which have no host equivalent.
    return StartOp(request, op);
}

// @func 0x00571350 IssueClose
bool IssueClose(AsyncFileHandle* handle, AsyncFileRequest* request, const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_Close, params);
    op->handle = handle;
    return StartOp(request, op);
}

// @func 0x005714B8 IssueRead
bool IssueRead(AsyncFileHandle* handle, AsyncFileRequest* request, void* dst, uint64_t bytes,
               uint64_t offset, const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_Read, params);
    op->handle = handle;
    op->dst = dst;
    op->bytes = bytes;
    op->offset = offset;
    return StartOp(request, op);
}

// @func 0x00571688 IssueWrite
bool IssueWrite(AsyncFileHandle* handle, AsyncFileRequest* request, const void* src, uint64_t bytes,
                uint64_t offset, const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_Write, params);
    op->handle = handle;
    op->src = src;
    op->bytes = bytes;
    op->offset = offset;
    return StartOp(request, op);
}

// The console asks FIOS for the size of the handle's path; we ask the
// stream behind the handle directly.
// @func 0x00571858 IssueGetSize
bool IssueGetSize(AsyncFileHandle* handle, AsyncFileRequest* request, uint64_t* outSize,
                  const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_GetSize, params);
    op->handle = handle;
    op->outSize = outSize;
    if (handle && handle->stream)
        op->path = handle->stream->Path();
    return StartOp(request, op);
}

// @func 0x00571A00 IssuePrefetch
bool IssuePrefetch(AsyncFileRequest* request, const char* path, uint64_t offset, uint64_t bytes,
                   const AsyncFileParams& params)
{
    AsyncFileOp* op = NewOp(request, kAsyncFileCmd_Prefetch, params);
    op->path = path ? path : "";
    op->offset = offset;
    op->bytes = bytes;
    return StartOp(request, op);
}

// Reports the request's error (once) and runs its completion callback.
// @func 0x00561374 AsyncFileRequestComplete
void OnRequestComplete(AsyncFileRequest* request)
{
    ReportRequestError(request);
    request->callback(request);
}

} // namespace

// ---------------------------------------------------------------------------
// Handles
// ---------------------------------------------------------------------------

// @func 0x00571D80 AsyncFileHandle_Init
AsyncFileHandle::AsyncFileHandle() = default;

// @func 0x00571D98 AsyncFileHandle_Destroy
AsyncFileHandle::~AsyncFileHandle() = default;

// ---------------------------------------------------------------------------
// Request primitives
// ---------------------------------------------------------------------------

// @func 0x00570B9C AsyncFileRequest_Init
void InitRequest(AsyncFileRequest* request, AsyncFileHandle* handle, AsyncFileCmd cmd,
                 const AsyncFileParams& params)
{
    request->op = nullptr;
    request->cmd = cmd;
    request->handle = handle;
    request->callback = params.callback;
    request->userData = params.userData;
    request->reportErrors = (params.flags & kAsyncFileFlag_Quiet) == 0;
}

// @func 0x00570BD4 AsyncFileRequest_Wait
void WaitRequest(AsyncFileRequest* request)
{
    if (request->op)
        Scheduler::Get().Wait(request->op);
}

// @func 0x00570C0C AsyncFileRequest_Cancel
void CancelRequest(AsyncFileRequest* request)
{
    if (request->op)
        request->op->cancelled = true;
}

// @func 0x00570C38 AsyncFileRequest_GetError
uint32_t GetRequestError(const AsyncFileRequest* request)
{
    if (!request->op)
        return kAsyncFileError_BadParam;
    if (!request->op->done)
        return kAsyncFileError_InProgress;
    return request->op->error;
}

// @func 0x00570C98 AsyncFileRequest_GetErrorName
const char* GetRequestErrorName(const AsyncFileRequest* request)
{
    const uint32_t error = GetRequestError(request);
    for (const NamedCode& e : kErrorNames)
        if (e.code == error)
            return e.name;
    return TempFormat("Unknown:0x%x", error);
}

// @func 0x00570D54 AsyncFileRequest_GetTransferred
uint64_t GetRequestTransferred(const AsyncFileRequest* request)
{
    return request->op ? request->op->transferred : 0;
}

// @func 0x00570D80 AsyncFileRequest_GetPath
const char* GetRequestPath(const AsyncFileRequest* request)
{
    if (!request->op || request->op->path.empty())
        return nullptr;
    return request->op->path.c_str();
}

// @func 0x00571DA0 AsyncFileHandle_GetPath
const char* GetHandlePath(const AsyncFileHandle* handle)
{
    if (!handle || !handle->stream || handle->stream->Path().empty())
        return nullptr;
    return handle->stream->Path().c_str();
}

// @func 0x005612B8 AsyncFileRequest_ReportError
void ReportRequestError(AsyncFileRequest* request)
{
    if (!request->reportErrors)
        return;
    request->reportErrors = false;
    const uint32_t error = GetRequestError(request);
    if (error == kAsyncFileError_None || error == kAsyncFileError_Cancelled)
        return;
    const char* path = GetRequestPath(request);
    plat::Log(plat::LogLevel::Warning, "%s failed (%s): %s\n", CommandName(request->cmd),
              path ? path : "", GetRequestErrorName(request));
}

// @func 0x00561450 AsyncFileRequest_WaitAndReport
void WaitRequestAndReport(AsyncFileRequest* request)
{
    WaitRequest(request);
    ReportRequestError(request);
}

// Settles the handle ownership of open/close requests: a request still in
// flight is cancelled first, a failed open frees its new handle and a
// successful close frees the closed one.
// @func 0x00561514 AsyncFileRequest_Finalize
void FinalizeRequest(AsyncFileRequest* request)
{
    if (request->cmd != kAsyncFileCmd_Open && request->cmd != kAsyncFileCmd_Close)
        return;
    uint32_t error = GetRequestError(request);
    if (error == kAsyncFileError_InProgress) {
        CancelRequest(request);
        WaitRequestAndReport(request);
        error = GetRequestError(request);
    }
    const bool freeHandle = request->cmd == kAsyncFileCmd_Open ? error != kAsyncFileError_None
                                                              : error == kAsyncFileError_None;
    if (freeHandle && request->handle) {
        delete request->handle;
        request->handle = nullptr;
    }
}

// @func 0x00570DA8 AsyncFileRequest_Release
void ReleaseRequest(AsyncFileRequest* request)
{
    FinalizeRequest(request);
    if (request->op) {
        Scheduler::Get().Delete(request->op);
        request->op = nullptr;
    }
}

void DestroyRequest(AsyncFileRequest* request)
{
    if (!request)
        return;
    ReleaseRequest(request);
    delete request;
}

// @func 0x005616C8 AsyncFileRequest_GetHandle
AsyncFileHandle* GetRequestHandle(const AsyncFileRequest* request)
{
    return GetRequestError(request) == kAsyncFileError_None ? request->handle : nullptr;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

// @func 0x00561714 AsyncFileOpen
AsyncFileRequest* AsyncOpen(const char* path, const char* mode, const AsyncFileParams& params)
{
    AsyncFileHandle* handle = new AsyncFileHandle();
    return NewRequest(handle, kAsyncFileCmd_Open, params, [&](AsyncFileRequest* r) {
        return IssueOpen(r, handle, path, mode, params);
    });
}

// @func 0x005618B8 AsyncFileOpenSync
AsyncFileHandle* OpenFileSync(const char* path, const char* mode, const AsyncFileParams& params)
{
    AsyncFileRequest* request = AsyncOpen(path, mode, params);
    if (!request)
        return nullptr;
    WaitRequestAndReport(request);
    AsyncFileHandle* handle = GetRequestHandle(request);
    DestroyRequest(request);
    return handle;
}

// @func 0x00561B34 AsyncFileClose
AsyncFileRequest* AsyncClose(AsyncFileHandle* handle, const AsyncFileParams& params)
{
    return NewRequest(handle, kAsyncFileCmd_Close, params,
                      [&](AsyncFileRequest* r) { return IssueClose(handle, r, params); });
}

// @func 0x00561C78 AsyncFileCloseSync
void CloseFileSync(AsyncFileHandle* handle, const AsyncFileParams& params)
{
    AsyncFileRequest* request = AsyncClose(handle, params);
    if (!request)
        return;
    WaitRequestAndReport(request);
    DestroyRequest(request);
}

// @func 0x00561E74 AsyncFileRead
AsyncFileRequest* AsyncRead(AsyncFileHandle* handle, void* dst, uint64_t bytes, uint64_t offset,
                            const AsyncFileParams& params)
{
    return NewRequest(handle, kAsyncFileCmd_Read, params, [&](AsyncFileRequest* r) {
        return IssueRead(handle, r, dst, bytes, offset, params);
    });
}

// @func 0x00561FE8 AsyncFileReadSync
uint64_t ReadFileSync(AsyncFileHandle* handle, void* dst, uint64_t bytes, uint64_t offset,
                      const AsyncFileParams& params)
{
    AsyncFileRequest* request = AsyncRead(handle, dst, bytes, offset, params);
    if (!request)
        return 0;
    WaitRequestAndReport(request);
    const uint64_t transferred = GetRequestTransferred(request);
    DestroyRequest(request);
    return transferred;
}

// @func 0x00562230 AsyncFileWriteSync
uint64_t WriteFileSync(AsyncFileHandle* handle, const void* src, uint64_t bytes, uint64_t offset,
                       const AsyncFileParams& params)
{
    AsyncFileRequest* request = NewRequest(handle, kAsyncFileCmd_Write, params, [&](AsyncFileRequest* r) {
        return IssueWrite(handle, r, src, bytes, offset, params);
    });
    if (!request)
        return 0;
    WaitRequestAndReport(request);
    const uint64_t transferred = GetRequestTransferred(request);
    DestroyRequest(request);
    return transferred;
}

// @func 0x00562474 AsyncFileGetSize
AsyncFileRequest* AsyncGetSize(AsyncFileHandle* handle, uint64_t* outSize, const AsyncFileParams& params)
{
    return NewRequest(handle, kAsyncFileCmd_GetSize, params,
                      [&](AsyncFileRequest* r) { return IssueGetSize(handle, r, outSize, params); });
}

// @func 0x005625C8 AsyncFileGetSizeSync
uint64_t GetFileSizeSync(AsyncFileHandle* handle, const AsyncFileParams& params)
{
    uint64_t size = 0;
    AsyncFileRequest* request = AsyncGetSize(handle, &size, params);
    if (!request)
        return 0;
    WaitRequestAndReport(request);
    DestroyRequest(request);
    return size;
}

// @func 0x005627D4 AsyncFilePrefetch
AsyncFileRequest* AsyncPrefetch(const char* path, uint64_t offset, uint64_t bytes,
                                const AsyncFileParams& params)
{
    return NewRequest(nullptr, kAsyncFileCmd_Prefetch, params, [&](AsyncFileRequest* r) {
        return IssuePrefetch(r, path, offset, bytes, params);
    });
}

// ---------------------------------------------------------------------------
// Archive mounting
// ---------------------------------------------------------------------------

// Opens the archive file on the device. With kAsyncFileFlag_Async the open is
// polled while the system callbacks keep being pumped (the boot disc may need
// a system dialog). On PS3 the opened handle is what keeps FIOS's dearchiver
// mounted; here we additionally parse the archive and register it with the
// scheduler.
// @func 0x00571B90 AsyncFileMountArchive
AsyncFileHandle* MountArchive(const char* path, const AsyncFileParams& params)
{
    AsyncFileHandle* handle = nullptr;
    if ((params.flags & kAsyncFileFlag_Async) == 0) {
        handle = OpenFileSync(path, "rb", params);
    } else if (AsyncFileRequest* request = AsyncOpen(path, "rb", params)) {
        while (GetRequestError(request) == kAsyncFileError_InProgress) {
            SysUtilCheckCallbacks();
            plat::SleepUsec(1000);
        }
        WaitRequestAndReport(request);
        handle = GetRequestHandle(request);
        DestroyRequest(request);
    }

    if (handle) {
        std::unique_ptr<PsarcArchive> archive =
            PsarcArchive::Open(plat::File::Open(path, plat::File::Mode::Read), path);
        if (archive)
            Scheduler::Get().AddArchive(std::move(archive));
    }
    return handle;
}

} // namespace ape::file
