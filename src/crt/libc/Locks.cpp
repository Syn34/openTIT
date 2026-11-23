// openTIT - C runtime errno and locking support.
//
// The console library serialises its global state (atexit table, stream
// list, ...) with a small table of recursive "system locks", and every FILE
// carries a lock of its own. Both are built from sys_lwmutex plus an owner
// thread id and a recursion count. On the PC the platform mutex is recursive
// already, and host FILE objects have their own locks.
#include "crt/libc/Libc.h"

#include <atomic>
#include <cerrno>

namespace crt {

// @func 0x0071B560 ErrnoLocation
// Address of the calling thread's errno (the original returns a slot in the
// thread's TLS block).
int* ErrnoLocation()
{
    return &errno;
}

// @func 0x007271C0 MutexInit
// _Mtxinit(): clears the recursion state and creates the underlying mutex.
// The original aborts if the kernel refuses to create the lwmutex.
bool MutexInit(LibMutex* mutex)
{
    mutex->impl = std::make_unique<plat::Mutex>();
    return true;
}

// @func 0x0072716C MutexDestroy
// _Mtxdst().
void MutexDestroy(LibMutex* mutex)
{
    mutex->impl.reset();
}

// @func 0x007270C4 MutexLock
// _Mtxlock(): re-entrant lock. If the calling thread already owns the mutex
// only the recursion count is bumped.
void MutexLock(LibMutex* mutex)
{
    if (mutex->impl)
        mutex->impl->lock();
}

// @func 0x00727080 MutexUnlock
// _Mtxunlock(): drops one level of recursion, releasing the mutex at zero.
void MutexUnlock(LibMutex* mutex)
{
    if (mutex->impl)
        mutex->impl->unlock();
}

namespace {

constexpr int kNumSysLocks = 5;

struct SysLockTable {
    // Number of usable locks; zero until InitSysLocks has run, which turns
    // locking into a no-op during early start-up and late shutdown.
    std::atomic<int> count{0};
    LibMutex locks[kNumSysLocks];
};

SysLockTable& GetSysLocks()
{
    static SysLockTable table;
    return table;
}

} // namespace

// @func 0x00722D68 InitSysLocks
// _Initlocks(): creates the system locks (and, on the console, the locks of
// the three standard streams; host streams bring their own) and enables them.
void InitSysLocks()
{
    SysLockTable& table = GetSysLocks();
    for (LibMutex& lock : table.locks)
        MutexInit(&lock);
    table.count = kNumSysLocks;
}

// @func 0x00722E60 DestroySysLocks
// Disables and destroys the system locks at shutdown.
void DestroySysLocks()
{
    SysLockTable& table = GetSysLocks();
    table.count = 0;
    for (LibMutex& lock : table.locks)
        MutexDestroy(&lock);
}

// @func 0x00722CEC LockSysLock
// _Locksyslock(): takes system lock `index` if the lock table is live.
void LockSysLock(int index)
{
    SysLockTable& table = GetSysLocks();
    if (index >= 0 && index < table.count)
        MutexLock(&table.locks[index]);
}

// @func 0x00722C70 UnlockSysLock
// _Unlocksyslock().
void UnlockSysLock(int index)
{
    SysLockTable& table = GetSysLocks();
    if (index >= 0 && index < table.count)
        MutexUnlock(&table.locks[index]);
}

// @func 0x00722D38 LockFile
// _Lockfilelock(): locks a stream for a multi-step operation (null is ignored).
void LockFile(FILE* stream)
{
    if (!stream)
        return;
#if defined(_WIN32)
    _lock_file(stream);
#else
    flockfile(stream);
#endif
}

// @func 0x00722CBC UnlockFile
// _Unlockfilelock().
void UnlockFile(FILE* stream)
{
    if (!stream)
        return;
#if defined(_WIN32)
    _unlock_file(stream);
#else
    funlockfile(stream);
#endif
}

} // namespace crt
